#pragma once
#include <LovyanGFX.hpp>
#include "engine_types.h"
#include "capture_driver.h"
#include "signal_sniffer.h"
#include "joystick_driver.h"
#include "page_main_hub.h"
#include "page_dashboard.h"
#include "page_ckp.h"
#include "page_cmp.h"
#include "page_capture.h"
#include "page_eps_tester.h"
#include "page_speedo_tester.h"
#include "eps_controller.h"
#include "speedo_controller.h"

namespace EcuUi {

enum class UiLevel : uint8_t {
    MainHub = 0,
    Generator = 1,
    Capture = 2,
    EpsTester = 3,
    SpeedoTester = 4
};

class MenuManager {
public:
    explicit MenuManager(LovyanGFX* gfx);

    void init(EcuHal::CaptureDriver* capDriver, EcuEngine::SignalSniffer* sniffer,
              EcuEngine::EpsController* eps, EcuEngine::SpeedoController* speedo);
    void render(const EcuEngine::EngineRuntimeState& state,
                const EcuEngine::ParametricWheel& wheel,
                const EcuEngine::CamEventTable& cam);

    void onEncoderTurn(int32_t delta,
                       EcuEngine::EngineRuntimeState& state,
                       EcuEngine::ParametricWheel& wheel,
                       EcuEngine::CamEventTable& cam);

    void onJoystickAction(EcuHal::JoyAction action,
                          EcuEngine::EngineRuntimeState& state,
                          EcuEngine::ParametricWheel& wheel,
                          EcuEngine::CamEventTable& cam);

    void onEncoderClick();
    void onEncoderDoubleClick(EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam);
    void returnToMainHub();

    uint8_t getUiLevel() const { return static_cast<uint8_t>(_uiLevel); }
    uint8_t getActiveTab() const { return _genTab; }
    const PageCapture& getPageCapture() const { return _pageCapture; }
    PageEpsTester& getPageEps() { return _pageEps; }
    PageSpeedoTester& getPageSpeedo() { return _pageSpeedo; }

    void setUiLevel(UiLevel level);
    void setGenTab(uint8_t tab);
    void markNeedsRedraw() { _needsFullRedraw = true; }

private:
    LovyanGFX* _gfx;
    UiLevel _uiLevel{UiLevel::MainHub};
    uint8_t _hubIndex{0};

    uint8_t _genTab{1};       // 0: < MENU, 1: DASH/COCKPIT, 2: CKP/CAL, 3: CMP/HW
    uint8_t _lastTab{0xFF};
    bool    _isEditMode{false};
    uint8_t _editRow{0};
    bool    _focusTabBar{false};
    bool    _needsFullRedraw{true};

    uint8_t _lastDrawnTab{0xFF};
    bool    _lastDrawnEditMode{false};
    bool    _lastDrawnFocusTabBar{false};

    PageMainHub      _pageHub;
    PageDashboard    _pageDash;
    PageCkp          _pageCkp;
    PageCmp          _pageCmp;
    PageCapture      _pageCapture;
    PageEpsTester    _pageEps;
    PageSpeedoTester _pageSpeedo;

    EcuEngine::EpsController*    _epsController{nullptr};
    EcuEngine::SpeedoController* _speedoController{nullptr};

    void _drawGeneratorTabBar(bool force);
};

} // namespace EcuUi
