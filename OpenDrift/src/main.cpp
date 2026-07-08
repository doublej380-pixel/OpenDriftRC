#include <Arduino.h>
#include <WiFi.h>

#include "LGFX_OpenDrift.hpp"
#include "IMU.h"


LGFX lcd;

IMU imu;


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
    lcd.drawCenterString("OpenDrift",120,30);



    // Start IMU

    if(!imu.begin())
    {
        lcd.setTextSize(2);
        lcd.drawCenterString(
            "IMU ERROR",
            120,
            100
        );

        while(true)
        {
            delay(1000);
        }
    }


    lcd.setTextSize(2);
    lcd.drawCenterString(
        "IMU OK",
        120,
        80
    );



    // WiFi

    WiFi.mode(WIFI_AP);

    WiFi.softAP(
        ssid,
        password
    );


    IPAddress IP = WiFi.softAPIP();



    lcd.drawCenterString(
        "WiFi OK",
        120,
        120
    );


    lcd.setTextSize(1);

    lcd.drawCenterString(
        IP.toString(),
        120,
        150
    );



    delay(1500);


    lcd.fillScreen(TFT_BLACK);

    lcd.setTextSize(3);
    lcd.drawCenterString(
        "OpenDrift",
        120,
        20
    );
}



void loop()
{
    imu.update();


    lcd.fillRect(
        0,
        70,
        240,
        100,
        TFT_BLACK
    );


    lcd.setTextSize(2);



    lcd.drawString(
        "Yaw:",
        20,
        80
    );


    lcd.drawFloat(
        imu.getYawRate(),
        2,
        90,
        80
    );


    lcd.drawString(
        "dps",
        170,
        80
    );



    lcd.drawString(
        "X:",
        20,
        120
    );


    lcd.drawFloat(
        imu.getGyroX(),
        2,
        60,
        120
    );



    lcd.drawString(
        "Y:",
        130,
        120
    );


    lcd.drawFloat(
        imu.getGyroY(),
        2,
        170,
        120
    );


    delay(100);
}