#include "UI.h"


static int mapSteeringForDisplay(
    int pulse,
    Settings& settings
)
{
    int steeringMin =
        settings.getSteeringMin();

    int steeringCenter =
        settings.getSteeringCenter();

    int steeringMax =
        settings.getSteeringMax();

    if(abs(pulse - steeringCenter) <= 4)
    {
        return 1500;
    }

    int mappedPulse = 1500;

    if(
        steeringCenter <= steeringMin ||
        steeringCenter >= steeringMax
    )
    {
        mappedPulse =
            constrain(
            pulse,
            1000,
            2000
        );
    }
    else if(pulse < steeringCenter)
    {
        mappedPulse =
            map(
                constrain(
                    pulse,
                    steeringMin,
                    steeringCenter
                ),
                steeringMin,
                steeringCenter,
                1000,
                1500
            );
    }
    else
    {
        mappedPulse =
            map(
                constrain(
                    pulse,
                    steeringCenter,
                    steeringMax
                ),
                steeringCenter,
                steeringMax,
                1500,
                2000
            );
    }

    int offset =
        mappedPulse - 1500;

    offset =
        (offset * settings.getRadioSteeringTravel())
        /
        100;

    return constrain(
        1500 + offset,
        1000,
        2000
    );
}



void UI::begin(
    LGFX* display,
    GyroController& gyro,
    WiFiManager& wifi,
    Settings& settings,
    RadioInput& steeringRadio,
    RadioInput& gainRadio
)
{
    lcd = display;

    page = 0;


    drawMainPage(
        gyro,
        settings
    );
}





void UI::drawPage(
    GyroController& gyro,
    WiFiManager& wifi,
    Settings& settings,
    RadioInput& steeringRadio,
    RadioInput& gainRadio
)
{

    switch(page)
    {

        case 0:
            drawMainPage(
                gyro,
                settings
            );
            break;


        case 1:
            drawControlPage(
                gyro,
                settings
            );
            break;


        case 2:
            drawTunePage(
                settings
            );
            break;


        case 3:
            drawResponsePage(
                settings
            );
            break;


        case 4:
            radioSection = 0;

            drawRadioPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );
            break;


        case 5:
            radioSection = 1;

            drawRadioPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );
            break;


        case 6:
            drawWifiPage(
                wifi,
                settings
            );
            break;


        case 7:
            drawSystemPage(
                settings
            );
            break;

    }

}



void UI::changePage(
    int8_t direction,
    GyroController& gyro,
    WiFiManager& wifi,
    Settings& settings,
    RadioInput& steeringRadio,
    RadioInput& gainRadio
)
{
    if(direction > 0)
    {
        page++;

        if(page >= totalPages)
            page = 0;
    }
    else
    {
        if(page == 0)
            page = totalPages-1;
        else
            page--;
    }

    if(page != 3)
        radioSection = 0;

    drawPage(
        gyro,
        wifi,
        settings,
        steeringRadio,
        gainRadio
    );
}





void UI::drawMainPage(
    GyroController& gyro,
    Settings& settings
)
{

    lcd->fillScreen(TFT_BLACK);

    lcd->setTextColor(TFT_WHITE);



    lcd->setTextSize(3);

    lcd->drawCenterString(
        "OpenDrift",
        120,
        20
    );



    lcd->setTextSize(2);


    lcd->drawString(
        "Gain:",
        20,
        70
    );


    lcd->drawFloat(
        gyro.getGain(),
        2,
        110,
        70
    );




    lcd->drawRect(
        20,
        120,
        60,
        40,
        TFT_WHITE
    );


    lcd->drawCenterString(
        "-",
        50,
        130
    );



    lcd->drawRect(
        160,
        120,
        60,
        40,
        TFT_WHITE
    );


    lcd->drawCenterString(
        "+",
        190,
        130
    );





    lcd->drawRect(
        70,
        180,
        100,
        40,
        TFT_WHITE
    );


    lcd->drawCenterString(
        "CAL",
        120,
        190
    );



    lcd->setTextSize(1);


    lcd->drawCenterString(
        "Swipe left",
        120,
        230
    );


    drawPageDots();

}







void UI::drawControlPage(
    GyroController& gyro,
    Settings& settings
)
{

    lcd->fillScreen(
        TFT_BLACK
    );


    lcd->setTextColor(
        TFT_WHITE
    );


    lcd->setTextSize(3);


    lcd->drawCenterString(
        "Gyro",
        120,
        20
    );



    lcd->setTextSize(2);


    lcd->drawString(
        "Deadband:",
        20,
        80
    );


    lcd->drawFloat(
        gyro.getDeadband(),
        2,
        150,
        80
    );

    lcd->drawRect(
        20,
        120,
        60,
        40,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "-",
        50,
        130
    );

    lcd->drawRect(
        160,
        120,
        60,
        40,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "+",
        190,
        130
    );

    lcd->drawRect(
        50,
        175,
        140,
        35,
        TFT_WHITE
    );

    lcd->drawCenterString(
        settings.getGyroReverse() ? "GYRO REV ON" : "GYRO REV OFF",
        120,
        185
    );



    lcd->setTextSize(1);


    lcd->drawCenterString(
        "Swipe left",
        120,
        230
    );


    drawPageDots();

}







