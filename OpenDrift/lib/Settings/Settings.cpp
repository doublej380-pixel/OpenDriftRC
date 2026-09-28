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
    #elif defined(OPENDRIFT_BOARD_MATRIX) && defined(OPENDRIFT_INPUT_CRSF)
    // The private Matrix experiments must not inherit actuator calibration or
    // tuning from any display-equipped OpenDrift build.
    prefs.begin("ODMatrixCRSF", false);
    #elif defined(OPENDRIFT_BOARD_MATRIX)
    prefs.begin("ODMatrixPWM", false);
    #elif defined(OPENDRIFT_INPUT_CRSF)
    // Keep experimental CRSF tuning completely separate from the RC1 PWM
    // build, even when both firmwares are flashed onto the same board.
    prefs.begin("OpenDriftCRSF", false);
    #else
    prefs.begin("OpenDrift", false);
    #endif

    displayBrightness = constrain(
        prefs.getUChar("dispBright", 100),
        10,
        100
    );

    displayDimTimeout = constrain(
        prefs.getUShort("dispDim", 0),
        0,
        600
    );

    themeText = constrain(
        prefs.getUChar("thmText", 0),
        0,
        1
    );

    themeAccent = constrain(
        prefs.getUChar("thmAccent", 0),
        0,
        THEME_ACCENT_COUNT - 1
    );

    {
        const String storedBackground =
            sanitizeBackgroundName(prefs.getString("bgName", ""));

        snprintf(
            backgroundName,
            sizeof(backgroundName),
            "%s",
            storedBackground.c_str()
        );
    }

    gain = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::GYRO_GAIN,
        prefs.getFloat(
            OpenDriftParameters::key(OpenDriftParameters::Id::GYRO_GAIN),
            OpenDriftParameters::Defaults::GYRO_GAIN
        )
    );

    deadband = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::DEADBAND,
        prefs.getFloat(
            OpenDriftParameters::key(OpenDriftParameters::Id::DEADBAND),
            OpenDriftParameters::Defaults::DEADBAND
        )
    );

    gyroReverse = prefs.getBool(
        OpenDriftParameters::key(OpenDriftParameters::Id::GYRO_REVERSE),
        false
    );

    bool maxCorrectionUsesFullSpan =
        prefs.getBool("maxSpanV1", false);

    if(prefs.isKey(OpenDriftParameters::key(OpenDriftParameters::Id::MAX_CORRECTION)))
    {
        gyroMaxCorrection = OpenDriftParameters::clamp(
            OpenDriftParameters::Id::MAX_CORRECTION,
            prefs.getInt(
                OpenDriftParameters::key(OpenDriftParameters::Id::MAX_CORRECTION),
                OpenDriftParameters::Defaults::MAX_CORRECTION
            )
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
        prefs.putInt(
            OpenDriftParameters::key(OpenDriftParameters::Id::MAX_CORRECTION),
            gyroMaxCorrection
        );
    }
    else
    {
        gyroMaxCorrection = OpenDriftParameters::Defaults::MAX_CORRECTION;
    }

    if(!maxCorrectionUsesFullSpan)
    {
        prefs.putInt(
            OpenDriftParameters::key(OpenDriftParameters::Id::MAX_CORRECTION),
            gyroMaxCorrection
        );
        prefs.putBool("maxSpanV1", true);
    }

    gyroSmoothing = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::SMOOTHING,
        prefs.getFloat(
            OpenDriftParameters::key(OpenDriftParameters::Id::SMOOTHING),
            OpenDriftParameters::Defaults::SMOOTHING
        )
    );

    gyroLpfMode = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::GYRO_LPF,
        (int)prefs.getUChar(
            OpenDriftParameters::key(OpenDriftParameters::Id::GYRO_LPF),
            OpenDriftParameters::Defaults::GYRO_LPF
        )
    );

    gyroIntegralGain = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::DRIFT_MEMORY,
        prefs.getFloat(
            OpenDriftParameters::key(OpenDriftParameters::Id::DRIFT_MEMORY),
            OpenDriftParameters::Defaults::DRIFT_MEMORY
        )
    );

    gyroIntegralLimit = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::MEMORY_LIMIT,
        prefs.getInt(
            OpenDriftParameters::key(OpenDriftParameters::Id::MEMORY_LIMIT),
            OpenDriftParameters::Defaults::MEMORY_LIMIT
        )
    );

    gyroHoldBoost = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::HOLD_ASSIST,
        prefs.getInt(
            OpenDriftParameters::key(OpenDriftParameters::Id::HOLD_ASSIST),
            OpenDriftParameters::Defaults::HOLD_ASSIST
        )
    );

    gyroCounterSteerAssist = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::COUNTERSTEER,
        prefs.getInt(
            OpenDriftParameters::key(OpenDriftParameters::Id::COUNTERSTEER),
            OpenDriftParameters::Defaults::COUNTERSTEER
        )
    );

    if(prefs.isKey(OpenDriftParameters::key(OpenDriftParameters::Id::TRANSITION_SPEED)))
    {
        gyroTransitionSpeed = OpenDriftParameters::clamp(
            OpenDriftParameters::Id::TRANSITION_SPEED,
            prefs.getInt(OpenDriftParameters::key(OpenDriftParameters::Id::TRANSITION_SPEED), OpenDriftParameters::Defaults::TRANSITION_SPEED)
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

        prefs.putInt(
            OpenDriftParameters::key(OpenDriftParameters::Id::TRANSITION_SPEED),
            gyroTransitionSpeed
        );
    }

    if(prefs.isKey(OpenDriftParameters::key(OpenDriftParameters::Id::PREDICTION)))
    {
        predictionStrength = OpenDriftParameters::clamp(
            OpenDriftParameters::Id::PREDICTION,
            prefs.getInt(
                OpenDriftParameters::key(OpenDriftParameters::Id::PREDICTION),
                OpenDriftParameters::Defaults::PREDICTION
            )
        );
    }
    else
    {
        predictionStrength = OpenDriftParameters::clamp(
            OpenDriftParameters::Id::PREDICTION,
            prefs.getInt("gyroHunt", 0)
        );
        prefs.putInt(
            OpenDriftParameters::key(OpenDriftParameters::Id::PREDICTION),
            predictionStrength
        );
    }

    gyroHuntStrength = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::ANTI_WOBBLE,
        prefs.getInt(OpenDriftParameters::key(OpenDriftParameters::Id::ANTI_WOBBLE), OpenDriftParameters::Defaults::ANTI_WOBBLE)
    );

    driverPriority = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::DRIVER_PRIORITY,
        prefs.getInt(OpenDriftParameters::key(OpenDriftParameters::Id::DRIVER_PRIORITY), OpenDriftParameters::Defaults::DRIVER_PRIORITY)
    );

    gyroOutputHysteresis = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::GYRO_HYSTERESIS,
        prefs.getInt(OpenDriftParameters::key(OpenDriftParameters::Id::GYRO_HYSTERESIS), OpenDriftParameters::Defaults::GYRO_HYSTERESIS)
    );

    antiWobbleScale = constrain(
        prefs.getUChar(OpenDriftParameters::key(OpenDriftParameters::Id::ANTI_WOBBLE_SCALE), 0),
        0,
        1
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
        OpenDriftParameters::key(OpenDriftParameters::Id::SERVO_CENTER),
        OpenDriftParameters::Defaults::SERVO_CENTER
    );

    servoReverse = prefs.getBool(
        OpenDriftParameters::key(OpenDriftParameters::Id::SERVO_REVERSE),
        false
    );

    servoTravel = prefs.getInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::SERVO_TRAVEL),
        OpenDriftParameters::Defaults::SERVO_TRAVEL
    );

    servoQuiet = prefs.getInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::SERVO_QUIET),
        OpenDriftParameters::Defaults::SERVO_QUIET
    );

    controlLoopHz =
        prefs.getUShort(OpenDriftParameters::key(OpenDriftParameters::Id::SERVO_RATE), 250) == 333
        ? 333
        : 250;

    setThrottleOutputHz(
        prefs.getUShort(OpenDriftParameters::key(OpenDriftParameters::Id::THROTTLE_RATE), 50)
    );

    // Loading a valid stored value is not a user edit.
    dirty = false;

    displayRotation = prefs.getUChar(
        OpenDriftParameters::key(OpenDriftParameters::Id::DISPLAY_ROTATION),
        #if defined(OPENDRIFT_BOARD_MATRIX)
        3
        #else
        0
        #endif
    );

    #if defined(OPENDRIFT_BOARD_MATRIX)
    displayRotation = constrain(displayRotation, 0, 3);
    #else
    displayRotation = displayRotation == 2 ? 2 : 0;
    #endif

    wifiEnabled = prefs.getBool(
        "wifi",
        true
    );

    wifiTimeout = prefs.getULong(
        "timeout",
        40000
    );

    setWifiSsid(
        prefs.getString("wifiSsid", "OpenDrift")
    );

    // Loading a valid stored value is not a user edit.
    dirty = false;

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
        OpenDriftParameters::key(OpenDriftParameters::Id::STEERING_TRAVEL),
        OpenDriftParameters::Defaults::STEERING_TRAVEL
    );

    gainMin = prefs.getInt(
        "gainMin",
        1000
    );

    gainMax = prefs.getInt(
        "gainMax",
        2000
    );

    channel3GainMin = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN,
        prefs.getFloat(
            OpenDriftParameters::key(OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN),
            OpenDriftParameters::Defaults::CHANNEL_3_GAIN_MIN
        )
    );

    channel3GainMax = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX,
        prefs.getFloat(
            OpenDriftParameters::key(OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX),
            OpenDriftParameters::Defaults::CHANNEL_3_GAIN_MAX
        )
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

