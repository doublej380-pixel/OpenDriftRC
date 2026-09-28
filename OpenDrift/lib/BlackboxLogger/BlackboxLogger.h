#pragma once

#include <Arduino.h>


class BlackboxLogger
{
public:

    bool begin();

    void log(
        unsigned long timeMs,
        float yaw,
        float filteredYaw,
        float gyroX,
        float gyroY,
        float accelX,
        float accelY,
        float accelZ,
        float accelMagnitude,
        float accelDelta,
        float tiltRate,
        float surfaceDisturbance,
        int requestedGyroCorrection,
        int limitedGyroCorrection,
        int appliedGyroCorrection,
        bool correctionSaturated,
        int steeringRaw,
        int steeringCommand,
        int servoCommand,
        int servoQuiet,
        int throttleRaw,
        int gainRaw,
        float gain,
        int driverPriority,
        float driverPriorityScale,
        float effectiveDirectGain,
        float deadband,
        int maxCorrection,
        float smoothing,
        int gyroLpfMode,
        float integralGain,
        int integralLimit,
        int integralCorrection,
        int holdBoost,
        int counterSteerAssist,
        int predictionStrength,
        float predictedYaw,
        float driftReferenceYaw,
        float referenceError,
        float referenceLock,
        float throttlePrediction,
        float directCorrection,
        int counterSteerCorrection,
        float memoryFeedback,
        float driverActivityBlend,
        float throttlePredictionBlend,
        float steeringActivity,
        int controlPhase,
        float settledBlend,
        float throttleTransient,
        bool steeringSignal,
        bool throttleSignal,
        bool gainSignal,
        bool throttleOutputMode,
        int transitionSpeed,
        float transitionSpeedBlend,
        float transitionSlewCorrection,
        float huntSuppression,
        float huntFrequency,
        float transitionAuthorityBlend,
        float throttleLiftBlend,
        float transitionPredictionScale,
        float huntResidual,
        float huntRemovedCorrection,
        int huntConsistentHalfCycles,
        float huntLatch,
        int huntStrength,
        float huntResidualEnvelope,
        float huntNotchCenter,
        int gyroOutputHysteresis,
        float preHysteresisCorrection,
        float postHysteresisCorrection
    );

    void clear();

    bool isReady() const;

    bool isFull() const;

    size_t getSize() const;

    size_t getCapacityBytes() const;

    size_t getRecordCount() const;

    size_t getOverwrittenRows() const;

    unsigned long getDurationMs() const;

    const char* getCsvHeader() const;

    size_t formatCsvRecord(
        size_t logicalIndex,
        char* output,
        size_t outputSize
    ) const;

    size_t getBinaryRecordSize() const;

    bool copyBinaryRecord(
        size_t logicalIndex,
        void* output,
        size_t outputSize
    ) const;

    size_t formatBinaryRecord(
        const void* record,
        size_t recordSize,
        char* output,
        size_t outputSize
    ) const;

    void setPaused(bool value);

    bool isPaused() const;

private:

    struct Record
    {
        uint32_t timeMs;
        float yaw;
        float filteredYaw;
        float gyroX;
        float gyroY;
        float accelX;
        float accelY;
        float accelZ;
        float accelMagnitude;
        float accelDelta;
        float tiltRate;
        float surfaceDisturbance;
        int32_t requestedGyroCorrection;
        int32_t limitedGyroCorrection;
        int32_t appliedGyroCorrection;
        int32_t steeringRaw;
        int32_t steeringCommand;
        int32_t servoCommand;
        int32_t servoQuiet;
        int32_t throttleRaw;
        int32_t gainRaw;
        float gain;
        int32_t driverPriority;
        float driverPriorityScale;
        float effectiveDirectGain;
        float deadband;
        int32_t maxCorrection;
        float smoothing;
        int32_t gyroLpfMode;
        float integralGain;
        int32_t integralLimit;
        int32_t integralCorrection;
        int32_t holdBoost;
        int32_t counterSteerAssist;
        int32_t predictionStrength;
        float predictedYaw;
        float driftReferenceYaw;
        float referenceError;
        float referenceLock;
        float throttlePrediction;
        float directCorrection;
        int32_t counterSteerCorrection;
        float memoryFeedback;
        float driverActivityBlend;
        float throttlePredictionBlend;
        float steeringActivity;
        int32_t controlPhase;
        float settledBlend;
        float throttleTransient;
        uint32_t signalFlags;
        int32_t transitionSpeed;
        float transitionSpeedBlend;
        float transitionSlewCorrection;
        float huntSuppression;
        float huntFrequency;
        float transitionAuthorityBlend;
        float throttleLiftBlend;
        float transitionPredictionScale;
        float huntResidual;
        float huntRemovedCorrection;
        int32_t huntConsistentHalfCycles;
        float huntLatch;
        int32_t huntStrength;
        float huntResidualEnvelope;
        float huntNotchCenter;
        int32_t gyroOutputHysteresis;
        float preHysteresisCorrection;
        float postHysteresisCorrection;
    };

    static const size_t preferredBufferBytes =
        4UL * 1024UL * 1024UL;

    static const size_t minimumBufferBytes =
        1UL * 1024UL * 1024UL;

    static const uint32_t steeringSignalFlag = 1UL << 0;
    static const uint32_t throttleSignalFlag = 1UL << 1;
    static const uint32_t gainSignalFlag = 1UL << 2;
    static const uint32_t throttleOutputFlag = 1UL << 3;
    static const uint32_t correctionSaturatedFlag = 1UL << 4;
    Record* records = nullptr;

    size_t capacity = 0;

    size_t writeIndex = 0;

    size_t recordCount = 0;

    size_t overwrittenRows = 0;

    bool ready = false;

    volatile bool paused = false;

    bool allocateBuffer();

    const Record* getRecord(
        size_t logicalIndex
    ) const;

    size_t formatRecord(
        const Record* record,
        char* output,
        size_t outputSize
    ) const;
};
