#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_system.h>

#if !defined(OPENDRIFT_HEADLESS)
#include "LGFX_OpenDrift.hpp"
#include "Touch.h"
#include "UI.h"
#endif
#if defined(OPENDRIFT_BOARD_MATRIX)
#include "MatrixStatus.h"
#endif
#include "IMU.h"
#include "Servo.h"
#include "EscOutput.h"
#include "GyroController.h"
#include "WIFIManager.h"
#include "Settings.h"
#include "RadioInput.h"
#if defined(OPENDRIFT_INPUT_CRSF)
#include "CrsfInput.h"
#include "CrsfParameterDevice.h"
#if defined(OPENDRIFT_BOARD_AMOLED_164)
#include "AuxChannelOutputs.h"
#endif
#endif
#include "WebConfigurator.h"
#include "BlackboxLogger.h"
#if defined(OPENDRIFT_BOARD_AMOLED_164)
#include "Backgrounds.h"
#include "BlackboxArchive.h"
#include "OnboardStorage.h"
#include "UsbMaintenance.h"
#endif

#if !defined(OPENDRIFT_HEADLESS)
LGFX lcd;
#endif

IMU imu;

ServoOutput steeringServo;

EscOutput throttleOutput;

GyroController gyro;

#if !defined(OPENDRIFT_HEADLESS)
Touch touch;
UI ui;
#endif

#if defined(OPENDRIFT_BOARD_MATRIX)
MatrixStatus matrixStatus;
#endif

WiFiManager wifi;

Settings settings;

WebConfigurator webConfig;

RadioInput steeringRadio;

RadioInput gainRadio;

RadioInput throttleRadio;

#if defined(OPENDRIFT_INPUT_CRSF)
CrsfInput crsf;
CrsfParameterDevice crsfParameters;
#if defined(OPENDRIFT_BOARD_AMOLED_164)
AuxChannelOutputs auxChannelOutputs;
#endif
#endif

BlackboxLogger blackbox;

#if defined(OPENDRIFT_BOARD_AMOLED_164)
Backgrounds backgrounds;
BlackboxArchive blackboxArchive;
OnboardStorage onboardStorage;
#if defined(OPENDRIFT_USB_MAINTENANCE)
UsbMaintenance usbMaintenance;
#endif
#endif

unsigned long lastBlackboxLog = 0;

bool blackboxStarted = false;

bool lastBlackboxEnabled = false;

bool blackboxStartAttempted = false;

static uint32_t controlLoopHz = 250;
static uint32_t controlLoopPeriodMs = 4;
static uint8_t appliedDisplayRotation = 0xFF;

#if defined(OPENDRIFT_BOARD_AMOLED_164)
static constexpr uint8_t STARTUP_RETRY_COUNT = 3;
static constexpr int I2C_SDA_PIN = 47;
static constexpr int I2C_SCL_PIN = 48;
static constexpr int AMOLED_RESET_PIN = 21;
#if defined(OPENDRIFT_AMOLED_V2)
static constexpr int AMOLED_CS_PIN = 46;
#else
static constexpr int AMOLED_CS_PIN = 9;
#endif
#endif

struct ControlTelemetry
{
    float yaw = 0.0f;
    int requestedGyroCorrection = 0;
    int limitedGyroCorrection = 0;
    int appliedGyroCorrection = 0;
    bool correctionSaturated = false;
    int steeringCommand = 1500;
    int servoCommand = 1500;
    bool steeringSignal = false;
    bool throttleSignal = false;
    bool imuHealthy = true;
};

ControlTelemetry controlTelemetry;

portMUX_TYPE controlTelemetryMux =
    portMUX_INITIALIZER_UNLOCKED;

SemaphoreHandle_t i2cBusMutex = nullptr;

TaskHandle_t controlTaskHandle = nullptr;

#if defined(OPENDRIFT_INPUT_CRSF)
TaskHandle_t crsfTaskHandle = nullptr;
#endif

#if defined(OPENDRIFT_INPUT_CRSF)
#if defined(OPENDRIFT_BOARD_MATRIX)
#define SERVO_OUTPUT_PIN 1
#define CRSF_RX_PIN 3
#define CRSF_TX_PIN 4
#define CRSF_THROTTLE_OUTPUT_PIN 2
#elif defined(OPENDRIFT_AMOLED_V2)
// V2 connects the IMU and touch interrupt outputs to GPIO17/18. Keep the
// receiver UART off those lines to prevent electrical contention.
#define SERVO_OUTPUT_PIN 15
#define CRSF_RX_PIN 1
#define CRSF_TX_PIN 2
#if defined(OPENDRIFT_CRSF_V2_THROTTLE_GPIO8)
#define CRSF_THROTTLE_OUTPUT_PIN 8
#else
#define CRSF_THROTTLE_OUTPUT_PIN 16
#endif
#else
#define SERVO_OUTPUT_PIN 15
#define CRSF_RX_PIN 17
#define CRSF_TX_PIN 18
#define CRSF_THROTTLE_OUTPUT_PIN 16
#endif
static constexpr uint8_t CRSF_STEERING_CHANNEL = 0;
static constexpr uint8_t CRSF_THROTTLE_CHANNEL = 1;
static constexpr uint8_t CRSF_GAIN_CHANNEL = 2;
static constexpr uint32_t CRSF_SIGNAL_TIMEOUT_MS = 50;
static constexpr uint32_t CRSF_THROTTLE_NEUTRAL_MS = 500;
static constexpr int CRSF_THROTTLE_NEUTRAL_BAND_US = 50;
#else
#if defined(OPENDRIFT_BOARD_MATRIX)
#define SERVO_OUTPUT_PIN 3
#define RADIO_STEERING_PIN 1
#define RADIO_THROTTLE_PIN 2
#elif defined(OPENDRIFT_AMOLED_V2)
#define SERVO_OUTPUT_PIN 1
#define RADIO_STEERING_PIN 15
#define RADIO_THROTTLE_PIN 16
#else
#define SERVO_OUTPUT_PIN 17
#define RADIO_STEERING_PIN 15
#define RADIO_THROTTLE_PIN 16
#endif
#endif
#if defined(OPENDRIFT_BOARD_MATRIX)
#define SHARED_GAIN_THROTTLE_PIN 4
#elif defined(OPENDRIFT_AMOLED_V2)
#define SHARED_GAIN_THROTTLE_PIN 2
#else
#define SHARED_GAIN_THROTTLE_PIN 18
#endif

bool pin18ModeConfigured = false;

volatile bool pin18ThrottleOutputMode = false;

volatile bool throttleOutputActive = false;

#if defined(OPENDRIFT_INPUT_CRSF)
volatile bool crsfThrottleArmed = false;
bool lastCrsfSignal = false;
volatile uint32_t crsfThrottleNeutralSinceMs = 0;
volatile bool crsfThrottleSignalSnapshot = false;
volatile float crsfThrottlePulseSnapshot = 1500.0f;
volatile bool crsfThrottleOutputArmed = false;
#endif

volatile bool gyroCalibrationRequested = false;

#if defined(OPENDRIFT_USB_MAINTENANCE)
volatile bool usbMaintenanceRequested = false;
bool usbMaintenanceActive = false;
bool usbMaintenanceLastTouch = false;
bool firmwareUpdateCompletedAtBoot = false;
uint32_t lastUsbUpdateRevision = 0;
LGFX_Sprite usbMaintenanceCanvas;
LGFX_Sprite usbMaintenancePanel;
bool usbMaintenanceCanvasReady = false;
#endif


void requestGyroCalibration()
{
    gyroCalibrationRequested = true;
}


#if defined(OPENDRIFT_USB_MAINTENANCE)
void requestUsbMaintenance()
{
    usbMaintenanceRequested = true;
}


void flushUsbMaintenanceCanvas()
{
    if(!usbMaintenanceCanvasReady) return;

    uint16_t* source = static_cast<uint16_t*>(
        usbMaintenanceCanvas.getBuffer()
    );
    uint16_t* target = static_cast<uint16_t*>(
        usbMaintenancePanel.getBuffer()
    );

    for(int y = 0; y < 280; y++)
    {
        for(int x = 0; x < 456; x++)
        {
            target[((455 - x) * 280) + y] = source[(y * 456) + x];
        }
    }

    usbMaintenancePanel.pushSprite(&lcd, 0, 0);
}


