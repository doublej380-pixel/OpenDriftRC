#pragma once
#include "Arduino.h"
#include <array>
#include <vector>
struct FakeWire
{
    std::array<uint8_t, 256> registers{};
    std::vector<uint8_t> reads;
    uint8_t address = 0;
    size_t remaining = 0;
    bool shortBurst = false;
    void begin(int, int) {}
    void beginTransmission(uint8_t) {}
    void write(uint8_t value) { address = value; }
    int endTransmission(bool) { return 0; }
    size_t requestFrom(uint8_t, size_t count)
    {
        reads.push_back(address);
        remaining = shortBurst && count == 17 ? 3 : count;
        return remaining;
    }
    int available() { return remaining; }
    int read() { --remaining; return registers[address++]; }
};
extern FakeWire Wire;