void Settings::factoryReset()
{
    prefs.clear();
    dirty = false;
}

void Settings::save()
{
    prefs.putFloat(
        OpenDriftParameters::key(OpenDriftParameters::Id::GYRO_GAIN),
        gain
    );

    prefs.putFloat(
        OpenDriftParameters::key(OpenDriftParameters::Id::DEADBAND),
        deadband
    );

    prefs.putBool(
        OpenDriftParameters::key(OpenDriftParameters::Id::GYRO_REVERSE),
        gyroReverse
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::MAX_CORRECTION),
        gyroMaxCorrection
    );

    prefs.putFloat(
        OpenDriftParameters::key(OpenDriftParameters::Id::SMOOTHING),
        gyroSmoothing
    );

    prefs.putUChar(
        OpenDriftParameters::key(OpenDriftParameters::Id::GYRO_LPF),
        gyroLpfMode
    );

    prefs.putFloat(
        OpenDriftParameters::key(OpenDriftParameters::Id::DRIFT_MEMORY),
        gyroIntegralGain
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::MEMORY_LIMIT),
        gyroIntegralLimit
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::HOLD_ASSIST),
        gyroHoldBoost
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::COUNTERSTEER),
        gyroCounterSteerAssist
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::TRANSITION_SPEED),
        gyroTransitionSpeed
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::PREDICTION),
        predictionStrength
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::ANTI_WOBBLE),
        gyroHuntStrength
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::DRIVER_PRIORITY),
        driverPriority
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::GYRO_HYSTERESIS),
        gyroOutputHysteresis
    );

    prefs.putUChar(
        OpenDriftParameters::key(OpenDriftParameters::Id::ANTI_WOBBLE_SCALE),
        antiWobbleScale
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::SERVO_CENTER),
        servoCenter
    );

    prefs.putBool(
        OpenDriftParameters::key(OpenDriftParameters::Id::SERVO_REVERSE),
        servoReverse
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::SERVO_TRAVEL),
        servoTravel
    );

    prefs.putInt(
        OpenDriftParameters::key(OpenDriftParameters::Id::SERVO_QUIET),
        servoQuiet
    );

    prefs.putUShort(
        OpenDriftParameters::key(OpenDriftParameters::Id::SERVO_RATE),
        controlLoopHz
    );

    prefs.putUShort(
        OpenDriftParameters::key(OpenDriftParameters::Id::THROTTLE_RATE),
        throttleOutputHz
    );

    prefs.putUChar(
        OpenDriftParameters::key(OpenDriftParameters::Id::DISPLAY_ROTATION),
        displayRotation
    );

    prefs.putUChar(
        "dispBright",
        displayBrightness
    );

    prefs.putUShort(
        "dispDim",
        displayDimTimeout
    );

    prefs.putUChar(
        "thmText",
        themeText
    );

    prefs.putUChar(
        "thmAccent",
        themeAccent
    );

    prefs.putString(
        "bgName",
        backgroundName
    );

    prefs.putBool(
        "wifi",
        wifiEnabled
    );

    prefs.putULong(
        "timeout",
        wifiTimeout
    );

    prefs.putString(
        "wifiSsid",
        wifiSsid
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
        OpenDriftParameters::key(OpenDriftParameters::Id::STEERING_TRAVEL),
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
        OpenDriftParameters::key(OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN),
        channel3GainMin
    );

    prefs.putFloat(
        OpenDriftParameters::key(OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX),
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
    gain = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::GYRO_GAIN,
        value
    );
    dirty = true;
}

