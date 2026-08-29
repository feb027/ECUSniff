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

    uint8_t _lastTab{0xFF};
    uint8_t _lastEditRow{0xFF};
    int32_t _lastKmh{-1};
    int32_t _lastRpm{-1};
    int32_t _lastTemp{-1};
    int32_t _lastFuel{-1};
    float   _lastHzKmh{-1.0f};
    float   _lastHzRpm{-1.0f};
    float   _lastVoltTemp{-1.0f};
    float   _lastVoltFuel{-1.0f};
    bool    _lastRunning{false};
    uint8_t _lastMode{0xFF};
    bool    _lastEnKmh{false};
    bool    _lastEnRpm{false};
    bool    _lastEnTemp{false};
    bool    _lastEnFuel{false};

    // Tab 2 & 3 Caches
    int32_t _lastCalVals[6]{-99, -99, -99, -99, -99, -99};
    float   _lastPpk{-1.0f};
    float   _lastPpr{-1.0f};
    uint8_t _lastRouting{0xFF};
    uint8_t _lastCurve{0xFF};
    float   _lastSweepTime{-1.0f};
    bool    _lastDac1{false};
    bool    _lastDac2{false};

    void _renderTabCockpit(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller);
    void _renderTabCalibration(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller);
    void _renderTabHardware(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller);
    void _drawPanelFrame(int32_t x, int32_t y, int32_t w, int32_t h, bool isSelected);
};

} // namespace EcuUi
