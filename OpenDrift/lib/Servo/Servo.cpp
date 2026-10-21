#include "Servo.h"


bool ServoOutput::begin(
    int pin,
    int requestedFrequencyHz
)
{
    end();

    frequencyHz = constrain(
        requestedFrequencyHz,
        50,
        333
    );

    servo.setPeriodHertz(frequencyHz);

    int channel = servo.attach(
        pin,
        900,
        2100
    );

    if(channel == 0)
    {
        return false;
    }

    // ESP32Servo defaults to a 10-bit timer on ESP32-S3. At 333 Hz that
    // quantizes steering into roughly 2.9 us steps, which is visible at the
    // wheels. Use the S3's maximum supported width and feed it native ticks
    // so the float correction path remains sub-microsecond at the output.
    servo.setTimerWidth(14);

    ticksPerMicrosecond =
        ((float)(1UL << servo.readTimerWidth()) * frequencyHz)
        /
        1000000.0f;

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



float ServoOutput::computePulse(float us)
{
    us = constrain(us, 1000.0f, 2000.0f);

    float targetPulse = centerPulse;

    if(endpointCalibrationActive)
    {
        targetPulse = us <= 1500
            ? leftEndpointPulse
                + (us - 1000.0f) / 500.0f
                * (centerPulse - leftEndpointPulse)
            : centerPulse
                + (us - 1500.0f) / 500.0f
                * (rightEndpointPulse - centerPulse);
    }
    else
    {
        float correction = us - 1500.0f;

        if(reversed)
        {
            correction = -correction;
        }

        correction = (correction * travelPercent) / 100;
        targetPulse = centerPulse + correction;
    }

    return constrain(targetPulse, 900.0f, 2100.0f);
}


void ServoOutput::writePulse(float pulseUs)
{
    int ticks = (int)roundf(
        pulseUs * ticksPerMicrosecond
    );

    servo.writeTicks(ticks);
}


void ServoOutput::writeMicroseconds(float us)
{
    float targetPulse = computePulse(us);

    #if defined(OPENDRIFT_IMU_MPU6050)
    if(
        quietBand > 0 &&
        fabsf(targetPulse - currentPulse) <= quietBand
    )
    {
        return;
    }
    #endif

    currentPulse =
        targetPulse;

    writePulse(currentPulse);
}



void ServoOutput::center()
{
    currentPulse =
        constrain(
            centerPulse,
            900,
            2100
        );

    writePulse(currentPulse);
}



int ServoOutput::getPosition()
{
    return (int)roundf(currentPulse);
}


void ServoOutput::noteCommandPulse(int us)
{
    commandPulse = computePulse(us);
}


int ServoOutput::getCommandPosition()
{
    return (int)roundf(commandPulse);
}



void ServoOutput::configure(
    int centerPulseValue,
    bool reversedValue,
    int travelPercentValue,
    int quietBandValue,
    bool calibratedEndpointsActive,
    int leftEndpointValue,
    int calibratedCenterValue,
    int rightEndpointValue
)
{
    int candidateCenter = constrain(calibratedCenterValue, 900, 2100);
    int leftDelta = leftEndpointValue - candidateCenter;
    int rightDelta = rightEndpointValue - candidateCenter;

    endpointCalibrationActive =
        calibratedEndpointsActive &&
        abs(leftDelta) >= 10 &&
        abs(rightDelta) >= 10 &&
        leftDelta * rightDelta < 0;

    centerPulse = endpointCalibrationActive
        ? candidateCenter
        : constrain(centerPulseValue, 1000, 2000);

    leftEndpointPulse = constrain(leftEndpointValue, 900, 2100);
    rightEndpointPulse = constrain(rightEndpointValue, 900, 2100);

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
