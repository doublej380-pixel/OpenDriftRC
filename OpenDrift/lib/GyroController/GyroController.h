#pragma once

#include <Arduino.h>

class GyroController
{
public:

    bool begin();

    int update(float yawRate);

    void setGain(float gain);

    float getGain();

    int getServoOutput();

private:

    float gyroGain = 1.5f;

    int servoOutput = 1500;
};