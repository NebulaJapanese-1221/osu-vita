#include "audio.h"
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#define PLAY_BUFFER_SAMPLES 1024

static int g_bgm_port = -1;
static short *g_bgm_samples = NULL;
static int g_bgm_total_samples = 0;
static int g_bgm_pos = 0;
static int g_bgm_channels = 1;
static bool g_bgm_stop = false;
static SceUID g_bgm_thread = -1;
static wav_t g_bgm_wav;
static bool g_bgm_loaded = false;

static int bgm_thread_func(SceSize args, void *argp) {
	(void)args;
	(void)argp;
	int16_t buf[PLAY_BUFFER_SAMPLES * 2];
	while (!g_bgm_stop) {
		if (g_bgm_pos >= g_bgm_total_samples) {
			g_bgm_pos = 0;
		}
		int remaining = g_bgm_total_samples - g_bgm_pos;
		int to_copy = PLAY_BUFFER_SAMPLES;
		if (to_copy > remaining) to_copy = remaining;
		for (int i = 0; i < to_copy; i++) {
			int16_t s = (int16_t)g_bgm_samples[g_bgm_pos + i];
			if (g_bgm_channels == 2) {
				buf[i * 2] = s;
				buf[i * 2 + 1] = s;
			} else {
				buf[i * 2] = s;
				buf[i * 2 + 1] = s;
			}
		}
		for (int i = to_copy; i < PLAY_BUFFER_SAMPLES; i++) {
			buf[i * 2] = 0;
			buf[i * 2 + 1] = 0;
		}
		g_bgm_pos += to_copy;
		sceAudioOutOutput(g_bgm_port, buf);
	}
	return sceKernelExitDeleteThread(0);
}

static void parse_le_uint16(uint8_t *p, uint16_t *out) {
	*out = (uint16_t)(p[0] | (p[1] << 8));
}

static void parse_le_uint32(uint8_t *p, uint32_t *out) {
	*out = (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

int audio_load_wav(wav_t *wav, const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) return -1;

	uint8_t header[44];
	if (fread(header, 1, 44, f) != 44) {
		fclose(f);
		return -1;
	}
	if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
		fclose(f);
		return -1;
	}

	off_t data_size = 0;
	uint8_t data_buf[16];
	uint8_t data_chunk_id[4];
	while (!feof(f)) {
		if (fread(data_chunk_id, 1, 4, f) != 4) break;
		uint32_t chunk_size;
		parse_le_uint32(data_buf, &chunk_size);
		fread(data_buf, 1, 4, f);
		if (memcmp(data_chunk_id, "data", 4) == 0) {
			uint32_t dsize;
			parse_le_uint32(data_buf, &dsize);
			data_size = dsize;
			break;
		}
		fseek(f, (long)chunk_size, SEEK_CUR);
	}

	uint16_t audio_format, num_channels, bits_per_sample;
	parse_le_uint16(header + 20, &audio_format);
	parse_le_uint16(header + 22, &num_channels);
	uint32_t sample_rate;
	parse_le_uint32(header + 24, &sample_rate);
	parse_le_uint16(header + 32, &bits_per_sample);

	wav->sample_rate = (int)sample_rate;
	wav->channels = (int)num_channels;
	wav->bit_per_sample = (int)bits_per_sample;

	int bytes_per_sample = bits_per_sample / 8;
	int total_samples = (int)(data_size / bytes_per_sample);
	wav->sample_count = total_samples;

	wav->samples = malloc(data_size);
	if (!wav->samples) {
		fclose(f);
		return -1;
	}
	if (fread(wav->samples, 1, data_size, f) != (size_t)data_size) {
		free(wav->samples);
		fclose(f);
		return -1;
	}
	fclose(f);

	if (bytes_per_sample == 1) {
		for (int i = 0; i < total_samples; i++) {
			wav->samples[i] = (short)((wav->samples[i] - 128) * 256);
		}
		wav->bit_per_sample = 16;
	}
	if (bytes_per_sample == 2 && num_channels == 2) {
		short *interleaved = malloc(total_samples * 2);
		for (int i = 0; i < total_samples / 2; i++) {
			interleaved[i] = wav->samples[i * 2];
		}
		free(wav->samples);
		wav->samples = interleaved;
		wav->sample_count = total_samples / 2;
		wav->channels = 1;
	}
	return 0;
}

