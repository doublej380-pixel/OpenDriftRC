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

LGFX lcd;

IMU imu;

ServoOutput steeringServo;

GyroController gyro;

Touch touch;

UI ui;

WiFiManager wifi;

Settings settings;

RadioInput steeringRadio;

RadioInput gainRadio;

#define SERVO_OUTPUT_PIN 16
#define RADIO_STEERING_PIN 17
#define RADIO_GAIN_PIN 18

const float radioGainMin = 0.5f;
const float radioGainMax = 3.0f;

const char* ssid = "OpenDrift";
const char* password = "opendrift";

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
    // WIFI
    //-------------------

    wifi.begin(
        ssid,
        password,
        settings.getWifiEnabled()
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
    }

    //-------------------
    // UI
    //-------------------

    ui.begin(
        &lcd,
        gyro,
        wifi,
        settings
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

    //-------------------
    // WIFI
    //-------------------

    wifi.update();

    //-------------------
    // RADIO
    //-------------------

    if(gainRadio.hasSignal())
    {
        gyro.setGain(
            gainRadio.getMappedValue(
                radioGainMin,
                radioGainMax
            )
        );
    }

    //-------------------
    // UI
    //-------------------

    ui.update(
        touch,
        gyro,
        imu,
        wifi,
        settings
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
            steeringRadio.getPulseWidth();
    }

    int gyroCorrection =
        gyroCommand - 1500;

    int servoCommand =
        constrain(
            steeringCommand + gyroCorrection,
            1000,
            2000
        );

    steeringServo.writeMicroseconds(
        servoCommand
    );

    //-------------------
    // DEBUG
    //-------------------

    Serial.print("Yaw: ");
    Serial.print(yaw);

    Serial.print(" Filtered: ");
    Serial.print(gyro.getFilteredYaw());

    Serial.print(" Servo: ");
    Serial.print(servoCommand);

    Serial.print(" Steering: ");
    Serial.print(steeringRadio.getPulseWidth());

    Serial.print(" SteeringAge: ");
    Serial.print(steeringRadio.getSignalAgeMs());

    Serial.print(" RadioGain: ");
    Serial.print(gainRadio.getPulseWidth());

    Serial.print(" GainAge: ");
    Serial.print(gainRadio.getSignalAgeMs());

    Serial.print(" Gain: ");
    Serial.println(gyro.getGain());

    delay(20);
}
