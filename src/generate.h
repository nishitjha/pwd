#ifndef GENERATOR_H
#define GENERATOR_H

typedef struct {
    uint8_t secret[64];
    size_t secret_length;
} secret_t;

void generate(char *secret32, char dest[7]);
void hmac_sha1(const uint8_t *key, size_t key_len,
               const uint8_t *msg, size_t msg_len,
               uint8_t out_digest[20]);

uint32_t truncateAndMod(const uint8_t digest[20]);

void code_to_string(uint32_t code, char dest[7]);
#endif