void UI::drawSystemPage(
    Settings& settings
)
{

    lcd->fillScreen(
        TFT_BLACK
    );


    lcd->setTextColor(
        TFT_WHITE
    );


    lcd->setTextSize(3);


    lcd->drawCenterString(
        "System",
        120,
        20
    );



    lcd->setTextSize(2);


    lcd->drawString(
        "OpenDrift",
        20,
        80
    );


    lcd->drawString(
        "v1.0",
        20,
        120
    );



    lcd->setTextSize(1);


    lcd->drawCenterString(
        "Swipe left: WiFi",
        120,
        230
    );


    drawPageDots();

}









void UI::drawTunePage(
    Settings& settings
)
{

    lcd->fillScreen(
        TFT_BLACK
    );

    lcd->setTextColor(
        TFT_WHITE
    );

    lcd->setTextSize(3);

    lcd->drawCenterString(
        "Gyro Tune",
        120,
        20
    );

    lcd->setTextSize(1);

    lcd->drawString(
        "MAX CORR",
        70,
        36
    );

    lcd->drawString(
        "SMOOTH",
        78,
        67
    );

    lcd->drawString(
        "I GAIN",
        78,
        98
    );

    lcd->drawString(
        "I LIM",
        84,
        129
    );

    lcd->drawString(
        "HOLD",
        86,
        160
    );

    lcd->setTextSize(2);

    lcd->drawRect(
        20,
        47,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "-",
        42,
        52
    );

    lcd->drawNumber(
        settings.getGyroMaxCorrection(),
        90,
        52
    );

    lcd->drawRect(
        176,
        47,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "+",
        198,
        52
    );

    lcd->drawRect(
        20,
        78,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "-",
        42,
        83
    );

    lcd->drawFloat(
        settings.getGyroSmoothing(),
        2,
        90,
        83
    );

    lcd->drawRect(
        176,
        78,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "+",
        198,
        83
    );

    lcd->drawRect(
        20,
        109,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "-",
        42,
        114
    );

    lcd->drawFloat(
        settings.getGyroIntegralGain(),
        2,
        90,
        114
    );

    lcd->drawRect(
        176,
        109,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "+",
        198,
        114
    );

    lcd->drawRect(
        20,
        140,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "-",
        42,
        145
    );

    lcd->drawNumber(
        settings.getGyroIntegralLimit(),
        90,
        145
    );

    lcd->drawRect(
        176,
        140,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "+",
        198,
        145
    );

    lcd->drawRect(
        20,
        171,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "-",
        42,
        176
    );

    lcd->drawNumber(
        settings.getGyroHoldBoost(),
        90,
        176
    );

    lcd->drawRect(
        176,
        171,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "+",
        198,
        176
    );

    drawPageDots();

}


void UI::drawResponsePage(
    Settings& settings
)
{

    lcd->fillScreen(
        TFT_BLACK
    );

    lcd->setTextColor(
        TFT_WHITE
    );

    lcd->setTextSize(3);

    lcd->drawCenterString(
        "Response",
        120,
        20
    );

    lcd->setTextSize(1);

    lcd->drawString(
        "ATTACK",
        78,
        43
    );

    lcd->drawString(
        "RETURN",
        76,
        77
    );

    lcd->drawString(
        "DAMP",
        86,
        111
    );

    lcd->drawString(
        "WOBBLE",
        78,
        145
    );

    lcd->setTextSize(2);

    lcd->drawRect(
        20,
        54,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "-",
        42,
        59
    );

    lcd->drawNumber(
        settings.getGyroAttackSpeed(),
        90,
        59
    );

    lcd->drawRect(
        176,
        54,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "+",
        198,
        59
    );

    lcd->drawRect(
        20,
        88,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "-",
        42,
        93
    );

    lcd->drawNumber(
        settings.getGyroReturnSpeed(),
        90,
        93
    );

    lcd->drawRect(
        176,
        88,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "+",
        198,
        93
    );

    lcd->drawRect(
        20,
        122,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "-",
        42,
        127
    );

    lcd->drawNumber(
        settings.getSteeringDamper(),
        90,
        127
    );

    lcd->drawRect(
        176,
        122,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "+",
        198,
        127
    );

    lcd->drawRect(
        20,
        156,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "-",
        42,
        161
    );

    lcd->drawNumber(
        settings.getGyroAntiWobble(),
        90,
        161
    );

    lcd->drawRect(
        176,
        156,
        44,
        28,
        TFT_WHITE
    );

    lcd->drawCenterString(
        "+",
        198,
        161
    );

    drawPageDots();

}



