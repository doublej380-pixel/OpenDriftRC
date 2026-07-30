#include "GyroController.h"

#include <math.h>


namespace
{
    static constexpr uint8_t PHASE_IDLE = 0;
    static constexpr uint8_t PHASE_ENTRY = 1;
    static constexpr uint8_t PHASE_SETTLED = 2;
    static constexpr uint8_t PHASE_TRANSITION = 3;

    static constexpr float TRANSITION_SECONDS = 0.18f;
    static constexpr float THROTTLE_TRANSIENT_SECONDS = 0.22f;
    static constexpr float MEMORY_GAIN_SCALE = 6.0f;
}


void GyroController::resetDynamicState()
{
    filteredYaw = 0.0f;
    previousFilteredYaw = 0.0f;
    filteredYawAcceleration = 0.0f;

    driftReferenceYaw = 0.0f;
    driftReferenceReady = false;
    driftDirection = 0;
    transitionTime = 0.0f;

    integralAccumulator = 0.0f;
    integralCorrection = 0;
    counterSteerCorrection = 0;

    steeringActivity = 0.0f;
    tailSlideBlend = 0.0f;
    lastSteeringCommand = 1500;
    steeringReady = false;

    throttleRate = 0.0f;
    throttleTransientTime = 0.0f;
    lastThrottlePulse = 1500;
    throttleReady = false;

    controlPhase = PHASE_IDLE;
    settledBlend = 0.0f;

    huntControlYaw = 0.0f;
    huntSlowYaw = 0.0f;
    huntFastYaw = 0.0f;
    huntBlend = 0.0f;
    huntScore = 0.0f;

    outputChatterSlow = 0.0f;
    outputChatterFast = 0.0f;
    outputChatterBlend = 0.0f;
    outputChatterScore = 0.0f;

    terrainAssistBlend = 0.0f;
    activeHoldFactor = 1.0f;

    servoOutput = 1500;
    correctionOutput = 0;
}


bool GyroController::begin()
{
    gyroOffset = 0.0f;
    calibrated = false;
    lastUpdateMicros = 0;

    resetDynamicState();

    return true;
}


void GyroController::calibrate(float yawRate)
{
    gyroOffset = yawRate;
    calibrated = true;

    resetDynamicState();

    lastUpdateMicros = micros();
}


