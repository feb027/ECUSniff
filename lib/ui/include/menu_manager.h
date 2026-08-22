#pragma once
#include <LovyanGFX.hpp>
#include "engine_types.h"
#include "capture_driver.h"
#include "signal_sniffer.h"
#include "page_main_hub.h"
#include "page_dashboard.h"
#include "page_ckp.h"
#include "page_cmp.h"
#include "page_capture.h"

namespace EcuUi {

enum class UiLevel : uint8_t {
    MainHub = 0,
    Generator = 1,
    Capture = 2
};

class MenuManager {
public:
    explicit MenuManager(LovyanGFX* gfx);

    void init(EcuHal::CaptureDriver* capDriver, EcuEngine::SignalSniffer* sniffer);
    void render(const EcuEngine::EngineRuntimeState& state,
                const EcuEngine::ParametricWheel& wheel,
                const EcuEngine::CamEventTable& cam);

    void onEncoderTurn(int32_t delta,
                       EcuEngine::EngineRuntimeState& state,
                       EcuEngine::ParametricWheel& wheel,
                       EcuEngine::CamEventTable& cam);

    void onEncoderClick();
    void onEncoderDoubleClick(EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam);
    void returnToMainHub();

    uint8_t getUiLevel() const { return static_cast<uint8_t>(_uiLevel); }
    uint8_t getActiveTab() const { return _genTab; }
    const PageCapture& getPageCapture() const { return _pageCapture; }

    void setUiLevel(UiLevel level);
    void setGenTab(uint8_t tab);
    void markNeedsRedraw() { _needsFullRedraw = true; }

private:
    LovyanGFX* _gfx;
    UiLevel _uiLevel{UiLevel::MainHub};
    uint8_t _hubIndex{0};

    uint8_t _genTab{1};       // 0: < MENU, 1: DASH, 2: CKP, 3: CMP
    uint8_t _lastTab{0xFF};
    bool    _isEditMode{false};
    uint8_t _editRow{0};
    bool    _needsFullRedraw{true};

    uint8_t _lastDrawnTab{0xFF};
    bool    _lastDrawnEditMode{false};

    PageMainHub   _pageHub;
    PageDashboard _pageDash;
    PageCkp       _pageCkp;
    PageCmp       _pageCmp;
    PageCapture   _pageCapture;

    void _drawGeneratorTabBar(bool force);
};

} // namespace EcuUi
