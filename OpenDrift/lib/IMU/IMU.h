#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "SensorQMI8658.hpp"


class IMU
{
public:

    bool begin();

    void update();


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

    SensorQMI8658 qmi;

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

    uint32_t lastUpdateMicros = 0;


    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    static constexpr int SDA_PIN = 47;
    static constexpr int SCL_PIN = 48;
    #else
    static constexpr int SDA_PIN = 6;
    static constexpr int SCL_PIN = 7;
    #endif
};