void audio_free_wav(wav_t *wav) {
	if (wav && wav->samples) {
		free(wav->samples);
		wav->samples = NULL;
	}
}

int audio_init(audio_t *audio) {
	memset(audio, 0, sizeof(*audio));
	audio->bgm_volume = 1.0f;
	audio->initialized = true;
	return 0;
}

void audio_fini(audio_t *audio) {
	audio_stop_bgm(audio);
	memset(audio, 0, sizeof(*audio));
}

int audio_play_bgm(audio_t *audio, const char *path) {
	wav_t wav;
	if (audio_load_wav(&wav, path) != 0) {
		return -1;
	}
	audio_stop_bgm(audio);

	g_bgm_wav = wav;
	g_bgm_loaded = true;
	g_bgm_samples = wav.samples;
	g_bgm_total_samples = wav.sample_count;
	g_bgm_channels = wav.channels;
	g_bgm_pos = 0;
	g_bgm_stop = false;

	int fmt = SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO;
	int mode = SCE_AUDIO_OUT_MODE_STEREO;
	g_bgm_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, PLAY_BUFFER_SAMPLES, wav.sample_rate, mode);
	if (g_bgm_port < 0) {
		audio_free_wav(&g_bgm_wav);
		g_bgm_loaded = false;
		return -1;
	}
	(void)fmt;

	SceUID uid = sceKernelCreateThread("bgm_thread", bgm_thread_func, 0x10, 0x1000, 0, 0, NULL);
	if (uid < 0) {
		sceAudioOutReleasePort(g_bgm_port);
		audio_free_wav(&g_bgm_wav);
		g_bgm_loaded = false;
		return -1;
	}
	sceKernelStartThread(uid, 0, NULL);
	g_bgm_thread = uid;
	audio->playing_bgm = true;
	return 0;
}

void audio_stop_bgm(audio_t *audio) {
	g_bgm_stop = true;
	if (g_bgm_thread >= 0) {
		sceKernelWaitThreadEnd(g_bgm_thread, NULL, 0);
		sceKernelDeleteThread(g_bgm_thread);
		g_bgm_thread = -1;
	}
	if (g_bgm_port >= 0) {
		sceAudioOutReleasePort(g_bgm_port);
		g_bgm_port = -1;
	}
	if (g_bgm_loaded) {
		audio_free_wav(&g_bgm_wav);
		g_bgm_loaded = false;
	}
	g_bgm_samples = NULL;
	audio->playing_bgm = false;
}

void audio_set_bgm_volume(audio_t *audio, float vol) {
	if (vol < 0.0f) vol = 0.0f;
	if (vol > 1.0f) vol = 1.0f;
	audio->bgm_volume = vol;
	if (g_bgm_port >= 0) {
		int vol_left = (int)(SCE_AUDIO_OUT_MAX_VOL * vol);
		int vol_right = (int)(SCE_AUDIO_OUT_MAX_VOL * vol);
		int vols[2] = {vol_left, vol_right};
		sceAudioOutSetVolume(g_bgm_port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vols);
	}
}

static int g_sfx_port = -1;

void audio_play_hit_sound(audio_t *audio, int result) {
	if (!g_sfx_port) {
		g_sfx_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, 512, 44100, SCE_AUDIO_OUT_MODE_MONO);
	}
	if (g_sfx_port < 0) return;

	int freq = 44100;
	float duration_s = 0.12f;
	int n = (int)(freq * duration_s);
	short *samples = malloc(n * sizeof(short));
	if (!samples) return;

	int base_freq;
	switch (result) {
		case HIT_300: base_freq = 660; break;
		case HIT_100: base_freq = 440; break;
		case HIT_50:  base_freq = 220; break;
		default:      base_freq = 110; break;
	}
	int volume = (result == HIT_MISS) ? 3276 : 8192;

	for (int i = 0; i < n; i++) {
		float t = (float)i / (float)freq;
		float env = 1.0f - (t / duration_s);
		env = powf(env, 3.0f);
		float wave = sinf(2.0f * (float)M_PI * base_freq * t) * env * volume;
		samples[i] = (short)wave;
	}
	sceAudioOutOutput(g_sfx_port, samples);
	free(samples);
}
