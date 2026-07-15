#include "UI.h"


#if defined(OPENDRIFT_BOARD_AMOLED_164)
#include "../../assets/backgrounds/background.c"

static constexpr int OD_BACKGROUND_WIDTH = 456;
static constexpr int OD_BACKGROUND_HEIGHT = 280;
static constexpr float OD_TEXT_SCALE = 1.15f;

// LovyanGFX supports fractional text scaling. This enlarges all AMOLED
// typography without adding another rendering pass.
#define setTextSize(size) setTextSize(static_cast<float>(size) * OD_TEXT_SCALE)

static_assert(
    sizeof(background_map) ==
        OD_BACKGROUND_WIDTH * OD_BACKGROUND_HEIGHT * 2,
    "AMOLED background must be a 456x280 RGB565 image"
);

static constexpr uint16_t OD_BG = TFT_BLACK;
static constexpr uint16_t OD_TEXT = 0xFFFF;
static constexpr uint16_t OD_MUTED = 0x9CF3;
static constexpr uint16_t OD_DIM = 0x3186;
static constexpr uint16_t OD_CYAN = 0x07FF;
static constexpr uint16_t OD_BLUE = 0x3D9F;
static constexpr uint16_t OD_MAGENTA = 0xF81F;
static constexpr uint16_t OD_AMBER = 0xFD20;
static constexpr uint16_t OD_GREEN = 0x07E0;
static constexpr uint16_t OD_RED = 0xF800;


static void drawUiBackground(
    LGFX_Sprite* lcd
)
{
    // Page canvases use black as a transparent color. The fixed
    // background is added later while composing the physical display.
    lcd->fillScreen(
        TFT_BLACK
    );
}


static uint16_t readBackgroundPixel(
    int x,
    int y
)
{
    const uint16_t* pixels =
        reinterpret_cast<const uint16_t*>(background_map);

    return pgm_read_word(
        pixels + (y * OD_BACKGROUND_WIDTH) + x
    );
}


static uint16_t readBackgroundPixelRaw(
    int x,
    int y
)
{
    uint16_t color =
        readBackgroundPixel(x, y);

    // A 16-bit LovyanGFX sprite stores RGB565 in wire (big-endian)
    // order, while the generated image array contains native RGB565.
    return
        static_cast<uint16_t>(
            (color << 8) |
            (color >> 8)
        );
}


static void drawAmoledHeader(
    LGFX_Sprite* lcd,
    const char* title,
    uint16_t accent
)
{
    lcd->setTextSize(
        2
    );

    lcd->setTextColor(
        accent
    );

    lcd->drawString(
        title,
        18,
        16
    );

    lcd->drawFastHLine(
        18,
        42,
        128,
        accent
    );

    lcd->drawFastHLine(
        150,
        42,
        48,
        OD_DIM
    );
}


