#pragma once

#include <Arduino.h>
#include <Wire.h>
#if !defined(OPENDRIFT_IMU_MPU6050)
#include "SensorQMI8658.hpp"
#endif


class IMU
{
public:

    // Locked acquisition remains experimental; normal boots use the validated
    // asynchronous track baseline without retrying the failed CTRL9 handshake.
    bool begin(bool tryLocking = false);

    bool setGyroLpfMode(uint8_t mode);
    uint8_t getGyroLpfMode() const;

    void update();

    bool isYawValid() const;
    bool lastGyroReadOk() const;
    bool isHealthy() const;
    bool isAccelHealthy() const;

    struct SampleDiagnostics
    {
        uint32_t sampleCounter = 0;
        uint32_t counterDelta = 0;
        uint32_t readUs = 0;
        uint32_t readErrors = 0;
        uint32_t noDataCount = 0;
        bool fresh = false;
        bool locked = false;
    };
    const SampleDiagnostics& getSampleDiagnostics() const { return diagnostics; }


    float getGyroX();
    float getGyroY();
    float getYawRate();

    float getAccelX();
    float getAccelY();
    float getAccelZ();
    float getAccelMagnitude();
    float getAccelDelta();
    float getTiltRate();
    float getSurfaceDisturbanceScore();


private:

    #if defined(OPENDRIFT_IMU_MPU6050)
    uint8_t mpuAddress = 0x68;
    bool mpuConfigured = false;
    bool writeRegister(uint8_t address, uint8_t value);
    #else
    SensorQMI8658 qmi;
    #endif
    SampleDiagnostics diagnostics;
    bool counterReady = false;
    bool configureLocking();
    bool verifyGyroConfiguration(uint8_t mode);
    bool readRegisters(uint8_t address, uint8_t* buffer, size_t length);
    bool readCoherentSample();

    float gyroX = 0;
    float gyroY = 0;
    float gyroZ = 0;

    float accelX = 0;
    float accelY = 0;
    float accelZ = 0;

    float slowAccelX = 0;
    float slowAccelY = 0;
    float slowAccelZ = 0;

    float accelMagnitude = 0;
    float accelDelta = 0;
    float tiltRate = 0;
    float surfaceDisturbanceScore = 0;

    bool accelFilterReady = false;

    uint8_t gyroReadFailures = 0;
    bool gyroReadOk = false;
    uint8_t accelReadFailures = 0;

    uint8_t gyroLpfMode = 0;
    uint8_t lpfRetryMode = 0;
    uint8_t lpfRetryCount = 0;
    uint32_t lpfLastAttemptMs = 0;
    uint32_t enableRetryMs = 0;
    uint32_t settleUntilMs = 0;

    static constexpr uint32_t GYRO_SETTLE_MS = 60;

    bool isSettling() const;

    uint32_t lastUpdateMicros = 0;


    #if defined(OPENDRIFT_BOARD_MATRIX) || defined(OPENDRIFT_BOARD_ZERO)
    static constexpr int SDA_PIN = 11;
    static constexpr int SCL_PIN = 12;
    #elif defined(OPENDRIFT_BOARD_AMOLED_164)
    static constexpr int SDA_PIN = 47;
    static constexpr int SCL_PIN = 48;
    #else
    static constexpr int SDA_PIN = 6;
    static constexpr int SCL_PIN = 7;
    #endif
};
