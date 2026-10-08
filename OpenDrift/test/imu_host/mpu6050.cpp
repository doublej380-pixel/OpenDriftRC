#include "IMU.h"
#include <cassert>
#include <cmath>
#include <cstdio>
uint32_t hostTime = 100000;
FakeWire Wire;
static void axis(uint8_t address, int16_t value)
{
    Wire.registers[address] = uint16_t(value) >> 8;
    Wire.registers[address + 1] = uint16_t(value) & 255;
}
int main()
{
    IMU missing;
    assert(!missing.begin());
    assert(!missing.isYawValid());
    Wire.registers[0x75] = 0x68;
    Wire.ackDevice = 0x69; // Address fallback; identity stays 0x68 with AD0 high.
    IMU imu;
    assert(imu.begin());
    assert(!imu.isYawValid()); // No successful sample yet.
    assert(Wire.registers[0x1B] == 0x10 && Wire.registers[0x1C] == 0x08);
    assert(Wire.registers[0x1A] == 4 && Wire.registers[0x19] == 0);
    hostTime += 100000;
    Wire.registers[0x3A] = 1;
    axis(0x3B, 8192); axis(0x3D, -8192); axis(0x3F, 4096);
    axis(0x43, -3280); axis(0x45, 1640); axis(0x47, -6560);
    imu.update();
    assert(imu.isYawValid() && imu.lastGyroReadOk());
    assert(std::fabs(imu.getYawRate() + 200) < 0.001f);
    assert(std::fabs(imu.getGyroX() + 100) < 0.001f);
    assert(std::fabs(imu.getGyroY() - 50) < 0.001f);
    assert(imu.getAccelX() == 1 && imu.getAccelY() == -1 && imu.getAccelZ() == 0.5f);
    assert(Wire.reads.back() == 0x3B);
    assert(imu.getSampleDiagnostics().sampleCounter == 1 && !imu.getSampleDiagnostics().locked);
    Wire.registers[0x3A] = 0;
    imu.update(); imu.update(); imu.update();
    assert(!imu.isYawValid() && imu.getSampleDiagnostics().noDataCount == 3);
    Wire.registers[0x3A] = 1;
    imu.update();
    assert(imu.isYawValid());
    const float previousYaw = imu.getYawRate();
    Wire.shortBurst = true; Wire.shortLength = 14;
    imu.update();
    assert(!imu.lastGyroReadOk() && imu.getSampleDiagnostics().readErrors == 1);
    assert(imu.getYawRate() == previousYaw); // Never publish a partial sample.
    Wire.shortBurst = false;
    assert(imu.setGyroLpfMode(1));
    assert(Wire.registers[0x1A] == 2 && Wire.registers[0x19] == 0);
    assert(!imu.isYawValid()); // Settling guard.
    hostTime += 100000; imu.update();
    assert(imu.isYawValid());
    assert(imu.setGyroLpfMode(2));
    assert(Wire.registers[0x1A] == 0 && Wire.registers[0x19] == 7);
    Wire.failWrites = true;
    assert(!imu.setGyroLpfMode(0));
    hostTime += 100000;
    imu.update(); imu.update(); imu.update();
    assert(!imu.isYawValid()); // Failed configuration cannot become healthy.
    Wire.failWrites = false;
    hostTime += 1100000;
    assert(imu.setGyroLpfMode(2)); // Can restore even the last known mode.
    hostTime += 100000; imu.update();
    assert(imu.isYawValid());
    // Configuration readback must reject incorrect sensor identity/range.
    Wire.registers[0x75] = 0x70;
    assert(!imu.setGyroLpfMode(1));
    assert(!imu.isYawValid());
    std::puts("MPU6050 host tests passed");
}
