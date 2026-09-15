#include "GyroController.h"

#include <math.h>


namespace
{
    static constexpr uint8_t PHASE_IDLE = 0;
    static constexpr uint8_t PHASE_ENTRY = 1;
    static constexpr uint8_t PHASE_SETTLED = 2;
    static constexpr uint8_t PHASE_TRANSITION = 3;

    static constexpr float TRANSITION_SECONDS = 0.18f;
    static constexpr float THROTTLE_APPLY_SECONDS = 0.22f;
    static constexpr float THROTTLE_LIFT_SECONDS = 0.48f;
    static constexpr float MEMORY_GAIN_SCALE = 6.0f;

    static constexpr float HUNT_BASELINE_SECONDS = 0.45f;
    static constexpr float HUNT_NOTCH_FREQUENCY_HZ = 3.2f;
    static constexpr float HUNT_NOTCH_Q = 1.25f;
    static constexpr float HUNT_NOTCH_MIN_HZ = 2.5f;
    static constexpr float HUNT_NOTCH_MAX_HZ = 3.6f;
    static constexpr float HUNT_NOTCH_TRACK_SECONDS = 1.20f;
    static constexpr float HUNT_NOTCH_ENTRY_TRACK_SECONDS = 2.40f;
    static constexpr float HUNT_NOTCH_UPDATE_STEP_HZ = 0.01f;
    static constexpr float HUNT_ENTRY_ARM_BLEND = 0.30f;
    static constexpr float HUNT_ENTRY_HOLD_SECONDS = 0.65f;
    static constexpr float HUNT_ENTRY_HOLD_FADE_SECONDS = 0.20f;
    static constexpr float HUNT_SETTLED_DEPTH_BOOST = 0.17f;
    static constexpr float HUNT_GUARD_DEPTH_SCALE = 0.20f;
    static constexpr float HUNT_TRANSIENT_RELEASE_SECONDS = 0.10f;
    // Blackbox 39-41 identified the repeatable wheel mode at 2.6-3.5 Hz.
    // Keep margin around it without accepting the slower chassis motion that
    // log 41 showed being mistaken for hunt.
    static constexpr float HUNT_MIN_HALF_PERIOD = 0.11f;
    static constexpr float HUNT_MAX_HALF_PERIOD = 0.23f;
    static constexpr float HUNT_LATCH_SECONDS = 0.75f;
    static float lastCorrection = 0;
    static float filteredOutput = 0;
    static float correctionOutput = 0;

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
    transitionSpeedBlend = 0.0f;
    lastSteeringCommand = 1500;
    steeringReady = false;

    throttleRate = 0.0f;
    throttleTransientTime = 0.0f;
    throttleApplyTime = 0.0f;
    throttleLiftTime = 0.0f;
    previousThrottleLevel = 0.0f;
    filteredThrottleLoadRate = 0.0f;
    throttleLiftBlend = 0.0f;
    lastThrottlePulse = 1500;
    throttleReady = false;

    controlPhase = PHASE_IDLE;
    settledBlend = 0.0f;
    transitionAuthorityBlend = 0.0f;

    huntBaselineYaw = 0.0f;
    huntHalfCyclePeak = 0.0f;
    huntAmplitude = 0.0f;
    huntCrossingAge = 0.0f;
    huntConfidence = 0.0f;
    huntSuppression = 0.0f;
    huntFrequency = 0.0f;
    huntResidualEnvelope = 0.0f;
    huntCandidateHoldTime = 0.0f;
    huntLatchTime = 0.0f;
    huntConsistentHalfCycles = 0;
    huntResidualSign = 0;
    huntBaselineReady = false;
    huntNotchX1 = 0.0f;
    huntNotchX2 = 0.0f;
    huntNotchY1 = 0.0f;
    huntNotchY2 = 0.0f;
    huntNotchReady = false;
    huntNotchTrackingHz = huntNotchCenterHz;
    huntNotchTargetHz = huntNotchCenterHz;

    predictedYawTelemetry = 0.0f;
    driftReferenceTelemetry = 0.0f;
    referenceErrorTelemetry = 0.0f;
    referenceLockTelemetry = 0.0f;
    throttlePredictionTelemetry = 0.0f;
    directCorrectionTelemetry = 0.0f;
    memoryFeedbackTelemetry = 0.0f;
    driverActivityTelemetry = 0.0f;
    throttlePredictionBlendTelemetry = 0.0f;
    throttleLiftBlendTelemetry = 0.0f;
    huntSuppressionTelemetry = 0.0f;
    huntFrequencyTelemetry = 0.0f;
    huntResidualTelemetry = 0.0f;
    huntResidualEnvelopeTelemetry = 0.0f;
    huntRemovedCorrectionTelemetry = 0.0f;
    huntConsistentHalfCyclesTelemetry = 0;
    huntLatchTelemetry = 0.0f;
    transitionAuthorityTelemetry = 0.0f;
    transitionPredictionScaleTelemetry = 1.0f;

    requestedCorrectionOutput = 0;
    correctionOutput = 0;
    lastCorrection = 0;
}


