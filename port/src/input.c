#include "input.h"
#include "gb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const button_names[8] = {"right", "left", "up", "down", "a", "b", "select", "start"};

static void set_buttons(InputScript *s, int from, int to, uint8_t b)
{
	if (from < 0)
		from = 0;
	if (to > s->len) {
		s->buttons = realloc(s->buttons, (size_t)to);
		memset(s->buttons + s->len, 0, (size_t)(to - s->len));
		s->len = to;
	}
	for (int i = from; i < to; i++)
		s->buttons[i] |= b;
}

static char *resolve_relative(const char *script, const char *path)
{
	bool absolute = path[0] == '/' || path[0] == '\\' || (path[0] && path[1] == ':');
	const char *slash = strrchr(script, '/');
	const char *bslash = strrchr(script, '\\');
	if (bslash > slash)
		slash = bslash;
	size_t dir = (absolute || !slash) ? 0 : (size_t)(slash - script + 1);
	char *out = malloc(dir + strlen(path) + 1);
	memcpy(out, script, dir);
	strcpy(out + dir, path);
	return out;
}

bool input_load(InputScript *s, const char *path)
{
	memset(s, 0, sizeof(*s));
	FILE *f = fopen(path, "r");
	if (!f)
		return false;
	char line[512];
	int lineno = 0;
	while (fgets(line, sizeof(line), f)) {
		lineno++;
		line[strcspn(line, "\r\n")] = 0;
		char *p = line + strspn(line, " \t");
		if (!*p || *p == '#')
			continue;
		if (!strncmp(p, "state ", 6)) {
			p += 6 + strspn(p + 6, " \t");
			free(s->state_path);
			s->state_path = resolve_relative(path, p);
			continue;
		}
		if (!strncmp(p, "end ", 4)) {
			s->end = atoi(p + 4);
			continue;
		}
		int frame, duration, n;
		if (sscanf(p, "%d %d %n", &frame, &duration, &n) != 2) {
			fprintf(stderr, "%s:%d: can't parse '%s'\n", path, lineno, p);
			continue;
		}
		uint8_t b = 0;
		for (char *t = strtok(p + n, " +,\t"); t; t = strtok(NULL, " +,\t")) {
			int i;
			for (i = 0; i < 8; i++) {
				if (!strcmp(t, button_names[i])) {
					b |= (uint8_t)(1 << i);
					break;
				}
			}
			if (i == 8)
				fprintf(stderr, "%s:%d: unknown button '%s'\n", path, lineno, t);
		}
		set_buttons(s, frame, frame + duration, b);
	}
	fclose(f);
	return true;
}

uint8_t input_buttons(const InputScript *s, int frame)
{
	return frame >= 0 && frame < s->len ? s->buttons[frame] : 0;
}

int input_end(const InputScript *s)
{
	if (s->end)
		return s->end;
	int end = s->len;
	while (end > 0 && !s->buttons[end - 1])
		end--;
	return end;
}

void recorder_start(InputRecorder *r, int frame, uint8_t *state, size_t state_len)
{
	r->start = frame;
	r->len = 0;
	free(r->state);
	r->state = state;
	r->state_len = state_len;
}

void recorder_frame(InputRecorder *r, int frame, uint8_t buttons)
{
	int i = frame - r->start;
	if (i < 0)
		return;
	if (i >= r->cap) {
		r->cap = r->cap ? r->cap * 2 : 4096;
		while (r->cap <= i)
			r->cap *= 2;
		r->buttons = realloc(r->buttons, (size_t)r->cap);
	}
	if (i >= r->len) {
		memset(r->buttons + r->len, 0, (size_t)(i + 1 - r->len));
		r->len = i + 1;
	}
	r->buttons[i] = buttons;
}

bool recorder_save(const InputRecorder *r, const char *path)
{
	FILE *f;
	if (r->state) {
		char *state_path = malloc(strlen(path) + 7);
		sprintf(state_path, "%s.state", path);
		f = fopen(state_path, "wb");
		free(state_path);
		if (!f || fwrite(r->state, 1, r->state_len, f) != r->state_len) {
			if (f)
				fclose(f);
			return false;
		}
		fclose(f);
	}
	f = fopen(path, "wb"); // \n line endings on every platform
	if (!f)
		return false;
	fprintf(f, "# Recorded with wl3 --record\n");
	if (r->state) {
		const char *base = path;
		for (const char *p = path; *p; p++)
			if (*p == '/' || *p == '\\')
				base = p + 1;
		fprintf(f, "state %s.state\n", base);
	}
	fprintf(f, "end %d\n", r->start + r->len - 1);
	for (int i = 0; i < r->len;) {
		uint8_t b = r->buttons[i];
		int j = i + 1;
		while (j < r->len && r->buttons[j] == b)
			j++;
		if (b) {
			fprintf(f, "%d %d ", r->start + i, j - i);
			bool first = true;
			for (int k = 0; k < 8; k++) {
				if (b & (1 << k)) {
					fprintf(f, "%s%s", first ? "" : "+", button_names[k]);
					first = false;
				}
			}
			fputc('\n', f);
		}
		i = j;
	}
	return fclose(f) == 0;
}
