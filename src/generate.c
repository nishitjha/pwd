#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <decode32.h>
#include "counter.h"
#include "hmac.h"

uint32_t truncateAndMod(const uint8_t digest[20])
{
    int offset = digest[19] & 0x0F;

    uint32_t code = ((digest[offset] & 0x7F) << 24) | ((digest[offset + 1] & 0xFF) << 16) | ((digest[offset + 2] & 0xFF) << 8) | (digest[offset + 3] & 0xFF);

    return code % 1000000;
}

void code_to_string(uint32_t code, char dest[7])
{
    snprintf(dest, 7, "%06d", code);
}

void generate(char *secret32, char out[7]){

    uint8_t secret[64];
    size_t secret_length;

    decode(secret32, secret, &secret_length);

    CounterBytes counter = getCounter();
    static uint8_t digest[20];
    hmac_sha1((const uint8_t *)secret, secret_length,
              (const uint8_t *)counter.bytes, 8,
              digest);
    int unpaddedTOTP = truncateAndMod(digest); // it's like sometimes you're gonna end up with a 4/5 digit number and then you're gonna have to slap some leading zeroes on the front to make it valid
    // shoutout to claude for suggesting this in the first place no way i would've thought of this edge case

    code_to_string(unpaddedTOTP, out);
}
