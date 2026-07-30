#pragma once

#include <Arduino.h>


// ESC pulses deliberately use LEDC instead of ESP32Servo. On ESP32-S3 the
// current ESP32Servo MCPWM allocator can route a second frequency through the
// first timer's signal, causing the throttle pin to mirror steering output.
class EscOutput
{
public:

    bool begin(
        int outputPin,
        int frequencyHz = 50
    );

    void end();

    void writeMicroseconds(
        int pulseUs
    );

    void configure(
        int centerPulse,
        bool reversed,
        int travelPercent,
        int quietBand
    );

    bool isActive() const;


private:

    static constexpr uint8_t LEDC_CHANNEL = 7;
    // ESP32-S3 LEDC supports at most 14-bit duty resolution in this Arduino
    // core. At 50 Hz this still resolves an ESC pulse to about 1.22 us.
    static constexpr uint8_t LEDC_RESOLUTION_BITS = 14;

    int pin = -1;
    int frequency = 50;
    int currentPulse = 1500;
    int center = 1500;
    bool reversed = false;
    int travel = 100;
    int quiet = 0;
    bool active = false;
};
