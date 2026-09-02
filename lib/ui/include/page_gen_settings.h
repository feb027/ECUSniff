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

    uint8_t getSubCategory() const { return _subCategory; }
    void setSubCategory(uint8_t cat) { _subCategory = (cat % 3); }
    void nextSubCategory() { _subCategory = (_subCategory + 1) % 3; }
    void prevSubCategory() { _subCategory = (_subCategory > 0) ? (_subCategory - 1) : 2; }

    bool isFocusSubNav() const { return _focusSubNav; }
    void setFocusSubNav(bool focus) { _focusSubNav = focus; }

    uint8_t getItemCountForCategory(uint8_t cat) const {
        if (cat == 0) return 5; // Cranking: Crank RPM, Crank Duration, Spin-Up, Transition, Gradual Duration
        if (cat == 1) return 4; // Sweep: Step, Min, Max, Rate
        return 1;               // Hardware: Polarity
    }

private:
    LovyanGFX* _gfx;
    uint8_t  _subCategory{0}; // 0: Cranking, 1: Auto Sweep, 2: Hardware
    bool     _focusSubNav{true}; // true: cursor is on category bar, false: cursor is on items
    uint8_t  _lastSubCategory{0xFF};
    bool     _lastFocusSubNav{false};
    uint8_t  _lastEditRow{0xFF};
    uint32_t _lastRpmStep{0};
    uint32_t _lastSweepMin{0};
    uint32_t _lastSweepMax{0};
    uint32_t _lastSweepRate{0};
    uint32_t _lastCrankRpm{0};
    uint32_t _lastCrankDur{0};
    uint32_t _lastSpinUpDur{0};
    uint32_t _lastRampDur{0};
    bool     _lastFastFlare{false};
    bool     _lastInverted{false};

    void _drawSubNav(bool fullRedraw);
    void _drawPanel(int32_t x, int32_t y, int32_t w, int32_t h, bool isSel);
};

} // namespace EcuUi
