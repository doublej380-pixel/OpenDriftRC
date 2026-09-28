#include "ParameterCatalog.h"

#include <math.h>


namespace OpenDriftParameters
{
    namespace
    {
        constexpr uint16_t P = PERSISTENT;
        constexpr uint16_t R = READ_ONLY;
        constexpr uint16_t A = AMOLED_ONLY;
        constexpr uint16_t D = DISPLAY_BOARD_ONLY;
        constexpr uint16_t G = GPIO_OUTPUT;
        constexpr uint16_t F = PROFILE;

        const Definition DEFINITIONS[] =
        {
            {Id::GYRO_GAIN, "gain", "Saved Gain", Type::NUMBER, 0.0f, 6.0f, Defaults::GYRO_GAIN, 0.05f, 2, "x", nullptr, P | F},
            {Id::DEADBAND, "deadband", "Deadband", Type::NUMBER, 0.0f, 100.0f, Defaults::DEADBAND, 0.1f, 1, "dps", nullptr, P | F},
            {Id::MAX_CORRECTION, "gyroMaxPct", "Max Correction", Type::NUMBER, 0.0f, 100.0f, Defaults::MAX_CORRECTION, 1.0f, 0, "%", nullptr, P | F},
            {Id::SMOOTHING, "gyroSmooth", "Smoothing", Type::NUMBER, 0.0f, 1.0f, Defaults::SMOOTHING, 0.01f, 2, "", nullptr, P | F},
            {Id::DRIFT_MEMORY, "gyroIGain", "Drift Memory", Type::NUMBER, 0.0f, 20.0f, Defaults::DRIFT_MEMORY, 0.01f, 2, "", nullptr, P | F},
            {Id::MEMORY_LIMIT, "gyroILim", "Memory Limit", Type::NUMBER, 0.0f, 500.0f, Defaults::MEMORY_LIMIT, 5.0f, 0, "us", nullptr, P | F},
            {Id::HOLD_ASSIST, "gyroHold", "Hold Assist", Type::NUMBER, 0.0f, 100.0f, Defaults::HOLD_ASSIST, 1.0f, 0, "%", nullptr, P | F},
            {Id::COUNTERSTEER, "counterAssist", "Countersteer", Type::NUMBER, 0.0f, 100.0f, Defaults::COUNTERSTEER, 1.0f, 0, "%", nullptr, P | F},
            {Id::TRANSITION_SPEED, "tailSpeedC", "Transition Speed", Type::NUMBER, 0.0f, 100.0f, Defaults::TRANSITION_SPEED, 1.0f, 0, "%", nullptr, P | F},
            {Id::PREDICTION, "prediction", "Prediction", Type::NUMBER, 0.0f, 100.0f, Defaults::PREDICTION, 1.0f, 0, "%", nullptr, P | F},
            {Id::SERVO_QUIET, "quiet", "Servo Quiet", Type::NUMBER, 0.0f, 50.0f, Defaults::SERVO_QUIET, 1.0f, 0, "us", nullptr, P},
            {Id::STEERING_TRAVEL, "strTravel", "Steering Travel", Type::NUMBER, 0.0f, 100.0f, Defaults::STEERING_TRAVEL, 1.0f, 0, "%", nullptr, P | F},
            {Id::SERVO_TRAVEL, "travel", "Servo Travel", Type::NUMBER, 1.0f, 100.0f, Defaults::SERVO_TRAVEL, 1.0f, 0, "%", nullptr, P},
            {Id::SERVO_CENTER, "center", "Servo Center", Type::NUMBER, 1000.0f, 2000.0f, Defaults::SERVO_CENTER, 1.0f, 0, "us", nullptr, P},
            {Id::SERVO_REVERSE, "reverse", "Servo Reverse", Type::SELECTION, 0.0f, 1.0f, 0.0f, 1.0f, 0, "", "Off;On", P},
            {Id::GYRO_REVERSE, "gyroRev", "Gyro Reverse", Type::SELECTION, 0.0f, 1.0f, 0.0f, 1.0f, 0, "", "Off;On", P},
            {Id::GPIO_1, "auxCh1", "GPIO1", Type::SELECTION, 0.0f, 16.0f, 0.0f, 1.0f, 0, "", "-;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16", P | A | G},
            {Id::GPIO_2, "auxCh2", "GPIO2", Type::SELECTION, 0.0f, 16.0f, 0.0f, 1.0f, 0, "", "-;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16", P | A | G},
            {Id::GPIO_3, "auxCh3", "GPIO3", Type::SELECTION, 0.0f, 16.0f, 0.0f, 1.0f, 0, "", "-;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16", P | A | G},
            {Id::GPIO_4, "auxCh4", "GPIO4", Type::SELECTION, 0.0f, 16.0f, 0.0f, 1.0f, 0, "", "-;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16", P | A | G},
            {Id::GPIO_5, "auxCh5", "GPIO5", Type::SELECTION, 0.0f, 16.0f, 0.0f, 1.0f, 0, "", "-;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16", P | A | G},
            {Id::GPIO_6, "auxCh6", "GPIO6", Type::SELECTION, 0.0f, 16.0f, 0.0f, 1.0f, 0, "", "-;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16", P | A | G},
            {Id::GPIO_7, "auxCh7", "GPIO7", Type::SELECTION, 0.0f, 16.0f, 0.0f, 1.0f, 0, "", "-;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16", P | A | G},
            {Id::GPIO_8, "auxCh8", "GPIO8", Type::SELECTION, 0.0f, 16.0f, 0.0f, 1.0f, 0, "", "-;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16", P | A | G},
            {Id::SERVO_RATE, "loopHz", "Servo Rate*", Type::SELECTION, 0.0f, 1.0f, 0.0f, 1.0f, 0, "", "250 Hz;333 Hz", P},
            {Id::ANTI_WOBBLE, "huntStrength", "Anti Wobble", Type::NUMBER, 0.0f, 100.0f, Defaults::ANTI_WOBBLE, 1.0f, 0, "%", nullptr, P | F},
            {Id::ENDPOINT_STATUS, "", "Endpoints", Type::STATUS, 0.0f, 2.0f, 0.0f, 1.0f, 0, "", "NOT CAL;PARTIAL;CALIBRATED", R},
            {Id::CAPTURE_LEFT, "", "Capture Left", Type::ACTION, 0.0f, 1.0f, 0.0f, 1.0f, 0, "", "READY;CAPTURE", NONE},
            {Id::CAPTURE_CENTER, "", "Capture Center", Type::ACTION, 0.0f, 1.0f, 0.0f, 1.0f, 0, "", "READY;CAPTURE", NONE},
            {Id::CAPTURE_RIGHT, "", "Capture Right", Type::ACTION, 0.0f, 1.0f, 0.0f, 1.0f, 0, "", "READY;CAPTURE", NONE},
            {Id::RESET_CALIBRATION, "", "Reset Cal", Type::ACTION, 0.0f, 1.0f, 0.0f, 1.0f, 0, "", "READY;RESET", NONE},
            {Id::GYRO_LPF, "gyroLpf", "Gyro LPF", Type::SELECTION, 0.0f, 2.0f, Defaults::GYRO_LPF, 1.0f, 0, "", "24 Hz;120 Hz;Off", P},
            {Id::CHANNEL_3_GAIN_MIN, "ch3GainLo", "CH3 Gain Min", Type::NUMBER, 0.0f, 6.0f, Defaults::CHANNEL_3_GAIN_MIN, 0.05f, 2, "x", nullptr, P},
            {Id::CHANNEL_3_GAIN_MAX, "ch3GainHi", "CH3 Gain Max", Type::NUMBER, 0.0f, 6.0f, Defaults::CHANNEL_3_GAIN_MAX, 0.05f, 2, "x", nullptr, P},
            {Id::DISPLAY_ROTATION, "displayRot", "Display Rotation", Type::SELECTION, 0.0f, 3.0f, 0.0f, 1.0f, 0, "", "0 deg;90 CW;180 deg;90 CCW", P | D},
            {Id::ANTI_WOBBLE_SCALE, "wobbleScale", "Anti Wobble Scale", Type::SELECTION, 0.0f, 1.0f, 0.0f, 1.0f, 0, "", "1/10;Micro", P | D},
            {Id::LIVE_GAIN, "", "Live Gain", Type::NUMBER, 0.0f, 6.0f, Defaults::GYRO_GAIN, 0.05f, 2, "x", nullptr, R},
            {Id::DRIVER_PRIORITY, "driverPrio", "Driver Priority", Type::NUMBER, 0.0f, 50.0f, Defaults::DRIVER_PRIORITY, 1.0f, 0, "%", nullptr, P | F},
            {Id::THROTTLE_RATE, "thrOutHz", "Throttle Rate*", Type::SELECTION, 0.0f, 2.0f, 0.0f, 1.0f, 0, "", "50 Hz;250 Hz;333 Hz", P},
            {Id::ARCHIVE_LOG, "", "Log", Type::ACTION, 0.0f, 8.0f, 0.0f, 1.0f, 0, "", "PARK;READY;BUSY;DONE;EMPTY;FULL;FAIL;STOP;NA", NONE}
        };

