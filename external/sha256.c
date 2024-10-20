#include "sha256.h"

// SHA-256 Constants
static const uint32_t k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19b4c79b, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa11, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Right rotate
static inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

// SHA256 transformation
static void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
    uint32_t a, b, c, d, e, f, g, h, temp1, temp2;
    uint32_t m[64] = {0};

    for (size_t i = 0; i < 16; i++) {
        m[i] = (data[i * 4] << 24) | (data[i * 4 + 1] << 16) |
               (data[i * 4 + 2] << 8) | (data[i * 4 + 3]);
    }
    for (size_t i = 16; i < 64; i++) {
        m[i] = rotr(m[i - 2], 17) ^ rotr(m[i - 2], 19) ^ (m[i - 2] >> 10);
        m[i] += m[i - 7] + rotr(m[i - 15], 7) ^
                rotr(m[i - 15], 18) ^ (m[i - 15] >> 3);
        m[i] += m[i - 16];
    }

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];

    for (size_t i = 0; i < 64; i++) {
        temp1 = h + rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        temp1 += (e & f) ^ ((~e) & g) + k[i] + m[i];
        temp2 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        temp2 += (a & b) ^ (a & c) ^ (b & c);
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

// Initialize the SHA256 state
void sha256_init(uint32_t state[8]) {
    state[0] = 0x6a09e667;
    state[1] = 0xbb67ae85;
    state[2] = 0x3c6ef372;
    state[3] = 0xa54ff53a;
    state[4] = 0x510e527f;
    state[5] = 0x9b05688c;
    state[6] = 0x1f83d9ab;
    state[7] = 0x5be0cd19;
}

// SHA256 hash function
void SHA256(const uint8_t *data, size_t len, uint8_t *hash) {
    uint32_t state[8];
    sha256_init(state);

    uint8_t buffer[64];
    size_t i;
    
    for (i = 0; i + 64 <= len; i += 64) {
        sha256_transform(state, data + i);
    }

    memcpy(buffer, data + i, len - i);
    buffer[len - i] = 0x80; // Append the '1' bit

    if (len % 64 >= 56) {
        memset(buffer + (len - i + 1), 0, 64 - (len - i + 1));
        sha256_transform(state, buffer);
        memset(buffer, 0, 56);
    } else {
        memset(buffer + (len - i + 1), 0, 56 - (len - i + 1));
    }

    // Append the length
    uint64_t bit_len = len * 8;
    for (int j = 0; j < 8; j++) {
        buffer[56 + j] = (bit_len >> (56 - j * 8)) & 0xff;
    }
    sha256_transform(state, buffer);

    for (i = 0; i < 8; i++) {
        hash[i * 4] = (state[i] >> 24) & 0xff;
        hash[i * 4 + 1] = (state[i] >> 16) & 0xff;
        hash[i * 4 + 2] = (state[i] >> 8) & 0xff;
        hash[i * 4 + 3] = state[i] & 0xff;
    }
}
