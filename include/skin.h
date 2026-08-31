#ifndef SKIN_H
#define SKIN_H

#include "config.h"

typedef struct {
	int tex_count;
	int cursor_size;
	int hit_circle_overlay_above_number;
} skin_t;

void skin_init(skin_t *skin);
void skin_load(skin_t *skin, const char *path);
void skin_load_ini(skin_t *skin, const char *path);
void skin_fini(skin_t *skin);

void skin_draw_hit_circle(skin_t *skin, float x, float y, float radius, unsigned int color, unsigned int outline, int number);
void skin_draw_approach_circle(skin_t *skin, float x, float y, float radius, unsigned int color);
void skin_draw_circle_overlay(skin_t *skin, float x, float y, float radius, unsigned int color);

#endif
