#ifndef SCENE_H
#define SCENE_H

#include "config.h"
#include "input.h"

typedef struct {
	scene_id_t id;
	int enter_time;
	bool active;
} scene_t;

void scene_init(scene_t *scene);
void scene_change(scene_t *scene, scene_id_t new_id);

scene_id_t scene_current(scene_t *scene);
int scene_time(scene_t *scene);

#endif
