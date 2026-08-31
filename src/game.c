#include "game.h"
#include "beatmap.h"
#include "skin.h"
#include "audio.h"
#include "render.h"
#include "input.h"
#include "main.h"
#include <vita2d.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

static int approach_radius_for_circle(void) {
	int radius = 64;
	return radius;
}

static float slider_angle_for_object(const active_object_t *obj) {
	return obj->slider_start_angle;
}

void game_init(game_state_t *gs, const beatmap_t *bm) {
	memset(gs, 0, sizeof(*gs));
	memcpy(&gs->beatmap, bm, sizeof(beatmap_t));
	skin_init(&gs->skin);
	audio_init(&gs->audio);

	gs->combo_colors[0] = RGBA8(255, 192, 0, 220);
	gs->combo_colors[1] = RGBA8(0, 202, 0, 220);
	gs->combo_colors[2] = RGBA8(18, 124, 255, 220);
	gs->combo_colors[3] = RGBA8(242, 24, 57, 220);
	gs->current_combo_color = 0;
	gs->background = NULL;
	gs->info_shown = true;
	gs->rank_shown = false;
	gs->rank_show_time = 0;
	gs->next_scene = -1;

	gs->time_multiplier = 1.0f;
	gs->next_object_idx = 0;
	gs->score = 0;
	gs->combo = 0;
	gs->max_combo = 0;
	gs->largest_combo = 0;
	gs->accuracy_300 = 0;
	gs->accuracy_100 = 0;
	gs->accuracy_50 = 0;
	gs->misses = 0;
	gs->cursor_pos.x = SCREEN_W / 2.0f;
	gs->cursor_pos.y = SCREEN_H / 2.0f;
	gs->paused = false;
	gs->finished = false;
}

void game_fini(game_state_t *gs) {
	skin_fini(&gs->skin);
	audio_fini(&gs->audio);
	if (gs->background) {
		vita2d_free_texture(gs->background);
		gs->background = NULL;
	}
}

void game_start(game_state_t *gs) {
	gs->next_object_idx = 0;
	gs->active_count = 0;
	gs->score = 0;
	gs->combo = 0;
	gs->max_combo = 0;
	gs->largest_combo = 0;
	gs->accuracy_300 = 0;
	gs->accuracy_100 = 0;
	gs->accuracy_50 = 0;
	gs->misses = 0;
	gs->paused = false;
	gs->finished = false;
	gs->game_start_ms = 0;
	gs->song_offset_ms = -PREEMPT;
	gs->info_shown = true;
	gs->rank_shown = false;
	gs->rank_show_time = 0;

	if (gs->background) {
		vita2d_free_texture(gs->background);
		gs->background = NULL;
	}

	if (gs->beatmap.metadata.background_file[0]) {
		char bg_path[512];
		const char *last_slash = strrchr(gs->beatmap.path, '/');
		if (last_slash) {
			int dir_len = (int)(last_slash - gs->beatmap.path + 1);
			snprintf(bg_path, sizeof(bg_path), "%.*s%s", dir_len, gs->beatmap.path, gs->beatmap.metadata.background_file);
		} else {
			snprintf(bg_path, sizeof(bg_path), "%s", gs->beatmap.metadata.background_file);
		}
		gs->background = vita2d_load_PNG_file(bg_path);
	}

	for (int i = 0; i < MAX_ACTIVE_OBJECTS; i++) {
		gs->objects[i].active = false;
	}

	char bgm_path[256];
	snprintf(bgm_path, sizeof(bgm_path), "%s%s", DATA_PATH, gs->beatmap.metadata.audio_file);
	audio_play_bgm(&gs->audio, bgm_path);

	char skin_path[512];
	snprintf(skin_path, sizeof(skin_path), "%sskin.ini", DATA_PATH);
	skin_load_ini(&gs->skin, skin_path);
}

