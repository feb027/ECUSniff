#include "page_dashboard.h"

namespace EcuUi {

WheelPresetItem PageDashboard::s_capturedPreset{};
char PageDashboard::s_capturedName[32] = "Captured: Kustom";
bool PageDashboard::s_hasCaptured = false;

const WheelPresetItem PageDashboard::PRESETS[] = {
    { "Honda / Ford 36-1", 36, 1, 0, 0.50f, false, 4, {120.0f, 180.0f, 420.0f, 470.0f}, {true, false, true, false} },
    { "Toyota 1NZ/2NZ 36-2", 36, 2, 0, 0.50f, false, 2, {90.0f, 270.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Bosch / VW / BMW 60-2", 60, 2, 0, 0.50f, false, 2, {180.0f, 540.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Mazda Miata 24-2", 24, 2, 0, 0.50f, false, 2, {120.0f, 360.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Suzuki / Daihatsu 12-1", 12, 1, 0, 0.50f, false, 2, {180.0f, 360.0f, 0.0f, 0.0f}, {true, false, false, false} },
    { "Mitsubishi 4G63 4-0", 4, 0, 0, 0.50f, false, 4, {70.0f, 180.0f, 370.0f, 450.0f}, {true, false, true, false} }
};

PageDashboard::PageDashboard(LovyanGFX* gfx) : _gfx(gfx), _canvas(gfx) {}

void PageDashboard::init() {
    _canvas.init(448, 82);
}

void PageDashboard::setCapturedPreset(const char* name, const EcuEngine::ParametricWheel& wheel, const EcuEngine::CamEventTable& cam) {
    s_hasCaptured = true;
    if (name && name[0] != '\0') {
        strncpy(s_capturedName, name, sizeof(s_capturedName) - 1);
        s_capturedName[sizeof(s_capturedName) - 1] = '\0';
    }
    s_capturedPreset.name = s_capturedName;
    s_capturedPreset.totalTeeth = wheel.totalTeeth;
    s_capturedPreset.missingTeeth = wheel.missingTeeth;
    s_capturedPreset.missingPosition = wheel.missingPosition;
    s_capturedPreset.dutyCycle = wheel.dutyCycle;
    s_capturedPreset.inverted = wheel.inverted;
    s_capturedPreset.camCount = cam.getEventCount();
    const auto* evs = cam.getEvents();
    for (uint8_t i = 0; i < 4; ++i) {
        if (evs && i < s_capturedPreset.camCount) {
            s_capturedPreset.camAngles[i] = evs[i].angleDeg;
            s_capturedPreset.camHighs[i] = evs[i].levelHigh;
        } else {
            s_capturedPreset.camAngles[i] = 0.0f;
            s_capturedPreset.camHighs[i] = false;
        }
    }
}

bool PageDashboard::hasCapturedPreset() {
    return s_hasCaptured;
}

void PageDashboard::_applyPreset(uint8_t idx, EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam) {
    const WheelPresetItem* p = nullptr;
    if (idx < BASE_PRESET_COUNT) {
        p = &PRESETS[idx];
    } else if (idx == BASE_PRESET_COUNT && s_hasCaptured) {
        p = &s_capturedPreset;
    }
    if (!p) return;

    wheel.totalTeeth = p->totalTeeth;
    wheel.missingTeeth = p->missingTeeth;
    wheel.missingPosition = p->missingPosition;
    wheel.dutyCycle = p->dutyCycle;
    wheel.inverted = p->inverted;

    cam.clear();
    for (uint8_t i = 0; i < p->camCount; ++i) {
        cam.addEvent(p->camAngles[i], p->camHighs[i]);
    }
}

void PageDashboard::render(bool fullRedraw, bool isEditMode, uint8_t editRow,
                           const EcuEngine::EngineRuntimeState& state,
                           const EcuEngine::ParametricWheel& wheel,
                           const EcuEngine::CamEventTable& cam) {
    uint32_t activeRpm = state.isRunning ? state.currentRpm : state.targetRpm;
    uint8_t curMode = static_cast<uint8_t>(state.runMode);

    if (fullRedraw) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        _canvas.render(wheel, cam, 16, 50);

        _gfx->fillRoundRect(16, 138, 220, 66, 6, 0x0841);
        _gfx->drawRoundRect(16, 138, 220, 66, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841);
        _gfx->setTextSize(1);
        _gfx->drawString("TARGET RPM MESIN:", 26, 146);

        _gfx->fillRoundRect(244, 138, 220, 66, 6, 0x0841);
        _gfx->drawRoundRect(244, 138, 220, 66, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841);
        _gfx->setTextSize(1);
        _gfx->drawString("SIMULATION MODE:", 254, 146);

        _gfx->fillRoundRect(16, 210, 448, 66, 6, 0x0841);
        _gfx->drawRoundRect(16, 210, 448, 66, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841);
        _gfx->setTextSize(1);
        _gfx->drawString("POLA RODA & PROFIL MESIN AKTIF:", 26, 218);

        _lastRpm = 0xFFFFFFFF;
        _lastMode = 0xFF;
        _lastIsRunning = !state.isRunning;
        _lastIsEditMode = !isEditMode;
        _lastEditRow = 0xFF;
        _lastTotalTeeth = 0xFFFF;
    }

    bool isRunningChanged = (state.isRunning != _lastIsRunning);
    bool rpmChanged = (activeRpm != _lastRpm);
    bool modeChanged = (curMode != _lastMode);
    bool editChanged = (isEditMode != _lastIsEditMode || editRow != _lastEditRow);

    if (rpmChanged || isRunningChanged || fullRedraw) {
        char rpmStr[8]; snprintf(rpmStr, sizeof(rpmStr), "%04u", (unsigned)activeRpm);
        _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(4); _gfx->drawString(rpmStr, 26, 162);
        _gfx->setTextSize(2); _gfx->setTextColor(0x07FF, 0x0841); _gfx->drawString("RPM", 162, 174);
    }

    if (modeChanged || isRunningChanged || fullRedraw) {
        const char* modeNames[] = { "FIX", "CRANK", "SWEEP" };
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(3);
        _gfx->drawString("        ", 254, 165); _gfx->drawString(modeNames[curMode % 3], 254, 165);

        if (state.isRunning) {
            _gfx->fillRoundRect(354, 152, 98, 40, 4, 0x03E0); _gfx->drawRoundRect(354, 152, 98, 40, 4, 0x07E0);
            _gfx->setTextColor(0x07E0, 0x03E0); _gfx->setTextSize(2); _gfx->drawCenterString("RUNNING", 403, 164);
        } else {
            _gfx->fillRoundRect(354, 152, 98, 40, 4, 0x3800); _gfx->drawRoundRect(354, 152, 98, 40, 4, 0xF800);
            _gfx->setTextColor(0xF800, 0x3800); _gfx->setTextSize(2); _gfx->drawCenterString("STOPPED", 403, 164);
        }
    }

    if (wheel.totalTeeth != _lastTotalTeeth || wheel.missingTeeth != _lastMissingTeeth || fullRedraw) {
        const char* name = (state.activeWheelName[0] != '\0') ? state.activeWheelName : "Pola Kustom";
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString("                             ", 26, 234); _gfx->drawString(name, 26, 234);

        char detailBuf[48];
        snprintf(detailBuf, sizeof(detailBuf), "(%u-%u CKP | %u Pulsa Cam)          ",
                 (unsigned)wheel.totalTeeth, (unsigned)wheel.missingTeeth, (unsigned)cam.getEventCount());
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1); _gfx->drawString(detailBuf, 26, 256);

        _lastTotalTeeth = wheel.totalTeeth; _lastMissingTeeth = wheel.missingTeeth;
        if (!fullRedraw) _canvas.render(wheel, cam, 16, 50);
    }

    if (editChanged || isRunningChanged || fullRedraw) {
        _drawEditFrames(isEditMode, editRow, state.isRunning, wheel);
    }

    _lastRpm = activeRpm; _lastIsRunning = state.isRunning; _lastMode = curMode;
    _lastIsEditMode = isEditMode; _lastEditRow = editRow;
}

void PageDashboard::_drawEditFrames(bool isEditMode, uint8_t editRow, bool isRunning, const EcuEngine::ParametricWheel& wheel) {
    uint32_t cRpm   = (editRow == 0) ? 0xFFE0 : 0x52AA;
    uint32_t cMode  = (editRow == 1) ? 0xFFE0 : 0x52AA;
    uint32_t cWheel = (editRow == 2) ? 0xFFE0 : 0x52AA;

    _gfx->drawRoundRect(16, 138, 220, 66, 6, cRpm);
    _gfx->drawRoundRect(244, 138, 220, 66, 6, cMode);
    _gfx->drawRoundRect(16, 210, 448, 66, 6, cWheel);

    _gfx->fillRect(16, 282, 448, 24, 0x0841);
    _gfx->drawRoundRect(16, 282, 448, 24, 4, 0x31A6);
    _gfx->setTextSize(1);

    if (editRow == 0) {
        _gfx->setTextColor(0xFFE0, 0x0841);
        _gfx->drawString("[TARGET RPM] Putar: +/-50 RPM | Joystick Bawah: Ganti Baris", 24, 289);
    } else if (editRow == 1) {
        _gfx->setTextColor(0xFFE0, 0x0841);
        _gfx->drawString("[MODE MESIN] Putar: FIX / CRANK / SWEEP | Joystick Bawah: Pola", 16, 289);
    } else if (editRow == 2) {
        _gfx->setTextColor(0xFFE0, 0x0841);
        _gfx->drawString("[PROFIL POLA] Putar: Ganti Preset Mobil / Rekaman | Klik: Run/Stop", 16, 289);
    }
}

void PageDashboard::onEncoderTurn(int32_t delta, uint8_t editRow,
                                  EcuEngine::EngineRuntimeState& state,
                                  EcuEngine::ParametricWheel& wheel,
                                  EcuEngine::CamEventTable& cam) {
    if (editRow == 0) {
        int32_t newRpm = static_cast<int32_t>(state.targetRpm) + (delta * 50);
        state.targetRpm = constrain(newRpm, 100, 12000);
    } else if (editRow == 1) {
        int32_t m = static_cast<int32_t>(state.runMode) + (delta > 0 ? 1 : -1);
        if (m < 0) m = 2;
        if (m > 2) m = 0;
        state.runMode = static_cast<EcuEngine::EngineRunMode>(m);
    } else if (editRow == 2) {
        size_t count = s_hasCaptured ? (BASE_PRESET_COUNT + 1) : BASE_PRESET_COUNT;
        int32_t nextIdx = _activePresetIdx + (delta > 0 ? 1 : -1);
        if (nextIdx < 0) nextIdx = count - 1;
        if (nextIdx >= (int32_t)count) nextIdx = 0;
        _activePresetIdx = nextIdx;
        _applyPreset(_activePresetIdx, wheel, cam);
        const char* pName = (_activePresetIdx < (int32_t)BASE_PRESET_COUNT) ? PRESETS[_activePresetIdx].name : s_capturedPreset.name;
        strncpy(state.activeWheelName, pName, sizeof(state.activeWheelName));
    }
}

} // namespace EcuUi
