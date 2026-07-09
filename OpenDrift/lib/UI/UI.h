#pragma once

#include <Arduino.h>

#include "LGFX_OpenDrift.hpp"
#include "Touch.h"
#include "GyroController.h"
#include "IMU.h"
#include "WiFiManager.h"


class UI
{

public:

    void begin(
        LGFX* display
    );


    void update(
        Touch& touch,
        GyroController& gyro,
        IMU& imu,
        WiFiManager& wifi
    );


private:

    LGFX* lcd;


    uint8_t page = 0;


    bool lastTouchState = false;


    unsigned long lastPressTime = 0;


    int touchStartX = 0;


    bool trackingSwipe = false;



    void drawPage(
        GyroController& gyro,
        WiFiManager& wifi
    );


    void drawMainPage(
        GyroController& gyro
    );


    void drawWifiPage(
        WiFiManager& wifi
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