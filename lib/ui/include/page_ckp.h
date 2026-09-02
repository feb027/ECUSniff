#pragma once
#include <LovyanGFX.hpp>
#include "parametric_pattern.h"
#include "../../engine/include/wheel_database.h"

namespace EcuUi {

class PageCkp {
public:
    explicit PageCkp(LovyanGFX* gfx);
    void render(uint8_t activePresetIdx, const EcuEngine::ParametricWheel& wheel, 
                bool isEditMode, uint8_t selectedItem, bool fullRedraw);

private:
    LovyanGFX* _gfx;
    uint8_t _lastDrawnItem{0xFF};
    bool    _lastEditMode{false};
    uint8_t _lastPresetIdx{0xFF};
    EcuEngine::ParametricWheel _lastWheel{};

    void _renderRow(uint8_t idx, const char* label, const char* value, bool isSelected);
};

} // namespace EcuUi
