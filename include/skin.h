#ifndef SKIN_H
#define SKIN_H

#include <vita2d.h>
#include "config.h"

#define SKIN_MAX_TEXTURES 16

typedef struct {
	vita2d_texture *hit_circle;
	vita2d_texture *approach_circle;
	vita2d_texture *hit_circle_overlay;
	vita2d_texture *slider_track;
	vita2d_texture *slider_ball;
	vita2d_texture *spinner_circle;
	vita2d_texture *menu_button;
	int tex_count;
	vita2d_texture *textures[SKIN_MAX_TEXTURES];
} skin_t;

void skin_init(skin_t *skin);
void skin_load(skin_t *skin, const char *path);
void skin_fini(skin_t *skin);

vita2d_texture *skin_get_or_fallback(skin_t *skin, int index);
vita2d_texture *skin_get_hit_circle(skin_t *skin);

void skin_draw_hit_circle(skin_t *skin, float x, float y, float radius, unsigned int color, unsigned int outline);
void skin_draw_approach_circle(skin_t *skin, float x, float y, float radius, unsigned int color);
void skin_draw_circle_overlay(skin_t *skin, float x, float y, float radius, unsigned int color);

#endif
