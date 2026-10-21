// Host-only transport tests; not a substitute for validation on the sensor.
#include "IMU.h"
#include <cassert>
#include <cmath>
#include <cstdio>
uint32_t hostTime = 100000;
FakeWire Wire;
bool hostLockingFails = false;
static void counter(uint32_t value)
{
    for(int i = 0; i < 3; ++i) Wire.registers[0x30 + i] = value >> (8 * i);
}
static void axis(int address, int16_t value)
{
    Wire.registers[address] = value & 255;
    Wire.registers[address + 1] = (uint16_t)value >> 8;
}
int main()
{
    IMU imu;
    assert(imu.begin(true));
    assert(imu.getGyroLpfMode() == 2 && Wire.registers[6] == 0);
    assert(imu.getSampleDiagnostics().locked);
    Wire.registers[0x2D] = 1;
    counter(0xFFFFFE);
    axis(0x35, 8192); axis(0x37, -8192); axis(0x39, 4096);
    axis(0x3B, -3200); axis(0x3D, 1600); axis(0x3F, -6400);
    imu.update();
    assert(imu.lastGyroReadOk());
    assert(imu.getYawRate() == -200.0f);
    assert(imu.getGyroX() == -100.0f && imu.getGyroY() == 50.0f);
    assert(imu.getAccelX() == 1.0f && imu.getAccelY() == -1.0f);
    assert(imu.getAccelZ() == 0.5f);
    assert(Wire.reads.back() == 0x30);
    counter(2); imu.update();
    assert(imu.getSampleDiagnostics().counterDelta == 4);
    imu.update();
    assert(!imu.getSampleDiagnostics().fresh);
    Wire.registers[0x2D] = 0; imu.update();
    assert(imu.getSampleDiagnostics().noDataCount == 1);
    assert(imu.getYawRate() == -200.0f);
    imu.update();
    assert(!imu.isYawValid()); // A stopped sample stream cannot stay valid.
    Wire.registers[0x2D] = 1;
    Wire.shortBurst = true; imu.update();
    assert(!imu.lastGyroReadOk());
    assert(imu.getSampleDiagnostics().readErrors == 1);
    assert(Wire.reads.back() == 0x40); // Release lock after short read.
    Wire.shortBurst = false;
    counter(6); imu.update();
    assert(imu.isYawValid() && imu.lastGyroReadOk());
    assert(imu.getAccelMagnitude() == 0 && imu.getTiltRate() == 0);
    imu.setLoggingMetricsEnabled(true);
    counter(7); imu.update();
    assert(std::fabs(imu.getAccelMagnitude() - 1.5f) < 0.0001f);
    assert(imu.getTiltRate() > 100);
    imu.setLoggingMetricsEnabled(false);
    axis(0x35, 4096);
    counter(8); imu.update();
    assert(imu.getAccelMagnitude() == 0 && imu.getSurfaceDisturbanceScore() == 0);
    assert(imu.getAccelDelta() > 0); // Parked detection still receives motion.
    assert(imu.setGyroLpfMode(1) && Wire.registers[6] == 0x70);
    assert(imu.setGyroLpfMode(2) && Wire.registers[6] == 0);
    hostLockingFails = true;
    IMU fallback;
    assert(fallback.begin(true));
    assert(fallback.getGyroLpfMode() == 2 && Wire.registers[6] == 0);
    assert(!fallback.getSampleDiagnostics().locked);
    assert(Wire.registers[8] == 3); // Both sensors enabled, sync cleared.
    counter(12);
    fallback.update();
    assert(fallback.lastGyroReadOk());
    assert(fallback.getYawRate() == -200.0f);
    assert(fallback.setGyroLpfMode(1));
    IMU normal;
    assert(normal.begin()); // No failed locking attempt is needed by default.
    assert(normal.getGyroLpfMode() == 2 && Wire.registers[6] == 0);
    assert(!normal.getSampleDiagnostics().locked);
    std::puts("IMU host tests passed");
}
