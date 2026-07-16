#include "Servo.h"


bool ServoOutput::begin(int pin)
{
    end();

    servo.setPeriodHertz(50);

    int channel =
        servo.attach(
        pin,
        1000,
        2000
    );

    if(channel == 0)
    {
        return false;
    }

    active = true;


    center();


    Serial.print("Servo attached GPIO ");
    Serial.println(pin);


    return true;
}



void ServoOutput::end()
{
    if(!active)
    {
        return;
    }

    servo.detach();
    active = false;
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

    int targetPulse =
        constrain(
            centerPulse + correction,
            1000,
            2000
        );

    if(
        quietBand > 0 &&
        abs(targetPulse - currentPulse) <= quietBand
    )
    {
        return;
    }

    currentPulse =
        targetPulse;

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
    int travelPercentValue,
    int quietBandValue
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

    quietBand =
        constrain(
            quietBandValue,
            0,
            50
        );
}