static float get_slider_position(const hit_object_t *src, float t, vec2_t *out_pos) {
	if (src->point_count == 0) {
		out_pos->x = src->pos.x;
		out_pos->y = src->pos.y;
		return t;
	}
	int total = src->point_count + 1;
	int idx = (int)(t * total);
	if (idx >= total) idx = total - 1;
	if (idx < 0) idx = 0;
	float frac = (t * total) - idx;
	if (idx >= src->point_count) {
		out_pos->x = src->points[src->point_count - 1].x;
		out_pos->y = src->points[src->point_count - 1].y;
	} else {
		out_pos->x = src->pos.x + (src->points[idx].x - src->pos.x) * frac;
		out_pos->y = src->pos.y + (src->points[idx].y - src->pos.y) * frac;
	}
	return t;
}

static active_object_t *spawn_object(game_state_t *gs, int beatmap_idx) {
	if (gs->active_count >= MAX_ACTIVE_OBJECTS) return NULL;
	active_object_t *obj = NULL;
	for (int i = 0; i < MAX_ACTIVE_OBJECTS; i++) {
		if (!gs->objects[i].active) {
			obj = &gs->objects[i];
			break;
		}
	}
	if (!obj) return NULL;

	const hit_object_t *src = &gs->beatmap.objects[beatmap_idx];
	obj->pos.x = src->pos.x;
	obj->pos.y = src->pos.y;
	obj->hit_time = src->time;
	obj->start_time = src->time - PREEMPT;
	obj->target_radius = (float)approach_radius_for_circle();
	obj->type = src->type;
	obj->hit = false;
	obj->result = HIT_NONE;
	obj->active = true;
	obj->id = beatmap_idx;
	obj->combo_color_index = gs->current_combo_color;
	obj->active_type = (src->type == OBJECT_SLIDER) ? OBJ_SLIDER : OBJ_CIRCLE;
	obj->slider_progress = 0.0f;
	obj->slider_start_angle = 0.0f;
	obj->slider_start_pos.x = src->pos.x;
	obj->slider_start_pos.y = src->pos.y;
	obj->slider_following = false;

	if (beatmap_idx > 0 && gs->beatmap.objects[beatmap_idx].combo_offset == 0) {
		gs->current_combo_color = (gs->current_combo_color + 1) % 4;
	}

	return obj;
}

static bool check_hit(game_state_t *gs, active_object_t *obj, vec2_t click_pos) {
	float dx = click_pos.x - obj->pos.x;
	float dy = click_pos.y - obj->pos.y;
	float dist = sqrtf(dx * dx + dy * dy);
	if (dist <= obj->target_radius + 4.0f) {
		if (!obj->hit) {
			int delta = gs->song_offset_ms - obj->hit_time;
			if (delta < 0) delta = -delta;

			hit_result_t result = HIT_MISS;
			if (delta <= HIT_WINDOW_300) {
				result = HIT_300;
				gs->accuracy_300++;
			} else if (delta <= HIT_WINDOW_100) {
				result = HIT_100;
				gs->accuracy_100++;
			} else if (delta <= HIT_WINDOW_50) {
				result = HIT_50;
				gs->accuracy_50++;
			} else if (delta <= MISSTIMEOUT) {
				result = HIT_MISS;
				gs->misses++;
			} else {
				return false;
			}

			obj->hit = true;
			obj->result = result;

			if (result == HIT_MISS) {
				gs->combo = 0;
				gs->current_combo_color = (gs->current_combo_color + 1) % 4;
			} else {
				gs->combo++;
				if (gs->combo > gs->max_combo) gs->max_combo = gs->combo;
				if (gs->combo > gs->largest_combo) gs->largest_combo = gs->combo;

				int score_gain = 0;
				switch (result) {
					case HIT_300: score_gain = 300; break;
					case HIT_100: score_gain = 100; break;
					case HIT_50:  score_gain = 50; break;
					default: break;
				}
				gs->score += score_gain * (1 + gs->combo / 10);
			}
			audio_play_hit_sound(&gs->audio, result);
			return true;
		}
	}
	return false;
}

