#pragma once
#include <LovyanGFX.hpp>
#include "speedo_controller.h"
#include "joystick_driver.h"

namespace EcuUi {

class PageSpeedoTester {
public:
    explicit PageSpeedoTester(LovyanGFX* gfx);

    void init();
    void render(uint8_t currentTab, bool fullRedraw, uint8_t editRow,
                const EcuEngine::SpeedoController& controller);

    void onEncoderTurn(uint8_t currentTab, int32_t delta, uint8_t editRow,
                       EcuEngine::SpeedoController& controller);

    void onEncoderClick(uint8_t currentTab, uint8_t editRow,
                        EcuEngine::SpeedoController& controller);

private:
    LovyanGFX* _gfx;
    uint8_t    _lastTab{0xFF};
    uint8_t    _lastEditRow{0xFF};

    void _renderTabCockpit(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller);
    void _renderTabCalibration(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller);
    void _renderTabHardware(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller);
    void _drawRowFrame(int32_t x, int32_t y, int32_t w, int32_t h, bool isSelected);
};

} // namespace EcuUi
