#include "render.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static render_t g_render;
static vita2d_pgf *g_pgf = NULL;

void render_init(void) {
	vita2d_init();
	g_pgf = vita2d_load_default_pgf();
	g_render.font = NULL;
	vita2d_set_clear_color(COLOR_BLACK);
}

void render_fini(void) {
	if (g_pgf) vita2d_free_pgf(g_pgf);
	vita2d_fini();
}

void render_begin(void) {
	vita2d_start_drawing();
}

void render_end(void) {
	vita2d_end_drawing();
	vita2d_swap_buffers();
}

void render_clear(unsigned int color) {
	vita2d_clear_screen();
	(void)color;
}

void render_fill_rect(float x, float y, float w, float h, unsigned int color) {
	vita2d_draw_rectangle(x, y, w, h, color);
}

void render_draw_rect(float x, float y, float w, float h, unsigned int color) {
	vita2d_draw_line(x, y, x + w, y, color);
	vita2d_draw_line(x + w, y, x + w, y + h, color);
	vita2d_draw_line(x + w, y + h, x, y + h, color);
	vita2d_draw_line(x, y + h, x, y, color);
}

void render_fill_circle(float cx, float cy, float r, unsigned int color) {
	vita2d_draw_fill_circle(cx, cy, r, color);
}

void render_draw_circle_outline(float cx, float cy, float r, float thickness, unsigned int color) {
	render_draw_ring(cx, cy, r - thickness / 2.0f, r + thickness / 2.0f, color, 64);
}

void render_draw_line(float x0, float y0, float x1, float y1, float thickness, unsigned int color) {
	(void)thickness;
	vita2d_draw_line(x0, y0, x1, y1, color);
}

void render_draw_ring(float cx, float cy, float inner_r, float outer_r, unsigned int color, int segments) {
	if (segments < 8) segments = 8;
	float step = (2.0f * (float)M_PI) / (float)segments;
	for (int i = 0; i < segments; i++) {
		float a1 = i * step;
		float a2 = ((i + 1) % segments) * step;
		float x1 = cx + cosf(a1) * inner_r;
		float y1 = cy + sinf(a1) * inner_r;
		float x2 = cx + cosf(a1) * outer_r;
		float y2 = cy + sinf(a1) * outer_r;
		float x3 = cx + cosf(a2) * inner_r;
		float y3 = cy + sinf(a2) * inner_r;
		float x4 = cx + cosf(a2) * outer_r;
		float y4 = cy + sinf(a2) * outer_r;
		vita2d_color_vertex verts[4];
		verts[0].x = x1; verts[0].y = y1; verts[0].z = 0; verts[0].color = color;
		verts[1].x = x2; verts[1].y = y2; verts[1].z = 0; verts[1].color = color;
		verts[2].x = x3; verts[2].y = y3; verts[2].z = 0; verts[2].color = color;
		verts[3].x = x4; verts[3].y = y4; verts[3].z = 0; verts[3].color = color;
		vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, verts, 4);
	}
}

void render_draw_triangle_fan(vec2_t *verts, int count, unsigned int color) {
	if (count < 3) return;
	vita2d_color_vertex *cv = vita2d_pool_malloc(sizeof(vita2d_color_vertex) * count);
	if (!cv) return;
	for (int i = 0; i < count; i++) {
		cv[i].x = verts[i].x;
		cv[i].y = verts[i].y;
		cv[i].z = 0;
		cv[i].color = color;
	}
	vita2d_draw_array(SCE_GXM_PRIMITIVE_TRIANGLE_FAN, cv, count);
}

void render_draw_text(const char *text, int x, int y, unsigned int color, float scale) {
	if (g_pgf) {
		vita2d_pgf_draw_text(g_pgf, x, y, color, scale, text);
	}
}

int render_text_width(const char *text, float scale) {
	if (g_pgf) return vita2d_pgf_text_width(g_pgf, scale, text);
	return (int)(strlen(text) * 8 * scale);
}

int render_text_height(float scale) {
	if (g_pgf) return vita2d_pgf_text_height(g_pgf, scale, "Ag");
	return (int)(16 * scale);
}

void render_draw_hit_circle(float x, float y, float radius, unsigned int color, unsigned int outline) {
	vita2d_draw_fill_circle(x, y, radius, color);
	render_draw_circle_outline(x, y, radius, 1.5f, outline);
	render_draw_circle_outline(x, y, radius * 0.65f, 1.0f, outline);
}

void render_draw_approach_circle(float x, float y, float radius, float fraction, unsigned int color) {
	if (fraction < 0.0f) fraction = 0.0f;
	if (fraction > 1.0f) fraction = 1.0f;
	float inner = radius * (1.0f - fraction) * 0.5f;
	float thickness = 2.5f;
	render_draw_ring(x, y, radius - thickness / 2.0f + inner * 0.3f, radius + thickness / 2.0f + inner * 0.3f, color, 48);
}

