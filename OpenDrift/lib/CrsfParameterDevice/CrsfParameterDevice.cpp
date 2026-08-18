#include "CrsfParameterDevice.h"

#include <math.h>


namespace
{
    const CrsfParameterDevice::FloatDefinition FLOAT_PARAMETERS[] =
    {
        {"Active Gain",      50,  500, 150, 2,   5, "x"},
        {"Deadband",          0,  200,  20, 1,   1, "dps"},
        {"Max Correction",    0, 1000, 250, 0,  10, "us"},
        {"Smoothing",         1,  100,  10, 2,   1, ""},
        {"Drift Memory",      0, 2000,   0, 2,   1, ""},
        {"Memory Limit",      0,  500, 120, 0,   5, "us"},
        {"Hold Assist",       0,  100,   0, 0,   1, "%"},
        {"Countersteer",      0,  100,   0, 0,   1, "%"},
        {"Tail Slide Speed",  0,  100,  50, 0,   1, "%"},
        {"Prediction",        0,  100,   0, 0,   1, "%"},
        {"Servo Quiet",       0,   50,   0, 0,   1, "us"},
        {"Steering Travel",   0,  100, 100, 0,   1, "%"},
        {"Servo Travel",     10,  150, 100, 0,   1, "%"},
        {"Servo Center",   1000, 2000,1500, 0,   1, "us"}
    };
}


void CrsfParameterDevice::begin(
    CrsfInput& input,
    Settings& storedSettings,
    GyroController& activeGyro
)
{
    crsf = &input;
    settings = &storedSettings;
    gyro = &activeGyro;
}


void CrsfParameterDevice::update()
{
    if(crsf == nullptr || settings == nullptr)
    {
        return;
    }

    CrsfInput::ExtendedFrame frame;
    uint8_t processed = 0;

    while(
        processed < 4 &&
        crsf->popExtendedFrame(frame)
    )
    {
        processFrame(frame);
        processed++;
    }
}


bool CrsfParameterDevice::consumeSettingsChanged()
{
    bool changed = settingsChanged;
    settingsChanged = false;
    return changed;
}


void CrsfParameterDevice::processFrame(
    const CrsfInput::ExtendedFrame& frame
)
{
    if(
        frame.destination != DEVICE_ADDRESS &&
        frame.destination != 0x00
    )
    {
        return;
    }

    if(frame.type == TYPE_PARAMETER_PING)
    {
        sendDeviceInfo(frame.origin);
    }
    else if(
        frame.type == TYPE_PARAMETER_READ &&
        frame.payloadLength >= 2
    )
    {
        sendParameter(
            frame.payload[0],
            frame.origin
        );
    }
    else if(
        frame.type == TYPE_PARAMETER_WRITE &&
        frame.payloadLength >= 2
    )
    {
        writeParameter(
            frame.payload[0],
            &frame.payload[1],
            frame.payloadLength - 1,
            frame.origin
        );
    }
}


void CrsfParameterDevice::sendDeviceInfo(
    uint8_t destination
)
{
    uint8_t payload[48] = {0};
    uint8_t length = 0;

    appendString(payload, length, "OpenDrift");
    appendInt32(payload, length, 0x4F445243);
    appendInt32(payload, length, 0x00000128);
    appendInt32(payload, length, 0x00010000);
    appendByte(payload, length, PARAMETER_COUNT);
    appendByte(payload, length, 1);

    crsf->sendExtendedFrame(
        TYPE_DEVICE_INFO,
        destination,
        DEVICE_ADDRESS,
        payload,
        length
    );
}