void UI::drawWifiPage(
    WiFiManager& wifi,
    Settings& settings
)
{

    lcd->fillScreen(
        TFT_BLACK
    );


    lcd->setTextColor(
        TFT_WHITE
    );



    lcd->setTextSize(3);


    lcd->drawCenterString(
        "WiFi",
        120,
        20
    );



    lcd->setTextSize(2);



    lcd->drawString(
        "Status:",
        20,
        70
    );



    if(wifi.isEnabled())
    {
        lcd->drawString(
            "ON",
            130,
            70
        );
    }
    else
    {
        lcd->drawString(
            "OFF",
            130,
            70
        );
    }





    lcd->drawString(
        "Clients:",
        20,
        110
    );


    if(wifi.isEnabled())
    {
        lcd->drawNumber(
            WiFi.softAPgetStationNum(),
            140,
            110
        );
    }
    else
    {
        lcd->drawNumber(
            0,
            140,
            110
        );
    }





    lcd->drawRect(
        50,
        150,
        140,
        45,
        TFT_WHITE
    );



    if(wifi.isEnabled())
    {

        lcd->drawCenterString(
            "WIFI OFF",
            120,
            162
        );

    }
    else
    {

        lcd->drawCenterString(
            "WIFI ON",
            120,
            162
        );

    }




    lcd->setTextSize(1);


    lcd->drawCenterString(
        "Swipe right",
        120,
        230
    );


    drawPageDots();

}
void UI::drawRadioPage(
    RadioInput& steeringRadio,
    RadioInput& gainRadio,
    Settings& settings,
    GyroController& gyro
)
{

    lcd->fillScreen(
        TFT_BLACK
    );

    lcd->setTextColor(
        TFT_WHITE
    );

    lcd->setTextSize(3);

    lcd->drawCenterString(
        radioSection == 0 ? "Radio" : "Steering",
        120,
        20
    );

    if(radioSection == 1)
    {
        lcd->setTextSize(2);

        lcd->drawString(
            "OUT:",
            20,
            65
        );

        lcd->setTextSize(1);

        lcd->drawString(
            "RAW:",
            20,
            85
        );

        lcd->setTextSize(2);

        lcd->drawRect(
            35,
            95,
            170,
            32,
            TFT_WHITE
        );

        lcd->drawCenterString(
            "MAX LEFT",
            120,
            103
        );

        lcd->drawRect(
            35,
            135,
            170,
            32,
            TFT_WHITE
        );

        lcd->drawCenterString(
            "CENTER",
            120,
            143
        );

        lcd->drawRect(
            35,
            175,
            170,
            32,
            TFT_WHITE
        );

        lcd->drawCenterString(
            "MAX RIGHT",
            120,
            183
        );

        lcd->drawRect(
            18,
            210,
            50,
            24,
            TFT_WHITE
        );

        lcd->drawCenterString(
            "REV",
            43,
            216
        );

        lcd->drawRect(
            82,
            210,
            28,
            24,
            TFT_WHITE
        );

        lcd->drawCenterString(
            "-",
            96,
            216
        );

        lcd->drawString(
            "TRV",
            118,
            216
        );

        lcd->drawNumber(
            settings.getRadioSteeringTravel(),
            148,
            216
        );

        lcd->drawRect(
            192,
            210,
            28,
            24,
            TFT_WHITE
        );

        lcd->drawCenterString(
            "+",
            206,
            216
        );

        lcd->drawString(
            settings.getServoReverse() ? "ON" : "OFF",
            70,
            216
        );

        lcd->setTextSize(1);

        lcd->drawCenterString(
            "Swipe left/right",
            120,
            236
        );

        updateRadioPage(
            steeringRadio,
            gainRadio,
            settings,
            gyro
        );

        return;
    }

    lcd->setTextSize(2);

    lcd->drawString(
        "STR:",
        20,
        65
    );

    lcd->drawNumber(
        steeringRadio.getPulseWidth(),
        90,
        65
    );

    lcd->drawString(
        steeringRadio.hasSignal() ? "OK" : "NO",
        170,
        65
    );

    int steeringBarX = 30;
    int steeringBarY = 92;
    int steeringBarW = 180;
    int steeringBarH = 8;

    int steeringMin =
        settings.getSteeringMin();

    int steeringCenter =
        settings.getSteeringCenter();

    int steeringMax =
        settings.getSteeringMax();

    if(steeringMax <= steeringMin)
    {
        steeringMin = 1000;
        steeringCenter = 1500;
        steeringMax = 2000;
    }

    int steeringPulse =
        constrain(
            steeringRadio.getPulseWidth(),
            steeringMin,
            steeringMax
        );

    int steeringPos =
        map(
            steeringPulse,
            steeringMin,
            steeringMax,
            steeringBarX,
            steeringBarX + steeringBarW
        );

    int steeringCenterPos =
        map(
            constrain(
                steeringCenter,
                steeringMin,
                steeringMax
            ),
            steeringMin,
            steeringMax,
            steeringBarX,
            steeringBarX + steeringBarW
        );

    lcd->drawRect(
        steeringBarX,
        steeringBarY,
        steeringBarW,
        steeringBarH,
        TFT_WHITE
    );

    lcd->drawFastVLine(
        steeringCenterPos,
        steeringBarY - 3,
        steeringBarH + 6,
        TFT_WHITE
    );

    lcd->fillRect(
        steeringPos - 2,
        steeringBarY - 4,
        5,
        steeringBarH + 8,
        steeringRadio.hasSignal() ? TFT_GREEN : TFT_RED
    );

    lcd->drawString(
        "GAIN:",
        20,
        112
    );

    lcd->drawNumber(
        gainRadio.getPulseWidth(),
        100,
        112
    );

    lcd->drawString(
        gainRadio.hasSignal() ? "OK" : "NO",
        170,
        112
    );

    int gainBarX = 30;
    int gainBarY = 140;
    int gainBarW = 180;
    int gainBarH = 8;

    int gainMin =
        settings.getGainMin();

    int gainMax =
        settings.getGainMax();

    if(gainMax <= gainMin)
    {
        gainMin = 1000;
        gainMax = 2000;
    }

    int gainPulse =
        constrain(
            gainRadio.getPulseWidth(),
            gainMin,
            gainMax
        );

    int gainPos =
        map(
            gainPulse,
            gainMin,
            gainMax,
            gainBarX,
            gainBarX + gainBarW
        );

    lcd->drawRect(
        gainBarX,
        gainBarY,
        gainBarW,
        gainBarH,
        TFT_WHITE
    );

    lcd->fillRect(
        gainPos - 2,
        gainBarY - 4,
        5,
        gainBarH + 8,
        gainRadio.hasSignal() ? TFT_GREEN : TFT_RED
    );

    lcd->drawString(
        "G:",
        20,
        158
    );

    lcd->drawFloat(
        gyro.getGain(),
        2,
        60,
        158
    );

    lcd->setTextSize(1);

    lcd->drawString(
        "S:",
        20,
        184
    );

    lcd->drawNumber(
        settings.getSteeringMin(),
        40,
        184
    );

    lcd->drawNumber(
        settings.getSteeringCenter(),
        90,
        184
    );

    lcd->drawNumber(
        settings.getSteeringMax(),
        140,
        184
    );

    lcd->drawString(
        "G:",
        20,
        202
    );

    lcd->drawNumber(
        settings.getGainMin(),
        40,
        202
    );

    lcd->drawNumber(
        settings.getGainMax(),
        100,
        202
    );

    drawPageDots();

}





