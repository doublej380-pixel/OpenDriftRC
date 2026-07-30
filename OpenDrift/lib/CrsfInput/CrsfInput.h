#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>


class CrsfInput
{
public:

    static constexpr uint8_t CHANNEL_COUNT = 16;
    static constexpr uint8_t MAX_EXTENDED_PAYLOAD = 58;

    struct ExtendedFrame
    {
        uint8_t type = 0;
        uint8_t destination = 0;
        uint8_t origin = 0;
        uint8_t payloadLength = 0;
        uint8_t payload[MAX_EXTENDED_PAYLOAD] = {0};
    };

    bool begin(
        int8_t rxPin,
        int8_t txPin
    );

    void end();

    void update();

    bool hasSignal(
        uint32_t timeoutMs = 50
    ) const;

    bool hasChannels() const;

    uint16_t getChannelRaw(
        uint8_t channel
    ) const;

    uint16_t getChannelMicroseconds(
        uint8_t channel
    ) const;

    uint32_t getFrameAgeMs() const;

    uint32_t getReceivedByteCount() const;
    uint32_t getValidFrameCount() const;
    uint32_t getChannelFrameCount() const;
    uint32_t getCrcErrorCount() const;

    uint8_t getUplinkLinkQuality() const;
    int8_t getUplinkSnr() const;

    bool popExtendedFrame(
        ExtendedFrame& result
    );

    bool sendExtendedFrame(
        uint8_t type,
        uint8_t destination,
        uint8_t origin,
        const uint8_t* payload,
        uint8_t payloadLength
    );


private:

    static constexpr uint32_t BAUD_RATE = 420000;
    static constexpr uint8_t MAX_FRAME_SIZE = 64;
    static constexpr uint16_t MAX_BYTES_PER_UPDATE = 192;
    static constexpr uint8_t FLIGHT_CONTROLLER_ADDRESS = 0xC8;
    static constexpr uint8_t TYPE_LINK_STATISTICS = 0x14;
    static constexpr uint8_t TYPE_RC_CHANNELS_PACKED = 0x16;

    HardwareSerial serial = HardwareSerial(1);

    uint8_t frame[MAX_FRAME_SIZE] = {0};
    uint8_t framePosition = 0;
    uint8_t expectedFrameSize = 0;

    volatile uint16_t channels[CHANNEL_COUNT] = {0};

    volatile uint32_t lastChannelFrameMicros = 0;
    volatile uint32_t receivedByteCount = 0;
    volatile uint32_t validFrameCount = 0;
    volatile uint32_t channelFrameCount = 0;
    volatile uint32_t crcErrorCount = 0;

    volatile uint8_t uplinkLinkQuality = 0;
    volatile int8_t uplinkSnr = 0;
    volatile bool linkStatisticsReady = false;

    static constexpr uint8_t EXTENDED_QUEUE_SIZE = 8;
    ExtendedFrame extendedQueue[EXTENDED_QUEUE_SIZE];
    volatile uint8_t extendedQueueHead = 0;
    volatile uint8_t extendedQueueTail = 0;
    portMUX_TYPE extendedQueueMux = portMUX_INITIALIZER_UNLOCKED;

    bool active = false;
    volatile bool channelsReady = false;

    void consumeByte(
        uint8_t value
    );

    void processFrame();

    void queueExtendedFrame(
        uint8_t type,
        const uint8_t* payload,
        uint8_t payloadLength
    );

    void decodeChannels(
        const uint8_t* payload,
        uint8_t payloadLength
    );

    static uint8_t crc8DvbS2(
        const uint8_t* data,
        uint8_t length
    );

    static uint16_t channelToMicroseconds(
        uint16_t raw
    );
};
