#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sodium.h>

const char* generate(int length, int min) {

    if (length >= 256){
        printf("Can't generate a password longer than 255 characters as of now. \n");
        exit(0);
    } else if (length < 8) {
        printf("Can't generate a strong password less than 8 characters long. \n");
        exit(0);
    }

    static char password[256];

    char* charset = "";

    if (min != 1) {
        charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";
    } else {
        charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!?@#$_";
    }

    for (int i = 0; i < length; i++) {
        password[i] = charset[randombytes_uniform(strlen(charset))];
    }
    password[length] = '\0'; 
    return password;
}