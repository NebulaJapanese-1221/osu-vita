#include "scene.h"
#include <string.h>

void scene_init(scene_t *scene) {
	memset(scene, 0, sizeof(*scene));
	scene->current = SCENE_SPLASH;
}

void scene_change(scene_t *scene, scene_id_t new_scene) {
	scene->previous = scene->current;
	scene->current = new_scene;
	scene->enter_time = 0;
}

scene_id_t scene_current(const scene_t *scene) {
	return scene->current;
}

bool scene_is(scene_t *scene, scene_id_t id) {
	return scene->current == id;
}
