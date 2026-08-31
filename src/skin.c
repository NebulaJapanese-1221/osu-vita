#include "skin.h"
#include "render.h"
#include <vita2d.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void skin_init(skin_t *skin) {
	memset(skin, 0, sizeof(*skin));
	skin->cursor_size = 24;
	skin->hit_circle_overlay_above_number = 0;
}

static char *find_line(const char *data, const char *key) {
	char needle[128];
	snprintf(needle, sizeof(needle), "%s:", key);
	const char *p = strstr(data, needle);
	if (!p) return NULL;
	p += strlen(needle);
	while (*p == ' ' || *p == '\t') p++;
	return (char *)p;
}

void skin_load(skin_t *skin, const char *path) {
	(void)path;
}

void skin_load_ini(skin_t *skin, const char *path) {
	int size = 0;
	FILE *f = fopen(path, "rb");
	if (!f) return;
	fseek(f, 0, SEEK_END);
	size = (int)ftell(f);
	fseek(f, 0, SEEK_SET);
	char *data = malloc(size + 1);
	if (!data) { fclose(f); return; }
	fread(data, 1, size, f);
	data[size] = '\0';
	fclose(f);

	char *line = strstr(data, "[Colours]");
	if (line) {
		line = strchr(line, '\n');
		if (!line) line = strchr(line, '\r');
		if (line) line++;
		char *end = strstr(line, "\n[");
		if (!end) end = data + size;
		while (line < end && *line) {
			char *nl = strchr(line, '\n');
			if (!nl) nl = data + size;
			char buf[256];
			int len = (int)(nl - line);
			if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;
			memcpy(buf, line, len);
			buf[len] = '\0';

			char *eq = strchr(buf, ':');
			if (eq) {
				*eq = '\0';
				char *key = buf;
				while (*key == ' ' || *key == '\t') key++;
				char *val = eq + 1;
				while (*val == ' ' || *val == '\t') val++;

				if (strcmp(key, "CursorSize") == 0) {
					skin->cursor_size = atoi(val);
					if (skin->cursor_size < 8) skin->cursor_size = 8;
					if (skin->cursor_size > 64) skin->cursor_size = 64;
				} else if (strcmp(key, "HitCircleOverlayAboveNumber") == 0) {
					skin->hit_circle_overlay_above_number = atoi(val);
				}
			}
			line = nl + 1;
		}
	}

	free(data);
}

void skin_fini(skin_t *skin) {
	(void)skin;
}

vita2d_texture *skin_get_or_fallback(skin_t *skin, int index) {
	(void)skin;
	(void)index;
	return NULL;
}

vita2d_texture *skin_get_hit_circle(skin_t *skin) {
	(void)skin;
	return NULL;
}

void skin_draw_hit_circle(skin_t *skin, float x, float y, float radius, unsigned int color, unsigned int outline, int number) {
	(void)skin;
	render_draw_hit_circle(x, y, radius, color, outline);

	char buf[8];
	snprintf(buf, sizeof(buf), "%d", number);
	float scale = radius / 48.0f;
	int w = render_text_width(buf, scale);
	int h = render_text_height(scale);
	render_draw_text(buf, (int)(x - w / 2), (int)(y + h / 3), COLOR_WHITE, scale);
}

void skin_draw_approach_circle(skin_t *skin, float x, float y, float radius, unsigned int color) {
	(void)skin;
	vita2d_draw_fill_circle(x, y, 1.0f, color);
	render_draw_ring(x, y, radius - 2.0f, radius + 2.0f, color, 64);
}

void skin_draw_circle_overlay(skin_t *skin, float x, float y, float radius, unsigned int color) {
	(void)skin;
	render_draw_circle_outline(x, y, radius, 1.5f, color);
}
