#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_system.h>

#include "LGFX_OpenDrift.hpp"
#include "IMU.h"
#include "Servo.h"
#include "EscOutput.h"
#include "GyroController.h"
#include "Touch.h"
#include "UI.h"
#include "WiFiManager.h"
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

LGFX lcd;

IMU imu;

ServoOutput steeringServo;

EscOutput throttleOutput;

GyroController gyro;

Touch touch;

UI ui;

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

unsigned long lastBlackboxLog = 0;

bool blackboxStarted = false;

bool lastBlackboxEnabled = false;

bool blackboxStartAttempted = false;

static constexpr uint32_t CONTROL_LOOP_HZ = 250;
static constexpr uint32_t CONTROL_LOOP_PERIOD_MS =
    1000 / CONTROL_LOOP_HZ;

#if defined(OPENDRIFT_BOARD_AMOLED_164)
static constexpr uint8_t STARTUP_RETRY_COUNT = 3;
static constexpr int I2C_SDA_PIN = 47;
static constexpr int I2C_SCL_PIN = 48;
#endif

struct ControlTelemetry
{
    float yaw = 0.0f;
    int rawGyroCorrection = 0;
    int gyroCorrection = 0;
    int steeringCommand = 1500;
    int servoCommand = 1500;
    bool steeringSignal = false;
    bool throttleSignal = false;
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
#if defined(OPENDRIFT_AMOLED_V2)
// V2 connects the IMU and touch interrupt outputs to GPIO17/18. Keep the
// receiver UART off those lines to prevent electrical contention.
#define SERVO_OUTPUT_PIN 15
#define CRSF_RX_PIN 1
#define CRSF_TX_PIN 2
#define CRSF_THROTTLE_OUTPUT_PIN 16
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
#if defined(OPENDRIFT_AMOLED_V2)
#define SERVO_OUTPUT_PIN 1
#else
#define SERVO_OUTPUT_PIN 17
#endif
#define RADIO_STEERING_PIN 15
#define RADIO_THROTTLE_PIN 16
#endif
#if defined(OPENDRIFT_AMOLED_V2)
#define SHARED_GAIN_THROTTLE_PIN 2
#else
#define SHARED_GAIN_THROTTLE_PIN 18
#endif

bool pin18ModeConfigured = false;

bool pin18ThrottleOutputMode = false;

bool throttleOutputActive = false;

#if defined(OPENDRIFT_INPUT_CRSF)
bool crsfThrottleArmed = false;
bool lastCrsfSignal = false;
uint32_t crsfThrottleNeutralSinceMs = 0;
volatile bool crsfThrottleSignalSnapshot = false;
volatile int crsfThrottlePulseSnapshot = 1500;
#endif

const float radioGainMin = 0.5f;
const float radioGainMax = 3.0f;

const char* ssid = "OpenDrift";
const char* password = "opendrift";


#if defined(OPENDRIFT_BOARD_AMOLED_164)
static constexpr float AMOLED_BOOT_LOG_TEXT_SIZE = 1.15f;
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
#endif



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
            "OpenDrift CRSF verbose boot",
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
            "control kernel 1.0.4 crsf  ttyOD0",
            #else
            "control kernel 1.0.4 pwm  ttyOD0",
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
    int throttlePulse,
    bool signalValid
)
{
    if(!signalValid)
    {
        if(throttleOutputActive)
        {
            // Active neutral is deterministic and does not depend on the
            // receiver or ESC having matching failsafe configuration.
            throttleOutput.writeMicroseconds(1500);
        }

        crsfThrottleArmed = false;
        crsfThrottleNeutralSinceMs = 0;

        return;
    }

    bool throttleNeutral =
        abs(throttlePulse - 1500) <=
        CRSF_THROTTLE_NEUTRAL_BAND_US;

    if(!crsfThrottleArmed)
    {
        if(!throttleNeutral)
        {
            crsfThrottleNeutralSinceMs = 0;

            if(throttleOutputActive)
            {
                throttleOutput.writeMicroseconds(1500);
            }

            return;
        }

        if(crsfThrottleNeutralSinceMs == 0)
        {
            crsfThrottleNeutralSinceMs = millis();
            return;
        }

        if(
            millis() - crsfThrottleNeutralSinceMs <
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
                    50
                );
        }

        crsfThrottleArmed = throttleOutputActive;

        if(!crsfThrottleArmed)
        {
            crsfThrottleNeutralSinceMs = 0;
            return;
        }

        Serial.println(
            "CRSF throttle output armed after neutral hold"
        );
    }

    throttleOutput.writeMicroseconds(
        constrain(
            throttlePulse,
            1000,
            2000
        )
    );
}
#endif

