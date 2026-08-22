#include "page_cmp.h"

namespace EcuUi {

PageCmp::PageCmp(LovyanGFX* gfx) : _gfx(gfx) {}

void PageCmp::_renderRow(uint8_t idx, const char* label, bool isSelected) {
    int32_t y = 86 + (idx * 54);
    uint16_t bg = isSelected ? 0x0965 : 0x0841;
    uint16_t border = isSelected ? 0x07E0 : 0x52AA;
    uint16_t textCol = isSelected ? TFT_WHITE : 0xCE79;

    _gfx->fillRoundRect(18, y - 4, 444, 44, 5, bg);
    _gfx->drawRoundRect(18, y - 4, 444, 44, 5, border);
    if (isSelected) {
        _gfx->drawRoundRect(17, y - 5, 446, 46, 6, 0x07E0);
    }

    _gfx->setTextSize(2);
    _gfx->setTextColor(textCol, bg);
    _gfx->drawString(label, 32, y + 8);
}

void PageCmp::render(const EcuEngine::CamEventTable& cam, 
                     bool isEditMode, uint8_t selectedItem, bool fullRedraw) {
    const char* events[] = {
        "Event 1: 120.0 deg -> HIGH",
        "Event 2: 180.0 deg -> LOW ",
        "Event 3: 420.0 deg -> HIGH",
        "Event 4: 470.0 deg -> LOW "
    };

    if (fullRedraw) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        _gfx->setTextSize(2);
        _gfx->setTextColor(0x07E0, 0x10A2);
        _gfx->drawString("TABEL SUDUT CAMSHAFT (CMP)", 24, 54);

        for (uint8_t i = 0; i < 4; ++i) {
            _renderRow(i, events[i], isEditMode && (selectedItem == i));
        }
        _lastDrawnItem = selectedItem;
        return;
    }

    if (_lastDrawnItem != selectedItem) {
        for (uint8_t i = 0; i < 4; ++i) {
            bool isSel = isEditMode && (selectedItem == i);
            _renderRow(i, events[i], isSel);
        }
        _lastDrawnItem = selectedItem;
    }
}

} // namespace EcuUi
