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
    currentPulse = us;

    servo.writeMicroseconds(us);
}



void ServoOutput::center()
{
    writeMicroseconds(1500);
}



int ServoOutput::getPosition()
{
    return currentPulse;
}