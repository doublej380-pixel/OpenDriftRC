#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>


class ServoOutput
{
public:

    bool begin(int pin);

    void writeMicroseconds(int us);

    void center();

    int getPosition();


private:

    Servo servo;

    int currentPulse = 1500;
};