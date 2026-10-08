#include "IMU.h"

#if defined(OPENDRIFT_IMU_MPU6050)
// InvenSense RM-MPU-6000A-00. No DMP/FIFO batching: read the newest
// accel/temperature/gyro shadow-register block at every control tick.
namespace {
uint8_t filterConfig(uint8_t mode) { return mode == 0 ? 4 : (mode == 1 ? 2 : 0); }
uint8_t sampleDivider(uint8_t mode) { return mode == 2 ? 7 : 0; }
}

bool IMU::writeRegister(uint8_t address, uint8_t value)
{
    Wire.beginTransmission(mpuAddress);
    Wire.write(address);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool IMU::readRegisters(uint8_t address, uint8_t* buffer, size_t length)
{
    Wire.beginTransmission(mpuAddress);
    Wire.write(address);
    if(Wire.endTransmission(false) != 0) return false;
    if(Wire.requestFrom(mpuAddress, length) != length)
    {
        while(Wire.available()) Wire.read();
        return false;
    }
    for(size_t i = 0; i < length; ++i) buffer[i] = Wire.read();
    return true;
}

bool IMU::verifyGyroConfiguration(uint8_t mode)
{
    uint8_t config[4], power[2], identity;
    return readRegisters(0x75, &identity, 1) && identity == 0x68 &&
        readRegisters(0x19, config, sizeof(config)) &&
        config[0] == sampleDivider(mode) && config[1] == filterConfig(mode) &&
        config[2] == 0x10 && config[3] == 0x08 &&
        readRegisters(0x6B, power, sizeof(power)) && power[0] == 0x01 && power[1] == 0;
}

bool IMU::begin(bool tryLocking)
{
    (void)tryLocking; // QMI locking is not a supported MPU6050 option.
    mpuConfigured = false;
    gyroReadFailures = 3;
    accelReadFailures = 25;
    gyroReadOk = false;
    diagnostics = {};
    counterReady = false;
    lastUpdateMicros = 0;
    accelFilterReady = false;
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);
    Wire.setTimeOut(2); // Bound bus faults; three bad ticks invalidate yaw.
    bool found = false;
    for(uint8_t address : {uint8_t(0x68), uint8_t(0x69)})
    {
        mpuAddress = address;
        uint8_t identity = 0;
        if(readRegisters(0x75, &identity, 1) && identity == 0x68)
        {
            found = true;
            break;
        }
    }
    if(!found) { Serial.println("MPU6050: WHO_AM_I probe failed at 0x68/0x69"); return false; }
    if(!writeRegister(0x6B, 0x80)) return false;
    delay(100);
    // PLL X gyro clock, all axes on, FIFO/DMP/I2C master off, data-ready
    // status enabled without requiring an external interrupt wire.
    if(!writeRegister(0x6B, 0x01) || !writeRegister(0x6C, 0) ||
       !writeRegister(0x6A, 0) || !writeRegister(0x23, 0) ||
       !writeRegister(0x37, 0) || !writeRegister(0x38, 1) ||
       !writeRegister(0x1B, 0x10) || !writeRegister(0x1C, 0x08) ||
       !writeRegister(0x1A, filterConfig(0)) || !writeRegister(0x19, sampleDivider(0))) return false;
    delay(100);
    if(!verifyGyroConfiguration(0)) return false;
    mpuConfigured = true;
    gyroLpfMode = 0;
    lpfRetryMode = 0;
    lpfRetryCount = 0;
    lpfLastAttemptMs = 0;
    settleUntilMs = millis() + GYRO_SETTLE_MS;
    Serial.printf("MPU6050: address=0x%02X, +/-1000 dps, +/-4 g, 1000 Hz, LPF 20 Hz\n", mpuAddress);
    return true;
}

bool IMU::setGyroLpfMode(uint8_t mode)
{
    mode = constrain(mode, 0, 2);
    if(mode == gyroLpfMode && mpuConfigured) return true;
    if(mode != lpfRetryMode) { lpfRetryMode = mode; lpfRetryCount = 0; }
    if(lpfRetryCount >= 5 || (lpfRetryCount && millis() - lpfLastAttemptMs < 1000)) return false;
    // Invalidate feedback throughout any partially applied configuration.
    gyroReadFailures = 3;
    gyroReadOk = false;
    mpuConfigured = false;
    const bool ok = writeRegister(0x1A, filterConfig(mode)) &&
        writeRegister(0x19, sampleDivider(mode)) && verifyGyroConfiguration(mode);
    if(!ok)
    {
        ++lpfRetryCount;
        lpfLastAttemptMs = millis();
        return false;
    }
    gyroLpfMode = mode;
    mpuConfigured = true;
    lpfRetryCount = 0;
    settleUntilMs = millis() + GYRO_SETTLE_MS;
    Serial.printf("MPU6050: LPF %u Hz, sample output 1000 Hz\n", mode == 0 ? 20 : (mode == 1 ? 98 : 256));
    return true;
}

bool IMU::readCoherentSample()
{
    const uint32_t started = micros();
    diagnostics.fresh = false;
    diagnostics.counterDelta = 0;
    if(!mpuConfigured)
    {
        diagnostics.readUs = micros() - started;
        return false;
    }
    uint8_t status;
    if(!readRegisters(0x3A, &status, 1))
    {
        ++diagnostics.readErrors;
        diagnostics.readUs = micros() - started;
        return false;
    }
    if(!(status & 1))
    {
        ++diagnostics.noDataCount;
        diagnostics.readUs = micros() - started;
        return true;
    }
    uint8_t bytes[14];
    if(!readRegisters(0x3B, bytes, sizeof(bytes)))
    {
        ++diagnostics.readErrors;
        diagnostics.readUs = micros() - started;
        return false;
    }
    auto raw = [&bytes](uint8_t offset) -> int16_t {
        return static_cast<int16_t>((uint16_t(bytes[offset]) << 8) | bytes[offset + 1]);
    };
    accelX = raw(0) / 8192.0f;
    accelY = raw(2) / 8192.0f;
    accelZ = raw(4) / 8192.0f;
    gyroX = raw(8) / 32.8f;
    gyroY = raw(10) / 32.8f;
    gyroZ = raw(12) / 32.8f;
    // MPU6050 has no QMI timestamp counter. This counts accepted bursts,
    // not elapsed sensor samples; skipped 1 kHz samples cannot be inferred.
    ++diagnostics.sampleCounter;
    diagnostics.counterDelta = counterReady ? 1 : 0;
    counterReady = true;
    diagnostics.fresh = true;
    diagnostics.locked = false;
    diagnostics.readUs = micros() - started;
    return true;
}
#endif