void drawUsbMaintenanceScreen()
{
    if(!usbMaintenanceCanvasReady)
    {
        const bool usePsram = psramFound();
        usbMaintenanceCanvas.setPsram(usePsram);
        usbMaintenancePanel.setPsram(usePsram);
        usbMaintenanceCanvas.setColorDepth(16);
        usbMaintenancePanel.setColorDepth(16);

        const bool canvasCreated =
            usbMaintenanceCanvas.createSprite(456, 280) != nullptr;
        const bool panelCreated =
            usbMaintenancePanel.createSprite(280, 456) != nullptr;

        usbMaintenanceCanvasReady = canvasCreated && panelCreated;

        if(!usbMaintenanceCanvasReady)
        {
            usbMaintenanceCanvas.deleteSprite();
            usbMaintenancePanel.deleteSprite();
        }
    }

    if(!usbMaintenanceCanvasReady)
    {
        Serial.println("USB maintenance display buffers unavailable");
        return;
    }

    usbMaintenanceCanvas.fillScreen(TFT_BLACK);
    usbMaintenanceCanvas.setTextWrap(false);
    usbMaintenanceCanvas.setTextDatum(top_center);

    const UsbMaintenance::UpdateState updateState =
        usbMaintenance.getUpdateState();
    const size_t updateBytes = usbMaintenance.getUpdateBytes();

    usbMaintenanceCanvas.setTextColor(
        updateState == UsbMaintenance::UPDATE_COMPLETE
            ? TFT_GREEN
            : updateState == UsbMaintenance::UPDATE_ERROR
                ? TFT_RED
                : updateState == UsbMaintenance::UPDATE_WRITING
                    ? 0xFD20
                    : TFT_CYAN
    );
    usbMaintenanceCanvas.setTextSize(3);
    usbMaintenanceCanvas.drawString(
        updateState == UsbMaintenance::UPDATE_COMPLETE
            ? "UPDATE COMPLETE"
            : updateState == UsbMaintenance::UPDATE_ERROR
                ? "UPDATE FAILED"
                : updateState == UsbMaintenance::UPDATE_WRITING
                    ? "INSTALLING UPDATE"
                    : "USB MAINTENANCE",
        228,
        20
    );

    if(updateState != UsbMaintenance::UPDATE_IDLE)
    {
        char updateMessage[64];
        snprintf(
            updateMessage,
            sizeof(updateMessage),
            "%u KB written",
            (unsigned int)(updateBytes / 1024U)
        );

        usbMaintenanceCanvas.setTextSize(2);
        usbMaintenanceCanvas.setTextColor(TFT_WHITE);
        usbMaintenanceCanvas.drawString(updateMessage, 228, 86);

        usbMaintenanceCanvas.setTextSize(1);
        usbMaintenanceCanvas.setTextColor(
            updateState == UsbMaintenance::UPDATE_COMPLETE
                ? TFT_GREEN
                : updateState == UsbMaintenance::UPDATE_ERROR
                    ? TFT_RED
                    : 0xFD20
        );
        usbMaintenanceCanvas.drawString(
            updateState == UsbMaintenance::UPDATE_COMPLETE
                ? "Firmware verified. Restarting into the update..."
                : updateState == UsbMaintenance::UPDATE_ERROR
                    ? "The update was rejected. Existing firmware is safe."
                    : "DO NOT UNPLUG OR RESTART",
            228,
            132
        );

        if(updateState == UsbMaintenance::UPDATE_WRITING)
        {
            const int activityWidth = 28 + (int)((updateBytes / 4096U) % 200U);
            usbMaintenanceCanvas.drawRoundRect(108, 166, 240, 18, 5, 0xFD20);
            usbMaintenanceCanvas.fillRoundRect(
                112,
                170,
                activityWidth,
                10,
                0xFD20
            );
        }
    }
    else
    {
        usbMaintenanceCanvas.setTextSize(2);
        usbMaintenanceCanvas.setTextColor(TFT_WHITE);
        usbMaintenanceCanvas.drawString(
            usbMaintenance.firmwareDriveReady()
                ? "Firmware drive: READY"
                : "Firmware drive: UNAVAILABLE",
            228,
            72
        );
        usbMaintenanceCanvas.drawString(
            usbMaintenance.sdDriveReady()
                ? "SD logs: READY (READ ONLY)"
                : "SD logs: NO CARD",
            228,
            102
        );

        usbMaintenanceCanvas.setTextColor(0x9CF3);
        usbMaintenanceCanvas.setTextSize(1);
        usbMaintenanceCanvas.drawString(
            "Copy firmware.bin to update OpenDrift.",
            228,
            146
        );
        usbMaintenanceCanvas.drawString(
            "Copy logs from the read-only SD drive.",
            228,
            163
        );
        usbMaintenanceCanvas.drawString(
            "Restart safely disconnects both USB drives.",
            228,
            180
        );
    }

    const bool restartEnabled =
        updateState != UsbMaintenance::UPDATE_WRITING;
    usbMaintenanceCanvas.fillRoundRect(
        118,
        216,
        220,
        46,
        8,
        restartEnabled ? 0x3186 : 0x2104
    );
    usbMaintenanceCanvas.drawRoundRect(
        118,
        216,
        220,
        46,
        8,
        restartEnabled ? TFT_CYAN : 0x8410
    );
    usbMaintenanceCanvas.setTextColor(TFT_WHITE);
    usbMaintenanceCanvas.setTextSize(2);
    usbMaintenanceCanvas.drawString(
        restartEnabled ? "EXIT / RESTART" : "UPDATE IN PROGRESS",
        228,
        230
    );
    usbMaintenanceCanvas.setTextDatum(top_left);

    flushUsbMaintenanceCanvas();
}
#endif

const char* password = "opendrift";


#if defined(OPENDRIFT_BOARD_AMOLED_164)
static constexpr float AMOLED_BOOT_LOG_TEXT_SIZE = 1.15f;
#endif

#if defined(OPENDRIFT_HEADLESS)
static constexpr uint16_t TFT_RED = 0;
static constexpr uint16_t TFT_GREEN = 0;
static constexpr uint16_t TFT_YELLOW = 0;
static constexpr uint16_t TFT_CYAN = 0;
#endif


const char* resetReasonName(
    esp_reset_reason_t reason
)
{
    switch(reason)
    {
        case ESP_RST_POWERON: return "power-on";
        case ESP_RST_EXT: return "external-reset";
        case ESP_RST_SW: return "software-reset";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "interrupt-watchdog";
        case ESP_RST_TASK_WDT: return "task-watchdog";
        case ESP_RST_WDT: return "watchdog";
        case ESP_RST_DEEPSLEEP: return "deep-sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "sdio";
        #if defined(ESP_RST_USB)
        case ESP_RST_USB: return "usb";
        #endif
        #if defined(ESP_RST_JTAG)
        case ESP_RST_JTAG: return "jtag";
        #endif
        #if defined(ESP_RST_PWR_GLITCH)
        case ESP_RST_PWR_GLITCH: return "power-glitch";
        #endif
        #if defined(ESP_RST_CPU_LOCKUP)
        case ESP_RST_CPU_LOCKUP: return "cpu-lockup";
        #endif
        default: return "unknown";
    }
}


#if defined(OPENDRIFT_BOARD_AMOLED_164)
void releaseI2cBus()
{
    Wire.end();

    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, OUTPUT_OPEN_DRAIN);
    digitalWrite(I2C_SCL_PIN, HIGH);
    delayMicroseconds(10);

    // A peripheral interrupted mid-byte can hold SDA low indefinitely.
    // Nine clocks finish that byte before generating a STOP condition.
    for(uint8_t pulse = 0; pulse < 9; pulse++)
    {
        digitalWrite(I2C_SCL_PIN, LOW);
        delayMicroseconds(10);
        digitalWrite(I2C_SCL_PIN, HIGH);
        delayMicroseconds(10);
    }

    pinMode(I2C_SDA_PIN, OUTPUT_OPEN_DRAIN);
    digitalWrite(I2C_SDA_PIN, LOW);
    delayMicroseconds(10);
    digitalWrite(I2C_SCL_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(I2C_SDA_PIN, HIGH);
    delayMicroseconds(10);

    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, INPUT_PULLUP);
}


void resetAmoledPanelHardware()
{
    // Hold chip-select inactive while the panel reset line is sequenced. This
    // prevents partial QSPI commands if the ESP starts before the AMOLED rail
    // has completely settled or after a short off/on power cycle.
    pinMode(AMOLED_CS_PIN, OUTPUT);
    digitalWrite(AMOLED_CS_PIN, HIGH);

    pinMode(AMOLED_RESET_PIN, OUTPUT);
    digitalWrite(AMOLED_RESET_PIN, HIGH);
    delay(10);
    digitalWrite(AMOLED_RESET_PIN, LOW);
    delay(20);
    digitalWrite(AMOLED_RESET_PIN, HIGH);
    delay(120);
}
#endif


void applyDisplayRotation()
{
    uint8_t rotation = settings.getDisplayRotation();

    if(rotation == appliedDisplayRotation)
    {
        return;
    }

    #if defined(OPENDRIFT_BOARD_MATRIX)
    matrixStatus.setRotation(rotation);
    #elif defined(OPENDRIFT_BOARD_AMOLED_164)
    lcd.setRotation(rotation);
    touch.setRotation(rotation);
    ui.requestRefresh();
    #endif

    appliedDisplayRotation = rotation;

    Serial.printf(
        "Display rotation: %u degrees\n",
        (unsigned int)rotation * 90
    );
}



#if !defined(OPENDRIFT_HEADLESS)
class BootConsole
{

public:

