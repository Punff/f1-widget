#pragma once
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7796 _panel;
    lgfx::Bus_SPI _bus;

public:
    LGFX(void)
    {
        // ================= SPI BUS =================
        {
            auto cfg = _bus.config();

            cfg.spi_host = VSPI_HOST;

            cfg.pin_sclk = 14;
            cfg.pin_mosi = 13;
            cfg.pin_miso = 12;
            cfg.pin_dc = 2;

            cfg.freq_write = 27000000;

            _bus.config(cfg);
            _panel.setBus(&_bus);
        }

        // ================= PANEL =================
        {
            auto cfg = _panel.config();

            cfg.pin_cs = 15;
            cfg.pin_rst = 33;

            cfg.panel_width = 320;
            cfg.panel_height = 480;

            cfg.offset_x = 0;
            cfg.offset_y = 0;

            cfg.invert = false;
            cfg.rgb_order = false;

            _panel.config(cfg);
        }

        setPanel(&_panel);
    }
};