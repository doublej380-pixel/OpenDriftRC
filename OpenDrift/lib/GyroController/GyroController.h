#pragma once

#include <Arduino.h>


class GyroController
{
public:

    bool begin();

    int update(float yawRate);


    void calibrate(float yawRate);


    void setGain(float gain);

    float getGain();


    void setDeadband(float deadband);

    float getFilteredYaw();


    int getServoOutput();



private:

    float gyroGain = 1.5f;


    float gyroOffset = 0;


    float deadband = 2.0f;


    float filteredYaw = 0;


    int servoOutput = 1500;


    bool calibrated = false;
};