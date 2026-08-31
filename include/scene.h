#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>
#include "config.h"

typedef struct {
	scene_id_t current;
	scene_id_t previous;
	int enter_time;
} scene_t;

void scene_init(scene_t *scene);
scene_id_t scene_current(const scene_t *scene);
void scene_change(scene_t *scene, scene_id_t new_scene);
bool scene_is(scene_t *scene, scene_id_t id);

#endif
