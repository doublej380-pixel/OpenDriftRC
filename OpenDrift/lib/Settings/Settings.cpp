#include "Settings.h"

namespace
{
    int legacyMaxCorrectionToPercent(int value)
    {
        // Legacy values used center-to-endpoint microseconds. The current
        // percentage covers the complete endpoint-to-endpoint correction.
        return constrain((value + 5) / 10, 0, 100);
    }

    int centerSpanPercentToFullSpanPercent(int value)
    {
        return constrain((value + 1) / 2, 0, 100);
    }

    struct DrivingProfileV10
    {
        uint32_t version;
        char name[Settings::PROFILE_NAME_LENGTH];
        float gain;
        float deadband;
        float gyroSmoothing;
        float gyroIntegralGain;
        int32_t gyroMaxCorrection;
        int32_t gyroIntegralLimit;
        int32_t gyroHoldBoost;
        int32_t predictionStrength;
        int32_t radioSteeringTravel;
        int32_t gyroCounterSteerAssist;
        int32_t gyroTransitionSpeed;
        int32_t gyroHuntStrength;
    };

    struct DrivingProfileV8
    {
        uint32_t version;
        char name[Settings::PROFILE_NAME_LENGTH];
        float gain;
        float deadband;
        float gyroSmoothing;
        float gyroIntegralGain;
        int32_t gyroMaxCorrection;
        int32_t gyroIntegralLimit;
        int32_t gyroHoldBoost;
        int32_t predictionStrength;
        int32_t radioSteeringTravel;
        int32_t gyroCounterSteerAssist;
        int32_t gyroTransitionSpeed;
        int32_t gyroHuntStrength;
    };

    struct DrivingProfileV1
    {
        uint32_t version;
        char name[Settings::PROFILE_NAME_LENGTH];
        float gain;
        float deadband;
        float gyroSmoothing;
        float gyroIntegralGain;
        int32_t gyroMaxCorrection;
        int32_t gyroAttackSpeed;
        int32_t gyroReturnSpeed;
        int32_t gyroIntegralLimit;
        int32_t gyroHoldBoost;
        int32_t gyroAntiWobble;
        int32_t gyroHuntDamping;
        int32_t steeringDamper;
        int32_t radioSteeringTravel;
    };

    struct DrivingProfileV2
    {
        uint32_t version;
        char name[Settings::PROFILE_NAME_LENGTH];
        float gain;
        float deadband;
        float gyroSmoothing;
        float gyroIntegralGain;
        int32_t gyroMaxCorrection;
        int32_t gyroAttackSpeed;
        int32_t gyroReturnSpeed;
        int32_t gyroIntegralLimit;
        int32_t gyroHoldBoost;
        int32_t gyroAntiWobble;
        int32_t gyroHuntDamping;
        int32_t steeringDamper;
        int32_t radioSteeringTravel;
        int32_t gyroCounterSteerAssist;
    };

    struct DrivingProfileV3
    {
        uint32_t version;
        char name[Settings::PROFILE_NAME_LENGTH];
        float gain;
        float deadband;
        float gyroSmoothing;
        float gyroIntegralGain;
        int32_t gyroMaxCorrection;
        int32_t gyroAttackSpeed;
        int32_t gyroReturnSpeed;
        int32_t gyroIntegralLimit;
        int32_t gyroHoldBoost;
        int32_t gyroAntiWobble;
        int32_t gyroHuntDamping;
        int32_t steeringDamper;
        int32_t radioSteeringTravel;
        int32_t gyroCounterSteerAssist;
        int32_t gyroTailSlideSpeed;
    };

    struct DrivingProfileV4
    {
        uint32_t version;
        char name[Settings::PROFILE_NAME_LENGTH];
        float gain;
        float deadband;
        float gyroSmoothing;
        float gyroIntegralGain;
        int32_t gyroMaxCorrection;
        int32_t gyroAttackSpeed;
        int32_t gyroReturnSpeed;
        int32_t gyroIntegralLimit;
        int32_t gyroHoldBoost;
        int32_t gyroAntiWobble;
        int32_t gyroHuntDamping;
        int32_t steeringDamper;
        int32_t radioSteeringTravel;
        int32_t gyroCounterSteerAssist;
        int32_t gyroTailSlideSpeed;
    };

    struct DrivingProfileV5
    {
        uint32_t version;
        char name[Settings::PROFILE_NAME_LENGTH];
        float gain;
        float deadband;
        float gyroSmoothing;
        float gyroIntegralGain;
        int32_t gyroMaxCorrection;
        int32_t gyroIntegralLimit;
        int32_t gyroHoldBoost;
        int32_t predictionStrength;
        int32_t radioSteeringTravel;
        int32_t gyroCounterSteerAssist;
        int32_t gyroTransitionSpeed;
    };

