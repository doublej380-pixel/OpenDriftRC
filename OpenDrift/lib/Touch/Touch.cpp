#include "Touch.h"


#if defined(OPENDRIFT_BOARD_AMOLED_164)

static constexpr uint8_t FT3168_ADDR = 0x38;
static constexpr unsigned long FT3168_RELEASE_HOLD_MS = 18;
static constexpr unsigned long FT3168_READ_FAILURE_HOLD_MS = 80;
static constexpr uint8_t FT3168_DISABLE_AFTER_FAILURES = 16;

Touch::Touch()
{
}


bool Touch::begin()
{
    touchReadFailures = 0;
    pressed = false;
    trackingTouch = false;

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


void Touch::setRotation(uint8_t rotation)
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    displayRotation = rotation == 2 ? 2 : 0;
    #else
    (void)rotation;
    #endif
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

    if(Wire.endTransmission(false) != 0)
    {
        return false;
    }

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

    uint8_t data[5];

    if(!readTouchBytes(
        0x02,
        data,
        sizeof(data)
    ))
    {
        if(touchReadFailures < UINT8_MAX)
        {
            touchReadFailures++;
        }

        if(
            touchReadFailures >= 8 &&
            millis() - lastTouchErrorMs > 5000
        )
        {
            lastTouchErrorMs =
                millis();

            Serial.println("Touch read unstable");
        }

        if(touchReadFailures >= FT3168_DISABLE_AFTER_FAILURES)
        {
            // A missing or wedged touch controller must not keep occupying the
            // IMU's shared I2C bus. A reboot performs the normal probe/retry
            // sequence again; CRSF and USB maintenance remain available.
            touchOnline = false;
            pressed = false;
            trackingTouch = false;
            Serial.println("Touch disabled after repeated I2C failures");
        }

        if(millis() - lastEventMs > FT3168_READ_FAILURE_HOLD_MS)
        {
            pressed = false;

            trackingTouch = false;
        }

        return;
    }

    touchReadFailures = 0;

    if((data[0] & 0x0F) == 0)
    {
        if(millis() - lastEventMs > FT3168_RELEASE_HOLD_MS)
        {
            pressed = false;

            trackingTouch = false;

            gestureReported = false;
        }

        return;
    }

    uint16_t rawX =
        (((uint16_t)data[1] & 0x0F) << 8)
        |
        data[2];

    uint16_t rawY =
        (((uint16_t)data[3] & 0x0F) << 8)
        |
        data[4];

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

    if(displayRotation == 2)
    {
        x = TOUCH_HEIGHT - 1 - x;
        y = TOUCH_WIDTH - 1 - y;
    }

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

    touchOnline = true;

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


bool Touch::isOnline() const
{
    return touchOnline;
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
