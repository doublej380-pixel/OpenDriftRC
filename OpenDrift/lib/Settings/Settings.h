#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "ParameterCatalog.h"
#include "ParameterStore.h"

class Settings
{
public:

    static constexpr uint8_t MAX_PROFILES = 12;
    static constexpr size_t PROFILE_NAME_LENGTH = 24;
    static constexpr size_t BACKGROUND_NAME_LENGTH = 24;

    struct DrivingProfile
    {
        uint32_t version = 12;
        char name[PROFILE_NAME_LENGTH] = {0};
        float values[ParameterStore::VALUE_COUNT] = {0.0f};
    };

    struct SteeringCalibration
    {
        bool calibrated = false;
        uint8_t mask = 0;
        int min = 1000;
        int center = 1500;
        int max = 2000;
        int inputMin = 1000;
        int inputCenter = 1500;
        int inputMax = 2000;
    };

    struct ControllerSnapshot
    {
        float gain;
        float deadband;
        float smoothing;
        float driftMemory;
        int maxCorrection;
        int memoryLimit;
        int holdAssist;
        int countersteer;
        int transitionSpeed;
        int prediction;
        int driverPriority;
        int antiWobble;
        uint8_t antiWobbleScale;
        uint8_t gyroLpfMode;
        bool gyroReverse;
    };

    bool begin();

    void update();

    void factoryReset();

    float getParameterValue(OpenDriftParameters::Id id) const;
    bool setParameterValue(OpenDriftParameters::Id id, float value);
    uint32_t getParameterGeneration() const;
    bool getControllerSnapshot(
        uint32_t& lastGeneration,
        ControllerSnapshot& snapshot
    ) const;

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

    // Reduces only the fast direct gyro path as the driver's steering command
    // moves away from center. Zero preserves the existing controller exactly.
    int getDriverPriority();
    void setDriverPriority(int value);

    // 0 = 1/10 scale steering resonance, 1 = micro scale resonance.
    uint8_t getAntiWobbleScale();
    void setAntiWobbleScale(uint8_t value);

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

    // Regenerated receiver/ESC PWM rate. 50 Hz is the compatibility default;
    // 250/333 Hz are opt-in for ESCs that explicitly support high-rate PWM.
    uint16_t getThrottleOutputHz();
    void setThrottleOutputHz(uint16_t value);

    // Display orientation is stored as clockwise quarter turns. Matrix builds
    // support all four positions; AMOLED builds support normal (0) and the
    // physically useful 180-degree flip (2).
    uint8_t getDisplayRotation();
    void setDisplayRotation(uint8_t value);

    uint8_t getDisplayBrightness();
    void setDisplayBrightness(int value);
    uint16_t getDisplayDimTimeout();
    void setDisplayDimTimeout(int value);

    // AMOLED theme. Text 0 = light text, 1 = dark text for light
    // backgrounds. Accent 0 preserves the original mixed page colours.
    static constexpr uint8_t THEME_ACCENT_COUNT = 7;
    static const char* themeAccentName(uint8_t accent);
    uint8_t getThemeText();
    void setThemeText(int value);
    uint8_t getThemeAccent();
    void setThemeAccent(int value);

    // Empty selects the immutable background compiled into the firmware.
    const char* getBackgroundName();
    void setBackgroundName(const String& value);
    static String sanitizeBackgroundName(const String& value);

    // WiFi
    bool getWifiEnabled();
    void setWifiEnabled(bool value);

    uint32_t getWifiTimeout();
    void setWifiTimeout(uint32_t value);

    const char* getWifiSsid();
    void setWifiSsid(const String& value);

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
    void getSteeringCalibration(SteeringCalibration& out);
    int getSteeringCapturedPulse(uint8_t point);
    int getSteeringCapturedInputPulse(uint8_t point);
    bool captureSteeringCalibrationPoint(
        uint8_t point,
        int physicalPulse,
        int inputPulse = 0
    );
    bool confirmStoredSteeringCalibration();
    bool setStoredSteeringEndpoints(
        int minimum,
        int center,
        int maximum
    );
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
    float getProfileValue(
        const DrivingProfile& profile,
        OpenDriftParameters::Id id
    ) const;

