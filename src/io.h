#ifndef IO_H
#define IO_H
#include <stdint.h>

void addNewEntry(const char *appName, const char *secret);
uint8_t* getEntries(long *destSize);

#endif