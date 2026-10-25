// Input scripts: recorded or hand-written button presses, used for replays,
// smoke tests and difftest coverage.
//
// Format, one item per line (# starts a comment):
//   state <path>                     start from this save state (path is
//                                    relative to the script)
//   end <frame>                      the frame the script ends at
//   <frame> <duration> <buttons>     hold buttons from frame for duration
//                                    frames; buttons are a, b, start, select,
//                                    up, down, left, right joined by + or
//                                    spaces
// Frame numbers are absolute: a script that starts from a save state uses
// the frame numbers that continue from it.
#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint8_t *buttons; // per frame
	int len;          // frames covered by buttons
	int end;          // "end" directive, or 0
	char *state_path; // "state" directive resolved against the script, or NULL
} InputScript;

bool input_load(InputScript *s, const char *path);
uint8_t input_buttons(const InputScript *s, int frame);
// The "end" directive, or one frame past the last button press.
int input_end(const InputScript *s);

typedef struct {
	uint8_t *buttons;
	int start, len, cap;
	// Serialized save state the recording starts from (NULL: from boot).
	// Saved next to the script as <script>.state.
	uint8_t *state;
	size_t state_len;
} InputRecorder;

// Start (or restart) recording at the given frame, optionally from a state
// (the recorder takes ownership of it).
void recorder_start(InputRecorder *r, int frame, uint8_t *state, size_t state_len);
void recorder_frame(InputRecorder *r, int frame, uint8_t buttons);
bool recorder_save(const InputRecorder *r, const char *path);

#endif
