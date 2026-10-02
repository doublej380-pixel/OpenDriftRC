#include "UI.h"

#if defined(OPENDRIFT_BOARD_AMOLED_164)
#include <esp_heap_caps.h>
#endif

static constexpr uint16_t ROUND_CYAN = 0x07FF;
static constexpr uint16_t ROUND_DIM = 0x3186;

static constexpr uint8_t PAGE_DRIVE = 0;
static constexpr uint8_t PAGE_CORE = 1;
static constexpr uint8_t PAGE_RESPONSE = 2;
static constexpr uint8_t PAGE_DRIFT_ASSIST = 3;
static constexpr uint8_t PAGE_EXPERIMENTAL = 4;
static constexpr uint8_t PAGE_PROFILES = 5;
static constexpr uint8_t PAGE_RADIO = 6;
static constexpr uint8_t PAGE_STEERING = 7;
static constexpr uint8_t PAGE_STEERING_CAL = 8;
static constexpr uint8_t PAGE_WIFI = 9;
static constexpr uint8_t PAGE_SYSTEM = 10;
static constexpr uint8_t PAGE_BLACKBOX = 11;
static constexpr uint8_t PAGE_BACKGROUNDS = 12;


static uint8_t radioSectionForPage(
    uint8_t page
)
{
    if(page == PAGE_STEERING_CAL)
    {
        return 2;
    }

    return page == PAGE_STEERING ? 1 : 0;
}


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
static constexpr uint16_t OD_AMBER = 0xFD20;
static constexpr uint16_t OD_GREEN = 0x07E0;
static constexpr uint16_t OD_RED = 0xF800;

// Runtime AMOLED palette. Defaults preserve OpenDrift's original theme.
// Theme/panel concept contributed by J3vb and adapted to the current UI.
static uint16_t OD_TEXT = 0xFFFF;
static uint16_t OD_MUTED = 0x9CF3;
static uint16_t OD_DIM = 0x3186;
static uint16_t OD_CYAN = 0x07FF;
static uint16_t OD_BLUE = 0x3D9F;
static uint16_t OD_MAGENTA = 0xF81F;
static uint16_t OD_WARM = 0xFD20;
static bool themeDarkText = false;

// Page sprites use black as transparency. This otherwise-unused near-black
// is replaced by a background-aware translucent panel during composition.
static constexpr uint16_t OD_PANEL = 0x0020;
static constexpr uint16_t OD_PANEL_RAW = 0x2000;

struct AmoledAccentPreset
{
    uint16_t primary;
    uint16_t secondary;
    uint16_t tertiary;
    uint16_t warm;
};

static const AmoledAccentPreset ACCENT_PRESETS[Settings::THEME_ACCENT_COUNT] =
{
    {0x07FF, 0x3D9F, 0xF81F, 0xFD20},
    {0x07FF, 0x07FF, 0x07FF, 0x07FF},
    {0x3D9F, 0x3D9F, 0x3D9F, 0x3D9F},
    {0xF81F, 0xF81F, 0xF81F, 0xF81F},
    {0xFD20, 0xFD20, 0xFD20, 0xFD20},
    {0x07E0, 0x07E0, 0x07E0, 0x07E0},
    {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
};

static void applyAmoledTheme(uint8_t textMode, uint8_t accent)
{
    themeDarkText = textMode == 1;

    if(themeDarkText)
    {
        // Pure black is reserved as the page transparency key.
        OD_TEXT = 0x0841;
        OD_MUTED = 0x4228;
        OD_DIM = 0xAD75;
    }
    else
    {
        OD_TEXT = 0xFFFF;
        OD_MUTED = 0x9CF3;
        OD_DIM = 0x3186;
    }

    const AmoledAccentPreset& preset =
        ACCENT_PRESETS[accent < Settings::THEME_ACCENT_COUNT ? accent : 0];

    OD_CYAN = preset.primary;
    OD_BLUE = preset.secondary;
    OD_MAGENTA = preset.tertiary;
    OD_WARM = preset.warm;
}

static inline uint16_t swapColorBytes(uint16_t value)
{
    return static_cast<uint16_t>((value << 8) | (value >> 8));
}

static inline uint16_t blendPanel(uint16_t background)
{
    uint16_t backgroundPart =
        ((background >> 2) & 0x39E7) +
        ((background >> 3) & 0x18E3);

    return themeDarkText
        ? static_cast<uint16_t>(backgroundPart + 0x9CF3)
        : backgroundPart;
}

static inline uint16_t blendPanelRaw(uint16_t backgroundRaw)
{
    return swapColorBytes(blendPanel(swapColorBytes(backgroundRaw)));
}

static void drawAmoledRowPanel(LGFX_Sprite* lcd, int y, int h)
{
    lcd->fillRoundRect(18, y, 420, h, 6, OD_PANEL);
}


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


static const uint16_t* activeBackgroundPixels = nullptr;


static uint16_t readBackgroundPixel(
    int x,
    int y
)
{
    if(activeBackgroundPixels != nullptr)
    {
        return activeBackgroundPixels[(y * OD_BACKGROUND_WIDTH) + x];
    }

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
    lcd->fillRect(
        x + 1,
        y + 1,
        w - 2,
        h - 2,
        OD_PANEL
    );

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
#include "../../assets/backgrounds/background.c"

static constexpr int ROUND_BACKGROUND_SOURCE_WIDTH = 456;
static constexpr int ROUND_BACKGROUND_SOURCE_HEIGHT = 280;
static LGFX_Sprite roundBackground;
static bool roundBackgroundReady = false;

static LGFX_Sprite roundFrame;
static bool roundFrameReady = false;


static uint16_t readRoundBackgroundPixel(
    int x,
    int y
)
{
    const uint16_t* pixels =
        reinterpret_cast<const uint16_t*>(background_map);

    int sourceX =
        (x * ROUND_BACKGROUND_SOURCE_WIDTH) / 240;

    int sourceY =
        (y * ROUND_BACKGROUND_SOURCE_HEIGHT) / 240;

    uint16_t color =
        pgm_read_word(
            pixels +
            (sourceY * ROUND_BACKGROUND_SOURCE_WIDTH) +
            sourceX
        );

    return color;
}


static bool initializeRoundBackground()
{
    if(
        roundBackgroundReady &&
        roundFrameReady
    )
    {
        return true;
    }

    roundBackground.setPsram(
        psramFound()
    );

    roundBackground.setColorDepth(16);

    roundFrame.setPsram(
        psramFound()
    );

    roundFrame.setColorDepth(16);

    roundBackgroundReady =
        roundBackground.createSprite(240, 240) != nullptr;

    roundFrameReady =
        roundFrame.createSprite(240, 240) != nullptr;

    if(
        !roundBackgroundReady ||
        !roundFrameReady
    )
    {
        return false;
    }

    for(int y = 0; y < 240; y++)
    {
        for(int x = 0; x < 240; x++)
        {
            roundBackground.drawPixel(
                x,
                y,
                readRoundBackgroundPixel(x, y)
            );
        }
    }

    return true;
}


static void drawRoundAdjustRow(
    LGFX_Sprite* lcd,
    const char* label,
    const String& value,
    int row,
    uint16_t accent
)
{
    int labelY = 47 + (row * 55);
    int buttonY = 61 + (row * 55);

    lcd->setTextSize(1);
    lcd->setTextColor(0xBDF7);
    lcd->drawCenterString(label, 120, labelY);

    lcd->drawRect(26, buttonY, 44, 30, accent);
    lcd->drawRect(170, buttonY, 44, 30, accent);

    lcd->setTextSize(2);
    lcd->setTextColor(TFT_WHITE);
    lcd->drawCenterString("-", 48, buttonY + 7);
    lcd->drawCenterString("+", 192, buttonY + 7);
    lcd->drawCenterString(value.c_str(), 120, buttonY + 7);
}


static void drawRoundCompactAdjustRow(
    LGFX_Sprite* lcd,
    const char* label,
    const String& value,
    int row,
    uint16_t accent
)
{
    int labelY = 39 + (row * 41);
    int buttonY = 50 + (row * 41);

    lcd->setTextSize(1);
    lcd->setTextColor(0xBDF7);
    lcd->drawCenterString(label, 120, labelY);

    lcd->drawRect(26, buttonY, 44, 28, accent);
    lcd->drawRect(170, buttonY, 44, 28, accent);

    lcd->setTextSize(2);
    lcd->setTextColor(TFT_WHITE);
    lcd->drawCenterString("-", 48, buttonY + 6);
    lcd->drawCenterString("+", 192, buttonY + 6);
    lcd->drawCenterString(value.c_str(), 120, buttonY + 6);
}


static bool prepareRoundFrame()
{
    if(
        !roundBackgroundReady ||
        !roundFrameReady ||
        roundBackground.getBuffer() == nullptr ||
        roundFrame.getBuffer() == nullptr ||
        roundBackground.bufferLength() != roundFrame.bufferLength()
    )
    {
        return false;
    }

    memcpy(
        roundFrame.getBuffer(),
        roundBackground.getBuffer(),
        roundFrame.bufferLength()
    );

    return true;
}


static void overlayRoundPage(
    LGFX_Sprite& sourceSprite,
    int16_t xOffset
)
{
    uint16_t* source =
        static_cast<uint16_t*>(
            sourceSprite.getBuffer()
        );

    uint16_t* target =
        static_cast<uint16_t*>(
            roundFrame.getBuffer()
        );

    if(source == nullptr || target == nullptr)
    {
        return;
    }

    for(int y = 0; y < 240; y++)
    {
        for(int x = 0; x < 240; x++)
        {
            int sourceX =
                x - xOffset;

            if(sourceX < 0 || sourceX >= 240)
            {
                continue;
            }

            uint16_t color =
                source[(y * 240) + sourceX];

            if(color != 0)
            {
                target[(y * 240) + x] =
                    color;
            }
        }
    }
}


static void drawUiBackground(
    LGFX_Sprite* lcd
)
{
    // Round pages are transparent overlays. The full-color background is
    // pushed directly to the physical display by the compositor.
    lcd->fillScreen(TFT_BLACK);
}
#endif


static int mapSteeringForDisplay(
    int pulse,
    Settings& settings
)
{
    if(settings.isSteeringCalibrated())
    {
        int left = settings.getSteeringCapturedInputPulse(0);
        int center = settings.getSteeringCapturedInputPulse(1);
        int right = settings.getSteeringCapturedInputPulse(2);
        int leftDelta = left - center;
        int rightDelta = right - center;

        if(
            abs(leftDelta) >= 10 &&
            abs(rightDelta) >= 10 &&
            leftDelta * rightDelta < 0
        )
        {
            bool towardLeft = left < center
                ? pulse <= center
                : pulse >= center;

            int mappedPulse = towardLeft
                ? map(constrain(pulse, min(left, center), max(left, center)), left, center, 1000, 1500)
                : map(constrain(pulse, min(center, right), max(center, right)), center, right, 1500, 2000);

            int offset = mappedPulse - 1500;
            offset = (offset * settings.getRadioSteeringTravel()) / 100;
            return constrain(1500 + offset, 1000, 2000);
        }
    }

    #if defined(OPENDRIFT_INPUT_CRSF)
    int mappedPulse = map(
        constrain(pulse, 988, 2012),
        988,
        2012,
        1000,
        2000
    );
    #else
    int mappedPulse = constrain(pulse, 1000, 2000);
    #endif

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


static inline uint16_t uiTextColor()
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    return OD_TEXT;
    #else
    return TFT_WHITE;
    #endif
}



void UI::begin(
    LGFX* display,
    GyroController& gyro,
    WiFiManager& wifi,
    Settings& settings,
    RadioInput& steeringRadio,
    RadioInput& gainRadio,
    ServoOutput& steeringServo
)
{
    this->display = display;
    steeringServoOutput = &steeringServo;

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
    canvas.setPsram(
        psramFound()
    );

    transitionCanvas.setPsram(
        psramFound()
    );

    canvas.setColorDepth(
        16
    );

    transitionCanvas.setColorDepth(
        16
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

    transitionCanvasReady =
        transitionCanvas.createSprite(
            UI_CANVAS_WIDTH,
            UI_CANVAS_HEIGHT
        )
        !=
        nullptr;

    initializeRoundBackground();
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

    transitionCanvas.setTextWrap(
        false
    );

    lcd =
        &canvas;
    #endif

    display->fillScreen(
        TFT_BLACK
    );

    page = 0;

    steeringCalibrationError = false;

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    syncTheme(settings);
    applyBackground(settings);
    #endif

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
        case PAGE_DRIVE:
            drawMainPage(
                gyro,
                settings
            );
            break;

        case PAGE_CORE:
            drawCorePage(
                gyro,
                settings
            );
            break;

        case PAGE_RESPONSE:
            drawResponsePage(
                settings
            );
            break;

        case PAGE_DRIFT_ASSIST:
            drawDriftAssistPage(
                settings
            );
            break;

        case PAGE_EXPERIMENTAL:
            drawExperimentalPage(
                settings
            );
            break;

        case PAGE_PROFILES:
            drawProfilesPage(
                settings
            );
            break;

        case PAGE_RADIO:
            radioSection = 0;
            drawRadioPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );
            break;

        case PAGE_STEERING:
            radioSection = 1;
            drawRadioPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );
            break;

        case PAGE_STEERING_CAL:
            radioSection = 2;
            drawSteeringCalibrationPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );
            break;

        case PAGE_WIFI:
            drawWifiPage(
                wifi,
                settings
            );
            break;

        case PAGE_SYSTEM:
            drawSystemPage(
                settings
            );
            break;

        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        case PAGE_BLACKBOX:
            drawBlackboxPage(settings);
            break;

        case PAGE_BACKGROUNDS:
            drawBackgroundsPage(
                settings
            );
            break;
        #endif
    }
}


