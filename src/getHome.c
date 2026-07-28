#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

//eh i know this is for me but still in case i use it on my other laptop with windows

const char *getHomeDir(){
#ifdef _WIN32

    static char path[256];
    char *env = getenv("USERPROFILE");
    if (env)
        return env;

    char *drive = getenv("HOMEDRIVE");
    char *home = getenv("HOMEPATH");
    if (drive && home)
    {
        snprintf(path, sizeof(path), "%s%s", drive, home);
        return path;
    }
#else
    char *env = getenv("HOME");
    if (env) {
        return env;
    }

    struct passwd *pw = getpwuid(getuid());
    if (pw)
        return pw->pw_dir;

#endif
    return NULL;
}