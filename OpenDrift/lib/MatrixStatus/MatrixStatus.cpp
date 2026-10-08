#include "MatrixStatus.h"

#if defined(OPENDRIFT_BOARD_MATRIX) || defined(OPENDRIFT_BOARD_ZERO)

void MatrixStatus::begin()
{
    pixels.begin();
    pixels.setBrightness(BRIGHTNESS);
    pixels.clear();
    pixels.show();

    started = true;
    state = State::Boot;
    dirty = true;
    render();
}


void MatrixStatus::setState(State nextState)
{
    if(state == nextState && !dirty)
    {
        return;
    }

    state = nextState;
    dirty = true;
    render();
}


void MatrixStatus::setRotation(uint8_t nextRotation)
{
    nextRotation &= 3;

    if(rotation == nextRotation)
    {
        return;
    }

    rotation = nextRotation;
    dirty = true;
    render();
}


void MatrixStatus::update(
    bool receiverSignal,
    bool wifiEnabled,
    bool blackboxReady
)
{
    // A fatal startup fault must remain visible instead of being replaced by
    // the normal receiver-status update on the next pass through loop().
    if(state == State::Error)
    {
        return;
    }

    State nextState = receiverSignal
        ? State::Ready
        : State::NoSignal;

    if(
        nextState == state &&
        wifiEnabled == wifi &&
        blackboxReady == blackbox
    )
    {
        return;
    }

    state = nextState;
    wifi = wifiEnabled;
    blackbox = blackboxReady;
    dirty = true;
    render();
}


void MatrixStatus::render()
{
    if(!started || !dirty)
    {
        return;
    }

    clear();

    #if defined(OPENDRIFT_BOARD_ZERO)
    // Single RGB LED: never run 8x8 drawing/rotation on this hardware.
    uint32_t color = pixels.Color(0, 28, 32); // Boot: cyan.
    switch(state)
    {
        case State::Boot: break;
        case State::Calibrating: color = pixels.Color(30, 18, 0); break;
        case State::Ready: color = pixels.Color(0, 32, 5); break;
        case State::NoSignal: color = pixels.Color(36, 0, 0); break;
        case State::Error: color = pixels.Color(36, 0, 18); break;
    }
    pixels.setPixelColor(0, color);
    pixels.show();
    dirty = false;
    return;
    #endif

    switch(state)
    {
        case State::Boot:
            drawHourglass(pixels.Color(0, 28, 32));
            break;

        case State::Calibrating:
            drawHourglass(pixels.Color(30, 18, 0));
            break;

        case State::Ready:
            drawCheck(pixels.Color(0, 32, 5));
            break;

        case State::NoSignal:
            drawCross(pixels.Color(36, 0, 0));
            break;

        case State::Error:
            drawCross(pixels.Color(36, 0, 18));
            break;
    }

    // Corner indicators preserve the main status symbol: blue means that the
    // web configurator AP is enabled, and purple means blackbox RAM is ready.
    if(wifi)
    {
        setPixel(0, 7, pixels.Color(0, 8, 32));
    }

    if(blackbox)
    {
        setPixel(7, 7, pixels.Color(18, 0, 28));
    }

    pixels.show();
    dirty = false;
}


void MatrixStatus::clear()
{
    pixels.clear();
}


void MatrixStatus::setPixel(
    int x,
    int y,
    uint32_t color
)
{
    if(x < 0 || x >= 8 || y < 0 || y >= 8)
    {
        return;
    }

    int physicalX = x;
    int physicalY = y;

    switch(rotation)
    {
        case 1:
            physicalX = 7 - y;
            physicalY = x;
            break;

        case 2:
            physicalX = 7 - x;
            physicalY = 7 - y;
            break;

        case 3:
            physicalX = y;
            physicalY = 7 - x;
            break;

        default:
            break;
    }

    pixels.setPixelColor(
        (physicalY * 8) + physicalX,
        color
    );
}


void MatrixStatus::drawCheck(uint32_t color)
{
    setPixel(1, 4, color);
    setPixel(2, 5, color);
    setPixel(3, 6, color);
    setPixel(4, 5, color);
    setPixel(5, 4, color);
    setPixel(6, 3, color);
    setPixel(6, 2, color);
}


void MatrixStatus::drawCross(uint32_t color)
{
    for(int index = 1; index <= 6; index++)
    {
        setPixel(index, index, color);
        setPixel(7 - index, index, color);
    }
}


void MatrixStatus::drawHourglass(uint32_t color)
{
    for(int x = 2; x <= 5; x++)
    {
        setPixel(x, 1, color);
        setPixel(x, 6, color);
    }

    setPixel(2, 2, color);
    setPixel(5, 2, color);
    setPixel(3, 3, color);
    setPixel(4, 3, color);
    setPixel(3, 4, color);
    setPixel(4, 4, color);
    setPixel(2, 5, color);
    setPixel(5, 5, color);
}

#endif
