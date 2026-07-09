#include "GyroController.h"

bool GyroController::begin()
{
    servoOutput = 1500;
    return true;
}

int GyroController::update(float yawRate)
{
    servoOutput = 1500 - (yawRate * gyroGain);

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

int GyroController::getServoOutput()
{
    return servoOutput;
}