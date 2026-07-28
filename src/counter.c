#include "stdint.h"
#include "counter.h"
#include "time.h"


CounterBytes getCounter() {
    CounterBytes counterBytes;
    time_t currentTime = time(NULL);
    uint64_t counter = (uint64_t)(currentTime / 30);
    
    for (int i = 7; i >= 0; i--) {
        counterBytes.bytes[i] = counter & 0xFF;
        counter >>= 8;
    }
    
    return counterBytes;
}