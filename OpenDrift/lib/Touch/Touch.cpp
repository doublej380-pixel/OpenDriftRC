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

    touch.disable_auto_sleep();

    return true;
}




void Touch::update()
{
    gesture = NONE;

    if(touch.available())
    {
        x = touch.data.x;

        y = touch.data.y;

        gesture = touch.data.gestureID;

        lastEventMs =
            millis();

        pressed =
            touch.data.points > 0 &&
            touch.data.event != 1;
    }
    else
    {
        if(millis() - lastEventMs > 80)
        {
            pressed = false;
        }
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




uint8_t Touch::getGesture()
{
    return gesture;
}
