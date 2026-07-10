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

    if(
        steeringCenter <= steeringMin ||
        steeringCenter >= steeringMax
    )
    {
        return constrain(
            pulse,
            1000,
            2000
        );
    }

    if(pulse < steeringCenter)
    {
        return map(
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

    return map(
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
            drawSystemPage(
                settings
            );
            break;


        case 3:
            radioSection = 0;

            drawRadioPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );
            break;


        case 4:
            radioSection = 1;

            drawRadioPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );
            break;


        case 5:
            drawWifiPage(
                wifi,
                settings
            );
            break;

    }

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
        "Control",
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
            35,
            210,
            80,
            24,
            TFT_WHITE
        );

        lcd->drawCenterString(
            "REV",
            75,
            216
        );

        lcd->drawString(
            settings.getServoReverse() ? "ON" : "OFF",
            140,
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

        lcd->drawNumber(
            steeringRadio.getPulseWidth(),
            58,
            85
        );

        lcd->fillRect(
            20,
            84,
            200,
            10,
            TFT_BLACK
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
            140,
            216,
            50,
            10,
            TFT_BLACK
        );

        lcd->drawString(
            settings.getServoReverse() ? "ON" : "OFF",
            140,
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

    for(
        int i = 0;
        i < totalPages;
        i++
    )
    {

        if(i == page)
        {
            lcd->fillCircle(
                80 + (i*25),
                215,
                5,
                TFT_WHITE
            );
        }
        else
        {
            lcd->drawCircle(
                80 + (i*25),
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

    if(
        (
            page == 3 ||
            page == 4
        ) &&
        millis() - lastRadioRefresh > 100
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
                page == 3 &&
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

                page++;

                if(page >= totalPages)
                    page = 0;

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


            else if(delta > 50)
            {

                if(page == 0)
                    page = totalPages-1;
                else
                    page--;

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

        }


        trackingSwipe=false;

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
                    gyro.getGain()-0.1f;


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
                    gyro.getGain()+0.1f;


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
                    gyro.getDeadband()-0.5f;


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
                    gyro.getDeadband()+0.5f;


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







        // RADIO PAGE BUTTONS

        if(
            page == 4
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
                35,210,
                80,24
            ))
            {

                settings.setServoReverse(
                    !settings.getServoReverse()
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

        if(page == 5)
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
