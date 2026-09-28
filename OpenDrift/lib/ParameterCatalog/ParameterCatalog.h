#pragma once

#include <stdint.h>


namespace OpenDriftParameters
{
    enum class Id : uint8_t
    {
        ROOT = 0,
        GYRO_GAIN = 1,
        DEADBAND = 2,
        MAX_CORRECTION = 3,
        SMOOTHING = 4,
        DRIFT_MEMORY = 5,
        MEMORY_LIMIT = 6,
        HOLD_ASSIST = 7,
        COUNTERSTEER = 8,
        TRANSITION_SPEED = 9,
        PREDICTION = 10,
        SERVO_QUIET = 11,
        STEERING_TRAVEL = 12,
        SERVO_TRAVEL = 13,
        SERVO_CENTER = 14,
        SERVO_REVERSE = 15,
        GYRO_REVERSE = 16,
        GPIO_1 = 17,
        GPIO_2 = 18,
        GPIO_3 = 19,
        GPIO_4 = 20,
        GPIO_5 = 21,
        GPIO_6 = 22,
        GPIO_7 = 23,
        GPIO_8 = 24,
        SERVO_RATE = 25,
        ANTI_WOBBLE = 26,
        ENDPOINT_STATUS = 27,
        CAPTURE_LEFT = 28,
        CAPTURE_CENTER = 29,
        CAPTURE_RIGHT = 30,
        RESET_CALIBRATION = 31,
        GYRO_LPF = 32,
        CHANNEL_3_GAIN_MIN = 33,
        CHANNEL_3_GAIN_MAX = 34,
        DISPLAY_ROTATION = 35,
        ANTI_WOBBLE_SCALE = 36,
        LIVE_GAIN = 37,
        DRIVER_PRIORITY = 38,
        THROTTLE_RATE = 39,
        ARCHIVE_LOG = 40,
        GYRO_HYSTERESIS = 41
    };

    enum class Type : uint8_t
    {
        NUMBER,
        SELECTION,
        STATUS,
        ACTION
    };

    enum Flags : uint16_t
    {
        NONE = 0,
        PERSISTENT = 1 << 0,
        READ_ONLY = 1 << 1,
        AMOLED_ONLY = 1 << 2,
        DISPLAY_BOARD_ONLY = 1 << 3,
        GPIO_OUTPUT = 1 << 4
    };

    struct Definition
    {
        Id id;
        const char* key;
        const char* name;
        Type type;
        float minimum;
        float maximum;
        float defaultValue;
        float step;
        uint8_t decimals;
        const char* unit;
        const char* choices;
        uint16_t flags;
    };

    // Published CRSF IDs are a compatibility contract. Never renumber an ID;
    // leave a tombstone in the catalog if a parameter is retired.
    static constexpr uint8_t MAX_PUBLISHED_ID = 41;

    const Definition* find(Id id);
    const Definition* find(uint8_t id);
    const char* key(Id id);
    bool isAvailable(const Definition& definition);
    bool isWritable(const Definition& definition);
    int32_t scaledMinimum(const Definition& definition);
    int32_t scaledMaximum(const Definition& definition);
    int32_t scaledDefault(const Definition& definition);
    int32_t scaledStep(const Definition& definition);
    float clamp(Id id, float value);
    int clamp(Id id, int value);

    namespace Defaults
    {
        static constexpr float GYRO_GAIN = 1.5f;
        static constexpr float DEADBAND = 2.0f;
        static constexpr int MAX_CORRECTION = 100;
        static constexpr float SMOOTHING = 0.10f;
        static constexpr float DRIFT_MEMORY = 0.0f;
        static constexpr int MEMORY_LIMIT = 120;
        static constexpr int HOLD_ASSIST = 0;
        static constexpr int COUNTERSTEER = 100;
        static constexpr int TRANSITION_SPEED = 50;
        static constexpr int PREDICTION = 0;
        static constexpr int SERVO_QUIET = 0;
        static constexpr int STEERING_TRAVEL = 100;
        static constexpr int SERVO_TRAVEL = 100;
        static constexpr int SERVO_CENTER = 1500;
        static constexpr int ANTI_WOBBLE = 50;
        static constexpr int GYRO_LPF = 0;
        static constexpr float CHANNEL_3_GAIN_MIN = 0.5f;
        static constexpr float CHANNEL_3_GAIN_MAX = 3.0f;
        static constexpr int DRIVER_PRIORITY = 0;
        static constexpr int GYRO_HYSTERESIS = 0;
    }
}
