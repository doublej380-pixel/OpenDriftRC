#include "AuxChannelOutputs.h"

#include <driver/mcpwm.h>


namespace
{
    static constexpr uint32_t AUX_OUTPUT_HZ = 50;
    static constexpr uint16_t AUX_FAILSAFE_PULSE_US = 1500;

    mcpwm_unit_t unitForSlot(uint8_t slot)
    {
        return slot < 6
            ? MCPWM_UNIT_0
            : MCPWM_UNIT_1;
    }

    uint8_t localSlot(uint8_t slot)
    {
        return slot < 6
            ? slot
            : slot - 6;
    }

    mcpwm_timer_t timerForSlot(uint8_t slot)
    {
        return (mcpwm_timer_t)(localSlot(slot) / 2);
    }

    mcpwm_generator_t generatorForSlot(uint8_t slot)
    {
        return (localSlot(slot) % 2) == 0
            ? MCPWM_GEN_A
            : MCPWM_GEN_B;
    }

    mcpwm_io_signals_t signalForSlot(uint8_t slot)
    {
        return (mcpwm_io_signals_t)localSlot(slot);
    }
}


bool AuxChannelOutputs::begin(
    Settings& settings
)
{
    bool ready = true;

    for(uint8_t gpio = FIRST_GPIO; gpio <= LAST_GPIO; gpio++)
    {
        uint8_t channel =
            settings.getAuxChannelForGpio(gpio);

        if(
            channel == 0 ||
            !isPinAvailable(gpio)
        )
        {
            continue;
        }

        uint8_t slot = gpio - FIRST_GPIO;

        if(!attachOutput(slot))
        {
            ready = false;
        }
    }

    return ready;
}


void AuxChannelOutputs::update(
    Settings& settings,
    const CrsfInput& crsf,
    bool signalValid
)
{
    for(uint8_t gpio = FIRST_GPIO; gpio <= LAST_GPIO; gpio++)
    {
        uint8_t slot = gpio - FIRST_GPIO;
        uint8_t channel =
            settings.getAuxChannelForGpio(gpio);

        if(
            channel == 0 ||
            !isPinAvailable(gpio)
        )
        {
            if(attached[slot])
            {
                detachOutput(slot);
            }

            continue;
        }

        if(
            !attached[slot] &&
            !attachOutput(slot)
        )
        {
            continue;
        }

        uint16_t pulse =
            signalValid
            ? crsf.getChannelMicroseconds(channel - 1)
            : AUX_FAILSAFE_PULSE_US;

        writeOutput(
            slot,
            constrain(
                pulse,
                (uint16_t)988,
                (uint16_t)2012
            )
        );
    }
}


bool AuxChannelOutputs::isPinAvailable(
    uint8_t gpio
)
{
    if(gpio < FIRST_GPIO || gpio > LAST_GPIO)
    {
        return false;
    }

    #if defined(OPENDRIFT_AMOLED_V2)
    // V2 uses GPIO1/2 for the full-duplex CRSF UART because its onboard IMU
    // and touch interrupt lines occupy GPIO17/18.
    return gpio >= 3;
    #else
    return true;
    #endif
}


bool AuxChannelOutputs::attachOutput(
    uint8_t slot
)
{
    if(slot >= OUTPUT_COUNT)
    {
        return false;
    }

    uint8_t gpio = FIRST_GPIO + slot;

    if(!isPinAvailable(gpio))
    {
        return false;
    }

    mcpwm_unit_t unit = unitForSlot(slot);
    mcpwm_timer_t timer = timerForSlot(slot);
    mcpwm_generator_t generator = generatorForSlot(slot);

    if(!timerInitialized[(uint8_t)unit][(uint8_t)timer])
    {
        mcpwm_config_t config = {};
        config.frequency = AUX_OUTPUT_HZ;
        config.cmpr_a = 7.5f;
        config.cmpr_b = 7.5f;
        config.counter_mode = MCPWM_UP_COUNTER;
        config.duty_mode = MCPWM_DUTY_MODE_0;

        if(mcpwm_init(unit, timer, &config) != ESP_OK)
        {
            return false;
        }

        timerInitialized[(uint8_t)unit][(uint8_t)timer] = true;
    }

    if(
        mcpwm_gpio_init(
            unit,
            signalForSlot(slot),
            gpio
        ) != ESP_OK
    )
    {
        return false;
    }

    mcpwm_set_duty_type(
        unit,
        timer,
        generator,
        MCPWM_DUTY_MODE_0
    );

    if(
        mcpwm_set_duty_in_us(
            unit,
            timer,
            generator,
            AUX_FAILSAFE_PULSE_US
        ) != ESP_OK
    )
    {
        pinMode(gpio, INPUT_PULLDOWN);
        return false;
    }

    attached[slot] = true;
    lastPulse[slot] = AUX_FAILSAFE_PULSE_US;

    return true;
}


void AuxChannelOutputs::detachOutput(
    uint8_t slot
)
{
    if(slot >= OUTPUT_COUNT)
    {
        return;
    }

    mcpwm_set_signal_low(
        unitForSlot(slot),
        timerForSlot(slot),
        generatorForSlot(slot)
    );

    pinMode(
        FIRST_GPIO + slot,
        INPUT_PULLDOWN
    );

    attached[slot] = false;
    lastPulse[slot] = 0;
}


void AuxChannelOutputs::writeOutput(
    uint8_t slot,
    uint16_t pulse
)
{
    if(
        slot >= OUTPUT_COUNT ||
        !attached[slot] ||
        lastPulse[slot] == pulse
    )
    {
        return;
    }

    if(
        mcpwm_set_duty_in_us(
            unitForSlot(slot),
            timerForSlot(slot),
            generatorForSlot(slot),
            pulse
        ) == ESP_OK
    )
    {
        lastPulse[slot] = pulse;
    }
}