void game_update(game_state_t *gs, const pad_state_t *pad, const touch_state_t *touch, int dt_ms) {
	if (gs->finished) return;

	gs->game_start_ms += dt_ms;
	gs->song_offset_ms = gs->game_start_ms;

	if (gs->paused) {
		if (input_just_pressed_cross()) {
			game_resume(gs);
		}
		return;
	}

	if (gs->info_shown) {
		if (input_just_pressed_cross()) {
			gs->info_shown = false;
		}
		return;
	}

	if (gs->rank_shown) {
		gs->rank_show_time += dt_ms;
		if (input_just_pressed_cross() && gs->rank_show_time > 500) {
			gs->next_scene = SCENE_BEATMAP_SELECT;
		}
		return;
	}

	if (pad->ps) {
		game_pause(gs);
		return;
	}

	int current_ms = gs->song_offset_ms;

	while (gs->next_object_idx < gs->beatmap.object_count) {
		const hit_object_t *obj = &gs->beatmap.objects[gs->next_object_idx];
		if (obj->time - PREEMPT <= current_ms) {
			if (spawn_object(gs, gs->next_object_idx) == NULL) break;
			gs->next_object_idx++;
		} else {
			break;
		}
	}

	for (int i = 0; i < MAX_ACTIVE_OBJECTS; i++) {
		active_object_t *obj = &gs->objects[i];
		if (!obj->active || obj->hit) continue;

		int elapsed = current_ms - obj->start_time;
		int lifetime = obj->hit_time - obj->start_time + MISSTIMEOUT;

		if (elapsed > lifetime) {
			if (!obj->hit) {
				obj->result = HIT_MISS;
				obj->hit = true;
				obj->active = false;
				gs->misses++;
				gs->combo = 0;
				gs->current_combo_color = (gs->current_combo_color + 1) % 4;
				audio_play_hit_sound(&gs->audio, HIT_MISS);
			}
			continue;
		}

		if (obj->active_type == OBJ_SLIDER && obj->hit) {
			int slider_duration = 200;
			float progress = (float)(current_ms - obj->hit_time) / (float)slider_duration;
			if (progress > 1.0f) progress = 1.0f;
			obj->slider_progress = progress;

			const hit_object_t *src = &gs->beatmap.objects[obj->id];
			get_slider_position(src, progress, &obj->pos);

			if (progress >= 1.0f) {
				obj->active = false;
			}
		}
	}

	if (input_just_pressed_cross() || input_just_released_cross()) {
		vec2_t click_pos;
		if (input_clicked_in_playfield(&click_pos)) {
			gs->cursor_pos = click_pos;
		} else if (pad->dleft || pad->dright || pad->dup || pad->ddown) {
			gs->cursor_pos.x += (pad->dright ? 20 : 0);
			gs->cursor_pos.x -= (pad->dleft ? 20 : 0);
			gs->cursor_pos.y += (pad->ddown ? 20 : 0);
			gs->cursor_pos.y -= (pad->dup ? 20 : 0);
		}
	}

	const touch_state_t *ts = touch;
	if (ts && ts->count > 0) {
		gs->cursor_pos.x = ts->points[0].x;
		gs->cursor_pos.y = ts->points[0].y;
	}

	if (current_ms > 0) {
		for (int i = 0; i < MAX_ACTIVE_OBJECTS; i++) {
			active_object_t *obj = &gs->objects[i];
			if (!obj->active || obj->hit) continue;
			if (obj->active_type == OBJ_SLIDER && obj->hit) continue;

			int delta = current_ms - obj->hit_time;
			if (delta < 0) delta = -delta;
			if (delta > MISSTIMEOUT) continue;

			check_hit(gs, obj, gs->cursor_pos);
		}
	}

	if (gs->next_object_idx >= gs->beatmap.object_count) {
		bool any_active = false;
		for (int i = 0; i < MAX_ACTIVE_OBJECTS; i++) {
			if (gs->objects[i].active) { any_active = true; break; }
		}
		if (!any_active) {
			gs->finished = true;
			gs->finish_time = current_ms;
			gs->rank_shown = true;
			gs->rank_show_time = 0;
		}
	}
}

