#pragma once

#include <Arduino.h>


class RadioInput
{
public:

    bool begin(
        uint8_t inputPin
    );

    void end();

    bool hasSignal(
        uint32_t timeoutMs = 250
    );

    uint16_t getPulseWidth();

    uint32_t getSignalAgeMs();

    float getMappedValue(
        float minValue,
        float maxValue
    );


private:

    uint8_t pin = 0;

    volatile uint32_t riseTime = 0;

    volatile uint16_t pulseWidth = 1500;

    volatile uint32_t lastPulseMicros = 0;

    bool active = false;

    static void IRAM_ATTR handleInterrupt(
        void* arg
    );

    void IRAM_ATTR handlePinChange();
};