static void drawAmoledButton(
    LGFX_Sprite* lcd,
    int x,
    int y,
    int w,
    int h,
    const char* label,
    uint16_t accent,
    uint8_t textSize = 2
)
{
    lcd->drawRect(
        x,
        y,
        w,
        h,
        accent
    );

    lcd->setTextSize(
        textSize
    );

    lcd->setTextColor(
        OD_TEXT
    );

    lcd->drawCenterString(
        label,
        x + (w / 2),
        y + ((h - (textSize * 8)) / 2)
    );
}
#else
static void drawUiBackground(
    LGFX_Sprite* lcd
)
{
    lcd->fillScreen(
        TFT_BLACK
    );
}
#endif


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
    this->display = display;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    bool usePsram =
        psramFound();

    canvas.setPsram(
        usePsram
    );

    transitionCanvas.setPsram(
        usePsram
    );

    panelCanvas.setPsram(
        usePsram
    );

    canvas.setColorDepth(
        16
    );

    transitionCanvas.setColorDepth(
        16
    );

    panelCanvas.setColorDepth(
        16
    );
    #else
    canvas.setColorDepth(
        8
    );
    #endif

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    canvasReady =
        canvas.createSprite(
            UI_CANVAS_WIDTH,
            UI_CANVAS_HEIGHT
        )
        !=
        nullptr;

    if(canvasReady)
    {
        canvas.setPivot(
            UI_CANVAS_WIDTH / 2,
            UI_CANVAS_HEIGHT / 2
        );
    }

    panelCanvasReady =
        panelCanvas.createSprite(
            280,
            456
        )
        !=
        nullptr;

    transitionCanvasReady =
        transitionCanvas.createSprite(
            UI_CANVAS_WIDTH,
            UI_CANVAS_HEIGHT
        )
        !=
        nullptr;
    #else
    canvasReady =
        canvas.createSprite(
            UI_CANVAS_WIDTH,
            UI_CANVAS_HEIGHT
        )
        !=
        nullptr;
    #endif

    Serial.print(
        "UI canvas: "
    );

    Serial.println(
        canvasReady ? "OK" : "FAIL"
    );

    Serial.print(
        "UI display: "
    );

    Serial.print(
        display->width()
    );

    Serial.print(
        "x"
    );

    Serial.println(
        display->height()
    );

    Serial.print(
        "UI canvas size: "
    );

    Serial.print(
        UI_CANVAS_WIDTH
    );

    Serial.print(
        "x"
    );

    Serial.println(
        UI_CANVAS_HEIGHT
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    Serial.print(
        "UI panel canvas: "
    );

    Serial.println(
        panelCanvasReady ? "OK" : "FAIL"
    );

    Serial.print(
        "UI transition canvas: "
    );

    Serial.println(
        transitionCanvasReady ? "OK" : "FAIL"
    );

    if(
        canvasReady &&
        panelCanvasReady &&
        transitionCanvasReady
    )
    {
        Serial.print(
            "UI buffers: "
        );

        Serial.print(
            canvas.bufferLength()
        );

        Serial.print(
            " / "
        );

        Serial.print(
            transitionCanvas.bufferLength()
        );

        Serial.print(
            " / "
        );

        Serial.println(
            panelCanvas.bufferLength()
        );

        Serial.print(
            "UI raw animation: "
        );

        Serial.println(
            canUseRawAmoledBuffers() ? "ON" : "OFF"
        );
    }

    Serial.print(
        "UI canvas mode: "
    );

    Serial.println(
        usePsram ? "PSRAM 16-bit" : "internal 16-bit"
    );
    #endif

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    canvas.setTextWrap(
        false
    );

    panelCanvas.setTextWrap(
        false
    );

    transitionCanvas.setTextWrap(
        false
    );

    lcd =
        &canvas;
    #else
    canvas.setTextWrap(
        false
    );

    lcd =
        &canvas;
    #endif

    display->fillScreen(
        TFT_BLACK
    );

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
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(canUseRawAmoledBuffers())
    {
        uint8_t targetPage =
            page;

        if(direction > 0)
        {
            targetPage++;

            if(targetPage >= totalPages)
                targetPage = 0;
        }
        else
        {
            if(targetPage == 0)
                targetPage = totalPages - 1;
            else
                targetPage--;
        }

        page =
            targetPage;

        radioSection =
            page == 5 ? 1 : 0;

        LGFX_Sprite* previousLcd =
            lcd;

        suppressFlush =
            true;

        lcd =
            &transitionCanvas;

        drawPage(
            gyro,
            wifi,
            settings,
            steeringRadio,
            gainRadio
        );

        lcd =
            previousLcd;

        suppressFlush =
            false;

        const int16_t startOffset = 0;
        const int16_t endOffset =
            direction > 0 ? -UI_CANVAS_WIDTH : UI_CANVAS_WIDTH;

        for(uint8_t frame = 1; frame <= 12; frame++)
        {
            int32_t eased =
                (int32_t)frame *
                (int32_t)frame *
                (3 * 12 - 2 * frame);

            int16_t offset =
                startOffset +
                (
                    (int32_t)(endOffset - startOffset) *
                    eased
                )
                /
                (12 * 12 * 12);

            flushTransitionDisplay(
                offset,
                direction
            );
        }

        memcpy(
            canvas.getBuffer(),
            transitionCanvas.getBuffer(),
            canvas.bufferLength()
        );

        flushDisplay();

        return;
    }
    #endif

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

    radioSection =
        page == 5 ? 1 : 0;

    drawPage(
        gyro,
        wifi,
        settings,
        steeringRadio,
        gainRadio
    );
}


bool UI::canUseRawAmoledBuffers()
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    return
        canvasReady &&
        panelCanvasReady &&
        transitionCanvasReady &&
        canvas.getBuffer() != nullptr &&
        panelCanvas.getBuffer() != nullptr &&
        transitionCanvas.getBuffer() != nullptr &&
        canvas.bufferLength() == (UI_CANVAS_WIDTH * UI_CANVAS_HEIGHT * 2) &&
        transitionCanvas.bufferLength() == (UI_CANVAS_WIDTH * UI_CANVAS_HEIGHT * 2) &&
        panelCanvas.bufferLength() == (UI_CANVAS_HEIGHT * UI_CANVAS_WIDTH * 2);
    #else
    return false;
    #endif
}


bool UI::prepareSwipePreview(
    int8_t direction,
    GyroController& gyro,
    WiFiManager& wifi,
    Settings& settings,
    RadioInput& steeringRadio,
    RadioInput& gainRadio
)
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(!canUseRawAmoledBuffers())
    {
        return false;
    }

    if(
        swipePreviewActive &&
        swipePreviewDirection == direction
    )
    {
        return true;
    }

    swipePreviewSourcePage =
        page;

    swipePreviewSourceRadioSection =
        radioSection;

    uint8_t targetPage =
        page;

    if(direction > 0)
    {
        targetPage++;

        if(targetPage >= totalPages)
            targetPage = 0;
    }
    else
    {
        if(targetPage == 0)
            targetPage = totalPages - 1;
        else
            targetPage--;
    }

    LGFX_Sprite* previousLcd =
        lcd;

    uint8_t previousPage =
        page;

    uint8_t previousRadioSection =
        radioSection;

    suppressFlush =
        true;

    lcd =
        &transitionCanvas;

    page =
        targetPage;

    radioSection =
        page == 5 ? 1 : 0;

    drawPage(
        gyro,
        wifi,
        settings,
        steeringRadio,
        gainRadio
    );

    page =
        previousPage;

    radioSection =
        previousRadioSection;

    lcd =
        previousLcd;

    suppressFlush =
        false;

    swipePreviewActive =
        true;

    swipePreviewDirection =
        direction;

    swipePreviewOffset =
        0;

    return true;
    #else
    return false;
    #endif
}


