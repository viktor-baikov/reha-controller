#pragma once
#include <stdint.h>
uint32_t millis();
struct TestSerial {
    template<class... A> void printf(const char *,A...) {}
    void println(const char *) {}
};
extern TestSerial Serial;