void UI::setThrottleRadio(
    RadioInput& throttleRadio
)
{
    throttleRadioInput = &throttleRadio;
}


void UI::requestRefresh()
{
    refreshRequested = true;
}


void UI::setCalibrationCallback(
    void (*callback)()
)
{
    calibrationCallback = callback;
}


#if defined(OPENDRIFT_USB_MAINTENANCE)
void UI::setUsbMaintenanceCallback(
    void (*callback)()
)
{
    usbMaintenanceCallback = callback;
}
#endif



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
        uint8_t sourcePage =
            page;

        uint8_t sourceRadioSection =
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

        page =
            targetPage;

        radioSection =
            radioSectionForPage(page);

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

        page =
            sourcePage;

        radioSection =
            sourceRadioSection;

        lcd =
            previousLcd;

        suppressFlush =
            false;

        const int16_t startOffset = 0;
        const int16_t endOffset =
            direction > 0 ? -UI_CANVAS_WIDTH : UI_CANVAS_WIDTH;

        prepareTransitionPixels();
        animateTransition(startOffset, endOffset, direction, 220);
        reportTransitionTiming();

        page =
            targetPage;

        radioSection =
            radioSectionForPage(page);

        memcpy(
            canvas.getBuffer(),
            transitionCanvas.getBuffer(),
            canvas.bufferLength()
        );

        flushDisplay();

        return;
    }
    #endif

    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    if(
        canvasReady &&
        transitionCanvasReady &&
        roundBackgroundReady &&
        roundFrameReady
    )
    {
        uint8_t sourcePage =
            page;

        uint8_t sourceRadioSection =
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

        suppressFlush =
            true;

        lcd =
            &transitionCanvas;

        page =
            targetPage;

        radioSection =
            radioSectionForPage(page);

        drawPage(
            gyro,
            wifi,
            settings,
            steeringRadio,
            gainRadio
        );

        page =
            sourcePage;

        radioSection =
            sourceRadioSection;

        lcd =
            previousLcd;

        suppressFlush =
            false;

        int16_t endOffset =
            direction > 0
            ? -UI_CANVAS_WIDTH
            : UI_CANVAS_WIDTH;

        for(uint8_t frame = 1; frame <= 12; frame++)
        {
            int32_t eased =
                (int32_t)frame *
                (int32_t)frame *
                (3 * 12 - 2 * frame);

            int16_t offset =
                ((int32_t)endOffset * eased) /
                (12 * 12 * 12);

            flushTransitionDisplay(
                offset,
                direction
            );
        }

        page =
            targetPage;

        radioSection =
            radioSectionForPage(page);

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
        radioSectionForPage(page);

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
        radioSectionForPage(page);

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

    prepareTransitionPixels();

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

    animateTransition(startOffset, endOffset, swipePreviewDirection,
        constrain(abs(endOffset - startOffset) * 220 / UI_CANVAS_WIDTH, 70, 220));
    reportTransitionTiming();

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
            radioSectionForPage(page);

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


#if defined(OPENDRIFT_BOARD_AMOLED_164)
void UI::drawBlackboxProgress(
    uint8_t progress,
    BlackboxArchive::Status status
)
{
    if(!canvasReady || blackboxArchive == nullptr) return;

    const bool saving = status == BlackboxArchive::SAVING;
    const bool saved = status == BlackboxArchive::SAVED;
    const uint16_t accent = saving ? OD_CYAN : (saved ? OD_GREEN : OD_RED);

    canvas.fillRoundRect(48, 64, 360, 152, 10, 0x0841);
    canvas.drawRoundRect(48, 64, 360, 152, 10, accent);
    canvas.setTextDatum(top_center);
    canvas.setTextColor(accent);
    canvas.setTextSize(2);
    canvas.drawString(
        saving ? "DUMPING BLACKBOX" : (saved ? "DUMP COMPLETE" : "DUMP FAILED"),
        UI_CENTER_X,
        83
    );

    canvas.setTextColor(OD_TEXT);
    canvas.setTextSize(1);
    canvas.drawString(
        saving
            ? blackboxArchive->getStorageName()
            : (saved ? "Safe to restart or remove power" : "Log was not saved"),
        UI_CENTER_X,
        116
    );

    canvas.drawRoundRect(78, 143, 300, 28, 6, OD_DIM);
    canvas.fillRoundRect(82, 147, 292, 20, 4, 0x1082);

    const uint16_t fillWidth =
        (uint16_t)((constrain(progress, 0, 100) * 292UL) / 100UL);

    if(fillWidth > 0)
    {
        canvas.fillRoundRect(82, 147, fillWidth, 20, 4, accent);
    }

    char percent[8];
    snprintf(percent, sizeof(percent), "%u%%", (unsigned int)progress);
    canvas.setTextColor(OD_TEXT);
    canvas.setTextSize(1);
    canvas.drawString(percent, UI_CENTER_X, 181);
    canvas.setTextDatum(top_left);
    flushDisplay();
}
#endif


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

                    if(pageColor == OD_PANEL_RAW)
                    {
                        color = blendPanelRaw(color);
                    }
                    else if(pageColor != 0)
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

        drawFixedPageDots();

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

                if(pageColor == OD_PANEL)
                {
                    color = blendPanel(static_cast<uint16_t>(color));
                }
                else if(pageColor != TFT_BLACK)
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

    drawFixedPageDots();

    panelCanvas.pushSprite(
        display,
        0,
        0
    );

    return;
    #endif

    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    if(!prepareRoundFrame())
    {
        return;
    }

    overlayRoundPage(
        canvas,
        xOffset
    );

    drawFixedPageDots();

    roundFrame.pushSprite(
        display,
        0,
        0
    );
    #endif
}


