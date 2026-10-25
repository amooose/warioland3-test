#ifndef PNG_H
#define PNG_H

#include <stdbool.h>
#include <stdint.h>

bool png_write(const char *path, const uint32_t *argb, int w, int h);

#endif
