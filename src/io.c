#include <stdio.h>
#include <string.h>
#include "aes.h"
#include "getHome.h"
#include <stdlib.h>

uint8_t key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
uint8_t iv[16] = {
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};

uint8_t *getEntries(long *destSize)
{
    const char *homeDir = getHomeDir();
    char *filename = malloc(strlen(homeDir) + strlen("/.pwd_secrets.txt") + 1);
    snprintf(filename, strlen(homeDir) + strlen("/.pwd_secrets.txt") + 1, "%s/.pwd_secrets.txt", homeDir);
    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("Error opening secrets file for reading.\n");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *buffer = malloc(size);
    fread(buffer, 1, size, file);
    fclose(file);

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CTR_xcrypt_buffer(&ctx, buffer, size);

    free(filename);
    //free(buffer); - don't, return it so args.c can free it up itself
    // it causes problems otherwise with the two different cases
    *destSize = size;

    return buffer;
}

void addNewEntry(const char *appName, const char *secret)
{
    long existingSize = 0;
    uint8_t *existing = getEntries(&existingSize); 

    char newLine[256];
    int newLineLength = snprintf(newLine, sizeof(newLine), "%s: %s\n", appName, secret);

    long totalNewSize = existingSize + newLineLength;
    uint8_t *combinedSize = malloc(totalNewSize);
    if (existingSize > 0)
    {
        memcpy(combinedSize, existing, existingSize);
    }
    memcpy(combinedSize + existingSize, newLine, newLineLength);
    free(existing); //apparently uou can do this even if existing is NULL so no need to check for that

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CTR_xcrypt_buffer(&ctx, combinedSize, totalNewSize);

    const char *homeDir = getHomeDir();
    char *filename = malloc(strlen(homeDir) + strlen("/.pwd_secrets.txt") + 1);
    snprintf(filename, strlen(homeDir) + strlen("/.pwd_secrets.txt") + 1, "%s/.pwd_secrets.txt", homeDir);

    FILE *out = fopen(filename, "wb");
    free(filename);
    if (out == NULL)
    {
        printf("Error opening secrets file for writing.\n");
        free(combinedSize);
        return;
    }
    fwrite(combinedSize, 1, totalNewSize, out);
    fclose(out);
    free(combinedSize);
}

int removeEntry(const char *appName){
    long bufferLength;
    uint8_t *buffer = getEntries(&bufferLength);

    uint8_t *removedList = malloc(bufferLength); 
    long removedLength = 0;

    long lineStart = 0;
    for (long i = 0; i <= bufferLength; i++)
    {
        if (i == bufferLength || buffer[i] == '\n')
        {
            long lineLen = i - lineStart;

            if (lineLen > 0)
            {
                long colonPos = -1;
                for (long j = lineStart; j < i; j++)
                {
                    if (buffer[j] == ':')
                    {
                        colonPos = j;
                        break;
                    }
                }
                // i was just gonna see if the appName is a substring of the line but then i realized that would be bad because if you have an app called "app" and another called "app2" then removing "app" would also remove "app2" (okay i agree you're not gonna have that but how about apple and applebee's) so instead we check if the appName matches the part of the line before the colon shoutout to claude for catching this edge case again
                int isMatch = 0;
                if (colonPos != -1)
                {
                    long nameLen = colonPos - lineStart;
                    if (nameLen == (long)strlen(appName) &&
                        strncmp((char *)(buffer + lineStart), appName, nameLen) == 0)
                    {
                        isMatch = 1;
                    }
                }

                if (!isMatch)
                {
                    long copyLen = (i < bufferLength) ? lineLen + 1 : lineLen;
                    memcpy(removedList + removedLength, buffer + lineStart, copyLen);
                    removedLength += copyLen;
                }
            }

            lineStart = i + 1;
        }
    }
    // if the appName wasn't found, we just don't do anything at all lol
    if (removedLength == bufferLength)
    {
        printf("No entry found for \"%s\". Maybe learn how to spell, bozo.\n", appName);
        free(buffer);
        free(removedList);
        return -1;
    }


    free(buffer);
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CTR_xcrypt_buffer(&ctx, removedList, removedLength);

    const char *homeDir = getHomeDir();
    char *filename = malloc(strlen(homeDir) + strlen("/.pwd_secrets.txt") + 1);
    snprintf(filename, strlen(homeDir) + strlen("/.pwd_secrets.txt") + 1, "%s/.pwd_secrets.txt", homeDir);

    FILE *out = fopen(filename, "wb");
    free(filename);
    if (out == NULL)
    {
        printf("Error opening secrets file for writing.\n");
        free(removedList);
        return -1;
    }
    fwrite(removedList, 1, removedLength, out);
    fclose(out);
    free(removedList);
    return 0;
}
char *getSecret(const char *appName){
    long size = 0;
    uint8_t *entries = getEntries(&size);
    if (entries == NULL)
    {
        return NULL;
    }

    char *secret = NULL;
    char *entry = strtok((char *)entries, "\n");
    while (entry != NULL)
    {
        char *colonPos = strchr(entry, ':');
        if (colonPos != NULL)
        {
            size_t nameLength = colonPos - entry;
            if (strncmp(entry, appName, nameLength) == 0 && strlen(appName) == nameLength)
            {
                secret = strdup(colonPos + 2);
                break;
            }
        }
        entry = strtok(NULL, "\n");
    }

    free(entries);
    return secret;
}
