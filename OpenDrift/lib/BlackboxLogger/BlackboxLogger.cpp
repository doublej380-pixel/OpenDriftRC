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

    char line[256];

    snprintf(
        line,
        sizeof(line),
        "%lu,%.3f,%.3f,%d,%d,%d,%d,%d,%d,%.3f,%.2f,%d,%.3f,%d,%d,%d,%d\n",
        timeMs,
        yaw,
        filteredYaw,
        rawGyroCorrection,
        slewedGyroCorrection,
        steeringRaw,
        steeringCommand,
        servoCommand,
        gainRaw,
        gain,
        deadband,
        maxCorrection,
        smoothing,
        attack,
        returnSpeed,
        steeringSignal ? 1 : 0,
        gainSignal ? 1 : 0
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
        "time_ms,yaw,filtered_yaw,gyro_raw_us,gyro_slewed_us,steering_raw_us,steering_cmd_us,servo_us,gain_raw_us,gain,deadband,max_corr,smooth,attack,return,steering_signal,gain_signal"
    );

    file.close();

    lastFlush =
        millis();

    return true;
}
