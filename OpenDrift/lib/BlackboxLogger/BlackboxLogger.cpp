#include "BlackboxLogger.h"

#include <FFat.h>


bool BlackboxLogger::begin()
{
    ready =
        FFat.begin(true);

    if(!ready)
    {
        return false;
    }

    buffer.reserve(4096);

    File file =
        FFat.open(path, FILE_READ);

    bool needsHeader =
        !file ||
        file.size() == 0;

    if(file)
    {
        full =
            file.size() >= maxLogSize;

        file.close();
    }

    if(needsHeader)
    {
        return writeHeader();
    }

    return true;
}



void BlackboxLogger::update(
    bool allowFlush
)
{
    if(
        allowFlush &&
        ready &&
        buffer.length() > 0 &&
        millis() - lastFlush >= flushIntervalMs
    )
    {
        flush();
    }
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
    int antiWobble,
    int huntDamping,
    float huntControlYaw,
    float huntSlowYaw,
    float huntFastYaw,
    float huntBlend,
    float huntScore,
    float outputChatterSlow,
    int counterSteerCorrection,
    float outputChatterFast,
    float outputChatterBlend,
    float outputChatterScore,
    float steeringActivity,
    int controlPhase,
    float settledBlend,
    float throttleTransient,
    bool terrainActive,
    float terrainAssist,
    bool terrainAssistEnabled,
    float holdFactor,
    int attack,
    int returnSpeed,
    bool steeringSignal,
    bool throttleSignal,
    bool gainSignal,
    bool throttleOutputMode,
    int tailSlideSpeed,
    float tailSlideBlend
)
{
    if(
        !ready ||
        full
    )
    {
        return;
    }

    if(buffer.length() >= maxBufferSize)
    {
        full = true;

        return;
    }

    char line[672];

    snprintf(
        line,
        sizeof(line),
        "%lu,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%.4f,%.4f,%.3f,%.3f,%d,%d,%d,%d,%d,%d,%d,%d,%.3f,%.2f,%d,%.3f,%.3f,%d,%d,%d,%d,%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%.3f,%.3f,%.3f,%.3f,%d,%.3f,%.3f,%d,%.3f,%d,%.3f,%d,%d,%d,%d,%d,%d,%d,%.3f\n",
        timeMs,
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
        antiWobble,
        huntDamping,
        huntControlYaw,
        huntSlowYaw,
        huntFastYaw,
        huntBlend,
        huntScore,
        outputChatterSlow,
        counterSteerCorrection,
        outputChatterFast,
        outputChatterBlend,
        outputChatterScore,
        steeringActivity,
        controlPhase,
        settledBlend,
        throttleTransient,
        terrainActive ? 1 : 0,
        terrainAssist,
        terrainAssistEnabled ? 1 : 0,
        holdFactor,
        attack,
        returnSpeed,
        steeringSignal ? 1 : 0,
        throttleSignal ? 1 : 0,
        gainSignal ? 1 : 0,
        throttleOutputMode ? 1 : 0,
        tailSlideSpeed,
        tailSlideBlend
    );

    buffer += line;
}



bool BlackboxLogger::clear()
{
    if(!ready)
    {
        return false;
    }

    buffer = "";

    full = false;

    FFat.remove(path);

    return writeHeader();
}



bool BlackboxLogger::flush()
{
    if(
        !ready ||
        buffer.length() == 0
    )
    {
        return ready;
    }

    size_t currentSize =
        getSize();

    if(currentSize + buffer.length() >= maxLogSize)
    {
        full = true;

        buffer = "";

        return false;
    }

    File file =
        FFat.open(path, FILE_APPEND);

    if(!file)
    {
        return false;
    }

    file.print(buffer);

    file.close();

    buffer = "";

    full = false;

    lastFlush =
        millis();

    return true;
}



bool BlackboxLogger::isReady()
{
    return ready;
}



bool BlackboxLogger::isFull()
{
    return full;
}



size_t BlackboxLogger::getSize()
{
    if(!ready)
    {
        return 0;
    }

    File file =
        FFat.open(path, FILE_READ);

    if(!file)
    {
        return 0;
    }

    size_t size =
        file.size()
        +
        buffer.length();

    file.close();

    return size;
}



const char* BlackboxLogger::getPath()
{
    return path;
}



bool BlackboxLogger::writeHeader()
{
    File file =
        FFat.open(path, FILE_WRITE);

    if(!file)
    {
        return false;
    }

    file.println(
        "time_ms,yaw,filtered_yaw,gyro_x_dps,gyro_y_dps,accel_x_g,accel_y_g,accel_z_g,accel_mag_g,accel_delta_g,tilt_rate_dps,surface_disturbance,gyro_raw_us,gyro_v2_us,steering_raw_us,steering_cmd_us,servo_us,servo_quiet,throttle_raw_us,gain_raw_us,gain,deadband,max_corr,smooth,drift_memory,memory_limit,memory_feedback_us,hold_assist,countersteer_assist,anti_wobble_legacy,prediction_strength,predicted_yaw,drift_reference_yaw,reference_error,reference_lock,throttle_prediction,direct_correction_us,countersteer_us,memory_feedback_copy_us,driver_activity_blend,throttle_prediction_blend,steering_activity_us_s,control_phase,settled_blend,throttle_transient,terrain_active_legacy,terrain_assist_legacy,terrain_enabled_legacy,hold_factor_legacy,attack_legacy,return_legacy,steering_signal,throttle_signal,gain_signal,pin18_throttle_out,tail_slide_speed,tail_slide_blend"
    );

    file.close();

    lastFlush =
        millis();

    return true;
}
