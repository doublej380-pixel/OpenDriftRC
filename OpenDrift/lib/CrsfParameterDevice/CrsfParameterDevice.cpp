#include "CrsfParameterDevice.h"

#include <math.h>


void CrsfParameterDevice::begin(
    CrsfInput& input,
    Settings& storedSettings,
    GyroController& activeGyro,
    RadioInput& activeSteeringRadio,
    ServoOutput& activeSteeringServo
)
{
    crsf = &input;
    settings = &storedSettings;
    gyro = &activeGyro;
    steeringRadio = &activeSteeringRadio;
    steeringServo = &activeSteeringServo;
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


void CrsfParameterDevice::setBlackboxArchive(
    BlackboxArchive& archive
)
{
    blackboxArchive = &archive;
}


void CrsfParameterDevice::setControlDiagnostics(
    ControlDiagnostics& diagnostics
)
{
    controlDiagnostics = &diagnostics;
}


void CrsfParameterDevice::setUsbMaintenanceCallback(
    void (*callback)()
)
{
    usbMaintenanceCallback = callback;
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

        for(uint8_t child = 1; child <= PARAMETER_COUNT; child++)
        {
            const OpenDriftParameters::Definition* definition =
                OpenDriftParameters::find(child);

            if(
                definition != nullptr &&
                OpenDriftParameters::isAvailable(*definition)
            )
            {
                appendByte(payload, length, child);
            }
        }

        appendByte(payload, length, 0xFF);
    }
    else
    {
        const OpenDriftParameters::Definition* definition =
            OpenDriftParameters::find(parameter);

        if(
            definition == nullptr ||
            !OpenDriftParameters::isAvailable(*definition)
        )
        {
            appendByte(payload, length, 0);
            appendByte(payload, length, DATA_OUT_OF_RANGE);
        }
        else if(definition->type == OpenDriftParameters::Type::NUMBER)
        {
            appendByte(payload, length, 0);
            appendByte(payload, length, DATA_FLOAT);
            appendString(payload, length, definition->name);
            appendInt32(payload, length, getScaledValue(parameter));
            appendInt32(
                payload,
                length,
                OpenDriftParameters::scaledMinimum(*definition)
            );
            appendInt32(
                payload,
                length,
                OpenDriftParameters::scaledMaximum(*definition)
            );
            appendInt32(
                payload,
                length,
                OpenDriftParameters::scaledDefault(*definition)
            );
            appendByte(payload, length, definition->decimals);
            appendInt32(
                payload,
                length,
                OpenDriftParameters::scaledStep(*definition)
            );
            appendString(payload, length, definition->unit);
        }
        else
        {
            const char* choices = definition->choices;
            uint8_t minimum = (uint8_t)definition->minimum;
            uint8_t maximum = (uint8_t)definition->maximum;
            uint8_t defaultValue = (uint8_t)definition->defaultValue;
            uint8_t currentValue = (uint8_t)getScaledValue(parameter);

            if((definition->flags & OpenDriftParameters::GPIO_OUTPUT) != 0)
            {
                const uint8_t gpio = parameter - 16;
                bool gpioAvailable =
                #if defined(OPENDRIFT_AMOLED_V2)
                gpio >= 3
                #if defined(OPENDRIFT_CRSF_V2_THROTTLE_GPIO8)
                && gpio != 8
                #endif
                ;
                #else
                true;
                #endif

                if(!gpioAvailable)
                {
                    choices = "RES";
                    maximum = 0;
                    currentValue = 0;
                }
            }

            if(parameter == (uint8_t)OpenDriftParameters::Id::DISPLAY_ROTATION)
            {
                #if defined(OPENDRIFT_BOARD_MATRIX)
                defaultValue = 3;
                #else
                choices = "Normal;180 deg";
                maximum = 1;
                defaultValue = 0;
                #endif
            }

            appendByte(payload, length, 0);
            appendByte(payload, length, DATA_SELECTION);
            appendString(payload, length, definition->name);
            appendString(payload, length, choices != nullptr ? choices : "");
            appendByte(payload, length, currentValue);
            appendByte(payload, length, minimum);
            appendByte(payload, length, maximum);
            appendByte(payload, length, defaultValue);

            appendString(payload, length, "");
        }
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
    const OpenDriftParameters::Definition* definition =
        OpenDriftParameters::find(parameter);

    if(
        definition != nullptr &&
        OpenDriftParameters::isAvailable(*definition) &&
        OpenDriftParameters::isWritable(*definition) &&
        definition->type == OpenDriftParameters::Type::NUMBER &&
        length >= 4
    )
    {
        value = readInt32(data);
        valueLength = 4;
    }
    else if(
        definition != nullptr &&
        OpenDriftParameters::isAvailable(*definition) &&
        OpenDriftParameters::isWritable(*definition) &&
        definition->type != OpenDriftParameters::Type::NUMBER &&
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

    if(
        parameter == (uint8_t)OpenDriftParameters::Id::ARCHIVE_LOG ||
        parameter == (uint8_t)OpenDriftParameters::Id::CONTROL_DIAGNOSTICS ||
        parameter == (uint8_t)OpenDriftParameters::Id::USB_MAINTENANCE
    )
    {
        setScaledValue(parameter, value);
    }
    else
    {
        setScaledValue(parameter, value);
        settingsChanged = true;
    }

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
    const OpenDriftParameters::Definition* definition =
        OpenDriftParameters::find(parameter);

    switch(parameter)
    {
        case 27:
        {
            uint8_t mask =
                settings->getSteeringCalibrationMask();

            return settings->isSteeringCalibrated()
                ? 2
                : (mask != 0 ? 1 : 0);
        }
        case 28:
        case 29:
        case 30:
        case 31:
            return 0;
        case 37:
            return lroundf(
                (gyro != nullptr ? gyro->getGain() : settings->getGain())
                * 100.0f
            );
        case 40:
            return blackboxArchive != nullptr
                ? (int32_t)blackboxArchive->getStatus()
                : (int32_t)BlackboxArchive::UNAVAILABLE;
        case 42:
            if(controlDiagnostics == nullptr)
            {
                return 0;
            }
            if(controlDiagnostics->isCapturing())
            {
                return 2;
            }
            return controlDiagnostics->count() > 0 ? 3 : 0;
        case 43:
            return 0;
        #if defined(OPENDRIFT_BOARD_AMOLED_164) && !defined(OPENDRIFT_BOARD_MATRIX)
        case 35: return settings->getDisplayRotation() == 2 ? 1 : 0;
        #endif
        default:
            return definition != nullptr
                ? OpenDriftParameters::scaleValue(
                    *definition,
                    settings->getParameterValue(definition->id)
                )
                : 0;
    }
}


void CrsfParameterDevice::setScaledValue(
    uint8_t parameter,
    int32_t value
)
{
    const OpenDriftParameters::Definition* definition =
        OpenDriftParameters::find(parameter);

    if(definition != nullptr)
    {
        value = constrain(
            value,
            OpenDriftParameters::scaledMinimum(*definition),
            OpenDriftParameters::scaledMaximum(*definition)
        );
    }

    switch(parameter)
    {
        case 15: settings->setServoReverse(value != 0); break;
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
            if(
                gpio < 3
                #if defined(OPENDRIFT_CRSF_V2_THROTTLE_GPIO8)
                || gpio == 8
                #endif
            )
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
        case 40:
            if(value == 1 && blackboxArchive != nullptr)
            {
                blackboxArchive->requestSave();
            }
            break;
        case 42:
            if(value == 1 && controlDiagnostics != nullptr)
            {
                controlDiagnostics->start();
            }
            break;
        case 43:
            if(value == 1 && usbMaintenanceCallback != nullptr)
            {
                usbMaintenanceCallback();
            }
            break;
        case 27:
            // Status is read-only. It is derived from the shared persisted
            // calibration mask used by both the display and EdgeTX.
            break;
        case 28:
        case 29:
        case 30:
            if(
                value == 1 &&
                !settings->isSteeringCalibrated() &&
                steeringRadio != nullptr &&
                steeringRadio->hasSignal() &&
                steeringServo != nullptr
            )
            {
                settings->captureSteeringCalibrationPoint(
                    parameter - 28,
                    steeringServo->getCommandPosition(),
                    steeringRadio->getPulseWidth()
                );
            }
            break;
        case 31:
            if(value == 1)
            {
                settings->clearSteeringCalibration();
            }
            break;
        case 33:
            settings->setChannel3GainMin(value / 100.0f);
            break;
        case 34:
            settings->setChannel3GainMax(value / 100.0f);
            break;
        #if defined(OPENDRIFT_BOARD_MATRIX)
        case 35:
            settings->setDisplayRotation(constrain(value, 0, 3));
            break;
        #elif defined(OPENDRIFT_BOARD_AMOLED_164)
        case 35:
            settings->setDisplayRotation(value == 1 ? 2 : 0);
            break;
        #endif
        default:
            if(definition != nullptr)
            {
                settings->setParameterValue(
                    definition->id,
                    OpenDriftParameters::unscaleValue(*definition, value)
                );
            }
            break;
    }
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
