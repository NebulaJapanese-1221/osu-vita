#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>
#include <vita2d.h>
#include "config.h"

#define COLOR_BLACK   RGBA8(0, 0, 0, 255)
#define COLOR_WHITE   RGBA8(255, 255, 255, 255)
#define COLOR_RED     RGBA8(255, 0, 0, 255)
#define COLOR_GREEN   RGBA8(0, 255, 0, 255)
#define COLOR_BLUE    RGBA8(0, 0, 255, 255)
#define COLOR_YELLOW  RGBA8(255, 255, 0, 255)
#define COLOR_CYAN    RGBA8(0, 255, 255, 255)
#define COLOR_MAGENTA RGBA8(255, 0, 255, 255)
#define COLOR_GREY    RGBA8(128, 128, 128, 255)
#define COLOR_DARKGREY RGBA8(40, 40, 40, 255)

typedef struct {
	vita2d_pgf *font;
} render_t;

void render_init(void);
void render_fini(void);

void render_begin(void);
void render_end(void);

void render_clear(unsigned int color);

void render_fill_rect(float x, float y, float w, float h, unsigned int color);
void render_draw_rect(float x, float y, float w, float h, unsigned int color);

void render_fill_circle(float cx, float cy, float r, unsigned int color);
void render_draw_circle_outline(float cx, float cy, float r, float thickness, unsigned int color);

void render_draw_line(float x0, float y0, float x1, float y1, float thickness, unsigned int color);

void render_draw_ring(float cx, float cy, float inner_r, float outer_r, unsigned int color, int segments);

void render_draw_triangle_fan(vec2_t *verts, int count, unsigned int color);

void render_draw_text(const char *text, int x, int y, unsigned int color, float scale);
int render_text_width(const char *text, float scale);
int render_text_height(float scale);

void render_draw_hit_circle(float x, float y, float radius, unsigned int color, unsigned int outline);

void render_draw_approach_circle(float x, float y, float radius, float fraction, unsigned int color);

void render_draw_progress_bar(float x, float y, float w, float h, float fraction, unsigned int bg, unsigned int fill);

void render_draw_pause_menu(void);
void render_draw_rank_screen(const char *rank, int score, int max_combo, int accuracy300, int accuracy100, int accuracy50, int misses);
void render_draw_info_screen(const char *title, const char *artist, const char *difficulty, int objects);
void render_draw_keymode_overlay(bool k1, bool k2, bool m1, bool m2);

#endif
