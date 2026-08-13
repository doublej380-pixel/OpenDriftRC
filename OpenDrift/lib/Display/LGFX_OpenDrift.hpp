#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

#if defined(OPENDRIFT_BOARD_AMOLED_164)

#include <lgfx/v1/panel/Panel_SH8601Z.hpp>

class OpenDriftPanelSH8601 : public lgfx::Panel_SH8601Z
{
public:

    bool init(
        bool useReset
    ) override
    {
        if(!lgfx::Panel_Device::init(
            useReset
        ))
        {
            return false;
        }

        uint8_t initCommands[] =
        {
            0x11, 0 + CMD_INIT_DELAY, 80,
            0xC4, 1, 0x80,
            0x35, 1, 0x00,
            0x3A, 1 + CMD_INIT_DELAY, 0x55, 1,
            0x53, 1 + CMD_INIT_DELAY, 0x20, 1,
            0x63, 1 + CMD_INIT_DELAY, 0xFF, 1,
            0x51, 1 + CMD_INIT_DELAY, 0x00, 1,
            0x29, 0 + CMD_INIT_DELAY, 10,
            0x51, 1, 0xFF,
            0xFF, 0xFF
        };

        command_list(
            initCommands
        );

        return true;
    }

    void setRotation(
        uint_fast8_t r
    ) override
    {
        lgfx::Panel_SH8601Z::setRotation(
            r
        );

        uint8_t madctl = 0x00;

        switch(
            _internal_rotation
            &
            3
        )
        {
            case 1:
                madctl = 0x60;
                break;

            case 2:
                madctl = 0xC0;
                break;

            case 3:
                madctl = 0xA0;
                break;

            default:
                madctl = 0x00;
                break;
        }

        startWrite();
        cs_control(
            false
        );
        write_cmd(
            0x36
        );
        _bus->writeCommand(
            madctl,
            8
        );
        _bus->wait();
        cs_control(
            true
        );
        endWrite();
    }

    void setWindow(
        uint_fast16_t xs,
        uint_fast16_t ys,
        uint_fast16_t xe,
        uint_fast16_t ye
    ) override
    {
        static constexpr uint_fast16_t X_OFFSET = 0x14;

        lgfx::Panel_SH8601Z::setWindow(
            xs + X_OFFSET,
            ys,
            xe + X_OFFSET,
            ye
        );
    }
};

class LGFX : public lgfx::LGFX_Device
{
    OpenDriftPanelSH8601 _panel;
    lgfx::Bus_SPI _bus;

public:

    LGFX()
    {
        {
            auto cfg = _bus.config();

            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;

            // Waveshare's factory firmware drives this SH8601 QSPI panel at
            // 40 MHz. Matching that proven clock cuts full-frame transfer
            // time without changing the UI or controller execution paths.
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;

            cfg.spi_3wire = true;
            cfg.use_lock = true;

            cfg.dma_channel = SPI_DMA_CH_AUTO;

            cfg.pin_sclk = 10;
            cfg.pin_io0 = 11;
            cfg.pin_io1 = 12;
            cfg.pin_io2 = 13;
            cfg.pin_io3 = 14;

            _bus.config(cfg);
            _panel.setBus(&_bus);
        }

        {
            auto cfg = _panel.config();

            // Waveshare swapped LCD_CS and IMU_INT1 on the V2 PCB.
            // V1: LCD_CS=GPIO9, V2: LCD_CS=GPIO46.
            #if defined(OPENDRIFT_AMOLED_V2)
            cfg.pin_cs = 46;
            #else
            cfg.pin_cs = 9;
            #endif
            cfg.pin_rst = 21;
            cfg.pin_busy = -1;

            cfg.panel_width = 280;
            cfg.panel_height = 456;

            cfg.memory_width = 480;
            cfg.memory_height = 480;

            cfg.offset_x = 0;
            cfg.offset_y = 0;

            cfg.offset_rotation = 0;

            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;

            cfg.readable = false;

            cfg.invert = false;
            cfg.rgb_order = false;

            cfg.dlen_16bit = false;

            cfg.bus_shared = false;

            _panel.config(cfg);
        }

        setPanel(&_panel);
    }
};

#else

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

#endif
