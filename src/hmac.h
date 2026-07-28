#ifndef HMAC_H
#define HMAC_H
#include <stdint.h>

void hmac_sha1(const uint8_t *key, size_t key_len,
               const uint8_t *msg, size_t msg_len,
               uint8_t out_digest[20]);

#endif