#ifndef MAIN_H
#define MAIN_H

#include <vita2d.h>
#include "config.h"
#include "input.h"
#include "scene.h"
#include "beatmap.h"
#include "game.h"

void app_init(void);
void app_fini(void);
int app_run(void);

#endif
