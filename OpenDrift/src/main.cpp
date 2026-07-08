#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#include "LGFX_OpenDrift.hpp"
#include "SensorQMI8658.hpp"


LGFX lcd;

SensorQMI8658 qmi;


// WiFi Access Point settings
const char* ssid = "OpenDrift";
const char* password = "opendrift";


float gx = 0;
float gy = 0;
float gz = 0;


void setup()
{
    Serial.begin(115200);
    delay(500);

    // Backlight
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);


    // Display
    lcd.init();

    lcd.setRotation(0);

    lcd.fillScreen(TFT_BLACK);

    lcd.setTextColor(TFT_WHITE);


    lcd.setTextSize(3);
    lcd.drawCenterString("OpenDrift", 120, 30);


    lcd.setTextSize(2);
    lcd.drawCenterString("Starting...", 120, 80);



    // IMU
    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, 6, 7))
    {
        Serial.println("QMI8658 NOT detected!");

        lcd.drawCenterString("IMU ERROR", 120, 130);

        while(true)
        {
            delay(1000);
        }
    }


    qmi.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_1000Hz,
        SensorQMI8658::LPF_MODE_0
    );


    qmi.configGyroscope(
        SensorQMI8658::GYR_RANGE_1024DPS,
        SensorQMI8658::GYR_ODR_896_8Hz,
        SensorQMI8658::LPF_MODE_0
    );


    qmi.enableAccelerometer();
    qmi.enableGyroscope();


    lcd.fillScreen(TFT_BLACK);

    lcd.setTextSize(3);
    lcd.drawCenterString("OpenDrift", 120, 20);


    lcd.setTextSize(2);
    lcd.drawString("IMU: OK", 20, 60);


    // WiFi AP
    WiFi.mode(WIFI_AP);

    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();


    lcd.drawString("WiFi: OK", 20, 90);

    lcd.setTextSize(1);
    lcd.drawString(IP.toString(), 20, 120);


    Serial.println("OpenDrift Started");
    Serial.println(IP);
}



void loop()
{

    qmi.getGyroscope(gx, gy, gz);


    // Clear old values only
    lcd.fillRect(10, 150, 220, 80, TFT_BLACK);


    lcd.setTextSize(2);


    lcd.drawString("Gyro Z:", 20, 150);


    lcd.drawFloat(gz, 2, 120, 150);


    lcd.drawString("dps", 180, 150);



    lcd.drawString("X:", 20, 180);
    lcd.drawFloat(gx, 2, 60, 180);


    lcd.drawString("Y:", 130, 180);
    lcd.drawFloat(gy, 2, 170, 180);


    Serial.print("X:");
    Serial.print(gx);
    Serial.print(" Y:");
    Serial.print(gy);
    Serial.print(" Z:");
    Serial.println(gz);


    delay(100);
}