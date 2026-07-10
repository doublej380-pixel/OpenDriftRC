#include "Settings.h"

bool Settings::begin()
{
    prefs.begin("OpenDrift", false);

    gain = prefs.getFloat(
        "gain",
        1.5f
    );

    deadband = prefs.getFloat(
        "deadband",
        2.0f
    );

    gyroReverse = prefs.getBool(
        "gyroRev",
        false
    );

    gyroMaxCorrection = prefs.getInt(
        "gyroMax",
        250
    );

    gyroSmoothing = prefs.getFloat(
        "gyroSmooth",
        0.10f
    );

    gyroSteeringCut = prefs.getFloat(
        "gyroCut",
        0.50f
    );

    servoCenter = prefs.getInt(
        "center",
        1500
    );

    servoReverse = prefs.getBool(
        "reverse",
        false
    );

    servoTravel = prefs.getInt(
        "travel",
        100
    );

    wifiEnabled = prefs.getBool(
        "wifi",
        true
    );

    wifiTimeout = prefs.getULong(
        "timeout",
        40000
    );

    steeringMin = prefs.getInt(
        "strMin",
        1000
    );

    steeringCenter = prefs.getInt(
        "strCenter",
        1500
    );

    steeringMax = prefs.getInt(
        "strMax",
        2000
    );

    gainMin = prefs.getInt(
        "gainMin",
        1000
    );

    gainMax = prefs.getInt(
        "gainMax",
        2000
    );

    return true;
}

void Settings::update()
{
    if(
        dirty &&
        millis() - lastSave > 1000
    )
    {
        save();
    }
}

void Settings::save()
{
    prefs.putFloat(
        "gain",
        gain
    );

    prefs.putFloat(
        "deadband",
        deadband
    );

    prefs.putBool(
        "gyroRev",
        gyroReverse
    );

    prefs.putInt(
        "gyroMax",
        gyroMaxCorrection
    );

    prefs.putFloat(
        "gyroSmooth",
        gyroSmoothing
    );

    prefs.putFloat(
        "gyroCut",
        gyroSteeringCut
    );

    prefs.putInt(
        "center",
        servoCenter
    );

    prefs.putBool(
        "reverse",
        servoReverse
    );

    prefs.putInt(
        "travel",
        servoTravel
    );

    prefs.putBool(
        "wifi",
        wifiEnabled
    );

    prefs.putULong(
        "timeout",
        wifiTimeout
    );

    prefs.putInt(
        "strMin",
        steeringMin
    );

    prefs.putInt(
        "strCenter",
        steeringCenter
    );

    prefs.putInt(
        "strMax",
        steeringMax
    );

    prefs.putInt(
        "gainMin",
        gainMin
    );

    prefs.putInt(
        "gainMax",
        gainMax
    );

    dirty = false;

    lastSave = millis();
}

// --------------------
// Gyro
// --------------------

float Settings::getGain()
{
    return gain;
}

void Settings::setGain(float value)
{
    gain = value;
    dirty = true;
}

float Settings::getDeadband()
{
    return deadband;
}

void Settings::setDeadband(float value)
{
    deadband = value;
    dirty = true;
}

bool Settings::getGyroReverse()
{
    return gyroReverse;
}

void Settings::setGyroReverse(bool value)
{
    gyroReverse = value;
    dirty = true;
}

int Settings::getGyroMaxCorrection()
{
    return gyroMaxCorrection;
}

void Settings::setGyroMaxCorrection(int value)
{
    gyroMaxCorrection =
        constrain(
            value,
            0,
            500
        );

    dirty = true;
}

float Settings::getGyroSmoothing()
{
    return gyroSmoothing;
}

void Settings::setGyroSmoothing(float value)
{
    gyroSmoothing =
        constrain(
            value,
            0.01f,
            1.0f
        );

    dirty = true;
}

float Settings::getGyroSteeringCut()
{
    return gyroSteeringCut;
}

void Settings::setGyroSteeringCut(float value)
{
    gyroSteeringCut =
        constrain(
            value,
            0.0f,
            1.0f
        );

    dirty = true;
}

// --------------------
// Servo
// --------------------

int Settings::getServoCenter()
{
    return servoCenter;
}

void Settings::setServoCenter(int value)
{
    servoCenter = value;
    dirty = true;
}

bool Settings::getServoReverse()
{
    return servoReverse;
}

void Settings::setServoReverse(bool value)
{
    servoReverse = value;
    dirty = true;
}

int Settings::getServoTravel()
{
    return servoTravel;
}

void Settings::setServoTravel(int value)
{
    servoTravel = value;
    dirty = true;
}

// --------------------
// WiFi
// --------------------

bool Settings::getWifiEnabled()
{
    return wifiEnabled;
}

void Settings::setWifiEnabled(bool value)
{
    wifiEnabled = value;
    dirty = true;
}

uint32_t Settings::getWifiTimeout()
{
    return wifiTimeout;
}

void Settings::setWifiTimeout(uint32_t value)
{
    wifiTimeout = value;
    dirty = true;
}

// --------------------
// Radio
// --------------------

int Settings::getSteeringMin()
{
    return steeringMin;
}

void Settings::setSteeringMin(int value)
{
    steeringMin = value;
    dirty = true;
}

int Settings::getSteeringCenter()
{
    return steeringCenter;
}

void Settings::setSteeringCenter(int value)
{
    steeringCenter = value;
    dirty = true;
}

int Settings::getSteeringMax()
{
    return steeringMax;
}

void Settings::setSteeringMax(int value)
{
    steeringMax = value;
    dirty = true;
}

int Settings::getGainMin()
{
    return gainMin;
}

void Settings::setGainMin(int value)
{
    gainMin = value;
    dirty = true;
}

int Settings::getGainMax()
{
    return gainMax;
}

void Settings::setGainMax(int value)
{
    gainMax = value;
    dirty = true;
}
