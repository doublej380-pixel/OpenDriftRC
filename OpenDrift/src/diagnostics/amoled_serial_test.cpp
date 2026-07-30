#include <Arduino.h>
#include <esp_system.h>


#if defined(OPENDRIFT_AMOLED_SERIAL_TEST)

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("OpenDrift AMOLED serial-only diagnostic");
    Serial.printf(
        "reset_reason=%d flash=%u psram=%u heap=%u\n",
        (int)esp_reset_reason(),
        (unsigned int)ESP.getFlashChipSize(),
        (unsigned int)ESP.getPsramSize(),
        (unsigned int)ESP.getFreeHeap()
    );
}


void loop()
{
    Serial.printf(
        "SERIAL TEST ALIVE uptime=%lu ms heap=%u\n",
        (unsigned long)millis(),
        (unsigned int)ESP.getFreeHeap()
    );

    delay(1000);
}

#endif
