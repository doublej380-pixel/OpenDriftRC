#pragma once
// Minimal output-call stub; this does not simulate electrical PWM timing.
#include <cmath>
#include <cstdlib>
extern int hostWrites;
class Servo {
    int width=14;
public:
    void setPeriodHertz(int) {}
    int attach(int,int,int) { return 1; }
    void detach() {}
    void setTimerWidth(int n) { width=n; }
    int readTimerWidth() { return width; }
    void writeTicks(int) { ++hostWrites; }
};
