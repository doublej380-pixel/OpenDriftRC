#include <Arduino.h>
#include <WiFi.h>

#include "LGFX_OpenDrift.hpp"
#include "IMU.h"
#include "Servo.h"
#include "GyroController.h"
#include "Touch.h"
#include "UI.h"
#include "WiFiManager.h"
#include "Settings.h"
#include "RadioInput.h"
#include "WebConfigurator.h"
#include "BlackboxLogger.h"

LGFX lcd;

IMU imu;

ServoOutput steeringServo;

ServoOutput throttleOutput;

GyroController gyro;

Touch touch;

UI ui;

WiFiManager wifi;

Settings settings;

WebConfigurator webConfig;

RadioInput steeringRadio;

RadioInput gainRadio;

RadioInput throttleRadio;

BlackboxLogger blackbox;

float slewedGyroCorrection = 0;

float dampedSteeringCommand = 1500;

uint32_t lastCorrectionMicros = 0;

uint32_t lastSteeringDampMicros = 0;

bool steeringDamperReady = false;

unsigned long lastBlackboxLog = 0;

bool blackboxStarted = false;

bool lastBlackboxEnabled = false;

bool blackboxStartAttempted = false;

#define SERVO_OUTPUT_PIN 16
#define RADIO_STEERING_PIN 17
#if defined(OPENDRIFT_BOARD_AMOLED_164)
#define RADIO_THROTTLE_PIN 15
#else
// The round board does not have a spare input. Its former GPIO 18 gain
// channel is dedicated to throttle logging; gyro gain comes from settings.
#define RADIO_THROTTLE_PIN 18
#endif
#define SHARED_GAIN_THROTTLE_PIN 18

bool pin18ModeConfigured = false;

bool pin18ThrottleOutputMode = false;

bool throttleOutputActive = false;

const float radioGainMin = 0.5f;
const float radioGainMax = 3.0f;

const char* ssid = "OpenDrift";
const char* password = "opendrift";


#if defined(OPENDRIFT_BOARD_AMOLED_164)
static constexpr float AMOLED_BOOT_LOG_TEXT_SIZE = 1.15f;
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
            "OpenDrift verbose boot",
            8,
            7
        );

        canvas.setTextSize(AMOLED_BOOT_LOG_TEXT_SIZE);
        canvas.setTextColor(0x7BEF);
        canvas.drawString(
            "control kernel 1.0.0-amoled  ttyOD0",
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
            "OpenDrift boot",
            120,
            12
        );

        display->setTextSize(1);
        display->setTextColor(0x7BEF);
        display->drawCenterString(
            "control kernel 1.0-round  ttyOD0",
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
    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    // throttleRadio owns GPIO 18 for the lifetime of the round build.
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
            ? "GPIO 18 mode: throttle output"
            : "GPIO 18 throttle output unavailable"
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
            ? "GPIO 18 mode: gyro gain input"
            : "GPIO 18 gain input unavailable"
        );
    }

    pin18ThrottleOutputMode =
        throttleMode;

    pin18ModeConfigured =
        true;

    return configured;
    #endif
}

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



int dampSteeringInput(
    int target,
    Settings& settings
)
{
    int damperMs =
        settings.getSteeringDamper();

    uint32_t now =
        micros();

    float dt =
        0.02f;

    if(lastSteeringDampMicros != 0)
    {
        dt =
            (now - lastSteeringDampMicros)
            /
            1000000.0f;

        dt =
            constrain(
                dt,
                0.001f,
                0.05f
            );
    }

    lastSteeringDampMicros =
        now;

    if(
        damperMs <= 0 ||
        !steeringDamperReady
    )
    {
        dampedSteeringCommand =
            target;

        steeringDamperReady =
            true;

        return target;
    }

    float tau =
        damperMs
        /
        1000.0f;

    float alpha =
        dt
        /
        (tau + dt);

    dampedSteeringCommand +=
        (target - dampedSteeringCommand)
        *
        alpha;

    return constrain(
        (int)roundf(dampedSteeringCommand),
        1000,
        2000
    );
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

    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    #endif

    bool displayOk =
        lcd.init();

    Serial.print("Display init: ");
    Serial.println(displayOk ? "OK" : "FAIL");

    lcd.setColorDepth(16);
    lcd.setSwapBytes(false);
    lcd.setRotation(0);
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);

    bootConsole.begin(
        &lcd
    );

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

    if(!imu.begin())
    {
        bootConsole.log(
            "qmi8658: IMU probe failed",
            "[FAIL]",
            TFT_RED
        );

        while(true)
            delay(1000);
    }

    Serial.println("IMU OK");

    bootConsole.log(
        "qmi8658: 6-axis inertial sensor ready"
    );

    //-------------------
    // SERVO
    //-------------------

    if(!steeringServo.begin(SERVO_OUTPUT_PIN))
    {
        bootConsole.log(
            "ledc: steering servo output failed",
            "[FAIL]",
            TFT_RED
        );

        while(true)
            delay(1000);
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
        "ledc: steering servo attached on gpio16"
    );

    //-------------------
    // RADIO
    //-------------------

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

    bool radioOk =
        steeringRadioOk &&
        throttleRadioOk &&
        sharedPinOk;

    bootConsole.log(
        "rc-input: steering and throttle channels armed",
        radioOk ? "[ OK ]" : "[FAIL]",
        radioOk ? TFT_GREEN : TFT_RED
    );

    bootConsole.log(
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        pin18ThrottleOutputMode
        ? "gpio18: throttle passthrough output"
        : "gpio18: gyro gain adjustment input"
        #else
        "gpio18: throttle input (gain uses saved setting)"
        #endif
    );

    Serial.println("Radio inputs initialized");

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

    gyro.setAntiWobble(
        settings.getGyroAntiWobble()
    );

    gyro.setHuntDamping(
        settings.getGyroHuntDamping()
    );

    //-------------------
    // TOUCH
    //-------------------

    Serial.println("Starting Touch");

    if(!touch.begin())
    {
        bootConsole.log(
            #if defined(OPENDRIFT_BOARD_AMOLED_164)
            "cst92xx: touch controller probe failed",
            #else
            "cst816s: touch controller probe failed",
            #endif
            "[FAIL]",
            TFT_RED
        );

        while(true)
            delay(1000);
    }

    Serial.println("TOUCH OK");

    bootConsole.log(
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        "cst92xx: capacitive touch input ready"
        #else
        "cst816s: capacitive touch input ready"
        #endif
    );

    delay(1000);

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

    touch.update();

}