        int32_t scale(const Definition& definition, float value)
        {
            float multiplier = 1.0f;
            for(uint8_t index = 0; index < definition.decimals; index++)
            {
                multiplier *= 10.0f;
            }
            return lroundf(value * multiplier);
        }
    }

    const Definition* find(Id id)
    {
        for(const Definition& definition : DEFINITIONS)
        {
            if(definition.id == id) return &definition;
        }
        return nullptr;
    }

    const Definition* find(uint8_t id)
    {
        return find(static_cast<Id>(id));
    }

    const Definition* definitions()
    {
        return DEFINITIONS;
    }

    size_t definitionCount()
    {
        return sizeof(DEFINITIONS) / sizeof(DEFINITIONS[0]);
    }

    const char* key(Id id)
    {
        const Definition* definition = find(id);
        return definition != nullptr ? definition->key : "";
    }

    bool isAvailable(const Definition& definition)
    {
        if((definition.flags & AMOLED_ONLY) != 0)
        {
            #if !defined(OPENDRIFT_BOARD_AMOLED_164)
            return false;
            #endif
        }

        if((definition.flags & DISPLAY_BOARD_ONLY) != 0)
        {
            #if !defined(OPENDRIFT_BOARD_AMOLED_164) && !defined(OPENDRIFT_BOARD_MATRIX)
            return false;
            #endif
        }

        return true;
    }