void UI::finishSwipePreview(
    bool commit
)
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(!swipePreviewActive)
    {
        return;
    }

    int16_t startOffset =
        swipePreviewOffset;

    int16_t endOffset =
        commit
        ?
        (
            swipePreviewDirection > 0
            ?
            -UI_CANVAS_WIDTH
            :
            UI_CANVAS_WIDTH
        )
        :
        0;

    for(uint8_t frame = 1; frame <= 6; frame++)
    {
        int32_t eased =
            (int32_t)frame *
            (int32_t)frame *
            (3 * 6 - 2 * frame);

        int16_t offset =
            startOffset +
            (
                (int32_t)(endOffset - startOffset) *
                eased
            )
            /
            (6 * 6 * 6);

        flushTransitionDisplay(
            offset,
            swipePreviewDirection
        );
    }

    if(commit)
    {
        if(swipePreviewDirection > 0)
        {
            page =
                swipePreviewSourcePage + 1;

            if(page >= totalPages)
                page = 0;
        }
        else
        {
            if(swipePreviewSourcePage == 0)
                page = totalPages - 1;
            else
                page = swipePreviewSourcePage - 1;
        }

        radioSection =
            page == 5 ? 1 : 0;

        memcpy(
            canvas.getBuffer(),
            transitionCanvas.getBuffer(),
            canvas.bufferLength()
        );
    }
    else
    {
        page =
            swipePreviewSourcePage;

        radioSection =
            swipePreviewSourceRadioSection;
    }

    swipePreviewActive =
        false;

    swipePreviewDirection =
        0;

    swipePreviewOffset =
        0;

    flushDisplay();
    #endif
}


void UI::flushDisplay()
{
    flushDisplay(
        0
    );
}


void UI::flushDisplay(
    int16_t xOffset
)
{
    if(
        display == nullptr ||
        lcd == nullptr ||
        !canvasReady ||
        suppressFlush
    )
    {
        return;
    }

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(!panelCanvasReady)
    {
        return;
    }

    if(canUseRawAmoledBuffers())
    {
        uint16_t* source =
            static_cast<uint16_t*>(
                canvas.getBuffer()
            );

        uint16_t* target =
            static_cast<uint16_t*>(
                panelCanvas.getBuffer()
            );

        for(int y = 0; y < UI_CANVAS_HEIGHT; y++)
        {
            for(int x = 0; x < UI_CANVAS_WIDTH; x++)
            {
                int sourceX =
                    x - xOffset;

                uint16_t color =
                    readBackgroundPixelRaw(
                        x,
                        y
                    );

                if(
                    sourceX >= 0 &&
                    sourceX < UI_CANVAS_WIDTH
                )
                {
                    uint16_t pageColor =
                        source[
                            (y * UI_CANVAS_WIDTH) +
                            sourceX
                        ];

                    if(pageColor != 0)
                    {
                        color =
                            pageColor;
                    }
                }

                target[
                    ((UI_CANVAS_WIDTH - 1 - x) * UI_CANVAS_HEIGHT) +
                    y
                ] =
                    color;
            }
        }

        panelCanvas.pushSprite(
            display,
            0,
            0
        );

        return;
    }

    for(
        int y = 0;
        y < UI_CANVAS_HEIGHT;
        y++
    )
    {
        for(
            int x = 0;
            x < UI_CANVAS_WIDTH;
            x++
        )
        {
            int sourceX =
                x - xOffset;

            uint32_t color =
                readBackgroundPixel(
                    x,
                    y
                );

            if(
                sourceX >= 0 &&
                sourceX < UI_CANVAS_WIDTH
            )
            {
                uint32_t pageColor =
                    canvas.readPixel(
                        sourceX,
                        y
                    );

                if(pageColor != TFT_BLACK)
                {
                    color =
                        pageColor;
                }
            }

            panelCanvas.drawPixel(
                y,
                UI_CANVAS_WIDTH - 1 - x,
                color
            );
        }
    }

    panelCanvas.pushSprite(
        display,
        0,
        0
    );

    return;
    #endif

    canvas.pushSprite(
        display,
        0,
        0
    );
}


