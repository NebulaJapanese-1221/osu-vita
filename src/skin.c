#include "skin.h"
#include "render.h"
#include <vita2d.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static vita2d_texture *g_fallback_circle = NULL;
static vita2d_texture *g_fallback_approach = NULL;

static vita2d_texture *create_solid_texture(unsigned int color, int w, int h) {
	vita2d_texture *tex = vita2d_create_empty_texture(w, h);
	if (!tex) return NULL;
	void *data = vita2d_texture_get_datap(tex);
	uint32_t *pixels = (uint32_t *)data;
	for (int i = 0; i < w * h; i++) pixels[i] = color;
	return tex;
}

void skin_init(skin_t *skin) {
	memset(skin, 0, sizeof(*skin));
}

void skin_load(skin_t *skin, const char *path) {
	char full[512];
	snprintf(full, sizeof(full), "%s/skin/hitcircle.png", path);
	skin->hit_circle = vita2d_load_PNG_file(full);
	snprintf(full, sizeof(full), "%s/skin/approachcircle.png", path);
	skin->approach_circle = vita2d_load_PNG_file(full);
	snprintf(full, sizeof(full), "%s/skin/hitcircleoverlay.png", path);
	skin->hit_circle_overlay = vita2d_load_PNG_file(full);
	snprintf(full, sizeof(full), "%s/skin/slidertrack.png", path);
	skin->slider_track = vita2d_load_PNG_file(full);
	snprintf(full, sizeof(full), "%s/skin/sliderball.png", path);
	skin->slider_ball = vita2d_load_PNG_file(full);
	for (int i = 0; i < SKIN_MAX_TEXTURES && skin->textures[i]; i++) {
		skin->tex_count++;
	}
}

void skin_fini(skin_t *skin) {
	for (int i = 0; i < skin->tex_count && i < SKIN_MAX_TEXTURES; i++) {
		if (skin->textures[i]) {
			vita2d_free_texture(skin->textures[i]);
			skin->textures[i] = NULL;
		}
	}
	if (g_fallback_circle) vita2d_free_texture(g_fallback_circle);
	if (g_fallback_approach) vita2d_free_texture(g_fallback_approach);
	skin->hit_circle = NULL;
	skin->approach_circle = NULL;
	memset(skin, 0, sizeof(*skin));
}

vita2d_texture *skin_get_or_fallback(skin_t *skin, int index) {
	if (index < 0 || index >= SKIN_MAX_TEXTURES) return NULL;
	return skin->textures[index];
}

vita2d_texture *skin_get_hit_circle(skin_t *skin) {
	if (skin->hit_circle) return skin->hit_circle;
	if (!g_fallback_circle) {
		g_fallback_circle = create_solid_texture(RGBA8(255, 255, 255, 255), 64, 64);
	}
	return g_fallback_circle;
}

void skin_draw_hit_circle(skin_t *skin, float x, float y, float radius, unsigned int color, unsigned int outline) {
	if (skin->hit_circle) {
		float w = radius * 2;
		vita2d_draw_texture_tint_scale(skin->hit_circle, x - radius, y - radius, w / 64.0f, w / 64.0f, color);
		if (skin->hit_circle_overlay) {
			vita2d_draw_texture_tint_scale(skin->hit_circle_overlay, x - radius, y - radius, w / 64.0f, w / 64.0f, RGBA8(255, 255, 255, 255));
		}
	} else {
		render_draw_hit_circle(x, y, radius, color, outline);
	}
}

void skin_draw_approach_circle(skin_t *skin, float x, float y, float radius, unsigned int color) {
	if (skin->approach_circle) {
		float w = radius * 2;
		vita2d_draw_texture_tint_scale(skin->approach_circle, x - radius, y - radius, w / 64.0f, w / 64.0f, color);
	} else {
		vita2d_draw_fill_circle(x, y, 1.0f, color);
		render_draw_ring(x, y, radius - 2.0f, radius + 2.0f, color, 64);
	}
}

void skin_draw_circle_overlay(skin_t *skin, float x, float y, float radius, unsigned int color) {
	if (skin->hit_circle_overlay) {
		float w = radius * 1.0f;
		vita2d_draw_texture_tint_scale(skin->hit_circle_overlay, x - w / 2, y - w / 2, w / 64.0f, w / 64.0f, color);
	}
}
