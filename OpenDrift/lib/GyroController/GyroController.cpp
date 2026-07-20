#include "GyroController.h"

#include <math.h>


namespace
{
    static constexpr uint8_t PHASE_IDLE = 0;
    static constexpr uint8_t PHASE_ENTRY = 1;
    static constexpr uint8_t PHASE_SETTLED = 2;
    static constexpr uint8_t PHASE_TRANSITION = 3;

    static constexpr float SETTLE_DELAY_SECONDS = 0.45f;
    static constexpr float TRANSITION_SECONDS = 0.25f;
    static constexpr float THROTTLE_TRANSIENT_SECONDS = 0.25f;
}


void GyroController::resetDynamicState()
{
    filteredYaw = 0;

    integralAccumulator = 0;
    integralCorrection = 0;

    holdBoostFiltered = 0;
    activeHoldFactor = 1.0f;

    huntControlYaw = 0;
    huntSlowYaw = 0;
    huntFastYaw = 0;
    huntBlend = 0;
    huntScore = 0;
    huntReversalAge = 10.0f;
    huntFastDirection = 0;

    driftDirection = 0;
    directionTime = 0;
    transitionTime = 0;
    controlPhase = PHASE_IDLE;
    settledBlend = 0;

    throttleTransientTime = 0;
    lastThrottlePulse = 1500;
    throttleReady = false;

    servoOutput = 1500;
    correctionOutput = 0;
}