void UI::flushTransitionDisplay(
    int16_t xOffset,
    int8_t direction
)
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(
        display == nullptr ||
        lcd == nullptr ||
        !canUseRawAmoledBuffers() ||
        suppressFlush
    )
    {
        return;
    }

    uint16_t* current =
        static_cast<uint16_t*>(
            canvas.getBuffer()
        );

    uint16_t* incoming =
        static_cast<uint16_t*>(
            transitionCanvas.getBuffer()
        );

    uint16_t* target =
        static_cast<uint16_t*>(
            panelCanvas.getBuffer()
        );

    int16_t incomingOffset =
        xOffset +
        (
            direction > 0
            ?
            UI_CANVAS_WIDTH
            :
            -UI_CANVAS_WIDTH
        );

    for(int y = 0; y < UI_CANVAS_HEIGHT; y++)
    {
        for(int x = 0; x < UI_CANVAS_WIDTH; x++)
        {
            uint16_t color =
                readBackgroundPixelRaw(
                    x,
                    y
                );

            uint16_t pageColor =
                0;

            int currentX =
                x - xOffset;

            if(
                currentX >= 0 &&
                currentX < UI_CANVAS_WIDTH
            )
            {
                pageColor =
                    current[
                        (y * UI_CANVAS_WIDTH) +
                        currentX
                    ];
            }
            else
            {
                int incomingX =
                    x - incomingOffset;

                if(
                    incomingX >= 0 &&
                    incomingX < UI_CANVAS_WIDTH
                )
                {
                    pageColor =
                        incoming[
                            (y * UI_CANVAS_WIDTH) +
                            incomingX
                        ];
                }
            }

            if(pageColor != 0)
            {
                color =
                    pageColor;
            }

            target[
                ((UI_CANVAS_WIDTH - 1 - x) * UI_CANVAS_HEIGHT) +
                y
            ] =
                color;
        }
    }

    panelCanvas.pushSprite(
        display,
        0,
        0
    );
    #endif
}





void UI::drawMainPage(
    GyroController& gyro,
    Settings& settings
)
{

    drawUiBackground(lcd);

    lcd->setTextColor(TFT_WHITE);

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(
        lcd,
        "Drive",
        OD_CYAN
    );

    lcd->setTextSize(2);

    lcd->setTextColor(
        OD_MUTED
    );

    lcd->drawString(
        "GAIN",
        22,
        62
    );

    lcd->setTextSize(4);

    lcd->setTextColor(
        OD_TEXT
    );

    lcd->drawFloat(
        gyro.getGain(),
        2,
        22,
        82
    );

    lcd->setTextSize(2);

    lcd->setTextColor(
        OD_MUTED
    );

    lcd->drawString(
        "GYRO GAIN",
        24,
        136
    );

    drawAmoledButton(
        lcd,
        276,
        36,
        70,
        82,
        "-",
        OD_CYAN,
        4
    );

    drawAmoledButton(
        lcd,
        364,
        36,
        70,
        82,
        "+",
        OD_CYAN,
        4
    );

    drawAmoledButton(
        lcd,
        276,
        148,
        158,
        54,
        "CAL",
        OD_AMBER,
        2
    );

    drawPageDots();

    return;
    #endif



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
        UI_CENTER_X,
        UI_FOOTER_Y
    );


    drawPageDots();

}







