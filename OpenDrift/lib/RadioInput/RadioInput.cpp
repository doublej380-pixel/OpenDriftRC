#include "RadioInput.h"


bool RadioInput::begin(
    uint8_t inputPin
)
{
    pin =
        inputPin;

    pinMode(
        pin,
        INPUT_PULLDOWN
    );

    attachInterruptArg(
        digitalPinToInterrupt(pin),
        handleInterrupt,
        this,
        CHANGE
    );

    return true;
}



bool RadioInput::hasSignal(
    uint32_t timeoutMs
)
{
    uint16_t pulse =
        getPulseWidth();

    return(
        getSignalAgeMs() <= timeoutMs &&
        pulse >= 900 &&
        pulse <= 2100
    );
}



uint16_t RadioInput::getPulseWidth()
{
    noInterrupts();

    uint16_t pulse =
        pulseWidth;

    interrupts();

    return pulse;
}



uint32_t RadioInput::getSignalAgeMs()
{
    noInterrupts();

    uint32_t lastPulse =
        lastPulseMicros;

    interrupts();

    if(lastPulse == 0)
    {
        return UINT32_MAX;
    }

    return
        (micros() - lastPulse)
        /
        1000;
}



float RadioInput::getMappedValue(
    float minValue,
    float maxValue
)
{
    uint16_t pulse =
        constrain(
            getPulseWidth(),
            1000,
            2000
        );

    float normalized =
        (pulse - 1000)
        /
        1000.0f;

    return
        minValue +
        ((maxValue - minValue) * normalized);
}



void IRAM_ATTR RadioInput::handleInterrupt(
    void* arg
)
{
    RadioInput* input =
        static_cast<RadioInput*>(arg);

    if(input == nullptr)
    {
        return;
    }

    input->handlePinChange();
}



void IRAM_ATTR RadioInput::handlePinChange()
{
    uint32_t now =
        micros();

    if(digitalRead(pin) == HIGH)
    {
        riseTime =
            now;
    }
    else
    {
        uint32_t width =
            now - riseTime;

        if(
            width >= 800 &&
            width <= 2200
        )
        {
            pulseWidth =
                width;

            lastPulseMicros =
                now;
        }
    }
}
