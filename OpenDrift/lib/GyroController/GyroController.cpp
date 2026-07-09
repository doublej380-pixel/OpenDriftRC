#include "GyroController.h"



bool GyroController::begin()
{
    servoOutput = 1500;

    gyroOffset = 0;

    filteredYaw = 0;

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

    filteredYaw =
        (filteredYaw * 0.90f)
        +
        (correctedYaw * 0.10f);



    // Servo correction

    servoOutput =
        1500 -
        (filteredYaw * gyroGain);



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




float GyroController::getFilteredYaw()
{
    return filteredYaw;
}



int GyroController::getServoOutput()
{
    return servoOutput;
}