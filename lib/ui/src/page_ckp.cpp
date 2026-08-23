#include "page_ckp.h"

namespace EcuUi {

PageCkp::PageCkp(LovyanGFX* gfx) : _gfx(gfx) {}

void PageCkp::_renderRow(uint8_t idx, const EcuEngine::ParametricWheel& wheel, bool isSelected) {
    int32_t y = 80 + (idx * 45);
    uint16_t bg = isSelected ? 0x1165 : 0x0841;
    uint16_t border = isSelected ? 0xFFE0 : 0x52AA;
    uint16_t textCol = isSelected ? TFT_WHITE : 0xCE79;
    uint16_t valCol = isSelected ? 0x07E0 : 0x07FF;

    _gfx->fillRoundRect(18, y - 4, 444, 40, 5, bg);
    _gfx->drawRoundRect(18, y - 4, 444, 40, 5, border);
    if (isSelected) {
        _gfx->drawRoundRect(17, y - 5, 446, 42, 6, 0xFFE0);
    }

    _gfx->setTextSize(2);
    _gfx->setTextColor(textCol, bg);

    char valBuf[32];
    switch (idx) {
        case 0:
            _gfx->drawString("Total Gigi (N) :", 32, y + 6);
            snprintf(valBuf, sizeof(valBuf), "%-3u gigi", (unsigned)wheel.totalTeeth);
            _gfx->setTextColor(valCol, bg);
            _gfx->drawString(valBuf, 260, y + 6);
            break;
        case 1:
            _gfx->drawString("Missing Gap (M):", 32, y + 6);
            snprintf(valBuf, sizeof(valBuf), "%-3u gigi", (unsigned)wheel.missingTeeth);
            _gfx->setTextColor(valCol, bg);
            _gfx->drawString(valBuf, 260, y + 6);
            break;
        case 2:
            _gfx->drawString("Posisi Gap     :", 32, y + 6);
            snprintf(valBuf, sizeof(valBuf), "%-3u     ", (unsigned)wheel.missingPosition);
            _gfx->setTextColor(valCol, bg);
            _gfx->drawString(valBuf, 260, y + 6);
            break;
        case 3:
            _gfx->drawString("Duty Cycle     :", 32, y + 6);
            snprintf(valBuf, sizeof(valBuf), "%-3d %%   ", static_cast<int>(wheel.dutyCycle * 100));
            _gfx->setTextColor(valCol, bg);
            _gfx->drawString(valBuf, 260, y + 6);
            break;
        case 4:
            _gfx->drawString("Polaritas      :", 32, y + 6);
            _gfx->setTextColor(valCol, bg);
            _gfx->drawString(wheel.inverted ? "INVERTED" : "NORMAL  ", 260, y + 6);
            break;
    }
}

void PageCkp::render(const EcuEngine::ParametricWheel& wheel, 
                     bool isEditMode, uint8_t selectedItem, bool fullRedraw) {
    if (fullRedraw) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        _gfx->setTextSize(2);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->drawString("PENGATURAN CRANKSHAFT (CKP)", 24, 54);

        for (uint8_t i = 0; i < 5; ++i) {
            _renderRow(i, wheel, (selectedItem == i));
        }
        _lastDrawnItem = selectedItem;
        _lastEditMode = isEditMode;
        _lastWheel = wheel;
        return;
    }

    // Real-Time Value Detection
    bool wheelChanged = (_lastWheel.totalTeeth != wheel.totalTeeth ||
                         _lastWheel.missingTeeth != wheel.missingTeeth ||
                         _lastWheel.missingPosition != wheel.missingPosition ||
                         _lastWheel.dutyCycle != wheel.dutyCycle ||
                         _lastWheel.inverted != wheel.inverted);

    if (_lastEditMode != isEditMode || _lastDrawnItem != selectedItem || wheelChanged) {
        for (uint8_t i = 0; i < 5; ++i) {
            _renderRow(i, wheel, (selectedItem == i));
        }
        _lastDrawnItem = selectedItem;
        _lastEditMode = isEditMode;
        _lastWheel = wheel;
    }
}

} // namespace EcuUi
