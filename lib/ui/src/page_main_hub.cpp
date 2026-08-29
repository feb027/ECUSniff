#include "page_main_hub.h"

namespace EcuUi {

static constexpr uint8_t TOTAL_MODULES = 4;

static const char* CARD_TITLES[TOTAL_MODULES] = {
    "1. ENGINE SIGNAL GENERATOR",
    "2. SIGNAL CAPTURE / SNIFFER",
    "3. EPS & VSS BENCH TESTER",
    "4. SPEEDOMETER CLUSTER TESTER"
};

static const char* CARD_DESCS[TOTAL_MODULES] = {
    "Pembangkit Sinyal Mobil: CKP & CMP (FIX/CRANK/SWEEP)",
    "Perekam & Penganalisis Sinyal ECU Mobil 720-deg",
    "Simulasi EPS: VSS Speed, RPM Tach, Steer TRQ & Sweep",
    "Tester Speedo, Tacho, Suhu ECT, Bensin & Sweep"
};

PageMainHub::PageMainHub(LovyanGFX* gfx) : _gfx(gfx) {}

void PageMainHub::render(bool fullRedraw, uint8_t selectedIndex) {
    if (selectedIndex >= TOTAL_MODULES) selectedIndex = 0;

    if (fullRedraw) {
        _gfx->fillScreen(TFT_BLACK);
        _gfx->fillRect(0, 0, 480, 42, 0x0841);
        _gfx->drawFastHLine(0, 42, 480, 0x03E0);
        _gfx->setTextColor(TFT_WHITE, 0x0841);
        _gfx->setTextSize(2);
        _gfx->drawString("ECUSniff — MASTER HUB", 16, 12);

        _gfx->drawFastHLine(0, 286, 480, 0x03E0);
        _gfx->fillRect(0, 287, 480, 33, TFT_BLACK);
        _gfx->setTextColor(0x07FF, TFT_BLACK);
        _gfx->setTextSize(1);
        _gfx->drawString("Putar / Joy-Y: Pilih Modul  |  Klik: Masuk Modul", 70, 298);

        for (uint8_t i = 0; i < TOTAL_MODULES; ++i) {
            _drawCard(i, i == selectedIndex);
        }
        _lastSelectedIndex = selectedIndex;
        return;
    }

    if (selectedIndex != _lastSelectedIndex) {
        if (_lastSelectedIndex < TOTAL_MODULES) {
            _drawCard(_lastSelectedIndex, false);
        }
        _drawCard(selectedIndex, true);
        _lastSelectedIndex = selectedIndex;
    }
}

void PageMainHub::_drawCard(uint8_t moduleIndex, bool isSelected) {
    if (moduleIndex >= TOTAL_MODULES) return;
    int32_t y = 48 + (moduleIndex * 58);
    uint32_t bgColor = isSelected ? 0x18C3 : 0x0841;
    uint32_t borderColor = isSelected ? 0xFFE0 : 0x31A6;
    uint32_t titleColor = isSelected ? 0xFFE0 : TFT_WHITE;
    uint32_t descColor = isSelected ? 0x07FF : 0x52AA;

    _gfx->fillRoundRect(12, y, 456, 52, 6, bgColor);
    _gfx->drawRoundRect(12, y, 456, 52, 6, borderColor);
    if (isSelected) {
        _gfx->drawRoundRect(11, y - 1, 458, 54, 7, 0xFFE0);
    }

    _gfx->setTextColor(titleColor, bgColor);
    _gfx->setTextSize(2);
    _gfx->drawString(CARD_TITLES[moduleIndex], 24, y + 8);

    _gfx->setTextColor(descColor, bgColor);
    _gfx->setTextSize(1);
    _gfx->drawString(CARD_DESCS[moduleIndex], 24, y + 32);
}

void PageMainHub::onEncoderTurn(int32_t delta, uint8_t& selectedIndex) {
    int32_t next = static_cast<int32_t>(selectedIndex) + (delta > 0 ? 1 : -1);
    if (next < 0) next = TOTAL_MODULES - 1;
    if (next >= TOTAL_MODULES) next = 0;
    selectedIndex = static_cast<uint8_t>(next);
}

} // namespace EcuUi
