#include "beatmap.h"
#include <psp2/io/dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char *read_file(const char *path, int *size_out) {
	FILE *f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	int size = (int)ftell(f);
	fseek(f, 0, SEEK_SET);
	char *buf = malloc(size + 1);
	if (!buf) { fclose(f); return NULL; }
	if (fread(buf, 1, size, f) != (size_t)size) {
		free(buf); fclose(f); return NULL;
	}
	buf[size] = '\0';
	fclose(f);
	if (size_out) *size_out = size;
	return buf;
}

static int parse_int(char *s, int def) {
	char *end = NULL;
	int v = (int)strtol(s, &end, 10);
	return (end == s) ? def : v;
}

static float parse_float(char *s, float def) {
	char *end = NULL;
	float v = strtof(s, &end);
	return (end == s) ? def : v;
}

static char *find_section(char *data, const char *section) {
	char needle[64];
	snprintf(needle, sizeof(needle), "[%s]", section);
	char *p = strstr(data, needle);
	return p;
}

static char *next_line(char *p) {
	char *nl = strchr(p, '\n');
	if (!nl) { p = p + strlen(p); return p; }
	return nl + 1;
}

static void trim(char *s) {
	while (*s == '\r' || *s == '\n' || *s == ' ' || *s == '\t') *s = '\0';
}

