#pragma once

#include <Arduino.h>

#if defined(OPENDRIFT_USB_MAINTENANCE)
#include <FirmwareMSC.h>
#endif

#include "OnboardStorage.h"


class UsbMaintenance
{
public:
    enum UpdateState : uint8_t
    {
        UPDATE_IDLE = 0,
        UPDATE_WRITING = 1,
        UPDATE_COMPLETE = 2,
        UPDATE_ERROR = 3
    };

    static void armNextBoot();
    static bool consumeBootRequest();
    static bool consumeCompletedUpdate();

    bool begin(OnboardStorage& storage);
    void prepareForRestart(OnboardStorage& storage);
    bool firmwareDriveReady() const;
    bool sdDriveReady() const;
    UpdateState getUpdateState() const;
    size_t getUpdateBytes() const;
    uint32_t getUpdateRevision() const;

private:
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    FirmwareMSC firmwareDisk;
    #endif

    bool firmwareReady = false;
    bool sdReady = false;
};
