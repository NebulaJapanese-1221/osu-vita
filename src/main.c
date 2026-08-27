#include "main.h"
#include "config.h"
#include "input.h"
#include "scene.h"
#include "render.h"
#include "beatmap.h"
#include "game.h"
#include "audio.h"
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static scene_t g_scene;
static game_state_t g_game;
static beatmap_t g_beatmap;
static bool g_beatmap_loaded = false;

static beatmap_entry_t g_map_entries[MAX_BEATMAP_LIST];
static int g_map_count = 0;
static int g_selected_map = 0;
static int g_menu_scroll = 0;
#define MENU_VISIBLE 10

static audio_t g_audio;

static int g_main_menu_idx = 0;
static int g_settings_menu_idx = 0;
static float g_bgm_volume = 1.0f;

#define MAIN_MENU_COUNT 3
#define SETTINGS_MENU_COUNT 2

static const char *g_main_menu_items[MAIN_MENU_COUNT] = {
	"Play",
	"Settings",
	"Exit",
};

static const char *g_settings_menu_items[SETTINGS_MENU_COUNT] = {
	"BGM Volume",
	"Back",
};

static void load_map_list(void) {
	g_map_count = beatmap_list_dir(g_map_entries, MAX_BEATMAP_LIST, MAPS_PATH);
	if (g_map_count <= 0) {
		g_map_count = 0;
	}
}

static bool load_selected_map(void) {
	if (g_map_count == 0) return false;
	if (beatmap_load(&g_beatmap, g_map_entries[g_selected_map].fullpath) > 0) {
		g_beatmap_loaded = true;
		return true;
	}
	g_beatmap_loaded = false;
	return false;
}

static void scene_splash_update(void) {
	g_scene.enter_time++;
	if (g_scene.enter_time > 180) {
		load_map_list();
		scene_change(&g_scene, SCENE_MAIN_MENU);
	}
}

static void scene_splash_render(void) {
	render_clear(COLOR_BLACK);
	render_draw_text("osu! vita", SCREEN_W / 2 - 60, SCREEN_H / 2 - 20, RGBA8(255, 0, 128, 255), 2.0f);
	render_draw_text("loading...", 20, SCREEN_H - 40, COLOR_WHITE, 1.0f);
}

static void scene_main_menu_update(void) {
	if (input_just_pressed_up()) {
		if (g_main_menu_idx > 0) g_main_menu_idx--;
	}
	if (input_just_pressed_down()) {
		if (g_main_menu_idx < MAIN_MENU_COUNT - 1) g_main_menu_idx++;
	}
	if (input_just_pressed_cross()) {
		switch (g_main_menu_idx) {
			case 0:
				scene_change(&g_scene, SCENE_BEATMAP_SELECT);
				break;
			case 1:
				g_settings_menu_idx = 0;
				scene_change(&g_scene, SCENE_SETTINGS);
				break;
			case 2:
				sceKernelExitProcess(0);
				break;
			default:
				break;
		}
	}
}

static void scene_main_menu_render(void) {
	render_clear(COLOR_BLACK);
	render_draw_text("osu! vita", SCREEN_W / 2 - 60, 80, RGBA8(255, 0, 128, 255), 2.5f);

	for (int i = 0; i < MAIN_MENU_COUNT; i++) {
		int y = 180 + i * 40;
		unsigned int color = (i == g_main_menu_idx) ? COLOR_YELLOW : COLOR_WHITE;
		render_draw_text(g_main_menu_items[i], SCREEN_W / 2 - 50, y, color, 1.2f);
	}
	render_draw_text("UP/DOWN: navigate  CROSS: select", 20, SCREEN_H - 40, COLOR_GREY, 0.8f);
}

static void scene_settings_update(void) {
	if (input_just_pressed_up()) {
		if (g_settings_menu_idx > 0) g_settings_menu_idx--;
	}
	if (input_just_pressed_down()) {
		if (g_settings_menu_idx < SETTINGS_MENU_COUNT - 1) g_settings_menu_idx++;
	}
	if (g_settings_menu_idx == 0) {
		if (input_just_pressed_left()) {
			g_bgm_volume -= 0.1f;
			if (g_bgm_volume < 0.0f) g_bgm_volume = 0.0f;
			audio_set_bgm_volume(&g_audio, g_bgm_volume);
		}
		if (input_just_pressed_right()) {
			g_bgm_volume += 0.1f;
			if (g_bgm_volume > 1.0f) g_bgm_volume = 1.0f;
			audio_set_bgm_volume(&g_audio, g_bgm_volume);
		}
	}
	if (input_just_released_circle() || (input_just_pressed_cross() && g_settings_menu_idx == 1)) {
		scene_change(&g_scene, SCENE_MAIN_MENU);
	}
}