void game_render(game_state_t *gs) {
	render_clear(COLOR_BLACK);

	if (gs->background) {
		float tex_w = (float)vita2d_texture_get_width(gs->background);
		float tex_h = (float)vita2d_texture_get_height(gs->background);
		float scale_w = SCREEN_W / tex_w;
		float scale_h = SCREEN_H / tex_h;
		float scale = (scale_w > scale_h) ? scale_w : scale_h;
		float draw_w = tex_w * scale;
		float draw_h = tex_h * scale;
		float draw_x = (SCREEN_W - draw_w) / 2.0f;
		float draw_y = (SCREEN_H - draw_h) / 2.0f;
		vita2d_draw_texture_scale(gs->background, draw_x, draw_y, scale, scale);
		vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, RGBA8(0, 0, 0, 120));
	} else {
		vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, RGBA8(20, 20, 20, 255));
	}

	if (gs->info_shown) {
		char buf[256];
		render_draw_info_screen(gs->beatmap.metadata.title, gs->beatmap.metadata.artist, gs->beatmap.metadata.difficulty, gs->beatmap.object_count);
		return;
	}

	if (gs->rank_shown) {
		int total = gs->accuracy_300 + gs->accuracy_100 + gs->accuracy_50 + gs->misses;
		float acc = 0.0f;
		if (total > 0) {
			acc = (gs->accuracy_300 * 100.0f + gs->accuracy_100 * 66.67f + gs->accuracy_50 * 33.33f) / total;
		}
		const char *rank = "D";
		if (acc > 95.0f && gs->misses == 0) rank = "S";
		else if (acc > 90.0f) rank = "A";
		else if (acc > 80.0f) rank = "B";
		else if (acc > 70.0f) rank = "C";
		render_draw_rank_screen(rank, gs->score, gs->max_combo, gs->accuracy_300, gs->accuracy_100, gs->accuracy_50, gs->misses);
		return;
	}

	if (gs->paused) {
		render_draw_pause_menu();
		return;
	}

	render_draw_progress_bar(PLAYFIELD_X, PLAYFIELD_Y + PLAYFIELD_H + 15, PLAYFIELD_W, 6, game_get_progress_percent(gs) / 100.0f, RGBA8(40, 40, 40, 255), RGBA8(0, 200, 255, 255));

	for (int i = 0; i < MAX_ACTIVE_OBJECTS; i++) {
		active_object_t *obj = &gs->objects[i];
		if (!obj->active) continue;

		if (obj->hit && obj->active_type == OBJ_CIRCLE) {
			continue;
		}

		int elapsed = gs->song_offset_ms - obj->start_time;
		int lifetime = obj->hit_time - obj->start_time;
		if (lifetime <= 0) lifetime = APPROACH_DURATION;
		float fraction = (float)elapsed / (float)lifetime;
		if (fraction < 0.0f) fraction = 0.0f;
		if (fraction > 1.0f) fraction = 1.0f;

		float radius = obj->target_radius;
		unsigned int combo_color = gs->combo_colors[obj->combo_color_index];
		unsigned int circle_fill = combo_color;
		unsigned int circle_outline = RGBA8(255, 255, 255, 255);
		int number = (obj->combo_color_index * 8) + obj->id + 1;
		number = ((obj->id % 8) + 1);

		if (obj->active_type == OBJ_SLIDER) {
			const hit_object_t *src = &gs->beatmap.objects[obj->id];
			if (src->point_count > 0 && !obj->hit) {
				for (int p = 0; p < src->point_count; p++) {
					float r = 6.0f;
					render_fill_circle(src->points[p].x, src->points[p].y, r, RGBA8(255, 255, 255, 120));
					render_draw_circle_outline(src->points[p].x, src->points[p].y, r, 1.0f, circle_outline);
				}
			}
			if (obj->hit) {
				vita2d_draw_fill_circle(obj->pos.x, obj->pos.y, 8.0f, combo_color);
				render_draw_circle_outline(obj->pos.x, obj->pos.y, 8.0f, 2.0f, circle_outline);
			} else {
				skin_draw_approach_circle(&gs->skin, obj->pos.x, obj->pos.y, radius * 1.8f + (1.0f - fraction) * radius * 0.8f, combo_color);
				skin_draw_hit_circle(&gs->skin, obj->pos.x, obj->pos.y, radius, circle_fill, circle_outline, number);
			}
		} else {
			skin_draw_approach_circle(&gs->skin, obj->pos.x, obj->pos.y, radius * 1.8f + (1.0f - fraction) * radius * 0.8f, combo_color);
			skin_draw_hit_circle(&gs->skin, obj->pos.x, obj->pos.y, radius, circle_fill, circle_outline, number);
		}
	}

	for (int i = 0; i < MAX_ACTIVE_OBJECTS; i++) {
		active_object_t *obj = &gs->objects[i];
		if (!obj->active || obj->hit) continue;
		if (i > 0 && gs->objects[i - 1].active && !gs->objects[i - 1].hit) {
			active_object_t *prev = &gs->objects[i - 1];
			if (prev->active_type == OBJ_CIRCLE) {
				float dx = obj->pos.x - prev->pos.x;
				float dy = obj->pos.y - prev->pos.y;
				float dist = sqrtf(dx * dx + dy * dy);
				if (dist < 120.0f) {
					float alpha = 1.0f - (dist / 120.0f);
					render_fill_circle((prev->pos.x + obj->pos.x) / 2.0f, (prev->pos.y + obj->pos.y) / 2.0f, 3.0f, RGBA8(255, 255, 255, (unsigned int)(alpha * 180)));
				}
			}
		}
	}

	render_fill_circle(gs->cursor_pos.x, gs->cursor_pos.y, CURSOR_RADIUS, RGBA8(255, 255, 255, 200));
	render_draw_circle_outline(gs->cursor_pos.x, gs->cursor_pos.y, CURSOR_RADIUS + 2, 1.0f, COLOR_CYAN);

	char buf[128];
	snprintf(buf, sizeof(buf), "Score: %d", gs->score);
	render_draw_text(buf, 20, 30, COLOR_WHITE, 1.0f);

	snprintf(buf, sizeof(buf), "Combo: x%d", gs->combo);
	int combo_color = (gs->combo >= 20) ? COLOR_YELLOW : (gs->combo >= 10 ? COLOR_CYAN : COLOR_WHITE);
	render_draw_text(buf, 20, 60, combo_color, 1.2f);

	snprintf(buf, sizeof(buf), "Max: %d", gs->max_combo);
	render_draw_text(buf, 20, 90, COLOR_WHITE, 1.0f);

	render_draw_progress_bar(20, 130, 200, 8, (float)gs->accuracy_300 / 30.0f, RGBA8(30, 30, 30, 255), COLOR_YELLOW);

	render_draw_keymode_overlay(input_pad()->cross, false, false, false);
}

static void game_stop(game_state_t *gs) {
	audio_stop_bgm(&gs->audio);
}

void game_pause(game_state_t *gs) {
	gs->paused = true;
}

void game_resume(game_state_t *gs) {
	gs->paused = false;
}

void game_hard_reset(game_state_t *gs) {
	game_stop(gs);
}

bool game_is_finished(game_state_t *gs) {
	return gs->finished;
}

int game_get_progress_percent(game_state_t *gs) {
	int total = gs->beatmap.total_length;
	if (total <= 0) return 0;
	int pct = gs->song_offset_ms * 100 / total;
	if (pct < 0) pct = 0;
	if (pct > 100) pct = 100;
	return pct;
}

int game_get_song_position(game_state_t *gs) {
	return gs->song_offset_ms;
}
