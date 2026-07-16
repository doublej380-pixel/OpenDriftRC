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

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    static constexpr int UI_CANVAS_WIDTH = 456;
    static constexpr int UI_CANVAS_HEIGHT = 280;
    static constexpr int UI_CENTER_X = 228;
    static constexpr int UI_FOOTER_Y = 254;
    static constexpr int UI_DOTS_Y = 260;
    #else
    static constexpr int UI_CANVAS_WIDTH = 240;
    static constexpr int UI_CANVAS_HEIGHT = 240;
    static constexpr int UI_CENTER_X = 120;
    static constexpr int UI_FOOTER_Y = 230;
    static constexpr int UI_DOTS_Y = 215;
    #endif

    LGFX* display = nullptr;

    LGFX_Sprite canvas;

    LGFX_Sprite transitionCanvas;

    bool transitionCanvasReady = false;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    LGFX_Sprite panelCanvas;
    bool panelCanvasReady = false;
    #endif

    LGFX_Sprite* lcd = nullptr;

    bool canvasReady = false;

    bool suppressFlush = false;

    void flushDisplay();

    void flushDisplay(
        int16_t xOffset
    );

    void flushTransitionDisplay(
        int16_t xOffset,
        int8_t direction
    );

    void drawFixedPageDots();

    bool canUseRawAmoledBuffers();

    // Pages
    // 0 = Main
    // 1 = Gyro
    // 2 = Tune
    // 3 = Response
    // 4 = Radio
    // 5 = Steering
    // AMOLED: 6 = WiFi, 7 = System
    // Round: 6 = Steering Cal, 7 = WiFi, 8 = System

    uint8_t page = 0;


    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    const uint8_t totalPages = 8;
    #else
    const uint8_t totalPages = 9;
    #endif




    bool lastTouchState = false;


    int touchStartX = 0;

    int touchStartY = 0;

    bool trackingSwipe = false;

    unsigned long lastRadioRefresh = 0;

    unsigned long lastPageSwipe = 0;

    uint8_t radioSection = 0;

    int8_t heldRepeatButton = 0;

    unsigned long nextRepeatAt = 0;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    bool swipePreviewActive = false;

    int8_t swipePreviewDirection = 0;

    uint8_t swipePreviewSourcePage = 0;

    uint8_t swipePreviewSourceRadioSection = 0;

    int16_t swipePreviewOffset = 0;

    unsigned long lastSwipePreviewAt = 0;
    #endif





    void drawPage(
        GyroController& gyro,
        WiFiManager& wifi,
        Settings& settings,
        RadioInput& steeringRadio,
        RadioInput& gainRadio
    );

    void changePage(
        int8_t direction,
        GyroController& gyro,
        WiFiManager& wifi,
        Settings& settings,
        RadioInput& steeringRadio,
        RadioInput& gainRadio
    );

    bool prepareSwipePreview(
        int8_t direction,
        GyroController& gyro,
        WiFiManager& wifi,
        Settings& settings,
        RadioInput& steeringRadio,
        RadioInput& gainRadio
    );

    void finishSwipePreview(
        bool commit
    );



    void drawMainPage(
        GyroController& gyro,
        Settings& settings
    );



    void drawControlPage(
        GyroController& gyro,
        Settings& settings
    );

    void drawTunePage(
        Settings& settings
    );

    void drawResponsePage(
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

    void drawRoundRadioPage(
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

    int8_t repeatButtonAt(
        uint16_t x,
        uint16_t y
    );

    bool applyRepeatButton(
        int8_t button,
        GyroController& gyro,
        Settings& settings
    );

};
