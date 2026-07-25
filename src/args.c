#include "generate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


char* toLower(const char* str) {
    static char lowerStr[256];
    for (int i =0; str[i] != '\0'; i++) {
        lowerStr[i] = tolower(str[i]);
    }
    lowerStr[strlen(str)] = '\0'; 
    return lowerStr;
}

int parse_args(int argc, char *argv[]) {
    char* commands[] = {"generate"};

    if (argc < 2 || argc > 6) {
        fprintf(stderr, "Invalid usage.\n");
        return -1;
    }


    int found = 0;
    for (uint64_t i=0; i<sizeof(commands) / sizeof(commands[0]); i++ ) {
        if (strcmp(toLower(commands[i]), toLower(argv[1])) == 0) {
            switch (i){
            case 0:
                printf("Generated password: %s \n", generate(atoi(argv[2] ? argv[2] : "12"), atoi(argv[3] ? argv[3] : "0")));
                break;
            default:
                break;
            }
        } else {
            continue;
        }
    }

    if (found == 0) {
        return -1;
    }
    return 0;
}

