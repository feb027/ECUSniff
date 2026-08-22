#pragma once
#include <LovyanGFX.hpp>
#include "capture_driver.h"
#include "signal_sniffer.h"
#include "waveform_canvas.h"

namespace EcuUi {

class PageCapture {
public:
    explicit PageCapture(LovyanGFX* gfx);

    void init(EcuHal::CaptureDriver* driver, EcuEngine::SignalSniffer* sniffer);
    void render(uint8_t subTab, bool fullRedraw, bool isEditMode);
    void onEncoderTurn(int32_t delta);
    void onEncoderClick(uint8_t subTab);
    void onEncoderDoubleClick(EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam);

    bool hasDecodedPattern() const { return _lastResult.success; }
    const EcuEngine::SnifferResult& getLastResult() const { return _lastResult; }

private:
    LovyanGFX*                 _gfx;
    WaveformCanvas             _canvas;
    EcuHal::CaptureDriver*     _driver{nullptr};
    EcuEngine::SignalSniffer*  _sniffer{nullptr};
    EcuEngine::SnifferResult   _lastResult{};

    uint8_t  _lastDrawnState{0xFF};
    uint32_t _lastDrawnRpm{0xFFFFFFFF};

    void _processCaptureData();
    void _renderLiveTab(bool fullRedraw);
    void _renderDataTab(bool fullRedraw);
    void _renderCamTab(bool fullRedraw);
};

} // namespace EcuUi
