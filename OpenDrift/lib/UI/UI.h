#pragma once

#include <Arduino.h>

#include "LGFX_OpenDrift.hpp"
#include "Touch.h"
#include "GyroController.h"
#include "IMU.h"
#include "WiFiManager.h"
#include "Settings.h"
#include "RadioInput.h"


class UI
{

public:

    void begin(
        LGFX* display,
        GyroController& gyro,
        WiFiManager& wifi,
        Settings& settings,
        RadioInput& steeringRadio,
        RadioInput& gainRadio
    );


    void update(
        Touch& touch,
        GyroController& gyro,
        IMU& imu,
        WiFiManager& wifi,
        Settings& settings,
        RadioInput& steeringRadio,
        RadioInput& gainRadio
    );



private:

    LGFX* lcd = nullptr;



    // Pages
    // 0 = Main
    // 1 = Control
    // 2 = System
    // 3 = Radio
    // 4 = Steering
    // 5 = WiFi

    uint8_t page = 0;


    const uint8_t totalPages = 6;




    bool lastTouchState = false;


    int touchStartX = 0;

    int touchStartY = 0;

    bool trackingSwipe = false;

    unsigned long lastRadioRefresh = 0;

    uint8_t radioSection = 0;





    void drawPage(
        GyroController& gyro,
        WiFiManager& wifi,
        Settings& settings,
        RadioInput& steeringRadio,
        RadioInput& gainRadio
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

    void drawRadioPage(
        RadioInput& steeringRadio,
        RadioInput& gainRadio,
        Settings& settings,
        GyroController& gyro
    );

    void updateRadioPage(
        RadioInput& steeringRadio,
        RadioInput& gainRadio,
        Settings& settings,
        GyroController& gyro
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
