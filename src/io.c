#include <stdio.h>
#include <string.h>
#include "aes.h"
#include "getHome.h"
#include "hmac.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>

#define IV_LEN 16
#define MAC_LEN 20

uint8_t key[16];
uint8_t salt[16];

char *getPath(const char *name)
{
    const char *homeDir = getHomeDir();
    size_t len = strlen(homeDir) + strlen(name) + 2;
    char *path = malloc(len);
    if (path == NULL)
    {
        return NULL;
    }
    snprintf(path, len, "%s/%s", homeDir, name);
    return path;
}

int fillRandom(uint8_t *out, size_t len)
{
    FILE *urandom = fopen("/dev/urandom", "rb");
    if (urandom == NULL)
    {
        return -1;
    }
    size_t got = fread(out, 1, len, urandom);
    fclose(urandom);
    return got == len ? 0 : -1;
}

int writeFileSafely(const char *path, const uint8_t *data, size_t len)
{
    size_t tmpLen = strlen(path) + strlen(".tmp") + 1;
    char *tmpPath = malloc(tmpLen);
    if (tmpPath == NULL)
    {
        return -1;
    }
    snprintf(tmpPath, tmpLen, "%s.tmp", path);

    int fd = open(tmpPath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd == -1)
    {
        free(tmpPath);
        return -1;
    }
    fchmod(fd, 0600);

    FILE *out = fdopen(fd, "wb");
    if (out == NULL)
    {
        close(fd);
        unlink(tmpPath);
        free(tmpPath);
        return -1;
    }

    size_t written = fwrite(data, 1, len, out);
    int closed = fclose(out);
    if (written != len || closed != 0 || rename(tmpPath, path) != 0)
    {
        unlink(tmpPath);
        free(tmpPath);
        return -1;
    }

    free(tmpPath);
    return 0;
}

int initSalt()
{
    char *path = getPath(".pwd_salt");
    if (path == NULL)
    {
        return -1;
    }

    FILE *file = fopen(path, "rb");
    if (file != NULL)
    {
        size_t got = fread(salt, 1, sizeof(salt), file);
        fclose(file);
        free(path);
        return got == sizeof(salt) ? 0 : -1;
    }

    if (fillRandom(salt, sizeof(salt)) != 0)
    {
        free(path);
        return -1;
    }

    int result = writeFileSafely(path, salt, sizeof(salt));
    free(path);
    return result;
}

__attribute__((constructor)) static void saltSetup()
{
    if (initSalt() != 0)
    {
        printf("Couldn't set up the salt file.\n");
        exit(1);
    }
}

void pbkdf2(const uint8_t *password, size_t password_len,
            const uint8_t *salt, size_t salt_len,
            uint32_t iterations,
            uint8_t *out_key, size_t out_key_len)
{

    uint8_t bLockIndex[4] = {0, 0, 0, 1};

    uint8_t *saltIndex = malloc(salt_len + 4);
    if (saltIndex == NULL)
    {
        printf("Out of memory.\n");
        exit(1);
    }
    memcpy(saltIndex, salt, salt_len);
    memcpy(saltIndex + salt_len, bLockIndex, 4);

    uint8_t buffer1[20];
    hmac_sha1(password, password_len, saltIndex, salt_len + 4, buffer1);
    free(saltIndex);

    uint8_t buffer2[20];
    memcpy(buffer2, buffer1, 20);

    for (uint32_t i = 1; i < iterations; i++)
    {
        uint8_t next[20];
        hmac_sha1(password, password_len, buffer2, 20, next);
        memcpy(buffer2, next, 20);

        for (int j = 0; j < 20; j++)
        {
            buffer1[j] ^= buffer2[j];
        }
    }
    memcpy(out_key, buffer1, out_key_len < 20 ? out_key_len : 20);
}

static int unlocked = 0;
static int keyFromCache = 0;

void clearSudo()
{
    char *filename = getPath(".pwd_sudo.txt");
    if (filename != NULL)
    {
        unlink(filename);
        free(filename);
    }
}

void writeSudoKey()
{
    char hex[33];
    for (int i = 0; i < 16; i++)
    {
        snprintf(hex + i * 2, 3, "%02x", key[i]);
    }

    char content[128];
    int length = snprintf(content, sizeof(content), "%ld\n%s\n", (long)time(NULL), hex);

    char *filename = getPath(".pwd_sudo.txt");
    if (filename == NULL)
    {
        return;
    }
    writeFileSafely(filename, (const uint8_t *)content, length);
    free(filename);
}

