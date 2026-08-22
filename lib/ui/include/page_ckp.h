#pragma once
#include <LovyanGFX.hpp>
#include "parametric_pattern.h"

namespace EcuUi {

class PageCkp {
public:
    explicit PageCkp(LovyanGFX* gfx);
    void render(const EcuEngine::ParametricWheel& wheel, 
                bool isEditMode, uint8_t selectedItem, bool fullRedraw);

private:
    LovyanGFX* _gfx;
    uint8_t _lastDrawnItem{0xFF};
    bool    _lastEditMode{false};
    EcuEngine::ParametricWheel _lastWheel{};

    void _renderRow(uint8_t idx, const EcuEngine::ParametricWheel& wheel, bool isSelected);
};

} // namespace EcuUi
