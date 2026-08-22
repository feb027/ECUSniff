#pragma once
#include <LovyanGFX.hpp>
#include "engine_types.h"
#include "parametric_pattern.h"
#include "waveform_canvas.h"

namespace EcuUi {

struct WheelPresetItem {
    const char* name;
    uint16_t totalTeeth;
    uint8_t  missingTeeth;
    uint8_t  missingPosition;
    float    dutyCycle;
    bool     inverted;
    uint8_t  camCount;
    float    camAngles[4];
    bool     camHighs[4];
};

class PageDashboard {
public:
    explicit PageDashboard(LovyanGFX* gfx);

    void init();
    void render(bool fullRedraw, bool isEditMode, uint8_t editRow,
                const EcuEngine::EngineRuntimeState& state,
                const EcuEngine::ParametricWheel& wheel,
                const EcuEngine::CamEventTable& cam);

    void onEncoderTurn(int32_t delta, uint8_t editRow,
                       EcuEngine::EngineRuntimeState& state,
                       EcuEngine::ParametricWheel& wheel,
                       EcuEngine::CamEventTable& cam);

    static const WheelPresetItem PRESETS[];
    static constexpr size_t PRESET_COUNT = 6;

private:
    LovyanGFX*     _gfx;
    WaveformCanvas _canvas;

    uint32_t _lastRpm{0xFFFFFFFF};
    uint8_t  _lastMode{0xFF};
    bool     _lastIsRunning{false};
    bool     _lastIsEditMode{false};
    uint8_t  _lastEditRow{0xFF};
    int8_t   _activePresetIdx{0};
    uint16_t _lastTotalTeeth{0xFFFF};
    uint8_t  _lastMissingTeeth{0xFF};

    void _drawEditFrames(bool isEditMode, uint8_t editRow, bool isRunning, const EcuEngine::ParametricWheel& wheel);
    void _applyPreset(uint8_t idx, EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam);
};

} // namespace EcuUi
