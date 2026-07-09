#pragma once

#include <Arduino.h>

#include "LGFX_OpenDrift.hpp"
#include "Touch.h"
#include "GyroController.h"
#include "IMU.h"


class UI
{

public:

    void begin(
        LGFX* display
    );


    void update(
        Touch& touch,
        GyroController& gyro,
        IMU& imu
    );


private:

    LGFX* lcd;


    bool lastTouchState = false;


    unsigned long lastPressTime = 0;



    void drawScreen(
        GyroController& gyro
    );



    bool buttonPressed(
        uint16_t x,
        uint16_t y,
        uint16_t bx,
        uint16_t by,
        uint16_t bw,
        uint16_t bh
    );

};