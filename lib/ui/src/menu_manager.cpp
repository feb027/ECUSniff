#include "menu_manager.h"

namespace EcuUi {

MenuManager::MenuManager(LovyanGFX* gfx)
    : _gfx(gfx), _pageHub(gfx), _pageDash(gfx), _pageCkp(gfx), _pageCmp(gfx),
      _pageGenSettings(gfx), _pageCapture(gfx), _pageEps(gfx), _pageSpeedo(gfx),
      _pageBrowser(gfx), _pagePwrCycle(gfx) {}

void MenuManager::init(EcuHal::CaptureDriver* capDriver, EcuEngine::SignalSniffer* sniffer,
                       EcuEngine::EpsController* eps, EcuEngine::SpeedoController* speedo,
                       EcuEngine::PowerCycleController* pwrCycle) {
    _epsController = eps; _speedoController = speedo; _powerCycleController = pwrCycle;
    _pageDash.init(); _pageGenSettings.init(); _pageCapture.init(capDriver, sniffer);
    _pageEps.init(); _pageSpeedo.init(); _pageBrowser.init(); _pagePwrCycle.init();
    _uiLevel = UiLevel::MainHub; _hubIndex = 0; _genTab = 1;
    _focusTabBar = true; _needsFullRedraw = true;
}

void MenuManager::returnToMainHub() {
    _gfx->fillScreen(TFT_BLACK);
    _uiLevel = UiLevel::MainHub; _needsFullRedraw = true;
    _lastTab = 0xFF; _lastDrawnTab = 0xFF; _isEditMode = false; _focusTabBar = true;
    _pageBrowser.close();
}

void MenuManager::setUiLevel(UiLevel level) {
    if (_uiLevel != level) {
        _gfx->fillScreen(TFT_BLACK);
        _uiLevel = level; _genTab = 1; _editRow = 0;
        _isEditMode = false; _focusTabBar = true; _needsFullRedraw = true;
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

    const char* tabsGen[]      = { "< MENU", "DASHBOARD", "CKP WHEEL", "CMP CAM", "SETTINGS" };
    const char* tabsCap[]      = { "< MENU", "LIVE CAPTURE", "DECODE DATA", "CAM EVENTS" };
    const char* tabsEps[]      = { "< MENU", "EPS BENCH", "OEM PRESET", "AUTO SWEEP" };
    const char* tabsSpeedo[]   = { "< MENU", "SPEEDO COCKPIT", "3-PT CAL", "HARDWARE" };
    const char* tabsPwrCycle[] = { "< MENU", "POWER CYCLE TESTER" };

    struct TabTheme {
        uint16_t activeBg;
        uint16_t activeFg;
        uint16_t inactiveBg;
        uint16_t inactiveBorder;
        uint16_t inactiveFg;
    };

    static const TabTheme genThemes[5] = {
        { 0xF800, TFT_WHITE, 0x5800, 0xF800, TFT_WHITE }, // 0: < MENU (Crimson Red)
        { 0x07E0, TFT_BLACK, 0x0300, 0x07E0, TFT_WHITE }, // 1: DASHBOARD (Emerald Green)
        { 0xFD20, TFT_BLACK, 0x6280, 0xFD20, TFT_WHITE }, // 2: CKP WHEEL (Amber / Orange)
        { 0xD81F, TFT_WHITE, 0x4810, 0xD81F, TFT_WHITE }, // 3: CMP CAM (Magenta / Purple)
        { 0x07FF, TFT_BLACK, 0x0250, 0x07FF, TFT_WHITE }  // 4: SETTINGS (Electric Cyan)
    };

    static const TabTheme capThemes[4] = {
        { 0xF800, TFT_WHITE, 0x5800, 0xF800, TFT_WHITE }, // 0: < MENU
        { 0x07E0, TFT_BLACK, 0x0300, 0x07E0, TFT_WHITE }, // 1: LIVE CAPTURE
        { 0x07FF, TFT_BLACK, 0x0250, 0x07FF, TFT_WHITE }, // 2: DECODE DATA
        { 0xD81F, TFT_WHITE, 0x4810, 0xD81F, TFT_WHITE }  // 3: CAM EVENTS
    };

    static const TabTheme epsThemes[4] = {
        { 0xF800, TFT_WHITE, 0x5800, 0xF800, TFT_WHITE }, // 0: < MENU
        { 0xFD20, TFT_BLACK, 0x6280, 0xFD20, TFT_WHITE }, // 1: EPS BENCH
        { 0x07E0, TFT_BLACK, 0x0300, 0x07E0, TFT_WHITE }, // 2: OEM PRESET
        { 0x07FF, TFT_BLACK, 0x0250, 0x07FF, TFT_WHITE }  // 3: AUTO SWEEP
    };

    static const TabTheme speedoThemes[4] = {
        { 0xF800, TFT_WHITE, 0x5800, 0xF800, TFT_WHITE }, // 0: < MENU
        { 0x07E0, TFT_BLACK, 0x0300, 0x07E0, TFT_WHITE }, // 1: SPEEDO COCKPIT
        { 0xFFE0, TFT_BLACK, 0x6300, 0xFFE0, TFT_WHITE }, // 2: 3-PT CAL
        { 0x07FF, TFT_BLACK, 0x0250, 0x07FF, TFT_WHITE }  // 3: HARDWARE
    };

    static const TabTheme pwrThemes[2] = {
        { 0xF800, TFT_WHITE, 0x5800, 0xF800, TFT_WHITE }, // 0: < MENU
        { 0x07E0, TFT_BLACK, 0x0300, 0x07E0, TFT_WHITE }  // 1: POWER CYCLE TESTER
    };

    uint8_t totalTabs = (_uiLevel == UiLevel::Generator) ? 5 : ((_uiLevel == UiLevel::PowerCycle) ? 2 : 4);
    const char** tabs = (_uiLevel == UiLevel::Capture) ? tabsCap : 
                        ((_uiLevel == UiLevel::EpsTester) ? tabsEps : 
                        ((_uiLevel == UiLevel::SpeedoTester) ? tabsSpeedo : 
                        ((_uiLevel == UiLevel::PowerCycle) ? tabsPwrCycle : tabsGen)));

    const TabTheme* currentThemes = (_uiLevel == UiLevel::Capture) ? capThemes :
                                    ((_uiLevel == UiLevel::EpsTester) ? epsThemes :
                                    ((_uiLevel == UiLevel::SpeedoTester) ? speedoThemes :
                                    ((_uiLevel == UiLevel::PowerCycle) ? pwrThemes : genThemes)));

    for (uint8_t i = 0; i < totalTabs; ++i) {
        int32_t w = (totalTabs == 5) ? 88 : ((totalTabs == 2) ? 220 : 110);
        int32_t x = (totalTabs == 5) ? (6 + (i * 94)) : ((totalTabs == 2) ? (16 + (i * 232)) : (8 + (i * 118)));
        bool isActive = (_genTab == i);
        const TabTheme& th = currentThemes[i];

        uint16_t bg = isActive ? th.activeBg : th.inactiveBg;
        uint16_t border = isActive ? 0xFFE0 : th.inactiveBorder;
        uint16_t fg = isActive ? th.activeFg : th.inactiveFg;

        _gfx->fillRoundRect(x, 4, w, 32, 6, bg);
        _gfx->drawRoundRect(x, 4, w, 32, 6, border);
        if (isActive && (_focusTabBar || _genTab == 0)) {
            _gfx->drawRoundRect(x + 1, 5, w - 2, 30, 5, 0xFFE0);
        }
        _gfx->setTextColor(fg, bg);
        _gfx->setTextSize(1);
        int32_t tw = _gfx->textWidth(tabs[i]);
        _gfx->drawString(tabs[i], x + (w - tw) / 2, 14);
    }
    _lastDrawnTab = _genTab; _lastDrawnFocusTabBar = _focusTabBar;
}

void MenuManager::render(const EcuEngine::EngineRuntimeState& state,
                         const EcuEngine::ParametricWheel& wheel,
                         const EcuEngine::CamEventTable& cam) {
    if (_pageBrowser.isOpen()) { _pageBrowser.render(_needsFullRedraw); _needsFullRedraw = false; return; }
    if (_uiLevel == UiLevel::MainHub) { _pageHub.render(_needsFullRedraw, _hubIndex); _needsFullRedraw = false; return; }

    bool isRedraw = _needsFullRedraw || (_genTab != _lastTab);
    _drawGeneratorTabBar(isRedraw);

    if (_genTab == 0) {
        if (isRedraw) {
            _gfx->fillRect(0, 40, 480, 280, 0x0841);
            _gfx->fillRoundRect(8, 44, 464, 268, 8, 0x10A2);
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

    if (_uiLevel == UiLevel::EpsTester && _epsController) _pageEps.render(isRedraw, false, activeEditRow, *_epsController);
    else if (_uiLevel == UiLevel::SpeedoTester && _speedoController) _pageSpeedo.render(_genTab, isRedraw, activeEditRow, *_speedoController);
    else if (_uiLevel == UiLevel::Generator) {
        if (_genTab == 1) _pageDash.render(isRedraw, false, activeEditRow, state, wheel, cam);
        else if (_genTab == 2) _pageCkp.render(_pageDash.getActivePresetIdx(), wheel, false, activeEditRow, isRedraw);
        else if (_genTab == 3) _pageCmp.render(_pageDash.getActivePresetIdx(), cam, false, activeEditRow, isRedraw);
        else if (_genTab == 4) {
            uint8_t settingsEditRow = (_focusTabBar || _pageGenSettings.isFocusSubNav()) ? 255 : _editRow;
            _pageGenSettings.render(isRedraw, settingsEditRow, state, wheel);
        }
    } else if (_uiLevel == UiLevel::Capture) {
        _pageCapture.render(_genTab, isRedraw, false);
    } else if (_uiLevel == UiLevel::PowerCycle && _powerCycleController) {
        _pagePwrCycle.render(isRedraw, activeEditRow, 
                             _powerCycleController->getConfig(), 
                             _powerCycleController->getState(), 
                             state.mcpFound);
    }
    _lastTab = _genTab; _needsFullRedraw = false;
}

void MenuManager::onEncoderTurn(int32_t delta,
                               EcuEngine::EngineRuntimeState& state,
                               EcuEngine::ParametricWheel& wheel,
                               EcuEngine::CamEventTable& cam) {
    if (_pageBrowser.isOpen()) { _pageBrowser.onEncoderTurn(delta); return; }
    if (_uiLevel == UiLevel::MainHub) { _pageHub.onEncoderTurn(delta, _hubIndex); return; }
    uint8_t maxTabs = (_uiLevel == UiLevel::Generator) ? 4 : ((_uiLevel == UiLevel::PowerCycle) ? 1 : 3);
    if (_focusTabBar) { _genTab = constrain((int32_t)_genTab + delta, 0, (int32_t)maxTabs); _needsFullRedraw = true; return; }

    if (_uiLevel == UiLevel::EpsTester && _epsController) _pageEps.onEncoderTurn(delta, _editRow, *_epsController);
    else if (_uiLevel == UiLevel::SpeedoTester && _speedoController) _pageSpeedo.onEncoderTurn(_genTab, delta, _editRow, *_speedoController);
    else if (_uiLevel == UiLevel::PowerCycle && _powerCycleController) {
        _pagePwrCycle.onEncoderTurn(delta, _editRow, _powerCycleController->getConfig());
    }
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
            if (_pageGenSettings.isFocusSubNav()) {
                if (delta > 0) _pageGenSettings.nextSubCategory();
                else if (delta < 0) _pageGenSettings.prevSubCategory();
                _needsFullRedraw = true;
            } else {
                _pageGenSettings.onEncoderTurn(delta, _editRow, state, wheel);
            }
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
        if (action == EcuHal::JoyAction::Up || action == EcuHal::JoyAction::Left) _hubIndex = (_hubIndex > 0) ? (_hubIndex - 1) : 4;
        else if (action == EcuHal::JoyAction::Down || action == EcuHal::JoyAction::Right) _hubIndex = (_hubIndex + 1) % 5;
        else if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
        return;
    }
    if (_genTab == 0) {
        if (action == EcuHal::JoyAction::Right) { _genTab = 1; _editRow = 0; _focusTabBar = true; _needsFullRedraw = true; }
        else if (action == EcuHal::JoyAction::Click) returnToMainHub();
        return;
    }
    uint8_t maxTabs = (_uiLevel == UiLevel::Generator) ? 4 : ((_uiLevel == UiLevel::PowerCycle) ? 1 : 3);
    if (_focusTabBar) {
        if (action == EcuHal::JoyAction::Left && _genTab > 0) { _genTab--; _needsFullRedraw = true; }
        else if (action == EcuHal::JoyAction::Right && _genTab < maxTabs) { _genTab++; _needsFullRedraw = true; }
        else if (action == EcuHal::JoyAction::Down && _genTab > 0) {
            _focusTabBar = false; _editRow = 0;
            if (_genTab == 4) _pageGenSettings.setFocusSubNav(true);
            _needsFullRedraw = true;
        }
        else if (action == EcuHal::JoyAction::Click) {
            if (_genTab == 0) returnToMainHub();
            else {
                _focusTabBar = false; _editRow = 0;
                if (_genTab == 4) _pageGenSettings.setFocusSubNav(true);
                _needsFullRedraw = true;
            }
        }
        return;
    }

    if (_uiLevel == UiLevel::SpeedoTester) {
        if (_genTab == 1) {
            if (action == EcuHal::JoyAction::Up) { if (_editRow <= 1) { _focusTabBar = true; _needsFullRedraw = true; } else if (_editRow <= 3) _editRow -= 2; else _editRow = 2; }
            else if (action == EcuHal::JoyAction::Down) { if (_editRow <= 1) _editRow += 2; else if (_editRow <= 3) _editRow = 4; }
            else if (action == EcuHal::JoyAction::Left && (_editRow == 1 || _editRow == 3)) _editRow--;
            else if (action == EcuHal::JoyAction::Right && (_editRow == 0 || _editRow == 2)) _editRow++;
            else if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
            return;
        } else if (_genTab == 2) {
            if (action == EcuHal::JoyAction::Up) { if (_editRow == 0 || _editRow == 3) { _focusTabBar = true; _needsFullRedraw = true; } else if (_editRow % 3 > 0) _editRow--; }
            else if (action == EcuHal::JoyAction::Down && (_editRow % 3 < 2)) _editRow++;
            else if (action == EcuHal::JoyAction::Left && _editRow >= 3) _editRow -= 3;
            else if (action == EcuHal::JoyAction::Right && _editRow < 3) _editRow += 3;
            else if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
            return;
        } else if (_genTab == 3) {
            if (action == EcuHal::JoyAction::Up) { if (_editRow <= 1) { _focusTabBar = true; _needsFullRedraw = true; } else _editRow -= 2; }
            else if (action == EcuHal::JoyAction::Down && _editRow < 2) _editRow += 2;
            else if (action == EcuHal::JoyAction::Left && (_editRow % 2 == 1)) _editRow--;
            else if (action == EcuHal::JoyAction::Right && (_editRow % 2 == 0)) _editRow++;
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
        if (_genTab == 1) {
            if (action == EcuHal::JoyAction::Up) {
                if (_editRow <= 2) { _focusTabBar = true; _needsFullRedraw = true; }
                else if (_editRow == 3 || _editRow == 4) _editRow = 1;
                else if (_editRow == 5 || _editRow == 6) _editRow = 2;
                else if (_editRow == 7) _editRow = 0;
            } else if (action == EcuHal::JoyAction::Down) {
                if (_editRow == 0) _editRow = 7;
                else if (_editRow == 1) _editRow = 3;
                else if (_editRow == 2) _editRow = 5;
                else if (_editRow >= 3 && _editRow <= 6) _editRow = 7;
            } else if (action == EcuHal::JoyAction::Left) {
                if (_editRow == 1 || _editRow == 3) _editRow = 0;
                else if (_editRow == 2) _editRow = 1;
                else if (_editRow == 4) _editRow = 3;
                else if (_editRow == 5) _editRow = 4;
                else if (_editRow == 6) _editRow = 5;
            } else if (action == EcuHal::JoyAction::Right) {
                if (_editRow == 0) _editRow = 1;
                else if (_editRow == 1) _editRow = 2;
                else if (_editRow == 3) _editRow = 4;
                else if (_editRow == 4) _editRow = 5;
                else if (_editRow == 5) _editRow = 6;
            } else if (action == EcuHal::JoyAction::Click) {
                onEncoderClick(state, wheel, cam);
            }
        } else if (_genTab == 2) {
            if (action == EcuHal::JoyAction::Up) { if (_editRow == 0) { _focusTabBar = true; _needsFullRedraw = true; } else _editRow--; }
            else if (action == EcuHal::JoyAction::Down && _editRow < 4) _editRow++;
            else if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
        } else if (_genTab == 3) {
            if (action == EcuHal::JoyAction::Up) { if (_editRow == 0) { _focusTabBar = true; _needsFullRedraw = true; } else _editRow--; }
            else if (action == EcuHal::JoyAction::Down && _editRow < 3) _editRow++;
            else if (action == EcuHal::JoyAction::Click) onEncoderClick(state, wheel, cam);
        } else if (_genTab == 4) {
            uint8_t curCat = _pageGenSettings.getSubCategory();

            // TIER 2: Cursor berada di bar Sub-Menu Kategori
            if (_pageGenSettings.isFocusSubNav()) {
                if (action == EcuHal::JoyAction::Up) {
                    // Naik ke Tab Bar Utama
                    _focusTabBar = true;
                    _needsFullRedraw = true;
                } else if (action == EcuHal::JoyAction::Down || action == EcuHal::JoyAction::Click) {
                    // Masuk ke item pengaturan di bawahnya
                    _pageGenSettings.setFocusSubNav(false);
                    _editRow = 0;
                    _needsFullRedraw = true;
                } else if (action == EcuHal::JoyAction::Left) {
                    // Pindah sub-menu kategori ke kiri
                    _pageGenSettings.prevSubCategory();
                    _needsFullRedraw = true;
                } else if (action == EcuHal::JoyAction::Right) {
                    // Pindah sub-menu kategori ke kanan
                    _pageGenSettings.nextSubCategory();
                    _needsFullRedraw = true;
                }
                return;
            }

            // TIER 3: Cursor berada di Item Pengaturan
            if (curCat == 0) { // Cranking: 5 items (Row 0, 1 / Row 2, 3 / Row 4)
                if (action == EcuHal::JoyAction::Up) {
                    if (_editRow <= 1) {
                        // Kembali ke bar Sub-Menu Kategori
                        _pageGenSettings.setFocusSubNav(true);
                        _needsFullRedraw = true;
                    } else if (_editRow <= 3) {
                        _editRow -= 2;
                    } else {
                        _editRow = 2; // dari row 4 ke row 2
                    }
                } else if (action == EcuHal::JoyAction::Down) {
                    if (_editRow <= 1) { _editRow += 2; }
                    else if (_editRow <= 3) { _editRow = 4; }
                } else if (action == EcuHal::JoyAction::Left) {
                    if (_editRow == 1 || _editRow == 3) _editRow--;
                } else if (action == EcuHal::JoyAction::Right) {
                    if (_editRow == 0 || _editRow == 2) _editRow++;
                } else if (action == EcuHal::JoyAction::Click) {
                    onEncoderClick(state, wheel, cam);
                }
            } else if (curCat == 1) { // Fix Encoder: 3 items (Row 0: Step / Row 1: Min, Row 2: Max)
                if (action == EcuHal::JoyAction::Up) {
                    if (_editRow == 0) {
                        _pageGenSettings.setFocusSubNav(true);
                        _needsFullRedraw = true;
                    } else {
                        _editRow = 0;
                    }
                } else if (action == EcuHal::JoyAction::Down) {
                    if (_editRow == 0) _editRow = 1;
                } else if (action == EcuHal::JoyAction::Left) {
                    if (_editRow == 2) _editRow = 1;
                } else if (action == EcuHal::JoyAction::Right) {
                    if (_editRow == 1) _editRow = 2;
                } else if (action == EcuHal::JoyAction::Click) {
                    onEncoderClick(state, wheel, cam);
                }
            } else if (curCat == 2) { // Potensio: 3 items (Row 0: Step / Row 1: Min, Row 2: Max)
                if (action == EcuHal::JoyAction::Up) {
                    if (_editRow == 0) {
                        _pageGenSettings.setFocusSubNav(true);
                        _needsFullRedraw = true;
                    } else {
                        _editRow = 0;
                    }
                } else if (action == EcuHal::JoyAction::Down) {
                    if (_editRow == 0) _editRow = 1;
                } else if (action == EcuHal::JoyAction::Left) {
                    if (_editRow == 2) _editRow = 1;
                } else if (action == EcuHal::JoyAction::Right) {
                    if (_editRow == 1) _editRow = 2;
                } else if (action == EcuHal::JoyAction::Click) {
                    onEncoderClick(state, wheel, cam);
                }
            } else if (curCat == 3) { // Auto Sweep: 4 items (Row 0: Step / Row 1: Min, Row 2: Max / Row 3: Rate)
                if (action == EcuHal::JoyAction::Up) {
                    if (_editRow == 0) {
                        _pageGenSettings.setFocusSubNav(true);
                        _needsFullRedraw = true;
                    } else if (_editRow <= 2) {
                        _editRow = 0;
                    } else {
                        _editRow = 1;
                    }
                } else if (action == EcuHal::JoyAction::Down) {
                    if (_editRow == 0) _editRow = 1;
                    else if (_editRow <= 2) _editRow = 3;
                } else if (action == EcuHal::JoyAction::Left) {
                    if (_editRow == 2) _editRow = 1;
                } else if (action == EcuHal::JoyAction::Right) {
                    if (_editRow == 1) _editRow = 2;
                } else if (action == EcuHal::JoyAction::Click) {
                    onEncoderClick(state, wheel, cam);
                }
            } else if (curCat == 4) { // VVT-i: 4 items (Row 0: Enable, Row 1: Start RPM / Row 2: Max Adv, Row 3: Full RPM)
                if (action == EcuHal::JoyAction::Up) {
                    if (_editRow <= 1) {
                        _pageGenSettings.setFocusSubNav(true);
                        _needsFullRedraw = true;
                    } else if (_editRow == 2) {
                        _editRow = 0;
                    } else if (_editRow == 3) {
                        _editRow = 1;
                    }
                } else if (action == EcuHal::JoyAction::Down) {
                    if (_editRow == 0) _editRow = 2;
                    else if (_editRow == 1) _editRow = 3;
                } else if (action == EcuHal::JoyAction::Left) {
                    if (_editRow == 1) _editRow = 0;
                    else if (_editRow == 3) _editRow = 2;
                } else if (action == EcuHal::JoyAction::Right) {
                    if (_editRow == 0) _editRow = 1;
                    else if (_editRow == 2) _editRow = 3;
                } else if (action == EcuHal::JoyAction::Click) {
                    onEncoderClick(state, wheel, cam);
                }
            } else { // Hardware: 1 item (Row 0)
                if (action == EcuHal::JoyAction::Up) {
                    _pageGenSettings.setFocusSubNav(true);
                    _needsFullRedraw = true;
                } else if (action == EcuHal::JoyAction::Click) {
                    onEncoderClick(state, wheel, cam);
                }
            }
        }
        return;
    }

    if (_uiLevel == UiLevel::PowerCycle) {
        if (action == EcuHal::JoyAction::Up) {
            if (_editRow == 0) { _focusTabBar = true; _needsFullRedraw = true; }
            else if (_editRow == 5) _editRow = 4;
            else _editRow--;
        } else if (action == EcuHal::JoyAction::Down) {
            if (_editRow < 4) _editRow++;
        } else if (action == EcuHal::JoyAction::Left) {
            if (_editRow == 5) _editRow = 4;
        } else if (action == EcuHal::JoyAction::Right) {
            if (_editRow == 4) _editRow = 5;
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
            _pageDash.setActiveCategory(static_cast<uint8_t>(_pageBrowser.getSelectedCategory()));
            _needsFullRedraw = true;
        }
        return;
    }
    if (_uiLevel == UiLevel::MainHub) {
        if (_hubIndex == 0) { _uiLevel = UiLevel::Generator; _genTab = 1; }
        else if (_hubIndex == 1) { _uiLevel = UiLevel::Capture; _genTab = 1; }
        else if (_hubIndex == 2) { _uiLevel = UiLevel::EpsTester; _genTab = 1; _editRow = 0; }
        else if (_hubIndex == 3) { _uiLevel = UiLevel::SpeedoTester; _genTab = 1; _editRow = 0; }
        else if (_hubIndex == 4) { _uiLevel = UiLevel::PowerCycle; _genTab = 1; _editRow = 0; }
        _focusTabBar = true; _needsFullRedraw = true; return;
    }
    if (_genTab == 0) { returnToMainHub(); return; }
    if (_uiLevel == UiLevel::PowerCycle && _powerCycleController) {
        _pagePwrCycle.onEncoderClick(_editRow, *_powerCycleController);
        _needsFullRedraw = true;
        return;
    }
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
            if (_editRow == 7) { 
                _pageBrowser.open(_pageDash.getActivePresetIdx(), static_cast<WheelCategory>(_pageDash.getActiveCategory())); 
                _needsFullRedraw = true; 
                return; 
            } else if (_editRow == 1) {
                int32_t m = static_cast<int32_t>(state.runMode) + 1;
                if (m > 4) m = 0;
                state.runMode = static_cast<EcuEngine::EngineRunMode>(m);
                _needsFullRedraw = true;
                return;
            } else if (_editRow == 2) {
                state.isRunning = !state.isRunning;
                _needsFullRedraw = true;
                return;
            } else if (_editRow == 3) {
                state.ckpEnabled = !state.ckpEnabled;
                _needsFullRedraw = true;
                return;
            } else if (_editRow == 4) {
                state.cmp1Enabled = !state.cmp1Enabled;
                _needsFullRedraw = true;
                return;
            } else if (_editRow == 5) {
                state.cmp2Enabled = !state.cmp2Enabled;
                _needsFullRedraw = true;
                return;
            } else if (_editRow == 6) {
                wheel.inverted = !wheel.inverted;
                _needsFullRedraw = true;
                return;
            }
            state.isRunning = !state.isRunning; _needsFullRedraw = true;
        } else if (_genTab == 4) {
            if (_pageGenSettings.isFocusSubNav()) {
                _pageGenSettings.setFocusSubNav(false);
                _editRow = 0;
                _needsFullRedraw = true;
            } else {
                _pageGenSettings.onEncoderClick(_editRow, state, wheel);
                _needsFullRedraw = true;
            }
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
