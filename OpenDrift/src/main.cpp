#include <Arduino.h>
#include "LGFX_OpenDrift.hpp"

LGFX lcd;

void setup()
{
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);     // Backlight ON

    lcd.init();

    lcd.setRotation(0);

    lcd.fillScreen(TFT_BLACK);

    lcd.setTextColor(TFT_WHITE);

    lcd.setTextSize(3);

    lcd.drawCenterString("OpenDrift",120,80);

    lcd.setTextSize(2);

    lcd.drawCenterString("Hello World!",120,130);
}

void loop()
{

}