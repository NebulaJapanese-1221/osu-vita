#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define SCREEN_W 960
#define SCREEN_H 544

#define PLAYFIELD_W 512
#define PLAYFIELD_H 384

#define PLAYFIELD_X ((SCREEN_W - PLAYFIELD_W) / 2)
#define PLAYFIELD_Y ((SCREEN_H - PLAYFIELD_H) / 2)

#define FADE_IN_DURATION 1000

#define HIT_WINDOW_300 64
#define HIT_WINDOW_100 97
#define HIT_WINDOW_50 127
#define MISSTIMEOUT 75

#define APPROACH_DURATION 1000
#define PREEMPT 600

#define MAX_BEATMAP_OBJECTS 10000
#define MAX_TIMING_POINTS 1000
#define MAX_COMBO 1000

#define DATA_PATH "ux0:/data/osuvita/"
#define MAPS_PATH DATA_PATH "maps/"
#define DOWNLOADS_PATH DATA_PATH "downloads/"

typedef enum {
	SCENE_SPLASH,
	SCENE_MAIN_MENU,
	SCENE_BEATMAP_SELECT,
	SCENE_SETTINGS,
	SCENE_GAME,
	SCENE_RESULTS,
} scene_id_t;

typedef enum {
	HIT_NONE = 0,
	HIT_300,
	HIT_100,
	HIT_50,
	HIT_MISS,
} hit_result_t;

typedef struct {
	float x, y;
} vec2_t;

#endif
