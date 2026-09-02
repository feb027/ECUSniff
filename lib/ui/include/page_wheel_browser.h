#pragma once
#include <LovyanGFX.hpp>
#include "engine_types.h"
#include "joystick_driver.h"
#include "parametric_pattern.h"
#include "waveform_canvas.h"
#include "wheel_database.h"

namespace EcuUi {

// WheelCategory aliased to BrandCategory for clean UI compatibility
using WheelCategory = BrandCategory;

class PageWheelBrowser {
public:
    explicit PageWheelBrowser(LovyanGFX* gfx);

    void init();
    void open(uint16_t initialGlobalIdx = 0, BrandCategory initialCategory = BrandCategory::ALL);
    void close();
    bool isOpen() const { return _isOpen; }

    void render(bool fullRedraw);
    void onEncoderTurn(int32_t delta);
    void onJoystickAction(EcuHal::JoyAction action);
    bool onEncoderClick(EcuEngine::ParametricWheel& outWheel,
                        EcuEngine::CamEventTable& outCam,
                        char* outName, size_t maxNameLen);

    uint16_t getSelectedGlobalIndex() const;
    BrandCategory getSelectedCategory() const { return _category; }

    static bool matchesCategory(uint16_t globalIdx, BrandCategory cat);

private:
    LovyanGFX*     _gfx;
    WaveformCanvas _canvas;
    bool           _isOpen{false};
    BrandCategory  _category{BrandCategory::ALL};
    uint16_t       _filteredIndices[128]{};
    uint16_t       _filteredCount{0};
    int16_t        _cursorIdx{0};
    int16_t        _lastCursorIdx{-1};
    uint8_t        _lastCategory{0xFF};
    bool           _needsListRedraw{true};

    void _buildFilteredList();
    void _drawHeader();
    void _drawList(bool forceAll);
    void _drawPreview();
};

} // namespace EcuUi
