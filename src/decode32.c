#include <stdlib.h>
#include <string.h>

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

char* decode(char* secret32, u_int8_t * dest) {
    //figure out how to decode base-32 
    // from what I read it seems like base32 often makes use of padding (= signs) to ensure a constant length. 
    // i was gonna trim those and then decode the string but it turns out that many great men before me have tried this and failed (as seen on some github issues you need some big ooga booga logic to fix improper padding)
    // so fuck it we ball

    char secretCopy[256];
    strncpy(secretCopy, secret32, sizeof(secretCopy) - 1);
    secretCopy[sizeof(secretCopy) - 1] = '\0';

    /*for (int i = strlen(secret32) - 1; i >= 0; i--){
        if (secret32[i] == '='){
            secret32[i] = '\0'; 
        }
        else
        {
            break;
        }
    }*/

    for (int i = 0; i < strlen(secretCopy); i++) {
        secretCopy[i] = ((unsigned char)secretCopy[i]);
    }

    
}
