#include "Settings.h"

namespace
{
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
}

bool Settings::begin()
{
    #if defined(OPENDRIFT_INPUT_CRSF)
    // Keep experimental CRSF tuning completely separate from the RC1 PWM
    // build, even when both firmwares are flashed onto the same board.
    prefs.begin("OpenDriftCRSF", false);
    #else
    prefs.begin("OpenDrift", false);
    #endif

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
        gyroTailSlideSpeed = constrain(
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
        gyroTailSlideSpeed = constrain(
            50 + legacyTailSlideSpeed / 2,
            50,
            100
        );

        prefs.putInt("tailSpeedC", gyroTailSlideSpeed);
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

    const char* retiredKeys[] = {
        "gyroAttack", "gyroReturn", "gyroWob", "gyroHunt",
        "strDamp", "terrainAssist"
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

    throttleOutputEnabled = prefs.getBool(
        "thrOut",
        false
    );

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
        "gyroMax",
        gyroMaxCorrection
    );

    prefs.putFloat(
        "gyroSmooth",
        gyroSmoothing
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
        gyroTailSlideSpeed
    );

    prefs.putInt(
        "prediction",
        predictionStrength
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

int Settings::getGyroTailSlideSpeed()
{
    return gyroTailSlideSpeed;
}

void Settings::setGyroTailSlideSpeed(int value)
{
    gyroTailSlideSpeed = constrain(value, 0, 100);
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

bool Settings::getThrottleOutputEnabled()
{
    return throttleOutputEnabled;
}

void Settings::setThrottleOutputEnabled(bool value)
{
    throttleOutputEnabled = value;
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

        if(
            storedSize == sizeof(DrivingProfile) &&
            prefs.getBytes(
                key,
                &profiles[loadedCount],
                sizeof(DrivingProfile)
            ) == sizeof(DrivingProfile) &&
            profiles[loadedCount].version == 5 &&
            profiles[loadedCount].name[0] != '\0'
        )
        {
            profiles[loadedCount].name[PROFILE_NAME_LENGTH - 1] = '\0';
            loadedCount++;
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
                profile.gyroMaxCorrection = legacy.gyroMaxCorrection;
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.gyroHuntDamping;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTailSlideSpeed = legacy.gyroTailSlideSpeed;
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
                profile.gyroMaxCorrection = legacy.gyroMaxCorrection;
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.gyroHuntDamping;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTailSlideSpeed = constrain(
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
                profile.gyroMaxCorrection = legacy.gyroMaxCorrection;
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.gyroHuntDamping;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = legacy.gyroCounterSteerAssist;
                profile.gyroTailSlideSpeed = 50;
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
                profile.gyroMaxCorrection = legacy.gyroMaxCorrection;
                profile.gyroIntegralLimit = legacy.gyroIntegralLimit;
                profile.gyroHoldBoost = legacy.gyroHoldBoost;
                profile.predictionStrength = legacy.gyroHuntDamping;
                profile.radioSteeringTravel = legacy.radioSteeringTravel;
                profile.gyroCounterSteerAssist = 0;
                profile.gyroTailSlideSpeed = 50;
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
    profile.version = 5;
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
    profile.gyroTailSlideSpeed = gyroTailSlideSpeed;
}

void Settings::applyProfile(
    const DrivingProfile& profile
)
{
    gain = profile.gain;
    deadband = profile.deadband;
    gyroSmoothing = profile.gyroSmoothing;
    gyroIntegralGain = profile.gyroIntegralGain;
    gyroMaxCorrection = profile.gyroMaxCorrection;
    gyroIntegralLimit = profile.gyroIntegralLimit;
    gyroHoldBoost = profile.gyroHoldBoost;
    predictionStrength = profile.predictionStrength;
    radioSteeringTravel = profile.radioSteeringTravel;
    gyroCounterSteerAssist = profile.gyroCounterSteerAssist;
    gyroTailSlideSpeed = profile.gyroTailSlideSpeed;
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