    bool isWritable(const Definition& definition)
    {
        return (definition.flags & READ_ONLY) == 0;
    }

    int32_t scaledMinimum(const Definition& definition) { return scale(definition, definition.minimum); }
    int32_t scaledMaximum(const Definition& definition) { return scale(definition, definition.maximum); }
    int32_t scaledDefault(const Definition& definition) { return scale(definition, definition.defaultValue); }
    int32_t scaledStep(const Definition& definition) { return scale(definition, definition.step); }
    int32_t scaleValue(const Definition& definition, float value) { return scale(definition, value); }

    float unscaleValue(const Definition& definition, int32_t value)
    {
        float divisor = 1.0f;
        for(uint8_t index = 0; index < definition.decimals; index++)
        {
            divisor *= 10.0f;
        }
        return value / divisor;
    }

    float clamp(Id id, float value)
    {
        const Definition* definition = find(id);
        if(definition == nullptr) return value;
        if(value < definition->minimum) return definition->minimum;
        if(value > definition->maximum) return definition->maximum;
        return value;
    }

    int clamp(Id id, int value)
    {
        const Definition* definition = find(id);
        if(definition == nullptr) return value;
        const int minimum = lroundf(definition->minimum);
        const int maximum = lroundf(definition->maximum);
        if(value < minimum) return minimum;
        if(value > maximum) return maximum;
        return value;
    }
}
