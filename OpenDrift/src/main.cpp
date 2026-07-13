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

GyroController gyro;

Touch touch;

UI ui;

WiFiManager wifi;

Settings settings;

WebConfigurator webConfig;

RadioInput steeringRadio;

RadioInput gainRadio;

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
#define RADIO_GAIN_PIN 18

const float radioGainMin = 0.5f;
const float radioGainMax = 3.0f;

const char* ssid = "OpenDrift";
const char* password = "opendrift";

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

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);

    lcd.init();
    lcd.setRotation(0);
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);

    lcd.setTextSize(3);
    lcd.drawCenterString("OpenDrift",120,20);
    lcd.setTextSize(2);

    //-------------------
    // SETTINGS
    //-------------------

    settings.begin();

    //-------------------
    // IMU
    //-------------------

    if(!imu.begin())
    {
        lcd.setTextSize(2);
        lcd.drawCenterString("IMU ERROR",120,90);

        while(true)
            delay(1000);
    }

    lcd.drawCenterString("IMU OK",120,60);

    Serial.println("IMU OK");

    //-------------------
    // SERVO
    //-------------------

    if(!steeringServo.begin(SERVO_OUTPUT_PIN))
    {
        lcd.drawCenterString("SERVO ERROR",120,100);

        while(true)
            delay(1000);
    }

    steeringServo.configure(
        settings.getServoCenter(),
        settings.getServoReverse(),
        settings.getServoTravel()
    );

    steeringServo.center();

    lcd.drawCenterString("SERVO OK",120,85);

    Serial.println("SERVO OK");

    //-------------------
    // RADIO
    //-------------------

    steeringRadio.begin(
        RADIO_STEERING_PIN
    );

    gainRadio.begin(
        RADIO_GAIN_PIN
    );

    lcd.drawCenterString("RADIO OK",120,110);

    Serial.println("Radio gain input OK");

    //-------------------
    // GYRO CONTROLLER
    //-------------------

    gyro.begin();

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

    //-------------------
    // TOUCH
    //-------------------

    Serial.println("Starting Touch");

    if(!touch.begin())
    {
        lcd.drawCenterString("TOUCH ERROR",120,135);

        while(true)
            delay(1000);
    }

    lcd.drawCenterString("TOUCH OK",120,135);

    Serial.println("TOUCH OK");

    delay(1000);

    //-------------------
    // CALIBRATION
    //-------------------

    lcd.drawCenterString("Calibrating",120,170);

    delay(2000);

    imu.update();

    gyro.calibrate(
        imu.getYawRate()
    );

    Serial.println("Gyro calibrated");

    delay(500);

    //-------------------
    // BLACKBOX
    //-------------------

    if(settings.getBlackboxEnabled())
    {
        updateBlackboxAvailability();
    }
    else
    {
        Serial.println("Blackbox logging disabled");
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

        lcd.fillScreen(TFT_BLACK);

        lcd.setTextSize(2);
        lcd.drawCenterString("WiFi OK",120,40);

        lcd.setTextSize(1);
        lcd.drawCenterString(IP.toString(),120,70);

        delay(1000);

        webConfig.begin(
            settings,
            gyro,
            steeringRadio,
            gainRadio,
            blackbox
        );
    }

    //-------------------
    // UI
    //-------------------

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
            blackbox
        );
    }

    if(wifi.isEnabled())
    {
        webConfig.update();
    }

    //-------------------
    // RADIO
    //-------------------

    if(gainRadio.hasSignal())
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

    //-------------------
    // GYRO
    //-------------------

    float yaw =
        imu.getYawRate();

    int gyroCommand =
        gyro.update(yaw);

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
            settings.getServoTravel()
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
            rawGyroCorrection,
            gyroCorrection,
            steeringRadio.getPulseWidth(),
            steeringCommand,
            servoCommand,
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
            settings.getGyroAttackSpeed(),
            settings.getGyroReturnSpeed(),
            steeringRadio.hasSignal(),
            gainRadio.hasSignal()
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
 
