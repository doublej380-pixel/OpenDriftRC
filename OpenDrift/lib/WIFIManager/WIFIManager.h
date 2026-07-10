#pragma once

#include <Arduino.h>
#include <WiFi.h>


class WiFiManager
{

public:

    void begin(
        const char* ssid,
        const char* password,
        bool startEnabled = true
    );


    void update();



    void enable();


    void disable();



    bool isEnabled();


    bool hasClient();

    void setTimeout(
        unsigned long timeoutMs
    );



private:

    const char* wifiSSID = nullptr;

    const char* wifiPassword = nullptr;


    bool enabled = false;


    unsigned long startTime = 0;


    unsigned long timeout =
        40000;


};