void loop()
{
    static unsigned long lastHeartbeatMs = 0;

    if(millis() - lastHeartbeatMs > 5000)
    {
        lastHeartbeatMs =
            millis();

        Serial.println("OpenDrift heartbeat");
    }

    imu.update();

    touch.update();

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

    //-------------------
    // RADIO
    //-------------------

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

    gyro.setAntiWobble(
        settings.getGyroAntiWobble()
    );

    gyro.setHuntDamping(
        settings.getGyroHuntDamping()
    );

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
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        gainRadio
        #else
        throttleRadio
        #endif
    );

    //-------------------
    // GYRO
    //-------------------

    float yaw =
        imu.getYawRate();

    int gyroCommand =
        gyro.update(
            yaw,
            throttleRadio.getPulseWidth(),
            throttleRadio.hasSignal(),
            imu.getSurfaceDisturbanceScore(),
            settings.getTerrainAssistEnabled()
        );

    int steeringCommand = 1500;

    if(steeringRadio.hasSignal())
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

        steeringCommand =
            dampSteeringInput(
                steeringCommand,
                settings
            );
    }
    else
    {
        steeringDamperReady =
            false;

        lastSteeringDampMicros =
            0;
    }

    int gyroCorrection =
        gyroCommand - 1500;

    int rawGyroCorrection =
        gyroCorrection;

    if(settings.getGyroReverse())
    {
        gyroCorrection =
            -gyroCorrection;

        rawGyroCorrection =
            -rawGyroCorrection;
    }

    uint32_t correctionNow =
        micros();

    float correctionDt =
        0.02f;

    if(lastCorrectionMicros != 0)
    {
        correctionDt =
            (correctionNow - lastCorrectionMicros)
            /
            1000000.0f;

        correctionDt =
            constrain(
                correctionDt,
                0.001f,
                0.05f
            );
    }

    lastCorrectionMicros =
        correctionNow;

    float correctionDelta =
        gyroCorrection - slewedGyroCorrection;

    int rateSetting =
        abs(gyroCorrection) > abs(slewedGyroCorrection)
        ?
        settings.getGyroAttackSpeed()
        :
        settings.getGyroReturnSpeed();

    float rateLimit =
        rateSetting
        *
        (correctionDt / 0.02f);

    correctionDelta =
        constrain(
            correctionDelta,
            -rateLimit,
            rateLimit
        );

    slewedGyroCorrection +=
        correctionDelta;

    gyroCorrection =
        (int)roundf(
            slewedGyroCorrection
        );

    int servoCommand =
        steeringServo.getPosition();

    if(steeringRadio.hasSignal())
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

    //-------------------
    // BLACKBOX LOG
    //-------------------

    if(
        settings.getBlackboxEnabled() &&
        blackbox.isReady() &&
        steeringRadio.hasSignal() &&
        millis() - lastBlackboxLog >= 50
    )
    {
        lastBlackboxLog =
            millis();

        blackbox.log(
            lastBlackboxLog,
            yaw,
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
            rawGyroCorrection,
            gyroCorrection,
            steeringRadio.getPulseWidth(),
            steeringCommand,
            servoCommand,
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
            settings.getGyroAntiWobble(),
            settings.getGyroHuntDamping(),
            gyro.getHuntControlYaw(),
            gyro.getHuntSlowYaw(),
            gyro.getHuntFastYaw(),
            gyro.getHuntBlend(),
            gyro.getHuntScore(),
            gyro.getControlPhase(),
            gyro.getSettledBlend(),
            gyro.getThrottleTransient(),
            gyro.getTerrainActive(),
            gyro.getTerrainAssist(),
            settings.getTerrainAssistEnabled(),
            gyro.getActiveHoldFactor(),
            settings.getGyroAttackSpeed(),
            settings.getGyroReturnSpeed(),
            steeringRadio.hasSignal(),
            throttleRadio.hasSignal(),
            gainRadio.hasSignal(),
            pin18ThrottleOutputMode
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
 
