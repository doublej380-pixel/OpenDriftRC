#pragma once
#include <cstddef>
#include <cstring>
#include <vector>
#include "freertos/portmacro.h"

// In-memory stand-in for ParameterStore persistence tests, not ESP NVS.
class Preferences
{
    std::vector<unsigned char> bytes;
public:
    size_t getBytesLength(const char*) { return bytes.size(); }
    size_t getBytes(const char*, void* destination, size_t length)
    {
        if(length > bytes.size()) length = bytes.size();
        if(length) std::memcpy(destination, bytes.data(), length);
        return length;
    }
    size_t putBytes(const char*, const void* source, size_t length)
    {
        const auto* start = static_cast<const unsigned char*>(source);
        bytes.assign(start, start + length);
        return length;
    }
};
