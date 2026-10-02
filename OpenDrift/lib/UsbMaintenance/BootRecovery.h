#pragma once
#include <stdint.h>

// RTC state only: no flash writes during driving. Cold power loss starts a
// fresh audit; software resets/watchdogs retain unfinished startup attempts.
struct BootRecovery
{
    uint32_t magic;
    uint32_t failures;
    bool pending;
    static constexpr uint32_t MAGIC = 0x4F444252UL;

    bool begin(bool coldBoot)
    {
        if(coldBoot || magic != MAGIC || failures > 3)
        { magic = MAGIC; failures = 0; pending = false; }
        if(pending) ++failures;
        pending = false;
        if(failures >= 3) { healthy(); return true; }
        pending = true;
        return false;
    }
    void healthy() { magic = MAGIC; failures = 0; pending = false; }
};