int loadSudoKey()
{
    char *filename = getPath(".pwd_sudo.txt");
    if (filename == NULL)
    {
        return 0;
    }
    FILE *file = fopen(filename, "rb");
    free(filename);

    if (file == NULL)
    {
        return 0;
    }

    char ltoa[64];
    char keyLine[64];
    if (fgets(ltoa, sizeof(ltoa), file) == NULL || fgets(keyLine, sizeof(keyLine), file) == NULL)
    {
        fclose(file);
        return 0;
    }
    fclose(file);

    long elapsed = (long)time(NULL) - atol(ltoa);
    if (elapsed < 0 || elapsed >= 600)
    {
        clearSudo();
        return 0;
    }

    keyLine[strcspn(keyLine, "\n")] = '\0';
    if (strlen(keyLine) != 32 || strspn(keyLine, "0123456789abcdef") != 32)
    {
        return 0;
    }

    uint8_t parsed[16];
    for (int i = 0; i < 16; i++)
    {
        unsigned int byte;
        sscanf(keyLine + i * 2, "%2x", &byte);
        parsed[i] = (uint8_t)byte;
    }
    memcpy(key, parsed, sizeof(parsed));
    return 1;
}

int getSudo()
{
    uint8_t saved[16];
    memcpy(saved, key, sizeof(saved));
    int active = loadSudoKey();
    memcpy(key, saved, sizeof(saved));
    return active;
}

int computeMac(const uint8_t *fileIv, const uint8_t *cipher, long cipherLen, uint8_t *out)
{
    uint8_t macKey[20];
    hmac_sha1(key, sizeof(key), (const uint8_t *)"pwd-mac", 7, macKey);

    uint8_t *message = malloc(IV_LEN + cipherLen);
    if (message == NULL)
    {
        return -1;
    }
    memcpy(message, fileIv, IV_LEN);
    if (cipherLen > 0)
    {
        memcpy(message + IV_LEN, cipher, cipherLen);
    }
    hmac_sha1(macKey, sizeof(macKey), message, IV_LEN + cipherLen, out);
    free(message);
    return 0;
}

int macsMatch(const uint8_t *a, const uint8_t *b)
{
    uint8_t diff = 0;
    for (int i = 0; i < MAC_LEN; i++)
    {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

int readPassword(const char *prompt, char *buf, size_t size)
{
    struct termios oldTerm, newTerm;
    int hidden = tcgetattr(STDIN_FILENO, &oldTerm) == 0;
    if (hidden)
    {
        newTerm = oldTerm;
        newTerm.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);
    }

    printf("%s", prompt);
    fflush(stdout);
    char *got = fgets(buf, size, stdin);

    if (hidden)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
        printf("\n");
    }

    if (got == NULL)
    {
        return -1;
    }
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

int ensureKey()
{
    if (unlocked)
    {
        return 0;
    }

    char *path = getPath(".pwd_secrets.txt");
    if (path == NULL)
    {
        return -1;
    }
    int vaultExists = access(path, F_OK) == 0;
    free(path);

    if (vaultExists && loadSudoKey())
    {
        unlocked = 1;
        keyFromCache = 1;
        return 0;
    }

    char pwd[256];
    if (readPassword(vaultExists ? "Master password: " : "No vault yet, choose a master password: ", pwd, sizeof(pwd)) != 0)
    {
        return -1;
    }
    if (strlen(pwd) == 0)
    {
        printf("Password can't be empty.\n");
        return -1;
    }

    if (!vaultExists)
    {
        char again[256];
        if (readPassword("Confirm master password: ", again, sizeof(again)) != 0 || strcmp(pwd, again) != 0)
        {
            printf("Passwords didn't match.\n");
            memset(pwd, 0, sizeof(pwd));
            memset(again, 0, sizeof(again));
            return -1;
        }
        memset(again, 0, sizeof(again));
    }

    printf("Unlocking...\n");
    fflush(stdout);
    pbkdf2((const uint8_t *)pwd, strlen(pwd), salt, sizeof(salt), 1000000, key, sizeof(key));
    memset(pwd, 0, sizeof(pwd));
    unlocked = 1;
    return 0;
}

uint8_t *getEntries(long *destSize)
{
    *destSize = 0;
    char *filename = getPath(".pwd_secrets.txt");
    if (filename == NULL)
    {
        return NULL;
    }
    FILE *file = fopen(filename, "rb");
    int openError = errno;
    free(filename);

    if (file == NULL)
    {
        if (openError == ENOENT)
        {
            return calloc(1, 1);
        }
        printf("Error opening secrets file for reading.\n");
        return NULL;
    }

    if (ensureKey() != 0)
    {
        fclose(file);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < IV_LEN + MAC_LEN)
    {
        fclose(file);
        printf("Secrets file is corrupted.\n");
        return NULL;
    }

    uint8_t *raw = malloc(size);
    if (raw == NULL)
    {
        fclose(file);
        return NULL;
    }
    size_t got = fread(raw, 1, size, file);
    fclose(file);
    if (got != (size_t)size)
    {
        free(raw);
        printf("Error reading secrets file.\n");
        return NULL;
    }

    long cipherLen = size - IV_LEN - MAC_LEN;
    uint8_t expected[MAC_LEN];
    if (computeMac(raw, raw + IV_LEN, cipherLen, expected) != 0)
    {
        free(raw);
        return NULL;
    }
    if (!macsMatch(expected, raw + IV_LEN + cipherLen))
    {
        free(raw);
        if (keyFromCache)
        {
            clearSudo();
            printf("Saved session no longer matches the vault, run the command again.\n");
        }
        else
        {
            printf("Wrong master password, or the secrets file was tampered with.\n");
        }
        return NULL;
    }
    if (!keyFromCache)
    {
        writeSudoKey();
    }

    uint8_t *buffer = malloc(cipherLen + 1);
    if (buffer == NULL)
    {
        free(raw);
        return NULL;
    }
    memcpy(buffer, raw + IV_LEN, cipherLen);
    buffer[cipherLen] = '\0';

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, raw);
    AES_CTR_xcrypt_buffer(&ctx, buffer, cipherLen);
    free(raw);

    *destSize = cipherLen;

    return buffer;
}

