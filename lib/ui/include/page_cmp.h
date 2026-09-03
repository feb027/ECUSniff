#pragma once
#include <LovyanGFX.hpp>
#include "engine_types.h"
#include "parametric_pattern.h"
#include "waveform_canvas.h"
#include "../../engine/include/wheel_database.h"

namespace EcuUi {

class PageCmp {
public:
    explicit PageCmp(LovyanGFX* gfx);
    void init();
    void render(uint8_t activePresetIdx, 
                const EcuEngine::EngineRuntimeState& state,
                const EcuEngine::ParametricWheel& wheel,
                const EcuEngine::CamEventTable& cam, 
                bool isEditMode, uint8_t selectedItem, bool fullRedraw);

    void onEncoderTurn(int32_t delta, uint8_t selectedItem, EcuEngine::EngineRuntimeState& state);
    void onEncoderClick(uint8_t selectedItem, EcuEngine::EngineRuntimeState& state);

private:
    LovyanGFX*     _gfx;
    WaveformCanvas _canvas;
    uint8_t        _lastDrawnItem{0xFF};
    uint8_t        _lastPresetIdx{0xFF};
    int8_t         _lastVvtAdv{0x7F};
    uint32_t       _lastRpm{0xFFFFFFFF};
    bool           _lastVvtEnabled{false};

    void _drawPanel(int32_t x, int32_t y, int32_t w, int32_t h, bool isSelected);
};

} // namespace EcuUi
