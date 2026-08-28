#include "page_main_hub.h"

namespace EcuUi {

static const char* CARD_TITLES[] = {
    "1. ENGINE SIGNAL GENERATOR",
    "2. SIGNAL CAPTURE / SNIFFER",
    "3. EPS & VSS BENCH TESTER"
};

static const char* CARD_DESCS[] = {
    "Pembangkit Sinyal Mobil: CKP & CMP (FIX/CRANK/SWEEP)",
    "Perekam & Penganalisis Sinyal ECU Mobil 720-deg",
    "Simulasi EPS: VSS Speed, RPM Tach, Steer TRQ & Sweep"
};

void PageMainHub::render(bool fullRedraw, uint8_t selectedIndex) {
    if (fullRedraw) {
        _gfx->fillScreen(TFT_BLACK);
        _gfx->fillRect(0, 0, 480, 42, 0x0841);
        _gfx->drawFastHLine(0, 42, 480, 0x03E0);
        _gfx->setTextColor(TFT_WHITE, 0x0841);
        _gfx->setTextSize(2);
        _gfx->drawString("ECUSniff — CAR TEST PLATFORM", 20, 12);

        _gfx->drawFastHLine(0, 285, 480, 0x03E0);
        _gfx->setTextColor(0x07FF, TFT_BLACK);
        _gfx->setTextSize(1);
        _gfx->drawString("Putar Knob / Joystick: Pilih Modul  |  Klik: Masuk Modul", 60, 298);

        for (uint8_t i = 0; i < 3; ++i) {
            _drawModuleCard(i, CARD_TITLES[i], CARD_DESCS[i], selectedIndex == i);
        }
        _lastSelectedIndex = selectedIndex;
        return;
    }

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
    uint32_t bgColor = isSelected ? 0x1165 : 0x10A2;
    uint32_t borderColor = isSelected ? 0xFFE0 : 0x52AA;
    uint32_t descColor = isSelected ? 0xFFE0 : 0x07FF;

    // Erase bounding background cleanly
    _gfx->fillRoundRect(14, y - 2, 452, 68, 8, TFT_BLACK);

    _gfx->fillRoundRect(16, y, 448, 64, 6, bgColor);
    _gfx->drawRoundRect(16, y, 448, 64, 6, borderColor);
    if (isSelected) {
        _gfx->drawRoundRect(15, y - 1, 450, 66, 7, 0xFFE0);
    }

    _gfx->setTextColor(TFT_WHITE, bgColor);
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
