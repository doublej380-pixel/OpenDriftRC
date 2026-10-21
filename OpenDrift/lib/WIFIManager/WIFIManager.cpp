#include "WIFIManager.h"

#include <esp_netif.h>
#include <esp_netif_sta_list.h>
#include <esp_wifi.h>

namespace
{
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);
constexpr uint8_t AP_CHANNEL = 1;
constexpr uint8_t AP_MAX_CLIENTS = 4;
}

void WiFiManager::begin(
    const char* ssid,
    const char* password,
    bool startEnabled,
    const char* hostname
)
{
    wifiSSID = ssid;
    wifiPassword = password;
    wifiHostname = hostname;

    // OpenDrift never uses saved station credentials. Keeping Wi-Fi state out
    // of flash also prevents stale SDK settings from affecting AP startup.
    WiFi.persistent(false);

    if(startEnabled)
    {
        enable();
    }
    else
    {
        // Do not initialize an AP, DHCP, or mDNS on an off-by-default boot.
        // Arduino's WIFI_OFF path stops/deinitializes an active Wi-Fi driver.
        if(enabled) disable();
        else WiFi.mode(WIFI_OFF);
    }
}

bool WiFiManager::startAccessPoint()
{
    if(!WiFi.mode(WIFI_AP))
    {
        Serial.println("WiFi AP mode failed");
        return false;
    }

    // Modem sleep can delay AP/DHCP traffic enough for some phones to abandon
    // the lease request. OpenDrift powers Wi-Fi only while it is being used, so
    // reliability is more valuable than the small power saving here.
    WiFi.setSleep(false);

    if(!WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET))
    {
        Serial.println("WiFi AP network configuration failed");
        return false;
    }

    if(!WiFi.softAP(
        wifiSSID,
        wifiPassword,
        AP_CHANNEL,
        false,
        AP_MAX_CLIENTS
    ))
    {
        Serial.println("WiFi AP start failed");
        return false;
    }

    // softAP() normally starts DHCP. Verify it instead of assuming it did; a
    // stopped DHCP server produces the exact endless "Obtaining IP" symptom.
    esp_netif_t* apNetif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if(apNetif != nullptr)
    {
        esp_netif_dhcp_status_t status = ESP_NETIF_DHCP_INIT;
        if(
            esp_netif_dhcps_get_status(apNetif, &status) == ESP_OK &&
            status != ESP_NETIF_DHCP_STARTED
        )
        {
            esp_netif_dhcps_start(apNetif);
        }
    }

    mdnsRunning = MDNS.begin(wifiHostname);
    if(mdnsRunning)
    {
        MDNS.addService("http", "tcp", 80);
    }
    else
    {
        MDNS.end();
        Serial.println("WiFi mDNS start failed; use 192.168.4.1");
    }

    Serial.print("WiFi AP ready at ");
    Serial.println(WiFi.softAPIP());
    return true;
}

void WiFiManager::enable()
{
    if(enabled)
    {
        return;
    }

    if(wifiSSID == nullptr || wifiPassword == nullptr)
    {
        Serial.println("WiFi credentials not set");
        return;
    }

    // Always begin from a known state. This matters after a failed boot or a
    // previous AP shutdown because the ESP32 networking stack is stateful.
    WiFi.mode(WIFI_OFF);
    delay(20);

    if(!startAccessPoint())
    {
        WiFi.mode(WIFI_OFF);
        return;
    }

    enabled = true;
    associatedClientCount = 0;
    ipClientCount = 0;
    lastHealthCheck = 0;
    dhcpPendingSince = 0;
    dhcpRecoveryAttempts = 0;
    noClientSince = millis();
    clientWasPresent = false;

    Serial.println("WiFi Enabled");
}

void WiFiManager::disable()
{
    if(!enabled)
    {
        return;
    }

    MDNS.end();
    mdnsRunning = false;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    enabled = false;
    associatedClientCount = 0;
    ipClientCount = 0;
    lastHealthCheck = 0;
    dhcpPendingSince = 0;
    dhcpRecoveryAttempts = 0;
    noClientSince = 0;
    clientWasPresent = false;

    Serial.println("WiFi Disabled");
}