bool GyroController::begin()
{
    gyroOffset = 0;
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
    int throttlePulse,
    bool throttleSignal
)
{
    uint32_t now = micros();

    float dt = 0.02f;

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


    // Throttle is deliberately not treated as vehicle speed. A sufficiently
    // large pulse change only marks a short transient during which settled-
    // drift features are allowed to release instead of accumulating harder.
    if(throttleSignal)
    {
        throttlePulse = constrain(
            throttlePulse,
            900,
            2100
        );

        if(!throttleReady)
        {
            lastThrottlePulse = throttlePulse;
            throttleReady = true;
        }
        else
        {
            if(abs(throttlePulse - lastThrottlePulse) >= 20)
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
        throttleTransientTime = 0;
    }

    throttleTransientTime = max(
        0.0f,
        throttleTransientTime - dt
    );


    float correctedYaw =
        yawRate - gyroOffset;

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


    // Time-based input low-pass. Existing setting semantics are retained:
    // larger smoothing values produce a slower yaw signal.
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

    float oldFilteredYaw = filteredYaw;

    filteredYaw =
        (filteredYaw * (1.0f - filterAmount))
        +
        (correctedYaw * filterAmount);

    float yawAbs = fabsf(filteredYaw);
    float oldYawAbs = fabsf(oldFilteredYaw);

    float yawBuildRate =
        (yawAbs - oldYawAbs)
        /
        dt;

    bool yawCrossedZero =
        oldFilteredYaw * filteredYaw < 0.0f;

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

    bool directionEvent =
        yawCrossedZero || directionChanged;

    if(directionEvent)
    {
        driftDirection = definiteDirection;
        directionTime = 0;
        transitionTime = TRANSITION_SECONDS;

        huntSlowYaw = filteredYaw;
        huntFastDirection = 0;
        huntReversalAge = 10.0f;
        huntScore = 0;
        huntBlend = 0;
    }
    else if(driftDirection == 0 && definiteDirection != 0)
    {
        driftDirection = definiteDirection;
        directionTime = 0;
        huntSlowYaw = filteredYaw;
    }
    else if(driftDirection != 0)
    {
        directionTime += dt;
    }

    transitionTime = max(
        0.0f,
        transitionTime - dt
    );


    // Track the average rotation independently of the user-facing smoothing
    // control. Hunt detection compares the current yaw against this baseline.
    if(yawAbs < 3.0f || directionEvent)
    {
        huntSlowYaw = filteredYaw;
    }
    else
    {
        float slowAmount =
            1.0f - expf(-dt / 0.30f);

        huntSlowYaw +=
            (filteredYaw - huntSlowYaw)
            *
            slowAmount;
    }

    huntFastYaw =
        filteredYaw - huntSlowYaw;


    bool nearTrueIdle =
        yawAbs < 8.0f &&
        fabsf(huntSlowYaw) < 15.0f;

    if(nearTrueIdle)
    {
        controlPhase = PHASE_IDLE;
        directionTime = 0;

        if(yawAbs < 3.0f)
        {
            driftDirection = 0;
        }
    }
    else if(transitionTime > 0.0f)
    {
        controlPhase = PHASE_TRANSITION;
    }
    else if(
        throttleTransientTime > 0.0f ||
        directionTime < SETTLE_DELAY_SECONDS ||
        fabsf(huntSlowYaw) < 30.0f
    )
    {
        controlPhase = PHASE_ENTRY;
    }
    else
    {
        controlPhase = PHASE_SETTLED;
    }

    float settledTarget =
        controlPhase == PHASE_SETTLED
        ?
        1.0f
        :
        0.0f;

    float settledTimeConstant =
        settledTarget > settledBlend
        ?
        0.18f
        :
        0.07f;

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


    // Detect actual hunting. One fast-yaw excursion only establishes a side;
    // repeated alternating excursions build the score. Monotonic entries and
    // exits therefore do not automatically enable Hunt Damping.
    huntReversalAge += dt;

    if(
        controlPhase == PHASE_SETTLED &&
        settledBlend > 0.35f
    )
    {
        huntScore *= expf(-dt / 1.40f);

        int8_t fastDirection =
            huntFastYaw > 12.0f
            ?
            1
            :
            (
                huntFastYaw < -12.0f
                ?
                -1
                :
                0
            );

        if(
            fastDirection != 0 &&
            fastDirection != huntFastDirection
        )
        {
            bool validReversal =
                huntFastDirection != 0 &&
                huntReversalAge >= 0.08f &&
                huntReversalAge <= 1.20f;

            if(validReversal)
            {
                huntScore = min(
                    1.0f,
                    huntScore + 0.50f
                );
            }

            huntFastDirection = fastDirection;
            huntReversalAge = 0;
        }
    }
    else
    {
        huntScore *= expf(-dt / 0.12f);
        huntFastDirection = 0;
        huntReversalAge = 10.0f;
    }

    float huntThreshold =
        huntBlend > 0.10f
        ?
        0.30f
        :
        0.55f;

    bool huntDetected =
        huntDamping > 0 &&
        controlPhase == PHASE_SETTLED &&
        huntScore >= huntThreshold;

    float huntTarget =
        huntDetected ? 1.0f : 0.0f;

    float huntTimeConstant =
        huntDetected ? 0.12f : 0.06f;

    float huntAmount =
        1.0f - expf(-dt / huntTimeConstant);

    huntBlend +=
        (huntTarget - huntBlend)
        *
        huntAmount;

    huntBlend = constrain(
        huntBlend,
        0.0f,
        1.0f
    );

    float fastAttenuation =
        0.75f
        *
        (
            huntDamping
            /
            100.0f
        )
        *
        huntBlend;

    huntControlYaw =
        huntSlowYaw
        +
        (
            huntFastYaw
            *
            (1.0f - fastAttenuation)
        );

    // The old shelving filter could retain a large correction while measured
    // yaw was collapsing. Hunt Damping may soften a peak, but it may never
    // invent more yaw magnitude or carry the old direction through zero.
    if(
        huntControlYaw * filteredYaw <= 0.0f ||
        fabsf(huntControlYaw) > yawAbs
    )
    {
        huntControlYaw = filteredYaw;
    }

    if(huntDamping <= 0)
    {
        huntControlYaw = filteredYaw;
        huntBlend = 0;
        huntScore = 0;
    }


    // Hold Boost belongs to an established drift. Its reference is capped by
    // current measured yaw so it cannot preserve a stale high-yaw command on
    // the way out of a drift.
    float holdTarget = 0;

    if(
        holdBoost > 0 &&
        settledBlend > 0.0f
    )
    {
        float holdReference = min(
            yawAbs,
            fabsf(huntSlowYaw)
        );

        float holdAmountTarget = constrain(
            (holdReference - 45.0f) / 85.0f,
            0.0f,
            1.0f
        );

        if(yawBuildRate > 150.0f)
        {
            holdAmountTarget = 0;
        }
        else if(yawBuildRate > 50.0f)
        {
            holdAmountTarget *=
                1.0f -
                (
                    (yawBuildRate - 50.0f)
                    /
                    100.0f
                );
        }

        holdTarget =
            holdAmountTarget
            *
            settledBlend;
    }

    float holdTimeConstant =
        holdTarget > holdBoostFiltered
        ?
        0.12f
        :
        0.06f;

    float holdFilterAmount =
        1.0f - expf(-dt / holdTimeConstant);

    holdBoostFiltered +=
        (holdTarget - holdBoostFiltered)
        *
        holdFilterAmount;

    holdBoostFiltered = constrain(
        holdBoostFiltered,
        0.0f,
        1.0f
    );

    activeHoldFactor =
        1.0f
        +
        (
            holdBoostFiltered
            *
            (
                holdBoost
                /
                100.0f
            )
        );

    int proportionalCorrection =
        (int)(
            huntControlYaw
            *
            gyroGain
            *
            activeHoldFactor
        );


    // Drift Memory now accumulates only while settled. Entries, throttle
    // transients, and transitions actively release it instead of allowing a
    // previous drift to steer the next event.
    if(
        integralGain > 0.0f &&
        integralLimit > 0
    )
    {
        float integralDecayBase = 0.94f;

        if(controlPhase == PHASE_SETTLED)
        {
            integralDecayBase = 0.998f;
        }
        else if(controlPhase == PHASE_TRANSITION)
        {
            integralDecayBase = 0.72f;
        }
        else if(controlPhase == PHASE_IDLE)
        {
            integralDecayBase = 0.0f;
        }

        bool integralOpposesYaw =
            integralAccumulator * filteredYaw < 0.0f;

        if(integralOpposesYaw)
        {
            integralDecayBase = min(
                integralDecayBase,
                0.82f
            );
        }

        float decay =
            integralDecayBase <= 0.0f
            ?
            0.0f
            :
            powf(
                integralDecayBase,
                dt / 0.02f
            );

        integralAccumulator *= decay;

        if(controlPhase == PHASE_SETTLED)
        {
            integralAccumulator +=
                huntControlYaw
                *
                dt
                *
                settledBlend;
        }

        float maxAccumulator =
            integralLimit
            /
            integralGain;

        integralAccumulator = constrain(
            integralAccumulator,
            -maxAccumulator,
            maxAccumulator
        );

        integralCorrection = constrain(
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


    int targetCorrection = constrain(
        proportionalCorrection + integralCorrection,
        -maxCorrection,
        maxCorrection
    );

    bool idleSnap =
        correctedYaw == 0.0f &&
        yawAbs < 1.0f;

    if(idleSnap)
    {
        integralAccumulator = 0;
        integralCorrection = 0;
        holdBoostFiltered = 0;
        activeHoldFactor = 1.0f;
        huntBlend = 0;
        huntScore = 0;
        targetCorrection = 0;
    }


    // AntiWobble is intentionally limited to tiny, near-center correction
    // chatter. Broad drift smoothing belongs to Hunt Damping; applying it
    // here created phase delay in the old controller.
    int correctionDelta =
        targetCorrection - correctionOutput;

    int absCorrectionDelta =
        abs(correctionDelta);

    if(
        !idleSnap &&
        antiWobble > 0 &&
        yawAbs < 18.0f
    )
    {
        float antiStrength =
            antiWobble
            /
            200.0f;

        int holdThreshold =
            1 + (int)roundf(5.0f * antiStrength);

        int blendThreshold =
            6 + (int)roundf(18.0f * antiStrength);

        if(absCorrectionDelta <= holdThreshold)
        {
            targetCorrection = correctionOutput;
        }
        else if(absCorrectionDelta <= blendThreshold)
        {
            float correctionBlend =
                1.0f - (0.65f * antiStrength);

            targetCorrection =
                correctionOutput
                +
                (int)roundf(
                    correctionDelta
                    *
                    correctionBlend
                );
        }
    }

    correctionOutput = constrain(
        targetCorrection,
        -maxCorrection,
        maxCorrection
    );

    servoOutput = constrain(
        1500 - correctionOutput,
        1000,
        2000
    );

    return servoOutput;
}


void GyroController::setGain(float value)
{
    gyroGain = value;
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
    integralLimit = constrain(
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

    if(huntDamping == 0)
    {
        huntControlYaw = filteredYaw;
        huntBlend = 0;
        huntScore = 0;
        huntFastDirection = 0;
    }
}


int GyroController::getHuntDamping()
{
    return huntDamping;
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
    return constrain(
        throttleTransientTime
        /
        THROTTLE_TRANSIENT_SECONDS,
        0.0f,
        1.0f
    );
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
