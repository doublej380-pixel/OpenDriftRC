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
    size_t shortLength = 17;
    uint8_t device = 0;
    uint8_t ackDevice = 0; // Zero accepts either device, preserving QMI tests.
    uint8_t writes = 0;
    bool failWrites = false;
    void begin(int, int) {}
    void setClock(uint32_t) {}
    void setTimeOut(uint16_t) {}
    void beginTransmission(uint8_t value) { device = value; writes = 0; }
    void write(uint8_t value) {
        if(writes++ == 0) address = value;
        else if(!failWrites) registers[address++] = value;
    }
    int endTransmission(bool = true) { return (ackDevice && device != ackDevice) || (failWrites && writes > 1) ? 2 : 0; }
    size_t requestFrom(uint8_t, size_t count)
    {
        reads.push_back(address);
        remaining = shortBurst && count == shortLength ? 3 : count;
        return remaining;
    }
    int available() { return remaining; }
    int read() { --remaining; return registers[address++]; }
};
extern FakeWire Wire;
