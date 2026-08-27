#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include "config.h"

typedef struct {
	bool left, right, up, down;
	bool cross, circle, square, triangle;
	bool l1, r1, l2, r2;
	bool start, select, ps;
	bool dleft, dright, dup, ddown;
} pad_state_t;

typedef struct {
	bool active;
	float x, y;
	float ox, oy;
	int id;
} touch_point_t;

#define MAX_TOUCH_POINTS 2

typedef struct {
	touch_point_t points[MAX_TOUCH_POINTS];
	int count;
} touch_state_t;

void input_init(void);
void input_update(void);

const pad_state_t *input_pad(void);
const pad_state_t *input_pad2(void);
const touch_state_t *input_touch(void);

bool input_just_pressed_cross(void);
bool input_just_released_cross(void);
bool input_pressed_cross(void);

bool input_just_pressed_up(void);
bool input_just_pressed_down(void);
bool input_just_pressed_left(void);
bool input_just_pressed_right(void);

bool input_just_released_square(void);
bool input_just_released_circle(void);

bool input_clicked_in_playfield(vec2_t *out_pos);

#endif
