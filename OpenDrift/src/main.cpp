#include <Arduino.h>
#include <WiFi.h>

#include "LGFX_OpenDrift.hpp"
#include "IMU.h"
#include "Servo.h"
#include "GyroController.h"
#include "Touch.h"
#include "UI.h"



LGFX lcd;

IMU imu;

ServoOutput steeringServo;

GyroController gyro;

Touch touch;

UI ui;



#define SERVO_PIN 16



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

    lcd.drawCenterString(
        "OpenDrift",
        120,
        20
    );



    // -------------------
    // IMU
    // -------------------

    if(!imu.begin())
    {
        lcd.setTextSize(2);

        lcd.drawCenterString(
            "IMU ERROR",
            120,
            90
        );

        while(true)
            delay(1000);
    }


    lcd.drawCenterString(
        "IMU OK",
        120,
        60
    );


    Serial.println("IMU OK");



    // -------------------
    // SERVO
    // -------------------

    if(!steeringServo.begin(SERVO_PIN))
    {
        lcd.drawCenterString(
            "SERVO ERROR",
            120,
            100
        );

        while(true)
            delay(1000);
    }



    steeringServo.center();



    lcd.drawCenterString(
        "SERVO OK",
        120,
        90
    );


    Serial.println("SERVO OK");



    // -------------------
    // GYRO CONTROLLER
    // -------------------

    gyro.begin();

    gyro.setGain(1.5f);

    gyro.setDeadband(2.0f);




    // -------------------
    // TOUCH
    // -------------------

    Serial.println("Starting Touch");


    if(!touch.begin())
    {
        lcd.drawCenterString(
            "TOUCH ERROR",
            120,
            120
        );

        while(true)
            delay(1000);
    }


    lcd.drawCenterString(
        "TOUCH OK",
        120,
        120
    );


    Serial.println("TOUCH OK");



    delay(1000);




    // -------------------
    // CALIBRATION
    // -------------------

    lcd.drawCenterString(
        "Calibrating",
        120,
        150
    );


    delay(2000);


    imu.update();


    gyro.calibrate(
        imu.getYawRate()
    );


    Serial.println("Gyro calibrated");



    delay(500);




    // -------------------
    // WIFI
    // -------------------

    WiFi.mode(WIFI_AP);


    WiFi.softAP(
        ssid,
        password
    );


    IPAddress IP =
        WiFi.softAPIP();



    Serial.print("WiFi IP: ");
    Serial.println(IP);



    lcd.fillScreen(TFT_BLACK);



    lcd.setTextSize(2);


    lcd.drawCenterString(
        "WiFi OK",
        120,
        40
    );


    lcd.setTextSize(1);


    lcd.drawCenterString(
        IP.toString(),
        120,
        70
    );



    delay(1000);



    // -------------------
    // START UI
    // -------------------

    ui.begin(&lcd);


}







void loop()
{

    imu.update();


    touch.update();



    // -------------------
    // TOUCH BUTTONS
    // -------------------

    ui.update(
        touch,
        gyro,
        imu
    );



    // -------------------
    // GYRO CONTROL
    // -------------------

    float yaw =
        imu.getYawRate();



    int servoCommand =
        gyro.update(yaw);



    steeringServo.writeMicroseconds(
        servoCommand
    );




    // Debug

    Serial.print("Yaw: ");

    Serial.print(yaw);


    Serial.print(" Filtered: ");

    Serial.print(
        gyro.getFilteredYaw()
    );


    Serial.print(" Servo: ");

    Serial.println(
        gyro.getServoOutput()
    );



    delay(20);

}