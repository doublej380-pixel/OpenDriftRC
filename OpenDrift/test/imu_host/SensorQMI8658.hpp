#pragma once
#include "Wire.h"
constexpr uint8_t QMI8658_L_SLAVE_ADDRESS = 0x6B;
extern bool hostLockingFails;
struct SensorQMI8658
{
    enum LpfMode { LPF_MODE_0, LPF_MODE_3, LPF_OFF };
    enum { ACC_RANGE_4G, ACC_ODR_1000Hz, GYR_RANGE_1024DPS, GYR_ODR_896_8Hz };
    bool begin(FakeWire&, int, int, int) { Wire.registers[8] = 0; return true; }
    bool configAccelerometer(int, int, LpfMode) { return true; }
    bool configGyroscope(int, int, LpfMode mode)
    {
        Wire.registers[4] = 0x63;
        Wire.registers[6] = mode == LPF_OFF ? 0 : mode == LPF_MODE_3 ? 0x70 : 0x10;
        return true;
    }
    bool enableAccelerometer() { Wire.registers[8] |= 1; return true; }
    bool enableGyroscope() { Wire.registers[8] |= 2; return true; }
    bool disableAccelerometer() { Wire.registers[8] &= ~1; return true; }
    bool disableGyroscope() { Wire.registers[8] &= ~2; return true; }
    bool isEnableGyroscope() { return Wire.registers[8] & 2; }
    bool enableLockingMechanism() { Wire.registers[8] |= 0x80; return !hostLockingFails; }
    bool disableSyncSampleMode() { Wire.registers[8] &= ~0x80; return true; }
    bool getGyroscope(float& x, float& y, float& z) { return axes(0x3B, getGyroscopeScales(), x, y, z); }
    bool getAccelerometer(float& x, float& y, float& z) { return axes(0x35, getAccelerometerScales(), x, y, z); }
    bool axes(int address, float scale, float& x, float& y, float& z)
    {
        auto read = [&](int a) { return (int16_t)((uint16_t)Wire.registers[a] | ((uint16_t)Wire.registers[a+1] << 8)) * scale; };
        x = read(address); y = read(address+2); z = read(address+4); return true;
    }
    float getAccelerometerScales() { return 4.0f / 32768; }
    float getGyroscopeScales() { return 1024.0f / 32768; }
};
