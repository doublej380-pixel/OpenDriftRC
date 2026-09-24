#pragma once

#include <Arduino.h>
#include <Preferences.h>

class Settings
{
public:

    static constexpr uint8_t MAX_PROFILES = 12;
    static constexpr size_t PROFILE_NAME_LENGTH = 24;

    struct DrivingProfile
    {
        uint32_t version = 11;
        char name[PROFILE_NAME_LENGTH] = {0};

        float gain = 1.5f;
        float deadband = 2.0f;
        float gyroSmoothing = 0.10f;
        float gyroIntegralGain = 0.0f;

        int32_t gyroMaxCorrection = 25;
        int32_t gyroIntegralLimit = 120;
        int32_t gyroHoldBoost = 0;
        int32_t predictionStrength = 0;
        int32_t radioSteeringTravel = 100;
        int32_t gyroCounterSteerAssist = 0;
        int32_t gyroTransitionSpeed = 50;
        int32_t gyroHuntStrength = 50;

        float pca = 0.0f;
    };

    bool begin();

    void update();

    // Gyro
    float getGain();
    void setGain(float value);

    float getDeadband();
    void setDeadband(float value);

    bool getGyroReverse();
    void setGyroReverse(bool value);

    int getGyroMaxCorrection();
    void setGyroMaxCorrection(int value);

    float getGyroSmoothing();
    void setGyroSmoothing(float value);

    // 0 = 24 Hz (QMI mode 0), 1 = 120 Hz (mode 3), 2 = hardware LPF off.
    uint8_t getGyroLpfMode();
    void setGyroLpfMode(uint8_t value);

    float getGyroIntegralGain();
    void setGyroIntegralGain(float value);

    int getGyroIntegralLimit();
    void setGyroIntegralLimit(int value);

    int getGyroHoldBoost();
    void setGyroHoldBoost(int value);

    int getGyroCounterSteerAssist();
    void setGyroCounterSteerAssist(int value);

    int getGyroTransitionSpeed();
    void setGyroTransitionSpeed(int value);

    int getPredictionStrength();
    void setPredictionStrength(int value);

    int getGyroHuntStrength();
    void setGyroHuntStrength(int value);

    float getpca();
    void setpca(float value);

    // Servo
    int getServoCenter();
    void setServoCenter(int value);

    bool getServoReverse();
    void setServoReverse(bool value);

    int getServoTravel();
    void setServoTravel(int value);

    int getServoQuiet();
    void setServoQuiet(int value);

    uint16_t getControlLoopHz();
    void setControlLoopHz(uint16_t value);

    // WiFi
    bool getWifiEnabled();
    void setWifiEnabled(bool value);

    uint32_t getWifiTimeout();
    void setWifiTimeout(uint32_t value);

    // Blackbox
    bool getBlackboxEnabled();
    void setBlackboxEnabled(bool value);

    // Radio
    int getSteeringMin();
    void setSteeringMin(int value);

    int getSteeringCenter();
    void setSteeringCenter(int value);

    int getSteeringMax();
    void setSteeringMax(int value);

    uint8_t getSteeringCalibrationMask();
    bool isSteeringCalibrated();
    int getSteeringCapturedPulse(uint8_t point);
    int getSteeringCapturedInputPulse(uint8_t point);
    bool captureSteeringCalibrationPoint(
        uint8_t point,
        int physicalPulse,
        int inputPulse = 0
    );
    bool confirmStoredSteeringCalibration();
    void clearSteeringCalibration();

    int getRadioSteeringTravel();
    void setRadioSteeringTravel(int value);

    int getGainMin();
    void setGainMin(int value);

    int getGainMax();
    void setGainMax(int value);

    float getChannel3GainMin();
    void setChannel3GainMin(float value);

    float getChannel3GainMax();
    void setChannel3GainMax(float value);

    bool getThrottleOutputEnabled();
    void setThrottleOutputEnabled(bool value);

    // CRSF auxiliary receiver-style PWM outputs. A value of zero disables
    // the GPIO; values 1-16 select the corresponding CRSF radio channel.
    uint8_t getAuxChannelForGpio(uint8_t gpio);
    void setAuxChannelForGpio(uint8_t gpio, uint8_t channel);

    // Driving profiles
    uint8_t getProfileCount();
    int8_t getActiveProfileIndex();
    const char* getActiveProfileName();
    const DrivingProfile* getProfile(uint8_t index);

    int8_t createProfile(const String& name);
    bool activateProfile(uint8_t index);
    bool deleteProfile(uint8_t index);

private:

    Preferences prefs;

    bool dirty = false;

    unsigned long lastSave = 0;

    // Stored values

    float gain = 1.5f;

    float deadband = 2.0f;

    bool gyroReverse = false;

    int gyroMaxCorrection = 25;

    float gyroSmoothing = 0.10f;

    uint8_t gyroLpfMode = 0;

    float gyroIntegralGain = 0.0f;

    int gyroIntegralLimit = 120;

    int gyroHoldBoost = 0;

    int gyroCounterSteerAssist = 0;

    int gyroTransitionSpeed = 50;

    int predictionStrength = 0;

    int gyroHuntStrength = 50;

    float pca = 0.0f;

    int servoCenter = 1500;

    bool servoReverse = false;

    int servoTravel = 100;

    int servoQuiet = 0;

    uint16_t controlLoopHz = 250;

    bool wifiEnabled = true;

    uint32_t wifiTimeout = 40000;

    bool blackboxEnabled = false;

    int steeringMin = 1000;

    int steeringCenter = 1500;

    int steeringMax = 2000;

    uint8_t steeringCalibrationMask = 0;

    int steeringCapturedPulses[3] = {1000, 1500, 2000};

    int steeringCapturedInputPulses[3] = {1000, 1500, 2000};

    int radioSteeringTravel = 100;

    int gainMin = 1000;

    int gainMax = 2000;

    float channel3GainMin = 0.5f;

    float channel3GainMax = 3.0f;

    bool throttleOutputEnabled = false;

    uint8_t auxChannels[8] = {0};

    DrivingProfile profiles[MAX_PROFILES];

    uint8_t profileCount = 0;

    int8_t activeProfileIndex = -1;

    void save();

    void loadProfiles();
    void captureProfile(DrivingProfile& profile);
    void applyProfile(const DrivingProfile& profile);
    bool persistProfile(uint8_t index);
    String sanitizeProfileName(const String& name);
};