    struct DrivingProfileV6
    {
        uint32_t version;
        char name[Settings::PROFILE_NAME_LENGTH];
        float gain;
        float deadband;
        float gyroSmoothing;
        float gyroIntegralGain;
        int32_t gyroMaxCorrection;
        int32_t gyroIntegralLimit;
        int32_t gyroHoldBoost;
        int32_t predictionStrength;
        int32_t radioSteeringTravel;
        int32_t gyroCounterSteerAssist;
        int32_t gyroTransitionSpeed;
        int32_t gyroHuntSensitivity;
    };

    struct DrivingProfileV7
    {
        uint32_t version;
        char name[Settings::PROFILE_NAME_LENGTH];
        float gain;
        float deadband;
        float gyroSmoothing;
        float gyroIntegralGain;
        int32_t gyroMaxCorrection;
        int32_t gyroIntegralLimit;
        int32_t gyroHoldBoost;
        int32_t predictionStrength;
        int32_t radioSteeringTravel;
        int32_t gyroCounterSteerAssist;
        int32_t gyroTransitionSpeed;
        int32_t gyroHuntSensitivity;
        int32_t gyroHuntStrength;
    };
}

bool Settings::begin()
{
    #if defined(OPENDRIFT_ROUND_LOG51_TUNE)
    // Private round-display recovery build. Keep its calibration and tuning
    // isolated from both the normal CRSF and PWM firmware namespaces.
    prefs.begin("OpenDriftR51", false);
    #elif defined(OPENDRIFT_INPUT_CRSF)
    // Keep experimental CRSF tuning completely separate from the RC1 PWM
    // build, even when both firmwares are flashed onto the same board.
    prefs.begin("OpenDriftCRSF", false);
    #else
    prefs.begin("OpenDrift", false);
    #endif

    gain = constrain(
        prefs.getFloat(
            "gain",
            1.5f
        ),
        0.0f,
        6.0f
    );

    deadband = prefs.getFloat(
        "deadband",
        2.0f
    );

    gyroReverse = prefs.getBool(
        "gyroRev",
        false
    );

    bool maxCorrectionUsesFullSpan =
        prefs.getBool("maxSpanV1", false);

    if(prefs.isKey("gyroMaxPct"))
    {
        gyroMaxCorrection = constrain(
            prefs.getInt("gyroMaxPct", 50),
            0,
            100
        );

        if(!maxCorrectionUsesFullSpan)
        {
            gyroMaxCorrection =
                centerSpanPercentToFullSpanPercent(
                    gyroMaxCorrection
                );
        }
    }
    else if(prefs.isKey("gyroMax"))
    {
        gyroMaxCorrection = legacyMaxCorrectionToPercent(
            prefs.getInt("gyroMax", 250)
        );
        prefs.putInt("gyroMaxPct", gyroMaxCorrection);
    }
    else
    {
        gyroMaxCorrection = 25;
    }

    if(!maxCorrectionUsesFullSpan)
    {
        prefs.putInt("gyroMaxPct", gyroMaxCorrection);
        prefs.putBool("maxSpanV1", true);
    }

    gyroSmoothing = constrain(
        prefs.getFloat(
            "gyroSmooth",
            0.10f
        ),
        0.0f,
        1.0f
    );

    gyroLpfMode = constrain(
        prefs.getUChar("gyroLpf", 0),
        0,
        2
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

    gyroCounterSteerAssist = prefs.getInt(
        "counterAssist",
        0
    );

    if(prefs.isKey("tailSpeedC"))
    {
        gyroTransitionSpeed = constrain(
            prefs.getInt("tailSpeedC", 50),
            0,
            100
        );
    }
    else
    {
        int legacyTailSlideSpeed = prefs.getInt("tailSpeed", 0);

        // Experimental v3 used 0 as the proven response and 100 as the
        // maximum release. Preserve that exact behavior in the centered
        // scale, where old 0 -> new 50 and old 100 -> new 100.
        gyroTransitionSpeed = constrain(
            50 + legacyTailSlideSpeed / 2,
            50,
            100
        );

        prefs.putInt("tailSpeedC", gyroTransitionSpeed);
    }

    if(prefs.isKey("prediction"))
    {
        predictionStrength = prefs.getInt("prediction", 0);
    }
    else
    {
        predictionStrength = prefs.getInt("gyroHunt", 0);
        prefs.putInt("prediction", predictionStrength);
    }

    gyroHuntStrength = constrain(
        prefs.getInt("huntStrength", 50),
        0,
        100
    );

    curvePower = constrain(
        prefs.getFloat("curvePower", 1.0f),
        1.0f,
        5.0f
    );

    damperPower = constrain(
        prefs.getFloat("damperPower", 0.0f),
        0.0f,
        10.0f
    );

    damperPoint = constrain(
        prefs.getFloat("damperPoint", 0.5f),
        0.0f,
        1.0f
    );

    const char* retiredKeys[] = {
        "gyroAttack", "gyroReturn", "gyroWob", "gyroHunt",
        "strDamp", "huntSense", "terrainAssist"
    };

    for(const char* key : retiredKeys)
    {
        if(prefs.isKey(key)) prefs.remove(key);
    }

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

    servoQuiet = prefs.getInt(
        "quiet",
        0
    );

    controlLoopHz =
        prefs.getUShort("loopHz", 250) == 333
        ? 333
        : 250;

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

    // v1.0.7b stored receiver input endpoints under the steering keys. They
    // cannot safely be reused as physical servo stops, so only the new servo
    // endpoint schema is accepted as calibrated.
    steeringCenter = prefs.getInt("servoCalC", servoCenter);
    int fallbackOffset = (500 * constrain(servoTravel, 1, 100)) / 100;
    steeringMin = prefs.getInt(
        "servoCalL",
        servoCenter + (servoReverse ? fallbackOffset : -fallbackOffset)
    );
    steeringMax = prefs.getInt(
        "servoCalR",
        servoCenter + (servoReverse ? -fallbackOffset : fallbackOffset)
    );
    steeringCapturedPulses[0] = steeringMin;
    steeringCapturedPulses[1] = steeringCenter;
    steeringCapturedPulses[2] = steeringMax;
    steeringCapturedInputPulses[0] = prefs.getInt("servoInL", 1000);
    steeringCapturedInputPulses[1] = prefs.getInt("servoInC", 1500);
    steeringCapturedInputPulses[2] = prefs.getInt("servoInR", 2000);
    steeringCalibrationMask = prefs.getBool("servoEndV1", false)
        ? (prefs.getUChar("servoCalM", 0) & 0x07)
        : 0;

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

    channel3GainMin = constrain(
        prefs.getFloat("ch3GainLo", 0.5f),
        0.0f,
        6.0f
    );

    channel3GainMax = constrain(
        prefs.getFloat("ch3GainHi", 3.0f),
        0.0f,
        6.0f
    );

    if(channel3GainMax < channel3GainMin)
    {
        channel3GainMax = channel3GainMin;
    }

    throttleOutputEnabled = prefs.getBool(
        "thrOut",
        false
    );

    for(uint8_t index = 0; index < 8; index++)
    {
        char key[10];

        snprintf(
            key,
            sizeof(key),
            "auxCh%u",
            index + 1
        );

        auxChannels[index] = constrain(
            prefs.getUChar(key, 0),
            0,
            16
        );
    }

    #if defined(OPENDRIFT_ROUND_LOG51_TUNE)
    // Seed the proven blackbox-51 tune once. Subsequent UI, web, or EdgeTX
    // adjustments persist normally and are not overwritten on each boot.
    // Steering and servo calibration values are deliberately left untouched.
    if(!prefs.getBool("log51Preset", false))
    {
        gain = 1.85f;
        deadband = 2.0f;
        // 37% on the full-span scale preserves the old 74% authority.
        gyroMaxCorrection = 37;
        gyroSmoothing = 0.01f;
        gyroIntegralGain = 0.0f;
        gyroIntegralLimit = 120;
        gyroHoldBoost = 0;
        gyroCounterSteerAssist = 95;
        predictionStrength = 30;
        servoQuiet = 4;
        gyroTransitionSpeed = 25;
        gyroHuntStrength = 75;
        controlLoopHz = 333;
        curvePower = 1.0f;
        damperPower = 0.0f;
        damperPoint = 0.5f;

        prefs.putFloat("gain", gain);
        prefs.putFloat("deadband", deadband);
        prefs.putInt("gyroMaxPct", gyroMaxCorrection);
        prefs.putFloat("gyroSmooth", gyroSmoothing);
        prefs.putFloat("gyroIGain", gyroIntegralGain);
        prefs.putInt("gyroILim", gyroIntegralLimit);
        prefs.putInt("gyroHold", gyroHoldBoost);
        prefs.putInt("counterAssist", gyroCounterSteerAssist);
        prefs.putInt("prediction", predictionStrength);
        prefs.putInt("quiet", servoQuiet);
        prefs.putInt("tailSpeedC", gyroTransitionSpeed);
        prefs.putInt("huntStrength", gyroHuntStrength);
        prefs.putUShort("loopHz", controlLoopHz);
        prefs.putFloat("curvePower", curvePower);
        prefs.putFloat("damperPower", damperPower);
        prefs.putFloat("damperPoint", damperPoint);
        prefs.putBool("log51Preset", true);
    }
    #endif

    loadProfiles();

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
        "gyroMaxPct",
        gyroMaxCorrection
    );

    prefs.putFloat(
        "gyroSmooth",
        gyroSmoothing
    );

    prefs.putUChar(
        "gyroLpf",
        gyroLpfMode
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
        "counterAssist",
        gyroCounterSteerAssist
    );

    prefs.putInt(
        "tailSpeedC",
        gyroTransitionSpeed
    );

    prefs.putInt(
        "prediction",
        predictionStrength
    );

    prefs.putInt(
        "huntStrength",
        gyroHuntStrength
    );

    prefs.putFloat(
        "curvePower",
        curvePower
    );

    prefs.putFloat(
        "damperPower",
        damperPower
    );

    prefs.putFloat(
        "damperPoint",
        damperPoint
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

    prefs.putInt(
        "quiet",
        servoQuiet
    );

    prefs.putUShort(
        "loopHz",
        controlLoopHz
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

    prefs.putBool("servoEndV1", true);
    prefs.putInt("servoCalL", steeringCapturedPulses[0]);
    prefs.putInt("servoCalC", steeringCapturedPulses[1]);
    prefs.putInt("servoCalR", steeringCapturedPulses[2]);
    prefs.putUChar("servoCalM", steeringCalibrationMask & 0x07);
    prefs.putInt("servoInL", steeringCapturedInputPulses[0]);
    prefs.putInt("servoInC", steeringCapturedInputPulses[1]);
    prefs.putInt("servoInR", steeringCapturedInputPulses[2]);

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

    prefs.putFloat(
        "ch3GainLo",
        channel3GainMin
    );

    prefs.putFloat(
        "ch3GainHi",
        channel3GainMax
    );

    prefs.putBool(
        "thrOut",
        throttleOutputEnabled
    );

    for(uint8_t index = 0; index < 8; index++)
    {
        char key[10];

        snprintf(
            key,
            sizeof(key),
            "auxCh%u",
            index + 1
        );

        prefs.putUChar(
            key,
            auxChannels[index]
        );
    }

    if(
        activeProfileIndex >= 0 &&
        activeProfileIndex < profileCount
    )
    {
        captureProfile(
            profiles[activeProfileIndex]
        );

        persistProfile(
            activeProfileIndex
        );
    }

    prefs.putUChar(
        "profCnt",
        profileCount
    );

    prefs.putChar(
        "profAct",
        activeProfileIndex
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
    gain = constrain(value, 0.0f, 6.0f);
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
            100
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
            0.0f,
            1.0f
        );

    dirty = true;
}

uint8_t Settings::getGyroLpfMode()
{
    return gyroLpfMode;
}

void Settings::setGyroLpfMode(uint8_t value)
{
    gyroLpfMode = constrain(value, 0, 2);
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

int Settings::getGyroCounterSteerAssist()
{
    return gyroCounterSteerAssist;
}

void Settings::setGyroCounterSteerAssist(int value)
{
    gyroCounterSteerAssist = constrain(value, 0, 100);
    dirty = true;
}

int Settings::getGyroTransitionSpeed()
{
    return gyroTransitionSpeed;
}

void Settings::setGyroTransitionSpeed(int value)
{
    gyroTransitionSpeed = constrain(value, 0, 100);
    dirty = true;
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

int Settings::getPredictionStrength()
{
    return predictionStrength;
}

void Settings::setPredictionStrength(int value)
{
    predictionStrength =
        constrain(
            value,
            0,
            100
        );

    dirty = true;
}

int Settings::getGyroHuntStrength()
{
    return gyroHuntStrength;
}

void Settings::setGyroHuntStrength(int value)
{
    gyroHuntStrength = constrain(value, 0, 100);
    dirty = true;
}

float Settings::getCurvePower()
{
    return curvePower;
}

void Settings::setCurvePower(float value)
{
    curvePower = constrain(value, 1.0f, 5.0f);
    dirty = true;
}

float Settings::getDamperPower()
{
    return damperPower;
}

void Settings::setDamperPower(float value)
{
    damperPower = constrain(value, 0.0f, 10.0f);
    dirty = true;
}

float Settings::getDamperPoint()
{
    return damperPoint;
}

void Settings::setDamperPoint(float value)
{
    damperPoint = constrain(value, 0.0f, 1.0f);
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
    if(servoCenter == value)
    {
        return;
    }

    servoCenter = value;
    clearSteeringCalibration();
    dirty = true;
}

bool Settings::getServoReverse()
{
    return servoReverse;
}

void Settings::setServoReverse(bool value)
{
    if(servoReverse == value)
    {
        return;
    }

    servoReverse = value;
    clearSteeringCalibration();
    dirty = true;
}

int Settings::getServoTravel()
{
    return servoTravel;
}

void Settings::setServoTravel(int value)
{
    if(servoTravel == value)
    {
        return;
    }

    servoTravel = value;
    clearSteeringCalibration();
    dirty = true;
}

int Settings::getServoQuiet()
{
    return servoQuiet;
}

void Settings::setServoQuiet(int value)
{
    servoQuiet =
        constrain(
            value,
            0,
            50
        );

    dirty = true;
}

uint16_t Settings::getControlLoopHz()
{
    return controlLoopHz;
}

void Settings::setControlLoopHz(uint16_t value)
{
    controlLoopHz = value == 333 ? 333 : 250;
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
    if(steeringMin == value)
    {
        return;
    }

    steeringMin = value;
    steeringCapturedPulses[0] = value;
    steeringCalibrationMask = 0;
    dirty = true;
}

int Settings::getSteeringCenter()
{
    return steeringCenter;
}

void Settings::setSteeringCenter(int value)
{
    if(steeringCenter == value)
    {
        return;
    }

    steeringCenter = value;
    steeringCapturedPulses[1] = value;
    steeringCalibrationMask = 0;
    dirty = true;
}

int Settings::getSteeringMax()
{
    return steeringMax;
}

void Settings::setSteeringMax(int value)
{
    if(steeringMax == value)
    {
        return;
    }

    steeringMax = value;
    steeringCapturedPulses[2] = value;
    steeringCalibrationMask = 0;
    dirty = true;
}

uint8_t Settings::getSteeringCalibrationMask()
{
    return steeringCalibrationMask & 0x07;
}

bool Settings::isSteeringCalibrated()
{
    int leftDelta = steeringMin - steeringCenter;
    int rightDelta = steeringMax - steeringCenter;

    return
        getSteeringCalibrationMask() == 0x07 &&
        abs(leftDelta) >= 10 &&
        abs(rightDelta) >= 10 &&
        leftDelta * rightDelta < 0;
}

int Settings::getSteeringCapturedPulse(
    uint8_t point
)
{
    if(point >= 3)
    {
        return 1500;
    }

    return steeringCapturedPulses[point];
}

int Settings::getSteeringCapturedInputPulse(
    uint8_t point
)
{
    if(point >= 3)
    {
        return 1500;
    }

    return steeringCapturedInputPulses[point];
}

bool Settings::captureSteeringCalibrationPoint(
    uint8_t point,
    int physicalPulse,
    int inputPulse
)
{
    if(
        point >= 3 ||
        physicalPulse < 900 ||
        physicalPulse > 2100
    )
    {
        return false;
    }

    steeringCapturedPulses[point] = physicalPulse;

    if(inputPulse >= 800 && inputPulse <= 2200)
    {
        steeringCapturedInputPulses[point] = inputPulse;
    }
    steeringCalibrationMask |= (1U << point);
    dirty = true;

    if(getSteeringCalibrationMask() != 0x07)
    {
        return true;
    }

    int leftDelta =
        steeringCapturedPulses[0] - steeringCapturedPulses[1];

    int rightDelta =
        steeringCapturedPulses[2] - steeringCapturedPulses[1];

    bool validCalibration =
        abs(leftDelta) >= 10 &&
        abs(rightDelta) >= 10 &&
        leftDelta * rightDelta < 0;

    if(!validCalibration)
    {
        // Keep the two known-good captures and make the rejected position
        // visibly incomplete on both the display and radio tool.
        steeringCalibrationMask &= ~(1U << point);
        return false;
    }

    steeringMin = steeringCapturedPulses[0];
    steeringCenter = steeringCapturedPulses[1];
    steeringMax = steeringCapturedPulses[2];

    return true;
}

bool Settings::confirmStoredSteeringCalibration()
{
    int leftDelta = steeringMin - steeringCenter;
    int rightDelta = steeringMax - steeringCenter;

    bool validCalibration =
        abs(leftDelta) >= 10 &&
        abs(rightDelta) >= 10 &&
        leftDelta * rightDelta < 0;

    if(!validCalibration)
    {
        steeringCalibrationMask = 0;
        dirty = true;
        return false;
    }

    steeringCapturedPulses[0] = steeringMin;
    steeringCapturedPulses[1] = steeringCenter;
    steeringCapturedPulses[2] = steeringMax;
    steeringCalibrationMask = 0x07;
    dirty = true;

    return true;
}

void Settings::clearSteeringCalibration()
{
    steeringCalibrationMask = 0;
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

float Settings::getChannel3GainMin()
{
    return channel3GainMin;
}

void Settings::setChannel3GainMin(float value)
{
    channel3GainMin = constrain(value, 0.0f, 6.0f);

    if(channel3GainMax < channel3GainMin)
    {
        channel3GainMax = channel3GainMin;
    }

    dirty = true;
}

float Settings::getChannel3GainMax()
{
    return channel3GainMax;
}

void Settings::setChannel3GainMax(float value)
{
    channel3GainMax = constrain(value, 0.0f, 6.0f);

    if(channel3GainMin > channel3GainMax)
    {
        channel3GainMin = channel3GainMax;
    }

    dirty = true;
}

bool Settings::getThrottleOutputEnabled()
{
    return throttleOutputEnabled;
}

void Settings::setThrottleOutputEnabled(bool value)
{
    throttleOutputEnabled = value;
    dirty = true;
}

uint8_t Settings::getAuxChannelForGpio(uint8_t gpio)
{
    if(gpio < 1 || gpio > 8)
    {
        return 0;
    }

    return auxChannels[gpio - 1];
}

void Settings::setAuxChannelForGpio(
    uint8_t gpio,
    uint8_t channel
)
{
    if(gpio < 1 || gpio > 8)
    {
        return;
    }

    auxChannels[gpio - 1] = constrain(
        channel,
        (uint8_t)0,
        (uint8_t)16
    );

    dirty = true;
}

// --------------------
// Driving profiles
// --------------------

uint8_t Settings::getProfileCount()
{
    return profileCount;
}

int8_t Settings::getActiveProfileIndex()
{
    return activeProfileIndex;
}

const char* Settings::getActiveProfileName()
{
    if(
        activeProfileIndex < 0 ||
        activeProfileIndex >= profileCount
    )
    {
        return "Current Tune";
    }

    return profiles[activeProfileIndex].name;
}

const Settings::DrivingProfile* Settings::getProfile(
    uint8_t index
)
{
    if(index >= profileCount)
    {
        return nullptr;
    }

    return &profiles[index];
}

int8_t Settings::createProfile(
    const String& requestedName
)
{
    if(profileCount >= MAX_PROFILES)
    {
        return -1;
    }

    String name =
        sanitizeProfileName(requestedName);

    if(name.length() == 0)
    {
        return -1;
    }

    for(uint8_t i = 0; i < profileCount; i++)
    {
        if(name.equalsIgnoreCase(profiles[i].name))
        {
            return -1;
        }
    }

    if(dirty)
    {
        save();
    }

    DrivingProfile& profile =
        profiles[profileCount];

    profile = DrivingProfile();

    name.toCharArray(
        profile.name,
        PROFILE_NAME_LENGTH
    );

    captureProfile(profile);

    uint8_t newIndex = profileCount;

    profileCount++;
    activeProfileIndex = newIndex;

    persistProfile(newIndex);

    prefs.putUChar(
        "profCnt",
        profileCount
    );

    prefs.putChar(
        "profAct",
        activeProfileIndex
    );

    return activeProfileIndex;
}

bool Settings::activateProfile(
    uint8_t index
)
{
    if(index >= profileCount)
    {
        return false;
    }

    if(dirty)
    {
        save();
    }

    activeProfileIndex = index;

    applyProfile(
        profiles[index]
    );

    dirty = true;
    save();

    return true;
}

bool Settings::deleteProfile(
    uint8_t index
)
{
    if(index >= profileCount)
    {
        return false;
    }

    if(dirty)
    {
        save();
    }

    bool deletedActive =
        activeProfileIndex == index;

    for(uint8_t i = index; i + 1 < profileCount; i++)
    {
        profiles[i] = profiles[i + 1];
    }

    uint8_t previousLast =
        profileCount - 1;

    profiles[previousLast] = DrivingProfile();
    profileCount--;

    if(deletedActive)
    {
        activeProfileIndex = -1;
    }
    else if(activeProfileIndex > index)
    {
        activeProfileIndex--;
    }

    for(uint8_t i = 0; i < profileCount; i++)
    {
        persistProfile(i);
    }

    char key[12];

    snprintf(
        key,
        sizeof(key),
        "prof%u",
        previousLast
    );

    prefs.remove(key);

    prefs.putUChar(
        "profCnt",
        profileCount
    );

    prefs.putChar(
        "profAct",
        activeProfileIndex
    );

    return true;
}

void Settings::loadProfiles()
{
    profileCount = constrain(
        (int)prefs.getUChar("profCnt", 0),
        0,
        (int)MAX_PROFILES
    );

    uint8_t loadedCount = 0;

    for(uint8_t i = 0; i < profileCount; i++)
    {
        char key[12];

        snprintf(
            key,
            sizeof(key),
            "prof%u",
            i
        );

        size_t storedSize = prefs.getBytesLength(key);

        if(storedSize == sizeof(DrivingProfile))
        {
            DrivingProfile stored = {};

            if(
                prefs.getBytes(key, &stored, sizeof(stored)) == sizeof(stored) &&
                stored.name[0] != '\0' &&
                stored.version == 11
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                profile = stored;
                profile.version = 11;
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                loadedCount++;
            }
        }
        else if(storedSize == sizeof(DrivingProfileV10))
        {
            DrivingProfileV10 legacy = {};

            if(
                prefs.getBytes(key, &legacy, sizeof(legacy)) == sizeof(legacy) &&
                (legacy.version == 8 || legacy.version == 9 || legacy.version == 10) &&
                legacy.name[0] != '\0'
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                profile = DrivingProfile();
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                profile.gain = legacy.gain;
                profile.deadband = legacy.deadband;
                profile.gyroSmoothing = legacy.gyroSmoothing;
                profile.gyroIntegralGain = legacy.gyroIntegralGain;
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.predictionStrength;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTransitionSpeed = legacy.gyroTransitionSpeed;
                profile.gyroHuntStrength = legacy.gyroHuntStrength;
                profile.curvePower = 1.0f;
                profile.damperPower = 0.0f;
                profile.damperPoint = 0.5f;

                if(legacy.version == 9)
                {
                    profile.gyroMaxCorrection =
                        centerSpanPercentToFullSpanPercent(
                            legacy.gyroMaxCorrection
                        );
                }
                else if(legacy.version == 8)
                {
                    profile.gyroMaxCorrection =
                        legacyMaxCorrectionToPercent(
                            legacy.gyroMaxCorrection
                        );
                }
                else
                {
                    profile.gyroMaxCorrection = legacy.gyroMaxCorrection;
                }

                loadedCount++;
            }
        }
        else if(storedSize == sizeof(DrivingProfileV7))
        {
            DrivingProfileV7 legacy = {};

            if(
                prefs.getBytes(key, &legacy, sizeof(legacy)) == sizeof(legacy) &&
                legacy.version == 7 &&
                legacy.name[0] != '\0'
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                profile = DrivingProfile();
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                profile.gain = legacy.gain;
                profile.deadband = legacy.deadband;
                profile.gyroSmoothing = legacy.gyroSmoothing;
                profile.gyroIntegralGain = legacy.gyroIntegralGain;
                profile.gyroMaxCorrection = legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection);
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.predictionStrength;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTransitionSpeed = legacy.gyroTransitionSpeed;
                profile.gyroHuntStrength = legacy.gyroHuntStrength;
                profile.curvePower = 1.0f;
                profile.damperPower = 0.0f;
                profile.damperPoint = 0.5f;
                loadedCount++;
            }
        }
        else if(storedSize == sizeof(DrivingProfileV6))
        {
            DrivingProfileV6 legacy = {};

            if(
                prefs.getBytes(key, &legacy, sizeof(legacy)) == sizeof(legacy) &&
                legacy.version == 6 &&
                legacy.name[0] != '\0'
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                profile = DrivingProfile();
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                profile.gain = legacy.gain;
                profile.deadband = legacy.deadband;
                profile.gyroSmoothing = legacy.gyroSmoothing;
                profile.gyroIntegralGain = legacy.gyroIntegralGain;
                profile.gyroMaxCorrection = legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection);
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.predictionStrength;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTransitionSpeed = legacy.gyroTransitionSpeed;
                profile.gyroHuntStrength = 50;
                profile.curvePower = 1.0f;
                profile.damperPower = 0.0f;
                profile.damperPoint = 0.5f;
                loadedCount++;
            }
        }
        else if(storedSize == sizeof(DrivingProfileV5))
        {
            DrivingProfileV5 legacy = {};

            if(
                prefs.getBytes(key, &legacy, sizeof(legacy)) == sizeof(legacy) &&
                legacy.version == 5 &&
                legacy.name[0] != '\0'
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                profile = DrivingProfile();
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                profile.gain = legacy.gain;
                profile.deadband = legacy.deadband;
                profile.gyroSmoothing = legacy.gyroSmoothing;
                profile.gyroIntegralGain = legacy.gyroIntegralGain;
                profile.gyroMaxCorrection = legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection);
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.predictionStrength;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTransitionSpeed = legacy.gyroTransitionSpeed;
                profile.curvePower = 1.0f;
                profile.damperPower = 0.0f;
                profile.damperPoint = 0.5f;
                loadedCount++;
            }
        }
        else if(storedSize == sizeof(DrivingProfileV4))
        {
            DrivingProfileV4 legacy = {};

            if(
                prefs.getBytes(key, &legacy, sizeof(legacy)) == sizeof(legacy) &&
                legacy.version == 4 &&
                legacy.name[0] != '\0'
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                profile = DrivingProfile();
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                profile.gain = legacy.gain;
                profile.deadband = legacy.deadband;
                profile.gyroSmoothing = legacy.gyroSmoothing;
                profile.gyroIntegralGain = legacy.gyroIntegralGain;
                profile.gyroMaxCorrection = legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection);
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.gyroHuntDamping;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTransitionSpeed = legacy.gyroTailSlideSpeed;
                profile.curvePower = 1.0f;
                profile.damperPower = 0.0f;
                profile.damperPoint = 0.5f;
                loadedCount++;
            }
        }
        else if(storedSize == sizeof(DrivingProfileV3))
        {
            DrivingProfileV3 legacy = {};

            if(
                prefs.getBytes(key, &legacy, sizeof(legacy)) == sizeof(legacy) &&
                legacy.version == 3 &&
                legacy.name[0] != '\0'
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                profile = DrivingProfile();
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                profile.gain = legacy.gain;
                profile.deadband = legacy.deadband;
                profile.gyroSmoothing = legacy.gyroSmoothing;
                profile.gyroIntegralGain = legacy.gyroIntegralGain;
                profile.gyroMaxCorrection = legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection);
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.gyroHuntDamping;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTransitionSpeed = constrain(
                    50 + legacy.gyroTailSlideSpeed / 2,
                    50,
                    100
                );
                profile.curvePower = 1.0f;
                profile.damperPower = 0.0f;
                profile.damperPoint = 0.5f;
                loadedCount++;
            }
        }
        else if(storedSize == sizeof(DrivingProfileV2))
        {
            DrivingProfileV2 legacy = {};

            if(
                prefs.getBytes(key, &legacy, sizeof(legacy)) == sizeof(legacy) &&
                legacy.version == 2 &&
                legacy.name[0] != '\0'
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                profile = DrivingProfile();
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                profile.gain = legacy.gain;
                profile.deadband = legacy.deadband;
                profile.gyroSmoothing = legacy.gyroSmoothing;
                profile.gyroIntegralGain = legacy.gyroIntegralGain;
                profile.gyroMaxCorrection = legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection);
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.gyroHuntDamping;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTransitionSpeed = 50;
                profile.curvePower = 1.0f;
                profile.damperPower = 0.0f;
                profile.damperPoint = 0.5f;
                loadedCount++;
            }
        }
        else if(storedSize == sizeof(DrivingProfileV1))
        {
            DrivingProfileV1 legacy = {};

            if(
                prefs.getBytes(key, &legacy, sizeof(legacy)) == sizeof(legacy) &&
                legacy.version == 1 &&
                legacy.name[0] != '\0'
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                profile = DrivingProfile();
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                profile.gain = legacy.gain;
                profile.deadband = legacy.deadband;
                profile.gyroSmoothing = legacy.gyroSmoothing;
                profile.gyroIntegralGain = legacy.gyroIntegralGain;
                profile.gyroMaxCorrection = legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection);
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.gyroHuntDamping;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = 0;
                profile.gyroTransitionSpeed = 50;
                profile.curvePower = 1.0f;
                profile.damperPower = 0.0f;
                profile.damperPoint = 0.5f;
                loadedCount++;
            }
        }
    }

    profileCount = loadedCount;

    for(uint8_t i = 0; i < profileCount; i++)
    {
        persistProfile(i);
    }

    int storedActive =
        prefs.getChar("profAct", -1);

    activeProfileIndex =
        storedActive >= 0 &&
        storedActive < profileCount
        ?
        storedActive
        :
        -1;
}

