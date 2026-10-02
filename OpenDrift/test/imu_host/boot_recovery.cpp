#include "BootRecovery.h"
#include <cassert>
#include <cstdio>
int main()
{
    BootRecovery state = {};
    assert(!state.begin(true)); // Initial boot.
    assert(!state.begin(false)); // First failed boot.
    assert(!state.begin(false)); // Second failed boot.
    assert(state.begin(false)); // Third failure enters recovery on next boot.
    assert(!state.pending && state.failures == 0);
    assert(!state.begin(false)); // Restart from recovery gets another chance.
    state.healthy();
    assert(!state.begin(false)); // Intentional restart is not a failure.
    assert(state.failures == 0);
    assert(!state.begin(true)); // Power interruption resets the audit.
    assert(state.failures == 0);
    std::puts("Boot recovery tests passed");
}
