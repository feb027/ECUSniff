#include "menu_manager.h"

namespace EcuUi {

MenuManager::MenuManager(LovyanGFX* gfx)
    : _gfx(gfx), _pageHub(gfx), _pageDash(gfx), _pageCkp(gfx), _pageCmp(gfx),
      _pageGenSettings(gfx), _pageCapture(gfx), _pageEps(gfx), _pageSpeedo(gfx), _pageBrowser(gfx) {}

void MenuManager::init(EcuHal::CaptureDriver* capDriver, EcuEngine::SignalSniffer* sniffer,
                       EcuEngine::EpsController* eps, EcuEngine::SpeedoController* speedo) {
    _epsController = eps; _speedoController = speedo;
    _pageDash.init(); _pageGenSettings.init(); _pageCapture.init(capDriver, sniffer);
    _pageEps.init(); _pageSpeedo.init(); _pageBrowser.init();
    _uiLevel = UiLevel::MainHub; _hubIndex = 0; _genTab = 1;
    _focusTabBar = false; _needsFullRedraw = true;
}

void MenuManager::returnToMainHub() {
    _uiLevel = UiLevel::MainHub; _needsFullRedraw = true;
    _lastTab = 0xFF; _lastDrawnTab = 0xFF; _isEditMode = false; _focusTabBar = false;
    _pageBrowser.close();
}

void MenuManager::setUiLevel(UiLevel level) {
    if (_uiLevel != level) {
        _uiLevel = level; _genTab = 1; _editRow = 0;
        _isEditMode = false; _focusTabBar = false; _needsFullRedraw = true;
        _pageBrowser.close();
    }
}

void MenuManager::setGenTab(uint8_t tab) {
    if (_genTab != tab) { _genTab = tab; _needsFullRedraw = true; }
}

void MenuManager::_drawGeneratorTabBar(bool force) {
    if (_genTab == _lastDrawnTab && _focusTabBar == _lastDrawnFocusTabBar && !force) return;

    _gfx->fillRect(0, 0, 480, 40, 0x0841);
    _gfx->drawFastHLine(0, 40, 480, 0x52AA);

    const char* tabsGen[]    = { "< MENU", "DASHBOARD", "CKP WHEEL", "CMP CAM", "SETTINGS" };
    const char* tabsCap[]    = { "< MENU", "LIVE CAPTURE", "DECODE DATA", "CAM EVENTS" };
    const char* tabsEps[]    = { "< MENU", "EPS BENCH", "OEM PRESET", "AUTO SWEEP" };
    const char* tabsSpeedo[] = { "< MENU", "SPEEDO COCKPIT", "3-PT CAL", "HARDWARE" };

    uint8_t totalTabs = (_uiLevel == UiLevel::Generator) ? 5 : 4;
    const char** tabs = tabsGen;
    if (_uiLevel == UiLevel::Capture) tabs = tabsCap;
    else if (_uiLevel == UiLevel::EpsTester) tabs = tabsEps;
    else if (_uiLevel == UiLevel::SpeedoTester) tabs = tabsSpeedo;

    for (uint8_t i = 0; i < totalTabs; ++i) {
        int32_t w = (totalTabs == 5) ? 88 : 110;
        int32_t x = (totalTabs == 5) ? (6 + (i * 94)) : (8 + (i * 118));
        bool isActive = (_genTab == i);

        if (isActive) {
            uint32_t bg = (_genTab == 0) ? 0xF800 : 0x07E0;
            _gfx->fillRoundRect(x, 4, w, 32, 6, bg);
            if (_focusTabBar || _genTab == 0) {
                _gfx->drawRoundRect(x, 4, w, 32, 6, 0xFFE0);
                _gfx->drawRoundRect(x + 1, 5, w - 2, 30, 5, 0xFFE0);
            }
            _gfx->setTextColor((_genTab == 0) ? TFT_WHITE : TFT_BLACK, bg);
        } else {
            _gfx->fillRoundRect(x, 4, w, 32, 6, 0x18C3);
            _gfx->drawRoundRect(x, 4, w, 32, 6, 0x52AA);
            _gfx->setTextColor(TFT_WHITE, 0x18C3);
        }
        _gfx->setTextSize(1);
        int32_t tw = _gfx->textWidth(tabs[i]);
        _gfx->drawString(tabs[i], x + (w - tw) / 2, 14);
    }
    _lastDrawnTab = _genTab; _lastDrawnFocusTabBar = _focusTabBar;
}

void MenuManager::render(const EcuEngine::EngineRuntimeState& state,
                         const EcuEngine::ParametricWheel& wheel,
                         const EcuEngine::CamEventTable& cam) {
    if (_pageBrowser.isOpen()) {
        _pageBrowser.render(_needsFullRedraw); _needsFullRedraw = false; return;
    }
    if (_uiLevel == UiLevel::MainHub) {
        _pageHub.render(_needsFullRedraw, _hubIndex); _needsFullRedraw = false; return;
    }

    bool isRedraw = _needsFullRedraw || (_genTab != _lastTab);
    _drawGeneratorTabBar(isRedraw);

    if (_genTab == 0) {
        if (isRedraw) {
            _gfx->fillRect(8, 44, 464, 268, 0x10A2);
            _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);
            _gfx->fillRoundRect(40, 100, 400, 130, 8, 0x0841);
            _gfx->drawRoundRect(40, 100, 400, 130, 8, 0xF800);
            _gfx->setTextColor(0xF800, 0x0841); _gfx->setTextSize(2);
            _gfx->drawCenterString("< KELUAR KE MENU UTAMA", 240, 125);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawCenterString("Tekan / Klik Knob untuk Keluar", 240, 165);
            _gfx->setTextColor(0x07E0, 0x0841);
            _gfx->drawCenterString("Geser Joystick ke Kanan (>) untuk Batal", 240, 190);
        }
        _lastTab = _genTab; _needsFullRedraw = false; return;
    }

