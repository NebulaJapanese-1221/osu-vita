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
	if (g_render.font) vita2d_free_font(g_render.font);
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
