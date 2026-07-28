
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "generate.h"

char* toLower(const char* src, char* dest) {
    int i = 0;
    for (i = 0; src[i] != '\0'; i++) {
        dest[i] = tolower((unsigned char)src[i]);
    }
    dest[i] = '\0'; 
    return dest;
}

int parse_args(int argc, char *argv[]) {
    char* commands[] = {"add", "get", "remove", "list", "help"};

    if (argc < 2 || argc > 6) {
        fprintf(stderr, "Invalid usage.\n");
        return -1;
    }

    char lowerCmd[256];
    char lowerArg[256];
    toLower(argv[1], lowerArg);

    int found = 0;
    for (uint64_t i=0; i<sizeof(commands) / sizeof(commands[0]); i++ ) {
        toLower(commands[i], lowerCmd);
        if (strcmp(lowerCmd, lowerArg) == 0) {
            switch (i){
            case 0:
                //add new app
                if (argv[2] == NULL)
                {
                    printf("Please provide the app name to add a new entry. \n");
                    return -1;
                }
                if (argv[3] == NULL)
                {
                    printf("Please provide the base32 secret for the TOTP. \n");
                    return -1;
                }

                char totp_string[7];
                generate(argv[3], totp_string);
                printf("Added new entry for %s (didn't actually but you know) \n",argv[2]);
                printf("Your TOTP is: %s \n", totp_string);

                found = 1;
                break;
            case 1:
                // get totp for app
                if (argc < 3) {
                    printf("Please provide the app name to get its TOTP. \n");
                    return -1;
                }
                found = 1;
                break;
            case 2:
                // remove app
                if (argc < 3) {
                    printf("Please provide the app name to remove an entry. \n");
                    return -1;
                }
                printf("Removed entry for %s. \n", argv[2]);
                found = 1;
                break;
            case 3:
                // list apps
                printf("Listing all apps: \n");
                found = 1;
                break;
            case 4:
                // help
                printf("Available commands: \n");
                printf("  add <length> <min> - Add a new app with the specified password length and minimum value. \n");
                printf("  get <app_name> <length> <min> - Get the password for the specified app with the given length and minimum value. \n");
                printf("  remove <app_name> - Remove the specified app entry. \n");
                printf("  list - List all apps. \n");
                printf("  help - Show this help message. \n");
                found = 1;
                break;
            default:
                break;
            }
            break;
        } else {
            continue;
        }
    }

    if (found == 0) {
        printf("Invalid command. Please use help to see the list of available commands. \n");
        return -1;
    }
    return 0;
}

