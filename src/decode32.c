#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

const int8_t BASE32_DECODE_MAP[256] = {
    ['A'] = 0,
    ['B'] = 1,
    ['C'] = 2,
    ['D'] = 3,
    ['E'] = 4,
    ['F'] = 5,
    ['G'] = 6,
    ['H'] = 7,
    ['I'] = 8,
    ['J'] = 9,
    ['K'] = 10,
    ['L'] = 11,
    ['M'] = 12,
    ['N'] = 13,
    ['O'] = 14,
    ['P'] = 15,
    ['Q'] = 16,
    ['R'] = 17,
    ['S'] = 18,
    ['T'] = 19,
    ['U'] = 20,
    ['V'] = 21,
    ['W'] = 22,
    ['X'] = 23,
    ['Y'] = 24,
    ['Z'] = 25,
    ['2'] = 26,
    ['3'] = 27,
    ['4'] = 28,
    ['5'] = 29,
    ['6'] = 30,
    ['7'] = 31};
 

void decode(char* secret32, uint8_t output_bytes[64], size_t* output_length) {
    // from what I read it seems like base32 often makes use of padding (= signs) to ensure a constant length. so we'll skip those

    char secretCopy[256];
    strncpy(secretCopy, secret32, sizeof(secretCopy) - 1);
    secretCopy[sizeof(secretCopy) - 1] = '\0';

    uint32_t buffer = 0;
    int bitsInBuffer = 0;
    int bytes = 0;

    /*for (int i = strlen(secret32) - 1; i >= 0; i--){
        if (secret32[i] == '='){
            secret32[i] = '\0'; 
        }
        else
        {
            break;
        }
    }*/

    for (uint8_t i = 0; i < strlen(secretCopy); i++) {
        if (secret32[i] == '=' || secret32[i] == ' ')
        {
            continue;
        }

        int num = BASE32_DECODE_MAP[(unsigned char)toupper((unsigned char)secretCopy[i])];
        buffer = (buffer << 5) | num;
        bitsInBuffer += 5;

        while (bitsInBuffer >= 8)
        {
            uint8_t byteFull = (buffer >> (bitsInBuffer - 8)) & 0xFF;
            output_bytes[bytes] = byteFull;

            bytes++;
            bitsInBuffer -= 8;
        }
    }

    *output_length = bytes;
}
