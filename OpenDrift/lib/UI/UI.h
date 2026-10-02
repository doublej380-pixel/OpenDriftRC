#pragma once

#include <Arduino.h>

#include "LGFX_OpenDrift.hpp"
#include "Touch.h"
#include "GyroController.h"
#include "IMU.h"
#include "WIFIManager.h"
#include "Settings.h"
#include "RadioInput.h"
#include "Servo.h"

#if defined(OPENDRIFT_BOARD_AMOLED_164)
#include "Backgrounds.h"
#include "BlackboxArchive.h"
#endif


class UI
{

public:

    void begin(
        LGFX* display,
        GyroController& gyro,
        WiFiManager& wifi,
        Settings& settings,
        RadioInput& steeringRadio,
        RadioInput& gainRadio,
        ServoOutput& steeringServo
    );

    void setThrottleRadio(
        RadioInput& throttleRadio
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    void setBackgroundStore(
        Backgrounds& store
    );

    void setBlackboxArchive(
        BlackboxArchive& archive
    );
    #endif

    void requestRefresh();

    void setCalibrationCallback(
        void (*callback)()
    );

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    void setUsbMaintenanceCallback(
        void (*callback)()
    );

    void showFirmwareUpdateCompleted();
    #endif


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
    static constexpr int SWIPE_PREVIEW_DISTANCE_PX = 6;
    static constexpr int SWIPE_COMMIT_DISTANCE_PX = 24;
    static constexpr int SWIPE_FLICK_MIN_DISTANCE_PX = 8;
    static constexpr int SWIPE_FLICK_PROJECTED_DISTANCE_PX = 55;
    static constexpr uint32_t SWIPE_FLICK_MEMORY_MS = 120;
    static constexpr float SWIPE_FLICK_PROJECTION_SECONDS = 0.12f;
    #else
    static constexpr int UI_CANVAS_WIDTH = 240;
    static constexpr int UI_CANVAS_HEIGHT = 240;
    static constexpr int UI_CENTER_X = 120;
    static constexpr int UI_FOOTER_Y = 230;
    static constexpr int UI_DOTS_Y = 215;
    #endif

    LGFX* display = nullptr;

    RadioInput* throttleRadioInput = nullptr;

    ServoOutput* steeringServoOutput = nullptr;

    void (*calibrationCallback)() = nullptr;

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    void (*usbMaintenanceCallback)() = nullptr;
    unsigned long usbMaintenanceHoldStartedAt = 0;
    bool firmwareUpdateNoticeVisible = false;
    unsigned long firmwareUpdateNoticeUntil = 0;
    #endif

    LGFX_Sprite canvas;

    LGFX_Sprite transitionCanvas;

    bool transitionCanvasReady = false;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    LGFX_Sprite panelCanvas;
    bool panelCanvasReady = false;
    uint16_t* transitionCurrentPixels = nullptr;
    uint16_t* transitionIncomingPixels = nullptr;
    uint16_t* transitionBackgroundPixels = nullptr;
    bool transitionPixelsReady = false;
    uint32_t transitionFrames = 0;
    uint32_t transitionComposeUs = 0;
    uint32_t transitionTransferUs = 0;
    uint32_t transitionStartedUs = 0;
    void prepareTransitionPixels();
    void animateTransition(int16_t start, int16_t end, int8_t direction, uint32_t durationMs);
    void reportTransitionTiming();
    BlackboxArchive* blackboxArchive = nullptr;
    bool blackboxProgressVisible = false;
    uint8_t lastBlackboxProgress = 255;
    unsigned long blackboxResultShownAt = 0;

    void drawBlackboxProgress(
        uint8_t progress,
        BlackboxArchive::Status status
    );
    #endif

    LGFX_Sprite* lcd = nullptr;

    bool canvasReady = false;

    bool suppressFlush = false;
    bool refreshRequested = false;

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
    // Shared order: Drive, Core, Response, Drift Assist, Experimental,
    // Profiles, Radio, Steering, Physical Endpoints, WiFi, System,
    // Blackbox, and Backgrounds on AMOLED builds.

    uint8_t page = 0;


    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    const uint8_t totalPages = 13;
    #else
    const uint8_t totalPages = 11;
    #endif




    bool lastTouchState = false;


    int touchStartX = 0;

    int touchStartY = 0;

    bool trackingSwipe = false;

    unsigned long lastRadioRefresh = 0;

    int16_t lastDrawnGainHundredths = -1;

    unsigned long lastPageSwipe = 0;

    uint8_t radioSection = 0;

    bool steeringCalibrationError = false;

    // Completed endpoints are locked against incidental screen touches.
    // Holding an endpoint button deliberately starts a fresh calibration.
    int8_t steeringCalibrationResetPoint = -1;

    unsigned long steeringCalibrationResetStartedAt = 0;

    int8_t heldRepeatButton = 0;

    unsigned long nextRepeatAt = 0;

    uint8_t profileScroll = 0;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    bool swipePreviewActive = false;

    int8_t swipePreviewDirection = 0;

    uint8_t swipePreviewSourcePage = 0;

    uint8_t swipePreviewSourceRadioSection = 0;

    int16_t swipePreviewOffset = 0;

    unsigned long lastSwipePreviewAt = 0;

    int swipeLastX = 0;
    unsigned long swipeLastSampleAt = 0;
    unsigned long swipeLastMoveAt = 0;
    float swipeVelocityX = 0.0f;

    uint8_t appliedBrightnessLevel = 0xFF;
    unsigned long lastTouchMs = 0;
    uint16_t lastDimTimeoutSeconds = 0;
    uint8_t lastBrightnessPercent = 0;
    bool displayDimmed = false;
    bool swallowTouchUntilRelease = false;

    uint8_t appliedThemeText = 0;
    uint8_t appliedThemeAccent = 0;
    bool themeApplied = false;

    bool syncTheme(
        Settings& settings
    );

    Backgrounds* backgroundStore = nullptr;
    uint16_t* backgroundPixels = nullptr;
    char appliedBackgroundName[Backgrounds::NAME_LENGTH] = {0};
    uint32_t appliedBackgroundRevision = 0;
    bool backgroundApplied = false;
    uint8_t backgroundScroll = 0;

    bool applyBackground(
        Settings& settings
    );

    void drawBackgroundsPage(
        Settings& settings
    );

    bool isBackgroundsPage();

    bool updateDisplayBrightness(
        Settings& settings,
        bool touched
    );
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
        bool commit,
        float releaseVelocityX = 0.0f
    );



    void drawMainPage(
        GyroController& gyro,
        Settings& settings
    );



    void drawCorePage(
        GyroController& gyro,
        Settings& settings
    );

    void drawResponsePage(
        Settings& settings
    );

    void drawDriftAssistPage(
        Settings& settings
    );

    void drawExperimentalPage(
        Settings& settings
    );

    void drawProfilesPage(
        Settings& settings
    );

    bool isProfilesPage();



    void drawSystemPage(
        Settings& settings
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    void drawBlackboxPage(
        Settings& settings
    );
    #endif



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

    void drawSteeringCalibrationPage(
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

    bool actionButtonAt(
        uint16_t x,
        uint16_t y
    );

    bool captureSteeringCalibration(
        uint8_t point,
        RadioInput& steeringRadio,
        Settings& settings
    );

    bool applyRepeatButton(
        int8_t button,
        GyroController& gyro,
        Settings& settings
    );

};