float Settings::getDeadband()
{
    return deadband;
}

void Settings::setDeadband(float value)
{
    deadband = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::DEADBAND,
        value
    );
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
    gyroMaxCorrection = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::MAX_CORRECTION,
        value
    );

    dirty = true;
}

float Settings::getGyroSmoothing()
{
    return gyroSmoothing;
}

void Settings::setGyroSmoothing(float value)
{
    gyroSmoothing = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::SMOOTHING,
        value
    );

    dirty = true;
}

uint8_t Settings::getGyroLpfMode()
{
    return gyroLpfMode;
}

void Settings::setGyroLpfMode(uint8_t value)
{
    gyroLpfMode = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::GYRO_LPF,
        (int)value
    );
    dirty = true;
}

float Settings::getGyroIntegralGain()
{
    return gyroIntegralGain;
}

void Settings::setGyroIntegralGain(float value)
{
    gyroIntegralGain = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::DRIFT_MEMORY,
        value
    );

    dirty = true;
}

int Settings::getGyroIntegralLimit()
{
    return gyroIntegralLimit;
}

void Settings::setGyroIntegralLimit(int value)
{
    gyroIntegralLimit = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::MEMORY_LIMIT,
        value
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
    gyroCounterSteerAssist = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::COUNTERSTEER,
        value
    );
    dirty = true;
}

