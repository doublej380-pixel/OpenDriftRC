#pragma once

#include <Arduino.h>

#include "CrsfInput.h"
#include "Settings.h"


class CrsfParameterDevice
{
public:

    void begin(
        CrsfInput& input,
        Settings& settings
    );

    void update();

    bool consumeSettingsChanged();


private:

    static constexpr uint8_t DEVICE_ADDRESS = 0xC8;
    static constexpr uint8_t PARAMETER_COUNT = 17;

    static constexpr uint8_t TYPE_PARAMETER_PING = 0x28;
    static constexpr uint8_t TYPE_DEVICE_INFO = 0x29;
    static constexpr uint8_t TYPE_PARAMETER_ENTRY = 0x2B;
    static constexpr uint8_t TYPE_PARAMETER_READ = 0x2C;
    static constexpr uint8_t TYPE_PARAMETER_WRITE = 0x2D;

    static constexpr uint8_t DATA_FLOAT = 0x08;
    static constexpr uint8_t DATA_SELECTION = 0x09;
    static constexpr uint8_t DATA_FOLDER = 0x0B;
    static constexpr uint8_t DATA_OUT_OF_RANGE = 0x7F;

public:

    struct FloatDefinition
    {
        const char* name;
        int32_t minimum;
        int32_t maximum;
        int32_t defaultValue;
        uint8_t decimals;
        int32_t step;
        const char* unit;
    };

private:

    CrsfInput* crsf = nullptr;
    Settings* settings = nullptr;
    bool settingsChanged = false;

    void processFrame(
        const CrsfInput::ExtendedFrame& frame
    );

    void sendDeviceInfo(
        uint8_t destination
    );

    void sendParameter(
        uint8_t parameter,
        uint8_t destination
    );

    void writeParameter(
        uint8_t parameter,
        const uint8_t* data,
        uint8_t length,
        uint8_t destination
    );

    int32_t getScaledValue(
        uint8_t parameter
    );

    void setScaledValue(
        uint8_t parameter,
        int32_t value
    );

    static const FloatDefinition* getFloatDefinition(
        uint8_t parameter
    );

    static void appendByte(
        uint8_t* buffer,
        uint8_t& length,
        uint8_t value
    );

    static void appendInt32(
        uint8_t* buffer,
        uint8_t& length,
        int32_t value
    );

    static void appendString(
        uint8_t* buffer,
        uint8_t& length,
        const char* value
    );

    static int32_t readInt32(
        const uint8_t* data
    );
};