int GyroController::update(
    float yawRate,
    int steeringCommand,
    bool steeringSignal,
    int throttlePulse,
    bool throttleSignal,
    float surfaceDisturbance,
    bool terrainAssistEnabled
)
{
    (void)surfaceDisturbance;
    (void)terrainAssistEnabled;

    uint32_t now = micros();
    float dt = 0.004f;

    if(lastUpdateMicros != 0)
    {
        dt =
            (now - lastUpdateMicros)
            /
            1000000.0f;

        dt = constrain(
            dt,
            0.001f,
            0.05f
        );
    }

    lastUpdateMicros = now;

    if(!calibrated)
    {
        calibrate(yawRate);
    }

    steeringCommand = constrain(
        steeringCommand,
        1000,
        2000
    );

    if(!steeringSignal)
    {
        steeringReady = false;
        steeringActivity = 0.0f;
        lastSteeringCommand = 1500;
    }
    else if(!steeringReady)
    {
        lastSteeringCommand = steeringCommand;
        steeringReady = true;
    }
    else
    {
        float steeringRate =
            fabsf(
                steeringCommand - lastSteeringCommand
            )
            /
            dt;

        float steeringAmount =
            1.0f - expf(-dt / 0.04f);

        steeringActivity +=
            (steeringRate - steeringActivity)
            *
            steeringAmount;

        lastSteeringCommand = steeringCommand;
    }

    float driverActivityBlend = constrain(
        (steeringActivity - 80.0f) / 1800.0f,
        0.0f,
        1.0f
    );

    float throttleLevel = 0.0f;

    if(throttleSignal)
    {
        throttlePulse = constrain(
            throttlePulse,
            900,
            2100
        );

        throttleLevel = constrain(
            fabsf(throttlePulse - 1500.0f) / 500.0f,
            0.0f,
            1.0f
        );

        if(!throttleReady)
        {
            lastThrottlePulse = throttlePulse;
            throttleReady = true;
        }
        else
        {
            int throttleDelta =
                throttlePulse - lastThrottlePulse;

            float rawThrottleRate =
                throttleDelta
                /
                dt;

            float throttleAmount =
                1.0f - expf(-dt / 0.04f);

            throttleRate +=
                (rawThrottleRate - throttleRate)
                *
                throttleAmount;

            if(abs(throttleDelta) >= 8)
            {
                throttleTransientTime =
                    THROTTLE_TRANSIENT_SECONDS;
            }

            lastThrottlePulse = throttlePulse;
        }
    }
    else
    {
        throttleReady = false;
        throttleRate = 0.0f;
        throttleTransientTime = 0.0f;
    }

    throttleTransientTime = max(
        0.0f,
        throttleTransientTime - dt
    );

    float throttleRateBlend = constrain(
        (fabsf(throttleRate) - 250.0f) / 3000.0f,
        0.0f,
        1.0f
    );

    float throttleLatchBlend = constrain(
        throttleTransientTime
        /
        THROTTLE_TRANSIENT_SECONDS,
        0.0f,
        1.0f
    );

    float throttlePredictionBlend = max(
        throttleRateBlend,
        throttleLatchBlend * 0.55f
    );

    float correctedYaw =
        yawRate - gyroOffset;

    float yawMagnitude =
        fabsf(correctedYaw);

    if(yawMagnitude <= deadband)
    {
        correctedYaw = 0.0f;
    }
    else
    {
        correctedYaw =
            (correctedYaw > 0.0f ? 1.0f : -1.0f)
            *
            (yawMagnitude - deadband);
    }

    // One time-based gyro low-pass is the entire V2 filtering chain.
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

    filterAmount = constrain(
        filterAmount,
        0.001f,
        1.0f
    );

    previousFilteredYaw = filteredYaw;

    filteredYaw +=
        (correctedYaw - filteredYaw)
        *
        filterAmount;

    float rawYawAcceleration =
        (filteredYaw - previousFilteredYaw)
        /
        dt;

    rawYawAcceleration = constrain(
        rawYawAcceleration,
        -4000.0f,
        4000.0f
    );

    float accelerationAmount =
        1.0f - expf(-dt / 0.035f);

    filteredYawAcceleration +=
        (rawYawAcceleration - filteredYawAcceleration)
        *
        accelerationAmount;

    // Throttle does not pretend to be vehicle speed or prescribe a turn
    // direction. It announces an upcoming chassis-load change, extending the
    // short yaw-acceleration look-ahead before the resulting motion arrives.
    float predictionSeconds =
        0.003f
        +
        (
            huntDamping
            /
            100.0f
        )
        *
        0.024f
        +
        throttlePredictionBlend
        *
        0.012f
        +
        throttleLevel
        *
        0.003f;

    float predictionDelta = constrain(
        filteredYawAcceleration
        *
        predictionSeconds,
        -45.0f,
        45.0f
    );

    float predictedYaw =
        filteredYaw + predictionDelta;

    if(
        fabsf(filteredYaw) > 5.0f &&
        predictedYaw * filteredYaw < 0.0f
    )
    {
        predictedYaw = 0.0f;
    }

    float yawAbs = fabsf(filteredYaw);

    int8_t definiteDirection =
        filteredYaw > 12.0f
        ?
        1
        :
        (
            filteredYaw < -12.0f
            ?
            -1
            :
            0
        );

    bool directionChanged =
        definiteDirection != 0 &&
        driftDirection != 0 &&
        definiteDirection != driftDirection;

    if(directionChanged)
    {
        transitionTime = TRANSITION_SECONDS;
        driftReferenceReady = false;
        integralCorrection = 0;
    }

    if(definiteDirection != 0)
    {
        driftDirection = definiteDirection;
    }

    transitionTime = max(
        0.0f,
        transitionTime - dt
    );

    bool idle =
        yawAbs < 7.0f;

    float quietBlend =
        (1.0f - driverActivityBlend)
        *
        (1.0f - throttlePredictionBlend);

    bool driftActive =
        !idle &&
        definiteDirection != 0;

    float settledTarget =
        driftActive &&
        transitionTime <= 0.0f
        ?
        quietBlend
        :
        0.0f;

    float settledTimeConstant =
        settledTarget > settledBlend
        ?
        0.20f
        :
        0.06f;

    float settledAmount =
        1.0f - expf(-dt / settledTimeConstant);

    settledBlend +=
        (settledTarget - settledBlend)
        *
        settledAmount;

    settledBlend = constrain(
        settledBlend,
        0.0f,
        1.0f
    );

    if(idle)
    {
        controlPhase = PHASE_IDLE;
        driftDirection = 0;
        driftReferenceReady = false;
        driftReferenceYaw = 0.0f;
        integralAccumulator = 0.0f;
        integralCorrection = 0;
    }
    else if(transitionTime > 0.0f)
    {
        controlPhase = PHASE_TRANSITION;
    }
    else if(settledBlend > 0.55f)
    {
        controlPhase = PHASE_SETTLED;
    }
    else
    {
        controlPhase = PHASE_ENTRY;
    }

    if(driftActive)
    {
        if(!driftReferenceReady)
        {
            driftReferenceYaw = filteredYaw;
            driftReferenceReady = true;
        }
        else
        {
            float holdStrength =
                holdBoost
                /
                100.0f;

            float referenceTimeConstant =
                0.045f
                +
                quietBlend
                *
                (
                    0.18f
                    +
                    2.20f
                    *
                    holdStrength
                );

            float referenceAmount =
                1.0f -
                expf(
                    -dt
                    /
                    referenceTimeConstant
                );

            driftReferenceYaw +=
                (filteredYaw - driftReferenceYaw)
                *
                referenceAmount;
        }
    }

    float referenceError =
        driftReferenceReady
        ?
        filteredYaw - driftReferenceYaw
        :
        0.0f;

    integralAccumulator = referenceError;

    float memoryCorrection =
        referenceError
        *
        integralGain
        *
        MEMORY_GAIN_SCALE
        *
        settledBlend;

    integralCorrection =
        constrain(
            (int)roundf(memoryCorrection),
            -integralLimit,
            integralLimit
        );

    // Experimental Tail Slide Speed is centered at 50. It changes only the
    // fast damping path while deliberate steering movement is present:
    // lower values add damping, 50 preserves the proven RC1 response, and
    // higher values release damping. The 40% floor at 100 means it cannot
    // reverse or remove gyro correction, and settled drifts receive a
    // substantially smaller change in either direction.
    tailSlideBlend =
        ((tailSlideSpeed - 50) / 50.0f)
        *
        driverActivityBlend
        *
        (1.0f - 0.65f * settledBlend);

    tailSlideBlend = constrain(
        tailSlideBlend,
        -1.0f,
        1.0f
    );

    float directDampingScale =
        1.0f - 0.60f * tailSlideBlend;

    float directCorrection =
        predictedYaw
        *
        gyroGain
        *
        directDampingScale;

    // Countersteer Assist is deliberately sourced from the slow learned
    // drift reference. It increases how much of a settled drift OpenDrift
    // carries without raising fast yaw damping or responding to chatter.
    float steadyAssistCorrection =
        driftReferenceReady
        ?
        driftReferenceYaw
        *
        gyroGain
        *
        (counterSteerAssist / 100.0f)
        *
        settledBlend
        :
        0.0f;

    counterSteerCorrection =
        (int)roundf(steadyAssistCorrection);

    float baseCorrection =
        directCorrection
        +
        steadyAssistCorrection;

    // There is no accumulating state to wind up. When direct damping has
    // saturated, memory may help it unwind but may not push farther into the
    // same limit.
    if(
        fabsf(baseCorrection) >= maxCorrection &&
        integralCorrection * baseCorrection > 0.0f
    )
    {
        integralCorrection = 0;
    }

    int targetCorrection =
        constrain(
            (int)roundf(
                baseCorrection
                +
                integralCorrection
            ),
            -maxCorrection,
            maxCorrection
        );

    if(idle && correctedYaw == 0.0f)
    {
        targetCorrection = 0;
        filteredYawAcceleration = 0.0f;
    }

    correctionOutput = targetCorrection;

    servoOutput = constrain(
        1500 - correctionOutput,
        1000,
        2000
    );

    // Map V2 internals onto the existing diagnostics until the binary logger
    // replaces the legacy CSV schema.
    huntControlYaw = predictedYaw;
    huntSlowYaw = driftReferenceYaw;
    huntFastYaw = referenceError;
    huntBlend = settledBlend;
    huntScore = throttlePredictionBlend;

    outputChatterSlow = directCorrection;
    outputChatterFast = integralCorrection;
    outputChatterBlend = driverActivityBlend;
    outputChatterScore = throttlePredictionBlend;

    terrainAssistBlend = 0.0f;
    activeHoldFactor = 1.0f;

    return servoOutput;
}


