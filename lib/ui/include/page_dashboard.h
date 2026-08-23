#pragma once
#include <LovyanGFX.hpp>
#include "engine_types.h"
#include "parametric_pattern.h"
#include "waveform_canvas.h"

namespace EcuUi {

struct WheelPresetItem {
    char     name[32];
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

    static uint8_t addCapturedPreset(const char* name, const EcuEngine::ParametricWheel& wheel, const EcuEngine::CamEventTable& cam);
    static bool renameCustomPreset(uint8_t slot, const char* newName);
    static bool deleteCustomPreset(uint8_t slot);
    static uint8_t getCustomCount();
    static const WheelPresetItem* getCustomPreset(uint8_t slot);
    static void clearAllCustom();
    static void setCustomSlot(uint8_t slot, const WheelPresetItem& item);

    static constexpr size_t BASE_PRESET_COUNT = 64;
    static constexpr size_t MAX_CUSTOM_PRESETS = 16;

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

    static WheelPresetItem s_customSlots[MAX_CUSTOM_PRESETS];
    static uint8_t         s_customCount;

    void _drawEditFrames(bool isEditMode, uint8_t editRow, bool isRunning, const EcuEngine::ParametricWheel& wheel);
    void _applyPreset(uint8_t idx, EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam);
};

} // namespace EcuUi
