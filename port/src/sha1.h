// SHA-1, to check that a ROM is the one the port was built for.
#ifndef WL3_SHA1_H
#define WL3_SHA1_H

#include <stddef.h>
#include <stdint.h>

// Writes the digest of data as 40 lowercase hex digits plus a NUL.
void sha1_hex(const uint8_t *data, size_t len, char out[41]);

#endif
