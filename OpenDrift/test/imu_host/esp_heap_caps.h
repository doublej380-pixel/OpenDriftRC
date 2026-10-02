#pragma once
#include <cstdlib>
constexpr int MALLOC_CAP_SPIRAM = 1, MALLOC_CAP_8BIT = 2, MALLOC_CAP_INTERNAL = 4;
inline void* heap_caps_malloc(size_t length, int) { return std::malloc(length); }