void CrsfParameterDevice::sendParameter(
    uint8_t parameter,
    uint8_t destination
)
{
    uint8_t payload[CrsfInput::MAX_EXTENDED_PAYLOAD] = {0};
    uint8_t length = 0;

    appendByte(payload, length, parameter);
    appendByte(payload, length, 0);

    if(parameter == 0)
    {
        appendByte(payload, length, 0);
        appendByte(payload, length, DATA_FOLDER);
        appendString(payload, length, "ROOT");

        for(uint8_t child = 1; child < PARAMETER_COUNT; child++)
        {
            appendByte(payload, length, child);
        }

        appendByte(payload, length, 0xFF);
    }
    else if(parameter >= 1 && parameter <= 14)
    {
        const FloatDefinition* definition =
            getFloatDefinition(parameter);

        appendByte(payload, length, 0);
        appendByte(payload, length, DATA_FLOAT);
        appendString(payload, length, definition->name);
        appendInt32(payload, length, getScaledValue(parameter));
        appendInt32(payload, length, definition->minimum);
        appendInt32(payload, length, definition->maximum);
        appendInt32(payload, length, definition->defaultValue);
        appendByte(payload, length, definition->decimals);
        appendInt32(payload, length, definition->step);
        appendString(payload, length, definition->unit);
    }
    else if(
        parameter == 15 ||
        parameter == 16
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        || (parameter >= 17 && parameter <= 24)
        #endif
    )
    {
        appendByte(payload, length, 0);
        appendByte(payload, length, DATA_SELECTION);

        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        if(parameter >= 17)
        {
            uint8_t gpio = parameter - 16;
            bool available =
                #if defined(OPENDRIFT_AMOLED_V2)
                gpio >= 3;
                #else
                true;
                #endif

            char name[8];
            snprintf(
                name,
                sizeof(name),
                "GPIO%u",
                gpio
            );

            appendString(payload, length, name);
            appendString(payload, length, available ? "-;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16" : "RES");
            appendByte(payload, length, available ? getScaledValue(parameter) : 0);
            appendByte(payload, length, 0);
            appendByte(payload, length, available ? 16 : 0);
            appendByte(payload, length, 0);
        }
        else
        #endif
        {
            appendString(
                payload,
                length,
                parameter == 15 ? "Servo Reverse" : "Gyro Reverse"
            );
            appendString(payload, length, "Off;On");
            appendByte(payload, length, getScaledValue(parameter));
            appendByte(payload, length, 0);
            appendByte(payload, length, 1);
            appendByte(payload, length, 0);
        }

        appendString(payload, length, "");
    }
    else
    {
        appendByte(payload, length, 0);
        appendByte(payload, length, DATA_OUT_OF_RANGE);
    }

    crsf->sendExtendedFrame(
        TYPE_PARAMETER_ENTRY,
        destination,
        DEVICE_ADDRESS,
        payload,
        length
    );
}


void CrsfParameterDevice::writeParameter(
    uint8_t parameter,
    const uint8_t* data,
    uint8_t length,
    uint8_t destination
)
{
    int32_t value = 0;
    uint8_t valueLength = 0;

    if(parameter >= 1 && parameter <= 14 && length >= 4)
    {
        value = readInt32(data);
        valueLength = 4;
    }
    else if(
        (
            parameter == 15 ||
            parameter == 16
            #if defined(OPENDRIFT_BOARD_AMOLED_164)
            || (parameter >= 17 && parameter <= 24)
            #endif
        ) &&
        length >= 1
    )
    {
        value = data[0];
        valueLength = 1;
    }
    else
    {
        return;
    }

    setScaledValue(parameter, value);
    settingsChanged = true;

    uint8_t response[5] = {parameter, 0, 0, 0, 0};

    if(valueLength == 4)
    {
        uint8_t responseLength = 1;
        appendInt32(
            response,
            responseLength,
            getScaledValue(parameter)
        );
    }
    else
    {
        response[1] = (uint8_t)getScaledValue(parameter);
    }

    crsf->sendExtendedFrame(
        TYPE_PARAMETER_WRITE,
        destination,
        DEVICE_ADDRESS,
        response,
        valueLength + 1
    );
}