void UI::drawControlPage(
    GyroController& gyro,
    Settings& settings
)
{

    drawUiBackground(lcd);


    lcd->setTextColor(
        TFT_WHITE
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(
        lcd,
        "Gyro",
        OD_MAGENTA
    );

    lcd->setTextSize(2);

    lcd->setTextColor(
        OD_MUTED
    );

    lcd->drawString(
        "DEADBAND",
        22,
        58
    );

    lcd->drawString(
        "REVERSE",
        22,
        120
    );

    lcd->drawString(
        "WOBBLE",
        22,
        182
    );

    lcd->setTextSize(3);

    lcd->setTextColor(
        OD_TEXT
    );

    lcd->drawFloat(
        gyro.getDeadband(),
        1,
        146,
        48
    );

    lcd->drawString(
        settings.getGyroReverse() ? "ON" : "OFF",
        146,
        110
    );

    lcd->drawNumber(
        settings.getGyroAntiWobble(),
        146,
        172
    );

    drawAmoledButton(
        lcd,
        276,
        48,
        70,
        48,
        "-",
        OD_MAGENTA
    );

    drawAmoledButton(
        lcd,
        364,
        48,
        70,
        48,
        "+",
        OD_MAGENTA
    );

    drawAmoledButton(
        lcd,
        276,
        110,
        158,
        48,
        "GYRO REV",
        settings.getGyroReverse() ? OD_GREEN : OD_DIM
    );

    drawAmoledButton(
        lcd,
        276,
        172,
        70,
        48,
        "-",
        OD_MAGENTA
    );

    drawAmoledButton(
        lcd,
        364,
        172,
        70,
        48,
        "+",
        OD_MAGENTA
    );

    drawPageDots();

    return;
    #endif


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



    lcd->setTextSize(2);


    lcd->drawCenterString(
        "Swipe left",
        UI_CENTER_X,
        UI_FOOTER_Y
    );


    drawPageDots();

}







void UI::drawSystemPage(
    Settings& settings
)
{

    drawUiBackground(lcd);


    lcd->setTextColor(
        TFT_WHITE
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(
        lcd,
        "System",
        OD_BLUE
    );

    lcd->setTextSize(2);

    lcd->setTextColor(
        OD_MUTED
    );

    lcd->drawString(
        "OPEN",
        22,
        64
    );

    lcd->drawString(
        "BUILD",
        22,
        116
    );

    lcd->drawString(
        "BLACKBOX",
        22,
        168
    );

    lcd->setTextSize(3);

    lcd->setTextColor(
        OD_TEXT
    );

    lcd->drawString(
        "OpenDrift",
        150,
        56
    );

    lcd->drawString(
        "AMOLED",
        150,
        108
    );

    lcd->drawString(
        settings.getBlackboxEnabled() ? "ON" : "OFF",
        150,
        160
    );

    drawPageDots();

    return;
    #endif


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
        UI_CENTER_X,
        UI_FOOTER_Y
    );


    drawPageDots();

}









void UI::drawTunePage(
    Settings& settings
)
{

    drawUiBackground(lcd);

    lcd->setTextColor(
        TFT_WHITE
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(
        lcd,
        "Tune",
        OD_AMBER
    );

    lcd->setTextSize(2);

    lcd->setTextColor(
        OD_MUTED
    );

    lcd->drawString(
        "MAX CORR",
        22,
        58
    );

    lcd->drawString(
        "SMOOTH",
        22,
        120
    );

    lcd->drawString(
        "I GAIN",
        22,
        182
    );

    lcd->setTextSize(3);

    lcd->setTextColor(
        OD_TEXT
    );

    lcd->drawNumber(
        settings.getGyroMaxCorrection(),
        146,
        48
    );

    lcd->drawFloat(
        settings.getGyroSmoothing(),
        2,
        146,
        110
    );

    lcd->drawFloat(
        settings.getGyroIntegralGain(),
        2,
        146,
        172
    );

    for(
        int row = 0;
        row < 3;
        row++
    )
    {
        int y =
            48 + (row * 62);

        drawAmoledButton(
            lcd,
            276,
            y,
            70,
            48,
            "-",
            OD_AMBER
        );

        drawAmoledButton(
            lcd,
            364,
            y,
            70,
            48,
            "+",
            OD_AMBER
        );
    }

    drawPageDots();

    return;
    #endif

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

    drawUiBackground(lcd);

    lcd->setTextColor(
        TFT_WHITE
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(
        lcd,
        "Response",
        OD_BLUE
    );

    lcd->setTextSize(2);

    lcd->setTextColor(
        OD_MUTED
    );

    lcd->drawString(
        "ATTACK",
        22,
        58
    );

    lcd->drawString(
        "RETURN",
        22,
        120
    );

    lcd->drawString(
        "DAMPER",
        22,
        182
    );

    lcd->setTextSize(3);

    lcd->setTextColor(
        OD_TEXT
    );

    lcd->drawNumber(
        settings.getGyroAttackSpeed(),
        146,
        48
    );

    lcd->drawNumber(
        settings.getGyroReturnSpeed(),
        146,
        110
    );

    lcd->drawNumber(
        settings.getSteeringDamper(),
        146,
        172
    );

    for(
        int row = 0;
        row < 3;
        row++
    )
    {
        int y =
            48 + (row * 62);

        drawAmoledButton(
            lcd,
            276,
            y,
            70,
            48,
            "-",
            OD_BLUE
        );

        drawAmoledButton(
            lcd,
            364,
            y,
            70,
            48,
            "+",
            OD_BLUE
        );
    }

    drawPageDots();

    return;
    #endif

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

    drawUiBackground(lcd);


    lcd->setTextColor(
        TFT_WHITE
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(
        lcd,
        "WiFi",
        wifi.isEnabled() ? OD_GREEN : OD_RED
    );

    lcd->setTextSize(2);

    lcd->setTextColor(
        OD_MUTED
    );

    lcd->drawString(
        "STATUS",
        22,
        64
    );

    lcd->drawString(
        "CLIENTS",
        22,
        116
    );

    lcd->drawString(
        "IP",
        22,
        168
    );

    lcd->setTextSize(3);

    lcd->setTextColor(
        wifi.isEnabled() ? OD_GREEN : OD_RED
    );

    lcd->drawString(
        wifi.isEnabled() ? "ON" : "OFF",
        150,
        56
    );

    lcd->setTextColor(
        OD_TEXT
    );

    lcd->drawNumber(
        wifi.isEnabled() ? WiFi.softAPgetStationNum() : 0,
        150,
        108
    );

    lcd->setTextSize(2);

    lcd->setTextColor(
        OD_TEXT
    );

    lcd->drawString(
        wifi.isEnabled() ? WiFi.softAPIP().toString() : "--",
        150,
        170
    );

    drawAmoledButton(
        lcd,
        296,
        70,
        130,
        92,
        wifi.isEnabled() ? "WIFI OFF" : "WIFI ON",
        wifi.isEnabled() ? OD_RED : OD_GREEN,
        2
    );

    drawPageDots();

    return;
    #endif



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
        UI_CENTER_X,
        UI_FOOTER_Y
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

    drawUiBackground(lcd);

    lcd->setTextColor(
        TFT_WHITE
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    {
    drawAmoledHeader(
        lcd,
        radioSection == 0 ? "Radio" : "Steering",
        radioSection == 0 ? OD_CYAN : OD_AMBER
    );

    lcd->setTextSize(2);

    if(radioSection == 1)
    {
        lcd->setTextColor(
            OD_MUTED
        );

        lcd->drawString(
            "OUT",
            22,
            62
        );

        lcd->drawString(
            "RAW",
            22,
            116
        );

        lcd->drawString(
            "TRAVEL",
            22,
            170
        );

        lcd->setTextSize(3);

        lcd->setTextColor(
            OD_TEXT
        );

        lcd->drawNumber(
            mapSteeringForDisplay(
                steeringRadio.getPulseWidth(),
                settings
            ),
            120,
            54
        );

        lcd->drawNumber(
            steeringRadio.getPulseWidth(),
            120,
            108
        );

        lcd->drawNumber(
            settings.getRadioSteeringTravel(),
            120,
            162
        );

        lcd->setTextSize(2);

        drawAmoledButton(
            lcd,
            202,
            158,
            34,
            40,
            "-",
            OD_AMBER
        );

        drawAmoledButton(
            lcd,
            242,
            158,
            34,
            40,
            "+",
            OD_AMBER
        );

        drawAmoledButton(
            lcd,
            286,
            34,
            136,
            44,
            "MAX LEFT",
            OD_AMBER
        );

        drawAmoledButton(
            lcd,
            286,
            90,
            136,
            44,
            "CENTER",
            OD_AMBER
        );

        drawAmoledButton(
            lcd,
            286,
            146,
            136,
            44,
            "MAX RIGHT",
            OD_AMBER
        );

        drawAmoledButton(
            lcd,
            286,
            202,
            62,
            36,
            "REV",
            settings.getServoReverse() ? OD_GREEN : OD_DIM
        );

        lcd->setTextColor(
            settings.getServoReverse() ? OD_GREEN : OD_MUTED
        );

        lcd->drawString(
            settings.getServoReverse() ? "ON" : "OFF",
            360,
            210
        );

        drawPageDots();

        return;
    }

    lcd->setTextColor(
        OD_MUTED
    );

    lcd->drawString(
        "STEER",
        22,
        58
    );

    lcd->drawString(
        "GAIN IN",
        22,
        132
    );

    lcd->drawString(
        "GYRO",
        294,
        58
    );

    lcd->setTextSize(3);

    lcd->setTextColor(
        OD_TEXT
    );

    lcd->drawNumber(
        steeringRadio.getPulseWidth(),
        120,
        50
    );

    lcd->drawNumber(
        gainRadio.getPulseWidth(),
        120,
        124
    );

    lcd->drawFloat(
        gyro.getGain(),
        2,
        354,
        50
    );

    lcd->setTextSize(2);

    lcd->setTextColor(
        steeringRadio.hasSignal() ? OD_GREEN : OD_RED
    );

    lcd->drawString(
        steeringRadio.hasSignal() ? "OK" : "NO",
        224,
        58
    );

    lcd->setTextColor(
        gainRadio.hasSignal() ? OD_GREEN : OD_RED
    );

    lcd->drawString(
        gainRadio.hasSignal() ? "OK" : "NO",
        224,
        132
    );

    int steeringBarX = 28;
    int steeringBarY = 96;
    int steeringBarW = 224;
    int steeringBarH = 12;

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
        OD_DIM
    );

    lcd->drawFastVLine(
        steeringCenterPos,
        steeringBarY - 4,
        steeringBarH + 8,
        OD_CYAN
    );

    lcd->fillRect(
        steeringPos - 3,
        steeringBarY - 5,
        7,
        steeringBarH + 10,
        steeringRadio.hasSignal() ? TFT_GREEN : TFT_RED
    );

    int gainBarX = 28;
    int gainBarY = 170;
    int gainBarW = 224;
    int gainBarH = 12;

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
        OD_DIM
    );

    lcd->fillRect(
        gainPos - 3,
        gainBarY - 5,
        7,
        gainBarH + 10,
        gainRadio.hasSignal() ? TFT_GREEN : TFT_RED
    );

    lcd->setTextSize(1);

    lcd->setTextColor(
        OD_MUTED
    );

    lcd->drawString(
        "STEER CAL",
        294,
        116
    );

    lcd->drawNumber(
        settings.getSteeringMin(),
        294,
        136
    );

    lcd->drawNumber(
        settings.getSteeringCenter(),
        348,
        136
    );

    lcd->drawNumber(
        settings.getSteeringMax(),
        402,
        136
    );

    lcd->drawString(
        "GAIN CAL",
        294,
        166
    );

    lcd->drawNumber(
        settings.getGainMin(),
        294,
        186
    );

    lcd->drawNumber(
        settings.getGainMax(),
        360,
        186
    );

    drawPageDots();

    return;
    }
    #endif

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
            UI_CENTER_X,
            UI_FOOTER_Y
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

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    {
    if(radioSection == 1)
    {
        lcd->setTextSize(3);

        lcd->fillRect(
            120,
            54,
            130,
            36,
            TFT_BLACK
        );

        lcd->drawNumber(
            mapSteeringForDisplay(
                steeringRadio.getPulseWidth(),
                settings
            ),
            120,
            54
        );

        lcd->fillRect(
            120,
            108,
            130,
            36,
            TFT_BLACK
        );

        lcd->drawNumber(
            steeringRadio.getPulseWidth(),
            120,
            108
        );

        lcd->fillRect(
            120,
            162,
            80,
            36,
            TFT_BLACK
        );

        lcd->drawNumber(
            settings.getRadioSteeringTravel(),
            120,
            162
        );

        lcd->setTextSize(2);

        lcd->fillRect(
            358,
            208,
            58,
            22,
            TFT_BLACK
        );

        lcd->drawString(
            settings.getServoReverse() ? "ON" : "OFF",
            360,
            210
        );

        lcd->setTextColor(
            TFT_WHITE
        );

        flushDisplay();

        return;
    }

    lcd->setTextSize(3);

    lcd->fillRect(
        120,
        50,
        128,
        34,
        TFT_BLACK
    );

    lcd->drawNumber(
        steeringRadio.getPulseWidth(),
        120,
        50
    );

    lcd->setTextSize(2);

    lcd->fillRect(
        224,
        58,
        36,
        20,
        TFT_BLACK
    );

    lcd->drawString(
        steeringRadio.hasSignal() ? "OK" : "NO",
        224,
        58
    );

    int steeringBarX = 28;
    int steeringBarY = 96;
    int steeringBarW = 224;
    int steeringBarH = 12;

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
        steeringBarX - 4,
        steeringBarY - 6,
        steeringBarW + 8,
        steeringBarH + 12,
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
        steeringBarY - 4,
        steeringBarH + 8,
        TFT_WHITE
    );

    lcd->fillRect(
        steeringPos - 3,
        steeringBarY - 5,
        7,
        steeringBarH + 10,
        steeringRadio.hasSignal() ? TFT_GREEN : TFT_RED
    );

    lcd->setTextSize(3);

    lcd->fillRect(
        120,
        124,
        128,
        34,
        TFT_BLACK
    );

    lcd->drawNumber(
        gainRadio.getPulseWidth(),
        120,
        124
    );

    lcd->setTextSize(2);

    lcd->fillRect(
        224,
        132,
        36,
        20,
        TFT_BLACK
    );

    lcd->drawString(
        gainRadio.hasSignal() ? "OK" : "NO",
        224,
        132
    );

    int gainBarX = 28;
    int gainBarY = 170;
    int gainBarW = 224;
    int gainBarH = 12;

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
        gainBarX - 4,
        gainBarY - 6,
        gainBarW + 8,
        gainBarH + 12,
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
        gainPos - 3,
        gainBarY - 5,
        7,
        gainBarH + 10,
        gainRadio.hasSignal() ? TFT_GREEN : TFT_RED
    );

    lcd->setTextSize(3);

    lcd->fillRect(
        354,
        50,
        86,
        36,
        TFT_BLACK
    );

    lcd->drawFloat(
        gyro.getGain(),
        2,
        354,
        50
    );

    lcd->setTextColor(
        TFT_WHITE
    );

    flushDisplay();

    return;
    }
    #endif

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

        flushDisplay();

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

    flushDisplay();

}





void UI::drawPageDots()
{
    int spacing = 20;

    int startX =
        (UI_CANVAS_WIDTH / 2) -
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
                UI_DOTS_Y,
                5,
                #if defined(OPENDRIFT_BOARD_AMOLED_164)
                OD_CYAN
                #else
                TFT_WHITE
                #endif
            );
        }
        else
        {
            lcd->drawCircle(
                startX + (i*spacing),
                UI_DOTS_Y,
                5,
                #if defined(OPENDRIFT_BOARD_AMOLED_164)
                OD_DIM
                #else
                TFT_WHITE
                #endif
            );
        }

    }

    flushDisplay();

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
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(page == 0)
    {
        if(buttonPressed(x, y, 276, 36, 70, 82))
            return 1;

        if(buttonPressed(x, y, 364, 36, 70, 82))
            return 2;
    }

    if(page == 1)
    {
        if(buttonPressed(x, y, 276, 48, 70, 48))
            return 3;

        if(buttonPressed(x, y, 364, 48, 70, 48))
            return 4;

        if(buttonPressed(x, y, 276, 172, 70, 48))
            return 21;

        if(buttonPressed(x, y, 364, 172, 70, 48))
            return 22;
    }

    if(page == 2)
    {
        if(buttonPressed(x, y, 276, 48, 70, 48))
            return 5;

        if(buttonPressed(x, y, 364, 48, 70, 48))
            return 6;

        if(buttonPressed(x, y, 276, 110, 70, 48))
            return 7;

        if(buttonPressed(x, y, 364, 110, 70, 48))
            return 8;

        if(buttonPressed(x, y, 276, 172, 70, 48))
            return 15;

        if(buttonPressed(x, y, 364, 172, 70, 48))
            return 16;
    }

    if(page == 3)
    {
        if(buttonPressed(x, y, 276, 48, 70, 48))
            return 9;

        if(buttonPressed(x, y, 364, 48, 70, 48))
            return 10;

        if(buttonPressed(x, y, 276, 110, 70, 48))
            return 11;

        if(buttonPressed(x, y, 364, 110, 70, 48))
            return 12;

        if(buttonPressed(x, y, 276, 172, 70, 48))
            return 13;

        if(buttonPressed(x, y, 364, 172, 70, 48))
            return 14;
    }

    return 0;
    #endif

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
        page == 1 &&
        (
            button == 21 ||
            button == 22
        )
    )
    {
        drawControlPage(
            gyro,
            settings
        );
    }
    else if(
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

    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
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
    #endif

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

        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        swipePreviewActive = false;

        swipePreviewDirection = 0;

        swipePreviewOffset = 0;

        lastSwipePreviewAt = 0;
        #endif

    }



    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(
        touched &&
        trackingSwipe
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

        if(
            abs(delta) > 12 &&
            abs(delta) > abs(deltaY) + 6
        )
        {
            int8_t direction =
                delta < 0
                ?
                1
                :
                -1;

            if(prepareSwipePreview(
                direction,
                gyro,
                wifi,
                settings,
                steeringRadio,
                gainRadio
            ))
            {
                int16_t offset =
                    constrain(
                        delta,
                        -UI_CANVAS_WIDTH,
                        UI_CANVAS_WIDTH
                    );

                if(
                    millis() - lastSwipePreviewAt > 12 ||
                    abs(offset - swipePreviewOffset) > 10
                )
                {
                    swipePreviewOffset =
                        offset;

                    flushTransitionDisplay(
                        swipePreviewOffset,
                        swipePreviewDirection
                    );

                    lastSwipePreviewAt =
                        millis();
                }

                lastTouchState =
                    touched;

                return;
            }
        }
    }
    #endif





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
            #if defined(OPENDRIFT_BOARD_AMOLED_164)
            if(swipePreviewActive)
            {
                bool commit =
                    abs(delta) > 72 &&
                    abs(delta) > abs(deltaY);

                finishSwipePreview(
                    commit
                );
            }
            else
            #endif
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

        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        int8_t repeatButton =
            repeatButtonAt(
                x,
                y
            );

        if(repeatButton != 0)
        {
            applyRepeatButton(
                repeatButton,
                gyro,
                settings
            );

            lastTouchState =
                touched;

            return;
        }

        if(
            page == 0 &&
            buttonPressed(
                x,
                y,
                276,
                148,
                158,
                54
            )
        )
        {
            imu.update();

            gyro.calibrate(
                imu.getYawRate()
            );

            drawMainPage(
                gyro,
                settings
            );

            lastTouchState =
                touched;

            return;
        }

        if(
            page == 1 &&
            buttonPressed(
                x,
                y,
                276,
                110,
                158,
                48
            )
        )
        {
            settings.setGyroReverse(
                !settings.getGyroReverse()
            );

            drawControlPage(
                gyro,
                settings
            );

            lastTouchState =
                touched;

            return;
        }

        if(page == 5)
        {
            if(
                steeringRadio.hasSignal() &&
                buttonPressed(
                    x,
                    y,
                    286,
                    34,
                    136,
                    44
                )
            )
            {
                settings.setSteeringMin(
                    steeringRadio.getPulseWidth()
                );

                drawRadioPage(
                    steeringRadio,
                    gainRadio,
                    settings,
                    gyro
                );

                lastTouchState =
                    touched;

                return;
            }

            if(
                steeringRadio.hasSignal() &&
                buttonPressed(
                    x,
                    y,
                    286,
                    90,
                    136,
                    44
                )
            )
            {
                settings.setSteeringCenter(
                    steeringRadio.getPulseWidth()
                );

                drawRadioPage(
                    steeringRadio,
                    gainRadio,
                    settings,
                    gyro
                );

                lastTouchState =
                    touched;

                return;
            }

            if(
                steeringRadio.hasSignal() &&
                buttonPressed(
                    x,
                    y,
                    286,
                    146,
                    136,
                    44
                )
            )
            {
                settings.setSteeringMax(
                    steeringRadio.getPulseWidth()
                );

                drawRadioPage(
                    steeringRadio,
                    gainRadio,
                    settings,
                    gyro
                );

                lastTouchState =
                    touched;

                return;
            }

            if(buttonPressed(x, y, 202, 158, 34, 40))
            {
                settings.setRadioSteeringTravel(
                    settings.getRadioSteeringTravel() - 1
                );

                drawRadioPage(
                    steeringRadio,
                    gainRadio,
                    settings,
                    gyro
                );

                lastTouchState =
                    touched;

                return;
            }

            if(buttonPressed(x, y, 242, 158, 34, 40))
            {
                settings.setRadioSteeringTravel(
                    settings.getRadioSteeringTravel() + 1
                );

                drawRadioPage(
                    steeringRadio,
                    gainRadio,
                    settings,
                    gyro
                );

                lastTouchState =
                    touched;

                return;
            }

            if(buttonPressed(x, y, 286, 202, 62, 36))
            {
                settings.setServoReverse(
                    !settings.getServoReverse()
                );

                drawRadioPage(
                    steeringRadio,
                    gainRadio,
                    settings,
                    gyro
                );

                lastTouchState =
                    touched;

                return;
            }
        }

        if(
            page == 6 &&
            buttonPressed(
                x,
                y,
                296,
                70,
                130,
                92
            )
        )
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

            lastTouchState =
                touched;

            return;
        }

        lastTouchState =
            touched;

        return;
        #endif




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
