#ifndef BEATMAP_H
#define BEATMAP_H

#include "config.h"

typedef enum {
	OBJECT_CIRCLE = 1,
	OBJECT_SLIDER = 2,
	OBJECT_SPINNER = 8,
} object_type_t;

typedef struct {
	vec2_t pos;
	int time;
	int type;
	int hit_sound;
} hit_object_t;

typedef struct {
	float offset;
	float ms_per_beat;
	bool inherited;
	bool uninherited;
} timing_point_t;

typedef struct {
	char title[256];
	char artist[256];
	char creator[128];
	char difficulty[64];
	char audio_file[128];
	int hp_drain;
	int circle_size;
	int approach_rate;
	float overall_difficulty;
	int difficulty_multiplier;
	int grid_size;
	int countdown_offset;
} beatmap_metadata_t;

typedef struct {
	beatmap_metadata_t metadata;
	hit_object_t objects[MAX_BEATMAP_OBJECTS];
	int object_count;
	timing_point_t timing[MAX_TIMING_POINTS];
	int timing_count;
	int total_length;
	int offset;
} beatmap_t;

int beatmap_parse(beatmap_t *bm, const char *data, int size);
int beatmap_load(beatmap_t *bm, const char *path);

#define MAX_BEATMAP_LIST 64

typedef struct {
	char title[256];
	char artist[256];
	char creator[128];
	char difficulty[64];
	char filename[256];
	char fullpath[512];
	int circle_size;
	int approach_rate;
	float overall_difficulty;
} beatmap_entry_t;

int beatmap_list_dir(beatmap_entry_t *entries, int max, const char *dir);
int beatmap_entry_parse_meta(beatmap_entry_t *entry, const char *path);

const char *beatmap_sample_data(void);

#endif
