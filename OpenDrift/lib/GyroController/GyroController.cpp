#include "GyroController.h"

#include <math.h>



bool GyroController::begin()
{
    servoOutput = 1500;

    gyroOffset = 0;

    filteredYaw = 0;

    integralAccumulator = 0;

    integralCorrection = 0;

    holdBoostFiltered = 0;

    correctionOutput = 0;

    lastCorrectionMove = 0;

    calibrated = false;

    lastUpdateMicros = 0;


    return true;
}




void GyroController::calibrate(float yawRate)
{
    gyroOffset = yawRate;

    calibrated = true;

    filteredYaw = 0;

    integralAccumulator = 0;

    integralCorrection = 0;

    holdBoostFiltered = 0;

    lastCorrectionMove = 0;

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

    float oldFilteredYaw =
        filteredYaw;

    filteredYaw =
        (filteredYaw * (1.0f - filterAmount))
        +
        (correctedYaw * filterAmount);



    // Servo correction

    float yawAbs =
        fabsf(filteredYaw);

    float oldYawAbs =
        fabsf(oldFilteredYaw);

    float yawBuildRate =
        (yawAbs - oldYawAbs)
        /
        dt;

    float holdBoostFactor =
        1.0f;

    if(holdBoost > 0)
    {
        float holdAmount =
            constrain(
                (yawAbs - 45.0f) / 85.0f,
                0.0f,
                1.0f
            );

        if(yawBuildRate > 150.0f)
        {
            holdAmount = 0.0f;
        }
        else if(yawBuildRate > 50.0f)
        {
            holdAmount *=
                1.0f -
                (
                    (yawBuildRate - 50.0f)
                    /
                    100.0f
                );
        }

        float holdFilterAmount =
            1.0f -
            powf(
                0.82f,
                dt / 0.02f
            );

        holdFilterAmount =
            constrain(
                holdFilterAmount,
                0.001f,
                1.0f
            );

        holdBoostFiltered =
            (holdBoostFiltered * (1.0f - holdFilterAmount))
            +
            (holdAmount * holdFilterAmount);

        holdBoostFactor +=
            holdBoostFiltered
            *
            (
                holdBoost
                /
                100.0f
            );
    }
    else
    {
        holdBoostFiltered = 0;
    }

    int proportionalCorrection =
        (int)(filteredYaw * gyroGain * holdBoostFactor);

    if(
        integralGain > 0.0f &&
        integralLimit > 0
    )
    {
        float integralDecayBase =
            0.998f;

        bool yawCrossedZero =
            oldFilteredYaw * filteredYaw < 0.0f;

        bool integralOpposesYaw =
            integralAccumulator * filteredYaw < 0.0f;

        if(fabsf(filteredYaw) < 8.0f)
        {
            integralDecayBase =
                0.92f;
        }

        if(yawCrossedZero)
        {
            integralDecayBase =
                min(
                    integralDecayBase,
                    0.75f
                );
        }

        if(integralOpposesYaw)
        {
            integralDecayBase =
                min(
                    integralDecayBase,
                    0.85f
                );
        }

        float decay =
            powf(
                integralDecayBase,
                dt / 0.02f
            );

        integralAccumulator *=
            decay;

        integralAccumulator +=
            filteredYaw
            *
            dt;

        float maxAccumulator =
            integralLimit
            /
            integralGain;

        integralAccumulator =
            constrain(
                integralAccumulator,
                -maxAccumulator,
                maxAccumulator
            );

        integralCorrection =
            constrain(
                (int)(integralAccumulator * integralGain),
                -integralLimit,
                integralLimit
            );
    }
    else
    {
        integralAccumulator = 0;

        integralCorrection = 0;
    }

    int targetCorrection =
        constrain(
            proportionalCorrection + integralCorrection,
            -maxCorrection,
            maxCorrection
        );

    int previousCorrectionOutput =
        correctionOutput;

    int correctionDelta =
        targetCorrection - correctionOutput;

    int absCorrectionDelta =
        abs(correctionDelta);

    float antiWobbleStrength =
        antiWobble
        /
        50.0f;

    if(
        antiWobbleStrength > 0.0f
    )
    {
        bool correctionReversal =
            correctionDelta != 0 &&
            lastCorrectionMove != 0 &&
            (
                correctionDelta > 0
                ?
                lastCorrectionMove < 0
                :
                lastCorrectionMove > 0
            );

        if(
            correctionReversal &&
            absCorrectionDelta < 140
        )
        {
            int baseReversalLimit =
                yawAbs > 25.0f
                ?
                18
                :
                10;

            int reversalLimit =
                constrain(
                    (int)roundf(
                        baseReversalLimit
                        /
                        antiWobbleStrength
                    ),
                    4,
                    140
                );

            targetCorrection =
                correctionOutput
                +
                constrain(
                    correctionDelta,
                    -reversalLimit,
                    reversalLimit
                );

            correctionDelta =
                targetCorrection - correctionOutput;

            absCorrectionDelta =
                abs(correctionDelta);
        }

        if(
            yawAbs > 40.0f &&
            yawBuildRate < 120.0f &&
            absCorrectionDelta <= 90
        )
        {
            float driftBlend =
                absCorrectionDelta > 45
                ?
                0.55f
                :
                0.35f;

            if(antiWobbleStrength < 1.0f)
            {
                driftBlend =
                    1.0f -
                    (
                        (1.0f - driftBlend)
                        *
                        antiWobbleStrength
                    );
            }
            else
            {
                driftBlend =
                    driftBlend
                    /
                    antiWobbleStrength;
            }

            targetCorrection =
                correctionOutput
                +
                (int)roundf(
                    correctionDelta
                    *
                    driftBlend
                );
        }
        else if(
            absCorrectionDelta <=
            (int)roundf(3.0f * antiWobbleStrength)
        )
        {
            targetCorrection =
                correctionOutput;
        }
        else if(
            absCorrectionDelta <=
            (int)roundf(14.0f * antiWobbleStrength)
        )
        {
            float smallBlend =
                0.45f;

            if(antiWobbleStrength < 1.0f)
            {
                smallBlend =
                    1.0f -
                    (
                        (1.0f - smallBlend)
                        *
                        antiWobbleStrength
                    );
            }
            else
            {
                smallBlend =
                    smallBlend
                    /
                    antiWobbleStrength;
            }

            targetCorrection =
                correctionOutput
                +
                (int)roundf(
                    correctionDelta
                    *
                    smallBlend
                );
        }
    }

    correctionOutput =
        constrain(
            targetCorrection,
            -maxCorrection,
            maxCorrection
        );

    lastCorrectionMove =
        correctionOutput - previousCorrectionOutput;

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


void GyroController::setIntegralGain(float value)
{
    integralGain =
        constrain(
            value,
            0.0f,
            20.0f
        );

    if(integralGain <= 0.0f)
    {
        integralAccumulator = 0;

        integralCorrection = 0;
    }
}



float GyroController::getIntegralGain()
{
    return integralGain;
}



void GyroController::setIntegralLimit(int value)
{
    integralLimit =
        constrain(
            value,
            0,
            500
        );

    if(integralLimit <= 0)
    {
        integralAccumulator = 0;

        integralCorrection = 0;
    }
}



int GyroController::getIntegralLimit()
{
    return integralLimit;
}



int GyroController::getIntegralCorrection()
{
    return integralCorrection;
}


void GyroController::setHoldBoost(int value)
{
    holdBoost =
        constrain(
            value,
            0,
            100
        );
}



int GyroController::getHoldBoost()
{
    return holdBoost;
}


void GyroController::setAntiWobble(int value)
{
    antiWobble =
        constrain(
            value,
            0,
            100
        );
}



int GyroController::getAntiWobble()
{
    return antiWobble;
}




float GyroController::getFilteredYaw()
{
    return filteredYaw;
}



int GyroController::getServoOutput()
{
    return servoOutput;
}
