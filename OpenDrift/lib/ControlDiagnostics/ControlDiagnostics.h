#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

// A short, explicitly armed capture. Only fixed-size RAM writes happen in the
// control task; CSV formatting and network traffic happen after capture stops.
class ControlDiagnostics
{
public:
    struct Record
    {
        uint32_t timeUs, intervalUs, iterationUs, busWaitUs, readUs;
        uint32_t sampleCounter, sampleDelta, readErrors, noDataCount, busMisses;
        float yaw, gain, correctionUs, servoUs;
        uint32_t flags; // bit0 bus acquired, bit1 read OK, bit2 fresh, bit3 locked, bit4 signal
        uint32_t lpfMode;
    };
    bool start();
    void stop();
    bool beginExport();
    void endExport();
    void record(const Record& value);
    bool isCapturing() const;
    size_t count() const;
    size_t getCapacity() const { return capacity; }
    size_t format(size_t index, char* output, size_t length) const;
    static const char* header();

private:
    Record* records = nullptr;
    size_t capacity = 0;
    size_t used = 0;
    bool capturing = false;
    bool exporting = false;
    uint32_t startedUs = 0;
    mutable portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
};
