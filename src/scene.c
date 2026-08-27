#include "scene.h"
#include <string.h>

void scene_init(scene_t *scene) {
	memset(scene, 0, sizeof(*scene));
	scene->id = SCENE_SPLASH;
	scene->active = true;
}

void scene_change(scene_t *scene, scene_id_t new_id) {
	scene->id = new_id;
	scene->enter_time = 0;
	scene->active = true;
}

scene_id_t scene_current(scene_t *scene) {
	return scene->id;
}

int scene_time(scene_t *scene) {
	return scene->enter_time;
}
