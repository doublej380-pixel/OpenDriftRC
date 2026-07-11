#pragma once

#include <Arduino.h>
#include <CST816S.h>


#define TOUCH_SDA 6
#define TOUCH_SCL 7
#define TOUCH_RST 13
#define TOUCH_INT 5



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

    CST816S touch;


    uint16_t x = 0;

    uint16_t y = 0;


    bool pressed = false;

    uint8_t gesture = 0;
};
