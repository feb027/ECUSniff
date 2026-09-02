#pragma once
#include <LovyanGFX.hpp>
#include "power_cycle_controller.h"

namespace EcuUi {

class PagePowerCycle {
public:
    explicit PagePowerCycle(LovyanGFX* gfx);

    void init();
    void render(bool fullRedraw, uint8_t editRow,
                const EcuEngine::PowerCycleConfig& config,
                const EcuEngine::PowerCycleState& state,
                bool mcpFound);

    void onEncoderTurn(int32_t delta, uint8_t editRow,
                       EcuEngine::PowerCycleConfig& config);
    void onEncoderClick(uint8_t editRow,
                        EcuEngine::PowerCycleController& controller);

    static constexpr uint8_t ROW_ON_TIME     = 0;
    static constexpr uint8_t ROW_OFF_TIME    = 1;
    static constexpr uint8_t ROW_TARGET      = 2;
    static constexpr uint8_t ROW_GEN_PULSE   = 3;
    static constexpr uint8_t ROW_START_STOP  = 4;
    static constexpr uint8_t ROW_RESET_STATS = 5;

private:
    LovyanGFX* _gfx;

    uint32_t _lastOnMs{0};
    uint32_t _lastOffMs{0};
    uint32_t _lastTarget{0};
    bool     _lastGenPulse{false};
    bool     _lastRunning{false};
    uint32_t _lastCycle{0xFFFFFFFF};
    bool     _lastIgsw{false};
    bool     _lastMrel{false};
    uint32_t _lastBootSuccess{0xFFFFFFFF};
    uint32_t _lastBootFail{0xFFFFFFFF};
    uint8_t  _lastEditRow{0xFF};

    void _drawPanel(int32_t x, int32_t y, int32_t w, int32_t h, bool selected);
};

} // namespace EcuUi
