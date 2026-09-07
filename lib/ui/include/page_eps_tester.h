#pragma once
#include <LovyanGFX.hpp>
#include "eps_controller.h"
#include "joystick_driver.h"

namespace EcuUi {

class PageEpsTester {
public:
    explicit PageEpsTester(LovyanGFX* gfx);

    void init();
    void render(uint8_t currentTab, bool fullRedraw, uint8_t editRow,
                const EcuEngine::EpsController& controller);

    void onEncoderTurn(uint8_t currentTab, int32_t delta, uint8_t editRow,
                       EcuEngine::EpsController& controller);

    void onJoystickAction(uint8_t currentTab, EcuHal::JoyAction action,
                          EcuEngine::EpsController& controller);

    void onEncoderClick(uint8_t currentTab, uint8_t editRow,
                        EcuEngine::EpsController& controller);

private:
    LovyanGFX* _gfx;

    uint8_t  _lastTab{0xFF};
    uint8_t  _lastEditRow{0xFF};

    // Tab 1 state tracking (Digital Bench Cockpit)
    bool     _lastRunning{false};
    float    _lastSpeed{-1.0f};
    uint32_t _lastRpm{0xFFFFFFFF};
    float    _lastTorque{-99.0f};
    int32_t  _lastTrq1Mv{-99999};
    int32_t  _lastTrq2Mv{-99999};
    int32_t  _lastTrq1FbMv{-99999};
    int32_t  _lastTrq2FbMv{-99999};
    int32_t  _lastDelta1Mv{-99999};
    int32_t  _lastDelta2Mv{-99999};
    bool     _lastAdcFound{false};
    bool     _lastSweep{false};
    float    _lastVssFreq{-1.0f};
    float    _lastRpmFreq{-1.0f};

    // Tab 2 state tracking (Calibration & Setup)
    uint8_t  _lastPreset{0xFF};
    float    _lastCenterVolt{-1.0f};
    float    _lastSpanVolt{-1.0f};
    float    _lastVssPulses{-1.0f};
    uint8_t  _lastRpmPulses{0xFF};
    float    _lastT1Scale{-1.0f};
    float    _lastT1Offset{-99.0f};
    float    _lastT2Scale{-1.0f};
    float    _lastT2Offset{-99.0f};
    int32_t  _lastT1FbPreviewMv{-99999};
    int32_t  _lastT2FbPreviewMv{-99999};
    int32_t  _lastT1TargetMv{-99999};
    int32_t  _lastT2TargetMv{-99999};
    uint32_t _savedNoticeUntilMs{0};
    bool     _lastSavedNoticeActive{false};

    void _drawStaticLayoutTab1(const EcuEngine::EpsController& controller);
    void _renderValuesTab1(const EcuEngine::EpsController& controller);
    void _drawBoxBorderTab1(uint8_t boxIdx, bool selected);

    void _drawStaticLayoutTab2();
    void _renderValuesTab2(const EcuEngine::EpsController& controller);
    void _drawRowHighlightTab2(uint8_t row, bool selected);
};

} // namespace EcuUi
