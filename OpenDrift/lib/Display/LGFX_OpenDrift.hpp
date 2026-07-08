#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_GC9A01 _panel;
    lgfx::Bus_SPI _bus;

public:

    LGFX()
    {
        // ==========================
        // SPI BUS
        // ==========================
        {
            auto cfg = _bus.config();

            cfg.spi_host = SPI2_HOST;

            cfg.spi_mode = 0;

            cfg.freq_write = 40000000;
            cfg.freq_read  = 16000000;

            cfg.spi_3wire = true;
            cfg.use_lock  = true;

            cfg.dma_channel = SPI_DMA_CH_AUTO;

            cfg.pin_sclk = 10;
            cfg.pin_mosi = 11;
            cfg.pin_miso = 12;

            cfg.pin_dc   = 8;

            _bus.config(cfg);
            _panel.setBus(&_bus);
        }

        // ==========================
        // LCD PANEL
        // ==========================
        {
            auto cfg = _panel.config();

            cfg.pin_cs   = 9;
            cfg.pin_rst  = 14;
            cfg.pin_busy = -1;

            cfg.panel_width  = 240;
            cfg.panel_height = 240;

            cfg.memory_width  = 240;
            cfg.memory_height = 240;

            cfg.offset_x = 0;
            cfg.offset_y = 0;

            cfg.offset_rotation = 0;

            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;

            cfg.readable = false;

            cfg.invert = true;
            cfg.rgb_order = false;

            cfg.dlen_16bit = false;

            cfg.bus_shared = true;

            _panel.config(cfg);
        }

        setPanel(&_panel);
    }
};