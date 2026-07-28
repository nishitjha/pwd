#ifndef COUNTER_H
#define COUNTER_H
#include "stdint.h"

typedef struct
{
    uint8_t bytes[8];
} CounterBytes;

CounterBytes getCounter();


#endif