void WiFiManager::updateClientCounts()
{
    associatedClientCount = WiFi.softAPgetStationNum();
    ipClientCount = 0;

    if(associatedClientCount == 0)
    {
        return;
    }

    wifi_sta_list_t wifiStations = {};
    esp_netif_sta_list_t ipStations = {};

    if(
        esp_wifi_ap_get_sta_list(&wifiStations) == ESP_OK &&
        esp_netif_get_sta_list(&wifiStations, &ipStations) == ESP_OK
    )
    {
        ipClientCount = static_cast<uint8_t>(ipStations.num);
    }
}

bool WiFiManager::restartDhcpServer()
{
    esp_netif_t* apNetif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if(apNetif == nullptr)
    {
        return false;
    }

    esp_err_t stopResult = esp_netif_dhcps_stop(apNetif);
    if(stopResult != ESP_OK && stopResult != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)
    {
        Serial.printf("WiFi DHCP stop failed: %d\n", static_cast<int>(stopResult));
        return false;
    }

    esp_err_t startResult = esp_netif_dhcps_start(apNetif);
    if(startResult != ESP_OK && startResult != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED)
    {
        Serial.printf("WiFi DHCP start failed: %d\n", static_cast<int>(startResult));
        return false;
    }

    return true;
}

void WiFiManager::restartAccessPoint()
{
    Serial.println("WiFi DHCP still stalled; restarting access point");

    MDNS.end();
    mdnsRunning = false;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(20);

    if(!startAccessPoint())
    {
        // AP startup may have enabled the driver before a later stage failed.
        WiFi.mode(WIFI_OFF);
        enabled = false;
        Serial.println("WiFi recovery failed; toggle WiFi to retry");
    }

    associatedClientCount = 0;
    ipClientCount = 0;
    lastHealthCheck = millis();
    dhcpPendingSince = 0;
    dhcpRecoveryAttempts = 0;
    noClientSince = millis();
    clientWasPresent = false;
}

void WiFiManager::update()
{
    if(!enabled)
    {
        return;
    }

    const unsigned long now = millis();
    if(now - lastHealthCheck < HEALTH_CHECK_INTERVAL_MS)
    {
        return;
    }

    lastHealthCheck = now;
    updateClientCounts();

    const bool associated = associatedClientCount > 0;
    const bool clientReady = ipClientCount > 0;

    if(associated && !clientReady)
    {
        if(dhcpPendingSince == 0)
        {
            dhcpPendingSince = now;
            Serial.println("WiFi station associated; awaiting DHCP lease");
        }
        else if(now - dhcpPendingSince >= DHCP_LEASE_TIMEOUT_MS)
        {
            if(dhcpRecoveryAttempts == 0)
            {
                Serial.println("WiFi DHCP stalled; restarting DHCP server");
                restartDhcpServer();
                dhcpRecoveryAttempts = 1;
                dhcpPendingSince = now;
            }
            else
            {
                restartAccessPoint();
                return;
            }
        }
    }
    else
    {
        if(clientReady && dhcpPendingSince != 0)
        {
            Serial.println("WiFi DHCP lease assigned");
        }

        dhcpPendingSince = 0;
        dhcpRecoveryAttempts = 0;
    }

    // Association counts as activity while DHCP is in progress so the normal
    // auto-off timer cannot shut Wi-Fi down during lease recovery.
    if(associated)
    {
        if(clientReady && !clientWasPresent)
        {
            Serial.println("WiFi client connected; auto-off paused");
        }

        clientWasPresent = clientReady;
        noClientSince = 0;
        return;
    }

    if(clientWasPresent)
    {
        Serial.println("WiFi client disconnected; auto-off timer started");
        clientWasPresent = false;
        noClientSince = now;
    }
    else if(noClientSince == 0)
    {
        noClientSince = now;
    }

    if(timeout > 0 && now - noClientSince > timeout)
    {
        disable();
    }
}

bool WiFiManager::hasClient()
{
    return ipClientCount > 0;
}

String WiFiManager::getLocalName()
{
    return String(wifiHostname) + ".local";
}

bool WiFiManager::isEnabled()
{
    return enabled;
}

void WiFiManager::setTimeout(unsigned long timeoutMs)
{
    timeout = timeoutMs;
}