int Settings::getGyroTransitionSpeed()
{
    return gyroTransitionSpeed;
}

void Settings::setGyroTransitionSpeed(int value)
{
    gyroTransitionSpeed = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::TRANSITION_SPEED,
        value
    );
    dirty = true;
}

void Settings::setGyroHoldBoost(int value)
{
    gyroHoldBoost = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::HOLD_ASSIST,
        value
    );

    dirty = true;
}

int Settings::getPredictionStrength()
{
    return predictionStrength;
}

void Settings::setPredictionStrength(int value)
{
    predictionStrength = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::PREDICTION,
        value
    );

    dirty = true;
}

int Settings::getGyroHuntStrength()
{
    return gyroHuntStrength;
}

void Settings::setGyroHuntStrength(int value)
{
    gyroHuntStrength = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::ANTI_WOBBLE,
        value
    );
    dirty = true;
}

int Settings::getDriverPriority()
{
    return driverPriority;
}

void Settings::setDriverPriority(int value)
{
    driverPriority = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::DRIVER_PRIORITY,
        value
    );
    dirty = true;
}


int Settings::getGyroOutputHysteresis()
{
    return gyroOutputHysteresis;
}


void Settings::setGyroOutputHysteresis(int value)
{
    gyroOutputHysteresis = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::GYRO_HYSTERESIS,
        value
    );
    dirty = true;
}

uint8_t Settings::getAntiWobbleScale()
{
    return antiWobbleScale;
}

