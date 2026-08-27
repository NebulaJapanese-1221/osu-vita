#include "input.h"
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>

static pad_state_t g_pad[2];
static pad_state_t g_pad_prev[2];
static touch_state_t g_touch;
static touch_state_t g_touch_prev;

void input_init(void) {
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	memset(&g_pad, 0, sizeof(g_pad));
	memset(&g_pad_prev, 0, sizeof(g_pad_prev));
	memset(&g_touch, 0, sizeof(g_touch));
	memset(&g_touch_prev, 0, sizeof(g_touch_prev));
}

static void parse_pad(SceCtrlData *data, pad_state_t *p) {
	uint32_t b = data->buttons;
	p->cross     = (b & SCE_CTRL_CROSS) ? 1 : 0;
	p->circle    = (b & SCE_CTRL_CIRCLE) ? 1 : 0;
	p->square    = (b & SCE_CTRL_SQUARE) ? 1 : 0;
	p->triangle  = (b & SCE_CTRL_TRIANGLE) ? 1 : 0;
	p->l1        = (b & SCE_CTRL_L1) ? 1 : 0;
	p->r1        = (b & SCE_CTRL_R1) ? 1 : 0;
	p->l2        = (b & SCE_CTRL_L2) ? 1 : 0;
	p->r2        = (b & SCE_CTRL_R2) ? 1 : 0;
	p->start     = (b & SCE_CTRL_START) ? 1 : 0;
	p->select    = (b & SCE_CTRL_SELECT) ? 1 : 0;
	p->ps        = (b & SCE_CTRL_PSBUTTON) ? 1 : 0;
	p->dleft     = (b & SCE_CTRL_LEFT) ? 1 : 0;
	p->dright    = (b & SCE_CTRL_RIGHT) ? 1 : 0;
	p->dup       = (b & SCE_CTRL_UP) ? 1 : 0;
	p->ddown     = (b & SCE_CTRL_DOWN) ? 1 : 0;
	p->left      = p->dleft;
	p->right     = p->dright;
	p->up        = p->dup;
	p->down      = p->ddown;
}

void input_update(void) {
	g_pad_prev[0] = g_pad[0];
	g_pad_prev[1] = g_pad[1];

	SceCtrlData data;
	memset(&data, 0, sizeof(data));
	sceCtrlPeekBufferPositive(0, &data, 1);
	parse_pad(&data, &g_pad[0]);

	sceCtrlPeekBufferPositiveExt(0, &data, 1);
	if (data.buttons != 0) {
		parse_pad(&data, &g_pad[1]);
	}

	g_touch_prev = g_touch;
	memset(&g_touch, 0, sizeof(g_touch));
	memset(&g_touch, 0, sizeof(g_touch));

	SceTouchData touch_data;
	memset(&touch_data, 0, sizeof(touch_data));
	if (sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch_data, 1) >= 0) {
		int count = touch_data.reportNum;
		if (count > MAX_TOUCH_POINTS) count = MAX_TOUCH_POINTS;
		g_touch.count = count;
		for (int i = 0; i < count; i++) {
			g_touch.points[i].active = true;
			g_touch.points[i].x = (float)touch_data.report[i].x;
			g_touch.points[i].y = (float)touch_data.report[i].y;
			g_touch.points[i].ox = (float)touch_data.report[i].x;
			g_touch.points[i].oy = (float)touch_data.report[i].y;
			g_touch.points[i].id = touch_data.report[i].id;
		}
	}
}

const pad_state_t *input_pad(void) {
	return &g_pad[0];
}

const pad_state_t *input_pad2(void) {
	return &g_pad[1];
}

const touch_state_t *input_touch(void) {
	return &g_touch;
}

bool input_just_pressed_cross(void) {
	return g_pad[0].cross && !g_pad_prev[0].cross;
}

bool input_just_pressed_up(void) {
	return g_pad[0].dup && !g_pad_prev[0].dup;
}

bool input_just_pressed_down(void) {
	return g_pad[0].ddown && !g_pad_prev[0].ddown;
}

bool input_just_pressed_left(void) {
	return g_pad[0].dleft && !g_pad_prev[0].dleft;
}

bool input_just_pressed_right(void) {
	return g_pad[0].dright && !g_pad_prev[0].dright;
}

bool input_just_released_cross(void) {
	return !g_pad[0].cross && g_pad_prev[0].cross;
}

bool input_pressed_cross(void) {
	return g_pad[0].cross;
}

bool input_just_released_square(void) {
	return !g_pad[0].square && g_pad_prev[0].square;
}

bool input_just_released_circle(void) {
	return !g_pad[0].circle && g_pad_prev[0].circle;
}

bool input_clicked_in_playfield(vec2_t *out_pos) {
	if (!g_touch.count) return false;
	float x = g_touch.points[0].x;
	float y = g_touch.points[0].y;
	if (x >= PLAYFIELD_X && x <= PLAYFIELD_X + PLAYFIELD_W &&
		y >= PLAYFIELD_Y && y <= PLAYFIELD_Y + PLAYFIELD_H) {
		if (out_pos) {
			out_pos->x = x;
			out_pos->y = y;
		}
		return true;
	}
	return false;
}
