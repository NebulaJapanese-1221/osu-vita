#ifndef GAME_H
#define GAME_H

#include "config.h"
#include "beatmap.h"
#include "skin.h"
#include "audio.h"
#include "render.h"
#include "input.h"

#define MAX_ACTIVE_OBJECTS 64
#define CURSOR_RADIUS 8.0f

typedef enum {
	OBJ_CIRCLE,
	OBJ_SLIDER,
} active_object_type_t;

typedef struct {
	vec2_t pos;
	float target_radius;
	float approach_scale;
	int start_time;
	int hit_time;
	int type;
	bool active;
	bool hit;
	hit_result_t result;
	int id;
	int combo_color_index;
	active_object_type_t active_type;
	float slider_progress;
	float slider_start_angle;
	vec2_t slider_start_pos;
	bool slider_following;
} active_object_t;

typedef struct {
	beatmap_t beatmap;
	skin_t skin;
	audio_t audio;

	active_object_t objects[MAX_ACTIVE_OBJECTS];
	int active_count;

	int next_object_idx;
	int score;
	int combo;
	int max_combo;
	int accuracy_300;
	int accuracy_100;
	int accuracy_50;
	int misses;
	int largest_combo;

	float time_multiplier;
	int song_offset_ms;
	int game_start_ms;
	int pause_time;
	bool paused;

	vec2_t cursor_pos;

	bool finished;
	int finish_time;
	unsigned int combo_colors[4];
	int current_combo_color;
	vita2d_texture *background;
	bool info_shown;
	bool rank_shown;
	int rank_show_time;
	int next_scene;
} game_state_t;

void game_init(game_state_t *gs, const beatmap_t *bm);
void game_fini(game_state_t *gs);
void game_start(game_state_t *gs);

void game_update(game_state_t *gs, const pad_state_t *pad, const touch_state_t *touch, int dt_ms);
void game_render(game_state_t *gs);

void game_pause(game_state_t *gs);
void game_resume(game_state_t *gs);
void game_hard_reset(game_state_t *gs);

bool game_is_finished(game_state_t *gs);
int game_get_progress_percent(game_state_t *gs);
int game_get_song_position(game_state_t *gs);

#endif
