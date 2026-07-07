#include <Arduino.h>

void setup()
{
    Serial.begin(115200);

    Serial.println("OpenDrift Starting...");
}

void loop()
{
    delay(1000);

    Serial.println("Hello OpenDrift");
}