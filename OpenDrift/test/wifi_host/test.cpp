// Lifecycle tests use fake hardware APIs; real DHCP and RF need bench testing.
#include "WIFIManager.h"
#include <cassert>
#include <cstdio>
uint32_t hostTime=100000;
FakeWiFi WiFi;
FakeMDNS MDNS;
static void advance(unsigned ms) { hostTime+=ms*1000; }
int main() {
    WiFiManager manager;
    manager.begin("OpenDrift","opendrift",false);
    assert(!manager.isEnabled() && WiFi.current==WIFI_OFF);
    assert(WiFi.starts==0 && !MDNS.active);
    for(int i=0;i<20;i++){advance(300);manager.update();}
    assert(WiFi.queries==0);
    manager.enable();
    assert(manager.isEnabled() && MDNS.active && WiFi.current==WIFI_AP);
    manager.disable();
    assert(!manager.isEnabled() && !MDNS.active && WiFi.current==WIFI_OFF);
    int queries=WiFi.queries;
    advance(1000);manager.update();
    assert(WiFi.queries==queries);
    manager.enable(); // On -> off -> on remains usable.
    assert(manager.isEnabled() && WiFi.starts==2);
    manager.setTimeout(100);
    advance(300);manager.update();
    assert(!manager.isEnabled() && WiFi.current==WIFI_OFF && !MDNS.active);
    manager.enable();manager.setTimeout(0);
    WiFi.stations=1; // Exercise failed DHCP recovery and partial AP startup.
    advance(300);manager.update();
    advance(6100);manager.update();
    WiFi.failAP=true;
    advance(6100);manager.update();
    assert(!manager.isEnabled() && WiFi.current==WIFI_OFF && !MDNS.active);
    queries=WiFi.queries;
    advance(1000);manager.update();
    assert(queries==WiFi.queries);
    std::puts("Wi-Fi disabled startup, toggle, auto-off, and failed-recovery tests passed");
}
