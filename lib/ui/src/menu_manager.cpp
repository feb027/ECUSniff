#include "menu_manager.h"

namespace EcuUi {

MenuManager::MenuManager(LovyanGFX* gfx)
    : _gfx(gfx), _pageHub(gfx), _pageDash(gfx), _pageCkp(gfx), _pageCmp(gfx),
      _pageCapture(gfx), _pageEps(gfx), _pageSpeedo(gfx) {}

void MenuManager::init(EcuHal::CaptureDriver* capDriver, EcuEngine::SignalSniffer* sniffer,
                       EcuEngine::EpsController* eps, EcuEngine::SpeedoController* speedo) {
    _epsController = eps; _speedoController = speedo;
    _pageDash.init(); _pageCapture.init(capDriver, sniffer);
    _pageEps.init(); _pageSpeedo.init();
    _uiLevel = UiLevel::MainHub; _hubIndex = 0; _genTab = 1; _needsFullRedraw = true;
}

void MenuManager::returnToMainHub() {
    _uiLevel = UiLevel::MainHub; _needsFullRedraw = true;
    _lastTab = 0xFF; _lastDrawnTab = 0xFF; _isEditMode = false;
}

void MenuManager::setUiLevel(UiLevel level) {
    if (_uiLevel != level) {
        _uiLevel = level; _genTab = 1; _editRow = 0;
        _isEditMode = false; _needsFullRedraw = true;
    }
}

void MenuManager::setGenTab(uint8_t tab) {
    if (_genTab != tab) { _genTab = tab; _needsFullRedraw = true; }
}

void MenuManager::_drawGeneratorTabBar(bool force) {
    if (_genTab == _lastDrawnTab && _isEditMode == _lastDrawnEditMode && !force) return;

    _gfx->fillRect(0, 0, 480, 40, 0x0841);
    _gfx->drawFastHLine(0, 40, 480, 0x52AA);

    const char* tabsGen[]    = { "< MENU", "DASHBOARD", "CKP WHEEL", "CMP CAM" };
    const char* tabsCap[]    = { "< MENU", "LIVE CAPTURE", "DECODE DATA", "CAM EVENTS" };
    const char* tabsEps[]    = { "< MENU", "EPS BENCH", "OEM PRESET", "AUTO SWEEP" };
    const char* tabsSpeedo[] = { "< MENU", "SPEEDO COCKPIT", "3-PT CAL", "HARDWARE" };

    const char** tabs = tabsGen;
    if (_uiLevel == UiLevel::Capture) tabs = tabsCap;
    else if (_uiLevel == UiLevel::EpsTester) tabs = tabsEps;
    else if (_uiLevel == UiLevel::SpeedoTester) tabs = tabsSpeedo;

    for (uint8_t i = 0; i < 4; ++i) {
        int32_t x = 8 + (i * 118);
        bool isActive = (_genTab == i);

        if (isActive) {
            _gfx->fillRoundRect(x, 4, 110, 32, 6, _isEditMode ? 0xF800 : 0x07E0);
            _gfx->setTextColor(TFT_BLACK, _isEditMode ? 0xF800 : 0x07E0);
        } else {
            _gfx->fillRoundRect(x, 4, 110, 32, 6, 0x18C3);
            _gfx->drawRoundRect(x, 4, 110, 32, 6, 0x52AA);
            _gfx->setTextColor(TFT_WHITE, 0x18C3);
        }
        _gfx->setTextSize(1);
        int32_t tw = _gfx->textWidth(tabs[i]);
        _gfx->drawString(tabs[i], x + (110 - tw) / 2, 14);
    }
    _lastDrawnTab = _genTab; _lastDrawnEditMode = _isEditMode;
}

void MenuManager::render(const EcuEngine::EngineRuntimeState& state,
                         const EcuEngine::ParametricWheel& wheel,
                         const EcuEngine::CamEventTable& cam) {
    if (_uiLevel == UiLevel::MainHub) {
        _pageHub.render(_needsFullRedraw, _hubIndex); _needsFullRedraw = false; return;
    }

    bool isRedraw = _needsFullRedraw || (_genTab != _lastTab);
    _drawGeneratorTabBar(isRedraw);

    if (_uiLevel == UiLevel::EpsTester) {
        if (_epsController) _pageEps.render(isRedraw, _isEditMode, _editRow, *_epsController);
    } else if (_uiLevel == UiLevel::SpeedoTester) {
        if (_speedoController) _pageSpeedo.render(_genTab, isRedraw, _editRow, *_speedoController);
    } else if (_uiLevel == UiLevel::Generator) {
        switch (_genTab) {
            case 1: _pageDash.render(isRedraw, _isEditMode, _editRow, state, wheel, cam); break;
            case 2: _pageCkp.render(wheel, _isEditMode, _editRow, isRedraw); break;
            case 3: _pageCmp.render(cam, _isEditMode, _editRow, isRedraw); break;
        }
    } else if (_uiLevel == UiLevel::Capture) {
        _pageCapture.render(_genTab, isRedraw, _isEditMode);
    }
    _lastTab = _genTab; _needsFullRedraw = false;
}

void MenuManager::onEncoderTurn(int32_t delta,
                               EcuEngine::EngineRuntimeState& state,
                               EcuEngine::ParametricWheel& wheel,
                               EcuEngine::CamEventTable& cam) {
    if (_uiLevel == UiLevel::MainHub) { _pageHub.onEncoderTurn(delta, _hubIndex); return; }
    if (_uiLevel == UiLevel::EpsTester) {
        if (_epsController) _pageEps.onEncoderTurn(delta, _editRow, *_epsController);
        return;
    }
    if (_uiLevel == UiLevel::SpeedoTester) {
        if (_speedoController) _pageSpeedo.onEncoderTurn(_genTab, delta, _editRow, *_speedoController);
        return;
    }
    if (_uiLevel == UiLevel::Generator) {
        if (_genTab == 1) _pageDash.onEncoderTurn(delta, _editRow, state, wheel, cam);
        else if (_genTab == 2) {
            if (_editRow == 0) wheel.totalTeeth = constrain((int32_t)wheel.totalTeeth + delta, 4, 120);
            else if (_editRow == 1) wheel.missingTeeth = constrain((int32_t)wheel.missingTeeth + delta, 0, 4);
            else if (_editRow == 2) wheel.missingPosition = constrain((int32_t)wheel.missingPosition + delta, 0, (int)wheel.totalTeeth);
            else if (_editRow == 3) wheel.dutyCycle = constrain(wheel.dutyCycle + (delta * 0.05f), 0.10f, 0.90f);
            else if (_editRow == 4 && delta != 0) wheel.inverted = !wheel.inverted;
        } else if (_genTab == 3) {
            uint8_t count = cam.getEventCount();
            if (_editRow < count && count > 0) {
                EcuEngine::CmpEvent tempEvents[16]; const auto* evs = cam.getEvents();
                for (uint8_t i = 0; i < count; ++i) tempEvents[i] = evs[i];
                float newAngle = constrain(tempEvents[_editRow].angleDeg + (delta * 5.0f), 0.0f, 720.0f);
                tempEvents[_editRow].angleDeg = newAngle;
                cam.clear();
                for (uint8_t i = 0; i < count; ++i) cam.addEvent(tempEvents[i].angleDeg, tempEvents[i].levelHigh);
            }
        }
    }
}

void MenuManager::onJoystickAction(EcuHal::JoyAction action,
                                  EcuEngine::EngineRuntimeState& state,
                                  EcuEngine::ParametricWheel& wheel,
                                  EcuEngine::CamEventTable& cam) {
    if (action == EcuHal::JoyAction::None) return;

    if (_uiLevel == UiLevel::MainHub) {
        if (action == EcuHal::JoyAction::Up || action == EcuHal::JoyAction::Left) _hubIndex = (_hubIndex > 0) ? (_hubIndex - 1) : 3;
        else if (action == EcuHal::JoyAction::Down || action == EcuHal::JoyAction::Right) _hubIndex = (_hubIndex + 1) % 4;
        else if (action == EcuHal::JoyAction::Click) onEncoderClick();
        return;
    }

    if (_uiLevel == UiLevel::SpeedoTester) {
        if (_genTab == 0) {
            if (action == EcuHal::JoyAction::Right) { _genTab = 1; _editRow = 0; _needsFullRedraw = true; }
            else if (action == EcuHal::JoyAction::Click) returnToMainHub();
            return;
        }
        if (_genTab == 1) {
            if (action == EcuHal::JoyAction::Up) {
                if (_editRow == 2) _editRow = 0; else if (_editRow == 3) _editRow = 1;
                else if (_editRow == 4) _editRow = 2; else _editRow = 4;
            } else if (action == EcuHal::JoyAction::Down) {
                if (_editRow == 0) _editRow = 2; else if (_editRow == 1) _editRow = 3;
                else if (_editRow == 2 || _editRow == 3) _editRow = 4; else _editRow = 0;
            } else if (action == EcuHal::JoyAction::Left) {
                if (_editRow == 1) _editRow = 0; else if (_editRow == 3) _editRow = 2;
                else { _genTab = 0; _needsFullRedraw = true; }
            } else if (action == EcuHal::JoyAction::Right) {
                if (_editRow == 0) _editRow = 1; else if (_editRow == 2) _editRow = 3;
                else { _genTab = 2; _editRow = 0; _needsFullRedraw = true; }
            } else if (action == EcuHal::JoyAction::Click) onEncoderClick();
            return;
        } else if (_genTab == 2) {
            if (action == EcuHal::JoyAction::Up) { if (_editRow % 3 > 0) _editRow--; }
            else if (action == EcuHal::JoyAction::Down) { if (_editRow % 3 < 2) _editRow++; }
            else if (action == EcuHal::JoyAction::Left) {
                if (_editRow >= 3) _editRow -= 3;
                else { _genTab = 1; _editRow = 1; _needsFullRedraw = true; }
            } else if (action == EcuHal::JoyAction::Right) {
                if (_editRow < 3) _editRow += 3;
                else { _genTab = 3; _editRow = 0; _needsFullRedraw = true; }
            } else if (action == EcuHal::JoyAction::Click) onEncoderClick();
            return;
        } else if (_genTab == 3) {
            if (action == EcuHal::JoyAction::Up) { if (_editRow >= 2) _editRow -= 2; }
            else if (action == EcuHal::JoyAction::Down) { if (_editRow < 2) _editRow += 2; }
            else if (action == EcuHal::JoyAction::Left) {
                if (_editRow % 2 == 1) _editRow--;
                else { _genTab = 2; _editRow = 3; _needsFullRedraw = true; }
            } else if (action == EcuHal::JoyAction::Right) {
                if (_editRow % 2 == 0) _editRow++;
            } else if (action == EcuHal::JoyAction::Click) onEncoderClick();
            return;
        }
    }

    if (action == EcuHal::JoyAction::Left) {
        if (_genTab > 0) { _genTab--; _editRow = 0; _needsFullRedraw = true; }
        return;
    } else if (action == EcuHal::JoyAction::Right) {
        if (_genTab < 3) { _genTab++; _editRow = 0; _needsFullRedraw = true; }
        return;
    }

    if (_uiLevel == UiLevel::EpsTester) {
        if (action == EcuHal::JoyAction::Up) _editRow = (_editRow > 0) ? (_editRow - 1) : 4;
        else if (action == EcuHal::JoyAction::Down) _editRow = (_editRow + 1) % 5;
        else if (action == EcuHal::JoyAction::Click) onEncoderClick();
        return;
    }

    if (_uiLevel == UiLevel::Generator) {
        if (action == EcuHal::JoyAction::Up) {
            if (_genTab == 1) _editRow = (_editRow > 0) ? (_editRow - 1) : 2;
            else if (_genTab == 2) _editRow = (_editRow > 0) ? (_editRow - 1) : 4;
            else if (_genTab == 3) _editRow = (_editRow > 0) ? (_editRow - 1) : 3;
        } else if (action == EcuHal::JoyAction::Down) {
            if (_genTab == 1) _editRow = (_editRow + 1) % 3;
            else if (_genTab == 2) _editRow = (_editRow + 1) % 5;
            else if (_genTab == 3) _editRow = (_editRow + 1) % 4;
        } else if (action == EcuHal::JoyAction::Click) onEncoderClick();
    } else if (_uiLevel == UiLevel::Capture && action == EcuHal::JoyAction::Click) onEncoderClick();
}

void MenuManager::onEncoderClick() {
    if (_uiLevel == UiLevel::MainHub) {
        if (_hubIndex == 0) { _uiLevel = UiLevel::Generator; _genTab = 1; }
        else if (_hubIndex == 1) { _uiLevel = UiLevel::Capture; _genTab = 1; }
        else if (_hubIndex == 2) { _uiLevel = UiLevel::EpsTester; _genTab = 1; _editRow = 0; _isEditMode = false; }
        else if (_hubIndex == 3) { _uiLevel = UiLevel::SpeedoTester; _genTab = 1; _editRow = 0; _isEditMode = false; }
        _needsFullRedraw = true; return;
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
    if (_uiLevel == UiLevel::Generator && _genTab == 1) _isEditMode = !_isEditMode;
}

void MenuManager::onEncoderDoubleClick(EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam) {
    if (_uiLevel == UiLevel::Capture) { _pageCapture.onEncoderDoubleClick(wheel, cam); return; }
    if (_uiLevel == UiLevel::EpsTester) { if (_epsController) _epsController->toggleRunning(); return; }
    if (_uiLevel == UiLevel::SpeedoTester) { if (_speedoController) _speedoController->toggleRunning(); return; }
    returnToMainHub();
}

} // namespace EcuUi