void Settings::setAntiWobbleScale(uint8_t value)
{
    antiWobbleScale = value == 1 ? 1 : 0;
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
    value = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::SERVO_CENTER,
        value
    );

    if(servoCenter == value)
    {
        return;
    }

    servoCenter = value;
    // Physical endpoint calibration is authoritative once captured. Center
    // is retained as the fallback used only when calibration is explicitly
    // reset; changing it must not silently discard safe physical limits.
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

    bool calibrated = isSteeringCalibrated();

    portENTER_CRITICAL(&settingsMux);

    if(calibrated)
    {
        int swapped = steeringMin;
        steeringMin = steeringMax;
        steeringMax = swapped;
        steeringCapturedPulses[0] = steeringMin;
        steeringCapturedPulses[2] = steeringMax;
    }

    servoReverse = value;
    portEXIT_CRITICAL(&settingsMux);

    dirty = true;

    if(calibrated)
    {
        persistSteeringCalibration();
    }
}

int Settings::getServoTravel()
{
    return servoTravel;
}

void Settings::setServoTravel(int value)
{
    value = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::SERVO_TRAVEL,
        value
    );

    if(servoTravel == value)
    {
        return;
    }

    servoTravel = value;
    // Travel is likewise a fallback. Do not make an incidental UI or CRSF
    // write capable of disabling the calibrated hard stops while driving.
    dirty = true;
}

int Settings::getServoQuiet()
{
    return servoQuiet;
}

