#pragma once

#include <Arduino.h>

#if defined(OPENDRIFT_BOARD_AMOLED_164)
#include <Wire.h>

enum GESTURE {
  NONE = 0x00,
  SWIPE_UP = 0x01,
  SWIPE_DOWN = 0x02,
  SWIPE_LEFT = 0x03,
  SWIPE_RIGHT = 0x04,
};

#else
#include <CST816S.h>
#endif


#if defined(OPENDRIFT_BOARD_AMOLED_164)
#define TOUCH_SDA 47
#define TOUCH_SCL 48
#define TOUCH_RST -1
#define TOUCH_INT -1
#define TOUCH_WIDTH 280
#define TOUCH_HEIGHT 456
#else
#define TOUCH_SDA 6
#define TOUCH_SCL 7
#define TOUCH_RST 13
#define TOUCH_INT 5
#define TOUCH_WIDTH 240
#define TOUCH_HEIGHT 240
#endif



class Touch
{
public:

    Touch();


    bool begin();

    void update();


    bool isTouched();


    uint16_t getX();

    uint16_t getY();

    uint8_t getGesture();



private:

    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    CST816S touch;
    #endif


    uint16_t x = 0;

    uint16_t y = 0;

    uint16_t touchStartX = 0;

    uint16_t touchStartY = 0;

    bool trackingTouch = false;

    bool gestureReported = false;

    bool touchOnline = false;

    unsigned long lastTouchErrorMs = 0;

    uint8_t touchReadFailures = 0;


    bool pressed = false;

    uint8_t gesture = 0;

    unsigned long lastEventMs = 0;
};