    void begin(
        LGFX* display
    )
    {
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        this->display = display;

        bool usePsram =
            psramFound();

        canvas.setPsram(
            usePsram
        );

        panelCanvas.setPsram(
            usePsram
        );

        canvas.setColorDepth(16);
        panelCanvas.setColorDepth(16);

        canvasReady =
            canvas.createSprite(456, 280) != nullptr;

        panelReady =
            panelCanvas.createSprite(280, 456) != nullptr;

        if(!canvasReady || !panelReady)
        {
            return;
        }

        canvas.fillScreen(TFT_BLACK);
        canvas.setTextWrap(false);
        canvas.setTextColor(TFT_WHITE);
        canvas.setTextSize(2);
        canvas.drawString(
            #if defined(OPENDRIFT_INPUT_CRSF)
            #if defined(OPENDRIFT_CRSF_V2_THROTTLE_GPIO8)
            "OpenDrift GPIO8 OOPS boot",
            #else
            "OpenDrift CRSF verbose boot",
            #endif
            #else
            "OpenDrift verbose boot",
            #endif
            8,
            7
        );

        canvas.setTextSize(AMOLED_BOOT_LOG_TEXT_SIZE);
        canvas.setTextColor(0x7BEF);
        canvas.drawString(
            #if defined(OPENDRIFT_INPUT_CRSF)
            #if defined(OPENDRIFT_CRSF_V2_THROTTLE_GPIO8)
            "WARNING V2 throttle rerouted to GPIO8",
            #else
            "control kernel 1.0.9 crsf ttyOD0",
            #endif
            #else
            "control kernel 1.0.9 pwm  ttyOD0",
            #endif
            8,
            27
        );

        nextLineY = 44;
        flush();
        #else
        this->display = display;

        display->fillScreen(TFT_BLACK);
        display->setTextWrap(false);
        display->setTextColor(TFT_WHITE);
        display->setTextSize(2);
        display->drawCenterString(
            #if defined(OPENDRIFT_INPUT_CRSF)
            "OpenDrift CRSF BETA",
            #else
            "OpenDrift OPEN BETA",
            #endif
            120,
            12
        );

        display->setTextSize(1);
        display->setTextColor(0x7BEF);
        display->drawCenterString(
            "control kernel 2.0-round  ttyOD0",
            120,
            32
        );

        nextLineY = 49;
        #endif
    }


    void log(
        const char* message,
        const char* status = "[ OK ]",
        uint16_t statusColor = TFT_GREEN
    )
    {
        Serial.print(status);
        Serial.print(" ");
        Serial.println(message);

        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        if(!canvasReady || !panelReady)
        {
            return;
        }

        if(nextLineY > 263)
        {
            canvas.fillScreen(TFT_BLACK);
            canvas.setTextSize(AMOLED_BOOT_LOG_TEXT_SIZE);
            canvas.setTextColor(0x7BEF);
            canvas.drawString(
                "OpenDrift boot log (continued)",
                8,
                8
            );
            nextLineY = 27;
        }

        char timestamp[16];

        snprintf(
            timestamp,
            sizeof(timestamp),
            "[%7.3f]",
            millis() / 1000.0f
        );

        canvas.setTextSize(AMOLED_BOOT_LOG_TEXT_SIZE);
        canvas.setTextColor(0x8410);
        canvas.drawString(timestamp, 8, nextLineY);

        canvas.setTextColor(statusColor);
        canvas.drawString(status, 76, nextLineY);

        canvas.setTextColor(TFT_WHITE);
        canvas.drawString(message, 120, nextLineY);

        nextLineY += 13;
        flush();
        #else
        if(display == nullptr)
        {
            return;
        }

        if(nextLineY > 211)
        {
            display->fillScreen(TFT_BLACK);
            display->setTextSize(1);
            display->setTextColor(0x7BEF);
            display->drawCenterString(
                "OpenDrift boot log (continued)",
                120,
                18
            );
            nextLineY = 42;
        }

        char timestamp[12];

        snprintf(
            timestamp,
            sizeof(timestamp),
            "[%5.2f]",
            millis() / 1000.0f
        );

        display->setTextSize(1);
        display->setTextColor(0x8410);
        display->drawString(timestamp, 7, nextLineY);

        display->setTextColor(statusColor);
        display->drawString(status, 58, nextLineY);

        display->setTextColor(TFT_WHITE);
        display->drawString(message, 99, nextLineY);

        nextLineY += 12;
        #endif
    }


    void end()
    {
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        canvas.deleteSprite();
        panelCanvas.deleteSprite();
        canvasReady = false;
        panelReady = false;
        display = nullptr;
        #else
        display = nullptr;
        #endif
    }


private:

    LGFX* display = nullptr;
    int nextLineY = 43;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    LGFX_Sprite canvas;
    LGFX_Sprite panelCanvas;
    bool canvasReady = false;
    bool panelReady = false;


    void flush()
    {
        if(display == nullptr)
        {
            return;
        }

        uint16_t* source =
            static_cast<uint16_t*>(
                canvas.getBuffer()
            );

        uint16_t* target =
            static_cast<uint16_t*>(
                panelCanvas.getBuffer()
            );

        for(int y = 0; y < 280; y++)
        {
            for(int x = 0; x < 456; x++)
            {
                target[
                    ((455 - x) * 280) + y
                ] = source[(y * 456) + x];
            }
        }

        panelCanvas.pushSprite(
            display,
            0,
            0
        );
    }
    #endif
};
#else
class BootConsole
{
public:
    void begin()
    {
    }

    void log(
        const char* message,
        const char* status = "[ OK ]",
        uint16_t statusColor = 0
    )
    {
        (void)statusColor;
        Serial.print(status);
        Serial.print(" ");
        Serial.println(message);
    }

    void end()
    {
    }
};
#endif


BootConsole bootConsole;



bool configurePin18Mode()
{
    #if defined(OPENDRIFT_INPUT_CRSF)
    // The shared gain/throttle pin belongs to the full-duplex CRSF UART.
    pin18ThrottleOutputMode = false;
    pin18ModeConfigured = true;
    return true;
    #else
    bool throttleMode =
        settings.getThrottleOutputEnabled();

    if(
        pin18ModeConfigured &&
        pin18ThrottleOutputMode == throttleMode
    )
    {
        return true;
    }

    bool configured = false;

    if(throttleMode)
    {
        gainRadio.end();

        throttleOutput.end();
        throttleOutputActive = false;

        pinMode(
            SHARED_GAIN_THROTTLE_PIN,
            INPUT_PULLDOWN
        );

        configured = true;

        Serial.println(
            configured
            ? "shared pin mode: throttle output"
            : "shared throttle output unavailable"
        );
    }
    else
    {
        throttleOutput.end();
        throttleOutputActive = false;

        configured =
            gainRadio.begin(
                SHARED_GAIN_THROTTLE_PIN
            );

        Serial.println(
            configured
            ? "shared pin mode: gyro gain input"
            : "shared gain input unavailable"
        );
    }

    pin18ThrottleOutputMode =
        throttleMode;

    pin18ModeConfigured =
        true;

    return configured;
    #endif
}


#if defined(OPENDRIFT_INPUT_CRSF)
void updateCrsfThrottleOutput(
    float throttlePulse,
    bool signalValid
)
{
    if(!signalValid)
    {
        crsfThrottleArmed = false;
        crsfThrottleOutputArmed = false;
        crsfThrottleNeutralSinceMs = 0;

        return;
    }

    bool throttleNeutral =
        abs(throttlePulse - 1500) <=
        CRSF_THROTTLE_NEUTRAL_BAND_US;

    if(!crsfThrottleArmed || !crsfThrottleOutputArmed)
    {
        if(!throttleNeutral)
        {
            crsfThrottleNeutralSinceMs = 0;

            crsfThrottleOutputArmed = false;
            return;
        }

        uint32_t neutralSince = crsfThrottleNeutralSinceMs;

        if(neutralSince == 0)
        {
            crsfThrottleNeutralSinceMs = millis();
            return;
        }

        if(
            millis() - neutralSince <
            CRSF_THROTTLE_NEUTRAL_MS
        )
        {
            return;
        }

        if(!throttleOutputActive)
        {
            throttleOutput.configure(
                1500,
                false,
                100,
                0
            );

            throttleOutputActive =
                throttleOutput.begin(
                    CRSF_THROTTLE_OUTPUT_PIN,
                    settings.getThrottleOutputHz()
                );
        }

        crsfThrottleArmed = throttleOutputActive;

        if(!crsfThrottleArmed)
        {
            crsfThrottleNeutralSinceMs = 0;
            return;
        }

        crsfThrottleOutputArmed = true;

        Serial.println(
            "CRSF throttle output armed after neutral hold"
        );
    }

}
#endif

int mapSteeringPulse(
    int pulse,
    const Settings::SteeringCalibration& calibration
)
{
    if(calibration.calibrated)
    {
        int left = calibration.inputMin;
        int center = calibration.inputCenter;
        int right = calibration.inputMax;
        int leftDelta = left - center;
        int rightDelta = right - center;

        if(
            abs(leftDelta) >= 10 &&
            abs(rightDelta) >= 10 &&
            leftDelta * rightDelta < 0
        )
        {
            bool towardLeft = left < center
                ? pulse <= center
                : pulse >= center;

            return towardLeft
                ? map(constrain(pulse, min(left, center), max(left, center)), left, center, 1000, 1500)
                : map(constrain(pulse, min(center, right), max(center, right)), center, right, 1500, 2000);
        }
    }

    #if defined(OPENDRIFT_INPUT_CRSF)
    // CRSF's standard 172-1811 channel range decodes to 988-2012 us.
    // Normalize that protocol range here; physical endpoint calibration belongs
    // exclusively to the physical servo-endpoint stage.
    return map(constrain(pulse, 988, 2012), 988, 2012, 1000, 2000);
    #else
    return constrain(pulse, 1000, 2000);
    #endif
}



