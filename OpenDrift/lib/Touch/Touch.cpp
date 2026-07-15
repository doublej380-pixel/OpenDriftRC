#include "Touch.h"


#if defined(OPENDRIFT_BOARD_AMOLED_164)

static constexpr uint8_t FT3168_ADDR = 0x38;

Touch::Touch()
{
}


bool Touch::begin()
{
    Wire.setClock(
        300000
    );

    Wire.beginTransmission(
        FT3168_ADDR
    );

    Wire.write(
        0x00
    );

    Wire.write(
        0x00
    );

    touchOnline =
        Wire.endTransmission() == 0;

    return touchOnline;
}


static bool readTouchBytes(
    uint8_t reg,
    uint8_t* buffer,
    uint8_t length
)
{
    Wire.beginTransmission(
        FT3168_ADDR
    );

    Wire.write(
        reg
    );

    if(Wire.endTransmission(true) != 0)
    {
        return false;
    }

    delayMicroseconds(
        150
    );

    if(Wire.requestFrom(
        FT3168_ADDR,
        length
    ) != length)
    {
        return false;
    }

    for(uint8_t i = 0; i < length; i++)
    {
        buffer[i] =
            Wire.read();
    }

    return true;
}


void Touch::update()
{
    gesture = NONE;

    if(!touchOnline)
    {
        pressed = false;

        return;
    }

    uint8_t points = 0;

    if(!readTouchBytes(
        0x02,
        &points,
        1
    ))
    {
        touchReadFailures++;

        if(
            touchReadFailures >= 8 &&
            millis() - lastTouchErrorMs > 5000
        )
        {
            lastTouchErrorMs =
                millis();

            Serial.println("Touch read unstable");
        }

        if(millis() - lastEventMs > 80)
        {
            pressed = false;

            trackingTouch = false;
        }

        return;
    }

    touchReadFailures = 0;

    if((points & 0x0F) == 0)
    {
        if(millis() - lastEventMs > 80)
        {
            pressed = false;

            trackingTouch = false;

            gestureReported = false;
        }

        return;
    }

    uint8_t data[4];

    if(!readTouchBytes(
        0x03,
        data,
        sizeof(data)
    ))
    {
        touchReadFailures++;

        if(
            touchReadFailures >= 8 &&
            millis() - lastTouchErrorMs > 5000
        )
        {
            lastTouchErrorMs =
                millis();

            Serial.println("Touch coordinate read unstable");
        }

        return;
    }

    touchReadFailures = 0;

    uint16_t rawX =
        (((uint16_t)data[0] & 0x0F) << 8)
        |
        data[1];

    uint16_t rawY =
        (((uint16_t)data[2] & 0x0F) << 8)
        |
        data[3];

    rawX =
        constrain(
            rawX,
            0,
            TOUCH_WIDTH - 1
        );

    rawY =
        constrain(
            rawY,
            0,
            TOUCH_HEIGHT - 1
        );

    x =
        TOUCH_HEIGHT - 1 - rawY;

    y =
        rawX;

    if(!trackingTouch)
    {
        touchStartX = x;

        touchStartY = y;

        trackingTouch = true;

        gestureReported = false;
    }
    else if(!gestureReported)
    {
        int dx =
            (int)x - (int)touchStartX;

        int dy =
            (int)y - (int)touchStartY;

        if(
            abs(dx) > 45 &&
            abs(dx) > abs(dy)
        )
        {
            gesture =
                dx < 0
                ?
                SWIPE_LEFT
                :
                SWIPE_RIGHT;

            gestureReported = true;
        }
    }

    pressed = true;

    lastEventMs =
        millis();
}

#else

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

#endif




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
