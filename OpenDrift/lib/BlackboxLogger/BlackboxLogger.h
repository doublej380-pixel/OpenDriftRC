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
        int gainRaw,
        float gain,
        float deadband,
        int maxCorrection,
        float smoothing,
        int attack,
        int returnSpeed,
        bool steeringSignal,
        bool gainSignal
    );

    bool clear();

    bool flush();

    bool isReady();

    bool isFull();

    size_t getSize();

    const char* getPath();

private:

    static const size_t maxLogSize = 3UL * 1024UL * 1024UL;

    static const size_t maxBufferSize = 512UL * 1024UL;

    static const unsigned long flushIntervalMs = 1000;

    const char* path = "/blackbox.csv";

    String buffer;

    bool ready = false;

    bool full = false;

    unsigned long lastFlush = 0;

    bool writeHeader();
};
