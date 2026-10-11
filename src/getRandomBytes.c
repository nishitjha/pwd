#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void generate_random_bytes(uint8_t *buf, size_t len)
{
    FILE *f = fopen("/dev/urandom", "rb");
    fread(buf, 1, len, f);
    fclose(f);
}