void UI::updateRadioPage(
    RadioInput& steeringRadio,
    RadioInput& gainRadio,
    Settings& settings,
    GyroController& gyro
)
{

    lcd->setTextColor(
        TFT_WHITE,
        TFT_BLACK
    );

    if(radioSection == 1)
    {
        lcd->setTextSize(2);

        lcd->fillRect(
            105,
            65,
            100,
            30,
            TFT_BLACK
        );

        lcd->drawNumber(
            mapSteeringForDisplay(
                steeringRadio.getPulseWidth(),
                settings
            ),
            105,
            65
        );

        lcd->setTextSize(1);

        lcd->fillRect(
            20,
            84,
            200,
            10,
            TFT_BLACK
        );

        lcd->drawNumber(
            steeringRadio.getPulseWidth(),
            58,
            85
        );

        lcd->drawString(
            "L",
            20,
            84
        );

        lcd->drawNumber(
            settings.getSteeringMin(),
            48,
            84
        );

        lcd->drawString(
            "CTR",
            85,
            84
        );

        lcd->drawNumber(
            settings.getSteeringCenter(),
            113,
            84
        );

        lcd->drawString(
            "R",
            150,
            84
        );

        lcd->drawNumber(
            settings.getSteeringMax(),
            178,
            84
        );

        lcd->fillRect(
            68,
            216,
            120,
            10,
            TFT_BLACK
        );

        lcd->drawString(
            settings.getServoReverse() ? "ON" : "OFF",
            70,
            216
        );

        lcd->drawString(
            "TRV",
            118,
            216
        );

        lcd->drawNumber(
            settings.getRadioSteeringTravel(),
            148,
            216
        );

        lcd->setTextColor(
            TFT_WHITE
        );

        return;
    }

    lcd->setTextSize(2);

    lcd->fillRect(
        90,
        65,
        130,
        20,
        TFT_BLACK
    );

    lcd->drawNumber(
        steeringRadio.getPulseWidth(),
        90,
        65
    );

    lcd->drawString(
        steeringRadio.hasSignal() ? "OK" : "NO",
        170,
        65
    );

    int steeringBarX = 30;
    int steeringBarY = 92;
    int steeringBarW = 180;
    int steeringBarH = 8;

    int steeringMin =
        settings.getSteeringMin();

    int steeringCenter =
        settings.getSteeringCenter();

    int steeringMax =
        settings.getSteeringMax();

    if(steeringMax <= steeringMin)
    {
        steeringMin = 1000;
        steeringCenter = 1500;
        steeringMax = 2000;
    }

    int steeringPulse =
        constrain(
            steeringRadio.getPulseWidth(),
            steeringMin,
            steeringMax
        );

    int steeringPos =
        map(
            steeringPulse,
            steeringMin,
            steeringMax,
            steeringBarX,
            steeringBarX + steeringBarW
        );

    int steeringCenterPos =
        map(
            constrain(
                steeringCenter,
                steeringMin,
                steeringMax
            ),
            steeringMin,
            steeringMax,
            steeringBarX,
            steeringBarX + steeringBarW
        );

    lcd->fillRect(
        steeringBarX - 3,
        steeringBarY - 5,
        steeringBarW + 6,
        steeringBarH + 10,
        TFT_BLACK
    );

    lcd->drawRect(
        steeringBarX,
        steeringBarY,
        steeringBarW,
        steeringBarH,
        TFT_WHITE
    );

    lcd->drawFastVLine(
        steeringCenterPos,
        steeringBarY - 3,
        steeringBarH + 6,
        TFT_WHITE
    );

    lcd->fillRect(
        steeringPos - 2,
        steeringBarY - 4,
        5,
        steeringBarH + 8,
        steeringRadio.hasSignal() ? TFT_GREEN : TFT_RED
    );

    lcd->fillRect(
        100,
        112,
        120,
        20,
        TFT_BLACK
    );

    lcd->drawNumber(
        gainRadio.getPulseWidth(),
        100,
        112
    );

    lcd->drawString(
        gainRadio.hasSignal() ? "OK" : "NO",
        170,
        112
    );

    int gainBarX = 30;
    int gainBarY = 140;
    int gainBarW = 180;
    int gainBarH = 8;

    int gainMin =
        settings.getGainMin();

    int gainMax =
        settings.getGainMax();

    if(gainMax <= gainMin)
    {
        gainMin = 1000;
        gainMax = 2000;
    }

    int gainPulse =
        constrain(
            gainRadio.getPulseWidth(),
            gainMin,
            gainMax
        );

    int gainPos =
        map(
            gainPulse,
            gainMin,
            gainMax,
            gainBarX,
            gainBarX + gainBarW
        );

    lcd->fillRect(
        gainBarX - 3,
        gainBarY - 5,
        gainBarW + 6,
        gainBarH + 10,
        TFT_BLACK
    );

    lcd->drawRect(
        gainBarX,
        gainBarY,
        gainBarW,
        gainBarH,
        TFT_WHITE
    );

    lcd->fillRect(
        gainPos - 2,
        gainBarY - 4,
        5,
        gainBarH + 8,
        gainRadio.hasSignal() ? TFT_GREEN : TFT_RED
    );

    lcd->fillRect(
        60,
        158,
        80,
        20,
        TFT_BLACK
    );

    lcd->drawFloat(
        gyro.getGain(),
        2,
        60,
        158
    );

    lcd->setTextSize(1);

    lcd->fillRect(
        40,
        184,
        160,
        30,
        TFT_BLACK
    );

    lcd->drawNumber(
        settings.getSteeringMin(),
        40,
        184
    );

    lcd->drawNumber(
        settings.getSteeringCenter(),
        90,
        184
    );

    lcd->drawNumber(
        settings.getSteeringMax(),
        140,
        184
    );

    lcd->drawNumber(
        settings.getGainMin(),
        40,
        202
    );

    lcd->drawNumber(
        settings.getGainMax(),
        100,
        202
    );

    lcd->setTextColor(
        TFT_WHITE
    );

}