void Settings::setServoQuiet(int value)
{
    servoQuiet = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::SERVO_QUIET,
        value
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

uint16_t Settings::getThrottleOutputHz()
{
    return throttleOutputHz;
}

void Settings::setThrottleOutputHz(uint16_t value)
{
    throttleOutputHz =
        value == 333
        ? 333
        : (value == 250 ? 250 : 50);

    dirty = true;
}

uint8_t Settings::getDisplayRotation()
{
    return displayRotation;
}

void Settings::setDisplayRotation(uint8_t value)
{
    #if defined(OPENDRIFT_BOARD_MATRIX)
    displayRotation = constrain(value, 0, 3);
    #else
    displayRotation = value == 2 ? 2 : 0;
    #endif

    dirty = true;
}

uint8_t Settings::getDisplayBrightness()
{
    return displayBrightness;
}

void Settings::setDisplayBrightness(int value)
{
    int rounded = ((value + 5) / 10) * 10;
    displayBrightness = constrain(rounded, 10, 100);
    dirty = true;
}

uint16_t Settings::getDisplayDimTimeout()
{
    return displayDimTimeout;
}

void Settings::setDisplayDimTimeout(int value)
{
    displayDimTimeout = constrain(value, 0, 600);
    dirty = true;
}

const char* Settings::themeAccentName(uint8_t accent)
{
    static const char* const names[THEME_ACCENT_COUNT] =
    {
        "MIXED",
        "CYAN",
        "BLUE",
        "MAGENTA",
        "AMBER",
        "GREEN",
        "WHITE"
    };

    return accent < THEME_ACCENT_COUNT ? names[accent] : names[0];
}

uint8_t Settings::getThemeText()
{
    return themeText;
}

void Settings::setThemeText(int value)
{
    themeText = constrain(value, 0, 1);
    dirty = true;
}

uint8_t Settings::getThemeAccent()
{
    return themeAccent;
}

void Settings::setThemeAccent(int value)
{
    themeAccent = constrain(value, 0, THEME_ACCENT_COUNT - 1);
    dirty = true;
}

const char* Settings::getBackgroundName()
{
    return backgroundName;
}

void Settings::setBackgroundName(const String& value)
{
    const String clean = sanitizeBackgroundName(value);
    snprintf(backgroundName, sizeof(backgroundName), "%s", clean.c_str());
    dirty = true;
}

String Settings::sanitizeBackgroundName(const String& value)
{
    String input = value;
    input.trim();
    String clean;
    clean.reserve(BACKGROUND_NAME_LENGTH - 1);

    for(
        size_t i = 0;
        i < input.length() && clean.length() < BACKGROUND_NAME_LENGTH - 1;
        i++
    )
    {
        const char character = input.charAt(i);
        if(isAlphaNumeric(character) || character == '-' || character == '_')
        {
            clean += character;
        }
    }

    return clean;
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
    wifiTimeout =
        value == 0
        ? 0UL
        : constrain(value, 5000UL, 3600000UL);
    dirty = true;
}

const char* Settings::getWifiSsid()
{
    return wifiSsid;
}

void Settings::setWifiSsid(const String& value)
{
    String clean = value;
    clean.trim();

    String filtered;
    filtered.reserve(WIFI_SSID_LENGTH - 1);

    for(
        size_t i = 0;
        i < clean.length() &&
        filtered.length() < WIFI_SSID_LENGTH - 1;
        i++
    )
    {
        char character = clean.charAt(i);

        if(
            isAlphaNumeric(character) ||
            character == ' ' ||
            character == '-' ||
            character == '_' ||
            character == '.'
        )
        {
            filtered += character;
        }
    }

    filtered.trim();

    snprintf(
        wifiSsid,
        sizeof(wifiSsid),
        "%s",
        filtered.length() > 0 ? filtered.c_str() : "OpenDrift"
    );

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
    value = constrain(value, 900, 2100);

    if(steeringMin == value)
    {
        return;
    }

    portENTER_CRITICAL(&settingsMux);
    steeringMin = value;
    steeringCapturedPulses[0] = value;
    steeringCalibrationMask = 0;
    dirty = true;
    portEXIT_CRITICAL(&settingsMux);
    persistSteeringCalibration();
}

int Settings::getSteeringCenter()
{
    return steeringCenter;
}

void Settings::setSteeringCenter(int value)
{
    value = constrain(value, 900, 2100);

    if(steeringCenter == value)
    {
        return;
    }

    portENTER_CRITICAL(&settingsMux);
    steeringCenter = value;
    steeringCapturedPulses[1] = value;
    steeringCalibrationMask = 0;
    dirty = true;
    portEXIT_CRITICAL(&settingsMux);
    persistSteeringCalibration();
}

int Settings::getSteeringMax()
{
    return steeringMax;
}

void Settings::setSteeringMax(int value)
{
    value = constrain(value, 900, 2100);

    if(steeringMax == value)
    {
        return;
    }

    portENTER_CRITICAL(&settingsMux);
    steeringMax = value;
    steeringCapturedPulses[2] = value;
    steeringCalibrationMask = 0;
    dirty = true;
    portEXIT_CRITICAL(&settingsMux);
    persistSteeringCalibration();
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

void Settings::getSteeringCalibration(
    SteeringCalibration& out
)
{
    portENTER_CRITICAL(&settingsMux);

    out.mask = steeringCalibrationMask & 0x07;
    out.min = steeringMin;
    out.center = steeringCenter;
    out.max = steeringMax;
    out.inputMin = steeringCapturedInputPulses[0];
    out.inputCenter = steeringCapturedInputPulses[1];
    out.inputMax = steeringCapturedInputPulses[2];

    int leftDelta = out.min - out.center;
    int rightDelta = out.max - out.center;

    out.calibrated =
        out.mask == 0x07 &&
        abs(leftDelta) >= 10 &&
        abs(rightDelta) >= 10 &&
        leftDelta * rightDelta < 0;

    portEXIT_CRITICAL(&settingsMux);
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

    portENTER_CRITICAL(&settingsMux);

    int previousPhysical = steeringCapturedPulses[point];
    int previousInput = steeringCapturedInputPulses[point];
    uint8_t previousMask = steeringCalibrationMask & 0x07;

    steeringCapturedPulses[point] = physicalPulse;

    if(inputPulse >= 800 && inputPulse <= 2200)
    {
        steeringCapturedInputPulses[point] = inputPulse;
    }
    uint8_t proposedMask =
        previousMask | (1U << point);

    if(proposedMask != 0x07)
    {
        steeringCalibrationMask = proposedMask;
        dirty = true;
        portEXIT_CRITICAL(&settingsMux);
        persistSteeringCalibration();
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
        steeringCapturedPulses[point] = previousPhysical;
        steeringCapturedInputPulses[point] = previousInput;
        steeringCalibrationMask = previousMask & ~(1U << point);
        dirty = true;
        portEXIT_CRITICAL(&settingsMux);
        persistSteeringCalibration();
        return false;
    }

    // Publish the completed mask only after all three pulse values are valid
    // and committed. The controller task can never observe a completed mask
    // paired with stale endpoints.
    steeringMin = steeringCapturedPulses[0];
    steeringCenter = steeringCapturedPulses[1];
    steeringMax = steeringCapturedPulses[2];
    steeringCalibrationMask = proposedMask;
    dirty = true;
    portEXIT_CRITICAL(&settingsMux);
    persistSteeringCalibration();

    return true;
}

bool Settings::confirmStoredSteeringCalibration()
{
    portENTER_CRITICAL(&settingsMux);

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
        portEXIT_CRITICAL(&settingsMux);
        persistSteeringCalibration();
        return false;
    }

    steeringCapturedPulses[0] = steeringMin;
    steeringCapturedPulses[1] = steeringCenter;
    steeringCapturedPulses[2] = steeringMax;
    steeringCalibrationMask = 0x07;
    dirty = true;
    portEXIT_CRITICAL(&settingsMux);
    persistSteeringCalibration();

    return true;
}

bool Settings::setStoredSteeringEndpoints(
    int minimum,
    int center,
    int maximum
)
{
    minimum = constrain(minimum, 900, 2100);
    center = constrain(center, 900, 2100);
    maximum = constrain(maximum, 900, 2100);

    int leftDelta = minimum - center;
    int rightDelta = maximum - center;

    if(
        abs(leftDelta) < 10 ||
        abs(rightDelta) < 10 ||
        leftDelta * rightDelta >= 0
    )
    {
        return false;
    }

    portENTER_CRITICAL(&settingsMux);
    steeringMin = minimum;
    steeringCenter = center;
    steeringMax = maximum;
    steeringCapturedPulses[0] = minimum;
    steeringCapturedPulses[1] = center;
    steeringCapturedPulses[2] = maximum;
    steeringCalibrationMask = 0x07;
    dirty = true;
    portEXIT_CRITICAL(&settingsMux);

    persistSteeringCalibration();
    return true;
}

void Settings::clearSteeringCalibration()
{
    portENTER_CRITICAL(&settingsMux);
    steeringCalibrationMask = 0;
    dirty = true;
    portEXIT_CRITICAL(&settingsMux);
    persistSteeringCalibration();
}

void Settings::persistSteeringCalibration()
{
    int physical[3];
    int input[3];
    uint8_t mask;

    portENTER_CRITICAL(&settingsMux);
    for(uint8_t i = 0; i < 3; i++)
    {
        physical[i] = steeringCapturedPulses[i];
        input[i] = steeringCapturedInputPulses[i];
    }
    mask = steeringCalibrationMask & 0x07;
    portEXIT_CRITICAL(&settingsMux);

    // Invalidate first and publish the mask last. A power interruption during
    // these writes can leave calibration disabled, but can never enable a
    // partially written endpoint set.
    prefs.putUChar("servoCalM", 0);
    prefs.putBool("servoEndV1", true);
    prefs.putInt("servoCalL", physical[0]);
    prefs.putInt("servoCalC", physical[1]);
    prefs.putInt("servoCalR", physical[2]);
    prefs.putInt("servoInL", input[0]);
    prefs.putInt("servoInC", input[1]);
    prefs.putInt("servoInR", input[2]);
    prefs.putUChar("servoCalM", mask);
}

int Settings::getRadioSteeringTravel()
{
    return radioSteeringTravel;
}

void Settings::setRadioSteeringTravel(int value)
{
    radioSteeringTravel = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::STEERING_TRAVEL,
        value
    );

    dirty = true;
}

