#include "DriveArming.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
int main()
{
    DriveArming a;
    assert(!a.update(0,true,2000,1500,true,true)); // High at boot cannot arm.
    assert(!a.wantsDrive());
    assert(!a.update(1,true,1000,1500,true,true));
    assert(!a.update(2,true,2000,1800,true,true));
    assert(a.wantsDrive());
    assert(!a.update(1000,true,2000,1500,true,false)); // UI not asleep.
    assert(!a.update(1001,true,2000,1500,true,true));
    assert(!a.update(1500,true,2000,1500,true,true));
    assert(a.update(1501,true,2000,1500,true,true));
    assert(a.update(1502,true,2000,1900,true,true)); // Throttle can drive now.
    assert(!a.update(1503,true,1000,1900,true,true)); // Off immediately parks.
    assert(!a.isArmed() && !a.wantsDrive());
    assert(!a.update(1504,true,2000,1500,true,true));
    assert(a.update(2004,true,2000,1500,true,true));
    assert(!a.update(2005,false,2000,1500,true,true)); // Link lost.
    assert(!a.update(3000,true,2000,1500,true,true));
    assert(!a.wantsDrive()); // Reconnect high must not arm.
    a.update(3001,true,1000,1500,true,true);
    a.update(3002,true,2000,1500,true,true);
    assert(a.update(3502,true,2000,1500,true,true));
    assert(!a.update(3503,true,2000,1500,false,true)); // IMU unhealthy.
    assert(!a.update(5000,true,2000,1500,true,true));
    a.update(5001,true,1000,1500,true,true);
    a.update(5002,true,2000,1500,true,true);
    assert(a.update(5502,true,2000,1500,true,true));
    assert(!a.update(5503,true,1500,1500,true,true)); // Ambiguous switch.
    assert(!a.update(7000,true,2000,1500,true,true));
    a.update(7001,true,1000,1500,true,true);
    a.update(7002,true,2000,1500,true,true);
    assert(a.update(7502,true,2000,1500,true,true));
    assert(!a.update(7503,true,2000,1500,true,false)); // Peripheral gate revoked.
    assert(!a.update(9000,true,2000,1500,true,true));
    DriveArming wrap;
    uint32_t t=UINT32_MAX-200;
    wrap.update(t,true,1000,1500,true,true);
    wrap.update(t+1,true,2000,1500,true,true);
    assert(wrap.update(t+501,true,2000,1500,true,true));

    // ELRS/radio interpolation can produce intermediate pulses even with a
    // two-position physical switch. An off -> middle -> high sweep must work.
    DriveArming ramp;
    ramp.update(0,true,1000,1500,true,false);
    for(int pulse=1320;pulse<1700;pulse+=40)
    {
        assert(!ramp.update(pulse,true,pulse,1500,true,false));
        assert(!ramp.wantsDrive());
    }
    ramp.update(1800,true,2000,1500,true,false);
    assert(ramp.wantsDrive());
    assert(!ramp.update(1801,true,2000,1500,true,true));
    assert(ramp.update(2301,true,2000,1500,true,true));
    // But a middle position reached from ARMED must park and revoke permission.
    assert(!ramp.update(2302,true,1600,1500,true,true));
    assert(!ramp.update(3000,true,2000,1500,true,true));
    assert(!ramp.wantsDrive());
    ramp.update(3001,true,1000,1500,true,true);
    ramp.update(3002,true,1450,1500,true,true);
    ramp.update(3003,true,2000,1500,true,true);
    assert(ramp.update(3503,true,2000,1500,true,true));
    std::puts("CH4 arming interlock tests passed");
}
