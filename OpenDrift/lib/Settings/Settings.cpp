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

    gyroHuntDamping = prefs.getInt(
        "gyroHunt",
        0
    );

    steeringDamper = prefs.getInt(
        "strDamp",
        0
    );

    terrainAssistEnabled = prefs.getBool(
        "terrainAssist",
        true
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
        "gyroHunt",
        gyroHuntDamping
    );

    prefs.putInt(
        "strDamp",
        steeringDamper
    );

    prefs.putBool(
        "terrainAssist",
        terrainAssistEnabled
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
            200
        );

    dirty = true;
}

int Settings::getGyroHuntDamping()
{
    return gyroHuntDamping;
}

void Settings::setGyroHuntDamping(int value)
{
    gyroHuntDamping =
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


bool Settings::getTerrainAssistEnabled()
{
    return terrainAssistEnabled;
}


void Settings::setTerrainAssistEnabled(bool value)
{
    terrainAssistEnabled = value;
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

        if(
            prefs.getBytesLength(key) == sizeof(DrivingProfile) &&
            prefs.getBytes(
                key,
                &profiles[loadedCount],
                sizeof(DrivingProfile)
            ) == sizeof(DrivingProfile) &&
            profiles[loadedCount].version == 1 &&
            profiles[loadedCount].name[0] != '\0'
        )
        {
            profiles[loadedCount].name[PROFILE_NAME_LENGTH - 1] = '\0';
            loadedCount++;
        }
    }

    profileCount = loadedCount;

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
    profile.version = 1;
    profile.gain = gain;
    profile.deadband = deadband;
    profile.gyroSmoothing = gyroSmoothing;
    profile.gyroIntegralGain = gyroIntegralGain;
    profile.gyroMaxCorrection = gyroMaxCorrection;
    profile.gyroAttackSpeed = gyroAttackSpeed;
    profile.gyroReturnSpeed = gyroReturnSpeed;
    profile.gyroIntegralLimit = gyroIntegralLimit;
    profile.gyroHoldBoost = gyroHoldBoost;
    profile.gyroAntiWobble = gyroAntiWobble;
    profile.gyroHuntDamping = gyroHuntDamping;
    profile.steeringDamper = steeringDamper;
    profile.radioSteeringTravel = radioSteeringTravel;
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
    gyroAttackSpeed = profile.gyroAttackSpeed;
    gyroReturnSpeed = profile.gyroReturnSpeed;
    gyroIntegralLimit = profile.gyroIntegralLimit;
    gyroHoldBoost = profile.gyroHoldBoost;
    gyroAntiWobble = profile.gyroAntiWobble;
    gyroHuntDamping = profile.gyroHuntDamping;
    steeringDamper = profile.steeringDamper;
    radioSteeringTravel = profile.radioSteeringTravel;
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
