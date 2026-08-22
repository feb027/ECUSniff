#include "menu_manager.h"

namespace EcuUi {

static const char* GEN_TAB_NAMES[] = { "< MENU", "DASH", "CKP", "CMP" };
static const char* CAP_TAB_NAMES[] = { "< MENU", "LIVE", "DATA", "CAM" };

MenuManager::MenuManager(LovyanGFX* gfx)
    : _gfx(gfx),
      _pageHub(gfx),
      _pageDash(gfx),
      _pageCkp(gfx),
      _pageCmp(gfx),
      _pageCapture(gfx) {}

void MenuManager::init(EcuHal::CaptureDriver* capDriver, EcuEngine::SignalSniffer* sniffer) {
    _pageDash.init();
    _pageCapture.init(capDriver, sniffer);
    _needsFullRedraw = true;
}

void MenuManager::returnToMainHub() {
    _uiLevel = UiLevel::MainHub;
    _isEditMode = false;
    _needsFullRedraw = true;
    _lastTab = 0xFF;
    _lastDrawnTab = 0xFF;
}

void MenuManager::setUiLevel(UiLevel level) {
    if (_uiLevel != level) {
        _uiLevel = level;
        _isEditMode = false;
        _needsFullRedraw = true;
        _lastTab = 0xFF;
        _lastDrawnTab = 0xFF;
    }
}

void MenuManager::setGenTab(uint8_t tab) {
    if (_genTab != tab && tab <= 3) {
        _genTab = tab;
        _editRow = 0;
        _needsFullRedraw = true;
    }
}

void MenuManager::render(const EcuEngine::EngineRuntimeState& state,
                         const EcuEngine::ParametricWheel& wheel,
                         const EcuEngine::CamEventTable& cam) {
    if (_uiLevel == UiLevel::MainHub) {
        _pageHub.render(_needsFullRedraw, _hubIndex);
        _needsFullRedraw = false;
        return;
    }

    // 1. Draw Top Tab Bar
    if (_needsFullRedraw) {
        _gfx->fillScreen(TFT_BLACK);
        _drawGeneratorTabBar(true);
        _needsFullRedraw = false;
    } else {
        _drawGeneratorTabBar(false);
    }

    bool tabChanged = (_lastTab != _genTab);
    _lastTab = _genTab;

    // 2. Render Active Content Area
    if (_genTab == 0 && tabChanged) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        _gfx->fillRoundRect(24, 60, 432, 95, 8, 0x0965);
        _gfx->drawRoundRect(24, 60, 432, 95, 8, 0x07E0);
        _gfx->setTextColor(TFT_WHITE, 0x0965);
        _gfx->setTextSize(2);
        _gfx->drawString("KEMBALI KE MENU UTAMA", 42, 76);

        _gfx->setTextColor(0xFFE0, 0x0965);
        _gfx->setTextSize(1);
        _gfx->drawString("Klik tombol knob untuk keluar dan kembali", 42, 110);
        _gfx->drawString("ke Layar Pemilihan Instrumen Platform.", 42, 128);

        _gfx->fillRoundRect(24, 175, 432, 120, 8, 0x0841);
        _gfx->drawRoundRect(24, 175, 432, 120, 8, 0x52AA);
        _gfx->setTextColor(0x07FF, 0x0841);
        _gfx->setTextSize(2);
        _gfx->drawString("PINTASAN CEPAT", 42, 192);
        _gfx->setTextSize(1);
        _gfx->drawString("Tahan tombol knob selama 1.5 detik kapan saja", 42, 226);
        _gfx->drawString("untuk langsung kembali ke Menu Utama.", 42, 248);
        return;
    }

    if (_uiLevel == UiLevel::Generator) {
        switch (_genTab) {
            case 1: _pageDash.render(tabChanged, _isEditMode, _editRow, state, wheel, cam); break;
            case 2: _pageCkp.render(wheel, _isEditMode, _editRow, tabChanged); break;
            case 3: _pageCmp.render(cam, _isEditMode, _editRow, tabChanged); break;
            default: break;
        }
    } else if (_uiLevel == UiLevel::Capture) {
        _pageCapture.render(_genTab, tabChanged, _isEditMode);
    }
}

