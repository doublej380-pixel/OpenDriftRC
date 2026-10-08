#include "Settings.h"

namespace
{
    void setProfileValue(
        Settings::DrivingProfile& profile,
        OpenDriftParameters::Id id,
        float value
    )
    {
        profile.values[(uint8_t)id] = value;
    }

    void initializeProfileDefaults(Settings::DrivingProfile& profile)
    {
        profile = Settings::DrivingProfile();
        const OpenDriftParameters::Definition* definitions =
            OpenDriftParameters::definitions();

        for(size_t index = 0; index < OpenDriftParameters::definitionCount(); index++)
        {
            const OpenDriftParameters::Definition& definition = definitions[index];
            if((definition.flags & OpenDriftParameters::PROFILE) != 0)
            {
                profile.values[(uint8_t)definition.id] = definition.defaultValue;
            }
        }
    }

    struct DrivingProfileV11
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
        int32_t driverPriority;
    };

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
    #if defined(OPENDRIFT_BOARD_ZERO) && defined(OPENDRIFT_INPUT_CRSF)
    prefs.begin("ODZeroCRSF", false);
    #elif defined(OPENDRIFT_BOARD_ZERO)
    prefs.begin("ODZeroPWM", false);
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

    antiWobbleScale = constrain(
        prefs.getUChar(OpenDriftParameters::key(OpenDriftParameters::Id::ANTI_WOBBLE_SCALE), 0),
        0,
        1
    );

    const char* retiredKeys[] = {
        "gyroAttack", "gyroReturn", "gyroWob", "gyroHunt",
        "strDamp", "huntSense", "terrainAssist", "gyroHyst"
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

    // Keep this in the legacy scratch field until ParameterStore migration
    // below. Calling the public setter here would write the new store before
    // we have decided whether a store blob already exists.
    {
        uint16_t storedThrottleRate = prefs.getUShort(
            OpenDriftParameters::key(OpenDriftParameters::Id::THROTTLE_RATE),
            50
        );
        throttleOutputHz = storedThrottleRate == 333
            ? 333
            : (storedThrottleRate == 250 ? 250 : 50);
    }

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

    // v1.0.9 introduces a single catalog-backed runtime store. On the first
    // boot, import the values loaded through the legacy per-key schema so
    // existing tunes survive unchanged. Later boots load the coherent blob.
    if(!parameterStore.load(prefs))
    {
        parameterStore.setInitial(OpenDriftParameters::Id::GYRO_GAIN, gain);
        parameterStore.setInitial(OpenDriftParameters::Id::DEADBAND, deadband);
        parameterStore.setInitial(OpenDriftParameters::Id::GYRO_REVERSE, gyroReverse ? 1.0f : 0.0f);
        parameterStore.setInitial(OpenDriftParameters::Id::MAX_CORRECTION, gyroMaxCorrection);
        parameterStore.setInitial(OpenDriftParameters::Id::SMOOTHING, gyroSmoothing);
        parameterStore.setInitial(OpenDriftParameters::Id::GYRO_LPF, gyroLpfMode);
        parameterStore.setInitial(OpenDriftParameters::Id::DRIFT_MEMORY, gyroIntegralGain);
        parameterStore.setInitial(OpenDriftParameters::Id::MEMORY_LIMIT, gyroIntegralLimit);
        parameterStore.setInitial(OpenDriftParameters::Id::HOLD_ASSIST, gyroHoldBoost);
        parameterStore.setInitial(OpenDriftParameters::Id::COUNTERSTEER, gyroCounterSteerAssist);
        parameterStore.setInitial(OpenDriftParameters::Id::TRANSITION_SPEED, gyroTransitionSpeed);
        parameterStore.setInitial(OpenDriftParameters::Id::PREDICTION, predictionStrength);
        parameterStore.setInitial(OpenDriftParameters::Id::ANTI_WOBBLE, gyroHuntStrength);
        parameterStore.setInitial(OpenDriftParameters::Id::DRIVER_PRIORITY, driverPriority);
        parameterStore.setInitial(OpenDriftParameters::Id::ANTI_WOBBLE_SCALE, antiWobbleScale);
        parameterStore.setInitial(OpenDriftParameters::Id::SERVO_CENTER, servoCenter);
        parameterStore.setInitial(OpenDriftParameters::Id::SERVO_REVERSE, servoReverse ? 1.0f : 0.0f);
        parameterStore.setInitial(OpenDriftParameters::Id::SERVO_TRAVEL, servoTravel);
        parameterStore.setInitial(OpenDriftParameters::Id::SERVO_QUIET, servoQuiet);
        parameterStore.setInitial(OpenDriftParameters::Id::SERVO_RATE, controlLoopHz == 333 ? 1.0f : 0.0f);
        parameterStore.setInitial(OpenDriftParameters::Id::THROTTLE_RATE, throttleOutputHz == 333 ? 2.0f : (throttleOutputHz == 250 ? 1.0f : 0.0f));
        parameterStore.setInitial(OpenDriftParameters::Id::DISPLAY_ROTATION, displayRotation);
        parameterStore.setInitial(OpenDriftParameters::Id::STEERING_TRAVEL, radioSteeringTravel);
        parameterStore.setInitial(OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN, channel3GainMin);
        parameterStore.setInitial(OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX, channel3GainMax);

        for(uint8_t index = 0; index < 8; index++)
        {
            parameterStore.setInitial(
                static_cast<OpenDriftParameters::Id>(
                    (uint8_t)OpenDriftParameters::Id::GPIO_1 + index
                ),
                auxChannels[index]
            );
        }

        parameterStore.save(prefs);
    }

    // Board-specific constraints remain explicit hardware policy.
    setDisplayRotation(parameterStore.getInt(OpenDriftParameters::Id::DISPLAY_ROTATION));
    setChannel3GainMin(parameterStore.get(OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN));
    setChannel3GainMax(parameterStore.get(OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX));
    if(parameterStore.hasDirtyValues()) parameterStore.save(prefs);
    dirty = false;

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
    parameterStore.resetDefaults();
    dirty = false;
}

