#include "CrsfInput.h"


bool CrsfInput::begin(
    int8_t rxPin,
    int8_t txPin
)
{
    end();

    framePosition = 0;
    expectedFrameSize = 0;
    lastChannelFrameMicros = 0;
    receivedByteCount = 0;
    validFrameCount = 0;
    channelFrameCount = 0;
    crcErrorCount = 0;
    uplinkLinkQuality = 0;
    uplinkSnr = 0;
    linkStatisticsReady = false;
    extendedQueueHead = 0;
    extendedQueueTail = 0;
    channelsReady = false;

    for(uint8_t i = 0; i < CHANNEL_COUNT; i++)
    {
        channels[i] = 992;
    }

    serial.setRxBufferSize(1024);
    serial.begin(
        BAUD_RATE,
        SERIAL_8N1,
        rxPin,
        txPin
    );

    active = true;

    return true;
}


void CrsfInput::end()
{
    if(active)
    {
        serial.end();
    }

    active = false;
    channelsReady = false;
    lastChannelFrameMicros = 0;
    framePosition = 0;
    expectedFrameSize = 0;
}


void CrsfInput::update()
{
    if(!active)
    {
        return;
    }

    // A bound F1000 receiver can keep the UART non-empty continuously.
    // Never drain without a bound here: this function runs in the 250 Hz
    // control task and must return in time for IMU and servo processing.
    uint16_t processed = 0;

    while(
        serial.available() > 0 &&
        processed < MAX_BYTES_PER_UPDATE
    )
    {
        consumeByte(
            (uint8_t)serial.read()
        );

        processed++;
    }
}


bool CrsfInput::hasSignal(
    uint32_t timeoutMs
) const
{
    return
        channelsReady &&
        getFrameAgeMs() <= timeoutMs &&
        (!linkStatisticsReady || uplinkLinkQuality > 0);
}


bool CrsfInput::hasChannels() const
{
    return channelsReady;
}


uint16_t CrsfInput::getChannelRaw(
    uint8_t channel
) const
{
    if(channel >= CHANNEL_COUNT)
    {
        return 992;
    }

    return channels[channel];
}


uint16_t CrsfInput::getChannelMicroseconds(
    uint8_t channel
) const
{
    return channelToMicroseconds(
        getChannelRaw(channel)
    );
}


uint32_t CrsfInput::getFrameAgeMs() const
{
    if(lastChannelFrameMicros == 0)
    {
        return UINT32_MAX;
    }

    return
        (micros() - lastChannelFrameMicros)
        /
        1000;
}


uint32_t CrsfInput::getValidFrameCount() const
{
    return validFrameCount;
}


uint32_t CrsfInput::getReceivedByteCount() const
{
    return receivedByteCount;
}


uint32_t CrsfInput::getChannelFrameCount() const
{
    return channelFrameCount;
}


uint32_t CrsfInput::getCrcErrorCount() const
{
    return crcErrorCount;
}


uint8_t CrsfInput::getUplinkLinkQuality() const
{
    return uplinkLinkQuality;
}


int8_t CrsfInput::getUplinkSnr() const
{
    return uplinkSnr;
}


bool CrsfInput::popExtendedFrame(
    ExtendedFrame& result
)
{
    bool available = false;

    portENTER_CRITICAL(
        &extendedQueueMux
    );

    if(extendedQueueTail != extendedQueueHead)
    {
        result = extendedQueue[extendedQueueTail];
        extendedQueueTail =
            (extendedQueueTail + 1) % EXTENDED_QUEUE_SIZE;
        available = true;
    }

    portEXIT_CRITICAL(
        &extendedQueueMux
    );

    return available;
}


bool CrsfInput::sendExtendedFrame(
    uint8_t type,
    uint8_t destination,
    uint8_t origin,
    const uint8_t* payload,
    uint8_t payloadLength
)
{
    if(
        !active ||
        payloadLength > MAX_EXTENDED_PAYLOAD
    )
    {
        return false;
    }

    uint8_t output[MAX_FRAME_SIZE] = {0};
    uint8_t frameLength = payloadLength + 4;

    output[0] = FLIGHT_CONTROLLER_ADDRESS;
    output[1] = frameLength;
    output[2] = type;
    output[3] = destination;
    output[4] = origin;

    if(payloadLength > 0 && payload != nullptr)
    {
        memcpy(
            &output[5],
            payload,
            payloadLength
        );
    }

    output[5 + payloadLength] =
        crc8DvbS2(
            &output[2],
            payloadLength + 3
        );

    return
        serial.write(
            output,
            payloadLength + 6
        )
        ==
        payloadLength + 6;
}