void MenuManager::_drawGeneratorTabBar(bool force) {
    if (!force && _lastDrawnTab == _genTab && _lastDrawnEditMode == _isEditMode) return;

    if (force) {
        _gfx->fillRect(0, 0, 480, 42, 0x0841);
        _gfx->drawFastHLine(0, 42, 480, 0x03E0);
    }

    const char** tabNames = (_uiLevel == UiLevel::Capture) ? CAP_TAB_NAMES : GEN_TAB_NAMES;

    for (uint8_t i = 0; i < 4; ++i) {
        bool isSel = (_genTab == i);
        bool wasSel = (_lastDrawnTab == i);
        if (force || isSel || wasSel || (_isEditMode != _lastDrawnEditMode)) {
            int32_t x = i * 120;
            uint32_t bg = isSel ? (_isEditMode ? 0xF800 : 0x07E0) : 0x0841;
            uint32_t fg = isSel ? TFT_BLACK : 0xCE79;

            _gfx->fillRoundRect(x + 3, 4, 114, 34, 4, bg);
            _gfx->setTextColor(fg, bg);
            _gfx->setTextSize(2);
            _gfx->drawCenterString(tabNames[i], x + 60, 12);
        }
    }
    _lastDrawnTab = _genTab;
    _lastDrawnEditMode = _isEditMode;
}

void MenuManager::onEncoderTurn(int32_t delta,
                                EcuEngine::EngineRuntimeState& state,
                                EcuEngine::ParametricWheel& wheel,
                                EcuEngine::CamEventTable& cam) {
    if (_uiLevel == UiLevel::MainHub) {
        _pageHub.onEncoderTurn(delta, _hubIndex);
        return;
    }

    if (!_isEditMode) {
        int32_t next = static_cast<int32_t>(_genTab) + (delta > 0 ? 1 : -1);
        if (next < 0) next = 3;
        if (next > 3) next = 0;
        _genTab = static_cast<uint8_t>(next);
        _editRow = 0;
    } else {
        if (_uiLevel == UiLevel::Generator) {
            if (_genTab == 1) {
                _pageDash.onEncoderTurn(delta, _editRow, state, wheel, cam);
            } else if (_genTab == 2) {
                if (_editRow == 0) {
                    int32_t t = (int32_t)wheel.totalTeeth + delta;
                    wheel.totalTeeth = constrain(t, 4, 120);
                } else if (_editRow == 1) {
                    int32_t m = (int32_t)wheel.missingTeeth + delta;
                    wheel.missingTeeth = constrain(m, 0, 4);
                } else if (_editRow == 2) {
                    int32_t p = (int32_t)wheel.missingPosition + delta;
                    wheel.missingPosition = constrain(p, 0, (int)wheel.totalTeeth);
                } else if (_editRow == 3) {
                    float d = wheel.dutyCycle + (delta * 0.05f);
                    wheel.dutyCycle = constrain(d, 0.10f, 0.90f);
                } else if (_editRow == 4) {
                    if (delta != 0) wheel.inverted = !wheel.inverted;
                }
            }
        }
    }
}

void MenuManager::onEncoderClick() {
    if (_uiLevel == UiLevel::MainHub) {
        if (_hubIndex == 0) {
            _uiLevel = UiLevel::Generator;
            _genTab = 1; // Start at DASH
        } else if (_hubIndex == 1) {
            _uiLevel = UiLevel::Capture;
            _genTab = 1; // Start at LIVE
        }
        _needsFullRedraw = true;
        _lastTab = 0xFF;
        _lastDrawnTab = 0xFF;
        return;
    }

    if (_genTab == 0) {
        returnToMainHub();
        return;
    }

    if (_uiLevel == UiLevel::Capture) {
        _pageCapture.onEncoderClick(_genTab);
        return;
    }

    _isEditMode = !_isEditMode;
}

void MenuManager::onEncoderDoubleClick(EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam) {
    if (_uiLevel == UiLevel::Capture) {
        _pageCapture.onEncoderDoubleClick(wheel, cam);
        return;
    }

    if (_isEditMode && _uiLevel == UiLevel::Generator) {
        if (_genTab == 1) _editRow = (_editRow + 1) % 3; // Row 0: RPM, Row 1: Mode, Row 2: Wheel Pattern!
        else if (_genTab == 2) _editRow = (_editRow + 1) % 5; // 5 CKP rows
        else if (_genTab == 3) _editRow = (_editRow + 1) % 4; // Events
    }
}

} // namespace EcuUi
