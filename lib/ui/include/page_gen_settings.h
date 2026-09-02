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
    void setSubCategory(uint8_t cat) { _subCategory = (cat % 5); }
    void nextSubCategory() { _subCategory = (_subCategory + 1) % 5; }
    void prevSubCategory() { _subCategory = (_subCategory > 0) ? (_subCategory - 1) : 4; }

    bool isFocusSubNav() const { return _focusSubNav; }
    void setFocusSubNav(bool focus) { _focusSubNav = focus; }

    uint8_t getItemCountForCategory(uint8_t cat) const {
        if (cat == 0) return 5; // Cranking (5 items)
        if (cat == 1) return 3; // Fix Encoder (3 items: Step, Min, Max)
        if (cat == 2) return 3; // Potensio (3 items: Step/Quantization, Min, Max)
        if (cat == 3) return 4; // Auto Sweep (4 items: Step, Min, Max, Rate)
        return 1;               // Hardware: Polarity (1 item)
    }

private:
    LovyanGFX* _gfx;
    uint8_t  _subCategory{0}; // 0: Cranking, 1: Fix Enc, 2: Potensio, 3: Sweep, 4: Hardware
    bool     _focusSubNav{true}; // true: cursor is on category bar, false: cursor is on items
    uint8_t  _lastSubCategory{0xFF};
    bool     _lastFocusSubNav{false};
    uint8_t  _lastEditRow{0xFF};
    uint32_t _lastRpmStep{0};
    uint32_t _lastEncMin{0};
    uint32_t _lastEncMax{0};
    uint32_t _lastPotStep{0};
    uint32_t _lastPotMin{0};
    uint32_t _lastPotMax{0};
    uint32_t _lastSweepStep{0};
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