void UI::drawPageDots()
{
    int spacing = 20;

    int startX =
        120 -
        (
            (totalPages - 1)
            *
            spacing
            /
            2
        );

    for(
        int i = 0;
        i < totalPages;
        i++
    )
    {

        if(i == page)
        {
            lcd->fillCircle(
                startX + (i*spacing),
                215,
                5,
                TFT_WHITE
            );
        }
        else
        {
            lcd->drawCircle(
                startX + (i*spacing),
                215,
                5,
                TFT_WHITE
            );
        }

    }

}










bool UI::buttonPressed(
    uint16_t x,
    uint16_t y,
    uint16_t bx,
    uint16_t by,
    uint16_t bw,
    uint16_t bh
)
{

    return(
        x >= bx &&
        x <= bx+bw &&
        y >= by &&
        y <= by+bh
    );

}


int8_t UI::repeatButtonAt(
    uint16_t x,
    uint16_t y
)
{
    if(page == 0)
    {
        if(buttonPressed(x, y, 20, 120, 60, 40))
            return 1;

        if(buttonPressed(x, y, 160, 120, 60, 40))
            return 2;
    }

    if(page == 1)
    {
        if(buttonPressed(x, y, 20, 120, 60, 40))
            return 3;

        if(buttonPressed(x, y, 160, 120, 60, 40))
            return 4;
    }

    if(page == 2)
    {
        if(buttonPressed(x, y, 20, 47, 44, 28))
            return 5;

        if(buttonPressed(x, y, 176, 47, 44, 28))
            return 6;

        if(buttonPressed(x, y, 20, 78, 44, 28))
            return 7;

        if(buttonPressed(x, y, 176, 78, 44, 28))
            return 8;

        if(buttonPressed(x, y, 20, 109, 44, 28))
            return 15;

        if(buttonPressed(x, y, 176, 109, 44, 28))
            return 16;

        if(buttonPressed(x, y, 20, 140, 44, 28))
            return 17;

        if(buttonPressed(x, y, 176, 140, 44, 28))
            return 18;

        if(buttonPressed(x, y, 20, 171, 44, 28))
            return 19;

        if(buttonPressed(x, y, 176, 171, 44, 28))
            return 20;
    }

    if(page == 3)
    {
        if(buttonPressed(x, y, 20, 54, 44, 28))
            return 9;

        if(buttonPressed(x, y, 176, 54, 44, 28))
            return 10;

        if(buttonPressed(x, y, 20, 88, 44, 28))
            return 11;

        if(buttonPressed(x, y, 176, 88, 44, 28))
            return 12;

        if(buttonPressed(x, y, 20, 122, 44, 28))
            return 13;

        if(buttonPressed(x, y, 176, 122, 44, 28))
            return 14;

        if(buttonPressed(x, y, 20, 156, 44, 28))
            return 21;

        if(buttonPressed(x, y, 176, 156, 44, 28))
            return 22;
    }

    return 0;
}