void Settings::captureProfile(
    DrivingProfile& profile
)
{
    profile.version = 11;
    profile.gain = gain;
    profile.deadband = deadband;
    profile.gyroSmoothing = gyroSmoothing;
    profile.gyroIntegralGain = gyroIntegralGain;
    profile.gyroMaxCorrection = gyroMaxCorrection;
    profile.gyroIntegralLimit = gyroIntegralLimit;
    profile.gyroHoldBoost = gyroHoldBoost;
    profile.predictionStrength = predictionStrength;
    profile.radioSteeringTravel = radioSteeringTravel;
    profile.gyroCounterSteerAssist = gyroCounterSteerAssist;
    profile.gyroTransitionSpeed = gyroTransitionSpeed;
    profile.gyroHuntStrength = gyroHuntStrength;
    profile.curvePower = curvePower;
    profile.damperPower = damperPower;
    profile.damperPoint = damperPoint;
}

void Settings::applyProfile(
    const DrivingProfile& profile
)
{
    gain = constrain(profile.gain, 0.0f, 6.0f);
    deadband = profile.deadband;
    gyroSmoothing = constrain(profile.gyroSmoothing, 0.0f, 1.0f);
    gyroIntegralGain = profile.gyroIntegralGain;
    gyroMaxCorrection = profile.gyroMaxCorrection;
    gyroIntegralLimit = profile.gyroIntegralLimit;
    gyroHoldBoost = profile.gyroHoldBoost;
    predictionStrength = profile.predictionStrength;
    radioSteeringTravel = profile.radioSteeringTravel;
    gyroCounterSteerAssist = profile.gyroCounterSteerAssist;
    gyroTransitionSpeed = profile.gyroTransitionSpeed;
    gyroHuntStrength = profile.gyroHuntStrength;
    curvePower = constrain(profile.curvePower, 1.0f, 5.0f);
    damperPower = constrain(profile.damperPower, 0.0f, 10.0f);
    damperPoint = constrain(profile.damperPoint, 0.0f, 1.0f);
}

bool Settings::persistProfile(
    uint8_t index
)
{
    if(index >= profileCount)
    {
        return false;
    }

    char key[12];

    snprintf(
        key,
        sizeof(key),
        "prof%u",
        index
    );

    return prefs.putBytes(
        key,
        &profiles[index],
        sizeof(DrivingProfile)
    ) == sizeof(DrivingProfile);
}

String Settings::sanitizeProfileName(
    const String& requestedName
)
{
    String name = requestedName;
    name.trim();

    String clean;
    clean.reserve(PROFILE_NAME_LENGTH - 1);

    for(
        size_t i = 0;
        i < name.length() &&
        clean.length() < PROFILE_NAME_LENGTH - 1;
        i++
    )
    {
        char value = name.charAt(i);

        if(
            isAlphaNumeric(value) ||
            value == ' ' ||
            value == '-' ||
            value == '_' ||
            value == '.'
        )
        {
            clean += value;
        }
    }

    clean.trim();

    return clean;
}