int32_t CrsfParameterDevice::getScaledValue(
    uint8_t parameter
)
{
    switch(parameter)
    {
        // Channel 3 is authoritative while it has a valid signal. Report the
        // controller's live value so EdgeTX never shows the saved fallback
        // while the car is actually running a different gain.
        case 1:
            return lroundf(
                (gyro != nullptr ? gyro->getGain() : settings->getGain())
                * 100.0f
            );
        case 2: return lroundf(settings->getDeadband() * 10.0f);
        case 3: return settings->getGyroMaxCorrection();
        case 4: return lroundf(settings->getGyroSmoothing() * 100.0f);
        case 5: return lroundf(settings->getGyroIntegralGain() * 100.0f);
        case 6: return settings->getGyroIntegralLimit();
        case 7: return settings->getGyroHoldBoost();
        case 8: return settings->getGyroCounterSteerAssist();
        case 9: return settings->getGyroTailSlideSpeed();
        case 10: return settings->getPredictionStrength();
        case 11: return settings->getServoQuiet();
        case 12: return settings->getRadioSteeringTravel();
        case 13: return settings->getServoTravel();
        case 14: return settings->getServoCenter();
        case 15: return settings->getServoReverse() ? 1 : 0;
        case 16: return settings->getGyroReverse() ? 1 : 0;
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        case 17: return settings->getAuxChannelForGpio(1);
        case 18: return settings->getAuxChannelForGpio(2);
        case 19: return settings->getAuxChannelForGpio(3);
        case 20: return settings->getAuxChannelForGpio(4);
        case 21: return settings->getAuxChannelForGpio(5);
        case 22: return settings->getAuxChannelForGpio(6);
        case 23: return settings->getAuxChannelForGpio(7);
        case 24: return settings->getAuxChannelForGpio(8);
        #endif
        default: return 0;
    }
}


void CrsfParameterDevice::setScaledValue(
    uint8_t parameter,
    int32_t value
)
{
    if(parameter >= 1 && parameter <= 14)
    {
        const FloatDefinition* definition =
            getFloatDefinition(parameter);

        value = constrain(
            value,
            definition->minimum,
            definition->maximum
        );
    }

    switch(parameter)
    {
        case 1: settings->setGain(value / 100.0f); break;
        case 2: settings->setDeadband(value / 10.0f); break;
        case 3: settings->setGyroMaxCorrection(value); break;
        case 4: settings->setGyroSmoothing(value / 100.0f); break;
        case 5: settings->setGyroIntegralGain(value / 100.0f); break;
        case 6: settings->setGyroIntegralLimit(value); break;
        case 7: settings->setGyroHoldBoost(value); break;
        case 8: settings->setGyroCounterSteerAssist(value); break;
        case 9: settings->setGyroTailSlideSpeed(value); break;
        case 10: settings->setPredictionStrength(value); break;
        case 11: settings->setServoQuiet(value); break;
        case 12: settings->setRadioSteeringTravel(value); break;
        case 13: settings->setServoTravel(value); break;
        case 14: settings->setServoCenter(value); break;
        case 15: settings->setServoReverse(value != 0); break;
        case 16: settings->setGyroReverse(value != 0); break;
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        {
            uint8_t gpio = parameter - 16;

            #if defined(OPENDRIFT_AMOLED_V2)
            if(gpio < 3)
            {
                value = 0;
            }
            #endif

            settings->setAuxChannelForGpio(
                gpio,
                constrain(value, 0, 16)
            );
            break;
        }
        #endif
    }
}


const CrsfParameterDevice::FloatDefinition*
CrsfParameterDevice::getFloatDefinition(
    uint8_t parameter
)
{
    return &FLOAT_PARAMETERS[parameter - 1];
}


void CrsfParameterDevice::appendByte(
    uint8_t* buffer,
    uint8_t& length,
    uint8_t value
)
{
    if(length < CrsfInput::MAX_EXTENDED_PAYLOAD)
    {
        buffer[length++] = value;
    }
}


void CrsfParameterDevice::appendInt32(
    uint8_t* buffer,
    uint8_t& length,
    int32_t value
)
{
    appendByte(buffer, length, (uint8_t)((uint32_t)value >> 24));
    appendByte(buffer, length, (uint8_t)((uint32_t)value >> 16));
    appendByte(buffer, length, (uint8_t)((uint32_t)value >> 8));
    appendByte(buffer, length, (uint8_t)value);
}


void CrsfParameterDevice::appendString(
    uint8_t* buffer,
    uint8_t& length,
    const char* value
)
{
    while(
        *value != '\0' &&
        length + 1 < CrsfInput::MAX_EXTENDED_PAYLOAD
    )
    {
        appendByte(buffer, length, (uint8_t)*value++);
    }

    appendByte(buffer, length, 0);
}


int32_t CrsfParameterDevice::readInt32(
    const uint8_t* data
)
{
    return (int32_t)(
        ((uint32_t)data[0] << 24) |
        ((uint32_t)data[1] << 16) |
        ((uint32_t)data[2] << 8) |
        (uint32_t)data[3]
    );
}