void render_draw_progress_bar(float x, float y, float w, float h, float fraction, unsigned int bg, unsigned int fill) {
	if (fraction < 0.0f) fraction = 0.0f;
	if (fraction > 1.0f) fraction = 1.0f;
	vita2d_draw_rectangle(x, y, w, h, bg);
	if (fraction > 0.0f) {
		vita2d_draw_rectangle(x, y, w * fraction, h, fill);
	}
	vita2d_draw_line(x, y, x + w, y, COLOR_WHITE);
	vita2d_draw_line(x, y + h, x + w, y + h, COLOR_WHITE);
	vita2d_draw_line(x, y, x, y + h, COLOR_WHITE);
	vita2d_draw_line(x + w, y, x + w, y + h, COLOR_WHITE);
}

void render_draw_pause_menu(void) {
	float w = 400;
	float h = 220;
	float x = (SCREEN_W - w) / 2;
	float y = (SCREEN_H - h) / 2;
	vita2d_draw_rectangle(x, y, w, h, RGBA8(0, 0, 0, 200));
	render_draw_rect(x, y, w, h, RGBA8(255, 255, 255, 255));

	const char *items[] = {"Resume", "Retry", "Quit"};
	for (int i = 0; i < 3; i++) {
		int ty = (int)(y + 60 + i * 50);
		render_draw_text(items[i], (int)(x + 40), ty, COLOR_WHITE, 1.2f);
	}
	render_draw_text("CROSS: select  CIRCLE: resume", (int)x + 40, (int)(y + h - 40), COLOR_GREY, 0.8f);
}

void render_draw_rank_screen(const char *rank, int score, int max_combo, int accuracy300, int accuracy100, int accuracy50, int misses) {
	vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, RGBA8(0, 0, 0, 220));

	render_draw_text("Stage Clear!", SCREEN_W / 2 - 90, 80, COLOR_YELLOW, 2.0f);
	render_draw_text(rank, SCREEN_W / 2 - 20, 140, COLOR_CYAN, 2.5f);

	char buf[256];
	snprintf(buf, sizeof(buf), "Score: %d", score);
	render_draw_text(buf, SCREEN_W / 2 - 80, 200, COLOR_WHITE, 1.2f);
	snprintf(buf, sizeof(buf), "Combo: x%d", max_combo);
	render_draw_text(buf, SCREEN_W / 2 - 80, 230, COLOR_WHITE, 1.2f);
	snprintf(buf, sizeof(buf), "300:%d 100:%d 50:%d Miss:%d", accuracy300, accuracy100, accuracy50, misses);
	render_draw_text(buf, SCREEN_W / 2 - 120, 260, COLOR_WHITE, 1.0f);

	int total = accuracy300 + accuracy100 + accuracy50 + misses;
	if (total > 0) {
		float acc = (accuracy300 * 100.0f + accuracy100 * 66.67f + accuracy50 * 33.33f) / total;
		snprintf(buf, sizeof(buf), "Accuracy: %.2f%%", acc);
		render_draw_text(buf, SCREEN_W / 2 - 80, 290, COLOR_WHITE, 1.0f);
	}

	render_draw_text("Press CROSS for menu", SCREEN_W / 2 - 110, 340, COLOR_WHITE, 1.0f);
}

void render_draw_info_screen(const char *title, const char *artist, const char *difficulty, int objects) {
	vita2d_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, RGBA8(0, 0, 0, 240));

	char buf[256];
	snprintf(buf, sizeof(buf), "%s", title);
	render_draw_text(buf, SCREEN_W / 2 - render_text_width(buf, 1.5f) / 2, 140, COLOR_WHITE, 1.5f);
	snprintf(buf, sizeof(buf), "%s", artist);
	render_draw_text(buf, SCREEN_W / 2 - render_text_width(buf, 1.0f) / 2, 180, COLOR_GREY, 1.0f);
	snprintf(buf, sizeof(buf), "Difficulty: %s", difficulty);
	render_draw_text(buf, SCREEN_W / 2 - render_text_width(buf, 1.0f) / 2, 210, COLOR_YELLOW, 1.0f);
	snprintf(buf, sizeof(buf), "Objects: %d", objects);
	render_draw_text(buf, SCREEN_W / 2 - render_text_width(buf, 1.0f) / 2, 240, COLOR_WHITE, 1.0f);

	render_draw_text("Press CROSS to start", SCREEN_W / 2 - 110, 320, COLOR_WHITE, 1.0f);
}

void render_draw_keymode_overlay(bool k1, bool k2, bool m1, bool m2) {
	int x = SCREEN_W - 160;
	int y = SCREEN_H - 80;
	unsigned int c1 = k1 ? COLOR_YELLOW : COLOR_GREY;
	unsigned int c2 = k2 ? COLOR_YELLOW : COLOR_GREY;
	unsigned int c3 = m1 ? COLOR_YELLOW : COLOR_GREY;
	unsigned int c4 = m2 ? COLOR_YELLOW : COLOR_GREY;

	render_draw_text("K1", x, y, c1, 1.0f);
	render_draw_text("K2", x + 40, y, c2, 1.0f);
	render_draw_text("M1", x + 80, y, c3, 1.0f);
	render_draw_text("M2", x + 120, y, c4, 1.0f);
}
