#pragma once

#include <Arduino.h>
#include <Preferences.h>

class Settings
{
public:

    bool begin();

    void update();

    // Gyro
    float getGain();
    void setGain(float value);

    float getDeadband();
    void setDeadband(float value);

    // Servo
    int getServoCenter();
    void setServoCenter(int value);

    bool getServoReverse();
    void setServoReverse(bool value);

    int getServoTravel();
    void setServoTravel(int value);

    // WiFi
    bool getWifiEnabled();
    void setWifiEnabled(bool value);

    uint32_t getWifiTimeout();
    void setWifiTimeout(uint32_t value);

private:

    Preferences prefs;

    bool dirty = false;

    unsigned long lastSave = 0;

    // Stored values

    float gain = 1.5f;

    float deadband = 2.0f;

    int servoCenter = 1500;

    bool servoReverse = false;

    int servoTravel = 100;

    bool wifiEnabled = true;

    uint32_t wifiTimeout = 40000;

    void save();
};