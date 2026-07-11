#pragma once

#include <Arduino.h>


class GyroController
{
public:

    bool begin();


    int update(
        float yawRate
    );


    void calibrate(
        float yawRate
    );



    void setGain(
        float gain
    );


    float getGain();




    void setDeadband(
        float deadband
    );


    float getDeadband();   // NEW

    void setSmoothing(
        float smoothing
    );

    float getSmoothing();

    void setMaxCorrection(
        int correction
    );

    int getMaxCorrection();

    int getCorrection();




    float getFilteredYaw();


    int getServoOutput();



private:

    float gyroGain = 1.5f;


    float gyroOffset = 0;


    float deadband = 2.0f;

    float smoothing = 0.10f;

    int maxCorrection = 250;


    float filteredYaw = 0;


    int servoOutput = 1500;

    int correctionOutput = 0;


    bool calibrated = false;

    uint32_t lastUpdateMicros = 0;

};
