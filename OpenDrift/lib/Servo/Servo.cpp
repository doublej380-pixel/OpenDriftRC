#include "Servo.h"


bool ServoOutput::begin(int pin)
{
    servo.setPeriodHertz(50);

    servo.attach(
        pin,
        1000,
        2000
    );


    center();


    Serial.print("Servo attached GPIO ");
    Serial.println(pin);


    return true;
}



void ServoOutput::writeMicroseconds(int us)
{
    int correction =
        us - 1500;

    if(reversed)
    {
        correction =
            -correction;
    }

    correction =
        (correction * travelPercent)
        /
        100;

    currentPulse =
        constrain(
            centerPulse + correction,
            1000,
            2000
        );

    servo.writeMicroseconds(
        currentPulse
    );
}



void ServoOutput::center()
{
    currentPulse =
        constrain(
            centerPulse,
            1000,
            2000
        );

    servo.writeMicroseconds(
        currentPulse
    );
}



int ServoOutput::getPosition()
{
    return currentPulse;
}



void ServoOutput::configure(
    int centerPulseValue,
    bool reversedValue,
    int travelPercentValue
)
{
    centerPulse =
        constrain(
            centerPulseValue,
            1000,
            2000
        );

    reversed =
        reversedValue;

    travelPercent =
        constrain(
            travelPercentValue,
            1,
            100
        );

    if(travelPercentValue <= 0)
    {
        travelPercent = 100;
    }
}
