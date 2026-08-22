#pragma once
#include <LovyanGFX.hpp>
#include "parametric_pattern.h"

namespace EcuUi {

class PageCmp {
public:
    PageCmp(LovyanGFX* gfx);
    void render(const EcuEngine::CamEventTable& cam, 
                bool isEditMode, uint8_t selectedItem, bool fullRedraw);

private:
    LovyanGFX* _gfx;
    uint8_t _lastDrawnItem{0xFF};

    void _renderRow(uint8_t idx, const char* label, bool isSelected);
};

} // namespace EcuUi
