#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "pin_config.h"

namespace EcuHal {

class LGFX_TFT_4_0 : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Light_PWM     _light_instance;

public:
    LGFX_TFT_4_0();
};

class DisplayDriver {
public:
    DisplayDriver();
    bool init();
    void drawStaticLayout();
    void setBacklight(uint8_t brightness);
    LGFX_TFT_4_0& getGfx();

private:
    LGFX_TFT_4_0 _gfx;
};

} // namespace EcuHal
