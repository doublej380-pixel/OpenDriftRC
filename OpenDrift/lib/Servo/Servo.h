#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>


class ServoOutput
{
public:

    bool begin(
        int pin,
        int frequencyHz = 50
    );

    void end();

    void writeMicroseconds(int us);

    void center();

    int getPosition();

    void configure(
        int centerPulse,
        bool reversed,
        int travelPercent,
        int quietBand
    );


private:

    Servo servo;

    int currentPulse = 1500;

    int centerPulse = 1500;

    bool reversed = false;

    int travelPercent = 100;

    int quietBand = 0;

    bool active = false;
};