int mapSteeringPulse(
    int pulse,
    Settings& settings
)
{
    int steeringMin =
        settings.getSteeringMin();

    int steeringCenter =
        settings.getSteeringCenter();

    int steeringMax =
        settings.getSteeringMax();

    if(abs(pulse - steeringCenter) <= 4)
    {
        return 1500;
    }

    if(
        steeringCenter <= steeringMin ||
        steeringCenter >= steeringMax
    )
    {
        return constrain(
            pulse,
            1000,
            2000
        );
    }

    if(pulse < steeringCenter)
    {
        return map(
            constrain(
                pulse,
                steeringMin,
                steeringCenter
            ),
            steeringMin,
            steeringCenter,
            1000,
            1500
        );
    }

    return map(
        constrain(
            pulse,
            steeringCenter,
            steeringMax
        ),
        steeringCenter,
        steeringMax,
        1500,
        2000
    );
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



int constrainToRadioSteeringTravel(
    int steeringCommand,
    Settings& settings
)
{
    int travel =
        settings.getRadioSteeringTravel();

    int maxOffset =
        (500 * travel)
        /
        100;

    return constrain(
        steeringCommand,
        1500 - maxOffset,
        1500 + maxOffset
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
        radioGainMin +
        ((radioGainMax - radioGainMin) * normalized);
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
            crsf.getChannelMicroseconds(
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
        gyro.setGain(
            settings.getGain()
        );
    }

    gyro.setDeadband(
        settings.getDeadband()
    );

    gyro.setSmoothing(
        settings.getGyroSmoothing()
    );

    gyro.setMaxCorrection(
        settings.getGyroMaxCorrection()
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

    gyro.setTailSlideSpeed(
        settings.getGyroTailSlideSpeed()
    );

    gyro.setPredictionStrength(
        settings.getPredictionStrength()
    );

    if(i2cBusMutex != nullptr)
    {
        xSemaphoreTake(
            i2cBusMutex,
            portMAX_DELAY
        );
    }

    imu.update();

    if(i2cBusMutex != nullptr)
    {
        xSemaphoreGive(
            i2cBusMutex
        );
    }

    #if defined(OPENDRIFT_INPUT_CRSF)
    bool steeringSignal = crsfSignal;
    bool throttleSignal = crsfSignal;
    #else
    bool steeringSignal = steeringRadio.hasSignal();
    bool throttleSignal = throttleRadio.hasSignal();
    #endif

    int throttlePulse =
        throttleSignal
        ? throttleRadio.getPulseWidth()
        : 1500;

    int steeringCommand = 1500;

    if(steeringSignal)
    {
        steeringCommand =
            mapSteeringPulse(
                steeringRadio.getPulseWidth(),
                settings
            );

        steeringCommand =
            applyRadioSteeringTravel(
                steeringCommand,
                settings
            );
    }
    float yaw =
        imu.getYawRate();

    int gyroCommand =
        gyro.update(
            yaw,
            steeringCommand,
            steeringSignal,
            throttlePulse,
            throttleSignal
        );

    int gyroCorrection =
        gyroCommand - 1500;

    if(settings.getGyroReverse())
    {
        gyroCorrection =
            -gyroCorrection;
    }

    int rawGyroCorrection =
        gyroCorrection;

    int servoCommand =
        steeringServo.getPosition();

    if(steeringSignal)
    {
        servoCommand =
            constrainToRadioSteeringTravel(
                steeringCommand + gyroCorrection,
                settings
            );

        steeringServo.configure(
            settings.getServoCenter(),
            settings.getServoReverse(),
            settings.getServoTravel(),
            settings.getServoQuiet()
        );

        steeringServo.writeMicroseconds(
            servoCommand
        );
    }

    #if defined(OPENDRIFT_INPUT_CRSF)
    else if(lastCrsfSignal)
    {
        // Do not hold the last steering command after a receiver loss.
        steeringServo.center();
        servoCommand = steeringServo.getPosition();
    }

    lastCrsfSignal = steeringSignal;

    // The main loop owns throttle PWM attachment and removal. ESP32Servo's
    // dynamic LEDC allocation must not run inside this high-priority task.
    crsfThrottlePulseSnapshot = throttlePulse;
    crsfThrottleSignalSnapshot = throttleSignal;
    #endif

    ControlTelemetry nextTelemetry;

    nextTelemetry.yaw = yaw;
    nextTelemetry.rawGyroCorrection =
        rawGyroCorrection;
    nextTelemetry.gyroCorrection =
        gyroCorrection;
    nextTelemetry.steeringCommand =
        steeringCommand;
    nextTelemetry.servoCommand =
        servoCommand;
    nextTelemetry.steeringSignal =
        steeringSignal;
    nextTelemetry.throttleSignal =
        throttleSignal;

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
            CONTROL_LOOP_PERIOD_MS
        );

    while(true)
    {
        runControlIteration();

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

        Serial.println("Blackbox logging OK");
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

    delay(500);

    Serial.println("OpenDrift Starting");

    esp_reset_reason_t resetReason =
        esp_reset_reason();

    Serial.print("Reset reason: ");
    Serial.print(resetReasonName(resetReason));
    Serial.print(" (");
    Serial.print((int)resetReason);
    Serial.println(")");

    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    #endif

    bool displayOk = false;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    for(uint8_t attempt = 1; attempt <= STARTUP_RETRY_COUNT; attempt++)
    {
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

    bootConsole.log(
        "nvs: mounted OpenDrift settings store",
        settingsOk ? "[ OK ]" : "[WARN]",
        settingsOk ? TFT_GREEN : TFT_YELLOW
    );

    //-------------------
    // IMU
    //-------------------

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

    //-------------------
    // SERVO
    //-------------------

    if(!steeringServo.begin(
        SERVO_OUTPUT_PIN,
        CONTROL_LOOP_HZ
    ))
    {
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
        settings.getServoQuiet()
    );

    steeringServo.center();

    Serial.println("SERVO OK");

    bootConsole.log(
        #if defined(OPENDRIFT_INPUT_CRSF)
        #if defined(OPENDRIFT_AMOLED_V2)
        "ledc: steering servo output attached on gpio15"
        #else
        "ledc: steering servo output attached on gpio15"
        #endif
        #else
        #if defined(OPENDRIFT_AMOLED_V2)
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
    bool crsfOk =
        crsf.begin(
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

    bool crsfReaderOk =
        crsfTaskStarted == pdPASS;

    crsfParameters.begin(
        crsf,
        settings,
        gyro
    );

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
            50
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
        && gainRadioOk && crsfOk && crsfReaderOk && throttleOutputOk
        #endif
        ;

    bootConsole.log(
        #if defined(OPENDRIFT_INPUT_CRSF)
        "crsf: uart 420k channel decoder online",
        #else
        "rc-input: steering and throttle channels armed",
        #endif
        #if defined(OPENDRIFT_INPUT_CRSF)
        crsfOk && crsfReaderOk ? "[ OK ]" : "[FAIL]",
        crsfOk && crsfReaderOk ? TFT_GREEN : TFT_RED
        #else
        radioOk ? "[ OK ]" : "[FAIL]",
        radioOk ? TFT_GREEN : TFT_RED
        #endif
    );

    #if defined(OPENDRIFT_INPUT_CRSF)
    bootConsole.log(
        "ledc: esc neutral output attached on gpio16",
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
        #if defined(OPENDRIFT_AMOLED_V2)
        "gpio1/2: crsf rx/tx; gpio15/16: servo/esc out"
        #else
        "gpio17/18: crsf rx/tx; gpio15/16: servo/esc out"
        #endif
        #else
        pin18ThrottleOutputMode
        #if defined(OPENDRIFT_AMOLED_V2)
        ? "gpio2: throttle passthrough output"
        : "gpio2: gyro gain adjustment input"
        #else
        ? "gpio18: throttle passthrough output"
        : "gpio18: gyro gain adjustment input"
        #endif
        #endif
    );

    #if defined(OPENDRIFT_INPUT_CRSF)
    Serial.println("CRSF input initialized; ESC output awaiting neutral");
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
        settings.getGyroMaxCorrection()
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

    gyro.setTailSlideSpeed(
        settings.getGyroTailSlideSpeed()
    );

    gyro.setPredictionStrength(
        settings.getPredictionStrength()
    );

    //-------------------
    // CALIBRATION
    //-------------------

    bootConsole.log(
        "qmi8658: measuring stationary gyro bias",
        "[....]",
        TFT_CYAN
    );

    delay(2000);

    imu.update();

    gyro.calibrate(
        imu.getYawRate()
    );

    Serial.println("Gyro calibrated");

    bootConsole.log(
        "qmi8658: gyro bias calibration complete"
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
            ? "ffat: blackbox recorder mounted"
            : "ffat: blackbox recorder unavailable",
            blackbox.isReady() ? "[ OK ]" : "[WARN]",
            blackbox.isReady() ? TFT_GREEN : TFT_YELLOW
        );
    }
    else
    {
        Serial.println("Blackbox logging disabled");

        bootConsole.log(
            "ffat: blackbox recorder disabled",
            "[SKIP]",
            0x8410
        );
    }

    //-------------------
    // WIFI
    //-------------------

    wifi.begin(
        ssid,
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
        gainRadio
    );

    #if defined(OPENDRIFT_INPUT_CRSF)
    ui.setThrottleRadio(
        throttleRadio
    );
    #endif

    touch.update();

    i2cBusMutex =
        xSemaphoreCreateMutex();

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

    Serial.println(
        taskStarted == pdPASS
        ?
        "Controller: 250 Hz task online"
        :
        "Controller: task start failed"
    );
}

void loop()
{
    static unsigned long lastHeartbeatMs = 0;

    #if defined(OPENDRIFT_INPUT_CRSF)
    crsfParameters.update();

    if(crsfParameters.consumeSettingsChanged())
    {
        ui.requestRefresh();
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
            crsfThrottleArmed ? "ARMED" : "LOCKED"
        );
        #endif
    }

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

    configurePin18Mode();

    #if defined(OPENDRIFT_INPUT_CRSF)
    bool crsfThrottleSignal =
        crsfThrottleSignalSnapshot;

    int crsfThrottlePulse =
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
                    SHARED_GAIN_THROTTLE_PIN
                );
        }

        if(throttleOutputActive)
        {
            throttleOutput.writeMicroseconds(
                throttleRadio.getPulseWidth()
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

    ControlTelemetry telemetry;

    portENTER_CRITICAL(
        &controlTelemetryMux
    );

    telemetry =
        controlTelemetry;

    portEXIT_CRITICAL(
        &controlTelemetryMux
    );

    //-------------------
    // BLACKBOX LOG
    //-------------------

    if(
        settings.getBlackboxEnabled() &&
        blackbox.isReady() &&
        telemetry.steeringSignal &&
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
            telemetry.rawGyroCorrection,
            telemetry.gyroCorrection,
            steeringRadio.getPulseWidth(),
            telemetry.steeringCommand,
            telemetry.servoCommand,
            settings.getServoQuiet(),
            throttleRadio.getPulseWidth(),
            gainRadio.getPulseWidth(),
            gyro.getGain(),
            settings.getDeadband(),
            settings.getGyroMaxCorrection(),
            settings.getGyroSmoothing(),
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
            settings.getGyroTailSlideSpeed(),
            gyro.getTailSlideBlend()
        );
    }

    if(
        settings.getBlackboxEnabled() &&
        blackbox.isReady()
    )
    {
        blackbox.update(false);
    }

    delay(1);
}
 
