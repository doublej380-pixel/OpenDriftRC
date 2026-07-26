#pragma once

#include <Arduino.h>


class BlackboxLogger
{
public:

    bool begin();

    void update(
        bool allowFlush
    );

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
        bool throttleOutputMode
    );

    bool clear();

    bool flush();

    bool isReady();

    bool isFull();

    size_t getSize();

    const char* getPath();

private:

    static const size_t maxLogSize = 3UL * 1024UL * 1024UL;

    static const size_t maxBufferSize = 128UL * 1024UL;

    static const unsigned long flushIntervalMs = 1000;

    const char* path = "/blackbox-v10.csv";

    String buffer;

    bool ready = false;

    bool full = false;

    unsigned long lastFlush = 0;

    bool writeHeader();
};
