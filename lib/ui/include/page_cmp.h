#pragma once
#include <LovyanGFX.hpp>
#include "parametric_pattern.h"
#include "../../engine/include/wheel_database.h"

namespace EcuUi {

class PageCmp {
public:
    explicit PageCmp(LovyanGFX* gfx);
    void render(uint8_t activePresetIdx, const EcuEngine::CamEventTable& cam, 
                bool isEditMode, uint8_t selectedItem, bool fullRedraw);

private:
    LovyanGFX* _gfx;
    uint8_t _lastDrawnItem{0xFF};
    uint8_t _lastPresetIdx{0xFF};

    void _renderRow(uint8_t idx, const char* label, bool isSelected);
};

} // namespace EcuUi
