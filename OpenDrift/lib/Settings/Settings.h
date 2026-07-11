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

    // Radio
    int getSteeringMin();
    void setSteeringMin(int value);

    int getSteeringCenter();
    void setSteeringCenter(int value);

    int getSteeringMax();
    void setSteeringMax(int value);

    int getGainMin();
    void setGainMin(int value);

    int getGainMax();
    void setGainMax(int value);

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

    int servoCenter = 1500;

    bool servoReverse = false;

    int servoTravel = 100;

    bool wifiEnabled = true;

    uint32_t wifiTimeout = 40000;

    int steeringMin = 1000;

    int steeringCenter = 1500;

    int steeringMax = 2000;

    int gainMin = 1000;

    int gainMax = 2000;

    void save();
};