static void scene_settings_render(void) {
	render_clear(COLOR_BLACK);
	render_draw_text("Settings", 20, 30, COLOR_WHITE, 1.2f);

	char buf[128];
	snprintf(buf, sizeof(buf), "BGM Volume: %d%%", (int)(g_bgm_volume * 100.0f));
	unsigned int color = (g_settings_menu_idx == 0) ? COLOR_YELLOW : COLOR_WHITE;
	render_draw_text(buf, 40, 100, color, 1.0f);
	render_draw_text("LEFT/RIGHT: adjust", 40, 130, COLOR_GREY, 0.8f);

	color = (g_settings_menu_idx == 1) ? COLOR_YELLOW : COLOR_WHITE;
	render_draw_text(g_settings_menu_items[1], 40, 180, color, 1.0f);
	render_draw_text("CROSS/CIRCLE: back", 20, SCREEN_H - 40, COLOR_GREY, 0.8f);
}

static void scene_select_update(void) {
	const pad_state_t *pad = input_pad();
	if (input_just_pressed_cross()) {
		if (load_selected_map()) {
			scene_change(&g_scene, SCENE_GAME);
		}
	}
	if (input_just_released_circle()) {
		scene_change(&g_scene, SCENE_MAIN_MENU);
	}
	if (pad->dup) {
		if (g_selected_map > 0) {
			g_selected_map--;
			if (g_selected_map < g_menu_scroll) g_menu_scroll = g_selected_map;
		}
	}
	if (pad->ddown) {
		if (g_selected_map < g_map_count - 1) {
			g_selected_map++;
			if (g_selected_map >= g_menu_scroll + MENU_VISIBLE) g_menu_scroll++;
		}
	}
}

static void scene_select_render(void) {
	render_clear(COLOR_BLACK);
	render_draw_text("Select Beatmap", 20, 30, COLOR_WHITE, 1.2f);
	render_draw_text("(UP/DOWN: browse  CROSS: play  CIRCLE: back)", 20, 60, COLOR_GREY, 0.8f);

	if (g_map_count == 0) {
		render_draw_text("No beatmaps found in maps/", 20, 120, COLOR_RED, 0.9f);
		render_draw_text("Transfer .osu files via FTP", 20, 150, COLOR_GREY, 0.8f);
		return;
	}

	int end = g_menu_scroll + MENU_VISIBLE;
	if (end > g_map_count) end = g_map_count;
	for (int i = g_menu_scroll; i < end; i++) {
		unsigned int color = (i == g_selected_map) ? COLOR_YELLOW : COLOR_WHITE;
		char buf[384];
		const beatmap_entry_t *e = &g_map_entries[i];
		snprintf(buf, sizeof(buf), "%s - %s (%s) [%d★]", e->artist, e->title, e->difficulty, (int)e->overall_difficulty);
		render_draw_text(buf, 30, 100 + (i - g_menu_scroll) * 28, color, 0.85f);
	}
}

static void scene_results_render(void) {
	static bool shown = false;
	if (!shown) {
		render_clear(COLOR_BLACK);
		char buf[256];
		snprintf(buf, sizeof(buf), "Score: %d  Combo: x%d  300:%d 100:%d 50:%d  Miss:%d",
			g_game.score, g_game.max_combo, g_game.accuracy_300, g_game.accuracy_100, g_game.accuracy_50, g_game.misses);
		render_draw_text("Stage Clear!", SCREEN_W / 2 - 90, 100, COLOR_YELLOW, 2.0f);
		render_draw_text(buf, 20, 200, COLOR_WHITE, 1.0f);
		render_draw_text("Press CROSS for menu", 20, 330, COLOR_WHITE, 1.0f);
		shown = true;
	}
	if (input_just_pressed_cross()) {
		game_fini(&g_game);
		shown = false;
		scene_change(&g_scene, SCENE_BEATMAP_SELECT);
	}
}

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	render_init();
	input_init();
	scene_init(&g_scene);
	audio_init(&g_audio);
	vita2d_set_clear_color(COLOR_BLACK);

	sceIoMkdir("ux0:/data/osuvita", 0777);
	sceIoMkdir("ux0:/data/osuvita/maps", 0777);
	sceIoMkdir("ux0:/data/osuvita/downloads", 0777);

	bool running = true;
	while (running) {
		input_update();

		if (scene_current(&g_scene) == SCENE_SPLASH) {
			scene_splash_update();
		}

		switch (scene_current(&g_scene)) {
			case SCENE_MAIN_MENU:
				scene_main_menu_update();
				break;
			case SCENE_SETTINGS:
				scene_settings_update();
				break;
			case SCENE_BEATMAP_SELECT:
				scene_select_update();
				break;
			default:
				break;
		}

		render_begin();

		switch (scene_current(&g_scene)) {
			case SCENE_SPLASH:
				scene_splash_render();
				break;
			case SCENE_MAIN_MENU:
				scene_main_menu_render();
				break;
			case SCENE_SETTINGS:
				scene_settings_render();
				break;
			case SCENE_BEATMAP_SELECT:
				scene_select_render();
				break;
			case SCENE_GAME:
				if (game_is_finished(&g_game)) {
					scene_change(&g_scene, SCENE_RESULTS);
				} else {
					game_update(&g_game, input_pad(), input_touch(), 16);
					game_render(&g_game);
				}
				break;
			case SCENE_RESULTS:
				scene_results_render();
				break;
			default:
				break;
		}

		render_end();

		if (input_pad()->ps) {
			running = false;
		}
	}

	game_fini(&g_game);
	audio_fini(&g_audio);
	render_fini();
	sceKernelExitProcess(0);
	return 0;
}
