#include "UI.h"



void UI::begin(
    LGFX* display,
    GyroController& gyro,
    WiFiManager& wifi,
    Settings& settings
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
    Settings& settings
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
    Settings& settings
)
{

    bool touched =
        touch.isTouched();



    if(
        touched &&
        !lastTouchState
    )
    {

        touchStartX =
            touch.getX();

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



        if(trackingSwipe)
        {

            if(delta < -50)
            {

                page++;

                if(page >= totalPages)
                    page = 0;


                drawPage(
                    gyro,
                    wifi,
                    settings
                );

            }


            else if(delta > 50)
            {

                if(page == 0)
                    page = totalPages-1;
                else
                    page--;


                drawPage(
                    gyro,
                    wifi,
                    settings
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


            drawControlPage(
                gyro,
                settings
            );

        }







        // WIFI PAGE BUTTON

        if(page == 3)
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
