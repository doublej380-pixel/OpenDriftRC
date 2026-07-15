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


private:

    SensorQMI8658 qmi;

    float gyroX = 0;
    float gyroY = 0;
    float gyroZ = 0;


    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    static constexpr int SDA_PIN = 47;
    static constexpr int SCL_PIN = 48;
    #else
    static constexpr int SDA_PIN = 6;
    static constexpr int SCL_PIN = 7;
    #endif
};
