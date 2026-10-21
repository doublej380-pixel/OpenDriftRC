#include "Servo.h"
// Verify small command delivery and calibrated pulse clamps on the host.
#include <cassert>
#include <cstdio>
uint32_t hostTime=0;
int hostWrites=0;
int main() {
    ServoOutput output;
    assert(output.begin(15,333));
    output.configure(1500,false,100,50);
    int before=hostWrites;
    output.writeMicroseconds(1505);
    assert(hostWrites==before+1);
    assert(output.getPulseMicroseconds()==1505);
    output.configure(1500,false,100,50,true,1100,1500,1900);
    output.writeMicroseconds(3000);
    assert(output.getPulseMicroseconds()==1900);
    output.writeMicroseconds(0);
    assert(output.getPulseMicroseconds()==1100);
    std::puts("Servo suppression removal and physical clamp tests passed");
}