bool GyroController::begin()
{
    gyroOffset = 0.0f;
    calibrated = false;
    lastUpdateMicros = 0;

    // Always leave begin() with a working notch. The selected rate is applied
    // by setup immediately afterward, but 250 Hz is a safe standalone default.
    if(huntNotchControlLoopHz == 0)
    {
        setControlLoopHz(250);
    }

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
    bool throttleSignal
)
{
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
            previousThrottleLevel = throttleLevel;
            filteredThrottleLoadRate = 0.0f;
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

            float rawThrottleLoadRate =
                (throttleLevel - previousThrottleLevel)
                /
                dt;

            float throttleAmount =
                1.0f - expf(-dt / 0.04f);

            throttleRate +=
                (rawThrottleRate - throttleRate)
                *
                throttleAmount;

            filteredThrottleLoadRate +=
                (
                    rawThrottleLoadRate
                    -
                    filteredThrottleLoadRate
                )
                *
                throttleAmount;

            float throttleLoadDelta =
                throttleLevel - previousThrottleLevel;

            bool applyingThrottle =
                throttleLoadDelta >= 0.012f ||
                filteredThrottleLoadRate >= 0.30f;

            bool liftingThrottle =
                previousThrottleLevel >= 0.06f &&
                (
                    throttleLoadDelta <= -0.012f ||
                    filteredThrottleLoadRate <= -0.22f
                );

            if(applyingThrottle)
            {
                throttleApplyTime =
                    THROTTLE_APPLY_SECONDS;
            }

            if(liftingThrottle)
            {
                throttleLiftTime =
                    THROTTLE_LIFT_SECONDS;
            }

            lastThrottlePulse = throttlePulse;
            previousThrottleLevel = throttleLevel;
        }
    }

    else
    {
        throttleReady = false;
        throttleRate = 0.0f;
        filteredThrottleLoadRate = 0.0f;
        previousThrottleLevel = 0.0f;
        throttleApplyTime = 0.0f;
        throttleLiftTime = 0.0f;
        throttleTransientTime = 0.0f;
    }

    throttleApplyTime = max(
        0.0f,
        throttleApplyTime - dt
    );

    throttleLiftTime = max(
        0.0f,
        throttleLiftTime - dt
    );

    throttleTransientTime = max(
        throttleApplyTime,
        throttleLiftTime
    );

    float throttleRateBlend = constrain(
        (fabsf(throttleRate) - 250.0f) / 3000.0f,
        0.0f,
        1.0f
    );

    float throttleApplyBlend = constrain(
        throttleApplyTime
        /
        THROTTLE_APPLY_SECONDS,
        0.0f,
        1.0f
    );

    throttleLiftBlend = constrain(
        throttleLiftTime
        /
        THROTTLE_LIFT_SECONDS,
        0.0f,
        1.0f
    );

    float throttlePredictionBlend = max(
        throttleRateBlend,
        max(
            throttleApplyBlend * 0.55f,
            throttleLiftBlend * 0.80f
        )
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

    // Zero is a true software-filter bypass for sensor-bandwidth A/B tests.
    // Non-zero values retain the original time-based smoothing curve.
    float filterAmount = 1.0f;

    if(smoothing > 0.0f)
    {
        float baseFilterAmount =
            1.0f -
            constrain(
                smoothing,
                0.01f,
                0.99f
            );

        filterAmount =
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
    }

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
    // Moving toward neutral gets a longer envelope because track data shows
    // lift response developing over several hundred milliseconds.
    // Steering intent announces a transition before measured yaw crosses
    // zero. Retain the base look-ahead, but taper the optional acceleration
    // and throttle extensions so they cannot leap ahead of the chassis and
    // create a correction discontinuity during a direction change.
    float transitionPredictionIntent = max(
        transitionAuthorityBlend,
        constrain(
            (driverActivityBlend - 0.15f) / 0.85f,
            0.0f,
            1.0f
        )
    );

    float optionalPredictionSeconds =
        (
            predictionStrength
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
        throttleLiftBlend
        *
        0.010f
        +
        throttleLevel
        *
        0.003f;

    float transitionPredictionScale =
        1.0f - 0.75f * transitionPredictionIntent;

    float predictionSeconds =
        0.003f
        +
        optionalPredictionSeconds
        *
        transitionPredictionScale;

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
        // Acceleration may anticipate the new direction, but measured yaw is
        // still rotating the old way. Fall back to continuous measured-yaw
        // damping instead of creating a zero-correction hole.
        predictedYaw = filteredYaw;
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

    float deliberateTransitionBlend =
        !idle
        ?
        constrain(
            (driverActivityBlend - 0.25f) / 0.75f,
            0.0f,
            1.0f
        )
        :
        0.0f;

    float directionTransitionBlend =
        constrain(
            transitionTime / TRANSITION_SECONDS,
            0.0f,
            1.0f
        );

    float transitionAuthorityTarget = max(
        deliberateTransitionBlend,
        directionTransitionBlend
    );

    float transitionAuthoritySeconds =
        transitionAuthorityTarget > transitionAuthorityBlend
        ?
        0.025f
        :
        0.16f;

    float transitionAuthorityAmount =
        1.0f - expf(-dt / transitionAuthoritySeconds);

    transitionAuthorityBlend +=
        (
            transitionAuthorityTarget
            -
            transitionAuthorityBlend
        )
        *
        transitionAuthorityAmount;

    transitionAuthorityBlend = constrain(
        transitionAuthorityBlend,
        0.0f,
        1.0f
    );

    float quietBlend =
        (1.0f - driverActivityBlend)
        *
        (1.0f - throttlePredictionBlend);

    bool driftActive =
        !idle &&
        definiteDirection != 0;

    float settledTarget =
        driftActive &&
        transitionTime <= 0.0f &&
        transitionAuthorityBlend < 0.20f
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
    else if(
        transitionTime > 0.0f ||
        transitionAuthorityBlend > 0.35f
    )
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

    // Periodicity and residual energy remain diagnostic telemetry. Log 44
    // showed that using the detector as a gate made damping pulse on and off,
    // so the settled/driver context controls notch depth instead.
    static constexpr float huntResidualThreshold = 3.0f;
    static constexpr float huntMinimumPeak = 3.5f;
    static constexpr float huntAbsoluteFrequencyTolerance = 1.55f;
    static constexpr float huntRelativeFrequencyTolerance = 0.475f;
    static constexpr uint8_t huntRequiredHalfCycles = 1;

    bool huntCandidateSeed =
        huntStrength > 0 &&
        driftActive &&
        settledBlend >= HUNT_ENTRY_ARM_BLEND &&
        driverActivityBlend <= 0.38f &&
        transitionAuthorityBlend <= 0.25f &&
        yawAbs >= 15.0f;

    bool huntDisabled =
        huntStrength <= 0;

    bool huntTrackingBlocked =
        huntDisabled ||
        transitionTime > 0.0f ||
        transitionAuthorityBlend > 0.45f ||
        driverActivityBlend > 0.75f;

    if(huntTrackingBlocked)
    {
        huntCandidateHoldTime = 0.0f;
        huntLatchTime = 0.0f;
        huntConsistentHalfCycles = 0;
        huntConfidence = 0.0f;
        huntAmplitude = 0.0f;
        huntFrequency = 0.0f;
        huntResidualEnvelope = 0.0f;

        if(huntDisabled)
        {
            huntSuppression = 0.0f;
        }
    }
    else if(huntCandidateSeed)
    {
        // Log 51 showed the 3 Hz mode appearing during quiet late entry, then
        // notch depth falling as settledBlend briefly dipped before lock. Arm
        // before PHASE_SETTLED and bridge almost two full wheel-mode cycles.
        // Intentional steering or a real transition still freezes tracking.
        huntCandidateHoldTime = HUNT_ENTRY_HOLD_SECONDS;
    }
    else
    {
        huntCandidateHoldTime = max(
            0.0f,
            huntCandidateHoldTime - dt
        );
    }

    bool huntCandidate =
        !huntTrackingBlocked &&
        huntCandidateHoldTime > 0.0f;

    if(!huntTrackingBlocked)
    {
        huntLatchTime = max(
            0.0f,
            huntLatchTime - dt
        );
    }

    bool huntLatched =
        !huntTrackingBlocked &&
        huntLatchTime > 0.0f;

    float huntSettledContext = constrain(
        (settledBlend - 0.25f) / 0.35f,
        0.0f,
        1.0f
    );

    float huntHeldContext = constrain(
        huntCandidateHoldTime / HUNT_ENTRY_HOLD_FADE_SECONDS,
        0.0f,
        1.0f
    );

    float huntDriverQuiet =
        1.0f - constrain(
            (driverActivityBlend - 0.38f) / 0.37f,
            0.0f,
            1.0f
        );

    float huntTransitionQuiet =
        1.0f - constrain(
            (transitionAuthorityBlend - 0.25f) / 0.20f,
            0.0f,
            1.0f
        );

    float huntControlContext =
        !huntTrackingBlocked && huntStrength > 0
        ?
        max(huntSettledContext, huntHeldContext)
        *
        huntDriverQuiet
        *
        huntTransitionQuiet
        :
        0.0f;

    bool huntActiveContext =
        huntControlContext > 0.0f || huntLatched;

    if(!huntBaselineReady)
    {
        huntBaselineYaw = filteredYaw;
        huntBaselineReady = true;
    }

    float huntBaselineSeconds =
        huntActiveContext
        ?
        HUNT_BASELINE_SECONDS
        :
        0.08f;

    float huntBaselineAmount =
        1.0f - expf(-dt / huntBaselineSeconds);

    huntBaselineYaw +=
        (filteredYaw - huntBaselineYaw)
        *
        huntBaselineAmount;

    float huntResidual =
        filteredYaw - huntBaselineYaw;

    float huntEnvelopeTarget =
        huntActiveContext
        ?
        fabsf(huntResidual)
        :
        0.0f;

    float huntEnvelopeSeconds =
        huntEnvelopeTarget > huntResidualEnvelope
        ?
        0.06f
        :
        0.55f;

    huntResidualEnvelope +=
        (huntEnvelopeTarget - huntResidualEnvelope)
        *
        (1.0f - expf(-dt / huntEnvelopeSeconds));

    huntCrossingAge = min(
        huntCrossingAge + dt,
        2.0f
    );

    huntConfidence = max(
        0.0f,
        huntConfidence - dt * 0.18f
    );

    if(huntCandidate)
    {
        huntHalfCyclePeak = max(
            huntHalfCyclePeak,
            fabsf(huntResidual)
        );

        int8_t residualSign =
            huntResidual >= huntResidualThreshold
            ?
            1
            :
            (
                huntResidual <= -huntResidualThreshold
                ?
                -1
                :
                0
            );

        if(residualSign != 0)
        {
            if(huntResidualSign == 0)
            {
                huntResidualSign = residualSign;
                huntCrossingAge = 0.0f;
                huntHalfCyclePeak = fabsf(huntResidual);
            }
            else if(residualSign != huntResidualSign)
            {
                bool validHalfCycle =
                    huntCrossingAge >= HUNT_MIN_HALF_PERIOD &&
                    huntCrossingAge <= HUNT_MAX_HALF_PERIOD &&
                    huntHalfCyclePeak >= huntMinimumPeak;

                if(validHalfCycle)
                {
                    float measuredFrequency =
                        1.0f / (2.0f * huntCrossingAge);

                    bool frequencyConsistent =
                        huntFrequency <= 0.1f ||
                        fabsf(
                            measuredFrequency - huntFrequency
                        ) <= max(
                            huntAbsoluteFrequencyTolerance,
                            huntFrequency *
                                huntRelativeFrequencyTolerance
                        );

                    if(frequencyConsistent)
                    {
                        huntConsistentHalfCycles = min(
                            (uint8_t)8,
                            (uint8_t)(huntConsistentHalfCycles + 1)
                        );

                        huntConfidence = min(
                            1.0f,
                            huntConfidence + 0.30f
                        );
                    }
                    else
                    {
                        // Start a new candidate sequence rather than joining
                        // unrelated slow chassis motion into a hunt event.
                        huntConsistentHalfCycles = 1;
                        huntConfidence *= 0.45f;
                    }

                    if(huntFrequency <= 0.1f || !frequencyConsistent)
                    {
                        huntFrequency = measuredFrequency;
                    }
                    else
                    {
                        huntFrequency +=
                            (measuredFrequency - huntFrequency)
                            *
                            0.35f;
                    }

                    huntAmplitude +=
                        (huntHalfCyclePeak - huntAmplitude)
                        *
                        0.40f;

                    if(
                        frequencyConsistent &&
                        huntConsistentHalfCycles >=
                            huntRequiredHalfCycles
                    )
                    {
                        huntLatchTime = HUNT_LATCH_SECONDS;
                    }
                }
                else
                {
                    huntConsistentHalfCycles = 0;
                    huntConfidence *= 0.55f;
                }

                huntResidualSign = residualSign;
                huntCrossingAge = 0.0f;
                huntHalfCyclePeak = fabsf(huntResidual);
            }
        }
    }
    else
    {
        huntResidualSign = 0;
        huntCrossingAge = 0.0f;
        huntHalfCyclePeak = 0.0f;
        if(!huntLatched)
        {
            huntAmplitude +=
                (0.0f - huntAmplitude)
                *
                (1.0f - expf(-dt / 0.35f));
        }
    }

    // The observed wheel mode moves with loop gain. Quiet late entry may now
    // collect and follow confirmed half-cycles, but at half the settled
    // tracking speed so coefficient motion cannot become a steering input.
    // Intentional transitions still freeze both target and center.
    if(
        huntCandidate &&
        huntConsistentHalfCycles > 0 &&
        huntFrequency >= HUNT_NOTCH_MIN_HZ &&
        huntFrequency <= HUNT_NOTCH_MAX_HZ
    )
    {
        huntNotchTargetHz = constrain(
            huntFrequency,
            HUNT_NOTCH_MIN_HZ,
            HUNT_NOTCH_MAX_HZ
        );

        float huntTrackingSeconds =
            settledBlend >= 0.55f
            ?
            HUNT_NOTCH_TRACK_SECONDS
            :
            HUNT_NOTCH_ENTRY_TRACK_SECONDS;

        huntNotchTrackingHz +=
            (huntNotchTargetHz - huntNotchTrackingHz)
            *
            (1.0f - expf(-dt / huntTrackingSeconds));

        if(
            fabsf(huntNotchTrackingHz - huntNotchCenterHz) >=
                HUNT_NOTCH_UPDATE_STEP_HZ
        )
        {
            configureHuntNotch(huntNotchTrackingHz, false);
        }
    }

    float huntStrengthBlend =
        huntStrength / 100.0f;

    // Log 52 confirms that early arming removed most of the pre-settle burst,
    // but the remaining sustained-drift mode occurs with the notch correctly
    // tracked and suppression already at the requested depth. Add a gradual
    // settled-only depth boost instead of changing entry behavior or widening
    // the notch into useful chassis yaw. At Anti Wobble 75 this reaches
    // about 0.88 once fully settled, roughly halving the residual band that
    // was still being passed while preserving the proven entry response.
    float huntSettledDepthBlend = constrain(
        (settledBlend - 0.55f) / 0.30f,
        0.0f,
        1.0f
    );

    float huntEffectiveStrength = constrain(
        huntStrengthBlend
        *
        (
            1.0f
            +
            HUNT_SETTLED_DEPTH_BOOST
            *
            huntSettledDepthBlend
        ),
        0.0f,
        1.0f
    );

    // A binary bypass was releasing the 3 Hz steering mode completely during
    // entry, transitions, and strong driver input. Keep a shallow guard notch
    // anywhere the car is actively rotating, while still freezing frequency
    // tracking so an intentional reversal cannot drag the notch center. This
    // removes only a small part of the resonant band and leaves the rest of
    // the transition spectrum untouched.
    bool huntMotionActive =
        !idle &&
        (
            driftActive ||
            transitionTime > 0.0f ||
            transitionAuthorityBlend > 0.20f
        );

    float huntGuardTarget =
        !huntDisabled && huntMotionActive
        ?
        huntStrengthBlend * HUNT_GUARD_DEPTH_SCALE
        :
        0.0f;

    float huntContextTarget =
        huntControlContext > 0.0f
        ?
        huntEffectiveStrength * huntControlContext
        :
        0.0f;

    float huntSuppressionTarget =
        max(huntContextTarget, huntGuardTarget);

    float huntSuppressionSeconds =
        huntSuppressionTarget > huntSuppression
        ?
        0.08f
        :
        (
            huntTrackingBlocked || idle
            ?
            HUNT_TRANSIENT_RELEASE_SECONDS
            :
            0.45f
        );

    huntSuppression +=
        (
            huntSuppressionTarget
            -
            huntSuppression
        )
        *
        (
            1.0f
            -
            expf(-dt / huntSuppressionSeconds)
        );

    huntSuppression = constrain(
        huntSuppression,
        0.0f,
        1.0f
    );

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

    // Transition Speed is centered at 50 and follows the complete transition
    // envelope rather than the short steering-rate pulse. This keeps the
    // adjustment active through the physical yaw reversal and makes matched
    // 25/50/75 tests deliberately obvious.
    transitionSpeedBlend =
        ((transitionSpeed - 50) / 50.0f)
        *
        transitionAuthorityBlend;

    transitionSpeedBlend = constrain(
        transitionSpeedBlend,
        -1.0f,
        1.0f
    );

    float directDampingScale =
        transitionSpeedBlend < 0.0f
        ?
        1.0f - 0.75f * transitionSpeedBlend
        :
        1.0f - 0.55f * transitionSpeedBlend;

    // Run the notch continuously, even while its output is bypassed. This
    // keeps its history aligned with measured motion and prevents a kick when
    // settled-drift suppression engages. The notch passes slow chassis yaw
    // and fast transient correction while isolating the measured 3.2 Hz wheel
    // mode. Prediction remains ahead of the notch because the 0/60 A/B test
    // showed that removing its phase lead made the wobble worse.
    if(!huntNotchReady)
    {
        huntNotchX1 = predictedYaw;
        huntNotchX2 = predictedYaw;
        huntNotchY1 = predictedYaw;
        huntNotchY2 = predictedYaw;
        huntNotchReady = true;
    }

    float huntNotchedYaw =
        huntNotchB0 * predictedYaw
        + huntNotchB1 * huntNotchX1
        + huntNotchB2 * huntNotchX2
        - huntNotchA1 * huntNotchY1
        - huntNotchA2 * huntNotchY2;

    huntNotchX2 = huntNotchX1;
    huntNotchX1 = predictedYaw;
    huntNotchY2 = huntNotchY1;
    huntNotchY1 = huntNotchedYaw;

    float huntNotchBandYaw =
        predictedYaw - huntNotchedYaw;

    float huntRemovedYaw =
        huntNotchBandYaw
        *
        huntSuppression;

    float huntDampedYaw =
        predictedYaw - huntRemovedYaw;

    // Base Direct Correction (Unscaled)
    float baseDirectCorrection =
        huntDampedYaw
        *
        gyroGain
        *
        directDampingScale;

    // Direct correction calculation prior to curve/damper scaling
    float directCorrection = baseDirectCorrection;

    // Countersteer Assist from steady drift reference
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
        *
        (1.0f - 0.75f * transitionAuthorityBlend)
        :
        0.0f;

    counterSteerCorrection =
        (int)roundf(steadyAssistCorrection);

    // Initial total correction before stick/servo deflection scaling
    float baseCorrection =
        directCorrection
        +
        steadyAssistCorrection;

    // Calculate Final Commanded Servo Deflection Offset [0.0, 1.0]
    float estimatedCorrection = -(baseCorrection + integralCorrection);
    
    // Combine driver command with controller correction
    float rawServoCommand = (float)steeringCommand + correctionOutput;

    // Measure total final servo deflection from neutral center (1500 us)
    float commandOffset = fabsf(rawServoCommand - 1500.0f) / 500.0f;
    commandOffset = constrain(commandOffset, 0.0f, 1.0f);
    // float commandOffset = fabsf(estimatedCorrection) / 500.0f;
    // commandOffset = constrain(commandOffset, 0.0f, 1.0f);

    



    // if (curvePower > 1.0f)
    // {

    //     float curveGainScale = 1;
        
    //     if (commandOffset > damperPoint) {
    //         curveGainScale = 5.0f*(curvePower-1.0f)*powf(commandOffset-damperPoint,2) + 1;
    //         // curveGainScale = 5.0f*(curvePower-1.0f)*(commandOffset-damperPoint) + 1;
    //     }
    //     // Apply curve gain scaling to direct correction
    //     directCorrection *= curveGainScale;
    // }

    // // Curve Power (Progressive Stiffening based on Final Servo Deflection)
    // float curveGainScale = 1.0f;
    // if (curvePower > 1.0f)
    // {
    //     curveGainScale = 1.0f + (curvePower - 1.0f) * (commandOffset * commandOffset);
    // }

    // directCorrection *= curveGainScale;

    // // Damper & Damper Point Adjustment (based on Final Servo Deflection)
    // if (damperPower > 0.0f)
    // {
    //     float damperScale = 1.0f;
    //     if (commandOffset < damperPoint && (1.0f - damperPoint) > 0.001f)
    //     {
    //         float excess = (commandOffset - damperPoint) / (1.0f - damperPoint);
    //         damperScale += damperPower * excess * excess;
    //     }
    //     else
    //     {
    //         damperScale += 0; //damperPower * (commandOffset / max(damperPoint, 0.001f)) * 0.25f;
    //     }

    //     directCorrection /= damperScale;
    // }

    // // ------------------------------------------------------------------------
    // // Logarithmic Rate Limit Scheduling on directCorrection
    // // Rapid rate limit boost near center, tapering off near endpoints
    // // ------------------------------------------------------------------------

    // // 2. Rate limit boundaries (microseconds / second)
    // float minRateLimitUsPerSec = 100.0f; // Baseline rate limit at absolute neutral center
    // float maxRateLimitUsPerSec = 8000.0f; // Maximum rate limit near endpoints

    // // 3. Logarithmic Curvature Configuration
    // // k = 1.0f -> Gentle log curve
    // // k = 9.0f -> Steeper initial boost (ln(10) normalization makes math fast)
    // float logCurvature = curvePower*curvePower; 

    // // 4. Compute Normalized Logarithmic Scaling Factor [0.0 to 1.0]
    // float logFactor = logf(1.0f + logCurvature * commandOffset) / logf(1.0f + logCurvature);
    // logFactor = constrain(logFactor, 0.0f, 1.0f);

    // // 5. Interpolate Dynamic Rate Limit
    // float currentRateLimit = minRateLimitUsPerSec + (maxRateLimitUsPerSec - minRateLimitUsPerSec) * logFactor;

    // // 6. Execute Slew-Rate Limiter on directCorrection
    // float maxStep = currentRateLimit * dt;
    // float delta = directCorrection - lastCorrection;
    // delta = constrain(delta, -maxStep, maxStep);

    // lastCorrection += delta;
    // directCorrection = lastCorrection;


    // // 2. Define minimum rate limit at center and maximum rate limit at endpoints (us/sec)
    // float minRateLimitUsPerSec = 100.0f*curvePower*curvePower; // Soft, smooth limit at neutral center
    // float maxRateLimitUsPerSec = 8000.0f; // Fast, responsive limit at full lock

    // // 3. Interpolate dynamic rate limit across commandOffset
    // // (Option A: Linear Interpolation)
    // float currentRateLimit = minRateLimitUsPerSec + (maxRateLimitUsPerSec - minRateLimitUsPerSec) * commandOffset;

    // // float currentRateLimit = minRateLimitUsPerSec;
    // // if (commandOffset > damperPoint) {
    // //     currentRateLimit = minRateLimitUsPerSec + (maxRateLimitUsPerSec - minRateLimitUsPerSec) * powf(commandOffset-damperPoint,2);
    // // }

    // // (Option B: Quadratic Interpolation for a stronger progressive ramp)
    // // float currentRateLimit = minRateLimitUsPerSec + (maxRateLimitUsPerSec - minRateLimitUsPerSec) * (commandOffset * commandOffset);

    // // 4. Calculate maximum allowable step change for the current frame
    // float maxStep = currentRateLimit * dt;

    // // 5. Apply the slew-rate limiter directly to directCorrection
    // float delta = directCorrection - lastCorrection;
    // delta = constrain(delta, -maxStep, maxStep);

    // lastCorrection += delta;
    // directCorrection = lastCorrection;

    // -------------------------------------------------------------------
    // 7. FREQUENCY: Low-pass output smoothing filter (RC filter alpha)
    // -------------------------------------------------------------------
    // Idea: have a parameter that works like countersteer, but make the yaw tracking better
    // Dynamic yaw filter. Increase bandwidth at center of travel to track normal yaw, decrease at ends
    // Scale this term by steering angle
    // float maxFilterBandwidth = 1.0f/dt;
    // float minFilterBandwidth = 100.0f;

    // float currentFilterBandwidth = minFilterBandwidth;
    // if (commandOffset > damperPoint) {
    //     currentFilterBandwidth = minFilterBandwidth + (maxFilterBandwidth - minFilterBandwidth) * powf(commandOffset-damperPoint,2);
    // }

    // float logCurvature = 20; 

    // // 4. Compute Normalized Logarithmic Scaling Factor [0.0 to 1.0]
    // float logFactor = logf(1.0f + logCurvature * commandOffset) / logf(1.0f + logCurvature);
    // logFactor = constrain(logFactor, 0.0f, 1.0f);

    // float currentFilterBandwidth = maxFilterBandwidth + (maxFilterBandwidth - minFilterBandwidth) * -logFactor;

    float damperScale = 1.0f;
    if (commandOffset < damperPoint && (1.0f - damperPoint) > 0.001f)
    {
        // float excess = (commandOffset - damperPoint) / (1.0f - damperPoint);
        // damperScale += damperPower * excess * excess;
        damperScale = (damperPower)*powf(commandOffset-damperPoint,2)+1;
    }
    damperScale = constrain(damperScale,0.0f,1.0f);

    float rc = 1.0f / (2.0f * M_PI * 100);
    float alpha = dt / (rc + dt);

    // Calculate dynamic filtered yaw rate
    filteredOutput += alpha * (huntDampedYaw - filteredOutput);
    // filteredOutput += alpha * (lastCorrection - filteredOutput);

    directCorrection += (curvePower*curvePower-1)*filteredOutput*damperScale;



    float huntRemovedCorrection =
        huntRemovedYaw
        *
        gyroGain
        *
        directDampingScale;

    // Re-evaluate base correction with shaped direct correction
    baseCorrection =
        directCorrection
        +
        steadyAssistCorrection;

    // Transition Speed capping check
    int effectiveMaxCorrection = maxCorrection;

    if(
        fabsf(baseCorrection) >= effectiveMaxCorrection &&
        integralCorrection * baseCorrection > 0.0f
    )
    {
        integralCorrection = 0;
    }

    int requestedControllerCorrection =
        (int)roundf(
            baseCorrection
            +
            integralCorrection
        );

    int targetCorrection =
        constrain(
            requestedControllerCorrection,
            -effectiveMaxCorrection,
            effectiveMaxCorrection
        );
    // ========================================================================
    // END OF PASTED CODE BLOCK
    // ========================================================================

    if(idle && correctedYaw == 0.0f)
    {
        requestedControllerCorrection = 0;
        targetCorrection = 0;
        filteredYawAcceleration = 0.0f;
    }

    // Expose a signed correction, not a fake centered servo command. The
    // controller's sign convention is opposite the servo mix convention.
    // The caller combines this with driver input and performs the one final
    // normalized clamp before calibrated physical endpoints are applied.
    requestedCorrectionOutput = -requestedControllerCorrection;
    correctionOutput = -targetCorrection;

    predictedYawTelemetry = predictedYaw;
    driftReferenceTelemetry = driftReferenceYaw;
    referenceErrorTelemetry = referenceError;
    referenceLockTelemetry = settledBlend;
    throttlePredictionTelemetry = throttlePredictionBlend;
    directCorrectionTelemetry = directCorrection;
    memoryFeedbackTelemetry = integralCorrection;
    driverActivityTelemetry = driverActivityBlend;
    throttlePredictionBlendTelemetry = throttlePredictionBlend;
    throttleLiftBlendTelemetry = throttleLiftBlend;
    huntSuppressionTelemetry = huntSuppression;
    huntFrequencyTelemetry = huntFrequency;
    // Log the actual isolated notch band so the next blackbox directly shows
    // what the controller is targeting, rather than the broader detector
    // residual used only for frequency diagnostics.
    huntResidualTelemetry = huntNotchBandYaw;
    huntResidualEnvelopeTelemetry = huntResidualEnvelope;
    huntRemovedCorrectionTelemetry = huntRemovedCorrection;
    huntConsistentHalfCyclesTelemetry = huntConsistentHalfCycles;
    huntLatchTelemetry = constrain(
        huntLatchTime / HUNT_LATCH_SECONDS,
        0.0f,
        1.0f
    );
    transitionAuthorityTelemetry = transitionAuthorityBlend;
    transitionPredictionScaleTelemetry = transitionPredictionScale;

    return correctionOutput;
}


void GyroController::setGain(float value)
{
    gyroGain = constrain(
        value,
        0.0f,
        6.0f
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
        0.0f,
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


int GyroController::getRequestedCorrection()
{
    return requestedCorrectionOutput;
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

void GyroController::setTransitionSpeed(int value)
{
    transitionSpeed = constrain(value, 0, 100);

    if(transitionSpeed == 50)
    {
        transitionSpeedBlend = 0.0f;
    }
}

int GyroController::getTransitionSpeed()
{
    return transitionSpeed;
}

float GyroController::getTransitionSpeedBlend()
{
    return transitionSpeedBlend;
}


void GyroController::setPredictionStrength(int value)
{
    predictionStrength = constrain(
        value,
        0,
        100
    );
}


int GyroController::getPredictionStrength()
{
    return predictionStrength;
}


void GyroController::setHuntStrength(int value)
{
    huntStrength = constrain(value, 0, 100);
}


int GyroController::getHuntStrength()
{
    return huntStrength;
}


void GyroController::setControlLoopHz(int value)
{
    int normalizedRate = value == 333 ? 333 : 250;

    // Runtime settings are refreshed in several places. Never disturb the
    // biquad history unless the actual sample rate changes.
    if(normalizedRate == huntNotchControlLoopHz)
    {
        return;
    }

    huntNotchControlLoopHz = normalizedRate;

    configureHuntNotch(
        constrain(
            huntNotchTrackingHz,
            HUNT_NOTCH_MIN_HZ,
            HUNT_NOTCH_MAX_HZ
        ),
        true
    );
}


void GyroController::configureHuntNotch(
    float centerHz,
    bool resetHistory
)
{
    centerHz = constrain(
        centerHz,
        HUNT_NOTCH_MIN_HZ,
        HUNT_NOTCH_MAX_HZ
    );

    huntNotchCenterHz = centerHz;

    float sampleRate =
        huntNotchControlLoopHz == 333 ? 333.0f : 250.0f;
    float omega =
        2.0f * 3.14159265358979323846f
        * huntNotchCenterHz
        / sampleRate;
    float alpha = sinf(omega) / (2.0f * HUNT_NOTCH_Q);
    float inverseA0 = 1.0f / (1.0f + alpha);

    huntNotchB0 = inverseA0;
    huntNotchB1 = -2.0f * cosf(omega) * inverseA0;
    huntNotchB2 = inverseA0;
    huntNotchA1 = huntNotchB1;
    huntNotchA2 = (1.0f - alpha) * inverseA0;

    if(resetHistory)
    {
        huntNotchReady = false;
    }
}


int GyroController::getControlLoopHz()
{
    return huntNotchControlLoopHz;
}


bool GyroController::isHuntNotchConfigured()
{
    return
        huntNotchControlLoopHz > 0 &&
        fabsf(huntNotchB0 - 1.0f) > 0.001f &&
        fabsf(huntNotchB1) > 0.1f &&
        huntNotchA2 > 0.0f &&
        huntNotchA2 < 1.0f;
}


float GyroController::getHuntNotchCenter()
{
    return huntNotchCenterHz;
}


float GyroController::getPredictedYaw()
{
    return predictedYawTelemetry;
}


float GyroController::getDriftReferenceYaw()
{
    return driftReferenceTelemetry;
}


float GyroController::getReferenceError()
{
    return referenceErrorTelemetry;
}


float GyroController::getReferenceLock()
{
    return referenceLockTelemetry;
}


float GyroController::getThrottlePrediction()
{
    return throttlePredictionTelemetry;
}


float GyroController::getDirectCorrection()
{
    return directCorrectionTelemetry;
}


float GyroController::getMemoryFeedback()
{
    return memoryFeedbackTelemetry;
}


float GyroController::getDriverActivityBlend()
{
    return driverActivityTelemetry;
}


float GyroController::getThrottlePredictionBlend()
{
    return throttlePredictionBlendTelemetry;
}


float GyroController::getThrottleLiftBlend()
{
    return throttleLiftBlendTelemetry;
}


float GyroController::getSteeringActivity()
{
    return steeringActivity;
}


float GyroController::getHuntSuppression()
{
    return huntSuppressionTelemetry;
}


float GyroController::getHuntFrequency()
{
    return huntFrequencyTelemetry;
}


float GyroController::getHuntResidual()
{
    return huntResidualTelemetry;
}


float GyroController::getHuntResidualEnvelope()
{
    return huntResidualEnvelopeTelemetry;
}


float GyroController::getHuntRemovedCorrection()
{
    return huntRemovedCorrectionTelemetry;
}


int GyroController::getHuntConsistentHalfCycles()
{
    return huntConsistentHalfCyclesTelemetry;
}


float GyroController::getHuntLatch()
{
    return huntLatchTelemetry;
}


float GyroController::getTransitionAuthorityBlend()
{
    return transitionAuthorityTelemetry;
}


float GyroController::getTransitionPredictionScale()
{
    return transitionPredictionScaleTelemetry;
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


float GyroController::getFilteredYaw()
{
    return filteredYaw;
}


void GyroController::setCurvePower(float power)
{
    curvePower = constrain(power, 1.0f, 4.0f);
}

float GyroController::getCurvePower()
{
    return curvePower;
}

void GyroController::setDamperPower(float power)
{
    damperPower = constrain(power, 0.0f, 5.0f);
}

float GyroController::getDamperPower()
{
    return damperPower;
}

void GyroController::setDamperPoint(float threshold)
{
    damperPoint = constrain(threshold, 0.0f, 1.0f);
}

float GyroController::getDamperPoint()
{
    return damperPoint;
}