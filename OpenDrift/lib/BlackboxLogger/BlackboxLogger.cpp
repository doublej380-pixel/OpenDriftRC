#include "BlackboxLogger.h"

#include <esp_heap_caps.h>


namespace
{
    const char* BLACKBOX_HEADER =
        "time_ms,yaw,filtered_yaw,gyro_x_dps,gyro_y_dps,accel_x_g,accel_y_g,accel_z_g,accel_mag_g,accel_delta_g,tilt_rate_dps,surface_disturbance,gyro_raw_us,gyro_correction_us,steering_raw_us,steering_cmd_us,servo_us,servo_quiet,throttle_raw_us,gain_raw_us,gain,deadband,max_corr,smooth,drift_memory,memory_limit,memory_feedback_us,hold_assist,countersteer_assist,prediction_strength,predicted_yaw,drift_reference_yaw,reference_error,reference_lock,throttle_prediction,direct_correction_us,countersteer_us,memory_feedback_copy_us,driver_activity_blend,throttle_prediction_blend,steering_activity_us_s,control_phase,settled_blend,throttle_transient,steering_signal,throttle_signal,gain_signal,pin18_throttle_out,tail_slide_speed,tail_slide_blend";
}


bool BlackboxLogger::begin()
{
    if(ready)
    {
        return true;
    }

    ready = allocateBuffer();

    if(ready)
    {
        clear();
    }

    return ready;
}


void BlackboxLogger::log(
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
    int rawGyroCorrection,
    int slewedGyroCorrection,
    int steeringRaw,
    int steeringCommand,
    int servoCommand,
    int servoQuiet,
    int throttleRaw,
    int gainRaw,
    float gain,
    float deadband,
    int maxCorrection,
    float smoothing,
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
    int tailSlideSpeed,
    float tailSlideBlend
)
{
    if(!ready || capacity == 0)
    {
        return;
    }

    uint32_t signalFlags = 0;

    if(steeringSignal)
    {
        signalFlags |= steeringSignalFlag;
    }

    if(throttleSignal)
    {
        signalFlags |= throttleSignalFlag;
    }

    if(gainSignal)
    {
        signalFlags |= gainSignalFlag;
    }

    if(throttleOutputMode)
    {
        signalFlags |= throttleOutputFlag;
    }

    records[writeIndex] = {
        (uint32_t)timeMs,
        yaw,
        filteredYaw,
        gyroX,
        gyroY,
        accelX,
        accelY,
        accelZ,
        accelMagnitude,
        accelDelta,
        tiltRate,
        surfaceDisturbance,
        rawGyroCorrection,
        slewedGyroCorrection,
        steeringRaw,
        steeringCommand,
        servoCommand,
        servoQuiet,
        throttleRaw,
        gainRaw,
        gain,
        deadband,
        maxCorrection,
        smoothing,
        integralGain,
        integralLimit,
        integralCorrection,
        holdBoost,
        counterSteerAssist,
        predictionStrength,
        predictedYaw,
        driftReferenceYaw,
        referenceError,
        referenceLock,
        throttlePrediction,
        directCorrection,
        counterSteerCorrection,
        memoryFeedback,
        driverActivityBlend,
        throttlePredictionBlend,
        steeringActivity,
        controlPhase,
        settledBlend,
        throttleTransient,
        signalFlags,
        tailSlideSpeed,
        tailSlideBlend
    };

    writeIndex =
        (writeIndex + 1) % capacity;

    if(recordCount < capacity)
    {
        recordCount++;
    }
    else
    {
        overwrittenRows++;
    }
}


void BlackboxLogger::clear()
{
    writeIndex = 0;
    recordCount = 0;
    overwrittenRows = 0;
}


bool BlackboxLogger::isReady() const
{
    return ready;
}


bool BlackboxLogger::isFull() const
{
    return ready && recordCount == capacity;
}


size_t BlackboxLogger::getSize() const
{
    return recordCount * sizeof(Record);
}


size_t BlackboxLogger::getCapacityBytes() const
{
    return capacity * sizeof(Record);
}


size_t BlackboxLogger::getRecordCount() const
{
    return recordCount;
}


size_t BlackboxLogger::getOverwrittenRows() const
{
    return overwrittenRows;
}


