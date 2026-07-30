#pragma once

#include <Arduino.h>


class GyroController
{
public:

    bool begin();

    int update(
        float yawRate,
        int steeringCommand = 1500,
        bool steeringSignal = false,
        int throttlePulse = 1500,
        bool throttleSignal = false,
        float surfaceDisturbance = 0.0f,
        bool terrainAssistEnabled = true
    );

    void calibrate(float yawRate);

    void setGain(float gain);
    float getGain();

    void setDeadband(float deadband);
    float getDeadband();

    void setSmoothing(float smoothing);
    float getSmoothing();

    void setMaxCorrection(int correction);
    int getMaxCorrection();
    int getCorrection();

    // V2 keeps the existing setting names for profile compatibility.
    // Drift Memory is now equilibrium-error feedback, never an integral of
    // raw yaw. Drift Memory Limit caps that feedback.
    void setIntegralGain(float value);
    float getIntegralGain();
    void setIntegralLimit(int value);
    int getIntegralLimit();
    int getIntegralCorrection();

    // Hold Assist controls how firmly a quiet drift reference is retained.
    void setHoldBoost(int value);
    int getHoldBoost();

    // Adds only steady-state countersteer from the learned drift reference.
    // Zero preserves the base V2 response; 100 adds up to one additional
    // copy of the steady direct correction without increasing fast damping.
    void setCounterSteerAssist(int value);
    int getCounterSteerAssist();
    int getCounterSteerCorrection();

    // Experimental RC1 rotation-speed trim. While the driver is actively
    // moving the steering, lower values add damping and higher values let
    // commanded chassis rotation pass more freely. 50 preserves RC1 exactly.
    void setTailSlideSpeed(int value);
    int getTailSlideSpeed();
    float getTailSlideBlend();

    // Retained for profile/UI compatibility. V2 does not use a dead-zone
    // output holder in the primary feedback path.
    void setAntiWobble(int value);
    int getAntiWobble();

    // Hunt Damping is now a continuous, non-detecting prediction strength.
    void setHuntDamping(int value);
    int getHuntDamping();

    // Compatibility no-op. V2 has no second reactive chatter filter.
    int applyOutputChatterDamping(
        int correction,
        int steeringCommand,
        bool steeringSignal,
        float dt
    );

    float getHuntControlYaw();
    float getHuntSlowYaw();
    float getHuntFastYaw();
    float getHuntBlend();
    float getHuntScore();

    float getOutputChatterSlow();
    float getOutputChatterFast();
    float getOutputChatterBlend();
    float getOutputChatterScore();
    float getSteeringActivity();

    int getControlPhase();
    float getSettledBlend();
    float getThrottleTransient();
    bool getTerrainActive();
    float getTerrainAssist();
    float getActiveHoldFactor();

    float getFilteredYaw();
    int getServoOutput();


private:

    float gyroGain = 1.5f;
    float gyroOffset = 0.0f;
    float deadband = 2.0f;
    float smoothing = 0.10f;
    int maxCorrection = 250;

    float integralGain = 0.0f;
    int integralLimit = 120;
    int holdBoost = 0;
    int counterSteerAssist = 0;
    int tailSlideSpeed = 50;
    int antiWobble = 50;
    int huntDamping = 0;

    float filteredYaw = 0.0f;
    float previousFilteredYaw = 0.0f;
    float filteredYawAcceleration = 0.0f;

    float driftReferenceYaw = 0.0f;
    bool driftReferenceReady = false;
    int8_t driftDirection = 0;
    float transitionTime = 0.0f;

    float integralAccumulator = 0.0f;
    int integralCorrection = 0;
    int counterSteerCorrection = 0;

    float steeringActivity = 0.0f;
    float tailSlideBlend = 0.0f;
    int lastSteeringCommand = 1500;
    bool steeringReady = false;

    float throttleRate = 0.0f;
    float throttleTransientTime = 0.0f;
    int lastThrottlePulse = 1500;
    bool throttleReady = false;

    uint8_t controlPhase = 0;
    float settledBlend = 0.0f;

    // Existing telemetry accessors are mapped onto V2 signals so old UI and
    // blackbox consumers remain source-compatible.
    float huntControlYaw = 0.0f;
    float huntSlowYaw = 0.0f;
    float huntFastYaw = 0.0f;
    float huntBlend = 0.0f;
    float huntScore = 0.0f;

    float outputChatterSlow = 0.0f;
    float outputChatterFast = 0.0f;
    float outputChatterBlend = 0.0f;
    float outputChatterScore = 0.0f;

    float terrainAssistBlend = 0.0f;
    float activeHoldFactor = 1.0f;

    int servoOutput = 1500;
    int correctionOutput = 0;

    bool calibrated = false;
    uint32_t lastUpdateMicros = 0;

    void resetDynamicState();
};
