#include "ControlDiagnostics.h"
#include <cassert>
#include <cstring>
uint32_t hostTime = 1000;
int main()
{
    ControlDiagnostics capture;
    assert(!capture.beginExport());
    assert(capture.start());
    ControlDiagnostics::Record row = {};
    row.timeUs = hostTime;
    row.yaw = 42;
    capture.record(row);
    assert(capture.count() == 1);
    assert(capture.beginExport());
    assert(!capture.isCapturing());
    assert(!capture.start()); // SD save cannot have its snapshot replaced.
    assert(!capture.beginExport());
    capture.record(row);
    assert(capture.count() == 1);
    char output[320];
    assert(capture.format(0, output, sizeof(output)) != 0);
    assert(std::strstr(output, "42.00000") != nullptr);
    capture.endExport();
    assert(capture.start());
    assert(capture.count() == 0);
    row.timeUs = hostTime + 45000000UL;
    capture.record(row);
    assert(!capture.isCapturing());
    std::puts("Diagnostic export tests passed");
}
