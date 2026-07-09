#include "UI.h"



void UI::begin(
    LGFX* display
)
{
    lcd = display;

    page = 0;
}





void UI::drawPage(
    GyroController& gyro,
    WiFiManager& wifi
)
{

    if(page == 0)
    {
        drawMainPage(
            gyro
        );
    }
    else
    {
        drawWifiPage(
            wifi
        );
    }

}







void UI::drawMainPage(
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
        "Swipe left: Settings",
        120,
        230
    );

}









void UI::drawWifiPage(
    WiFiManager& wifi
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
        "Settings",
        120,
        20
    );



    lcd->setTextSize(2);



    lcd->drawString(
        "WiFi:",
        20,
        70
    );



    if(wifi.isEnabled())
    {

        lcd->drawString(
            "ON",
            100,
            70
        );

    }
    else
    {

        lcd->drawString(
            "OFF",
            100,
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
            130,
            110
        );

    }
    else
    {

        lcd->drawNumber(
            0,
            130,
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
        "Swipe right: Main",
        120,
        230
    );

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

    return (
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
    WiFiManager& wifi
)
{

    bool touched =
        touch.isTouched();




    if(touched && !lastTouchState)
    {

        touchStartX =
            touch.getX();


        trackingSwipe = true;

    }







    if(!touched && lastTouchState)
    {

        int endX =
            touch.getX();


        int delta =
            endX - touchStartX;





        if(
            trackingSwipe &&
            delta < -50
        )
        {

            page = 1;

            drawPage(
                gyro,
                wifi
            );

        }




        else if(
            trackingSwipe &&
            delta > 50
        )
        {

            page = 0;

            drawPage(
                gyro,
                wifi
            );

        }



        trackingSwipe = false;

    }








    if(
        page == 0 &&
        touched &&
        !lastTouchState
    )
    {


        uint16_t x =
            touch.getX();


        uint16_t y =
            touch.getY();




        if(buttonPressed(
            x,y,
            20,120,
            60,40))
        {

            gyro.setGain(
                gyro.getGain()-0.1f
            );

        }



        if(buttonPressed(
            x,y,
            160,120,
            60,40))
        {

            gyro.setGain(
                gyro.getGain()+0.1f
            );

        }





        if(buttonPressed(
            x,y,
            70,180,
            100,40))
        {

            imu.update();

            gyro.calibrate(
                imu.getYawRate()
            );

        }


        drawMainPage(
            gyro
        );

    }









    if(
        page == 1 &&
        touched &&
        !lastTouchState
    )
    {

        if(buttonPressed(
            touch.getX(),
            touch.getY(),
            50,
            150,
            140,
            45
        ))
        {


            if(wifi.isEnabled())
            {

                wifi.disable();

            }
            else
            {

                wifi.enable();

            }



            drawWifiPage(
                wifi
            );

        }

    }





    lastTouchState =
        touched;

}