void CrsfInput::consumeByte(
    uint8_t value
)
{
    receivedByteCount++;

    if(framePosition == 0)
    {
        // CRSF permits the serial sync byte, broadcast address, or a device
        // address in this position. Accept the addresses used along the
        // receiver/radio route; length and CRC still validate the frame.
        if(
            value != FLIGHT_CONTROLLER_ADDRESS &&
            value != 0x00 &&
            value != 0xEA &&
            value != 0xEC &&
            value != 0xEE
        )
        {
            return;
        }

        frame[framePosition++] = value;
        expectedFrameSize = 0;
        return;
    }

    if(framePosition == 1)
    {
        if(value < 2 || value > MAX_FRAME_SIZE - 2)
        {
            framePosition =
                (
                    value == FLIGHT_CONTROLLER_ADDRESS ||
                    value == 0x00 ||
                    value == 0xEA ||
                    value == 0xEC ||
                    value == 0xEE
                )
                ? 1
                : 0;

            if(framePosition == 1)
            {
                frame[0] = value;
            }

            expectedFrameSize = 0;
            return;
        }

        frame[framePosition++] = value;
        expectedFrameSize = value + 2;
        return;
    }

    if(framePosition >= MAX_FRAME_SIZE)
    {
        framePosition = 0;
        expectedFrameSize = 0;
        return;
    }

    frame[framePosition++] = value;

    if(
        expectedFrameSize > 0 &&
        framePosition >= expectedFrameSize
    )
    {
        processFrame();
        framePosition = 0;
        expectedFrameSize = 0;
    }
}


void CrsfInput::processFrame()
{
    if(expectedFrameSize < 4)
    {
        return;
    }

    uint8_t expectedCrc =
        frame[expectedFrameSize - 1];

    uint8_t calculatedCrc =
        crc8DvbS2(
            &frame[2],
            expectedFrameSize - 3
        );

    if(calculatedCrc != expectedCrc)
    {
        crcErrorCount++;
        return;
    }

    validFrameCount++;

    uint8_t type = frame[2];
    const uint8_t* payload = &frame[3];
    uint8_t payloadLength = expectedFrameSize - 4;

    if(type == TYPE_RC_CHANNELS_PACKED)
    {
        decodeChannels(
            payload,
            payloadLength
        );

        if(payloadLength == 22)
        {
            channelFrameCount++;
        }
    }
    else if(
        type == TYPE_LINK_STATISTICS &&
        payloadLength >= 4
    )
    {
        uplinkLinkQuality = payload[2];
        uplinkSnr = (int8_t)payload[3];
        linkStatisticsReady = true;
    }
    else if(
        type >= 0x28 &&
        payloadLength >= 2
    )
    {
        queueExtendedFrame(
            type,
            payload,
            payloadLength
        );
    }
}


void CrsfInput::queueExtendedFrame(
    uint8_t type,
    const uint8_t* payload,
    uint8_t payloadLength
)
{
    if(payloadLength < 2)
    {
        return;
    }

    ExtendedFrame next;
    next.type = type;
    next.destination = payload[0];
    next.origin = payload[1];
    next.payloadLength =
        min(
            (uint8_t)(payloadLength - 2),
            MAX_EXTENDED_PAYLOAD
        );

    if(next.payloadLength > 0)
    {
        memcpy(
            next.payload,
            &payload[2],
            next.payloadLength
        );
    }

    portENTER_CRITICAL(
        &extendedQueueMux
    );

    uint8_t nextHead =
        (extendedQueueHead + 1) % EXTENDED_QUEUE_SIZE;

    if(nextHead == extendedQueueTail)
    {
        extendedQueueTail =
            (extendedQueueTail + 1) % EXTENDED_QUEUE_SIZE;
    }

    extendedQueue[extendedQueueHead] = next;
    extendedQueueHead = nextHead;

    portEXIT_CRITICAL(
        &extendedQueueMux
    );
}


void CrsfInput::decodeChannels(
    const uint8_t* payload,
    uint8_t payloadLength
)
{
    if(payloadLength != 22)
    {
        return;
    }

    uint32_t bitBuffer = 0;
    uint8_t bitsAvailable = 0;
    uint8_t payloadPosition = 0;

    for(uint8_t channel = 0; channel < CHANNEL_COUNT; channel++)
    {
        while(bitsAvailable < 11)
        {
            bitBuffer |=
                ((uint32_t)payload[payloadPosition++])
                <<
                bitsAvailable;

            bitsAvailable += 8;
        }

        channels[channel] =
            bitBuffer & 0x07FF;

        bitBuffer >>= 11;
        bitsAvailable -= 11;
    }

    channelsReady = true;
    lastChannelFrameMicros = micros();
}


uint8_t CrsfInput::crc8DvbS2(
    const uint8_t* data,
    uint8_t length
)
{
    uint8_t crc = 0;

    for(uint8_t i = 0; i < length; i++)
    {
        crc ^= data[i];

        for(uint8_t bit = 0; bit < 8; bit++)
        {
            crc =
                crc & 0x80
                ?
                (uint8_t)((crc << 1) ^ 0xD5)
                :
                (uint8_t)(crc << 1);
        }
    }

    return crc;
}


uint16_t CrsfInput::channelToMicroseconds(
    uint16_t raw
)
{
    raw = constrain(
        raw,
        172,
        1811
    );

    return
        988
        +
        (
            ((uint32_t)(raw - 172) * 1024U + 819U)
            /
            1639U
        );
}
