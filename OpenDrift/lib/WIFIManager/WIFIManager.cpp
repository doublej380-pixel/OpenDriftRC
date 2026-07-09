#include "WiFiManager.h"




void WiFiManager::begin(
    const char* ssid,
    const char* password
)
{

    wifiSSID = ssid;

    wifiPassword = password;


    enable();

}





void WiFiManager::enable()
{

    if(enabled)
        return;



    WiFi.mode(
        WIFI_AP
    );


    WiFi.softAP(
        wifiSSID,
        wifiPassword
    );


    enabled = true;


    startTime =
        millis();



    Serial.println(
        "WiFi Enabled"
    );

}





void WiFiManager::disable()
{

    if(!enabled)
        return;



    WiFi.softAPdisconnect(
        true
    );


    WiFi.mode(
        WIFI_OFF
    );


    enabled = false;



    Serial.println(
        "WiFi Disabled"
    );

}





void WiFiManager::update()
{

    if(!enabled)
        return;



    if(hasClient())
    {
        return;
    }



    if(
        millis() - startTime
        > timeout
    )
    {

        disable();

    }

}





bool WiFiManager::hasClient()
{

    return (
        WiFi.softAPgetStationNum()
        > 0
    );

}





bool WiFiManager::isEnabled()
{

    return enabled;

}