    int8_t createProfile(const String& name);
    bool activateProfile(uint8_t index);
    bool deleteProfile(uint8_t index);

private:

    portMUX_TYPE settingsMux = portMUX_INITIALIZER_UNLOCKED;

    Preferences prefs;

    ParameterStore parameterStore;

    bool dirty = false;

    unsigned long lastSave = 0;

    // Stored values

    float gain = OpenDriftParameters::Defaults::GYRO_GAIN;

    float deadband = OpenDriftParameters::Defaults::DEADBAND;

    bool gyroReverse = false;

    int gyroMaxCorrection = OpenDriftParameters::Defaults::MAX_CORRECTION;

    float gyroSmoothing = OpenDriftParameters::Defaults::SMOOTHING;

    uint8_t gyroLpfMode = 0;

    float gyroIntegralGain = OpenDriftParameters::Defaults::DRIFT_MEMORY;

    int gyroIntegralLimit = OpenDriftParameters::Defaults::MEMORY_LIMIT;

    int gyroHoldBoost = OpenDriftParameters::Defaults::HOLD_ASSIST;

    int gyroCounterSteerAssist = OpenDriftParameters::Defaults::COUNTERSTEER;

    int gyroTransitionSpeed = OpenDriftParameters::Defaults::TRANSITION_SPEED;

    int predictionStrength = OpenDriftParameters::Defaults::PREDICTION;

    int gyroHuntStrength = OpenDriftParameters::Defaults::ANTI_WOBBLE;

    int driverPriority = OpenDriftParameters::Defaults::DRIVER_PRIORITY;

    uint8_t antiWobbleScale = 0;

    int servoCenter = OpenDriftParameters::Defaults::SERVO_CENTER;

    bool servoReverse = false;

    int servoTravel = OpenDriftParameters::Defaults::SERVO_TRAVEL;

    int servoQuiet = OpenDriftParameters::Defaults::SERVO_QUIET;

    uint16_t controlLoopHz = 250;

    uint16_t throttleOutputHz = 50;

    uint8_t displayRotation =
        #if defined(OPENDRIFT_BOARD_MATRIX)
        3;
        #else
        0;
        #endif

    uint8_t displayBrightness = 100;
    uint16_t displayDimTimeout = 0;

    uint8_t themeText = 0;
    uint8_t themeAccent = 0;

    char backgroundName[BACKGROUND_NAME_LENGTH] = {0};

    bool wifiEnabled = true;

    uint32_t wifiTimeout = 40000;

    static constexpr size_t WIFI_SSID_LENGTH = 33;
    char wifiSsid[WIFI_SSID_LENGTH] = "OpenDrift";

    bool blackboxEnabled = false;

    int steeringMin = 1000;

    int steeringCenter = 1500;

    int steeringMax = 2000;

    uint8_t steeringCalibrationMask = 0;

    int steeringCapturedPulses[3] = {1000, 1500, 2000};

    int steeringCapturedInputPulses[3] = {1000, 1500, 2000};

    int radioSteeringTravel = OpenDriftParameters::Defaults::STEERING_TRAVEL;

    int gainMin = 1000;

    int gainMax = 2000;

    float channel3GainMin = OpenDriftParameters::Defaults::CHANNEL_3_GAIN_MIN;

    float channel3GainMax = OpenDriftParameters::Defaults::CHANNEL_3_GAIN_MAX;

    bool throttleOutputEnabled = false;

    uint8_t auxChannels[8] = {0};

    DrivingProfile profiles[MAX_PROFILES];

    uint8_t profileCount = 0;

    int8_t activeProfileIndex = -1;

    void save();

    void persistSteeringCalibration();

    void loadProfiles();
    void captureProfile(DrivingProfile& profile);
    void clampProfile(DrivingProfile& profile);
    void applyProfile(const DrivingProfile& profile);
    bool persistProfile(uint8_t index);
    String sanitizeProfileName(const String& name);
};
