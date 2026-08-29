#pragma once
#include <LovyanGFX.hpp>
#include "engine_types.h"
#include "joystick_driver.h"
#include "parametric_pattern.h"
#include "waveform_canvas.h"

namespace EcuUi {

enum class WheelCategory : uint8_t {
    All = 0,
    ToyotaDaihatsu = 1,
    HondaSuzuki = 2,
    MitsuNissanMazda = 3,
    EuroAmerika = 4,
    Universal = 5,
    Custom = 6
};

class PageWheelBrowser {
public:
    explicit PageWheelBrowser(LovyanGFX* gfx);

    void init();
    void open(uint16_t initialGlobalIdx = 0);
    void close();
    bool isOpen() const { return _isOpen; }

    void render(bool fullRedraw);
    void onEncoderTurn(int32_t delta);
    void onJoystickAction(EcuHal::JoyAction action);
    bool onEncoderClick(EcuEngine::ParametricWheel& outWheel,
                        EcuEngine::CamEventTable& outCam,
                        char* outName, size_t maxNameLen);

    uint16_t getSelectedGlobalIndex() const;

private:
    LovyanGFX*     _gfx;
    WaveformCanvas _canvas;
    bool           _isOpen{false};
    WheelCategory  _category{WheelCategory::All};
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
    void _drawFooter();
    bool _matchesCategory(uint16_t globalIdx, WheelCategory cat);
};

} // namespace EcuUi