int applyRadioSteeringTravel(
    int steeringCommand,
    Settings& settings
)
{
    int travel =
        settings.getRadioSteeringTravel();

    int offset =
        steeringCommand - 1500;

    offset =
        (offset * travel)
        /
        100;

    return constrain(
        1500 + offset,
        1000,
        2000
    );
}



float mapGainPulse(
    int pulse,
    Settings& settings
)
{
    int gainMin =
        settings.getGainMin();

    int gainMax =
        settings.getGainMax();

    if(gainMax <= gainMin)
    {
        gainMin = 1000;
        gainMax = 2000;
    }

    pulse =
        constrain(
            pulse,
            gainMin,
            gainMax
        );

    float normalized =
        (pulse - gainMin)
        /
        (float)(gainMax - gainMin);

    return
        settings.getChannel3GainMin() +
        (
            (settings.getChannel3GainMax() - settings.getChannel3GainMin())
            * normalized
        );
}



void runControlIteration()
{
    #if defined(OPENDRIFT_INPUT_CRSF)
    bool crsfSignal =
        crsf.hasSignal(
            CRSF_SIGNAL_TIMEOUT_MS
        );

    if(crsfSignal)
    {
        steeringRadio.updateExternalPulse(
            crsf.getChannelMicroseconds(
                CRSF_STEERING_CHANNEL
            )
        );

        throttleRadio.updateExternalPulse(
            crsf.getChannelMicrosecondsFloat(
                CRSF_THROTTLE_CHANNEL
            )
        );

        gainRadio.updateExternalPulse(
            crsf.getChannelMicroseconds(
                CRSF_GAIN_CHANNEL
            )
        );
    }
    else
    {
        steeringRadio.invalidateExternal();
        throttleRadio.invalidateExternal();
        gainRadio.invalidateExternal();
    }
    #endif

    static Settings::ControllerSnapshot controllerSettings = {};
    static uint32_t controllerSettingsGeneration = UINT32_MAX;

    bool controllerSettingsChanged = settings.getControllerSnapshot(
        controllerSettingsGeneration,
        controllerSettings
    );

    if(controllerSettingsChanged)
    {
        gyro.setDeadband(controllerSettings.deadband);
        gyro.setSmoothing(controllerSettings.smoothing);
        gyro.setMaxCorrection(controllerSettings.maxCorrection * 10);
        gyro.setIntegralGain(controllerSettings.driftMemory);
        gyro.setIntegralLimit(controllerSettings.memoryLimit);
        gyro.setHoldBoost(controllerSettings.holdAssist);
        gyro.setCounterSteerAssist(controllerSettings.countersteer);
        gyro.setTransitionSpeed(controllerSettings.transitionSpeed);
        gyro.setPredictionStrength(controllerSettings.prediction);
        gyro.setDriverPriority(controllerSettings.driverPriority);
        gyro.setHuntStrength(controllerSettings.antiWobble);
        gyro.setAntiWobbleScale(controllerSettings.antiWobbleScale);
    }

    if(
        !pin18ThrottleOutputMode &&
        gainRadio.hasSignal()
    )
    {
        gyro.setGain(
            mapGainPulse(
                gainRadio.getPulseWidth(),
                settings
            )
        );
    }
    else
    {
        gyro.setGain(controllerSettings.gain);
    }

    static uint8_t i2cMisses = 0;
    static float lastYaw = 0.0f;

    bool i2cReady =
        i2cBusMutex == nullptr ||
        xSemaphoreTake(
            i2cBusMutex,
            pdMS_TO_TICKS(2)
        ) == pdTRUE;

    if(i2cReady)
    {
        i2cMisses = 0;

        if(controllerSettingsChanged)
        {
            imu.setGyroLpfMode(controllerSettings.gyroLpfMode);
        }

        imu.update();

        if(
            gyroCalibrationRequested &&
            !gyro.isCalibrating()
        )
        {
            gyro.startCalibration(controlLoopHz / 2);
            gyroCalibrationRequested = false;
        }

        if(
            gyro.isCalibrating() &&
            (!imu.isYawValid() || !imu.lastGyroReadOk())
        )
        {
            gyro.abortCalibration();
        }

        if(i2cBusMutex != nullptr)
        {
            xSemaphoreGive(i2cBusMutex);
        }
    }
    else
    {
        if(i2cMisses < 255)
        {
            i2cMisses++;
        }

        if(gyro.isCalibrating())
        {
            gyro.abortCalibration();
        }
    }

    #if defined(OPENDRIFT_INPUT_CRSF)
    bool steeringSignal = crsfSignal;
    bool throttleSignal = crsfSignal;
    #else
    bool steeringSignal = steeringRadio.hasSignal();
    bool throttleSignal = throttleRadio.hasSignal();
    #endif

    float throttleOutputPulse =
        throttleSignal
        ? throttleRadio.getPulseWidthFloat()
        : 1500.0f;

    int throttlePulse =
        (int)roundf(throttleOutputPulse);

    Settings::SteeringCalibration calibration;
    settings.getSteeringCalibration(calibration);

    int steeringCommand = 1500;
    int driverCommand = 1500;

    if(steeringSignal)
    {
        driverCommand =
            mapSteeringPulse(
                steeringRadio.getPulseWidth(),
                calibration
            );

        steeringCommand =
            applyRadioSteeringTravel(
                driverCommand,
                settings
            );
    }

    bool imuHealthy = imu.isHealthy();
    float yaw = 0.0f;

    if(i2cReady)
    {
        yaw = imu.isYawValid() ? imu.getYawRate() : 0.0f;
        lastYaw = yaw;
    }
    else if(i2cMisses < 3)
    {
        yaw = lastYaw;
    }

    static bool lastGyroReverse = controllerSettings.gyroReverse;

    bool gyroReverse = controllerSettings.gyroReverse;

    if(gyroReverse != lastGyroReverse)
    {
        gyro.reverseYawFrame();
        lastGyroReverse = gyroReverse;
    }

    float controllerYaw =
        gyroReverse ? -yaw : yaw;

    float gyroCorrection =
        gyro.update(
            controllerYaw,
            driverCommand,
            steeringSignal,
            throttlePulse,
            throttleSignal
        );

    int requestedGyroCorrection =
        gyro.getRequestedCorrection();

    int limitedGyroCorrection =
        (int)roundf(gyroCorrection);
    int appliedGyroCorrection = 0;
    bool correctionSaturated =
        requestedGyroCorrection != limitedGyroCorrection;

    float servoCommand =
        steeringServo.getPosition();

    if(steeringSignal)
    {
        // Steering Travel scales only the driver's command. Max Correction
        // independently controls gyro authority, and physical calibration is
        // the final hard clamp applied by ServoOutput.
        servoCommand = constrain(
            steeringCommand + gyroCorrection,
            1000.0f,
            2000.0f
        );

        appliedGyroCorrection =
            (int)roundf(
                servoCommand - steeringCommand
            );

        correctionSaturated =
            correctionSaturated ||
            appliedGyroCorrection != limitedGyroCorrection;

        steeringServo.configure(
            settings.getServoCenter(),
            settings.getServoReverse(),
            settings.getServoTravel(),
            settings.getServoQuiet(),
            calibration.calibrated,
            calibration.min,
            calibration.center,
            calibration.max
        );

        steeringServo.writeMicroseconds(
            servoCommand
        );

        steeringServo.noteCommandPulse(
            steeringCommand
        );

        servoCommand = steeringServo.getPosition();
    }

    #if defined(OPENDRIFT_INPUT_CRSF)
    else if(lastCrsfSignal)
    {
        // Do not hold the last steering command after a receiver loss.
        steeringServo.center();
        servoCommand = steeringServo.getPosition();

        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        auxChannelOutputs.writeFailsafe();
        #endif
    }

    lastCrsfSignal = steeringSignal;

    // The main loop owns throttle PWM attachment and removal so peripheral
    // setup never runs inside this high-priority task.
    crsfThrottlePulseSnapshot = throttleOutputPulse;
    crsfThrottleSignalSnapshot = throttleSignal;

    if(!throttleSignal)
    {
        crsfThrottleArmed = false;
        crsfThrottleOutputArmed = false;
        crsfThrottleNeutralSinceMs = 0;
    }

    if(throttleOutputActive)
    {
        throttleOutput.writeMicroseconds(
            (
                !throttleSignal ||
                !crsfThrottleArmed ||
                !crsfThrottleOutputArmed
            )
            ? 1500
            : constrain(throttleOutputPulse, 1000.0f, 2000.0f)
        );
    }
    #else
    if(pin18ThrottleOutputMode && throttleOutputActive)
    {
        throttleOutput.writeMicroseconds(
            throttleSignal
            ? throttleRadio.getPulseWidthFloat()
            : 1500
        );
    }
    #endif

    ControlTelemetry nextTelemetry;

    nextTelemetry.yaw = controllerYaw;
    nextTelemetry.requestedGyroCorrection =
        requestedGyroCorrection;
    nextTelemetry.limitedGyroCorrection =
        limitedGyroCorrection;
    nextTelemetry.appliedGyroCorrection =
        appliedGyroCorrection;
    nextTelemetry.correctionSaturated =
        correctionSaturated;
    nextTelemetry.steeringCommand =
        steeringCommand;
    nextTelemetry.servoCommand =
        (int)roundf(servoCommand);
    nextTelemetry.steeringSignal =
        steeringSignal;
    nextTelemetry.throttleSignal =
        throttleSignal;
    nextTelemetry.imuHealthy =
        imuHealthy;

    portENTER_CRITICAL(
        &controlTelemetryMux
    );

    controlTelemetry =
        nextTelemetry;

    portEXIT_CRITICAL(
        &controlTelemetryMux
    );
}


