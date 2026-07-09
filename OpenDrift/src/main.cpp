#include <Arduino.h>
#include <WiFi.h>

#include "LGFX_OpenDrift.hpp"
#include "IMU.h"
#include "Servo.h"
#include "GyroController.h"

LGFX lcd;

IMU imu;

ServoOutput steeringServo;

GyroController gyro;


// Servo output pin
#define SERVO_PIN 16


// WiFi
const char* ssid = "OpenDrift";
const char* password = "opendrift";



void setup()
{
    Serial.begin(115200);

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);

    lcd.init();
    lcd.setRotation(0);
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);

    lcd.setTextSize(3);
    lcd.drawCenterString("OpenDrift",120,20);

    // IMU

    if(!imu.begin())
    {
        lcd.setTextSize(2);
        lcd.drawCenterString("IMU ERROR",120,90);

        while(true)
            delay(1000);
    }

    lcd.setTextSize(2);
    lcd.drawCenterString("IMU OK",120,60);

    // Servo

    if(!steeringServo.begin(SERVO_PIN))
    {
        lcd.drawCenterString("SERVO ERROR",120,100);

        while(true)
            delay(1000);
    }

    steeringServo.center();

    lcd.drawCenterString("SERVO OK",120,90);

    // Gyro Controller

    gyro.begin();

    // WiFi

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid,password);

    IPAddress IP = WiFi.softAPIP();

    lcd.drawCenterString("WiFi OK",120,120);

    lcd.setTextSize(1);
    lcd.drawCenterString(IP.toString(),120,150);

    delay(1500);

    lcd.fillScreen(TFT_BLACK);

    lcd.setTextSize(3);
    lcd.drawCenterString("OpenDrift",120,20);
}



void loop()
{
    imu.update();

    float yaw = imu.getYawRate();

    int servoCommand = gyro.update(yaw);

    steeringServo.writeMicroseconds(servoCommand);



    // ---------- Display ----------

    lcd.fillRect(
        0,
        70,
        240,
        150,
        TFT_BLACK
    );

    lcd.setTextSize(2);

    lcd.drawString("Yaw:",20,80);
    lcd.drawFloat(yaw,2,90,80);
    lcd.drawString("dps",170,80);

    lcd.drawString("Gain:",20,110);
    lcd.drawFloat(gyro.getGain(),2,100,110);

    lcd.drawString("Servo:",20,140);
    lcd.drawNumber(gyro.getServoOutput(),110,140);
    lcd.drawString("us",170,140);

    Serial.print("Yaw: ");
    Serial.print(yaw);
    Serial.print(" Servo: ");
    Serial.println(gyro.getServoOutput());

    delay(20);
}