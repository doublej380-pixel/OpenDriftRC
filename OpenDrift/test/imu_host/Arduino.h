#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
extern uint32_t hostTime;
inline uint32_t micros() { return hostTime++; }
inline uint32_t millis() { return hostTime / 1000; }
inline void delayMicroseconds(uint32_t value) { hostTime += value; }
inline void delay(uint32_t value) { hostTime += value * 1000; }
template<class T, class L, class H> T constrain(T value, L low, H high)
{ return std::min<T>(std::max<T>(value, low), high); }
using std::max;
struct FakeSerial
{
    void println(const char* value) { std::puts(value); }
    template<class... T> void printf(const char* format, T... args) { std::printf(format, args...); }
};
static FakeSerial Serial;
