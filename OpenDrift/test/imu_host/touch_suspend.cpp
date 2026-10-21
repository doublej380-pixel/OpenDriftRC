#include "Touch.h"
#include <cassert>
#include <cstdio>
uint32_t hostTime=100000;
FakeWire Wire;
int main()
{
    Touch touch;
    assert(touch.begin());
    Wire.registers[2]=1;
    Wire.registers[3]=0; Wire.registers[4]=100;
    Wire.registers[5]=0; Wire.registers[6]=150;
    touch.update();
    assert(touch.isTouched());
    size_t reads=Wire.reads.size();
    touch.suspend(true);
    for(int i=0;i<100;i++) touch.update();
    assert(!touch.isTouched() && touch.getGesture()==0);
    assert(Wire.reads.size()==reads); // No I2C traffic while suspended.
    touch.suspend(false);
    touch.update();
    assert(touch.isTouched());
    assert(Wire.reads.size()>reads);
    std::puts("Touch suspend/resume tests passed");
}
