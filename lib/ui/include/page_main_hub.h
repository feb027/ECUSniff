#pragma once
#include <LovyanGFX.hpp>

namespace EcuUi {

class PageMainHub {
public:
    explicit PageMainHub(LovyanGFX* gfx);

    void render(bool fullRedraw, uint8_t selectedIndex);
    void onEncoderTurn(int32_t delta, uint8_t& selectedIndex);

private:
    LovyanGFX* _gfx;
    uint8_t    _lastSelectedIndex{0xFF};

    void _drawCard(uint8_t moduleIndex, bool isSelected);
};

} // namespace EcuUi
