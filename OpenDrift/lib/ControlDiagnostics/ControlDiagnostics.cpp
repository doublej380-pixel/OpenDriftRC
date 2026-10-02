#include "ControlDiagnostics.h"
#include <esp_heap_caps.h>

bool ControlDiagnostics::start()
{
    portENTER_CRITICAL(&mux);
    const bool busy = exporting;
    portEXIT_CRITICAL(&mux);
    if(busy) return false;
    stop();
    if(records == nullptr)
    {
        for(size_t requested : {16000U, 8000U, 4000U, 2000U})
        {
            records = static_cast<Record*>(heap_caps_malloc(requested * sizeof(Record), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
            if(records != nullptr) { capacity = requested; break; }
        }
        if(records == nullptr)
        {
            capacity = 512;
            records = static_cast<Record*>(heap_caps_malloc(capacity * sizeof(Record), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        }
        if(records == nullptr) { capacity = 0; return false; }
    }
    portENTER_CRITICAL(&mux);
    used = 0;
    startedUs = micros();
    capturing = true;
    portEXIT_CRITICAL(&mux);
    return true;
}

bool ControlDiagnostics::beginExport()
{
    portENTER_CRITICAL(&mux);
    const bool available = !exporting && used != 0;
    if(available) { capturing = false; exporting = true; }
    portEXIT_CRITICAL(&mux);
    return available;
}

void ControlDiagnostics::endExport()
{
    portENTER_CRITICAL(&mux);
    exporting = false;
    portEXIT_CRITICAL(&mux);
}

void ControlDiagnostics::stop()
{
    portENTER_CRITICAL(&mux);
    capturing = false;
    portEXIT_CRITICAL(&mux);
}

void ControlDiagnostics::record(const Record& value)
{
    portENTER_CRITICAL(&mux);
    if(capturing)
    {
        if(used < capacity && value.timeUs - startedUs < 45000000UL)
            records[used++] = value;
        else capturing = false;
    }
    portEXIT_CRITICAL(&mux);
}

bool ControlDiagnostics::isCapturing() const
{
    portENTER_CRITICAL(&mux);
    const bool value = capturing;
    portEXIT_CRITICAL(&mux);
    return value;
}

size_t ControlDiagnostics::count() const
{
    portENTER_CRITICAL(&mux);
    const size_t value = used;
    portEXIT_CRITICAL(&mux);
    return value;
}

const char* ControlDiagnostics::header()
{
    return "time_us,interval_us,iteration_us,i2c_wait_us,imu_read_us,sample_counter,sample_delta,read_errors,no_data_count,i2c_misses,yaw_dps,gain,correction_us,servo_us,bus_acquired,read_ok,fresh,locked,steering_signal,applied_lpf_mode\n";
}

size_t ControlDiagnostics::format(size_t index, char* output, size_t length) const
{
    Record value;
    portENTER_CRITICAL(&mux);
    if(capturing || index >= used) { portEXIT_CRITICAL(&mux); return 0; }
    value = records[index];
    portEXIT_CRITICAL(&mux);
    const int result = snprintf(output, length,
        "%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%.5f,%.5f,%.5f,%.5f,%u,%u,%u,%u,%u,%lu\n",
        (unsigned long)value.timeUs, (unsigned long)value.intervalUs, (unsigned long)value.iterationUs,
        (unsigned long)value.busWaitUs, (unsigned long)value.readUs, (unsigned long)value.sampleCounter,
        (unsigned long)value.sampleDelta, (unsigned long)value.readErrors, (unsigned long)value.noDataCount,
        (unsigned long)value.busMisses, value.yaw, value.gain, value.correctionUs, value.servoUs,
        (value.flags & 1) != 0, (value.flags & 2) != 0, (value.flags & 4) != 0,
        (value.flags & 8) != 0, (value.flags & 16) != 0, (unsigned long)value.lpfMode);
    return result > 0 && (size_t)result < length ? result : 0;
}
