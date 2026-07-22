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
        uint32_t version = 1;
        char name[PROFILE_NAME_LENGTH] = {0};

        float gain = 1.5f;
        float deadband = 2.0f;
        float gyroSmoothing = 0.10f;
        float gyroIntegralGain = 0.0f;

        int32_t gyroMaxCorrection = 250;
        int32_t gyroAttackSpeed = 80;
        int32_t gyroReturnSpeed = 30;
        int32_t gyroIntegralLimit = 120;
        int32_t gyroHoldBoost = 0;
        int32_t gyroAntiWobble = 50;
        int32_t gyroHuntDamping = 0;
        int32_t steeringDamper = 0;
        int32_t radioSteeringTravel = 100;
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

    int getGyroAttackSpeed();
    void setGyroAttackSpeed(int value);

    int getGyroReturnSpeed();
    void setGyroReturnSpeed(int value);

    float getGyroIntegralGain();
    void setGyroIntegralGain(float value);

    int getGyroIntegralLimit();
    void setGyroIntegralLimit(int value);

    int getGyroHoldBoost();
    void setGyroHoldBoost(int value);

    int getGyroAntiWobble();
    void setGyroAntiWobble(int value);

    int getGyroHuntDamping();
    void setGyroHuntDamping(int value);

    int getSteeringDamper();
    void setSteeringDamper(int value);

    bool getTerrainAssistEnabled();
    void setTerrainAssistEnabled(bool value);

    // Servo
    int getServoCenter();
    void setServoCenter(int value);

    bool getServoReverse();
    void setServoReverse(bool value);

    int getServoTravel();
    void setServoTravel(int value);

    int getServoQuiet();
    void setServoQuiet(int value);

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

    int getRadioSteeringTravel();
    void setRadioSteeringTravel(int value);

    int getGainMin();
    void setGainMin(int value);

    int getGainMax();
    void setGainMax(int value);

    bool getThrottleOutputEnabled();
    void setThrottleOutputEnabled(bool value);

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

    int gyroMaxCorrection = 250;

    float gyroSmoothing = 0.10f;

    int gyroAttackSpeed = 80;

    int gyroReturnSpeed = 30;

    float gyroIntegralGain = 0.0f;

    int gyroIntegralLimit = 120;

    int gyroHoldBoost = 0;

    int gyroAntiWobble = 50;

    int gyroHuntDamping = 0;

    int steeringDamper = 0;

    bool terrainAssistEnabled = true;

    int servoCenter = 1500;

    bool servoReverse = false;

    int servoTravel = 100;

    int servoQuiet = 0;

    bool wifiEnabled = true;

    uint32_t wifiTimeout = 40000;

    bool blackboxEnabled = false;

    int steeringMin = 1000;

    int steeringCenter = 1500;

    int steeringMax = 2000;

    int radioSteeringTravel = 100;

    int gainMin = 1000;

    int gainMax = 2000;

    bool throttleOutputEnabled = false;

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