    uint8_t activeEditRow = _focusTabBar ? 255 : _editRow;

    if (_uiLevel == UiLevel::EpsTester) {
        if (_epsController) _pageEps.render(isRedraw, false, activeEditRow, *_epsController);
    } else if (_uiLevel == UiLevel::SpeedoTester) {
        if (_speedoController) _pageSpeedo.render(_genTab, isRedraw, activeEditRow, *_speedoController);
    } else if (_uiLevel == UiLevel::Generator) {
        switch (_genTab) {
            case 1: _pageDash.render(isRedraw, false, activeEditRow, state, wheel, cam); break;
            case 2: _pageCkp.render(wheel, false, activeEditRow, isRedraw); break;
            case 3: _pageCmp.render(cam, false, activeEditRow, isRedraw); break;
            case 4: _pageGenSettings.render(isRedraw, activeEditRow, state, wheel); break;
        }
    } else if (_uiLevel == UiLevel::Capture) {
        _pageCapture.render(_genTab, isRedraw, false);
    }
    _lastTab = _genTab; _needsFullRedraw = false;
}

void MenuManager::onEncoderTurn(int32_t delta,
                               EcuEngine::EngineRuntimeState& state,
                               EcuEngine::ParametricWheel& wheel,
                               EcuEngine::CamEventTable& cam) {
    if (_pageBrowser.isOpen()) { _pageBrowser.onEncoderTurn(delta); return; }
    if (_uiLevel == UiLevel::MainHub) { _pageHub.onEncoderTurn(delta, _hubIndex); return; }
    uint8_t maxTabs = (_uiLevel == UiLevel::Generator) ? 4 : 3;
    if (_focusTabBar) { _genTab = constrain((int32_t)_genTab + delta, 0, (int32_t)maxTabs); _needsFullRedraw = true; return; }

    if (_uiLevel == UiLevel::EpsTester && _epsController) _pageEps.onEncoderTurn(delta, _editRow, *_epsController);
    else if (_uiLevel == UiLevel::SpeedoTester && _speedoController) _pageSpeedo.onEncoderTurn(_genTab, delta, _editRow, *_speedoController);
    else if (_uiLevel == UiLevel::Generator) {
        if (_genTab == 1) _pageDash.onEncoderTurn(delta, _editRow, state, wheel, cam);
        else if (_genTab == 2) {
            if (_editRow == 0) wheel.totalTeeth = constrain((int32_t)wheel.totalTeeth + delta, 1, 360);
            else if (_editRow == 1) wheel.missingTeeth = constrain((int32_t)wheel.missingTeeth + delta, 0, 4);
            else if (_editRow == 2) wheel.missingPosition = constrain((int32_t)wheel.missingPosition + delta, 0, (int)wheel.totalTeeth);
            else if (_editRow == 3) wheel.dutyCycle = constrain(wheel.dutyCycle + (delta * 0.05f), 0.10f, 0.90f);
            else if (_editRow == 4 && delta != 0) wheel.inverted = !wheel.inverted;
        } else if (_genTab == 3) {
            uint8_t count = cam.getEventCount();
            if (_editRow < count && count > 0) {
                EcuEngine::CmpEvent tempEvents[16]; const auto* evs = cam.getEvents();
                for (uint8_t i = 0; i < count; ++i) tempEvents[i] = evs[i];
                tempEvents[_editRow].angleDeg = constrain(tempEvents[_editRow].angleDeg + (delta * 5.0f), 0.0f, 720.0f);
                cam.clear();
                for (uint8_t i = 0; i < count; ++i) cam.addEvent(tempEvents[i].angleDeg, tempEvents[i].levelHigh);
            }
        } else if (_genTab == 4) {
            _pageGenSettings.onEncoderTurn(delta, _editRow, state, wheel);
        }
    }
}