unsigned long BlackboxLogger::getDurationMs() const
{
    if(recordCount < 2)
    {
        return 0;
    }

    const Record* oldest = getRecord(0);
    const Record* newest =
        getRecord(recordCount - 1);

    if(oldest == nullptr || newest == nullptr)
    {
        return 0;
    }

    return newest->timeMs - oldest->timeMs;
}


const char* BlackboxLogger::getCsvHeader() const
{
    return BLACKBOX_HEADER;
}


size_t BlackboxLogger::formatCsvRecord(
    size_t logicalIndex,
    char* output,
    size_t outputSize
) const
{
    if(output == nullptr || outputSize == 0)
    {
        return 0;
    }

    const Record* record =
        getRecord(logicalIndex);

    if(record == nullptr)
    {
        output[0] = '\0';
        return 0;
    }

    int formatted = snprintf(
        output,
        outputSize,
        "%lu,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%.4f,%.4f,%.3f,%.3f,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%.3f,%.2f,%ld,%.3f,%.3f,%ld,%ld,%ld,%ld,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%ld,%.3f,%.3f,%.3f,%.3f,%ld,%.3f,%.3f,%d,%d,%d,%d,%ld,%.3f\n",
        (unsigned long)record->timeMs,
        record->yaw,
        record->filteredYaw,
        record->gyroX,
        record->gyroY,
        record->accelX,
        record->accelY,
        record->accelZ,
        record->accelMagnitude,
        record->accelDelta,
        record->tiltRate,
        record->surfaceDisturbance,
        (long)record->rawGyroCorrection,
        (long)record->slewedGyroCorrection,
        (long)record->steeringRaw,
        (long)record->steeringCommand,
        (long)record->servoCommand,
        (long)record->servoQuiet,
        (long)record->throttleRaw,
        (long)record->gainRaw,
        record->gain,
        record->deadband,
        (long)record->maxCorrection,
        record->smoothing,
        record->integralGain,
        (long)record->integralLimit,
        (long)record->integralCorrection,
        (long)record->holdBoost,
        (long)record->counterSteerAssist,
        (long)record->predictionStrength,
        record->predictedYaw,
        record->driftReferenceYaw,
        record->referenceError,
        record->referenceLock,
        record->throttlePrediction,
        record->directCorrection,
        (long)record->counterSteerCorrection,
        record->memoryFeedback,
        record->driverActivityBlend,
        record->throttlePredictionBlend,
        record->steeringActivity,
        (long)record->controlPhase,
        record->settledBlend,
        record->throttleTransient,
        (record->signalFlags & steeringSignalFlag) ? 1 : 0,
        (record->signalFlags & throttleSignalFlag) ? 1 : 0,
        (record->signalFlags & gainSignalFlag) ? 1 : 0,
        (record->signalFlags & throttleOutputFlag) ? 1 : 0,
        (long)record->tailSlideSpeed,
        record->tailSlideBlend
    );

    if(formatted <= 0)
    {
        output[0] = '\0';
        return 0;
    }

    return min(
        (size_t)formatted,
        outputSize - 1
    );
}


bool BlackboxLogger::allocateBuffer()
{
    if(records != nullptr)
    {
        return true;
    }

    size_t requestedBytes =
        preferredBufferBytes;

    while(requestedBytes >= minimumBufferBytes)
    {
        records = static_cast<Record*>(
            heap_caps_malloc(
                requestedBytes,
                MALLOC_CAP_SPIRAM |
                MALLOC_CAP_8BIT
            )
        );

        if(records != nullptr)
        {
            capacity =
                requestedBytes /
                sizeof(Record);

            return capacity > 0;
        }

        requestedBytes -=
            1UL * 1024UL * 1024UL;
    }

    return false;
}


const BlackboxLogger::Record* BlackboxLogger::getRecord(
    size_t logicalIndex
) const
{
    if(
        !ready ||
        logicalIndex >= recordCount ||
        capacity == 0
    )
    {
        return nullptr;
    }

    size_t oldestIndex =
        (writeIndex + capacity - recordCount) %
        capacity;

    size_t physicalIndex =
        (oldestIndex + logicalIndex) %
        capacity;

    return &records[physicalIndex];
}
