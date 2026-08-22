#pragma once
#include <LovyanGFX.hpp>

namespace EcuUi {

class PageMainHub {
public:
    explicit PageMainHub(LovyanGFX* gfx) : _gfx(gfx) {}

    void render(bool fullRedraw, uint8_t selectedIndex);
    void onEncoderTurn(int32_t delta, uint8_t& selectedIndex);

private:
    LovyanGFX* _gfx;
    uint8_t _lastSelectedIndex{0xFF};

    void _drawModuleCard(uint8_t index, const char* title, const char* desc, bool isSelected);
};

} // namespace EcuUi