void Settings::save()
{
    parameterStore.save(prefs);

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
        "gainMin",
        gainMin
    );

    prefs.putInt(
        "gainMax",
        gainMax
    );

    prefs.putBool(
        "thrOut",
        throttleOutputEnabled
    );

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

float Settings::getParameterValue(OpenDriftParameters::Id id) const
{
    return parameterStore.get(id);
}


bool Settings::setParameterValue(OpenDriftParameters::Id id, float value)
{
    bool changed = parameterStore.set(id, value);
    if(changed) dirty = true;
    return changed;
}


uint32_t Settings::getParameterGeneration() const
{
    return parameterStore.generation();
}


bool Settings::getControllerSnapshot(
    uint32_t& lastGeneration,
    ControllerSnapshot& snapshot
) const
{
    float values[ParameterStore::VALUE_COUNT];
    if(!parameterStore.snapshot(
        lastGeneration,
        values,
        ParameterStore::VALUE_COUNT
    )) return false;

    snapshot.gain = values[(uint8_t)OpenDriftParameters::Id::GYRO_GAIN];
    snapshot.deadband = values[(uint8_t)OpenDriftParameters::Id::DEADBAND];
    snapshot.smoothing = values[(uint8_t)OpenDriftParameters::Id::SMOOTHING];
    snapshot.driftMemory = values[(uint8_t)OpenDriftParameters::Id::DRIFT_MEMORY];
    snapshot.maxCorrection = lroundf(values[(uint8_t)OpenDriftParameters::Id::MAX_CORRECTION]);
    snapshot.memoryLimit = lroundf(values[(uint8_t)OpenDriftParameters::Id::MEMORY_LIMIT]);
    snapshot.holdAssist = lroundf(values[(uint8_t)OpenDriftParameters::Id::HOLD_ASSIST]);
    snapshot.countersteer = lroundf(values[(uint8_t)OpenDriftParameters::Id::COUNTERSTEER]);
    snapshot.transitionSpeed = lroundf(values[(uint8_t)OpenDriftParameters::Id::TRANSITION_SPEED]);
    snapshot.prediction = lroundf(values[(uint8_t)OpenDriftParameters::Id::PREDICTION]);
    snapshot.driverPriority = lroundf(values[(uint8_t)OpenDriftParameters::Id::DRIVER_PRIORITY]);
    snapshot.antiWobble = lroundf(values[(uint8_t)OpenDriftParameters::Id::ANTI_WOBBLE]);
    snapshot.antiWobbleScale = lroundf(values[(uint8_t)OpenDriftParameters::Id::ANTI_WOBBLE_SCALE]);
    snapshot.gyroLpfMode = lroundf(values[(uint8_t)OpenDriftParameters::Id::GYRO_LPF]);
    snapshot.gyroReverse = values[(uint8_t)OpenDriftParameters::Id::GYRO_REVERSE] >= 0.5f;
    return true;
}