int Settings::getGainMin()
{
    return gainMin;
}

void Settings::setGainMin(int value)
{
    gainMin = constrain(value, 800, 2200);

    if(gainMax <= gainMin)
    {
        gainMax = min(2200, gainMin + 1);
    }
    dirty = true;
}

int Settings::getGainMax()
{
    return gainMax;
}

void Settings::setGainMax(int value)
{
    gainMax = constrain(value, 800, 2200);

    if(gainMin >= gainMax)
    {
        gainMin = max(800, gainMax - 1);
    }
    dirty = true;
}

float Settings::getChannel3GainMin()
{
    return channel3GainMin;
}

void Settings::setChannel3GainMin(float value)
{
    channel3GainMin = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN,
        value
    );

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
    channel3GainMax = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX,
        value
    );

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
        clampProfile(profiles[i]);
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

void Settings::clampProfile(
    DrivingProfile& profile
)
{
    profile.gain = constrain(profile.gain, 0.0f, 6.0f);
    profile.deadband = constrain(profile.deadband, 0.0f, 100.0f);
    profile.gyroSmoothing = constrain(profile.gyroSmoothing, 0.0f, 1.0f);
    profile.gyroIntegralGain = constrain(profile.gyroIntegralGain, 0.0f, 20.0f);
    profile.gyroMaxCorrection = constrain(profile.gyroMaxCorrection, 0, 100);
    profile.gyroIntegralLimit = constrain(profile.gyroIntegralLimit, 0, 500);
    profile.gyroHoldBoost = constrain(profile.gyroHoldBoost, 0, 100);
    profile.predictionStrength = constrain(profile.predictionStrength, 0, 100);
    profile.radioSteeringTravel = constrain(profile.radioSteeringTravel, 0, 100);
    profile.gyroCounterSteerAssist = constrain(profile.gyroCounterSteerAssist, 0, 100);
    profile.gyroTransitionSpeed = constrain(profile.gyroTransitionSpeed, 0, 100);
    profile.gyroHuntStrength = constrain(profile.gyroHuntStrength, 0, 100);
    profile.driverPriority = constrain(profile.driverPriority, 0, 50);
}