void MenuManager::onJoystickAction(EcuHal::JoyAction action,
                                  EcuEngine::EngineRuntimeState& state,
                                  EcuEngine::ParametricWheel& wheel,
                                  EcuEngine::CamEventTable& cam) {
    if (action == EcuHal::JoyAction::None) return;

    if (_pageBrowser.isOpen()) {
        if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
        else _pageBrowser.onJoystickAction(action);
        return;
    }
    if (_uiLevel == UiLevel::MainHub) {
        if (action == EcuHal::JoyAction::Up || action == EcuHal::JoyAction::Left) _hubIndex = (_hubIndex > 0) ? (_hubIndex - 1) : 3;
        else if (action == EcuHal::JoyAction::Down || action == EcuHal::JoyAction::Right) _hubIndex = (_hubIndex + 1) % 4;
        else if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
        return;
    }
    if (_genTab == 0) {
        if (action == EcuHal::JoyAction::Right) { _genTab = 1; _editRow = 0; _focusTabBar = false; _needsFullRedraw = true; }
        else if (action == EcuHal::JoyAction::Click) returnToMainHub();
        return;
    }
    uint8_t maxTabs = (_uiLevel == UiLevel::Generator) ? 4 : 3;
    if (_focusTabBar) {
        if (action == EcuHal::JoyAction::Left) { if (_genTab > 0) { _genTab--; _needsFullRedraw = true; } }
        else if (action == EcuHal::JoyAction::Right) { if (_genTab < maxTabs) { _genTab++; _needsFullRedraw = true; } }
        else if (action == EcuHal::JoyAction::Down) { if (_genTab > 0) { _focusTabBar = false; _editRow = 0; _needsFullRedraw = true; } }
        else if (action == EcuHal::JoyAction::Click) { if (_genTab == 0) returnToMainHub(); else { _focusTabBar = false; _editRow = 0; _needsFullRedraw = true; } }
        return;
    }

    if (_uiLevel == UiLevel::SpeedoTester) {
        if (_genTab == 1) {
            if (action == EcuHal::JoyAction::Up) { if (_editRow <= 1) { _focusTabBar = true; _needsFullRedraw = true; } else if (_editRow <= 3) _editRow -= 2; else _editRow = 2; }
            else if (action == EcuHal::JoyAction::Down) { if (_editRow <= 1) _editRow += 2; else if (_editRow <= 3) _editRow = 4; }
            else if (action == EcuHal::JoyAction::Left) { if (_editRow == 1 || _editRow == 3) _editRow--; }
            else if (action == EcuHal::JoyAction::Right) { if (_editRow == 0 || _editRow == 2) _editRow++; }
            else if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
            return;
        } else if (_genTab == 2) {
            if (action == EcuHal::JoyAction::Up) { if (_editRow == 0 || _editRow == 3) { _focusTabBar = true; _needsFullRedraw = true; } else if (_editRow % 3 > 0) _editRow--; }
            else if (action == EcuHal::JoyAction::Down) { if (_editRow % 3 < 2) _editRow++; }
            else if (action == EcuHal::JoyAction::Left) { if (_editRow >= 3) _editRow -= 3; }
            else if (action == EcuHal::JoyAction::Right) { if (_editRow < 3) _editRow += 3; }
            else if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
            return;
        } else if (_genTab == 3) {
            if (action == EcuHal::JoyAction::Up) { if (_editRow <= 1) { _focusTabBar = true; _needsFullRedraw = true; } else _editRow -= 2; }
            else if (action == EcuHal::JoyAction::Down) { if (_editRow < 2) _editRow += 2; }
            else if (action == EcuHal::JoyAction::Left) { if (_editRow % 2 == 1) _editRow--; }
            else if (action == EcuHal::JoyAction::Right) { if (_editRow % 2 == 0) _editRow++; }
            else if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
            return;
        }
    }

    if (_uiLevel == UiLevel::EpsTester) {
        if (action == EcuHal::JoyAction::Up) { if (_editRow == 0) { _focusTabBar = true; _needsFullRedraw = true; } else _editRow--; }
        else if (action == EcuHal::JoyAction::Down) _editRow = (_editRow + 1) % 5;
        else if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
        return;
    }

    if (_uiLevel == UiLevel::Generator) {
        if (action == EcuHal::JoyAction::Up) {
            if (_editRow == 0) { _focusTabBar = true; _needsFullRedraw = true; }
            else _editRow--;
        } else if (action == EcuHal::JoyAction::Down) {
            if (_genTab == 1 && _editRow < 2) _editRow++;
            else if (_genTab == 2 && _editRow < 4) _editRow++;
            else if (_genTab == 3 && _editRow < 3) _editRow++;
            else if (_genTab == 4 && _editRow < 5) _editRow++;
        } else if (action == EcuHal::JoyAction::Left && _genTab > 0) {
            _genTab--; _editRow = 0; _needsFullRedraw = true;
        } else if (action == EcuHal::JoyAction::Right && _genTab < 4) {
            _genTab++; _editRow = 0; _needsFullRedraw = true;
        } else if (action == EcuHal::JoyAction::Click) {
            onEncoderClick(state, wheel, cam);
        }
        return;
    }

    if (_uiLevel == UiLevel::Capture && action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
}

void MenuManager::onEncoderClick(EcuEngine::EngineRuntimeState& state,
                                EcuEngine::ParametricWheel& wheel,
                                EcuEngine::CamEventTable& cam) {
    if (_pageBrowser.isOpen()) {
        if (_pageBrowser.onEncoderClick(wheel, cam, state.activeWheelName, sizeof(state.activeWheelName))) {
            _pageDash.setActivePresetIdx(_pageBrowser.getSelectedGlobalIndex());
            _needsFullRedraw = true;
        }
        return;
    }
    if (_uiLevel == UiLevel::MainHub) {
        if (_hubIndex == 0) { _uiLevel = UiLevel::Generator; _genTab = 1; }
        else if (_hubIndex == 1) { _uiLevel = UiLevel::Capture; _genTab = 1; }
        else if (_hubIndex == 2) { _uiLevel = UiLevel::EpsTester; _genTab = 1; _editRow = 0; }
        else if (_hubIndex == 3) { _uiLevel = UiLevel::SpeedoTester; _genTab = 1; _editRow = 0; }
        _focusTabBar = false; _needsFullRedraw = true; return;
    }
    if (_genTab == 0) { returnToMainHub(); return; }
    if (_uiLevel == UiLevel::EpsTester) {
        if (_editRow == 4 && _epsController) _epsController->setAutoSweep(!_epsController->getConfig().autoSweep);
        else if (_epsController) _epsController->toggleRunning();
        return;
    }
    if (_uiLevel == UiLevel::SpeedoTester) {
        if (_speedoController) _pageSpeedo.onEncoderClick(_genTab, _editRow, *_speedoController);
        return;
    }
    if (_uiLevel == UiLevel::Capture) { _pageCapture.onEncoderClick(_genTab); return; }
    if (_uiLevel == UiLevel::Generator) {
        if (_genTab == 1) {
            if (_editRow == 2) { _pageBrowser.open(_pageDash.getActivePresetIdx()); _needsFullRedraw = true; return; }
            state.isRunning = !state.isRunning; _needsFullRedraw = true;
        } else if (_genTab == 4) {
            _pageGenSettings.onEncoderClick(_editRow, state, wheel);
            _needsFullRedraw = true;
        }
    }
}

void MenuManager::onEncoderDoubleClick(EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam) {
    if (_pageBrowser.isOpen()) { _pageBrowser.close(); _needsFullRedraw = true; return; }
    if (_uiLevel == UiLevel::Capture) { _pageCapture.onEncoderDoubleClick(wheel, cam); return; }
    if (_uiLevel == UiLevel::EpsTester) { if (_epsController) _epsController->toggleRunning(); return; }
    if (_uiLevel == UiLevel::SpeedoTester) { if (_speedoController) _speedoController->toggleRunning(); return; }
    returnToMainHub();
}

} // namespace EcuUi