void GyroController::setGain(float value)
{
    gyroGain = constrain(
        value,
        0.0f,
        10.0f
    );
}


float GyroController::getGain()
{
    return gyroGain;
}


void GyroController::setDeadband(float value)
{
    deadband = constrain(
        value,
        0.0f,
        100.0f
    );
}


float GyroController::getDeadband()
{
    return deadband;
}


void GyroController::setSmoothing(float value)
{
    smoothing = constrain(
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
    maxCorrection = constrain(
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
    integralGain = constrain(
        value,
        0.0f,
        20.0f
    );

    if(integralGain <= 0.0f)
    {
        integralAccumulator = 0.0f;
        integralCorrection = 0;
    }
}


float GyroController::getIntegralGain()
{
    return integralGain;
}


void GyroController::setIntegralLimit(int value)
{
    integralLimit = constrain(
        value,
        0,
        500
    );

    if(integralLimit <= 0)
    {
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
    holdBoost = constrain(
        value,
        0,
        100
    );
}


int GyroController::getHoldBoost()
{
    return holdBoost;
}

void GyroController::setCounterSteerAssist(int value)
{
    counterSteerAssist = constrain(value, 0, 100);

    if(counterSteerAssist <= 0)
    {
        counterSteerCorrection = 0;
    }
}

int GyroController::getCounterSteerAssist()
{
    return counterSteerAssist;
}

int GyroController::getCounterSteerCorrection()
{
    return counterSteerCorrection;
}

void GyroController::setTailSlideSpeed(int value)
{
    tailSlideSpeed = constrain(value, 0, 100);

    if(tailSlideSpeed == 50)
    {
        tailSlideBlend = 0.0f;
    }
}

int GyroController::getTailSlideSpeed()
{
    return tailSlideSpeed;
}

float GyroController::getTailSlideBlend()
{
    return tailSlideBlend;
}


void GyroController::setAntiWobble(int value)
{
    antiWobble = constrain(
        value,
        0,
        200
    );
}


int GyroController::getAntiWobble()
{
    return antiWobble;
}


void GyroController::setHuntDamping(int value)
{
    huntDamping = constrain(
        value,
        0,
        100
    );
}


int GyroController::getHuntDamping()
{
    return huntDamping;
}


int GyroController::applyOutputChatterDamping(
    int correction,
    int steeringCommand,
    bool steeringSignal,
    float dt
)
{
    (void)steeringCommand;
    (void)steeringSignal;
    (void)dt;

    return constrain(
        correction,
        -maxCorrection,
        maxCorrection
    );
}


float GyroController::getHuntControlYaw()
{
    return huntControlYaw;
}


float GyroController::getHuntSlowYaw()
{
    return huntSlowYaw;
}


float GyroController::getHuntFastYaw()
{
    return huntFastYaw;
}


float GyroController::getHuntBlend()
{
    return huntBlend;
}


float GyroController::getHuntScore()
{
    return huntScore;
}


float GyroController::getOutputChatterSlow()
{
    return outputChatterSlow;
}


float GyroController::getOutputChatterFast()
{
    return outputChatterFast;
}


float GyroController::getOutputChatterBlend()
{
    return outputChatterBlend;
}


float GyroController::getOutputChatterScore()
{
    return outputChatterScore;
}


float GyroController::getSteeringActivity()
{
    return steeringActivity;
}


int GyroController::getControlPhase()
{
    return controlPhase;
}


float GyroController::getSettledBlend()
{
    return settledBlend;
}


float GyroController::getThrottleTransient()
{
    return throttleTransientTime;
}


bool GyroController::getTerrainActive()
{
    return false;
}


float GyroController::getTerrainAssist()
{
    return terrainAssistBlend;
}


float GyroController::getActiveHoldFactor()
{
    return activeHoldFactor;
}


float GyroController::getFilteredYaw()
{
    return filteredYaw;
}


int GyroController::getServoOutput()
{
    return servoOutput;
}
