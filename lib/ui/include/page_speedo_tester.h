#pragma once
#include <LovyanGFX.hpp>
#include "speedo_controller.h"
#include "joystick_driver.h"

namespace EcuUi {

class PageSpeedoTester {
public:
    explicit PageSpeedoTester(LovyanGFX* gfx);

    void init();
    void render(bool fullRedraw, bool isEditMode, uint8_t editRow,
                const EcuEngine::SpeedoController& controller);

    void onEncoderTurn(int32_t delta, uint8_t editRow,
                       EcuEngine::SpeedoController& controller);

    void onJoystickAction(EcuHal::JoyAction action,
                          EcuEngine::SpeedoController& controller);

    void onEncoderClick(EcuEngine::SpeedoController& controller);

private:
    LovyanGFX* _gfx;

    bool     _lastRunning{false};
    float    _lastKmh{-1.0f};
    float    _lastRpm{-1.0f};
    float    _lastTemp{-1.0f};
    float    _lastFuel{-1.0f};
    float    _lastHzKmh{-1.0f};
    float    _lastHzRpm{-1.0f};
    bool     _lastSweep{false};
    bool     _lastEditMode{false};
    uint8_t  _lastEditRow{0xFF};
    bool     _lastDacFuel{false};
    bool     _lastDacTemp{false};

    void _drawStaticLayout();
    void _drawRowHighlight(uint8_t row, bool selected, bool isEditMode);
    void _renderValues(const EcuEngine::SpeedoController& controller);
};

} // namespace EcuUi
