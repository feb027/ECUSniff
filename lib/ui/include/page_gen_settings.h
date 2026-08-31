#pragma once
#include <LovyanGFX.hpp>
#include "engine_types.h"
#include "parametric_pattern.h"

namespace EcuUi {

class PageGenSettings {
public:
    explicit PageGenSettings(LovyanGFX* gfx);

    void init();
    void render(bool fullRedraw, uint8_t editRow,
                const EcuEngine::EngineRuntimeState& state,
                const EcuEngine::ParametricWheel& wheel);

    void onEncoderTurn(int32_t delta, uint8_t editRow,
                       EcuEngine::EngineRuntimeState& state,
                       EcuEngine::ParametricWheel& wheel);

    void onEncoderClick(uint8_t editRow,
                        EcuEngine::EngineRuntimeState& state,
                        EcuEngine::ParametricWheel& wheel);

private:
    LovyanGFX* _gfx;
    uint8_t  _lastEditRow{0xFF};
    uint32_t _lastRpmStep{0};
    uint32_t _lastSweepMin{0};
    uint32_t _lastSweepMax{0};
    uint32_t _lastSweepRate{0};
    uint32_t _lastCrankRpm{0};
    bool     _lastInverted{false};

    void _drawPanel(int32_t x, int32_t y, int32_t w, int32_t h, bool isSel);
};

} // namespace EcuUi
