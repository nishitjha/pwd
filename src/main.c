#include <stdio.h>
#include <stdlib.h>
#include "generate.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <length>\n", argv[0]);
        return 1;
    }

    printf("Generated password: %s\n", generate(atoi(argv[1])));
    return 0;
}
