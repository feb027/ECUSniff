#include "page_main_hub.h"

namespace EcuUi {

static const char* CARD_TITLES[] = {
    "1. ENGINE SIGNAL GENERATOR",
    "2. SIGNAL CAPTURE / SNIFFER",
    "3. SENSOR SIMULATOR (EXPANSION)"
};

static const char* CARD_DESCS[] = {
    "Pembangkit Sinyal CKP, CMP, CMP2 & Mode FIX/CRANK/SWEEP",
    "Perekam Sinyal Mobil, Auto-Detect Gigi & Cam Angle",
    "Modul Ekspansi: VSS, Analog TPS/MAP & Resistif Temp"
};

void PageMainHub::render(bool fullRedraw, uint8_t selectedIndex) {
    if (fullRedraw) {
        _gfx->fillScreen(TFT_BLACK);
        // Header
        _gfx->fillRect(0, 0, 480, 42, 0x0841);
        _gfx->drawFastHLine(0, 42, 480, 0x03E0);
        _gfx->setTextColor(TFT_WHITE, 0x0841);
        _gfx->setTextSize(2);
        _gfx->drawString("ECUSniff — TEST PLATFORM", 20, 12);

        // Footer helper (High Contrast Bright Cyan)
        _gfx->drawFastHLine(0, 285, 480, 0x03E0);
        _gfx->setTextColor(0x07FF, TFT_BLACK);
        _gfx->setTextSize(1);
        _gfx->drawString("Putar Knob: Pilih Modul  |  Klik Knob: Masuk Modul", 80, 298);

        for (uint8_t i = 0; i < 3; ++i) {
            _drawModuleCard(i, CARD_TITLES[i], CARD_DESCS[i], selectedIndex == i);
        }
        _lastSelectedIndex = selectedIndex;
        return;
    }

    // Delta-only update: Only redraw if selection changed
    if (selectedIndex != _lastSelectedIndex) {
        if (_lastSelectedIndex < 3) {
            _drawModuleCard(_lastSelectedIndex, CARD_TITLES[_lastSelectedIndex], CARD_DESCS[_lastSelectedIndex], false);
        }
        _drawModuleCard(selectedIndex, CARD_TITLES[selectedIndex], CARD_DESCS[selectedIndex], true);
        _lastSelectedIndex = selectedIndex;
    }
}

void PageMainHub::_drawModuleCard(uint8_t index, const char* title, const char* desc, bool isSelected) {
    int32_t y = 56 + (index * 74);
    uint32_t bgColor = isSelected ? 0x0965 : 0x10A2;
    uint32_t borderColor = isSelected ? 0x07E0 : 0x52AA;
    uint32_t titleColor = TFT_WHITE;
    uint32_t descColor = isSelected ? 0xFFE0 : 0x07FF; // Bright Yellow when selected, Bright Cyan when idle

    _gfx->fillRoundRect(16, y, 448, 64, 8, bgColor);
    _gfx->drawRoundRect(16, y, 448, 64, 8, borderColor);
    if (isSelected) {
        _gfx->drawRoundRect(15, y - 1, 450, 66, 9, 0x07E0);
    }

    _gfx->setTextColor(titleColor, bgColor);
    _gfx->setTextSize(2);
    _gfx->drawString(title, 32, y + 12);

    _gfx->setTextColor(descColor, bgColor);
    _gfx->setTextSize(1);
    _gfx->drawString(desc, 32, y + 38);
}

void PageMainHub::onEncoderTurn(int32_t delta, uint8_t& selectedIndex) {
    int32_t next = static_cast<int32_t>(selectedIndex) + (delta > 0 ? 1 : -1);
    if (next < 0) next = 2;
    if (next > 2) next = 0;
    selectedIndex = static_cast<uint8_t>(next);
}

} // namespace EcuUi