int saveEntries(const uint8_t *plain, long len)
{
    if (ensureKey() != 0)
    {
        return -1;
    }

    long total = IV_LEN + len + MAC_LEN;
    uint8_t *out = malloc(total);
    if (out == NULL)
    {
        return -1;
    }

    if (fillRandom(out, IV_LEN) != 0)
    {
        free(out);
        printf("Couldn't generate a random IV.\n");
        return -1;
    }
    if (len > 0)
    {
        memcpy(out + IV_LEN, plain, len);
    }

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, out);
    AES_CTR_xcrypt_buffer(&ctx, out + IV_LEN, len);

    if (computeMac(out, out + IV_LEN, len, out + IV_LEN + len) != 0)
    {
        free(out);
        return -1;
    }

    char *filename = getPath(".pwd_secrets.txt");
    if (filename == NULL)
    {
        free(out);
        return -1;
    }
    int result = writeFileSafely(filename, out, total);
    free(filename);
    free(out);

    if (result != 0)
    {
        printf("Error opening secrets file for writing.\n");
    }
    else if (!keyFromCache)
    {
        writeSudoKey();
    }
    return result;
}

void addNewEntry(const char *appName, const char *secret)
{
    if (strchr(appName, ':') != NULL || strchr(appName, '\n') != NULL || strchr(secret, '\n') != NULL)
    {
        printf("App name can't contain ':' and nothing can contain newlines.\n");
        return;
    }

    long existingSize = 0;
    uint8_t *existing = getEntries(&existingSize);
    if (existing == NULL)
    {
        return;
    }

    size_t lineCap = strlen(appName) + strlen(secret) + 4;
    char *newLine = malloc(lineCap);
    if (newLine == NULL)
    {
        free(existing);
        return;
    }
    int newLineLength = snprintf(newLine, lineCap, "%s: %s\n", appName, secret);

    long totalNewSize = existingSize + newLineLength;
    uint8_t *combinedSize = malloc(totalNewSize);
    if (combinedSize == NULL)
    {
        free(existing);
        free(newLine);
        return;
    }
    if (existingSize > 0)
    {
        memcpy(combinedSize, existing, existingSize);
    }
    memcpy(combinedSize + existingSize, newLine, newLineLength);
    free(existing);
    free(newLine);

    saveEntries(combinedSize, totalNewSize);
    free(combinedSize);
}

int removeEntry(const char *appName)
{
    long bufferLength = 0;
    uint8_t *buffer = getEntries(&bufferLength);
    if (buffer == NULL)
    {
        return -1;
    }

    uint8_t *removedList = malloc(bufferLength + 1);
    if (removedList == NULL)
    {
        free(buffer);
        return -1;
    }
    long removedLength = 0;
    int found = 0;

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

                if (isMatch)
                {
                    found = 1;
                }
                else
                {
                    long copyLen = (i < bufferLength) ? lineLen + 1 : lineLen;
                    memcpy(removedList + removedLength, buffer + lineStart, copyLen);
                    removedLength += copyLen;
                }
            }

            lineStart = i + 1;
        }
    }

    if (!found)
    {
        printf("No entry found for \"%s\". Maybe learn how to spell, bozo.\n", appName);
        free(buffer);
        free(removedList);
        return -1;
    }

    free(buffer);
    int result = saveEntries(removedList, removedLength);
    free(removedList);
    return result;
}
char *getSecret(const char *appName)
{
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