bool UI::applyRepeatButton(
    int8_t button,
    GyroController& gyro,
    Settings& settings
)
{
    switch(button)
    {
        case 1:
        {
            float gain =
                gyro.getGain() - 0.01f;

            gyro.setGain(gain);

            settings.setGain(gain);

            drawMainPage(
                gyro,
                settings
            );

            return true;
        }

        case 2:
        {
            float gain =
                gyro.getGain() + 0.01f;

            gyro.setGain(gain);

            settings.setGain(gain);

            drawMainPage(
                gyro,
                settings
            );

            return true;
        }

        case 3:
        {
            float deadband =
                gyro.getDeadband() - 1.0f;

            if(deadband < 0)
                deadband = 0;

            gyro.setDeadband(deadband);

            settings.setDeadband(deadband);

            drawControlPage(
                gyro,
                settings
            );

            return true;
        }

        case 4:
        {
            float deadband =
                gyro.getDeadband() + 1.0f;

            gyro.setDeadband(deadband);

            settings.setDeadband(deadband);

            drawControlPage(
                gyro,
                settings
            );

            return true;
        }

        case 5:
            settings.setGyroMaxCorrection(
                settings.getGyroMaxCorrection() - 1
            );
            break;

        case 6:
            settings.setGyroMaxCorrection(
                settings.getGyroMaxCorrection() + 1
            );
            break;

        case 7:
            settings.setGyroSmoothing(
                settings.getGyroSmoothing() - 0.01f
            );
            break;

        case 8:
            settings.setGyroSmoothing(
                settings.getGyroSmoothing() + 0.01f
            );
            break;

        case 9:
            settings.setGyroAttackSpeed(
                settings.getGyroAttackSpeed() - 1
            );
            break;

        case 10:
            settings.setGyroAttackSpeed(
                settings.getGyroAttackSpeed() + 1
            );
            break;

        case 11:
            settings.setGyroReturnSpeed(
                settings.getGyroReturnSpeed() - 1
            );
            break;

        case 12:
            settings.setGyroReturnSpeed(
                settings.getGyroReturnSpeed() + 1
            );
            break;

        case 13:
            settings.setSteeringDamper(
                settings.getSteeringDamper() - 1
            );
            break;

        case 14:
            settings.setSteeringDamper(
                settings.getSteeringDamper() + 1
            );
            break;

        case 15:
            settings.setGyroIntegralGain(
                settings.getGyroIntegralGain() - 0.01f
            );
            break;

        case 16:
            settings.setGyroIntegralGain(
                settings.getGyroIntegralGain() + 0.01f
            );
            break;

        case 17:
            settings.setGyroIntegralLimit(
                settings.getGyroIntegralLimit() - 1
            );
            break;

        case 18:
            settings.setGyroIntegralLimit(
                settings.getGyroIntegralLimit() + 1
            );
            break;

        case 19:
            settings.setGyroHoldBoost(
                settings.getGyroHoldBoost() - 1
            );
            break;

        case 20:
            settings.setGyroHoldBoost(
                settings.getGyroHoldBoost() + 1
            );
            break;

        case 21:
            settings.setGyroAntiWobble(
                settings.getGyroAntiWobble() - 1
            );

            gyro.setAntiWobble(
                settings.getGyroAntiWobble()
            );
            break;

        case 22:
            settings.setGyroAntiWobble(
                settings.getGyroAntiWobble() + 1
            );

            gyro.setAntiWobble(
                settings.getGyroAntiWobble()
            );
            break;

        default:
            return false;
    }

    if(
        (
            button >= 9 &&
            button <= 14
        )
        ||
        (
            button >= 21 &&
            button <= 22
        )
    )
    {
        drawResponsePage(
            settings
        );
    }
    else
    {
        drawTunePage(
            settings
        );
    }

    return true;
}









