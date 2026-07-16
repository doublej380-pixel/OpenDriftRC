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
        int antiWobble,
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

    const char* path = "/blackbox-v2.csv";

    String buffer;

    bool ready = false;

    bool full = false;

    unsigned long lastFlush = 0;

    bool writeHeader();
};
