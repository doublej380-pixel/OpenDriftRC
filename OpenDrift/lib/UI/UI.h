#pragma once

#include <Arduino.h>

#include "LGFX_OpenDrift.hpp"
#include "Touch.h"
#include "GyroController.h"
#include "IMU.h"
#include "WiFiManager.h"
#include "Settings.h"


class UI
{

public:

    void begin(
        LGFX* display,
        GyroController& gyro,
        WiFiManager& wifi,
        Settings& settings
    );


    void update(
        Touch& touch,
        GyroController& gyro,
        IMU& imu,
        WiFiManager& wifi,
        Settings& settings
    );



private:

    LGFX* lcd = nullptr;



    // Pages
    // 0 = Main
    // 1 = Control
    // 2 = System
    // 3 = WiFi

    uint8_t page = 0;


    const uint8_t totalPages = 4;




    bool lastTouchState = false;


    int touchStartX = 0;


    bool trackingSwipe = false;





    void drawPage(
        GyroController& gyro,
        WiFiManager& wifi,
        Settings& settings
    );



    void drawMainPage(
        GyroController& gyro,
        Settings& settings
    );



    void drawControlPage(
        GyroController& gyro,
        Settings& settings
    );



    void drawSystemPage(
        Settings& settings
    );



    void drawWifiPage(
        WiFiManager& wifi,
        Settings& settings
    );



    void drawPageDots();



    bool buttonPressed(
        uint16_t x,
        uint16_t y,
        uint16_t bx,
        uint16_t by,
        uint16_t bw,
        uint16_t bh
    );

};