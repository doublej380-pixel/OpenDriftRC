#include "UI.h"




void UI::begin(
    LGFX* display
)
{

    lcd = display;


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

}






void UI::drawScreen(
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



    // Gain display

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




    // Minus button

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




    // Plus button

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





    // Calibration button

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

    return
    (
        x >= bx &&
        x <= bx + bw &&
        y >= by &&
        y <= by + bh
    );

}








void UI::update(
    Touch& touch,
    GyroController& gyro,
    IMU& imu
)
{

    bool currentTouch =
        touch.isTouched();



    // Detect only touch start

    if(
        currentTouch &&
        !lastTouchState
    )
    {

        if(
            millis() -
            lastPressTime
            > 250
        )
        {


            uint16_t x =
                touch.getX();


            uint16_t y =
                touch.getY();



            Serial.print("Touch X:");
            Serial.print(x);


            Serial.print(" Y:");
            Serial.println(y);





            // GAIN DOWN

            if(buttonPressed(
                x,
                y,
                20,
                120,
                60,
                40
            ))
            {

                gyro.setGain(
                    gyro.getGain() - 0.1f
                );


                Serial.println(
                    "GAIN DOWN"
                );

            }






            // GAIN UP

            if(buttonPressed(
                x,
                y,
                160,
                120,
                60,
                40
            ))
            {

                gyro.setGain(
                    gyro.getGain() + 0.1f
                );


                Serial.println(
                    "GAIN UP"
                );

            }






            // CALIBRATE

            if(buttonPressed(
                x,
                y,
                70,
                180,
                100,
                40
            ))
            {

                imu.update();


                gyro.calibrate(
                    imu.getYawRate()
                );


                Serial.println(
                    "CALIBRATED"
                );

            }





            // Limit gain

            if(
                gyro.getGain()
                < 0.1f
            )
            {
                gyro.setGain(0.1f);
            }



            if(
                gyro.getGain()
                > 10.0f
            )
            {
                gyro.setGain(10.0f);
            }




            drawScreen(
                gyro
            );



            lastPressTime =
                millis();

        }

    }




    lastTouchState =
        currentTouch;

}