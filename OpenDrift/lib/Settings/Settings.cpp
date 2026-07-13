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

    gyroAttackSpeed = prefs.getInt(
        "gyroAttack",
        80
    );

    gyroReturnSpeed = prefs.getInt(
        "gyroReturn",
        30
    );

    gyroIntegralGain = prefs.getFloat(
        "gyroIGain",
        0.0f
    );

    gyroIntegralLimit = prefs.getInt(
        "gyroILim",
        120
    );

    gyroHoldBoost = prefs.getInt(
        "gyroHold",
        0
    );

    gyroAntiWobble = prefs.getInt(
        "gyroWob",
        50
    );

    steeringDamper = prefs.getInt(
        "strDamp",
        0
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

    blackboxEnabled = prefs.getBool(
        "blackbox",
        false
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

    radioSteeringTravel = prefs.getInt(
        "strTravel",
        100
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

    prefs.putInt(
        "gyroAttack",
        gyroAttackSpeed
    );

    prefs.putInt(
        "gyroReturn",
        gyroReturnSpeed
    );

    prefs.putFloat(
        "gyroIGain",
        gyroIntegralGain
    );

    prefs.putInt(
        "gyroILim",
        gyroIntegralLimit
    );

    prefs.putInt(
        "gyroHold",
        gyroHoldBoost
    );

    prefs.putInt(
        "gyroWob",
        gyroAntiWobble
    );

    prefs.putInt(
        "strDamp",
        steeringDamper
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

    prefs.putBool(
        "blackbox",
        blackboxEnabled
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
        "strTravel",
        radioSteeringTravel
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
            1000
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

int Settings::getGyroAttackSpeed()
{
    return gyroAttackSpeed;
}

void Settings::setGyroAttackSpeed(int value)
{
    gyroAttackSpeed =
        constrain(
            value,
            1,
            500
        );

    dirty = true;
}

int Settings::getGyroReturnSpeed()
{
    return gyroReturnSpeed;
}

void Settings::setGyroReturnSpeed(int value)
{
    gyroReturnSpeed =
        constrain(
            value,
            1,
            500
        );

    dirty = true;
}

float Settings::getGyroIntegralGain()
{
    return gyroIntegralGain;
}

void Settings::setGyroIntegralGain(float value)
{
    gyroIntegralGain =
        constrain(
            value,
            0.0f,
            20.0f
        );

    dirty = true;
}

int Settings::getGyroIntegralLimit()
{
    return gyroIntegralLimit;
}

void Settings::setGyroIntegralLimit(int value)
{
    gyroIntegralLimit =
        constrain(
            value,
            0,
            500
        );

    dirty = true;
}

int Settings::getGyroHoldBoost()
{
    return gyroHoldBoost;
}

void Settings::setGyroHoldBoost(int value)
{
    gyroHoldBoost =
        constrain(
            value,
            0,
            100
        );

    dirty = true;
}

int Settings::getGyroAntiWobble()
{
    return gyroAntiWobble;
}

void Settings::setGyroAntiWobble(int value)
{
    gyroAntiWobble =
        constrain(
            value,
            0,
            100
        );

    dirty = true;
}

int Settings::getSteeringDamper()
{
    return steeringDamper;
}

void Settings::setSteeringDamper(int value)
{
    steeringDamper =
        constrain(
            value,
            0,
            1000
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
// Blackbox
// --------------------

bool Settings::getBlackboxEnabled()
{
    return blackboxEnabled;
}

void Settings::setBlackboxEnabled(bool value)
{
    blackboxEnabled = value;
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

int Settings::getRadioSteeringTravel()
{
    return radioSteeringTravel;
}

void Settings::setRadioSteeringTravel(int value)
{
    radioSteeringTravel =
        constrain(
            value,
            0,
            100
        );

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
