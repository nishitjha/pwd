#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <decode32.h>

typedef struct {
    uint8_t totp[64];
    size_t totp_length;
} TOTP;

TOTP generate() {

    static uint8_t totp[64];
    static size_t totp_length;
    char secret32[256] = "IQXW4Z3FOI======"; //yeah i know hardcoded cry me a river

    decode(secret32, totp, &totp_length);

    TOTP totp_result;
    memset(&totp_result, 0, sizeof(TOTP));

    size_t bytes_to_copy = (totp_length < 64) ? totp_length : 64;
    memcpy(totp_result.totp, totp, bytes_to_copy);

    totp_result.totp_length = totp_length;
    return totp_result;
}