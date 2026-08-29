#include "page_main_hub.h"

namespace EcuUi {

static constexpr uint8_t TOTAL_MODULES = 4;
static constexpr uint8_t VISIBLE_CARDS = 3;

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

    uint8_t topIndex = 0;
    if (selectedIndex >= VISIBLE_CARDS) {
        topIndex = selectedIndex - VISIBLE_CARDS + 1;
    }

    bool needsCardRedraw = fullRedraw || (topIndex != _lastTopIndex) || (selectedIndex != _lastSelectedIndex);

    if (fullRedraw) {
        _gfx->fillScreen(TFT_BLACK);
        _gfx->fillRect(0, 0, 480, 42, 0x0841);
        _gfx->drawFastHLine(0, 42, 480, 0x03E0);
        _gfx->setTextColor(TFT_WHITE, 0x0841);
        _gfx->setTextSize(2);
        _gfx->drawString("ECUSniff — MASTER HUB", 16, 12);

        _gfx->drawFastHLine(0, 290, 480, 0x03E0);
        _gfx->setTextColor(0x07FF, TFT_BLACK);
        _gfx->setTextSize(1);
        _gfx->drawString("Putar / Joy-Y: Pilih Modul  |  Klik: Masuk Modul", 70, 302);
    }

    if (needsCardRedraw) {
        for (uint8_t slot = 0; slot < VISIBLE_CARDS; ++slot) {
            uint8_t modIdx = topIndex + slot;
            if (modIdx < TOTAL_MODULES) {
                _drawCard(slot, modIdx, modIdx == selectedIndex);
            }
        }

        // Scroll indicators
        _gfx->setTextColor(topIndex > 0 ? 0xFFE0 : 0x31A6, 0x0841);
        _gfx->setTextSize(2);
        _gfx->drawString(topIndex > 0 ? "^" : " ", 455, 12);

        _gfx->setTextColor((topIndex + VISIBLE_CARDS < TOTAL_MODULES) ? 0xFFE0 : TFT_BLACK, TFT_BLACK);
        _gfx->setTextSize(2);
        _gfx->drawString((topIndex + VISIBLE_CARDS < TOTAL_MODULES) ? "v" : " ", 455, 296);

        _lastSelectedIndex = selectedIndex;
        _lastTopIndex = topIndex;
    }
}

void PageMainHub::_drawCard(uint8_t slotIndex, uint8_t moduleIndex, bool isSelected) {
    int32_t y = 48 + (slotIndex * 78);
    uint32_t bgColor = isSelected ? 0x1165 : 0x10A2;
    uint32_t borderColor = isSelected ? 0xFFE0 : 0x52AA;
    uint32_t descColor = isSelected ? 0xFFE0 : 0x07FF;

    _gfx->fillRoundRect(12, y - 2, 456, 74, 6, TFT_BLACK);
    _gfx->fillRoundRect(14, y, 452, 70, 6, bgColor);
    _gfx->drawRoundRect(14, y, 452, 70, 6, borderColor);
    if (isSelected) {
        _gfx->drawRoundRect(13, y - 1, 454, 72, 7, 0xFFE0);
    }

    _gfx->setTextColor(TFT_WHITE, bgColor);
    _gfx->setTextSize(2);
    _gfx->drawString(CARD_TITLES[moduleIndex], 26, y + 14);

    _gfx->setTextColor(descColor, bgColor);
    _gfx->setTextSize(1);
    _gfx->drawString(CARD_DESCS[moduleIndex], 26, y + 42);
}

void PageMainHub::onEncoderTurn(int32_t delta, uint8_t& selectedIndex) {
    int32_t next = static_cast<int32_t>(selectedIndex) + (delta > 0 ? 1 : -1);
    if (next < 0) next = TOTAL_MODULES - 1;
    if (next >= TOTAL_MODULES) next = 0;
    selectedIndex = static_cast<uint8_t>(next);
}

} // namespace EcuUi