#if defined(OPENDRIFT_INPUT_CRSF)
void crsfTask(void* parameter)
{
    (void)parameter;

    while(true)
    {
        crsf.update();

        // F1000 supplies about one channel frame per millisecond. Keep the
        // UART drained independently from IMU, UI, storage, and WiFi work.
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
#endif


void controlTask(void* parameter)
{
    (void)parameter;

    TickType_t lastWake =
        xTaskGetTickCount();

    const TickType_t period =
        pdMS_TO_TICKS(
            controlLoopPeriodMs
        );

    while(true)
    {
        runControlIteration();

        if(
            xTaskGetTickCount() - lastWake >
            period * 5
        )
        {
            lastWake = xTaskGetTickCount();
        }

        vTaskDelayUntil(
            &lastWake,
            period
        );
    }
}



void updateBlackboxAvailability()
{
    bool enabled =
        settings.getBlackboxEnabled();

    if(!enabled)
    {
        lastBlackboxEnabled =
            false;

        blackboxStartAttempted =
            false;

        return;
    }

    if(
        lastBlackboxEnabled &&
        blackboxStartAttempted
    )
    {
        return;
    }

    lastBlackboxEnabled =
        true;

    blackboxStartAttempted =
        true;

    if(blackbox.isReady())
    {
        blackboxStarted =
            true;

        return;
    }

    if(blackbox.begin())
    {
        blackboxStarted =
            true;

        Serial.printf(
            "Blackbox PSRAM logger OK: %u KB\n",
            (unsigned)(
                blackbox.getCapacityBytes() /
                1024
            )
        );
    }
    else
    {
        blackboxStarted =
            false;

        Serial.println("Blackbox logging unavailable");
    }
}



void setup()
{
    Serial.begin(115200);

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    // Give the external panel and sensor rails time to settle before touching
    // either bus. This is especially important after a rapid power cycle.
    delay(800);
    #else
    delay(500);
    #endif

    Serial.println("OpenDrift Starting");

    #if defined(OPENDRIFT_USB_UPDATE_TEST_PAYLOAD)
    Serial.println("USB OTA TEST PAYLOAD ACTIVE");
    #endif

    #if defined(OPENDRIFT_INPUT_CRSF)
    // Do not let a powered F1000 receiver start UART activity while the panel,
    // sensors, PSRAM, and shared resources are still being initialized.
    pinMode(CRSF_RX_PIN, INPUT);
    pinMode(CRSF_TX_PIN, INPUT);
    #endif

    esp_reset_reason_t resetReason =
        esp_reset_reason();

    Serial.print("Reset reason: ");
    Serial.print(resetReasonName(resetReason));
    Serial.print(" (");
    Serial.print((int)resetReason);
    Serial.println(")");

    #if !defined(OPENDRIFT_BOARD_AMOLED_164) && !defined(OPENDRIFT_HEADLESS)
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    #endif

    #if defined(OPENDRIFT_BOARD_MATRIX)
    matrixStatus.begin();
    bootConsole.begin();
    bootConsole.log(
        "ws2812: low-brightness status matrix online"
    );
    #else
    bool displayOk = false;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    for(uint8_t attempt = 1; attempt <= STARTUP_RETRY_COUNT; attempt++)
    {
        resetAmoledPanelHardware();

        displayOk = lcd.init();

        Serial.printf(
            "AMOLED init attempt %u/%u: %s\n",
            attempt,
            STARTUP_RETRY_COUNT,
            displayOk ? "OK" : "FAIL"
        );

        if(displayOk)
        {
            break;
        }

        delay(150);
    }
    #else
    displayOk = lcd.init();
    #endif

    Serial.print("Display init: ");
    Serial.println(displayOk ? "OK" : "FAIL");

    if(displayOk)
    {
        lcd.setColorDepth(16);
        lcd.setSwapBytes(false);
        lcd.setRotation(0);
        lcd.fillScreen(TFT_BLACK);
        lcd.setTextColor(TFT_WHITE);

        bootConsole.begin(
            &lcd
        );
    }

    bootConsole.log(
        displayOk
        ?
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        "sh8601: AMOLED framebuffer online"
        #else
        "gc9a01: round framebuffer online"
        #endif
        : "display initialization failed",
        displayOk ? "[ OK ]" : "[FAIL]",
        displayOk ? TFT_GREEN : TFT_RED
    );
    #endif

    char memoryMessage[48];

    snprintf(
        memoryMessage,
        sizeof(memoryMessage),
        "memory: %u KB external PSRAM detected",
        (unsigned int)(ESP.getPsramSize() / 1024)
    );

    bootConsole.log(
        memoryMessage,
        psramFound() ? "[ OK ]" : "[WARN]",
        psramFound() ? TFT_GREEN : TFT_YELLOW
    );

    //-------------------
    // SETTINGS
    //-------------------

    bool settingsOk =
        settings.begin();

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    firmwareUpdateCompletedAtBoot =
        UsbMaintenance::consumeCompletedUpdate();
    #endif

    controlLoopHz = settings.getControlLoopHz();
    controlLoopPeriodMs = 1000 / controlLoopHz;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(displayOk)
    {
        lcd.setBrightness(
            (uint8_t)((settings.getDisplayBrightness() * 255U + 50U) / 100U)
        );
    }
    #endif

    applyDisplayRotation();

    bootConsole.log(
        "nvs: mounted OpenDrift settings store",
        settingsOk ? "[ OK ]" : "[WARN]",
        settingsOk ? TFT_GREEN : TFT_YELLOW
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    //-------------------
    // REMOVABLE STORAGE
    //-------------------

    const bool sdCardOk = onboardStorage.begin();

    char sdMessage[64];
    snprintf(
        sdMessage,
        sizeof(sdMessage),
        sdCardOk
            ? "sdcard: %.1f GB ready"
            : "sdcard: no readable card inserted",
        sdCardOk
            ? (double)onboardStorage.cardSizeBytes() / 1073741824.0
            : 0.0
    );

    bootConsole.log(
        sdMessage,
        sdCardOk ? "[ OK ]" : "[SKIP]",
        sdCardOk ? TFT_GREEN : 0x8410
    );

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    if(UsbMaintenance::consumeBootRequest())
    {
        bootConsole.log(
            "usb: entering isolated maintenance mode",
            "[WAIT]",
            TFT_CYAN
        );
        delay(250);
        bootConsole.end();

        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        Wire.setTimeOut(5);
        touch.begin();
        touch.setRotation(settings.getDisplayRotation());

        usbMaintenance.begin(onboardStorage);
        usbMaintenanceActive = true;
        lastUsbUpdateRevision = usbMaintenance.getUpdateRevision();
        drawUsbMaintenanceScreen();

        Serial.printf(
            "USB maintenance: firmware=%s sd=%s\n",
            usbMaintenance.firmwareDriveReady() ? "ready" : "unavailable",
            usbMaintenance.sdDriveReady() ? "ready/read-only" : "unavailable"
        );
        return;
    }
    #endif

    //-------------------
    // BACKGROUND STORAGE
    //-------------------

    // First-use FFat formatting happens before actuator/control tasks exist.
    // Runtime uploads are explicitly a stationary maintenance operation.
    const bool backgroundsOk = backgrounds.begin();

    char backgroundMessage[72];
    snprintf(
        backgroundMessage,
        sizeof(backgroundMessage),
        backgroundsOk
            ? "ffat: %u backgrounds, %u KB free"
            : "ffat: background storage unavailable",
        backgroundsOk ? (unsigned int)backgrounds.getCount() : 0,
        backgroundsOk ? (unsigned int)(backgrounds.getFreeBytes() / 1024) : 0
    );

    bootConsole.log(
        backgroundMessage,
        backgroundsOk ? "[ OK ]" : "[WARN]",
        backgroundsOk ? TFT_GREEN : TFT_YELLOW
    );

    webConfig.setBackgroundStore(backgrounds);
    blackboxArchive.begin(
        blackbox,
        onboardStorage.fileSystem(),
        onboardStorage.isSdAvailable() ? "SD card" : nullptr,
        onboardStorage.freeBytes()
    );
    webConfig.setBlackboxArchive(blackboxArchive);
    ui.setBackgroundStore(backgrounds);
    ui.setBlackboxArchive(blackboxArchive);
    #endif

    //-------------------
    // IMU
    //-------------------

    // Keep a stuck I2C peripheral from blocking several control periods.
    Wire.setTimeOut(5);

    bool imuOk = false;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    for(uint8_t attempt = 1; attempt <= STARTUP_RETRY_COUNT; attempt++)
    {
        if(attempt > 1)
        {
            releaseI2cBus();
            delay(50);
        }

        imuOk = imu.begin();

        Serial.printf(
            "IMU init attempt %u/%u: %s\n",
            attempt,
            STARTUP_RETRY_COUNT,
            imuOk ? "OK" : "FAIL"
        );

        if(imuOk)
        {
            break;
        }

        delay(150);
    }
    #else
    imuOk = imu.begin();
    #endif

    if(!imuOk)
    {
        #if defined(OPENDRIFT_BOARD_MATRIX)
        matrixStatus.setState(MatrixStatus::State::Error);
        #endif

        bootConsole.log(
            "qmi8658: probe failed; safe reboot",
            "[FAIL]",
            TFT_RED
        );

        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        releaseI2cBus();
        #endif

        Serial.flush();
        delay(1500);
        esp_restart();
    }

    Serial.println("IMU OK");

    bootConsole.log(
        "qmi8658: 6-axis inertial sensor ready"
    );

    // Calibrate with the same hardware filter used while driving.
    imu.setGyroLpfMode(
        settings.getGyroLpfMode()
    );

    delay(80);

    #if !defined(OPENDRIFT_HEADLESS)
    //-------------------
    // TOUCH
    //-------------------

    // Probe touch before attaching either actuator output. Touch is useful
    // but not safety-critical: a failed controller must never brick the gyro.
    Serial.println("Starting Touch");

    bool touchOk = false;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    for(uint8_t attempt = 1; attempt <= STARTUP_RETRY_COUNT; attempt++)
    {
        if(attempt > 1)
        {
            releaseI2cBus();
            Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
            Wire.setClock(300000);
            delay(50);
        }

        touchOk = touch.begin();

        Serial.printf(
            "Touch init attempt %u/%u: %s\n",
            attempt,
            STARTUP_RETRY_COUNT,
            touchOk ? "OK" : "FAIL"
        );

        if(touchOk)
        {
            break;
        }

        delay(150);
    }
    #else
    touchOk = touch.begin();
    #endif

    bootConsole.log(
        touchOk
        ?
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        "ft3168: capacitive touch input ready"
        #else
        "cst816s: capacitive touch input ready"
        #endif
        : "touch controller offline; continuing",
        touchOk ? "[ OK ]" : "[WARN]",
        touchOk ? TFT_GREEN : TFT_YELLOW
    );

    Serial.println(touchOk ? "TOUCH OK" : "TOUCH OFFLINE");
    #endif

    //-------------------
    // SERVO
    //-------------------

    if(!steeringServo.begin(
        SERVO_OUTPUT_PIN,
        controlLoopHz
    ))
    {
        #if defined(OPENDRIFT_BOARD_MATRIX)
        matrixStatus.setState(MatrixStatus::State::Error);
        #endif

        bootConsole.log(
            "ledc: steering output failed; safe reboot",
            "[FAIL]",
            TFT_RED
        );

        Serial.flush();
        delay(1500);
        esp_restart();
    }

    steeringServo.configure(
        settings.getServoCenter(),
        settings.getServoReverse(),
        settings.getServoTravel(),
        settings.getServoQuiet(),
        settings.isSteeringCalibrated(),
        settings.getSteeringMin(),
        settings.getSteeringCenter(),
        settings.getSteeringMax()
    );

    steeringServo.center();

    Serial.println("SERVO OK");

    bootConsole.log(
        #if defined(OPENDRIFT_INPUT_CRSF)
        #if defined(OPENDRIFT_BOARD_MATRIX)
        "ledc: steering servo output attached on gpio1"
        #elif defined(OPENDRIFT_AMOLED_V2)
        "ledc: steering servo output attached on gpio15"
        #else
        "ledc: steering servo output attached on gpio15"
        #endif
        #else
        #if defined(OPENDRIFT_BOARD_MATRIX)
        "ledc: steering servo output attached on gpio3"
        #elif defined(OPENDRIFT_AMOLED_V2)
        "ledc: steering servo output attached on gpio1"
        #else
        "ledc: steering servo output attached on gpio17"
        #endif
        #endif
    );

    //-------------------
    // RADIO
    //-------------------

    #if defined(OPENDRIFT_INPUT_CRSF)
    bool crsfOk = false;
    bool crsfReaderOk = false;

    crsfParameters.begin(
        crsf,
        settings,
        gyro,
        steeringRadio,
        steeringServo
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    crsfParameters.setBlackboxArchive(blackboxArchive);
    #endif

    bool steeringRadioOk = steeringRadio.beginExternal();
    bool throttleRadioOk = throttleRadio.beginExternal();
    bool gainRadioOk = gainRadio.beginExternal();
    bool sharedPinOk = configurePin18Mode();

    throttleOutput.configure(
        1500,
        false,
        100,
        0
    );

    throttleOutputActive =
        throttleOutput.begin(
            CRSF_THROTTLE_OUTPUT_PIN,
            settings.getThrottleOutputHz()
        );

    if(throttleOutputActive)
    {
        throttleOutput.writeMicroseconds(1500);
    }

    bool throttleOutputOk = throttleOutputActive;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    bool auxOutputsOk =
        auxChannelOutputs.begin(settings);
    #endif
    #else
    bool steeringRadioOk =
        steeringRadio.begin(
            RADIO_STEERING_PIN
        );

    bool throttleRadioOk =
        throttleRadio.begin(
            RADIO_THROTTLE_PIN
        );

    bool sharedPinOk =
        configurePin18Mode();
    #endif

    bool radioOk =
        steeringRadioOk &&
        throttleRadioOk &&
        sharedPinOk
        #if defined(OPENDRIFT_INPUT_CRSF)
        && gainRadioOk && throttleOutputOk
        #endif
        ;

    bootConsole.log(
        #if defined(OPENDRIFT_INPUT_CRSF)
        "crsf: uart startup deferred until UI ready",
        #else
        "rc-input: steering and throttle channels armed",
        #endif
        #if defined(OPENDRIFT_INPUT_CRSF)
        "[WAIT]",
        TFT_CYAN
        #else
        radioOk ? "[ OK ]" : "[FAIL]",
        radioOk ? TFT_GREEN : TFT_RED
        #endif
    );

    #if defined(OPENDRIFT_INPUT_CRSF)
    bootConsole.log(
        #if defined(OPENDRIFT_BOARD_MATRIX)
        "ledc: esc neutral output attached on gpio2",
        #elif defined(OPENDRIFT_CRSF_V2_THROTTLE_GPIO8)
        "ledc: esc neutral output attached on gpio8",
        #else
        "ledc: esc neutral output attached on gpio16",
        #endif
        throttleOutputOk ? "[ OK ]" : "[FAIL]",
        throttleOutputOk ? TFT_GREEN : TFT_RED
    );
    #endif

    #if defined(OPENDRIFT_INPUT_CRSF)
    bootConsole.log(
        "crsf: channel adapters and pin routing ready",
        radioOk ? "[ OK ]" : "[FAIL]",
        radioOk ? TFT_GREEN : TFT_RED
    );
    #endif

    #if defined(OPENDRIFT_INPUT_CRSF) && defined(OPENDRIFT_BOARD_AMOLED_164)
    bootConsole.log(
        #if defined(OPENDRIFT_AMOLED_V2)
        "mcpwm: auxiliary outputs ready on gpio3-8",
        #else
        "mcpwm: auxiliary outputs ready on gpio1-8",
        #endif
        auxOutputsOk ? "[ OK ]" : "[WARN]",
        auxOutputsOk ? TFT_GREEN : TFT_YELLOW
    );
    #endif

    bootConsole.log(
        #if defined(OPENDRIFT_INPUT_CRSF)
        #if defined(OPENDRIFT_BOARD_MATRIX)
        "gpio3/4: crsf rx/tx; gpio1/2: servo/esc out"
        #elif defined(OPENDRIFT_AMOLED_V2)
        "gpio1/2: crsf rx/tx; gpio15/16: servo/esc out"
        #else
        "gpio17/18: crsf rx/tx; gpio15/16: servo/esc out"
        #endif
        #else
        pin18ThrottleOutputMode
        #if defined(OPENDRIFT_BOARD_MATRIX)
        ? "gpio4: throttle passthrough output"
        : "gpio4: gyro gain adjustment input"
        #elif defined(OPENDRIFT_AMOLED_V2)
        ? "gpio2: throttle passthrough output"
        : "gpio2: gyro gain adjustment input"
        #else
        ? "gpio18: throttle passthrough output"
        : "gpio18: gyro gain adjustment input"
        #endif
        #endif
    );

    #if defined(OPENDRIFT_INPUT_CRSF)
    Serial.println("CRSF pins staged; ESC output awaiting neutral");
    #else
    Serial.println("Radio inputs initialized");
    #endif

    //-------------------
    // GYRO CONTROLLER
    //-------------------

    bool gyroOk =
        gyro.begin();

    bootConsole.log(
        "opendrift-gyro: controller state initialized",
        gyroOk ? "[ OK ]" : "[FAIL]",
        gyroOk ? TFT_GREEN : TFT_RED
    );

    gyro.setGain(
        settings.getGain()
    );

    gyro.setDeadband(
        settings.getDeadband()
    );

    gyro.setSmoothing(
        settings.getGyroSmoothing()
    );

    gyro.setMaxCorrection(
        settings.getGyroMaxCorrection() * 10
    );

    gyro.setIntegralGain(
        settings.getGyroIntegralGain()
    );

    gyro.setIntegralLimit(
        settings.getGyroIntegralLimit()
    );

    gyro.setHoldBoost(
        settings.getGyroHoldBoost()
    );

    gyro.setCounterSteerAssist(
        settings.getGyroCounterSteerAssist()
    );

    gyro.setTransitionSpeed(
        settings.getGyroTransitionSpeed()
    );

    gyro.setPredictionStrength(
        settings.getPredictionStrength()
    );

    gyro.setDriverPriority(
        settings.getDriverPriority()
    );

    gyro.setHuntStrength(
        settings.getGyroHuntStrength()
    );

    gyro.setAntiWobbleScale(
        settings.getAntiWobbleScale()
    );

    gyro.setControlLoopHz(
        settings.getControlLoopHz()
    );

    Serial.printf(
        "Hunt notch: %s (%s scale, %.1f Hz initial, Q 1.25, control %d Hz)\n",
        gyro.isHuntNotchConfigured() ? "READY" : "FAILED",
        gyro.getAntiWobbleScale() == 1 ? "micro" : "1/10",
        gyro.getHuntNotchCenter(),
        gyro.getControlLoopHz()
    );

    //-------------------
    // CALIBRATION
    //-------------------

    bootConsole.log(
        "qmi8658: measuring stationary gyro bias",
        "[....]",
        TFT_CYAN
    );

    #if defined(OPENDRIFT_BOARD_MATRIX)
    matrixStatus.setState(
        MatrixStatus::State::Calibrating
    );
    #endif

    delay(1500);

    for(uint8_t attempt = 1; attempt <= 2; attempt++)
    {
        if(attempt > 1)
        {
            delay(500);
        }

        gyro.startCalibration(controlLoopHz / 2);

        while(gyro.isCalibrating())
        {
            imu.update();

            if(!imu.isYawValid() || !imu.lastGyroReadOk())
            {
                gyro.abortCalibration();
                break;
            }

            gyro.update(
                settings.getGyroReverse()
                ? -imu.getYawRate()
                : imu.getYawRate(),
                1500,
                false,
                1500,
                false
            );

            delay(controlLoopPeriodMs);
        }

        if(
            gyro.getCalibrationState() ==
            GyroController::CALIBRATION_OK
        )
        {
            break;
        }
    }

    bool gyroBiasOk =
        gyro.getCalibrationState() ==
        GyroController::CALIBRATION_OK;

    Serial.println(
        gyroBiasOk
        ? "Gyro calibrated"
        : "Gyro bias rejected; zero offset retained"
    );

    bootConsole.log(
        gyroBiasOk
        ? "qmi8658: gyro bias calibration complete"
        : "qmi8658: gyro bias rejected (movement), using zero offset",
        gyroBiasOk ? "[ OK ]" : "[WARN]",
        gyroBiasOk ? TFT_GREEN : TFT_YELLOW
    );

    delay(500);

    //-------------------
    // BLACKBOX
    //-------------------

    if(settings.getBlackboxEnabled())
    {
        updateBlackboxAvailability();

        bootConsole.log(
            blackbox.isReady()
            ? "psram: blackbox recorder allocated"
            : "psram: blackbox recorder unavailable",
            blackbox.isReady() ? "[ OK ]" : "[WARN]",
            blackbox.isReady() ? TFT_GREEN : TFT_YELLOW
        );
    }
    else
    {
        Serial.println("Blackbox logging disabled");

        bootConsole.log(
            "psram: blackbox recorder disabled",
            "[SKIP]",
            0x8410
        );
    }

    //-------------------
    // WIFI
    //-------------------

    wifi.begin(
        settings.getWifiSsid(),
        password,
        settings.getWifiEnabled()
    );

    wifi.setTimeout(
        settings.getWifiTimeout()
    );

    if(wifi.isEnabled())
    {
        IPAddress IP =
            WiFi.softAPIP();

        Serial.print("WiFi IP: ");
        Serial.println(IP);

        char wifiMessage[48];

        snprintf(
            wifiMessage,
            sizeof(wifiMessage),
            "wlan0: AP OpenDrift ready at %s",
            IP.toString().c_str()
        );

        bootConsole.log(
            wifiMessage
        );

        webConfig.begin(
            settings,
            gyro,
            steeringRadio,
            gainRadio,
            throttleRadio,
            blackbox
        );

        bootConsole.log(
            "httpd: web configurator listening"
        );
    }
    else
    {
        bootConsole.log(
            "wlan0: interface disabled by settings",
            "[SKIP]",
            0x8410
        );
    }

    #if !defined(OPENDRIFT_HEADLESS)
    //-------------------
    // UI
    //-------------------

    bootConsole.log(
        "systemd[1]: Reached target OpenDrift UI"
    );

    delay(350);

    bootConsole.end();

    ui.begin(
        &lcd,
        gyro,
        wifi,
        settings,
        steeringRadio,
        gainRadio,
        steeringServo
    );

    ui.setThrottleRadio(
        throttleRadio
    );

    ui.setCalibrationCallback(
        requestGyroCalibration
    );

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    ui.setUsbMaintenanceCallback(
        requestUsbMaintenance
    );

    if(firmwareUpdateCompletedAtBoot)
    {
        ui.showFirmwareUpdateCompleted();
        Serial.println("USB firmware update completed successfully");
    }
    #endif

    touch.update();
    #else
    bootConsole.log(
        "systemd[1]: Reached target OpenDrift headless"
    );
    bootConsole.end();
    #endif

    i2cBusMutex =
        xSemaphoreCreateMutex();

    #if defined(OPENDRIFT_INPUT_CRSF)
    // Start the live receiver only after every boot-time peripheral and shared
    // resource is stable. This prevents a continuously streaming F1000 link
    // from competing with AMOLED/IMU/UI initialization on core 0.
    crsfOk = crsf.begin(
        CRSF_RX_PIN,
        CRSF_TX_PIN
    );

    BaseType_t crsfTaskStarted =
        crsfOk
        ? xTaskCreatePinnedToCore(
            crsfTask,
            "OpenDriftCRSF",
            4096,
            nullptr,
            3,
            &crsfTaskHandle,
            0
        )
        : pdFAIL;

    crsfReaderOk =
        crsfTaskStarted == pdPASS;

    Serial.printf(
        "CRSF deferred startup: UART=%s reader=%s\n",
        crsfOk ? "OK" : "FAIL",
        crsfReaderOk ? "OK" : "FAIL"
    );
    #endif

    BaseType_t taskStarted =
        xTaskCreatePinnedToCore(
            controlTask,
            "OpenDriftControl",
            8192,
            nullptr,
            4,
            &controlTaskHandle,
            1
        );

    if(taskStarted == pdPASS)
    {
        Serial.printf(
            "Controller: %lu Hz task online\n",
            (unsigned long)controlLoopHz
        );
    }
    else
    {
        Serial.println("Controller: task start failed");

        #if defined(OPENDRIFT_BOARD_MATRIX)
        matrixStatus.setState(MatrixStatus::State::Error);
        #endif
    }
}

void loop()
{
    static unsigned long lastHeartbeatMs = 0;

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    if(usbMaintenanceActive)
    {
        const uint32_t updateRevision =
            usbMaintenance.getUpdateRevision();
        if(updateRevision != lastUsbUpdateRevision)
        {
            lastUsbUpdateRevision = updateRevision;
            drawUsbMaintenanceScreen();
        }

        touch.update();
        const bool touched = touch.isTouched();

        if(
            touched &&
            !usbMaintenanceLastTouch &&
            touch.getX() >= 118 && touch.getX() <= 338 &&
            touch.getY() >= 216 && touch.getY() <= 262 &&
            usbMaintenance.getUpdateState() !=
                UsbMaintenance::UPDATE_WRITING
        )
        {
            usbMaintenanceCanvas.fillScreen(TFT_BLACK);
            usbMaintenanceCanvas.setTextDatum(middle_center);
            usbMaintenanceCanvas.setTextColor(TFT_WHITE);
            usbMaintenanceCanvas.setTextSize(2);
            usbMaintenanceCanvas.drawString(
                "Disconnecting USB drives...",
                228,
                140
            );
            usbMaintenanceCanvas.setTextDatum(top_left);
            flushUsbMaintenanceCanvas();

            usbMaintenance.prepareForRestart(onboardStorage);
            delay(250);
            ESP.restart();
        }

        usbMaintenanceLastTouch = touched;
        delay(10);
        return;
    }

    if(usbMaintenanceRequested)
    {
        usbMaintenanceRequested = false;

        if(controlTaskHandle != nullptr) vTaskSuspend(controlTaskHandle);
        #if defined(OPENDRIFT_INPUT_CRSF)
        if(crsfTaskHandle != nullptr) vTaskSuspend(crsfTaskHandle);
        #endif

        steeringServo.center();
        throttleOutput.writeMicroseconds(1500);
        delay(100);
        steeringServo.end();
        throttleOutput.end();

        UsbMaintenance::armNextBoot();
        Serial.println("Restarting into USB maintenance mode");
        Serial.flush();
        delay(100);
        ESP.restart();
    }
    #endif

    #if defined(OPENDRIFT_INPUT_CRSF)
    crsfParameters.update();

    if(crsfParameters.consumeSettingsChanged())
    {
        #if !defined(OPENDRIFT_HEADLESS)
        ui.requestRefresh();
        #endif
    }
    #endif

    if(millis() - lastHeartbeatMs > 5000)
    {
        lastHeartbeatMs =
            millis();

        Serial.println("OpenDrift heartbeat");

        #if defined(OPENDRIFT_INPUT_CRSF)
        Serial.printf(
            "CRSF bytes=%lu frames=%lu channels=%lu crc=%lu age=%lu ms LQ=%u SNR=%d throttle=%s\n",
            (unsigned long)crsf.getReceivedByteCount(),
            (unsigned long)crsf.getValidFrameCount(),
            (unsigned long)crsf.getChannelFrameCount(),
            (unsigned long)crsf.getCrcErrorCount(),
            (unsigned long)crsf.getFrameAgeMs(),
            crsf.getUplinkLinkQuality(),
            crsf.getUplinkSnr(),
            (crsfThrottleArmed && crsfThrottleOutputArmed)
            ? "ARMED"
            : "LOCKED"
        );
        #endif
    }

    #if !defined(OPENDRIFT_HEADLESS)
    if(i2cBusMutex != nullptr)
    {
        xSemaphoreTake(
            i2cBusMutex,
            portMAX_DELAY
        );
    }

    touch.update();

    if(i2cBusMutex != nullptr)
    {
        xSemaphoreGive(
            i2cBusMutex
        );
    }
    #endif

    //-------------------
    // SETTINGS
    //-------------------

    settings.update();

    updateBlackboxAvailability();

    //-------------------
    // WIFI
    //-------------------

    wifi.update();

    wifi.setTimeout(
        settings.getWifiTimeout()
    );

    if(
        wifi.isEnabled() &&
        !webConfig.isRunning()
    )
    {
        webConfig.begin(
            settings,
            gyro,
            steeringRadio,
            gainRadio,
            throttleRadio,
            blackbox
        );
    }

    if(wifi.isEnabled())
    {
        webConfig.update();
    }

    // Web and CRSF writes become visible immediately. AMOLED touch uses the
    // same rotation so its hit targets continue to follow the rendered UI.
    applyDisplayRotation();

    configurePin18Mode();

    #if defined(OPENDRIFT_INPUT_CRSF)
    bool crsfThrottleSignal =
        crsfThrottleSignalSnapshot;

    float crsfThrottlePulse =
        crsfThrottlePulseSnapshot;

    updateCrsfThrottleOutput(
        crsfThrottlePulse,
        crsfThrottleSignal
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    auxChannelOutputs.update(
        settings,
        crsf,
        crsf.hasSignal(CRSF_SIGNAL_TIMEOUT_MS)
    );
    #endif
    #else
    if(
        pin18ThrottleOutputMode &&
        throttleRadio.hasSignal()
    )
    {
        if(!throttleOutputActive)
        {
            throttleOutput.configure(
                1500,
                false,
                100,
                0
            );

            throttleOutputActive =
                throttleOutput.begin(
                    SHARED_GAIN_THROTTLE_PIN,
                    settings.getThrottleOutputHz()
                );
        }

    }
    else if(
        pin18ThrottleOutputMode &&
        throttleOutputActive
    )
    {
        // Removing the PWM signal lets the ESC's own signal-loss
        // failsafe take over instead of holding the last throttle value.
        throttleOutput.end();
        throttleOutputActive = false;

        pinMode(
            SHARED_GAIN_THROTTLE_PIN,
            INPUT_PULLDOWN
        );
    }
    #endif

    #if !defined(OPENDRIFT_HEADLESS)
    //-------------------
    // UI
    //-------------------

    ui.update(
        touch,
        gyro,
        imu,
        wifi,
        settings,
        steeringRadio,
        gainRadio
    );
    #endif

    ControlTelemetry telemetry;

    portENTER_CRITICAL(
        &controlTelemetryMux
    );

    telemetry =
        controlTelemetry;

    portEXIT_CRITICAL(
        &controlTelemetryMux
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    static uint32_t parkedCandidateSinceMs = 0;
    const float archiveThrottlePulse =
        #if defined(OPENDRIFT_INPUT_CRSF)
        crsfThrottlePulseSnapshot;
        #else
        throttleRadio.hasSignal()
            ? throttleRadio.getPulseWidthFloat()
            : 1500.0f;
        #endif

    const bool archiveParkCandidate =
        settings.getBlackboxEnabled() &&
        telemetry.steeringSignal &&
        telemetry.throttleSignal &&
        fabsf(archiveThrottlePulse - 1500.0f) <= 30.0f &&
        fabsf(telemetry.yaw) <= 4.0f &&
        imu.getAccelDelta() <= 0.025f &&
        gyro.getControlPhase() == 0;

    if(archiveParkCandidate)
    {
        if(parkedCandidateSinceMs == 0)
        {
            parkedCandidateSinceMs = millis();
        }
    }
    else
    {
        parkedCandidateSinceMs = 0;
    }

    const bool archiveParked =
        parkedCandidateSinceMs != 0 &&
        millis() - parkedCandidateSinceMs >= 2000;

    blackboxArchive.update(archiveParked);
    #endif

    #if defined(OPENDRIFT_BOARD_MATRIX)
    matrixStatus.update(
        telemetry.steeringSignal,
        wifi.isEnabled(),
        settings.getBlackboxEnabled() && blackbox.isReady()
    );
    #endif

    //-------------------
    // BLACKBOX LOG
    //-------------------

    const bool blackboxInputReady =
        telemetry.steeringSignal
        #if defined(OPENDRIFT_USB_MAINTENANCE)
        // The private maintenance build doubles as a bench logger. When no
        // receiver is present, capture IMU/controller state with failsafe
        // neutral inputs so SD and USB maintenance can be tested standalone.
        || (!telemetry.steeringSignal && !telemetry.throttleSignal)
        #endif
        ;

    if(
        settings.getBlackboxEnabled() &&
        blackbox.isReady() &&
        blackboxInputReady &&
        millis() - lastBlackboxLog >= 50
    )
    {
        lastBlackboxLog =
            millis();

        blackbox.log(
            lastBlackboxLog,
            telemetry.yaw,
            gyro.getFilteredYaw(),
            imu.getGyroX(),
            imu.getGyroY(),
            imu.getAccelX(),
            imu.getAccelY(),
            imu.getAccelZ(),
            imu.getAccelMagnitude(),
            imu.getAccelDelta(),
            imu.getTiltRate(),
            imu.getSurfaceDisturbanceScore(),
            telemetry.requestedGyroCorrection,
            telemetry.limitedGyroCorrection,
            telemetry.appliedGyroCorrection,
            telemetry.correctionSaturated,
            steeringRadio.getPulseWidth(),
            telemetry.steeringCommand,
            telemetry.servoCommand,
            settings.getServoQuiet(),
            throttleRadio.getPulseWidth(),
            gainRadio.getPulseWidth(),
            gyro.getGain(),
            settings.getDriverPriority(),
            gyro.getDriverPriorityScale(),
            gyro.getEffectiveDirectGain(),
            settings.getDeadband(),
            settings.getGyroMaxCorrection(),
            settings.getGyroSmoothing(),
            imu.getGyroLpfMode(),
            settings.getGyroIntegralGain(),
            settings.getGyroIntegralLimit(),
            gyro.getIntegralCorrection(),
            settings.getGyroHoldBoost(),
            settings.getGyroCounterSteerAssist(),
            settings.getPredictionStrength(),
            gyro.getPredictedYaw(),
            gyro.getDriftReferenceYaw(),
            gyro.getReferenceError(),
            gyro.getReferenceLock(),
            gyro.getThrottlePrediction(),
            gyro.getDirectCorrection(),
            gyro.getCounterSteerCorrection(),
            gyro.getMemoryFeedback(),
            gyro.getDriverActivityBlend(),
            gyro.getThrottlePredictionBlend(),
            gyro.getSteeringActivity(),
            gyro.getControlPhase(),
            gyro.getSettledBlend(),
            gyro.getThrottleTransient(),
            telemetry.steeringSignal,
            telemetry.throttleSignal,
            gainRadio.hasSignal(),
            pin18ThrottleOutputMode,
            settings.getGyroTransitionSpeed(),
            gyro.getTransitionSpeedBlend(),
            gyro.getTransitionSlewCorrection(),
            gyro.getHuntSuppression(),
            gyro.getHuntFrequency(),
            gyro.getTransitionAuthorityBlend(),
            gyro.getThrottleLiftBlend(),
            gyro.getTransitionPredictionScale(),
            gyro.getHuntResidual(),
            gyro.getHuntRemovedCorrection(),
            gyro.getHuntConsistentHalfCycles(),
            gyro.getHuntLatch(),
            settings.getGyroHuntStrength(),
            gyro.getHuntResidualEnvelope(),
            gyro.getHuntNotchCenter()
        );
    }

    delay(1);
}
 
