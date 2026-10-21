#pragma once
#include <Arduino.h>
#include <string>
using String = std::string;
enum wifi_mode_t { WIFI_OFF, WIFI_AP };
struct IPAddress { IPAddress(int,int,int,int) {} };
struct FakeWiFi {
    wifi_mode_t current = WIFI_OFF;
    int starts=0, stations=0, queries=0;
    bool failAP=false;
    void persistent(bool) {}
    bool mode(wifi_mode_t value) { current=value; return true; }
    void setSleep(bool) {}
    bool softAPConfig(const IPAddress&,const IPAddress&,const IPAddress&) { return true; }
    bool softAP(const char*,const char*,int,bool,int) { ++starts; return !failAP; }
    void softAPdisconnect(bool) {}
    const char* softAPIP() { return "192.168.4.1"; }
    int softAPgetStationNum() { ++queries; return stations; }
};
extern FakeWiFi WiFi;
