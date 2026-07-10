#include "GyroController.h"



bool GyroController::begin()
{
    servoOutput = 1500;

    gyroOffset = 0;

    filteredYaw = 0;

    correctionOutput = 0;

    calibrated = false;


    return true;
}




void GyroController::calibrate(float yawRate)
{
    gyroOffset = yawRate;

    calibrated = true;
}




int GyroController::update(float yawRate)
{
    if(!calibrated)
    {
        calibrate(yawRate);
    }



    // Remove gyro bias

    float correctedYaw =
        yawRate - gyroOffset;



    // Deadband

    if(abs(correctedYaw) < deadband)
    {
        correctedYaw = 0;
    }



    // Low pass filter

    float filterAmount =
        constrain(
            smoothing,
            0.01f,
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
            500
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