int beatmap_parse(beatmap_t *bm, const char *data, int size) {
	(void)size;
	memset(bm, 0, sizeof(*bm));
	strcpy(bm->metadata.title, "Unknown");
	strcpy(bm->metadata.audio_file, "bgm.wav");
	strcpy(bm->metadata.background_file, "");

	char *buf = (char *)data;

	char *general = find_section(buf, "General");
	if (general) {
		char *line = next_line(general);
		char *section_end = find_section(line, "Editor");
		if (!section_end) section_end = find_section(line, "Metadata");
		while (line < section_end && *line) {
			char *eq = strchr(line, ':');
			if (!eq) { line = next_line(line); continue; }
			*eq = '\0';
			char *val = eq + 1;
			while (*val == ' ') val++;
			if (strcmp(line, "AudioFilename") == 0) {
				strncpy(bm->metadata.audio_file, val, sizeof(bm->metadata.audio_file) - 1);
			}
			line = next_line(line);
		}
	}

	char *events = find_section(buf, "Events");
	if (events) {
		char *line = next_line(events);
		char *section_end = find_section(line, "TimingPoints");
		if (!section_end) section_end = find_section(line, "HitObjects");
		if (!section_end) section_end = buf + size;
		while (line < section_end && *line) {
			trim(line);
			if (*line == '\0') { line = next_line(line); continue; }
			if (line[0] == '/' && line[1] == '/') { line = next_line(line); continue; }
			if (line[0] == ' ') { line = next_line(line); continue; }
			
			int event_type = parse_int(line, -1);
			if (event_type == 0) {
				char *p1 = strchr(line, ',') + 1;
				(void)p1;
				char *p2 = strchr(p1, ',') + 1;
				if (p2) {
					char *start = strchr(p2, '"');
					if (start) {
						char *end = strchr(start + 1, '"');
						if (end) {
							int len = (int)(end - start - 1);
							if (len >= (int)sizeof(bm->metadata.background_file)) len = sizeof(bm->metadata.background_file) - 1;
							strncpy(bm->metadata.background_file, start + 1, len);
							bm->metadata.background_file[len] = '\0';
						}
					}
				}
			}
			line = next_line(line);
		}
	}

	char *diff = find_section(buf, "Difficulty");
	if (diff) {
		char *line = next_line(diff);
		char *section_end = find_section(line, "Events");
		if (!section_end) section_end = find_section(line, "TimingPoints");
		while (line < section_end && *line) {
			char *eq = strchr(line, ':');
			if (!eq) { line = next_line(line); continue; }
			*eq = '\0';
			char *val = eq + 1;
			while (*val == ' ') val++;
			if (strcmp(line, "HPDrainRate") == 0) bm->metadata.hp_drain = parse_float(val, 6);
			else if (strcmp(line, "CircleSize") == 0) bm->metadata.circle_size = parse_int(val, 4);
			else if (strcmp(line, "OverallDifficulty") == 0) bm->metadata.overall_difficulty = parse_float(val, 5);
			else if (strcmp(line, "ApproachRate") == 0) bm->metadata.approach_rate = parse_int(val, 5);
			line = next_line(line);
		}
	}

	char *tp = find_section(buf, "TimingPoints");
	if (tp) {
		char *line = next_line(tp);
		char *section_end = find_section(line, "HitObjects");
		if (!section_end) section_end = buf + size;
		while (line < section_end && *line && bm->timing_count < MAX_TIMING_POINTS) {
			trim(line);
			if (*line == '\0') { line = next_line(line); continue; }
			char *comma = strchr(line, ',');
			if (!comma) { line = next_line(line); continue; }
			float offset = parse_float(line, 0.0f);
			char *p2 = comma + 1;
			float ms_per_beat = parse_float(p2, 500.0f);
			timing_point_t *t = &bm->timing[bm->timing_count++];
			t->offset = offset;
			t->ms_per_beat = ms_per_beat;
			t->inherited = false;
			t->uninherited = true;
			line = next_line(line);
		}
	}

	char *ho = find_section(buf, "HitObjects");
	if (ho) {
		char *line = next_line(ho);
		int total_len = (int)strlen(buf);
		int combo_counter = 0;
		while (line < buf + total_len && *line && bm->object_count < MAX_BEATMAP_OBJECTS) {
			trim(line);
			if (*line == '\0') { line = next_line(line); continue; }
			int x = parse_int(line, 0);
			char *p1 = strchr(line, ',') + 1;
			int y = parse_int(p1, 0);
			char *p2 = strchr(p1, ',') + 1;
			int time = parse_int(p2, 0);
			char *p3 = strchr(p2, ',') + 1;
			int type = parse_int(p3, 0);
			char *p4 = strchr(p3, ',') + 1;
			int hit_sound = parse_int(p4, 0);

			hit_object_t *o = &bm->objects[bm->object_count];
			o->pos.x = (float)x;
			o->pos.y = (float)y;
			o->time = time;
			o->type = type;
			o->hit_sound = hit_sound;
			o->combo_offset = combo_counter % 8;
			o->slides = 1;
			o->slider_length = 0.0f;
			o->point_count = 0;

			if (type == OBJECT_SLIDER) {
				char *p5 = strchr(p4, ',') + 1;
				char *curve_type = p5;
				char *p6 = strchr(p5, '|');
				if (p6) {
					*p6 = '\0';
					p6++;
					char *p7 = strrchr(p6, ',');
					if (p7) {
						*p7 = '\0';
						char *slider_len_str = p7 + 1;
						o->slider_length = parse_float(slider_len_str, 0.0f);
						char *slides_str = strchr(p7, ',') + 1;
						o->slides = parse_int(slides_str, 1);
					}
					while (*p6 && o->point_count < MAX_SLIDER_POINTS) {
						int px = parse_int(p6, 0);
						char *py_str = strchr(p6, ':') + 1;
						int py = parse_int(py_str, 0);
						o->points[o->point_count].x = (float)px;
						o->points[o->point_count].y = (float)py;
						o->points[o->point_count].type = 1;
						o->point_count++;
						p6 = strchr(py_str, ',');
						if (p6) p6++;
						else break;
					}
				} else {
					char *p7 = strchr(p5, ',');
					if (p7) {
						*p7 = '\0';
						o->slider_length = parse_float(p7 + 1, 0.0f);
						char *slides_str = strchr(p7 + 1, ',') + 1;
						o->slides = parse_int(slides_str, 1);
					}
				}
			}

			combo_counter++;
			bm->object_count++;
			line = next_line(line);
		}
	}

	bm->total_length = 0;
	if (bm->object_count > 0) {
		bm->total_length = bm->objects[bm->object_count - 1].time + 1000;
	}
	return bm->object_count;
}