void Settings::loadProfiles()
{
    profileCount = constrain(
        (int)prefs.getUChar("profCnt", 0),
        0,
        (int)MAX_PROFILES
    );

    uint8_t storedProfileCount = profileCount;
    int storedActive = prefs.getChar("profAct", -1);
    int8_t mappedActive = -1;

    uint8_t loadedCount = 0;

    for(uint8_t i = 0; i < storedProfileCount; i++)
    {
        uint8_t loadedBefore = loadedCount;
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
                legacy.name[0] != '\0' &&
                (legacy.version == 8 || legacy.version == 9 || legacy.version == 10)
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
                profile.gyroMaxCorrection = legacy.gyroMaxCorrection;
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.predictionStrength;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTransitionSpeed = legacy.gyroTransitionSpeed;
                profile.gyroHuntStrength = legacy.gyroHuntStrength;

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
                loadedCount++;
            }
        }

        if(i == storedActive && loadedCount > loadedBefore)
        {
            mappedActive = (int8_t)(loadedCount - 1);
        }
    }

    profileCount = loadedCount;

    for(uint8_t i = 0; i < profileCount; i++)
    {
        clampProfile(profiles[i]);
        persistProfile(i);
    }

    for(uint8_t i = profileCount; i < storedProfileCount; i++)
    {
        char key[12];
        snprintf(key, sizeof(key), "prof%u", i);
        prefs.remove(key);
    }

    activeProfileIndex = mappedActive;
    prefs.putUChar("profCnt", profileCount);
    prefs.putChar("profAct", activeProfileIndex);
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
    profile.driverPriority = driverPriority;
}

void Settings::applyProfile(
    const DrivingProfile& profile
)
{
    gain = constrain(profile.gain, 0.0f, 6.0f);
    deadband = constrain(profile.deadband, 0.0f, 100.0f);
    gyroSmoothing = constrain(profile.gyroSmoothing, 0.0f, 1.0f);
    gyroIntegralGain = constrain(profile.gyroIntegralGain, 0.0f, 20.0f);
    gyroMaxCorrection = constrain(profile.gyroMaxCorrection, 0, 100);
    gyroIntegralLimit = constrain(profile.gyroIntegralLimit, 0, 500);
    gyroHoldBoost = constrain(profile.gyroHoldBoost, 0, 100);
    predictionStrength = constrain(profile.predictionStrength, 0, 100);
    radioSteeringTravel = constrain(profile.radioSteeringTravel, 0, 100);
    gyroCounterSteerAssist = constrain(profile.gyroCounterSteerAssist, 0, 100);
    gyroTransitionSpeed = constrain(profile.gyroTransitionSpeed, 0, 100);
    gyroHuntStrength = constrain(profile.gyroHuntStrength, 0, 100);
    driverPriority = constrain(profile.driverPriority, 0, 50);
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
