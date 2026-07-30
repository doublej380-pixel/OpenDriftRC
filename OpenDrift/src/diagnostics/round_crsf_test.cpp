#include <Arduino.h>

#include "LGFX_OpenDrift.hpp"
#include "CrsfInput.h"

#if defined(OPENDRIFT_CRSF_DIAGNOSTIC)

namespace
{
    constexpr int8_t CRSF_RX_PIN = 17;

    LGFX lcd;
    CrsfInput crsf;

    uint32_t lastDrawMs = 0;
    uint32_t lastBytes = 0;
    uint32_t lastProgressMs = 0;


    void drawStatus()
    {
        uint32_t bytes = crsf.getReceivedByteCount();

        if(bytes != lastBytes)
        {
            lastBytes = bytes;
            lastProgressMs = millis();
        }

        lcd.fillScreen(TFT_BLACK);
        lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        lcd.setTextSize(2);
        lcd.drawCenterString("CRSF RX TEST", 120, 20);

        lcd.setTextSize(1);
        lcd.setCursor(36, 65);
        lcd.printf("GPIO 17 RX only\n\n");
        lcd.printf("Bytes:    %lu\n", (unsigned long)bytes);
        lcd.printf("Frames:   %lu\n", (unsigned long)crsf.getValidFrameCount());
        lcd.printf("Channels: %lu\n", (unsigned long)crsf.getChannelFrameCount());
        lcd.printf("CRC err:  %lu\n", (unsigned long)crsf.getCrcErrorCount());

        uint32_t age = millis() - lastProgressMs;
        lcd.setTextColor(
            bytes > 0 && age < 1000 ? TFT_GREEN : TFT_YELLOW,
            TFT_BLACK
        );
        lcd.drawCenterString(
            bytes > 0 && age < 1000 ? "RECEIVING" : "WAITING",
            120,
            190
        );
    }
}


void setup()
{
    Serial.begin(115200);
    delay(250);

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);

    lcd.init();
    lcd.setRotation(0);
    lcd.setColorDepth(16);
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextSize(2);
    lcd.drawCenterString("CRSF TEST BOOT", 120, 105);

    Serial.println("Round CRSF diagnostic boot");
    delay(750);

    crsf.begin(CRSF_RX_PIN, -1);
    lastProgressMs = millis();

    Serial.println("CRSF RX-only UART online on GPIO 17");
    drawStatus();
}


void loop()
{
    crsf.update();

    uint32_t now = millis();

    if(now - lastDrawMs >= 250)
    {
        lastDrawMs = now;
        drawStatus();

        Serial.printf(
            "bytes=%lu frames=%lu channels=%lu crc=%lu\n",
            (unsigned long)crsf.getReceivedByteCount(),
            (unsigned long)crsf.getValidFrameCount(),
            (unsigned long)crsf.getChannelFrameCount(),
            (unsigned long)crsf.getCrcErrorCount()
        );
    }

    delay(1);
}

#endif