#if defined(OPENDRIFT_BOARD_AMOLED_164)
void UI::prepareTransitionPixels()
{
    transitionPixelsReady = false;
    transitionFrames = transitionComposeUs = transitionTransferUs = 0;
    const size_t pixels = UI_CANVAS_WIDTH * UI_CANVAS_HEIGHT;
    if(transitionCurrentPixels == nullptr)
    {
        // Three physical-layout buffers: two transparent page overlays and
        // the stationary background. No allocation happens per animation frame.
        transitionCurrentPixels = static_cast<uint16_t*>(heap_caps_malloc(
            pixels * sizeof(uint16_t) * 3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if(transitionCurrentPixels != nullptr)
        {
            transitionIncomingPixels = transitionCurrentPixels + pixels;
            transitionBackgroundPixels = transitionIncomingPixels + pixels;
        }
    }
    if(transitionCurrentPixels != nullptr && canUseRawAmoledBuffers())
    {
        const auto* current = static_cast<const uint16_t*>(canvas.getBuffer());
        const auto* incoming = static_cast<const uint16_t*>(transitionCanvas.getBuffer());
        for(int x = 0; x < UI_CANVAS_WIDTH; ++x)
        {
            const size_t row = (UI_CANVAS_WIDTH - 1 - x) * UI_CANVAS_HEIGHT;
            for(int y = 0; y < UI_CANVAS_HEIGHT; ++y)
            {
                const size_t source = y * UI_CANVAS_WIDTH + x;
                transitionCurrentPixels[row + y] = current[source];
                transitionIncomingPixels[row + y] = incoming[source];
                transitionBackgroundPixels[row + y] = readBackgroundPixelRaw(x, y);
            }
        }
        transitionPixelsReady = true;
    }
    transitionStartedUs = micros();
}

void UI::animateTransition(int16_t start, int16_t end, int8_t direction, uint32_t durationMs)
{
    const uint32_t started = micros();
    const uint32_t duration = durationMs * 1000UL;
    for(;;)
    {
        const uint32_t frameStarted = micros();
        const uint32_t elapsed = frameStarted - started;
        const float t = min(1.0f, elapsed / static_cast<float>(duration));
        const float eased = t * t * (3.0f - 2.0f * t);
        flushTransitionDisplay(start + static_cast<int16_t>((end - start) * eased), direction);
        if(elapsed >= duration) break;
        // Pace short frames, but never add another full interval to a slow
        // transfer. The independent control task continues throughout.
        while(micros() - frameStarted < 16667UL) delay(1);
    }
}

void UI::reportTransitionTiming()
{
    if(transitionFrames == 0) return;
    const uint32_t elapsed = micros() - transitionStartedUs;
    Serial.printf("UI swipe: %lu frames, %.1f fps, compose %.2f ms, transfer %.2f ms, cache=%s\n",
        (unsigned long)transitionFrames,
        elapsed ? transitionFrames * 1000000.0f / elapsed : 0.0f,
        transitionComposeUs / (1000.0f * transitionFrames),
        transitionTransferUs / (1000.0f * transitionFrames),
        transitionPixelsReady ? "ON" : "OFF");
}
#endif

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

    const uint32_t composeStarted = micros();
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

    if(transitionPixelsReady)
    {
        // Horizontal UI movement becomes contiguous physical scanline copies.
        // Merge sequentially in PSRAM rather than writing rotated pixels with
        // a large stride. Background/panel tint remain fixed during the swipe.
        for(int x = 0; x < UI_CANVAS_WIDTH; ++x)
        {
            const int currentX = x - xOffset;
            const int incomingX = x - incomingOffset;
            const uint16_t* overlay = nullptr;
            if(currentX >= 0 && currentX < UI_CANVAS_WIDTH)
                overlay = transitionCurrentPixels + (UI_CANVAS_WIDTH - 1 - currentX) * UI_CANVAS_HEIGHT;
            else if(incomingX >= 0 && incomingX < UI_CANVAS_WIDTH)
                overlay = transitionIncomingPixels + (UI_CANVAS_WIDTH - 1 - incomingX) * UI_CANVAS_HEIGHT;
            const size_t row = (UI_CANVAS_WIDTH - 1 - x) * UI_CANVAS_HEIGHT;
            for(int y = 0; y < UI_CANVAS_HEIGHT; ++y)
            {
                const uint16_t pageColor = overlay ? overlay[y] : 0;
                target[row + y] = pageColor == OD_PANEL_RAW
                    ? blendPanelRaw(transitionBackgroundPixels[row + y])
                    : (pageColor ? pageColor : transitionBackgroundPixels[row + y]);
            }
        }
    }
    else for(int y = 0; y < UI_CANVAS_HEIGHT; y++)
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

            if(pageColor == OD_PANEL_RAW)
            {
                color = blendPanelRaw(color);
            }
            else if(pageColor != 0)
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

    drawFixedPageDots();

    const uint32_t transferStarted = micros();
    transitionComposeUs += transferStarted - composeStarted;
    panelCanvas.pushSprite(
        display,
        0,
        0
    );
    transitionTransferUs += micros() - transferStarted;
    ++transitionFrames;
    #else
    if(
        display == nullptr ||
        !canvasReady ||
        !transitionCanvasReady ||
        !roundBackgroundReady ||
        !roundFrameReady ||
        suppressFlush
    )
    {
        return;
    }

    int16_t incomingOffset =
        xOffset +
        (
            direction > 0
            ? UI_CANVAS_WIDTH
            : -UI_CANVAS_WIDTH
        );

    if(!prepareRoundFrame())
    {
        return;
    }

    overlayRoundPage(
        canvas,
        xOffset
    );

    overlayRoundPage(
        transitionCanvas,
        incomingOffset
    );

    drawFixedPageDots();

    roundFrame.pushSprite(
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
    lastDrawnGainHundredths =
        (int16_t)(gyro.getGain() * 100.0f + 0.5f);

    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    drawUiBackground(lcd);

    lcd->setTextSize(3);
    lcd->setTextColor(ROUND_CYAN);
    lcd->drawCenterString("Drive", 120, 16);

    lcd->setTextSize(1);
    lcd->setTextColor(0xBDF7);
    lcd->drawCenterString("GYRO GAIN", 120, 57);
    lcd->setTextSize(3);
    lcd->setTextColor(TFT_WHITE);
    lcd->drawFloat(gyro.getGain(), 2, 89, 72);

    lcd->drawRect(30, 109, 58, 38, ROUND_CYAN);
    lcd->drawRect(152, 109, 58, 38, ROUND_CYAN);
    lcd->setTextSize(2);
    lcd->drawCenterString("-", 59, 119);
    lcd->drawCenterString("+", 181, 119);

    lcd->drawRect(55, 164, 130, 38, TFT_YELLOW);
    lcd->drawCenterString("CALIBRATE", 120, 174);

    drawPageDots();
    return;
    #endif

    drawUiBackground(lcd);

    lcd->setTextColor(TFT_WHITE);

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(
        lcd,
        "Drive",
        OD_CYAN
    );

    lcd->fillRoundRect(18, 52, 240, 106, 6, OD_PANEL);

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
    lcd->setTextColor(ROUND_CYAN);

    lcd->drawCenterString(
        "OpenDrift",
        120,
        20
    );

    lcd->setTextColor(TFT_WHITE);



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


#if defined(OPENDRIFT_USB_MAINTENANCE)
void UI::showFirmwareUpdateCompleted()
{
    if(lcd == nullptr) return;

    firmwareUpdateNoticeVisible = true;
    firmwareUpdateNoticeUntil = millis() + 4500;

    lcd->fillRoundRect(70, 70, 316, 136, 10, 0x10A2);
    lcd->drawRoundRect(70, 70, 316, 136, 10, TFT_GREEN);
    lcd->drawRoundRect(72, 72, 312, 132, 8, TFT_GREEN);
    lcd->setTextDatum(middle_center);
    lcd->setTextColor(TFT_GREEN);
    lcd->setTextSize(3);
    lcd->drawString(
        "UPDATE COMPLETE",
        228,
        110
    );
    lcd->setTextColor(TFT_WHITE);
    lcd->setTextSize(2);
    lcd->drawString(
        "New firmware is running",
        228,
        158
    );
    lcd->setTextDatum(top_left);
    flushDisplay();
}
#endif







void UI::drawCorePage(
    GyroController& gyro,
    Settings& settings
)
{
    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    drawUiBackground(lcd);

    lcd->setTextSize(3);
    lcd->setTextColor(TFT_MAGENTA);
    lcd->drawCenterString("Core", 120, 14);

    drawRoundAdjustRow(
        lcd,
        "DEADBAND",
        String(gyro.getDeadband(), 1),
        0,
        TFT_MAGENTA
    );

    drawRoundAdjustRow(
        lcd,
        "MAX CORR %",
        String(settings.getGyroMaxCorrection()),
        1,
        TFT_MAGENTA
    );

    lcd->setTextSize(1);
    lcd->setTextColor(0xBDF7);
    lcd->drawCenterString("GYRO REVERSE", 120, 157);
    lcd->drawRect(
        43,
        171,
        154,
        34,
        settings.getGyroReverse() ? TFT_GREEN : TFT_MAGENTA
    );
    lcd->setTextSize(2);
    lcd->setTextColor(TFT_WHITE);
    lcd->drawCenterString(
        settings.getGyroReverse() ? "ON" : "OFF",
        120,
        180
    );

    drawPageDots();
    return;
    #endif

    drawUiBackground(lcd);


    lcd->setTextColor(
        uiTextColor()
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(
        lcd,
        "Core",
        OD_MAGENTA
    );

    drawAmoledRowPanel(lcd, 48, 48);
    drawAmoledRowPanel(lcd, 110, 48);
    drawAmoledRowPanel(lcd, 172, 48);

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
        "MAX CORR %",
        22,
        120
    );

    lcd->drawString(
        "REVERSE",
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

    lcd->drawNumber(
        settings.getGyroMaxCorrection(),
        146,
        110
    );

    lcd->drawString(
        settings.getGyroReverse() ? "ON" : "OFF",
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
        70,
        48,
        "-",
        OD_MAGENTA
    );

    drawAmoledButton(
        lcd,
        364,
        110,
        70,
        48,
        "+",
        OD_MAGENTA
    );

    drawAmoledButton(
        lcd,
        276,
        172,
        158,
        48,
        "GYRO REV",
        settings.getGyroReverse() ? OD_GREEN : OD_DIM
    );

    drawPageDots();

    return;
    #endif


    lcd->setTextSize(3);
    lcd->setTextColor(TFT_MAGENTA);


    lcd->drawCenterString(
        "Gyro",
        120,
        20
    );

    lcd->setTextColor(TFT_WHITE);



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

    drawAmoledRowPanel(lcd, 48, 46);
    drawAmoledRowPanel(lcd, 102, 46);
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    drawAmoledRowPanel(lcd, 156, 38);
    #endif
    drawAmoledRowPanel(lcd, 198, 46);

    lcd->setTextSize(2);

    lcd->setTextColor(
        OD_MUTED
    );

    lcd->drawString(
        "RATE (REBOOT)",
        22,
        64
    );

    lcd->drawString(
        "THEME",
        22,
        116
    );

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    lcd->drawString("USB MAINT", 22, 168);
    #endif

    lcd->drawString(
        #if defined(OPENDRIFT_INPUT_CRSF)
        "CRSF",
        #else
        #if defined(OPENDRIFT_AMOLED_V2)
        "GPIO 2",
        #else
        "GPIO 18",
        #endif
        #endif
        22,
        212
    );

    lcd->setTextSize(3);

    lcd->setTextColor(
        OD_TEXT
    );

    drawAmoledButton(
        lcd,
        150,
        54,
        240,
        38,
        settings.getControlLoopHz() == 333 ? "333 HZ" : "250 HZ",
        settings.getControlLoopHz() == 333 ? OD_AMBER : OD_CYAN,
        2
    );

    drawAmoledButton(
        lcd,
        150,
        106,
        116,
        38,
        Settings::themeAccentName(settings.getThemeAccent()),
        OD_CYAN,
        2
    );

    drawAmoledButton(
        lcd,
        274,
        106,
        116,
        38,
        settings.getThemeText() == 1 ? "DARK" : "LIGHT",
        OD_MUTED,
        2
    );

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    drawAmoledButton(
        lcd,
        150,
        156,
        240,
        38,
        usbMaintenanceHoldStartedAt != 0 ? "KEEP HOLD" : "USB MODE",
        OD_AMBER,
        2
    );
    #endif

    drawAmoledButton(
        lcd,
        150,
        202,
        240,
        38,
        #if defined(OPENDRIFT_INPUT_CRSF)
        #if defined(OPENDRIFT_CRSF_V2_THROTTLE_GPIO8)
        "RX1 TX2 / ESC8",
        #elif defined(OPENDRIFT_AMOLED_V2)
        "RX1 / TX2",
        #else
        "RX17 / TX18",
        #endif
        OD_CYAN,
        #else
        settings.getThrottleOutputEnabled()
        ? "THROTTLE OUT"
        : "GAIN INPUT",
        settings.getThrottleOutputEnabled()
        ? OD_AMBER
        : OD_CYAN,
        #endif
        2
    );

    drawPageDots();

    return;
    #endif


    lcd->setTextSize(3);
    lcd->setTextColor(ROUND_CYAN);
    lcd->drawCenterString(
        "System",
        120,
        18
    );

    lcd->setTextSize(2);
    lcd->setTextColor(TFT_WHITE);
    lcd->drawCenterString(
        #if defined(OPENDRIFT_INPUT_CRSF)
        "OpenDrift CRSF BETA",
        #else
        "OpenDrift OPEN BETA",
        #endif
        120,
        57
    );

    lcd->setTextSize(1);
    lcd->setTextColor(0xBDF7);
    lcd->drawString(
        "BLACKBOX",
        38,
        94
    );

    lcd->setTextColor(
        settings.getBlackboxEnabled()
        ? TFT_GREEN
        : TFT_RED
    );
    lcd->drawString(
        settings.getBlackboxEnabled() ? "ON" : "OFF",
        156,
        94
    );

    lcd->setTextColor(0xBDF7);
    lcd->drawCenterString(
        #if defined(OPENDRIFT_INPUT_CRSF)
        "CRSF UART",
        #else
        #if defined(OPENDRIFT_AMOLED_V2)
        "GPIO 2",
        #else
        "GPIO 18",
        #endif
        #endif
        120,
        119
    );

    lcd->drawRect(
        43,
        137,
        154,
        40,
        ROUND_CYAN
    );

    lcd->setTextSize(2);
    lcd->setTextColor(TFT_WHITE);
    lcd->drawCenterString(
        #if defined(OPENDRIFT_INPUT_CRSF)
        #if defined(OPENDRIFT_CRSF_V2_THROTTLE_GPIO8)
        "1RX 2TX / ESC8",
        #elif defined(OPENDRIFT_AMOLED_V2)
        "1 RX / 2 TX",
        #else
        "17 RX / 18 TX",
        #endif
        #else
        settings.getThrottleOutputEnabled()
        ? "THROTTLE OUT"
        : "GAIN INPUT",
        #endif
        120,
        148
    );

    lcd->setTextSize(1);
    lcd->setTextColor(0xBDF7);
    lcd->drawCenterString(
        #if defined(OPENDRIFT_INPUT_CRSF)
        "gain on CRSF channel 3",
        #else
        "15 steer / 16 throttle / 17 servo",
        #endif
        120,
        184
    );


    drawPageDots();

}


#if defined(OPENDRIFT_BOARD_AMOLED_164)
void UI::drawBlackboxPage(Settings& settings)
{
    drawUiBackground(lcd);
    drawAmoledHeader(lcd, "Blackbox", OD_GREEN);

    drawAmoledRowPanel(lcd, 48, 44);
    drawAmoledRowPanel(lcd, 98, 56);
    drawAmoledRowPanel(lcd, 160, 42);

    lcd->setTextDatum(top_left);
    lcd->setTextSize(2);
    lcd->setTextColor(OD_MUTED);
    lcd->drawString("RECORDER", 22, 61);

    drawAmoledButton(
        lcd,
        300,
        51,
        128,
        38,
        settings.getBlackboxEnabled() ? "ENABLED" : "DISABLED",
        settings.getBlackboxEnabled() ? OD_GREEN : OD_RED,
        1
    );

    size_t ramRecords = 0;
    size_t ramBytes = 0;
    size_t ramCapacity = 0;
    uint32_t ramDuration = 0;
    size_t savedRecords = 0;
    size_t savedBytes = 0;
    uint32_t savedDuration = 0;
    const char* target = "UNAVAILABLE";

    if(blackboxArchive != nullptr)
    {
        ramRecords = blackboxArchive->getRamRecordCount();
        ramBytes = blackboxArchive->getRamBytes();
        ramCapacity = blackboxArchive->getRamCapacityBytes();
        ramDuration = blackboxArchive->getRamDurationMs();
        savedRecords = blackboxArchive->getArchiveRecordCount();
        savedBytes = blackboxArchive->getArchiveBytes();
        savedDuration = blackboxArchive->getArchiveDurationMs();
        target = blackboxArchive->getStorageName();
    }

    char ramStats[96];
    snprintf(
        ramStats,
        sizeof(ramStats),
        "%u records  |  %u / %u KB  |  %02lu:%02lu",
        (unsigned int)ramRecords,
        (unsigned int)(ramBytes / 1024U),
        (unsigned int)(ramCapacity / 1024U),
        (unsigned long)(ramDuration / 60000UL),
        (unsigned long)((ramDuration / 1000UL) % 60UL)
    );

    lcd->setTextColor(OD_CYAN);
    lcd->setTextSize(1);
    lcd->drawString("RAM CAPTURE", 22, 108);
    lcd->setTextColor(OD_TEXT);
    lcd->drawString(ramStats, 22, 130);

    char savedStats[112];
    snprintf(
        savedStats,
        sizeof(savedStats),
        "%s  |  %u records  |  %u KB  |  %02lu:%02lu",
        target,
        (unsigned int)savedRecords,
        (unsigned int)(savedBytes / 1024U),
        (unsigned long)(savedDuration / 60000UL),
        (unsigned long)((savedDuration / 1000UL) % 60UL)
    );

    lcd->setTextColor(OD_MUTED);
    lcd->setTextSize(1);
    lcd->drawString("SAVED: ", 22, 174);
    lcd->setTextColor(OD_TEXT);
    lcd->drawString(savedStats, 78, 174);

    const char* dumpLabel =
        strcmp(target, "SD card") == 0
        ? "DUMP TO SD CARD"
        : "DUMP TO FLASH";

    drawAmoledButton(lcd, 22, 210, 270, 38, dumpLabel, OD_GREEN, 2);
    drawAmoledButton(lcd, 300, 210, 128, 38, "CLEAR RAM", OD_AMBER, 1);

    drawPageDots();
}
#endif









void UI::drawResponsePage(
    Settings& settings
)
{
    drawUiBackground(lcd);

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(lcd, "Response", OD_WARM);

    drawAmoledRowPanel(lcd, 48, 48);
    drawAmoledRowPanel(lcd, 110, 48);
    drawAmoledRowPanel(lcd, 172, 48);

    lcd->setTextSize(2);
    lcd->setTextColor(OD_MUTED);
    lcd->drawString("SMOOTH", 22, 58);
    lcd->drawString("PREDICT", 22, 120);
    lcd->drawString("SERVO QUIET", 22, 182);

    lcd->setTextSize(3);
    lcd->setTextColor(OD_TEXT);
    lcd->drawFloat(settings.getGyroSmoothing(), 2, 146, 48);
    lcd->drawNumber(settings.getPredictionStrength(), 146, 110);
    lcd->drawNumber(settings.getServoQuiet(), 146, 172);

    for(int row = 0; row < 3; row++)
    {
        int y = 48 + (row * 62);
        drawAmoledButton(lcd, 276, y, 70, 48, "-", OD_WARM);
        drawAmoledButton(lcd, 364, y, 70, 48, "+", OD_WARM);
    }
    #else
    lcd->setTextSize(3);
    lcd->setTextColor(TFT_YELLOW);
    lcd->drawCenterString("Response", 120, 14);

    drawRoundAdjustRow(
        lcd,
        "SMOOTHING",
        String(settings.getGyroSmoothing(), 2),
        0,
        TFT_YELLOW
    );
    drawRoundAdjustRow(
        lcd,
        "PREDICTION",
        String(settings.getPredictionStrength()),
        1,
        TFT_YELLOW
    );
    drawRoundAdjustRow(
        lcd,
        "SERVO QUIET",
        String(settings.getServoQuiet()),
        2,
        TFT_YELLOW
    );
    #endif

    drawPageDots();
}


void UI::drawDriftAssistPage(
    Settings& settings
)
{
    drawUiBackground(lcd);

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(lcd, "Assistance", OD_BLUE);

    drawAmoledRowPanel(lcd, 48, 48);
    drawAmoledRowPanel(lcd, 110, 48);
    drawAmoledRowPanel(lcd, 172, 48);

    lcd->setTextSize(2);
    lcd->setTextColor(OD_MUTED);
    lcd->drawString("COUNTERSTEER", 22, 58);
    lcd->drawString("HOLD ASSIST", 22, 120);
    lcd->drawString("DRIFT MEM", 22, 182);

    lcd->setTextSize(3);
    lcd->setTextColor(OD_TEXT);
    lcd->drawNumber(settings.getGyroCounterSteerAssist(), 146, 48);
    lcd->drawNumber(settings.getGyroHoldBoost(), 146, 110);
    lcd->drawFloat(settings.getGyroIntegralGain(), 2, 146, 172);

    for(int row = 0; row < 3; row++)
    {
        int y = 48 + (row * 62);
        drawAmoledButton(lcd, 276, y, 70, 48, "-", OD_BLUE);
        drawAmoledButton(lcd, 364, y, 70, 48, "+", OD_BLUE);
    }
    #else
    lcd->setTextSize(3);
    lcd->setTextColor(ROUND_CYAN);
    lcd->drawCenterString("Assistance", 120, 14);

    drawRoundAdjustRow(
        lcd,
        "COUNTERSTEER",
        String(settings.getGyroCounterSteerAssist()),
        0,
        ROUND_CYAN
    );
    drawRoundAdjustRow(
        lcd,
        "HOLD ASSIST",
        String(settings.getGyroHoldBoost()),
        1,
        ROUND_CYAN
    );
    drawRoundAdjustRow(
        lcd,
        "DRIFT MEMORY",
        String(settings.getGyroIntegralGain(), 2),
        2,
        ROUND_CYAN
    );
    #endif

    drawPageDots();
}


bool UI::isProfilesPage()
{
    return page == PAGE_PROFILES;
}


#if defined(OPENDRIFT_BOARD_AMOLED_164)
void UI::setBackgroundStore(Backgrounds& store)
{
    backgroundStore = &store;
}


void UI::setBlackboxArchive(BlackboxArchive& archive)
{
    blackboxArchive = &archive;
}


bool UI::isBackgroundsPage()
{
    return page == PAGE_BACKGROUNDS;
}


bool UI::applyBackground(Settings& settings)
{
    const char* name = settings.getBackgroundName();
    const uint32_t revision =
        backgroundStore != nullptr ? backgroundStore->getRevision() : 0;

    if(
        backgroundApplied &&
        revision == appliedBackgroundRevision &&
        strcmp(name, appliedBackgroundName) == 0
    )
    {
        return false;
    }

    backgroundApplied = true;
    appliedBackgroundRevision = revision;
    snprintf(
        appliedBackgroundName,
        sizeof(appliedBackgroundName),
        "%s",
        name
    );

    // The built-in image is always selected first. A missing, corrupt, or
    // allocation-failed custom image can therefore never blank the display.
    activeBackgroundPixels = nullptr;

    if(
        name[0] != 0 &&
        backgroundStore != nullptr &&
        backgroundStore->isReady()
    )
    {
        if(backgroundPixels == nullptr)
        {
            backgroundPixels = static_cast<uint16_t*>(
                heap_caps_malloc(
                    Backgrounds::PIXEL_BYTES,
                    MALLOC_CAP_SPIRAM
                )
            );
        }

        if(
            backgroundPixels != nullptr &&
            backgroundStore->load(name, backgroundPixels)
        )
        {
            activeBackgroundPixels = backgroundPixels;
        }
        else
        {
            // Do not keep a dangling selection after deletion/corruption.
            settings.setBackgroundName("");
            appliedBackgroundName[0] = 0;
        }
    }

    return true;
}


void UI::drawBackgroundsPage(Settings& settings)
{
    drawUiBackground(lcd);
    drawAmoledHeader(lcd, "Backgrounds", OD_MAGENTA);

    static constexpr uint8_t VISIBLE_ROWS = 4;
    static constexpr int ROW_START = 46;
    static constexpr int ROW_HEIGHT = 47;

    const bool storeReady =
        backgroundStore != nullptr && backgroundStore->isReady();
    const uint8_t storedCount =
        storeReady ? backgroundStore->getCount() : 0;
    const uint8_t rowCount = storedCount + 1;
    const uint8_t maxScroll =
        rowCount > VISIBLE_ROWS ? rowCount - VISIBLE_ROWS : 0;

    backgroundScroll = min(backgroundScroll, maxScroll);
    const char* activeName = settings.getBackgroundName();

    for(uint8_t slot = 0; slot < VISIBLE_ROWS; slot++)
    {
        const uint8_t index = backgroundScroll + slot;
        if(index >= rowCount) break;

        const char* name =
            index == 0 ? "BUILT-IN" : backgroundStore->getName(index - 1);
        const bool active =
            index == 0 ? activeName[0] == 0 : strcmp(name, activeName) == 0;
        const int y = ROW_START + slot * ROW_HEIGHT;

        lcd->fillRoundRect(18, y, 420, 41, 6, OD_PANEL);
        lcd->drawRoundRect(18, y, 420, 41, 6, active ? OD_GREEN : OD_DIM);
        lcd->setTextSize(2);
        lcd->setTextColor(active ? OD_GREEN : OD_TEXT);
        lcd->drawString(name, 30, y + 10);

        if(active)
        {
            lcd->setTextSize(1);
            lcd->setTextColor(OD_MUTED);
            lcd->drawRightString("ACTIVE", 426, y + 14);
        }
    }

    if(storedCount == 0)
    {
        lcd->setTextSize(1);
        lcd->setTextColor(OD_MUTED);
        lcd->drawCenterString(
            storeReady
                ? "Upload backgrounds in the web configurator"
                : "Background storage unavailable",
            UI_CENTER_X,
            222
        );
    }

    if(rowCount > VISIBLE_ROWS)
    {
        const int trackHeight = 182;
        const int thumbHeight = max(24, trackHeight * VISIBLE_ROWS / rowCount);
        const int thumbY =
            47 + (trackHeight - thumbHeight) * backgroundScroll /
                max(1, (int)maxScroll);
        lcd->drawFastVLine(446, 47, trackHeight, OD_DIM);
        lcd->fillRect(443, thumbY, 7, thumbHeight, OD_MAGENTA);
    }

    drawPageDots();
}
#endif


void UI::drawExperimentalPage(
    Settings& settings
)
{
    drawUiBackground(lcd);

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(
        lcd,
        "Transition",
        OD_MAGENTA
    );

    drawAmoledRowPanel(lcd, 48, 48);
    drawAmoledRowPanel(lcd, 110, 48);

    lcd->setTextSize(2);
    lcd->setTextColor(OD_MUTED);
    lcd->drawString("TRANS SPEED", 22, 58);
    lcd->drawString("DRIVER PRIO", 22, 120);

    lcd->setTextSize(3);
    lcd->setTextColor(OD_TEXT);
    lcd->drawNumber(settings.getGyroTransitionSpeed(), 146, 48);
    lcd->drawNumber(settings.getDriverPriority(), 146, 110);

    drawAmoledButton(lcd, 276, 48, 70, 48, "-", OD_MAGENTA);
    drawAmoledButton(lcd, 364, 48, 70, 48, "+", OD_MAGENTA);
    drawAmoledButton(lcd, 276, 110, 70, 48, "-", OD_MAGENTA);
    drawAmoledButton(lcd, 364, 110, 70, 48, "+", OD_MAGENTA);

    lcd->setTextSize(1);
    lcd->setTextColor(OD_MUTED);
    lcd->drawString("PRIORITY: 0 FULL GYRO / START 10-20", 22, 190);
    lcd->drawString("TRANS: LOWER SMOOTH / HIGHER FAST", 22, 216);
    #else
    lcd->setTextSize(3);
    lcd->setTextColor(TFT_MAGENTA);
    lcd->drawCenterString("Transition", 120, 14);

    drawRoundAdjustRow(
        lcd,
        "TRANS SPEED",
        String(settings.getGyroTransitionSpeed()),
        0,
        TFT_MAGENTA
    );

    drawRoundAdjustRow(
        lcd,
        "ANTI WOBBLE",
        String(settings.getGyroHuntStrength()),
        1,
        TFT_MAGENTA
    );

    drawRoundAdjustRow(
        lcd,
        "RATE (REBOOT)",
        settings.getControlLoopHz() == 333 ? "333 HZ" : "250 HZ",
        2,
        TFT_MAGENTA
    );
    #endif

    drawPageDots();
}


void UI::drawProfilesPage(
    Settings& settings
)
{
    drawUiBackground(lcd);

    const uint8_t visibleProfiles = 4;

    uint8_t profileCount =
        settings.getProfileCount();

    uint8_t maxScroll =
        profileCount > visibleProfiles
        ?
        profileCount - visibleProfiles
        :
        0;

    profileScroll = min(
        profileScroll,
        maxScroll
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    drawAmoledHeader(
        lcd,
        "Profiles",
        OD_CYAN
    );

    if(profileCount == 0)
    {
        lcd->setTextSize(2);
        lcd->setTextColor(OD_TEXT);
        lcd->drawCenterString(
            "No driving profiles yet",
            UI_CENTER_X,
            104
        );

        lcd->setTextColor(OD_MUTED);
        lcd->drawCenterString(
            "Create profiles in the web configurator",
            UI_CENTER_X,
            142
        );
    }
    else
    {
        const int rowStart = 46;
        const int rowHeight = 47;

        for(uint8_t slot = 0; slot < visibleProfiles; slot++)
        {
            uint8_t index =
                profileScroll + slot;

            if(index >= profileCount)
            {
                break;
            }

            const Settings::DrivingProfile* profile =
                settings.getProfile(index);

            if(profile == nullptr)
            {
                continue;
            }

            bool active =
                settings.getActiveProfileIndex() == index;

            int y =
                rowStart + (slot * rowHeight);

            uint16_t accent =
                active ? OD_GREEN : OD_DIM;

            lcd->fillRoundRect(18, y, 420, 41, 6, OD_PANEL);

            lcd->drawRoundRect(
                18,
                y,
                420,
                41,
                6,
                accent
            );

            lcd->setTextSize(2);
            lcd->setTextColor(
                active ? OD_GREEN : OD_TEXT
            );
            lcd->drawString(
                profile->name,
                30,
                y + 10
            );

            lcd->setTextSize(1);
            lcd->setTextColor(OD_MUTED);

            String summary =
                "G " + String(settings.getProfileValue(*profile, OpenDriftParameters::Id::GYRO_GAIN), 2) +
                "   PRED " + String((int)settings.getProfileValue(*profile, OpenDriftParameters::Id::PREDICTION)) +
                "   HOLD " + String((int)settings.getProfileValue(*profile, OpenDriftParameters::Id::HOLD_ASSIST));

            lcd->drawRightString(
                summary.c_str(),
                426,
                y + 14
            );
        }

        if(profileCount > visibleProfiles)
        {
            int trackHeight = 182;
            int thumbHeight = max(
                24,
                (trackHeight * visibleProfiles) / profileCount
            );

            int thumbY =
                47 +
                (
                    (trackHeight - thumbHeight) *
                    profileScroll
                ) /
                max(1, (int)maxScroll);

            lcd->drawFastVLine(446, 47, trackHeight, OD_DIM);
            lcd->fillRect(443, thumbY, 7, thumbHeight, OD_CYAN);
        }
    }
    #else
    lcd->setTextSize(3);
    lcd->setTextColor(ROUND_CYAN);
    lcd->drawCenterString("Profiles", 120, 14);

    if(profileCount == 0)
    {
        lcd->setTextSize(2);
        lcd->setTextColor(TFT_WHITE);
        lcd->drawCenterString("NO PROFILES", 120, 94);

        lcd->setTextSize(1);
        lcd->setTextColor(0xBDF7);
        lcd->drawCenterString("create one in web config", 120, 128);
    }
    else
    {
        const int rowStart = 43;
        const int rowHeight = 40;

        for(uint8_t slot = 0; slot < visibleProfiles; slot++)
        {
            uint8_t index =
                profileScroll + slot;

            if(index >= profileCount)
            {
                break;
            }

            const Settings::DrivingProfile* profile =
                settings.getProfile(index);

            if(profile == nullptr)
            {
                continue;
            }

            bool active =
                settings.getActiveProfileIndex() == index;

            int y =
                rowStart + (slot * rowHeight);

            uint16_t accent =
                active ? TFT_GREEN : ROUND_DIM;

            lcd->drawRoundRect(
                24,
                y,
                192,
                35,
                5,
                accent
            );

            lcd->setTextSize(1);
            lcd->setTextColor(
                active ? TFT_GREEN : TFT_WHITE
            );
            lcd->drawString(
                profile->name,
                34,
                y + 6
            );

            lcd->setTextColor(0xBDF7);

            String summary =
                "G" + String(settings.getProfileValue(*profile, OpenDriftParameters::Id::GYRO_GAIN), 2) +
                " P" + String((int)settings.getProfileValue(*profile, OpenDriftParameters::Id::PREDICTION));

            lcd->drawRightString(
                summary.c_str(),
                205,
                y + 19
            );
        }

        if(profileCount > visibleProfiles)
        {
            int trackHeight = 154;
            int thumbHeight = max(
                20,
                (trackHeight * visibleProfiles) / profileCount
            );

            int thumbY =
                44 +
                (
                    (trackHeight - thumbHeight) *
                    profileScroll
                ) /
                max(1, (int)maxScroll);

            lcd->drawFastVLine(222, 44, trackHeight, ROUND_DIM);
            lcd->fillRect(220, thumbY, 5, thumbHeight, ROUND_CYAN);
        }
    }
    #endif

    drawPageDots();
}



void UI::drawWifiPage(
    WiFiManager& wifi,
    Settings& settings
)
{
    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    drawUiBackground(lcd);

    lcd->setTextSize(3);
    lcd->setTextColor(
        wifi.isEnabled() ? TFT_GREEN : TFT_RED
    );
    lcd->drawCenterString("WiFi", 120, 16);

    lcd->setTextSize(1);
    lcd->setTextColor(0xBDF7);
    lcd->drawString("STATUS", 36, 62);
    lcd->setTextSize(2);
    lcd->setTextColor(TFT_WHITE);
    lcd->drawString(
        wifi.isEnabled() ? "ONLINE" : "OFFLINE",
        112,
        56
    );

    lcd->setTextSize(1);
    lcd->setTextColor(0xBDF7);
    lcd->drawString("ADDRESS", 36, 104);
    lcd->setTextColor(TFT_WHITE);
    lcd->drawString(
        wifi.isEnabled()
        ? WiFi.softAPIP().toString().c_str()
        : "--",
        112,
        104
    );

    lcd->drawRect(
        50,
        145,
        140,
        42,
        wifi.isEnabled() ? TFT_RED : TFT_GREEN
    );
    lcd->setTextSize(2);
    lcd->drawCenterString(
        wifi.isEnabled() ? "WIFI OFF" : "WIFI ON",
        120,
        156
    );

    drawPageDots();
    return;
    #endif

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

    lcd->fillRoundRect(18, 48, 264, 164, 6, OD_PANEL);

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


    lcd->setTextColor(
        wifi.isEnabled() ? TFT_GREEN : TFT_RED
    );

    lcd->drawCenterString(
        "WiFi",
        120,
        20
    );

    lcd->setTextColor(TFT_WHITE);



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
void UI::drawRoundRadioPage(
    RadioInput& steeringRadio,
    RadioInput& gainRadio,
    Settings& settings,
    GyroController& gyro
)
{
    RadioInput& throttleRadio =
        throttleRadioInput != nullptr
        ? *throttleRadioInput
        : gainRadio;

    drawUiBackground(lcd);

    lcd->setTextColor(TFT_WHITE);
    lcd->setTextSize(3);
    lcd->setTextColor(
        radioSection == 2 ? TFT_YELLOW : ROUND_CYAN
    );

    const char* title =
        radioSection == 0
        ? "Radio"
        : (radioSection == 1 ? "Steering" : "Endpoints");

    lcd->drawCenterString(title, 120, 16);
    lcd->setTextColor(TFT_WHITE);

    if(radioSection == 0)
    {
        lcd->setTextSize(2);
        lcd->drawString("STR", 24, 58);
        lcd->drawNumber(steeringRadio.getPulseWidth(), 86, 58);
        lcd->setTextColor(
            steeringRadio.hasSignal() ? TFT_GREEN : TFT_RED
        );
        lcd->drawString(
            steeringRadio.hasSignal() ? "OK" : "NO",
            178,
            58
        );

        lcd->setTextColor(TFT_WHITE);
        lcd->drawRect(28, 82, 184, 8, ROUND_DIM);

        int steeringPos =
            map(
                constrain(steeringRadio.getPulseWidth(), 1000, 2000),
                1000,
                2000,
                30,
                210
            );

        lcd->fillRect(
            steeringPos - 2,
            78,
            5,
            16,
            steeringRadio.hasSignal() ? TFT_GREEN : TFT_RED
        );

        lcd->setTextSize(2);
        lcd->setTextColor(TFT_WHITE);
        lcd->drawString("THR", 24, 106);
        lcd->drawNumber(throttleRadio.getPulseWidth(), 96, 106);
        lcd->setTextColor(
            throttleRadio.hasSignal() ? TFT_GREEN : TFT_RED
        );
        lcd->drawString(
            throttleRadio.hasSignal() ? "OK" : "NO",
            178,
            106
        );

        lcd->setTextColor(TFT_WHITE);
        lcd->setTextSize(2);
        lcd->drawString("ACTIVE", 36, 157);
        lcd->setTextColor(ROUND_CYAN);
        lcd->drawFloat(gyro.getGain(), 2, 136, 157);
        lcd->setTextSize(1);
        lcd->setTextColor(0xBDF7);
        lcd->drawCenterString("CH1 steer / CH2 throttle / CH3 gain", 120, 188);
    }
    else if(radioSection == 1)
    {
        lcd->setTextSize(1);
        lcd->setTextColor(0xBDF7);
        lcd->drawString("STEERING OUT", 34, 58);

        lcd->setTextSize(2);
        lcd->setTextColor(TFT_WHITE);
        lcd->drawNumber(
            mapSteeringForDisplay(
                steeringRadio.getPulseWidth(),
                settings
            ),
            150,
            53
        );

        lcd->drawRect(
            43,
            88,
            154,
            34,
            settings.getServoReverse() ? TFT_YELLOW : ROUND_CYAN
        );
        lcd->drawCenterString(
            settings.getServoReverse() ? "REVERSE ON" : "REVERSE OFF",
            120,
            97
        );

        lcd->setTextSize(1);
        lcd->setTextColor(0xBDF7);
        lcd->drawCenterString("STEERING TRAVEL", 120, 139);

        lcd->drawRect(38, 158, 44, 34, ROUND_CYAN);
        lcd->drawRect(158, 158, 44, 34, ROUND_CYAN);
        lcd->setTextSize(2);
        lcd->setTextColor(TFT_WHITE);
        lcd->drawCenterString("-", 60, 167);
        lcd->drawCenterString("+", 180, 167);
        lcd->drawCenterString(
            String(settings.getRadioSteeringTravel()).c_str(),
            120,
            167
        );
    }
    else
    {
        const bool steeringSignal = steeringRadio.hasSignal();
        const uint8_t calibrationMask =
            settings.getSteeringCalibrationMask();
        const bool calibrationSaved =
            settings.isSteeringCalibrated();

        if(calibrationSaved)
        {
            steeringCalibrationError = false;
        }

        const char* status = calibrationSaved
            ? "SAVED - HOLD TO RESET"
            : (!steeringSignal
                ? "NO STEERING SIGNAL"
                : (steeringCalibrationError
                    ? "INVALID - RETRY"
                    : (calibrationMask != 0
                        ? "CAPTURE REMAINING"
                        : "SET PHYSICAL STOPS")));

        lcd->setTextSize(1);
        lcd->setTextColor(
            calibrationSaved
                ? TFT_GREEN
                : ((!steeringSignal || steeringCalibrationError)
                    ? TFT_RED
                    : 0xBDF7)
        );
        lcd->drawCenterString(status, 120, 43);

        const char* labels[3] =
        {
            "MAX LEFT",
            "CENTER",
            "MAX RIGHT"
        };

        for(int i = 0; i < 3; i++)
        {
            const int y = 57 + (i * 50);
            const bool captured =
                (calibrationMask & (1U << i)) != 0;
            const uint16_t color =
                captured ? TFT_GREEN : TFT_RED;
            String label = labels[i];

            if(captured)
            {
                label += "  ";
                label += String(
                    settings.getSteeringCapturedPulse(i)
                );
            }

            lcd->drawRect(20, y, 200, 42, color);
            lcd->drawRect(21, y + 1, 198, 40, color);
            lcd->setTextSize(2);
            lcd->setTextColor(TFT_WHITE);
            lcd->drawCenterString(label.c_str(), 120, y + 11);
        }
    }

    drawPageDots();
}



void UI::drawSteeringCalibrationPage(
    RadioInput& steeringRadio,
    RadioInput& gainRadio,
    Settings& settings,
    GyroController& gyro
)
{
    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    radioSection = 2;

    drawRoundRadioPage(
        steeringRadio,
        gainRadio,
        settings,
        gyro
    );

    return;
    #else
    drawUiBackground(lcd);

    drawAmoledHeader(
        lcd,
        "Physical Endpoints",
        OD_WARM
    );

    const bool steeringSignal =
        steeringRadio.hasSignal();

    uint8_t calibrationMask =
        settings.getSteeringCalibrationMask();

    bool leftCaptured =
        (calibrationMask & 0x01) != 0;

    bool centerCaptured =
        (calibrationMask & 0x02) != 0;

    bool rightCaptured =
        (calibrationMask & 0x04) != 0;

    bool calibrationSaved =
        settings.isSteeringCalibrated();

    if(calibrationSaved)
    {
        steeringCalibrationError = false;
    }

    const char* status =
        calibrationSaved
            ? "SAVED - TAP TO RESET"
        : (
            !steeringSignal
            ? "NO SIGNAL"
            : (
                steeringCalibrationError
                ? "INVALID - RETRY"
                : (
                    calibrationMask != 0
                    ? "CAPTURE REMAINING"
                    : "CAPTURE ALL 3"
                )
            )
        );

    uint16_t statusColor =
        calibrationSaved
        ? OD_GREEN
        : (
            !steeringSignal || steeringCalibrationError
            ? OD_RED
            : OD_MUTED
        );

    lcd->setTextSize(1);
    lcd->setTextColor(statusColor);
    lcd->drawRightString(status, 434, 20);

    String leftLabel =
        leftCaptured
        ? String("MAX LEFT   ") + String(settings.getSteeringCapturedPulse(0))
        : String("MAX LEFT");

    String centerLabel =
        centerCaptured
        ? String("CENTER   ") + String(settings.getSteeringCapturedPulse(1))
        : String("CENTER");

    String rightLabel =
        rightCaptured
        ? String("MAX RIGHT   ") + String(settings.getSteeringCapturedPulse(2))
        : String("MAX RIGHT");

    const int buttonX = 22;
    const int buttonWidth = 412;
    const int buttonHeight = 54;

    drawAmoledButton(
        lcd,
        buttonX,
        54,
        buttonWidth,
        buttonHeight,
        leftLabel.c_str(),
        leftCaptured ? OD_GREEN : OD_RED,
        2
    );

    drawAmoledButton(
        lcd,
        buttonX,
        116,
        buttonWidth,
        buttonHeight,
        centerLabel.c_str(),
        centerCaptured ? OD_GREEN : OD_RED,
        2
    );

    drawAmoledButton(
        lcd,
        buttonX,
        178,
        buttonWidth,
        buttonHeight,
        rightLabel.c_str(),
        rightCaptured ? OD_GREEN : OD_RED,
        2
    );

    // A second outline makes the calibration targets unmistakable without
    // adding an expensive filled/outlined text rendering pass.
    lcd->drawRect(buttonX + 1, 55, buttonWidth - 2, buttonHeight - 2,
        leftCaptured ? OD_GREEN : OD_RED);
    lcd->drawRect(buttonX + 1, 117, buttonWidth - 2, buttonHeight - 2,
        centerCaptured ? OD_GREEN : OD_RED);
    lcd->drawRect(buttonX + 1, 179, buttonWidth - 2, buttonHeight - 2,
        rightCaptured ? OD_GREEN : OD_RED);

    drawPageDots();
    #endif
}



void UI::drawRadioPage(
    RadioInput& steeringRadio,
    RadioInput& gainRadio,
    Settings& settings,
    GyroController& gyro
)
{
    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    drawRoundRadioPage(
        steeringRadio,
        gainRadio,
        settings,
        gyro
    );
    return;
    #endif

    drawUiBackground(lcd);

    lcd->setTextColor(
        uiTextColor()
    );

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    {
    drawAmoledHeader(
        lcd,
        radioSection == 0 ? "Radio" : "Steering",
        radioSection == 0 ? OD_CYAN : OD_WARM
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
            OD_WARM
        );

        drawAmoledButton(
            lcd,
            242,
            158,
            34,
            40,
            "+",
            OD_WARM
        );

        drawAmoledButton(
            lcd,
            294,
            66,
            134,
            64,
            "REVERSE",
            settings.getServoReverse() ? OD_GREEN : OD_DIM
        );

        lcd->setTextColor(
            settings.getServoReverse() ? OD_GREEN : OD_MUTED
        );

        lcd->drawCenterString(
            settings.getServoReverse() ? "ON" : "OFF",
            361,
            140
        );

        lcd->setTextSize(1);
        lcd->setTextColor(OD_MUTED);
        lcd->drawCenterString(
            "SWIPE FOR",
            361,
            176
        );

        lcd->setTextColor(OD_WARM);
        lcd->drawCenterString(
            "ENDPOINTS",
            361,
            194
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
    lcd->setTextColor(ROUND_CYAN);

    lcd->drawCenterString(
        radioSection == 0 ? "Radio" : "Steering",
        120,
        20
    );

    lcd->setTextColor(TFT_WHITE);

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
    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    // Restore the artwork before refreshing the live receiver values.
    drawRadioPage(
        steeringRadio,
        gainRadio,
        settings,
        gyro
    );
    return;
    #endif

    lcd->setTextColor(
        uiTextColor(),
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
            334,
            136,
            54,
            22,
            TFT_BLACK
        );

        lcd->setTextColor(
            settings.getServoReverse() ? OD_GREEN : OD_MUTED
        );

        lcd->drawCenterString(
            settings.getServoReverse() ? "ON" : "OFF",
            361,
            140
        );

        lcd->setTextColor(
            uiTextColor()
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
        uiTextColor()
    );

    lcd->drawFastVLine(
        steeringCenterPos,
        steeringBarY - 4,
        steeringBarH + 8,
        uiTextColor()
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
        uiTextColor()
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
        uiTextColor()
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
            uiTextColor()
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
        uiTextColor()
    );

    lcd->drawFastVLine(
        steeringCenterPos,
        steeringBarY - 3,
        steeringBarH + 6,
        uiTextColor()
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
        uiTextColor()
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
        uiTextColor()
    );

    flushDisplay();

}





void UI::drawPageDots()
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    // The AMOLED compositor draws the dots after the moving page layer.
    // Keep this flush because page renderers call drawPageDots() last.
    flushDisplay();
    return;
    #endif

    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    // Round dots are a fixed physical overlay, just like the AMOLED dots.
    flushDisplay();
    return;
    #endif

    int spacing =
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        20;
        #else
        16;
        #endif

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
                ROUND_CYAN
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
                ROUND_DIM
                #endif
            );
        }

    }

    flushDisplay();

}



void UI::drawFixedPageDots()
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(!panelCanvasReady)
    {
        return;
    }

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

    for(int i = 0; i < totalPages; i++)
    {
        int landscapeX =
            startX + (i * spacing);

        // The AMOLED panel canvas is the landscape UI rotated clockwise.
        int panelX =
            UI_DOTS_Y;

        int panelY =
            UI_CANVAS_WIDTH - 1 - landscapeX;

        if(i == page)
        {
            panelCanvas.fillCircle(
                panelX,
                panelY,
                5,
                OD_CYAN
            );
        }
        else
        {
            panelCanvas.drawCircle(
                panelX,
                panelY,
                5,
                OD_DIM
            );
        }
    }
    #else
    if(!roundFrameReady)
    {
        return;
    }

    const int spacing = 16;

    int startX =
        (UI_CANVAS_WIDTH / 2) -
        (
            (totalPages - 1)
            *
            spacing
            /
            2
        );

    for(int i = 0; i < totalPages; i++)
    {
        int x =
            startX + (i * spacing);

        if(i == page)
        {
            roundFrame.fillCircle(
                x,
                UI_DOTS_Y,
                4,
                ROUND_CYAN
            );
        }
        else
        {
            roundFrame.drawCircle(
                x,
                UI_DOTS_Y,
                4,
                ROUND_DIM
            );
        }
    }
    #endif
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


bool UI::captureSteeringCalibration(
    uint8_t point,
    RadioInput& steeringRadio,
    Settings& settings
)
{
    if(settings.isSteeringCalibrated())
    {
        steeringCalibrationError = false;
        return false;
    }

    if(!steeringRadio.hasSignal())
    {
        steeringCalibrationError = true;
        return false;
    }

    if(steeringServoOutput == nullptr)
    {
        steeringCalibrationError = true;
        return false;
    }

    int pulse = steeringServoOutput->getCommandPosition();

    if(pulse < 900 || pulse > 2100)
    {
        steeringCalibrationError = true;
        return false;
    }

    bool captured =
        settings.captureSteeringCalibrationPoint(
            point,
            pulse,
            steeringRadio.getPulseWidth()
        );

    steeringCalibrationError = !captured;
    return captured;
}


int8_t UI::repeatButtonAt(
    uint16_t x,
    uint16_t y
)
{
    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(page == PAGE_DRIVE)
    {
        if(buttonPressed(x, y, 276, 36, 70, 82))
            return 1;

        if(buttonPressed(x, y, 364, 36, 70, 82))
            return 2;
    }

    if(page == PAGE_CORE)
    {
        if(buttonPressed(x, y, 276, 48, 70, 48))
            return 3;

        if(buttonPressed(x, y, 364, 48, 70, 48))
            return 4;

        if(buttonPressed(x, y, 276, 110, 70, 48))
            return 5;

        if(buttonPressed(x, y, 364, 110, 70, 48))
            return 6;
    }

    if(page == PAGE_RESPONSE)
    {
        if(buttonPressed(x, y, 276, 48, 70, 48))
            return 7;

        if(buttonPressed(x, y, 364, 48, 70, 48))
            return 8;

        if(buttonPressed(x, y, 276, 110, 70, 48))
            return 23;

        if(buttonPressed(x, y, 364, 110, 70, 48))
            return 24;

        if(buttonPressed(x, y, 276, 172, 70, 48))
            return 25;

        if(buttonPressed(x, y, 364, 172, 70, 48))
            return 26;
    }

    if(page == PAGE_DRIFT_ASSIST)
    {
        if(buttonPressed(x, y, 276, 48, 70, 48))
            return 27;

        if(buttonPressed(x, y, 364, 48, 70, 48))
            return 28;

        if(buttonPressed(x, y, 276, 110, 70, 48))
            return 19;

        if(buttonPressed(x, y, 364, 110, 70, 48))
            return 20;

        if(buttonPressed(x, y, 276, 172, 70, 48))
            return 15;

        if(buttonPressed(x, y, 364, 172, 70, 48))
            return 16;
    }

    if(page == PAGE_EXPERIMENTAL)
    {
        if(buttonPressed(x, y, 276, 48, 70, 48))
            return 29;

        if(buttonPressed(x, y, 364, 48, 70, 48))
            return 30;

        if(buttonPressed(x, y, 276, 110, 70, 48))
            return 35;

        if(buttonPressed(x, y, 364, 110, 70, 48))
            return 36;
    }

    return 0;
    #endif

    if(page == PAGE_DRIVE)
    {
        if(buttonPressed(x, y, 30, 109, 58, 38))
            return 1;

        if(buttonPressed(x, y, 152, 109, 58, 38))
            return 2;
    }

    if(page == PAGE_CORE)
    {
        if(buttonPressed(x, y, 26, 61, 44, 30))
            return 3;

        if(buttonPressed(x, y, 170, 61, 44, 30))
            return 4;

        if(buttonPressed(x, y, 26, 116, 44, 30))
            return 5;

        if(buttonPressed(x, y, 170, 116, 44, 30))
            return 6;
    }

    if(page == PAGE_RESPONSE)
    {
        if(buttonPressed(x, y, 26, 61, 44, 30))
            return 7;

        if(buttonPressed(x, y, 170, 61, 44, 30))
            return 8;

        if(buttonPressed(x, y, 26, 116, 44, 30))
            return 23;

        if(buttonPressed(x, y, 170, 116, 44, 30))
            return 24;

        if(buttonPressed(x, y, 26, 171, 44, 30))
            return 25;

        if(buttonPressed(x, y, 170, 171, 44, 30))
            return 26;
    }

    if(page == PAGE_DRIFT_ASSIST)
    {
        if(buttonPressed(x, y, 26, 61, 44, 30))
            return 27;

        if(buttonPressed(x, y, 170, 61, 44, 30))
            return 28;

        if(buttonPressed(x, y, 26, 116, 44, 30))
            return 19;

        if(buttonPressed(x, y, 170, 116, 44, 30))
            return 20;

        if(buttonPressed(x, y, 26, 171, 44, 30))
            return 15;

        if(buttonPressed(x, y, 170, 171, 44, 30))
            return 16;
    }

    if(page == PAGE_EXPERIMENTAL)
    {
        if(buttonPressed(x, y, 26, 61, 44, 30))
            return 29;

        if(buttonPressed(x, y, 170, 61, 44, 30))
            return 30;

        if(buttonPressed(x, y, 26, 116, 44, 30))
            return 31;

        if(buttonPressed(x, y, 170, 116, 44, 30))
            return 32;

        if(buttonPressed(x, y, 26, 171, 44, 30))
            return 33;

        if(buttonPressed(x, y, 170, 171, 44, 30))
            return 34;
    }

    return 0;
}


bool UI::actionButtonAt(
    uint16_t x,
    uint16_t y
)
{
    if(repeatButtonAt(x, y) != 0)
    {
        return true;
    }

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(page == PAGE_DRIVE)
        return buttonPressed(x, y, 276, 148, 158, 54);

    if(page == PAGE_CORE)
        return buttonPressed(x, y, 276, 172, 158, 48);

    if(page == PAGE_STEERING)
    {
        return
            buttonPressed(x, y, 202, 158, 34, 40) ||
            buttonPressed(x, y, 242, 158, 34, 40) ||
            buttonPressed(x, y, 294, 66, 134, 64);
    }

    if(page == PAGE_STEERING_CAL)
    {
        return
            buttonPressed(x, y, 22, 54, 412, 54) ||
            buttonPressed(x, y, 22, 116, 412, 54) ||
            buttonPressed(x, y, 22, 178, 412, 54);
    }

    if(page == PAGE_WIFI)
        return buttonPressed(x, y, 296, 70, 130, 92);

    if(page == PAGE_SYSTEM && buttonPressed(x, y, 150, 54, 240, 38))
        return true;

    if(
        page == PAGE_SYSTEM &&
        (
            buttonPressed(x, y, 150, 106, 116, 38) ||
            buttonPressed(x, y, 274, 106, 116, 38)
        )
    )
        return true;

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    if(page == PAGE_SYSTEM && buttonPressed(x, y, 150, 156, 240, 38))
        return true;
    #endif

    if(page == PAGE_BLACKBOX)
    {
        return
            buttonPressed(x, y, 300, 51, 128, 38) ||
            buttonPressed(x, y, 22, 210, 270, 38) ||
            buttonPressed(x, y, 300, 210, 128, 38);
    }

    if(
        page == PAGE_SYSTEM
        #if defined(OPENDRIFT_INPUT_CRSF)
        && false
        #endif
    )
        return buttonPressed(x, y, 150, 202, 240, 38);
    #else
    if(page == PAGE_DRIVE)
        return buttonPressed(x, y, 55, 164, 130, 38);

    if(page == PAGE_CORE)
        return buttonPressed(x, y, 43, 171, 154, 34);

    if(page == PAGE_STEERING)
    {
        return
            buttonPressed(x, y, 43, 88, 154, 34) ||
            buttonPressed(x, y, 38, 158, 44, 34) ||
            buttonPressed(x, y, 158, 158, 44, 34);
    }

    if(page == PAGE_STEERING_CAL)
    {
        return
            buttonPressed(x, y, 20, 57, 200, 42) ||
            buttonPressed(x, y, 20, 107, 200, 42) ||
            buttonPressed(x, y, 20, 157, 200, 42);
    }

    if(page == PAGE_WIFI)
        return buttonPressed(x, y, 50, 145, 140, 42);

    if(
        page == PAGE_SYSTEM
        #if defined(OPENDRIFT_INPUT_CRSF)
        && false
        #endif
    )
        return buttonPressed(x, y, 43, 137, 154, 40);

    #endif

    return false;
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

            drawCorePage(
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

            drawCorePage(
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

            gyro.setHoldBoost(
                settings.getGyroHoldBoost()
            );
            break;

        case 20:
            settings.setGyroHoldBoost(
                settings.getGyroHoldBoost() + 1
            );

            gyro.setHoldBoost(
                settings.getGyroHoldBoost()
            );
            break;

        case 23:
            settings.setPredictionStrength(
                settings.getPredictionStrength() - 1
            );

            gyro.setPredictionStrength(
                settings.getPredictionStrength()
            );
            break;

        case 24:
            settings.setPredictionStrength(
                settings.getPredictionStrength() + 1
            );

            gyro.setPredictionStrength(
                settings.getPredictionStrength()
            );
            break;

        case 25:
            settings.setServoQuiet(
                settings.getServoQuiet() - 1
            );
            break;

        case 26:
            settings.setServoQuiet(
                settings.getServoQuiet() + 1
            );
            break;

        case 27:
            settings.setGyroCounterSteerAssist(
                settings.getGyroCounterSteerAssist() - 1
            );

            gyro.setCounterSteerAssist(
                settings.getGyroCounterSteerAssist()
            );
            break;

        case 28:
            settings.setGyroCounterSteerAssist(
                settings.getGyroCounterSteerAssist() + 1
            );

            gyro.setCounterSteerAssist(
                settings.getGyroCounterSteerAssist()
            );
            break;

        case 29:
            settings.setGyroTransitionSpeed(
                settings.getGyroTransitionSpeed() - 1
            );

            gyro.setTransitionSpeed(
                settings.getGyroTransitionSpeed()
            );
            break;

        case 30:
            settings.setGyroTransitionSpeed(
                settings.getGyroTransitionSpeed() + 1
            );

            gyro.setTransitionSpeed(
                settings.getGyroTransitionSpeed()
            );
            break;

        case 31:
            settings.setGyroHuntStrength(
                settings.getGyroHuntStrength() - 1
            );

            gyro.setHuntStrength(
                settings.getGyroHuntStrength()
            );
            break;

        case 32:
            settings.setGyroHuntStrength(
                settings.getGyroHuntStrength() + 1
            );

            gyro.setHuntStrength(
                settings.getGyroHuntStrength()
            );
            break;

        case 33:
            settings.setControlLoopHz(250);
            break;

        case 34:
            settings.setControlLoopHz(333);
            break;

        case 35:
            settings.setDriverPriority(
                settings.getDriverPriority() - 1
            );

            gyro.setDriverPriority(
                settings.getDriverPriority()
            );
            break;

        case 36:
            settings.setDriverPriority(
                settings.getDriverPriority() + 1
            );

            gyro.setDriverPriority(
                settings.getDriverPriority()
            );
            break;

        default:
            return false;
    }

    if(page == PAGE_CORE)
    {
        drawCorePage(
            gyro,
            settings
        );
    }
    else if(page == PAGE_DRIFT_ASSIST)
    {
        drawDriftAssistPage(
            settings
        );
    }
    else if(page == PAGE_EXPERIMENTAL)
    {
        drawExperimentalPage(
            settings
        );
    }
    else
    {
        drawResponsePage(
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

    #if defined(OPENDRIFT_USB_MAINTENANCE)
    if(firmwareUpdateNoticeVisible)
    {
        if((int32_t)(millis() - firmwareUpdateNoticeUntil) < 0)
        {
            lastTouchState = touched;
            return;
        }

        firmwareUpdateNoticeVisible = false;
        drawPage(
            gyro,
            wifi,
            settings,
            steeringRadio,
            gainRadio
        );
        lastTouchState = touched;
        return;
    }
    #endif

    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    if(updateDisplayBrightness(settings, touched))
    {
        lastTouchState = touched;
        return;
    }

    if(syncTheme(settings))
    {
        refreshRequested = true;
    }

    if(applyBackground(settings))
    {
        refreshRequested = true;
    }

    if(blackboxArchive != nullptr)
    {
        const BlackboxArchive::Status archiveStatus =
            blackboxArchive->getStatus();

        if(archiveStatus == BlackboxArchive::SAVING)
        {
            const uint8_t progress = blackboxArchive->getProgress();

            if(!blackboxProgressVisible || progress != lastBlackboxProgress)
            {
                drawBlackboxProgress(progress, archiveStatus);
                lastBlackboxProgress = progress;
            }

            blackboxProgressVisible = true;
            blackboxResultShownAt = 0;
            trackingSwipe = false;
            swipePreviewActive = false;
            heldRepeatButton = 0;
            lastTouchState = touched;
            return;
        }

        if(blackboxProgressVisible)
        {
            if(blackboxResultShownAt == 0)
            {
                drawBlackboxProgress(
                    archiveStatus == BlackboxArchive::SAVED ? 100 : lastBlackboxProgress,
                    archiveStatus
                );
                blackboxResultShownAt = millis();
            }

            if(millis() - blackboxResultShownAt < 1800)
            {
                lastTouchState = touched;
                return;
            }

            blackboxProgressVisible = false;
            blackboxResultShownAt = 0;
            lastBlackboxProgress = 255;
            trackingSwipe = false;
            swipePreviewActive = false;
            heldRepeatButton = 0;
            lastTouchState = touched;
            refreshRequested = false;

            drawPage(
                gyro,
                wifi,
                settings,
                steeringRadio,
                gainRadio
            );
            return;
        }
    }
    #endif

    uint8_t gesture =
        touch.getGesture();

    if(
        refreshRequested &&
        !touched &&
        !trackingSwipe
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        && !swipePreviewActive
        #endif
    )
    {
        refreshRequested = false;

        drawPage(
            gyro,
            wifi,
            settings,
            steeringRadio,
            gainRadio
        );

        return;
    }

    #if !defined(OPENDRIFT_BOARD_AMOLED_164)
    if(
        trackingSwipe &&
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
        trackingSwipe &&
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

    // Do not push a live-value refresh on the release frame. The swipe
    // handler below still owns the display until it commits or cancels the
    // preview; refreshing here would briefly restore the source page.
    if(
        (
            page == PAGE_DRIVE ||
            page == PAGE_RADIO ||
            page == PAGE_STEERING
            #if defined(OPENDRIFT_BOARD_AMOLED_164)
            || page == PAGE_BLACKBOX
            #endif
        ) &&
        !touched &&
        !lastTouchState &&
        !trackingSwipe &&
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        !swipePreviewActive &&
        #endif
        millis() - lastRadioRefresh >
            ((page == PAGE_RADIO || page == PAGE_STEERING) ? 50UL : 250UL)
    )
    {
        if(page == PAGE_DRIVE)
        {
            int16_t activeGainHundredths =
                (int16_t)(gyro.getGain() * 100.0f + 0.5f);

            if(activeGainHundredths != lastDrawnGainHundredths)
            {
                drawMainPage(
                    gyro,
                    settings
                );
            }
        }
        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        else if(page == PAGE_BLACKBOX)
        {
            drawBlackboxPage(settings);
        }
        #endif
        else
        {
            updateRadioPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );
        }

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

        heldRepeatButton =
            repeatButtonAt(
                touchStartX,
                touchStartY
            );

        trackingSwipe =
            !actionButtonAt(
                touchStartX,
                touchStartY
            );

        nextRepeatAt = 0;

        #if defined(OPENDRIFT_BOARD_AMOLED_164)
        swipePreviewActive = false;

        swipePreviewDirection = 0;

        swipePreviewOffset = 0;

        lastSwipePreviewAt = 0;
        #endif

        if(heldRepeatButton != 0)
        {
            applyRepeatButton(
                heldRepeatButton,
                gyro,
                settings
            );

            nextRepeatAt =
                millis() + 450;

            lastTouchState =
                touched;

            return;
        }

    }


    if(
        touched &&
        heldRepeatButton != 0
    )
    {
        int8_t currentButton =
            repeatButtonAt(
                touch.getX(),
                touch.getY()
            );

        if(currentButton == heldRepeatButton)
        {
            unsigned long now =
                millis();

            if((long)(now - nextRepeatAt) >= 0)
            {
                applyRepeatButton(
                    heldRepeatButton,
                    gyro,
                    settings
                );

                nextRepeatAt =
                    now + 90;
            }
        }
        else
        {
            heldRepeatButton = 0;
        }

        lastTouchState =
            touched;

        return;
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
                    millis() - lastSwipePreviewAt >= 16 &&
                    offset != swipePreviewOffset
                )
                {
                    swipePreviewOffset =
                        offset;

                    // Count composition/transfer time toward the next frame,
                    // rather than adding another delay after every push.
                    lastSwipePreviewAt = millis();

                    flushTransitionDisplay(
                        swipePreviewOffset,
                        swipePreviewDirection
                    );

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

        steeringCalibrationResetPoint = -1;
        steeringCalibrationResetStartedAt = 0;

        #if defined(OPENDRIFT_USB_MAINTENANCE)
        if(usbMaintenanceHoldStartedAt != 0)
        {
            usbMaintenanceHoldStartedAt = 0;
            if(page == PAGE_SYSTEM) drawSystemPage(settings);
        }
        #endif

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
                isProfilesPage() &&
                abs(deltaY) > 34 &&
                abs(deltaY) > abs(delta)
            )
            {
                #if defined(OPENDRIFT_BOARD_AMOLED_164)
                if(swipePreviewActive)
                {
                    finishSwipePreview(false);
                }
                #endif

                uint8_t profileCount =
                    settings.getProfileCount();

                uint8_t maxScroll =
                    profileCount > 4
                    ?
                    profileCount - 4
                    :
                    0;

                int rowHeight =
                    #if defined(OPENDRIFT_BOARD_AMOLED_164)
                    47;
                    #else
                    40;
                    #endif

                uint8_t steps = max(
                    1,
                    abs(deltaY) / rowHeight
                );

                if(deltaY < 0)
                {
                    profileScroll = min(
                        (int)maxScroll,
                        (int)profileScroll + steps
                    );
                }
                else
                {
                    profileScroll = max(
                        0,
                        (int)profileScroll - steps
                    );
                }

                drawProfilesPage(settings);
            }
            else if(
                isProfilesPage() &&
                abs(delta) < 22 &&
                abs(deltaY) < 22
            )
            {
                #if defined(OPENDRIFT_BOARD_AMOLED_164)
                if(swipePreviewActive)
                {
                    finishSwipePreview(false);
                }
                #endif

                int rowStart =
                    #if defined(OPENDRIFT_BOARD_AMOLED_164)
                    46;
                    #else
                    43;
                    #endif

                int rowHeight =
                    #if defined(OPENDRIFT_BOARD_AMOLED_164)
                    47;
                    #else
                    40;
                    #endif

                int rowEnd =
                    rowStart + (4 * rowHeight);

                if(
                    touchStartY >= rowStart &&
                    touchStartY < rowEnd
                )
                {
                    uint8_t slot =
                        (touchStartY - rowStart) /
                        rowHeight;

                    uint8_t index =
                        profileScroll + slot;

                    if(index < settings.getProfileCount())
                    {
                        settings.activateProfile(index);
                    }
                }

                drawProfilesPage(settings);
            }
            #if defined(OPENDRIFT_BOARD_AMOLED_164)
            else if(
                isBackgroundsPage() &&
                abs(deltaY) > 34 &&
                abs(deltaY) > abs(delta)
            )
            {
                if(swipePreviewActive)
                {
                    finishSwipePreview(false);
                }

                const uint8_t rowCount =
                    backgroundStore != nullptr && backgroundStore->isReady()
                        ? backgroundStore->getCount() + 1
                        : 1;
                const uint8_t maxScroll =
                    rowCount > 4 ? rowCount - 4 : 0;
                const uint8_t steps = max(1, abs(deltaY) / 47);

                backgroundScroll =
                    deltaY < 0
                        ? min((int)maxScroll, (int)backgroundScroll + steps)
                        : max(0, (int)backgroundScroll - steps);

                drawBackgroundsPage(settings);
            }
            else if(
                isBackgroundsPage() &&
                abs(delta) < 22 &&
                abs(deltaY) < 22
            )
            {
                if(swipePreviewActive)
                {
                    finishSwipePreview(false);
                }

                if(touchStartY >= 46 && touchStartY < 46 + 4 * 47)
                {
                    const uint8_t index =
                        backgroundScroll + (touchStartY - 46) / 47;

                    if(index == 0)
                    {
                        settings.setBackgroundName("");
                    }
                    else if(
                        backgroundStore != nullptr &&
                        index - 1 < backgroundStore->getCount()
                    )
                    {
                        settings.setBackgroundName(
                            backgroundStore->getName(index - 1)
                        );
                    }

                    applyBackground(settings);
                }

                drawBackgroundsPage(settings);
            }
            #endif
            else
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
                page == PAGE_RADIO &&
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
        }


        trackingSwipe=false;

        heldRepeatButton = 0;

        nextRepeatAt = 0;

    }


    #if defined(OPENDRIFT_BOARD_AMOLED_164)
    #if defined(OPENDRIFT_USB_MAINTENANCE)
    if(touched && page == PAGE_SYSTEM && usbMaintenanceHoldStartedAt != 0)
    {
        const bool stillOnButton = buttonPressed(
            touch.getX(),
            touch.getY(),
            150,
            156,
            240,
            38
        );

        if(!stillOnButton)
        {
            usbMaintenanceHoldStartedAt = 0;
            drawSystemPage(settings);
        }
        else if(millis() - usbMaintenanceHoldStartedAt >= 1500)
        {
            usbMaintenanceHoldStartedAt = 0;
            if(usbMaintenanceCallback != nullptr) usbMaintenanceCallback();
        }

        lastTouchState = touched;
        return;
    }
    #endif

    if(
        touched &&
        page == PAGE_STEERING_CAL &&
        steeringCalibrationResetPoint >= 0
    )
    {
        uint16_t currentX = touch.getX();
        uint16_t currentY = touch.getY();

        int8_t currentPoint =
            buttonPressed(currentX, currentY, 22, 54, 412, 54)
            ? 0
            : (
                buttonPressed(currentX, currentY, 22, 116, 412, 54)
                ? 1
                : (
                    buttonPressed(currentX, currentY, 22, 178, 412, 54)
                    ? 2
                    : -1
                )
            );

        if(currentPoint != steeringCalibrationResetPoint)
        {
            steeringCalibrationResetPoint = -1;
            steeringCalibrationResetStartedAt = 0;
        }
        else if(
            millis() - steeringCalibrationResetStartedAt >= 1200 &&
            settings.isSteeringCalibrated() &&
            fabsf(gyro.getFilteredYaw()) < 5.0f
        )
        {
            settings.clearSteeringCalibration();

            captureSteeringCalibration(
                currentPoint,
                steeringRadio,
                settings
            );

            steeringCalibrationResetPoint = -1;
            steeringCalibrationResetStartedAt = 0;

            drawSteeringCalibrationPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );
        }

        lastTouchState = touched;
        return;
    }
    #endif







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
        if(
            page == PAGE_DRIVE &&
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
            if(calibrationCallback != nullptr)
            {
                calibrationCallback();
            }

            drawMainPage(
                gyro,
                settings
            );

            lastTouchState =
                touched;

            return;
        }

        if(
            page == PAGE_CORE &&
            buttonPressed(
                x,
                y,
                276,
                172,
                158,
                48
            )
        )
        {
            settings.setGyroReverse(
                !settings.getGyroReverse()
            );

            drawCorePage(
                gyro,
                settings
            );

            lastTouchState =
                touched;

            return;
        }

        if(page == PAGE_STEERING)
        {
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

            if(buttonPressed(x, y, 294, 66, 134, 64))
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

        if(page == PAGE_STEERING_CAL)
        {
            int8_t calibrationPoint =
                buttonPressed(x, y, 22, 54, 412, 54)
                ? 0
                : (
                    buttonPressed(x, y, 22, 116, 412, 54)
                    ? 1
                    : (
                        buttonPressed(x, y, 22, 178, 412, 54)
                        ? 2
                        : -1
                    )
                );

            if(calibrationPoint >= 0)
            {
                if(settings.isSteeringCalibrated())
                {
                    steeringCalibrationResetPoint = calibrationPoint;
                    steeringCalibrationResetStartedAt = millis();
                }
                else
                {
                    captureSteeringCalibration(
                        calibrationPoint,
                        steeringRadio,
                        settings
                    );
                }

                drawSteeringCalibrationPage(
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
            page == PAGE_WIFI &&
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

        if(
            page == PAGE_SYSTEM &&
            buttonPressed(
                x,
                y,
                150,
                54,
                240,
                38
            )
        )
        {
            settings.setControlLoopHz(
                settings.getControlLoopHz() == 250 ? 333 : 250
            );

            drawSystemPage(settings);

            lastTouchState = touched;
            return;
        }

        if(
            page == PAGE_SYSTEM &&
            buttonPressed(x, y, 150, 106, 116, 38)
        )
        {
            settings.setThemeAccent(
                (settings.getThemeAccent() + 1) % Settings::THEME_ACCENT_COUNT
            );

            syncTheme(settings);
            drawSystemPage(settings);
            lastTouchState = touched;
            return;
        }

        if(
            page == PAGE_SYSTEM &&
            buttonPressed(x, y, 274, 106, 116, 38)
        )
        {
            settings.setThemeText(
                settings.getThemeText() == 1 ? 0 : 1
            );

            syncTheme(settings);
            drawSystemPage(settings);
            lastTouchState = touched;
            return;
        }

        if(page == PAGE_BLACKBOX)
        {
            if(buttonPressed(x, y, 300, 51, 128, 38))
            {
                settings.setBlackboxEnabled(!settings.getBlackboxEnabled());
                drawBlackboxPage(settings);
                lastTouchState = touched;
                return;
            }

            if(
                buttonPressed(x, y, 22, 210, 270, 38) &&
                blackboxArchive != nullptr
            )
            {
                const bool noReceiverSignals =
                    !steeringRadio.hasSignal() &&
                    (
                        throttleRadioInput == nullptr ||
                        !throttleRadioInput->hasSignal()
                    );

                blackboxArchive->requestSave(noReceiverSignals);
                drawBlackboxPage(settings);
                lastTouchState = touched;
                return;
            }

            if(
                buttonPressed(x, y, 300, 210, 128, 38) &&
                blackboxArchive != nullptr
            )
            {
                blackboxArchive->clearRamLog();
                drawBlackboxPage(settings);
                lastTouchState = touched;
                return;
            }
        }

        #if defined(OPENDRIFT_USB_MAINTENANCE)
        if(
            page == PAGE_SYSTEM &&
            buttonPressed(x, y, 150, 156, 240, 38)
        )
        {
            usbMaintenanceHoldStartedAt = millis();
            drawSystemPage(settings);
            lastTouchState = touched;
            return;
        }
        #endif

        if(
            page == PAGE_SYSTEM &&
            #if defined(OPENDRIFT_INPUT_CRSF)
            false &&
            #endif
            buttonPressed(
                x,
                y,
                150,
                202,
                240,
                38
            )
        )
        {
            settings.setThrottleOutputEnabled(
                !settings.getThrottleOutputEnabled()
            );

            drawSystemPage(
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

        if(page == PAGE_DRIVE)
        {


            if(buttonPressed(
                x,y,
                30,109,
                58,38
            ))
            {

                float g =
                    gyro.getGain()-0.01f;


                gyro.setGain(g);

                settings.setGain(g);

            }




            if(buttonPressed(
                x,y,
                152,109,
                58,38
            ))
            {

                float g =
                    gyro.getGain()+0.01f;


                gyro.setGain(g);

                settings.setGain(g);

            }




            if(buttonPressed(
                x,y,
                55,164,
                130,38
            ))
            {

                if(calibrationCallback != nullptr)
                {
                    calibrationCallback();
                }

            }


            drawMainPage(
                gyro,
                settings
            );

        }







        // CONTROL PAGE BUTTONS

        if(page == PAGE_CORE)
        {

            if(buttonPressed(
                x,y,
                26,61,
                44,30
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
                170,61,
                44,30
            ))
            {

                float deadband =
                    gyro.getDeadband()+1.0f;


                gyro.setDeadband(deadband);

                settings.setDeadband(deadband);

            }

            if(buttonPressed(
                x,y,
                43,171,
                154,34
            ))
            {

                settings.setGyroReverse(
                    !settings.getGyroReverse()
                );

            }

            drawCorePage(
                gyro,
                settings
            );

        }







        // RADIO PAGE BUTTONS

        if(page == PAGE_STEERING)
        {
            if(buttonPressed(x, y, 43, 88, 154, 34))
            {
                settings.setServoReverse(
                    !settings.getServoReverse()
                );
            }

            if(buttonPressed(x, y, 38, 158, 44, 34))
            {
                settings.setRadioSteeringTravel(
                    settings.getRadioSteeringTravel() - 1
                );
            }

            if(buttonPressed(x, y, 158, 158, 44, 34))
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


        if(page == PAGE_STEERING_CAL)
        {
            int8_t calibrationPoint =
                buttonPressed(x, y, 20, 57, 200, 42)
                ? 0
                : (buttonPressed(x, y, 20, 107, 200, 42)
                    ? 1
                    : (buttonPressed(x, y, 20, 157, 200, 42)
                        ? 2
                        : -1));

            if(calibrationPoint >= 0)
            {
                captureSteeringCalibration(
                    calibrationPoint,
                    steeringRadio,
                    settings
                );
            }

            drawSteeringCalibrationPage(
                steeringRadio,
                gainRadio,
                settings,
                gyro
            );
        }

        if(
            false &&
            page == PAGE_STEERING
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

        if(page == PAGE_WIFI)
        {

            if(buttonPressed(
                x,y,
                50,
                145,
                140,
                42
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

                // WiFi startup/shutdown can outlive the original tap. Prevent
                // its stale gesture from being handled as a page swipe.
                trackingSwipe = false;

                lastPageSwipe =
                    millis();

            }

        }

        if(
            page == PAGE_SYSTEM &&
            #if defined(OPENDRIFT_INPUT_CRSF)
            false &&
            #endif
            buttonPressed(x, y, 43, 137, 154, 40)
        )
        {
            settings.setThrottleOutputEnabled(
                !settings.getThrottleOutputEnabled()
            );

            drawSystemPage(settings);
        }


    }

    lastTouchState =
        touched;

}


#if defined(OPENDRIFT_BOARD_AMOLED_164)
bool UI::syncTheme(Settings& settings)
{
    uint8_t textMode = settings.getThemeText();
    uint8_t accent = settings.getThemeAccent();

    if(
        themeApplied &&
        textMode == appliedThemeText &&
        accent == appliedThemeAccent
    )
    {
        return false;
    }

    themeApplied = true;
    appliedThemeText = textMode;
    appliedThemeAccent = accent;
    applyAmoledTheme(textMode, accent);
    return true;
}


bool UI::updateDisplayBrightness(
    Settings& settings,
    bool touched
)
{
    unsigned long now = millis();
    uint16_t timeoutSeconds = settings.getDisplayDimTimeout();
    uint8_t brightnessPercent = settings.getDisplayBrightness();

    if(
        timeoutSeconds != lastDimTimeoutSeconds ||
        brightnessPercent != lastBrightnessPercent
    )
    {
        lastDimTimeoutSeconds = timeoutSeconds;
        lastBrightnessPercent = brightnessPercent;
        lastTouchMs = now;
    }

    if(touched)
    {
        if(displayDimmed)
        {
            swallowTouchUntilRelease = true;
        }

        lastTouchMs = now;
    }

    displayDimmed =
        timeoutSeconds > 0 &&
        now - lastTouchMs >= (unsigned long)timeoutSeconds * 1000UL;

    uint8_t level =
        (uint8_t)((brightnessPercent * 255U + 50U) / 100U);

    if(displayDimmed)
    {
        level = max((uint8_t)4, (uint8_t)(level / 10));
    }

    if(display != nullptr && level != appliedBrightnessLevel)
    {
        display->setBrightness(level);
        appliedBrightnessLevel = level;
    }

    if(swallowTouchUntilRelease)
    {
        if(touched)
        {
            return true;
        }

        swallowTouchUntilRelease = false;
    }

    return false;
}
#endif
