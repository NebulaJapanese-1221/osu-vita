#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include "config.h"

typedef struct {
	int port;
	bool initialized;
	bool playing_bgm;
	float bgm_volume;
} audio_t;

typedef struct {
	short *samples;
	int sample_count;
	int sample_rate;
	int channels;
	int bit_per_sample;
} wav_t;

int audio_init(audio_t *audio);
void audio_fini(audio_t *audio);

int audio_load_wav(wav_t *wav, const char *path);
void audio_free_wav(wav_t *wav);

int audio_play_bgm(audio_t *audio, const char *path);
void audio_stop_bgm(audio_t *audio);
void audio_set_bgm_volume(audio_t *audio, float vol);

void audio_play_hit_sound(audio_t *audio, int result);

#endif
