#include "display_driver.h"
#include <Arduino.h>

namespace EcuHal {

LGFX_TFT_4_0::LGFX_TFT_4_0() {
    {
        auto cfg = _bus_instance.config();
        cfg.spi_host    = SPI2_HOST;         // Kompatibel penuh ESP32 & ESP32-S3
        cfg.spi_mode    = 0;
        cfg.freq_write  = 40000000;          // 40 MHz ultra-fast SPI untuk ILI9488
        cfg.freq_read   = 16000000;
        cfg.spi_3wire   = false;
        cfg.use_lock    = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = PinConfig::TFT_SCK;   // GPIO 19
        cfg.pin_mosi = PinConfig::TFT_MOSI;  // GPIO 13
        cfg.pin_miso = PinConfig::TFT_MISO;  // -1
        cfg.pin_dc   = PinConfig::TFT_DC;    // GPIO 33

        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
    }

    {
        auto cfg = _panel_instance.config();
        cfg.pin_cs           = PinConfig::TFT_CS;   // GPIO 32
        cfg.pin_rst          = PinConfig::TFT_RST;  // GPIO 17
        cfg.pin_busy         = -1;
        cfg.memory_width     = 320;
        cfg.memory_height    = 480;
        cfg.panel_width      = 320;
        cfg.panel_height     = 480;
        cfg.offset_x         = 0;
        cfg.offset_y         = 0;
        cfg.offset_rotation  = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits  = 1;
        cfg.readable         = false;
        cfg.invert           = false;
        cfg.rgb_order        = false;
        cfg.dlen_16bit       = false;
        cfg.bus_shared       = false;

        _panel_instance.config(cfg);
    }

    {
        auto cfg = _light_instance.config();
        cfg.pin_bl = PinConfig::TFT_LED;  // GPIO 16
        cfg.invert = false;
        cfg.freq   = 44100;
        cfg.pwm_channel = 7;

        _light_instance.config(cfg);
        _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
}

DisplayDriver::DisplayDriver() {}

bool DisplayDriver::init() {
    Serial.println("[DISPLAY] Initializing 4.0\" ILI9488 TFT...");

    pinMode(PinConfig::TFT_RST, OUTPUT);
    digitalWrite(PinConfig::TFT_RST, HIGH);
    delay(20);
    digitalWrite(PinConfig::TFT_RST, LOW);
    delay(50);
    digitalWrite(PinConfig::TFT_RST, HIGH);
    delay(150);

    if (!_gfx.init()) {
        Serial.println("[DISPLAY] ILI9488 init FAILED!");
        return false;
    }

    _gfx.setRotation(1); // Landscape 480x320
    _gfx.setColorDepth(16);
    _gfx.fillScreen(TFT_BLACK);

    pinMode(PinConfig::TFT_LED, OUTPUT);
    digitalWrite(PinConfig::TFT_LED, HIGH);
    setBacklight(255);

    drawStaticLayout();
    Serial.println("[DISPLAY] ILI9488 Display initialized & layout drawn.");
    return true;
}

void DisplayDriver::drawStaticLayout() {
    _gfx.startWrite();
    _gfx.fillScreen(TFT_BLACK);

    // Header Bar
    _gfx.fillRect(0, 0, 480, 42, 0x0A2D); // Navy Header
    _gfx.setTextSize(2);
    _gfx.setTextColor(TFT_WHITE, 0x0A2D);
    _gfx.drawString("AUTOMOTIVE ECU TEST PLATFORM", 40, 12);

    // RPM Box Container
    _gfx.fillRoundRect(20, 55, 440, 68, 8, 0x18E3);
    _gfx.drawRoundRect(20, 55, 440, 68, 8, TFT_DARKGRAY);
    _gfx.setTextSize(1);
    _gfx.setTextColor(TFT_LIGHTGRAY, 0x18E3);
    _gfx.drawString("TARGET RPM (0 - 720 DEG CYCLE)", 35, 63);

    // Pattern Box Container
    _gfx.fillRoundRect(20, 132, 440, 52, 8, 0x18E3);
    _gfx.drawRoundRect(20, 132, 440, 52, 8, TFT_DARKGRAY);
    _gfx.setTextSize(2);
    _gfx.setTextColor(TFT_WHITE, 0x18E3);
    _gfx.drawString("CKP: 36-1 Parametric (GPIO 25)", 35, 148);

    // Footer WiFi Status
    _gfx.fillRect(0, 280, 480, 40, 0x0841);
    _gfx.setTextSize(2);
    _gfx.setTextColor(TFT_CYAN, 0x0841);
    _gfx.drawString("WiFi: ECU-Test-Platform (192.168.4.1)", 35, 290);
    _gfx.endWrite();
}

void DisplayDriver::setBacklight(uint8_t brightness) {
    _gfx.setBrightness(brightness);
}

LGFX_TFT_4_0& DisplayDriver::getGfx() {
    return _gfx;
}

} // namespace EcuHal
