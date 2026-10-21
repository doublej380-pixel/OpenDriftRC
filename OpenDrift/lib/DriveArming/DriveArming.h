#pragma once
#include <stdint.h>

#if defined(OPENDRIFT_INPUT_CRSF) && !defined(OPENDRIFT_IMU_MPU6050)
#define OPENDRIFT_CH4_ARMING
#endif

// Control-task-owned state. No dynamic allocation, peripheral calls, or blocking.
class DriveArming
{
public:
    enum Reason : uint8_t {
        NO_LINK, SENSOR_NOT_READY, PARKED, SWITCH_IN_BETWEEN, NEED_SWITCH_OFF,
        WAIT_PERIPHERALS, WAIT_NEUTRAL, NEUTRAL_HOLD, ARMED
    };
    Reason getReason() const { return reason; }
    static const char* reasonName(Reason value)
    {
        switch(value)
        {
            case NO_LINK: return "no receiver link";
            case SENSOR_NOT_READY: return "sensor/calibration not ready";
            case PARKED: return "parked: switch off";
            case SWITCH_IN_BETWEEN: return "switch between thresholds";
            case NEED_SWITCH_OFF: return "move switch off first";
            case WAIT_PERIPHERALS: return "waiting for UI/storage";
            case WAIT_NEUTRAL: return "throttle not neutral";
            case NEUTRAL_HOLD: return "holding neutral";
            case ARMED: return "armed";
        }
        return "unknown";
    }
    bool update(uint32_t nowMs, bool link, int ch4Us, int throttleUs,
                bool healthy, bool peripheralsReady)
    {
        if(!link || !healthy)
        {
            reason = !link ? NO_LINK : SENSOR_NOT_READY;
            offSeen = false;
            requested = armed = timingNeutral = false;
            return false;
        }
        if(ch4Us <= 1300)
        {
            reason = PARKED;
            offSeen = true;
            requested = armed = timingNeutral = false;
            return false;
        }
        if(ch4Us < 1700)
        {
            // A middle pulse never permits driving. Preserve an observed OFF
            // while sweeping toward ON, but revoke it when leaving ARMED.
            if(armed) offSeen = false;
            reason = SWITCH_IN_BETWEEN;
            requested = armed = timingNeutral = false;
            return false;
        }
        if(!offSeen)
        {
            reason = NEED_SWITCH_OFF;
            requested = armed = timingNeutral = false;
            return false;
        }
        requested = true;
        if(armed)
        {
            reason = ARMED;
            if(!peripheralsReady)
            {
                reason = WAIT_PERIPHERALS;
                offSeen = false;
                requested = armed = timingNeutral = false;
            }
            return armed;
        }
        if(!peripheralsReady || throttleUs < 1470 || throttleUs > 1530)
        {
            reason = !peripheralsReady ? WAIT_PERIPHERALS : WAIT_NEUTRAL;
            timingNeutral = false;
            return false;
        }
        if(!timingNeutral)
        {
            neutralSince = nowMs;
            timingNeutral = true;
        }
        armed = uint32_t(nowMs - neutralSince) >= 500;
        reason = armed ? ARMED : NEUTRAL_HOLD;
        return armed;
    }
    bool wantsDrive() const { return requested; }
    bool isArmed() const { return armed; }
private:
    Reason reason = NO_LINK;
    bool offSeen = false, requested = false, armed = false, timingNeutral = false;
    uint32_t neutralSince = 0;
};
