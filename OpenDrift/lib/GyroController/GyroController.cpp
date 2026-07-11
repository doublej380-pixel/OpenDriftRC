#include "GyroController.h"

#include <math.h>



bool GyroController::begin()
{
    servoOutput = 1500;

    gyroOffset = 0;

    filteredYaw = 0;

    correctionOutput = 0;

    calibrated = false;

    lastUpdateMicros = 0;


    return true;
}




void GyroController::calibrate(float yawRate)
{
    gyroOffset = yawRate;

    calibrated = true;

    filteredYaw = 0;

    lastUpdateMicros = micros();
}




int GyroController::update(float yawRate)
{
    uint32_t now =
        micros();

    float dt =
        0.02f;

    if(lastUpdateMicros != 0)
    {
        dt =
            (now - lastUpdateMicros)
            /
            1000000.0f;

        dt =
            constrain(
                dt,
                0.001f,
                0.05f
            );
    }

    lastUpdateMicros =
        now;

    if(!calibrated)
    {
        calibrate(yawRate);
    }



    // Remove gyro bias

    float correctedYaw =
        yawRate - gyroOffset;



    // Soft deadband removes center noise without a correction jump.

    float yawMagnitude =
        fabsf(correctedYaw);

    if(yawMagnitude <= deadband)
    {
        correctedYaw = 0;
    }
    else
    {
        correctedYaw =
            (
                correctedYaw > 0
                ?
                1.0f
                :
                -1.0f
            )
            *
            (yawMagnitude - deadband);
    }



    // Time-based low pass filter. The setting is referenced to 20 ms so
    // existing tune numbers stay close while loop timing becomes stable.

    float baseFilterAmount =
        1.0f -
        constrain(
            smoothing,
            0.01f,
            0.99f
        );

    float filterAmount =
        1.0f -
        powf(
            1.0f - baseFilterAmount,
            dt / 0.02f
        );

    filterAmount =
        constrain(
            filterAmount,
            0.001f,
            1.0f
        );

    filteredYaw =
        (filteredYaw * (1.0f - filterAmount))
        +
        (correctedYaw * filterAmount);



    // Servo correction

    correctionOutput =
        constrain(
            (int)(filteredYaw * gyroGain),
            -maxCorrection,
            maxCorrection
        );

    servoOutput =
        1500 -
        correctionOutput;



    servoOutput = constrain(
        servoOutput,
        1000,
        2000
    );


    return servoOutput;
}




void GyroController::setGain(float gain)
{
    gyroGain = gain;
}



float GyroController::getGain()
{
    return gyroGain;
}




void GyroController::setDeadband(float value)
{
    deadband = value;
}



float GyroController::getDeadband()
{
    return deadband;
}




void GyroController::setSmoothing(float value)
{
    smoothing =
        constrain(
            value,
            0.01f,
            1.0f
        );
}



float GyroController::getSmoothing()
{
    return smoothing;
}



void GyroController::setMaxCorrection(int value)
{
    maxCorrection =
        constrain(
            value,
            0,
            1000
        );
}



int GyroController::getMaxCorrection()
{
    return maxCorrection;
}



int GyroController::getCorrection()
{
    return correctionOutput;
}




float GyroController::getFilteredYaw()
{
    return filteredYaw;
}



int GyroController::getServoOutput()
{
    return servoOutput;
}