int beatmap_load(beatmap_t *bm, const char *path) {
	int size = 0;
	char *data = read_file(path, &size);
	if (!data) return -1;
	int ret = beatmap_parse(bm, data, size);
	free(data);
	if (ret > 0) {
		strncpy(bm->path, path, sizeof(bm->path) - 1);
		bm->path[sizeof(bm->path) - 1] = '\0';
	}
	return ret;
}

#include "sample_beatmap.h"

const char *beatmap_sample_data(void) {
	return SAMPLE_BEATMAP_DATA;
}

int beatmap_list_dir(beatmap_entry_t *entries, int max, const char *dir) {
	SceUID uid = sceIoDopen(dir);
	if (uid < 0) return 0;

	int count = 0;
	SceIoDirent dirent;
	while (sceIoDread(uid, &dirent) > 0) {
		if (dirent.d_name[0] == '.') continue;

		char *name = dirent.d_name;
		char *ext = strrchr(name, '.');
		if (!ext) continue;
		if (strcasecmp(ext, ".osu") != 0) continue;

		if (count >= max) break;

		beatmap_entry_t *e = &entries[count];
		memset(e, 0, sizeof(*e));
		strncpy(e->filename, name, sizeof(e->filename) - 1);
		snprintf(e->fullpath, sizeof(e->fullpath), "%s%s", dir, name);
		e->circle_size = 4;
		e->overall_difficulty = 5.0f;
		e->approach_rate = 5;

		beatmap_entry_parse_meta(e, e->fullpath);
		count++;
	}
	sceIoDclose(uid);
	return count;
}

int beatmap_entry_parse_meta(beatmap_entry_t *entry, const char *path) {
	int size = 0;
	char *data = read_file(path, &size);
	if (!data) return -1;

	char *meta = find_section(data, "Metadata");
	if (meta) {
		char *line = next_line(meta);
		char *end = find_section(line, "Difficulty");
		if (!end) end = data + size;
		while (line < end && *line) {
			char *eq = strchr(line, ':');
			if (!eq) { line = next_line(line); continue; }
			*eq = '\0';
			char *val = eq + 1;
			while (*val == ' ' || *val == '\t') val++;
			trim(val);
			if (strcmp(line, "Title") == 0) {
				strncpy(entry->title, val, sizeof(entry->title) - 1);
			} else if (strcmp(line, "Artist") == 0) {
				strncpy(entry->artist, val, sizeof(entry->artist) - 1);
			} else if (strcmp(line, "Creator") == 0) {
				strncpy(entry->creator, val, sizeof(entry->creator) - 1);
			} else if (strcmp(line, "Version") == 0) {
				strncpy(entry->difficulty, val, sizeof(entry->difficulty) - 1);
			}
			line = next_line(line);
		}
	}

	char *diff = find_section(data, "Difficulty");
	if (diff) {
		char *line = next_line(diff);
		char *end = find_section(line, "Events");
		if (!end) end = data + size;
		while (line < end && *line) {
			char *eq = strchr(line, ':');
			if (!eq) { line = next_line(line); continue; }
			*eq = '\0';
			char *val = eq + 1;
			while (*val == ' ') val++;
			if (strcmp(line, "CircleSize") == 0) entry->circle_size = parse_int(val, 4);
			else if (strcmp(line, "ApproachRate") == 0) entry->approach_rate = (int)parse_float(val, 5.0f);
			else if (strcmp(line, "OverallDifficulty") == 0) entry->overall_difficulty = parse_float(val, 5.0f);
			line = next_line(line);
		}
	}

	free(data);
	return 0;
}