float Settings::getGain()
{
    return parameterStore.get(OpenDriftParameters::Id::GYRO_GAIN);
}

void Settings::setGain(float value)
{
    setParameterValue(OpenDriftParameters::Id::GYRO_GAIN, value);
}

float Settings::getDeadband()
{
    return parameterStore.get(OpenDriftParameters::Id::DEADBAND);
}

void Settings::setDeadband(float value)
{
    setParameterValue(OpenDriftParameters::Id::DEADBAND, value);
}

bool Settings::getGyroReverse()
{
    return parameterStore.getBool(OpenDriftParameters::Id::GYRO_REVERSE);
}

void Settings::setGyroReverse(bool value)
{
    setParameterValue(OpenDriftParameters::Id::GYRO_REVERSE, value ? 1.0f : 0.0f);
}

int Settings::getGyroMaxCorrection()
{
    return parameterStore.getInt(OpenDriftParameters::Id::MAX_CORRECTION);
}

void Settings::setGyroMaxCorrection(int value)
{
    setParameterValue(OpenDriftParameters::Id::MAX_CORRECTION, value);
}

float Settings::getGyroSmoothing()
{
    return parameterStore.get(OpenDriftParameters::Id::SMOOTHING);
}

void Settings::setGyroSmoothing(float value)
{
    setParameterValue(OpenDriftParameters::Id::SMOOTHING, value);
}

uint8_t Settings::getGyroLpfMode()
{
    return parameterStore.getInt(OpenDriftParameters::Id::GYRO_LPF);
}

void Settings::setGyroLpfMode(uint8_t value)
{
    setParameterValue(OpenDriftParameters::Id::GYRO_LPF, value);
}

float Settings::getGyroIntegralGain()
{
    return parameterStore.get(OpenDriftParameters::Id::DRIFT_MEMORY);
}

void Settings::setGyroIntegralGain(float value)
{
    setParameterValue(OpenDriftParameters::Id::DRIFT_MEMORY, value);
}

int Settings::getGyroIntegralLimit()
{
    return parameterStore.getInt(OpenDriftParameters::Id::MEMORY_LIMIT);
}

void Settings::setGyroIntegralLimit(int value)
{
    setParameterValue(OpenDriftParameters::Id::MEMORY_LIMIT, value);
}

int Settings::getGyroHoldBoost()
{
    return parameterStore.getInt(OpenDriftParameters::Id::HOLD_ASSIST);
}

int Settings::getGyroCounterSteerAssist()
{
    return parameterStore.getInt(OpenDriftParameters::Id::COUNTERSTEER);
}

void Settings::setGyroCounterSteerAssist(int value)
{
    setParameterValue(OpenDriftParameters::Id::COUNTERSTEER, value);
}

int Settings::getGyroTransitionSpeed()
{
    return parameterStore.getInt(OpenDriftParameters::Id::TRANSITION_SPEED);
}

void Settings::setGyroTransitionSpeed(int value)
{
    setParameterValue(OpenDriftParameters::Id::TRANSITION_SPEED, value);
}

void Settings::setGyroHoldBoost(int value)
{
    setParameterValue(OpenDriftParameters::Id::HOLD_ASSIST, value);
}

int Settings::getPredictionStrength()
{
    return parameterStore.getInt(OpenDriftParameters::Id::PREDICTION);
}

