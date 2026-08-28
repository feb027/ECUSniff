#pragma once
#include <LovyanGFX.hpp>
#include "eps_controller.h"
#include "joystick_driver.h"

namespace EcuUi {

class PageEpsTester {
public:
    explicit PageEpsTester(LovyanGFX* gfx);

    void init();
    void render(bool fullRedraw, bool isEditMode, uint8_t editRow,
                const EcuEngine::EpsController& controller);

    void onEncoderTurn(int32_t delta, uint8_t editRow,
                       EcuEngine::EpsController& controller);

    void onJoystickAction(EcuHal::JoyAction action,
                          EcuEngine::EpsController& controller);

    void onEncoderClick(EcuEngine::EpsController& controller);

private:
    LovyanGFX* _gfx;

    // Snapshot values for delta micro-rendering
    bool     _lastRunning{false};
    uint8_t  _lastPreset{0xFF};
    float    _lastSpeed{-1.0f};
    uint32_t _lastRpm{0xFFFFFFFF};
    float    _lastTorque{-99.0f};
    bool     _lastSweep{false};
    bool     _lastEditMode{false};
    uint8_t  _lastEditRow{0xFF};
    float    _lastVssFreq{-1.0f};
    float    _lastRpmFreq{-1.0f};

    void _drawStaticLayout();
    void _drawRowHighlight(uint8_t row, bool selected, bool isEditMode);
    void _renderValues(const EcuEngine::EpsController& controller);
};

} // namespace EcuUi
