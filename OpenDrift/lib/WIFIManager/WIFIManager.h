#pragma once

#include <Arduino.h>
#include <WiFi.h>


class WiFiManager
{

public:

    void begin(
        const char* ssid,
        const char* password
    );


    void update();



    void enable();


    void disable();



    bool isEnabled();


    bool hasClient();



private:

    const char* wifiSSID;

    const char* wifiPassword;


    bool enabled = false;


    unsigned long startTime = 0;


    const unsigned long timeout =
        40000;


};