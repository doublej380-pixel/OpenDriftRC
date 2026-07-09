#include "Touch.h"



Touch::Touch()
:
touch(
    TOUCH_SDA,
    TOUCH_SCL,
    TOUCH_RST,
    TOUCH_INT
)
{

}




bool Touch::begin()
{
    touch.begin();

    return true;
}




void Touch::update()
{
    if(touch.available())
    {
        x = touch.data.x;

        y = touch.data.y;

        pressed = true;
    }
    else
    {
        pressed = false;
    }
}




bool Touch::isTouched()
{
    return pressed;
}




uint16_t Touch::getX()
{
    return x;
}




uint16_t Touch::getY()
{
    return y;
}