void Settings::setPredictionStrength(int value)
{
    setParameterValue(OpenDriftParameters::Id::PREDICTION, value);
}

int Settings::getGyroHuntStrength()
{
    return parameterStore.getInt(OpenDriftParameters::Id::ANTI_WOBBLE);
}

void Settings::setGyroHuntStrength(int value)
{
    setParameterValue(OpenDriftParameters::Id::ANTI_WOBBLE, value);
}

int Settings::getDriverPriority()
{
    return parameterStore.getInt(OpenDriftParameters::Id::DRIVER_PRIORITY);
}

void Settings::setDriverPriority(int value)
{
    setParameterValue(OpenDriftParameters::Id::DRIVER_PRIORITY, value);
}


uint8_t Settings::getAntiWobbleScale()
{
    return parameterStore.getInt(OpenDriftParameters::Id::ANTI_WOBBLE_SCALE);
}

void Settings::setAntiWobbleScale(uint8_t value)
{
    setParameterValue(OpenDriftParameters::Id::ANTI_WOBBLE_SCALE, value == 1 ? 1.0f : 0.0f);
}


// --------------------
// Servo
// --------------------

int Settings::getServoCenter()
{
    return parameterStore.getInt(OpenDriftParameters::Id::SERVO_CENTER);
}

void Settings::setServoCenter(int value)
{
    value = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::SERVO_CENTER,
        value
    );

    if(getServoCenter() == value)
    {
        return;
    }

    parameterStore.set(OpenDriftParameters::Id::SERVO_CENTER, value);
    // Physical endpoint calibration is authoritative once captured. Center
    // is retained as the fallback used only when calibration is explicitly
    // reset; changing it must not silently discard safe physical limits.
    dirty = true;
}

bool Settings::getServoReverse()
{
    return parameterStore.getBool(OpenDriftParameters::Id::SERVO_REVERSE);
}

void Settings::setServoReverse(bool value)
{
    if(getServoReverse() == value)
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

    parameterStore.set(
        OpenDriftParameters::Id::SERVO_REVERSE,
        value ? 1.0f : 0.0f
    );
    portEXIT_CRITICAL(&settingsMux);

    dirty = true;

    if(calibrated)
    {
        persistSteeringCalibration();
    }
}

int Settings::getServoTravel()
{
    return parameterStore.getInt(OpenDriftParameters::Id::SERVO_TRAVEL);
}

void Settings::setServoTravel(int value)
{
    value = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::SERVO_TRAVEL,
        value
    );

    if(getServoTravel() == value)
    {
        return;
    }

    parameterStore.set(OpenDriftParameters::Id::SERVO_TRAVEL, value);
    // Travel is likewise a fallback. Do not make an incidental UI or CRSF
    // write capable of disabling the calibrated hard stops while driving.
    dirty = true;
}

int Settings::getServoQuiet()
{
    return parameterStore.getInt(OpenDriftParameters::Id::SERVO_QUIET);
}

void Settings::setServoQuiet(int value)
{
    setParameterValue(OpenDriftParameters::Id::SERVO_QUIET, value);
}

uint16_t Settings::getControlLoopHz()
{
    return parameterStore.getInt(OpenDriftParameters::Id::SERVO_RATE) == 1
        ? 333
        : 250;
}

void Settings::setControlLoopHz(uint16_t value)
{
    setParameterValue(
        OpenDriftParameters::Id::SERVO_RATE,
        value == 333 ? 1.0f : 0.0f
    );
}

uint16_t Settings::getThrottleOutputHz()
{
    int selection = parameterStore.getInt(OpenDriftParameters::Id::THROTTLE_RATE);
    return selection == 2 ? 333 : (selection == 1 ? 250 : 50);
}

void Settings::setThrottleOutputHz(uint16_t value)
{
    setParameterValue(
        OpenDriftParameters::Id::THROTTLE_RATE,
        value == 333 ? 2.0f : (value == 250 ? 1.0f : 0.0f)
    );
}

uint8_t Settings::getDisplayRotation()
{
    return parameterStore.getInt(OpenDriftParameters::Id::DISPLAY_ROTATION);
}

