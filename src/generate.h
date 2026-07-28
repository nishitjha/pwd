#ifndef GENERATOR_H
#define GENERATOR_H

typedef struct {
    uint8_t totp[64];
    size_t totp_length;
} TOTP;

TOTP generate();

#endif