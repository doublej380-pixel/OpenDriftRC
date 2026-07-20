#pragma once

#include <Arduino.h>


class GyroController
{
public:

    bool begin();


    int update(
        float yawRate,
        int throttlePulse = 1500,
        bool throttleSignal = false
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

    void setIntegralGain(
        float value
    );

    float getIntegralGain();

    void setIntegralLimit(
        int value
    );

    int getIntegralLimit();

    int getIntegralCorrection();

    void setHoldBoost(
        int value
    );

    int getHoldBoost();

    void setAntiWobble(
        int value
    );

    int getAntiWobble();

    void setHuntDamping(
        int value
    );

    int getHuntDamping();

    float getHuntControlYaw();

    float getHuntSlowYaw();

    float getHuntFastYaw();

    float getHuntBlend();

    float getHuntScore();

    int getControlPhase();

    float getSettledBlend();

    float getThrottleTransient();

    float getActiveHoldFactor();


    float getFilteredYaw();


    int getServoOutput();



private:

    float gyroGain = 1.5f;


    float gyroOffset = 0;


    float deadband = 2.0f;

    float smoothing = 0.10f;

    int maxCorrection = 250;

    float integralGain = 0;

    int integralLimit = 120;

    int holdBoost = 0;

    int antiWobble = 50;

    int huntDamping = 0;


    float filteredYaw = 0;

    float integralAccumulator = 0;

    int integralCorrection = 0;

    float holdBoostFiltered = 0;

    float huntControlYaw = 0;

    float huntSlowYaw = 0;

    float huntFastYaw = 0;

    float huntBlend = 0;

    float huntScore = 0;

    float huntReversalAge = 10.0f;

    int8_t huntFastDirection = 0;

    int8_t driftDirection = 0;

    float directionTime = 0;

    float transitionTime = 0;

    uint8_t controlPhase = 0;

    float settledBlend = 0;

    float throttleTransientTime = 0;

    int lastThrottlePulse = 1500;

    bool throttleReady = false;

    float activeHoldFactor = 1.0f;


    int servoOutput = 1500;

    int correctionOutput = 0;

    bool calibrated = false;

    uint32_t lastUpdateMicros = 0;

    void resetDynamicState();

};
