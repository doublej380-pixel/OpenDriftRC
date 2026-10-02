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

    void writeMicroseconds(float us);

    void center();

    int getPosition();
    float getPulseMicroseconds() const { return currentPulse; }

    void noteCommandPulse(int us);
    int getCommandPosition();

    void configure(
        int centerPulse,
        bool reversed,
        int travelPercent,
        int quietBand,
        bool endpointCalibrationActive = false,
        int leftEndpointPulse = 1000,
        int calibratedCenterPulse = 1500,
        int rightEndpointPulse = 2000
    );


private:

    float computePulse(float us);
    void writePulse(float pulseUs);

    Servo servo;

    int frequencyHz = 50;

    float ticksPerMicrosecond = 1.0f;

    float currentPulse = 1500.0f;

    float commandPulse = 1500.0f;

    int centerPulse = 1500;

    bool reversed = false;

    int travelPercent = 100;

    int quietBand = 0;

    bool endpointCalibrationActive = false;

    int leftEndpointPulse = 1000;

    int rightEndpointPulse = 2000;

    bool active = false;

};
