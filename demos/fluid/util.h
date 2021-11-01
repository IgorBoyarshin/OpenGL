#ifndef UTIL_H
#define UTIL_H

#include <time.h>

float get_time_micros() noexcept {
    timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return static_cast<float>(t.tv_sec) * 1'000'000.0f + static_cast<float>(t.tv_nsec) / 1000.0f;
}


#endif