void Settings::setDisplayRotation(uint8_t value)
{
    #if defined(OPENDRIFT_BOARD_MATRIX)
    value = constrain(value, 0, 3);
    #else
    value = value == 2 ? 2 : 0;
    #endif

    setParameterValue(OpenDriftParameters::Id::DISPLAY_ROTATION, value);
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
    return parameterStore.getInt(OpenDriftParameters::Id::STEERING_TRAVEL);
}

void Settings::setRadioSteeringTravel(int value)
{
    setParameterValue(OpenDriftParameters::Id::STEERING_TRAVEL, value);
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
    return parameterStore.get(OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN);
}

void Settings::setChannel3GainMin(float value)
{
    value = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN,
        value
    );
    setParameterValue(OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN, value);

    if(getChannel3GainMax() < value)
    {
        setParameterValue(OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX, value);
    }
}

float Settings::getChannel3GainMax()
{
    return parameterStore.get(OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX);
}

void Settings::setChannel3GainMax(float value)
{
    value = OpenDriftParameters::clamp(
        OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX,
        value
    );
    setParameterValue(OpenDriftParameters::Id::CHANNEL_3_GAIN_MAX, value);

    if(getChannel3GainMin() > value)
    {
        setParameterValue(OpenDriftParameters::Id::CHANNEL_3_GAIN_MIN, value);
    }
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

    return parameterStore.getInt(
        static_cast<OpenDriftParameters::Id>(
            (uint8_t)OpenDriftParameters::Id::GPIO_1 + gpio - 1
        )
    );
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

    setParameterValue(
        static_cast<OpenDriftParameters::Id>(
            (uint8_t)OpenDriftParameters::Id::GPIO_1 + gpio - 1
        ),
        channel
    );
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


float Settings::getProfileValue(
    const DrivingProfile& profile,
    OpenDriftParameters::Id id
) const
{
    uint8_t index = (uint8_t)id;
    return index < ParameterStore::VALUE_COUNT
        ? profile.values[index]
        : 0.0f;
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

    initializeProfileDefaults(profile);

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
    parameterStore.clampProfile(
        profile.values,
        ParameterStore::VALUE_COUNT
    );
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
                stored.version == 12
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                profile = stored;
                profile.version = 12;
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                loadedCount++;
            }
        }
        else if(storedSize == sizeof(DrivingProfileV11))
        {
            DrivingProfileV11 legacy = {};

            if(
                prefs.getBytes(key, &legacy, sizeof(legacy)) == sizeof(legacy) &&
                legacy.name[0] != '\0' &&
                legacy.version == 11
            )
            {
                DrivingProfile& profile = profiles[loadedCount];
                initializeProfileDefaults(profile);
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                setProfileValue(profile, OpenDriftParameters::Id::GYRO_GAIN, legacy.gain);
                setProfileValue(profile, OpenDriftParameters::Id::DEADBAND, legacy.deadband);
                setProfileValue(profile, OpenDriftParameters::Id::SMOOTHING, legacy.gyroSmoothing);
                setProfileValue(profile, OpenDriftParameters::Id::DRIFT_MEMORY, legacy.gyroIntegralGain);
                setProfileValue(profile, OpenDriftParameters::Id::MAX_CORRECTION, legacy.gyroMaxCorrection);
                setProfileValue(profile, OpenDriftParameters::Id::MEMORY_LIMIT, legacy.gyroIntegralLimit);
                setProfileValue(profile, OpenDriftParameters::Id::HOLD_ASSIST, legacy.gyroHoldBoost);
                setProfileValue(profile, OpenDriftParameters::Id::PREDICTION, legacy.predictionStrength);
                setProfileValue(profile, OpenDriftParameters::Id::STEERING_TRAVEL, legacy.radioSteeringTravel);
                setProfileValue(profile, OpenDriftParameters::Id::COUNTERSTEER, legacy.gyroCounterSteerAssist);
                setProfileValue(profile, OpenDriftParameters::Id::TRANSITION_SPEED, legacy.gyroTransitionSpeed);
                setProfileValue(profile, OpenDriftParameters::Id::ANTI_WOBBLE, legacy.gyroHuntStrength);
                setProfileValue(profile, OpenDriftParameters::Id::DRIVER_PRIORITY, legacy.driverPriority);
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
                initializeProfileDefaults(profile);
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                setProfileValue(profile, OpenDriftParameters::Id::GYRO_GAIN, legacy.gain);
                setProfileValue(profile, OpenDriftParameters::Id::DEADBAND, legacy.deadband);
                setProfileValue(profile, OpenDriftParameters::Id::SMOOTHING, legacy.gyroSmoothing);
                setProfileValue(profile, OpenDriftParameters::Id::DRIFT_MEMORY, legacy.gyroIntegralGain);
                setProfileValue(profile, OpenDriftParameters::Id::MAX_CORRECTION, legacy.gyroMaxCorrection);
                setProfileValue(profile, OpenDriftParameters::Id::MEMORY_LIMIT, legacy.gyroIntegralLimit);
                setProfileValue(profile, OpenDriftParameters::Id::HOLD_ASSIST, legacy.gyroHoldBoost);
                setProfileValue(profile, OpenDriftParameters::Id::PREDICTION, legacy.predictionStrength);
                setProfileValue(profile, OpenDriftParameters::Id::STEERING_TRAVEL, legacy.radioSteeringTravel);
                setProfileValue(profile, OpenDriftParameters::Id::COUNTERSTEER, legacy.gyroCounterSteerAssist);
                setProfileValue(profile, OpenDriftParameters::Id::TRANSITION_SPEED, legacy.gyroTransitionSpeed);
                setProfileValue(profile, OpenDriftParameters::Id::ANTI_WOBBLE, legacy.gyroHuntStrength);

                if(legacy.version == 9)
                {
                    setProfileValue(
                        profile,
                        OpenDriftParameters::Id::MAX_CORRECTION,
                        centerSpanPercentToFullSpanPercent(legacy.gyroMaxCorrection)
                    );
                }
                else if(legacy.version == 8)
                {
                    setProfileValue(
                        profile,
                        OpenDriftParameters::Id::MAX_CORRECTION,
                        legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection)
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
                initializeProfileDefaults(profile);
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                setProfileValue(profile, OpenDriftParameters::Id::GYRO_GAIN, legacy.gain);
                setProfileValue(profile, OpenDriftParameters::Id::DEADBAND, legacy.deadband);
                setProfileValue(profile, OpenDriftParameters::Id::SMOOTHING, legacy.gyroSmoothing);
                setProfileValue(profile, OpenDriftParameters::Id::DRIFT_MEMORY, legacy.gyroIntegralGain);
                setProfileValue(profile, OpenDriftParameters::Id::MAX_CORRECTION, legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection));
                setProfileValue(profile, OpenDriftParameters::Id::MEMORY_LIMIT, legacy.gyroIntegralLimit);
                setProfileValue(profile, OpenDriftParameters::Id::HOLD_ASSIST, legacy.gyroHoldBoost);
                setProfileValue(profile, OpenDriftParameters::Id::PREDICTION, legacy.predictionStrength);
                setProfileValue(profile, OpenDriftParameters::Id::STEERING_TRAVEL, legacy.radioSteeringTravel);
                setProfileValue(profile, OpenDriftParameters::Id::COUNTERSTEER, legacy.gyroCounterSteerAssist);
                setProfileValue(profile, OpenDriftParameters::Id::TRANSITION_SPEED, legacy.gyroTransitionSpeed);
                setProfileValue(profile, OpenDriftParameters::Id::ANTI_WOBBLE, legacy.gyroHuntStrength);
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
                initializeProfileDefaults(profile);
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                setProfileValue(profile, OpenDriftParameters::Id::GYRO_GAIN, legacy.gain);
                setProfileValue(profile, OpenDriftParameters::Id::DEADBAND, legacy.deadband);
                setProfileValue(profile, OpenDriftParameters::Id::SMOOTHING, legacy.gyroSmoothing);
                setProfileValue(profile, OpenDriftParameters::Id::DRIFT_MEMORY, legacy.gyroIntegralGain);
                setProfileValue(profile, OpenDriftParameters::Id::MAX_CORRECTION, legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection));
                setProfileValue(profile, OpenDriftParameters::Id::MEMORY_LIMIT, legacy.gyroIntegralLimit);
                setProfileValue(profile, OpenDriftParameters::Id::HOLD_ASSIST, legacy.gyroHoldBoost);
                setProfileValue(profile, OpenDriftParameters::Id::PREDICTION, legacy.predictionStrength);
                setProfileValue(profile, OpenDriftParameters::Id::STEERING_TRAVEL, legacy.radioSteeringTravel);
                setProfileValue(profile, OpenDriftParameters::Id::COUNTERSTEER, legacy.gyroCounterSteerAssist);
                setProfileValue(profile, OpenDriftParameters::Id::TRANSITION_SPEED, legacy.gyroTransitionSpeed);
                setProfileValue(profile, OpenDriftParameters::Id::ANTI_WOBBLE, 50);
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
                initializeProfileDefaults(profile);
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                setProfileValue(profile, OpenDriftParameters::Id::GYRO_GAIN, legacy.gain);
                setProfileValue(profile, OpenDriftParameters::Id::DEADBAND, legacy.deadband);
                setProfileValue(profile, OpenDriftParameters::Id::SMOOTHING, legacy.gyroSmoothing);
                setProfileValue(profile, OpenDriftParameters::Id::DRIFT_MEMORY, legacy.gyroIntegralGain);
                setProfileValue(profile, OpenDriftParameters::Id::MAX_CORRECTION, legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection));
                setProfileValue(profile, OpenDriftParameters::Id::MEMORY_LIMIT, legacy.gyroIntegralLimit);
                setProfileValue(profile, OpenDriftParameters::Id::HOLD_ASSIST, legacy.gyroHoldBoost);
                setProfileValue(profile, OpenDriftParameters::Id::PREDICTION, legacy.predictionStrength);
                setProfileValue(profile, OpenDriftParameters::Id::STEERING_TRAVEL, legacy.radioSteeringTravel);
                setProfileValue(profile, OpenDriftParameters::Id::COUNTERSTEER, legacy.gyroCounterSteerAssist);
                setProfileValue(profile, OpenDriftParameters::Id::TRANSITION_SPEED, legacy.gyroTransitionSpeed);
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
                initializeProfileDefaults(profile);
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                setProfileValue(profile, OpenDriftParameters::Id::GYRO_GAIN, legacy.gain);
                setProfileValue(profile, OpenDriftParameters::Id::DEADBAND, legacy.deadband);
                setProfileValue(profile, OpenDriftParameters::Id::SMOOTHING, legacy.gyroSmoothing);
                setProfileValue(profile, OpenDriftParameters::Id::DRIFT_MEMORY, legacy.gyroIntegralGain);
                setProfileValue(profile, OpenDriftParameters::Id::MAX_CORRECTION, legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection));
                setProfileValue(profile, OpenDriftParameters::Id::MEMORY_LIMIT, legacy.gyroIntegralLimit);
                setProfileValue(profile, OpenDriftParameters::Id::HOLD_ASSIST, legacy.gyroHoldBoost);
                setProfileValue(profile, OpenDriftParameters::Id::PREDICTION, legacy.gyroHuntDamping);
                setProfileValue(profile, OpenDriftParameters::Id::STEERING_TRAVEL, legacy.radioSteeringTravel);
                setProfileValue(profile, OpenDriftParameters::Id::COUNTERSTEER, legacy.gyroCounterSteerAssist);
                setProfileValue(profile, OpenDriftParameters::Id::TRANSITION_SPEED, legacy.gyroTailSlideSpeed);
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
                initializeProfileDefaults(profile);
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                setProfileValue(profile, OpenDriftParameters::Id::GYRO_GAIN, legacy.gain);
                setProfileValue(profile, OpenDriftParameters::Id::DEADBAND, legacy.deadband);
                setProfileValue(profile, OpenDriftParameters::Id::SMOOTHING, legacy.gyroSmoothing);
                setProfileValue(profile, OpenDriftParameters::Id::DRIFT_MEMORY, legacy.gyroIntegralGain);
                setProfileValue(profile, OpenDriftParameters::Id::MAX_CORRECTION, legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection));
                setProfileValue(profile, OpenDriftParameters::Id::MEMORY_LIMIT, legacy.gyroIntegralLimit);
                setProfileValue(profile, OpenDriftParameters::Id::HOLD_ASSIST, legacy.gyroHoldBoost);
                setProfileValue(profile, OpenDriftParameters::Id::PREDICTION, legacy.gyroHuntDamping);
                setProfileValue(profile, OpenDriftParameters::Id::STEERING_TRAVEL, legacy.radioSteeringTravel);
                setProfileValue(profile, OpenDriftParameters::Id::COUNTERSTEER, legacy.gyroCounterSteerAssist);
                setProfileValue(
                    profile,
                    OpenDriftParameters::Id::TRANSITION_SPEED,
                    constrain(50 + legacy.gyroTailSlideSpeed / 2, 50, 100)
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
                initializeProfileDefaults(profile);
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                setProfileValue(profile, OpenDriftParameters::Id::GYRO_GAIN, legacy.gain);
                setProfileValue(profile, OpenDriftParameters::Id::DEADBAND, legacy.deadband);
                setProfileValue(profile, OpenDriftParameters::Id::SMOOTHING, legacy.gyroSmoothing);
                setProfileValue(profile, OpenDriftParameters::Id::DRIFT_MEMORY, legacy.gyroIntegralGain);
                setProfileValue(profile, OpenDriftParameters::Id::MAX_CORRECTION, legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection));
                setProfileValue(profile, OpenDriftParameters::Id::MEMORY_LIMIT, legacy.gyroIntegralLimit);
                setProfileValue(profile, OpenDriftParameters::Id::HOLD_ASSIST, legacy.gyroHoldBoost);
                setProfileValue(profile, OpenDriftParameters::Id::PREDICTION, legacy.gyroHuntDamping);
                setProfileValue(profile, OpenDriftParameters::Id::STEERING_TRAVEL, legacy.radioSteeringTravel);
                setProfileValue(profile, OpenDriftParameters::Id::COUNTERSTEER, legacy.gyroCounterSteerAssist);
                setProfileValue(profile, OpenDriftParameters::Id::TRANSITION_SPEED, 50);
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
                initializeProfileDefaults(profile);
                memcpy(profile.name, legacy.name, PROFILE_NAME_LENGTH);
                profile.name[PROFILE_NAME_LENGTH - 1] = '\0';
                setProfileValue(profile, OpenDriftParameters::Id::GYRO_GAIN, legacy.gain);
                setProfileValue(profile, OpenDriftParameters::Id::DEADBAND, legacy.deadband);
                setProfileValue(profile, OpenDriftParameters::Id::SMOOTHING, legacy.gyroSmoothing);
                setProfileValue(profile, OpenDriftParameters::Id::DRIFT_MEMORY, legacy.gyroIntegralGain);
                setProfileValue(profile, OpenDriftParameters::Id::MAX_CORRECTION, legacyMaxCorrectionToPercent(legacy.gyroMaxCorrection));
                setProfileValue(profile, OpenDriftParameters::Id::MEMORY_LIMIT, legacy.gyroIntegralLimit);
                setProfileValue(profile, OpenDriftParameters::Id::HOLD_ASSIST, legacy.gyroHoldBoost);
                setProfileValue(profile, OpenDriftParameters::Id::PREDICTION, legacy.gyroHuntDamping);
                setProfileValue(profile, OpenDriftParameters::Id::STEERING_TRAVEL, legacy.radioSteeringTravel);
                setProfileValue(profile, OpenDriftParameters::Id::COUNTERSTEER, 0);
                setProfileValue(profile, OpenDriftParameters::Id::TRANSITION_SPEED, 50);
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
    profile.version = 12;
    parameterStore.captureProfile(
        profile.values,
        ParameterStore::VALUE_COUNT
    );
}

void Settings::applyProfile(
    const DrivingProfile& profile
)
{
    if(parameterStore.applyProfile(
        profile.values,
        ParameterStore::VALUE_COUNT
    ))
    {
        dirty = true;
    }
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
