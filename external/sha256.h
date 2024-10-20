#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>
#include <string.h>

#define SHA256_BLOCK_SIZE 32

void SHA256(const uint8_t *data, size_t len, uint8_t *hash);

#endif // SHA256_H