void UI::update(
    Touch& touch,
    GyroController& gyro,
    IMU& imu,
    WiFiManager& wifi,
    Settings& settings,
    RadioInput& steeringRadio,
    RadioInput& gainRadio
)
{

    bool touched =
        touch.isTouched();

    uint8_t gesture =
        touch.getGesture();

    if(
        gesture == SWIPE_LEFT &&
        millis() - lastPageSwipe > 350
    )
    {
        lastPageSwipe =
            millis();

        changePage(
            1,
            gyro,
            wifi,
            settings,
            steeringRadio,
            gainRadio
        );

        trackingSwipe = false;

        heldRepeatButton = 0;

        nextRepeatAt = 0;

        lastTouchState =
            false;

        return;
    }

    if(
        gesture == SWIPE_RIGHT &&
        millis() - lastPageSwipe > 350
    )
    {
        lastPageSwipe =
            millis();

        changePage(
            -1,
            gyro,
            wifi,
            settings,
            steeringRadio,
            gainRadio
        );

        trackingSwipe = false;

        heldRepeatButton = 0;

        nextRepeatAt = 0;

        lastTouchState =
            false;

        return;
    }

    if(
        (
            page == 4 ||
            page == 5
        ) &&
        !touched &&
        millis() - lastRadioRefresh > 250
    )
    {
        updateRadioPage(
            steeringRadio,
            gainRadio,
            settings,
            gyro
        );

        lastRadioRefresh =
            millis();
    }



    if(
        touched &&
        !lastTouchState
    )
    {

        touchStartX =
            touch.getX();

        touchStartY =
            touch.getY();

        trackingSwipe = true;

        heldRepeatButton = 0;

    }





    if(
        !touched &&
        lastTouchState
    )
    {

        int delta =
            touch.getX()
            -
            touchStartX;

        int deltaY =
            touch.getY()
            -
            touchStartY;



        if(trackingSwipe)
        {

            if(
                false &&
                page == 4 &&
                abs(deltaY) > 50 &&
                abs(deltaY) > abs(delta)
            )
            {

                if(deltaY < 0)
                    radioSection = 1;
                else
                    radioSection = 0;


                drawPage(
                    gyro,
                    wifi,
                    settings,
                    steeringRadio,
                    gainRadio
                );

            }


            else if(delta < -50)
            {

                changePage(
                    1,
                    gyro,
                    wifi,
                    settings,
                    steeringRadio,
                    gainRadio
                );

            }


            else if(delta > 50)
            {

                changePage(
                    -1,
                    gyro,
                    wifi,
                    settings,
                    steeringRadio,
                    gainRadio
                );

            }

        }


        trackingSwipe=false;

        heldRepeatButton = 0;

        nextRepeatAt = 0;

    }







    if(
        touched &&
        !lastTouchState
    )
    {

        uint16_t x =
            touch.getX();

        uint16_t y =
            touch.getY();




        // MAIN PAGE BUTTONS

        if(page == 0)
        {


            if(buttonPressed(
                x,y,
                20,120,
                60,40
            ))
            {

                float g =
                    gyro.getGain()-0.01f;


                gyro.setGain(g);

                settings.setGain(g);

            }




            if(buttonPressed(
                x,y,
                160,120,
                60,40
            ))
            {

                float g =
                    gyro.getGain()+0.01f;


                gyro.setGain(g);

                settings.setGain(g);

            }




            if(buttonPressed(
                x,y,
                70,180,
                100,40
            ))
            {

                imu.update();

                gyro.calibrate(
                    imu.getYawRate()
                );

            }


            drawMainPage(
                gyro,
                settings
            );

        }







        // CONTROL PAGE BUTTONS

        if(page == 1)
        {

            if(buttonPressed(
                x,y,
                20,120,
                60,40
            ))
            {

                float deadband =
                    gyro.getDeadband()-1.0f;


                if(deadband < 0)
                    deadband = 0;


                gyro.setDeadband(deadband);

                settings.setDeadband(deadband);

            }




            if(buttonPressed(
                x,y,
                160,120,
                60,40
            ))
            {

                float deadband =
                    gyro.getDeadband()+1.0f;


                gyro.setDeadband(deadband);

                settings.setDeadband(deadband);

            }

            if(buttonPressed(
                x,y,
                50,175,
                140,35
            ))
            {

                settings.setGyroReverse(
                    !settings.getGyroReverse()
                );

            }


            drawControlPage(
                gyro,
                settings
            );

        }







        // TUNE PAGE BUTTONS

        if(page == 2)
        {

            if(buttonPressed(
                x,y,
                20,54,
                44,28
            ))
            {

                settings.setGyroMaxCorrection(
                    settings.getGyroMaxCorrection() - 1
                );

            }


            if(buttonPressed(
                x,y,
                176,54,
                44,28
            ))
            {

                settings.setGyroMaxCorrection(
                    settings.getGyroMaxCorrection() + 1
                );

            }


            if(buttonPressed(
                x,y,
                20,88,
                44,28
            ))
            {

                settings.setGyroSmoothing(
                    settings.getGyroSmoothing() - 0.01f
                );

            }


            if(buttonPressed(
                x,y,
                176,88,
                44,28
            ))
            {

                settings.setGyroSmoothing(
                    settings.getGyroSmoothing() + 0.01f
                );

            }

            if(buttonPressed(
                x,y,
                20,122,
                44,28
            ))
            {

                settings.setGyroIntegralGain(
                    settings.getGyroIntegralGain() - 0.01f
                );

            }


            if(buttonPressed(
                x,y,
                176,122,
                44,28
            ))
            {

                settings.setGyroIntegralGain(
                    settings.getGyroIntegralGain() + 0.01f
                );

            }


            if(buttonPressed(
                x,y,
                20,156,
                44,28
            ))
            {

                settings.setGyroIntegralLimit(
                    settings.getGyroIntegralLimit() - 1
                );

            }


            if(buttonPressed(
                x,y,
                176,156,
                44,28
            ))
            {

                settings.setGyroIntegralLimit(
                    settings.getGyroIntegralLimit() + 1
                );

            }

            drawTunePage(
                settings
            );

        }







        // RESPONSE PAGE BUTTONS

        if(page == 3)
        {

            if(buttonPressed(
                x,y,
                20,68,
                44,32
            ))
            {

                settings.setGyroAttackSpeed(
                    settings.getGyroAttackSpeed() - 1
                );

            }


            if(buttonPressed(
                x,y,
                176,68,
                44,32
            ))
            {

                settings.setGyroAttackSpeed(
                    settings.getGyroAttackSpeed() + 1
                );

            }


            if(buttonPressed(
                x,y,
                20,118,
                44,32
            ))
            {

                settings.setGyroReturnSpeed(
                    settings.getGyroReturnSpeed() - 1
                );

            }


            if(buttonPressed(
                x,y,
                176,118,
                44,32
            ))
            {

                settings.setGyroReturnSpeed(
                    settings.getGyroReturnSpeed() + 1
                );

            }

            if(buttonPressed(
                x,y,
                20,168,
                44,32
            ))
            {

                settings.setSteeringDamper(
                    settings.getSteeringDamper() - 1
                );

            }


            if(buttonPressed(
                x,y,
                176,168,
                44,32
            ))
            {

                settings.setSteeringDamper(
                    settings.getSteeringDamper() + 1
                );

            }


            drawResponsePage(
                settings
            );

        }







        // RADIO PAGE BUTTONS

        if(
            page == 5
        )
        {

            if(
                steeringRadio.hasSignal() &&
                buttonPressed(
                    x,y,
                    35,95,
                    170,32
                )
            )
            {

                settings.setSteeringMin(
                    steeringRadio.getPulseWidth()
                );

            }


            if(
                steeringRadio.hasSignal() &&
                buttonPressed(
                    x,y,
                    35,135,
                    170,32
                )
            )
            {

                settings.setSteeringCenter(
                    steeringRadio.getPulseWidth()
                );

            }


            if(
                steeringRadio.hasSignal() &&
                buttonPressed(
                    x,y,
                    35,175,
                    170,32
                )
            )
            {

                settings.setSteeringMax(
                    steeringRadio.getPulseWidth()
                );

            }

            if(buttonPressed(
                x,y,
                18,210,
                50,24
            ))
            {

                settings.setServoReverse(
                    !settings.getServoReverse()
                );

            }

            if(buttonPressed(
                x,y,
                82,210,
                28,24
            ))
            {

                settings.setRadioSteeringTravel(
                    settings.getRadioSteeringTravel() - 1
                );

            }


            if(buttonPressed(
                x,y,
                192,210,
                28,24
            ))
            {

                settings.setRadioSteeringTravel(
                    settings.getRadioSteeringTravel() + 1
                );

            }


            drawRadioPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );

        }







        // WIFI PAGE BUTTON

        if(page == 6)
        {

            if(buttonPressed(
                x,y,
                50,
                150,
                140,
                45
            ))
            {

                if(wifi.isEnabled())
                {

                    wifi.disable();

                    settings.setWifiEnabled(false);

                }
                else
                {

                    wifi.enable();

                    settings.setWifiEnabled(true);

                }


                drawWifiPage(
                    wifi,
                    settings
                );

            }

        }

    }

    lastTouchState =
        touched;

}
