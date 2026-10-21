#pragma once
struct FakeMDNS {
    bool active=false;
    bool begin(const char*) { active=true; return true; }
    void addService(const char*,const char*,int) {}
    void end() { active=false; }
};
extern FakeMDNS MDNS;
