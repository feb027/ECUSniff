#include "page_dashboard.h"
#include "wheel_database.h"

namespace EcuUi {

WheelPresetItem PageDashboard::s_customSlots[PageDashboard::MAX_CUSTOM_PRESETS]{};
uint8_t PageDashboard::s_customCount = 0;

static inline uint16_t getRpmGradientColor(int32_t px, int32_t totalW) {
    int32_t midW = (totalW * 55) / 100;
    uint8_t r = 0, g = 0;
    if (px <= midW) {
        r = (uint8_t)((px * 255) / midW);
        g = 255;
    } else {
        int32_t rem = px - midW;
        int32_t span = totalW - midW;
        r = 255;
        g = (span > 0) ? (uint8_t)(255 - ((rem * 255) / span)) : 0;
    }
    return ((r >> 3) << 11) | ((g >> 2) << 5);
}

PageDashboard::PageDashboard(LovyanGFX* gfx) : _gfx(gfx), _canvas(gfx) {}

void PageDashboard::init() {
    _canvas.init(448, 76);
}

uint8_t PageDashboard::addCapturedPreset(const char* name, const EcuEngine::ParametricWheel& wheel, const EcuEngine::CamEventTable& cam) {
    uint8_t slot = (s_customCount < MAX_CUSTOM_PRESETS) ? s_customCount : (MAX_CUSTOM_PRESETS - 1);
    WheelPresetItem& item = s_customSlots[slot];

    if (name && name[0] != '\0') {
        strncpy(item.name, name, sizeof(item.name) - 1);
    } else {
        snprintf(item.name, sizeof(item.name), "Capture %u", (unsigned)(slot + 1));
    }
    item.name[sizeof(item.name) - 1] = '\0';
    item.totalTeeth = wheel.totalTeeth;
    item.missingTeeth = wheel.missingTeeth;
    item.missingPosition = wheel.missingPosition;
    item.dutyCycle = wheel.dutyCycle;
    item.inverted = wheel.inverted;
    item.camCount = cam.getEventCount();
    const auto* evs = cam.getEvents();
    for (uint8_t i = 0; i < 4; ++i) {
        if (evs && i < item.camCount) {
            item.camAngles[i] = evs[i].angleDeg;
            item.camHighs[i] = evs[i].levelHigh;
        } else {
            item.camAngles[i] = 0.0f;
            item.camHighs[i] = false;
        }
    }
    if (s_customCount < MAX_CUSTOM_PRESETS) s_customCount++;
    return slot;
}

bool PageDashboard::renameCustomPreset(uint8_t slot, const char* newName) {
    if (slot >= s_customCount || !newName) return false;
    strncpy(s_customSlots[slot].name, newName, sizeof(s_customSlots[slot].name) - 1);
    s_customSlots[slot].name[sizeof(s_customSlots[slot].name) - 1] = '\0';
    return true;
}

bool PageDashboard::deleteCustomPreset(uint8_t slot) {
    if (slot >= s_customCount) return false;
    for (uint8_t i = slot; i + 1 < s_customCount; ++i) {
        s_customSlots[i] = s_customSlots[i + 1];
    }
    s_customCount--;
    return true;
}

uint8_t PageDashboard::getCustomCount() { return s_customCount; }

const WheelPresetItem* PageDashboard::getCustomPreset(uint8_t slot) {
    return (slot < s_customCount) ? &s_customSlots[slot] : nullptr;
}

void PageDashboard::clearAllCustom() { s_customCount = 0; }

void PageDashboard::setCustomSlot(uint8_t slot, const WheelPresetItem& item) {
    if (slot < MAX_CUSTOM_PRESETS) {
        s_customSlots[slot] = item;
        if (slot >= s_customCount) s_customCount = slot + 1;
    }
}

void PageDashboard::_applyPreset(uint8_t idx, EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam) {
    const WheelPresetItem* p = nullptr;
    if (idx < BASE_PRESET_COUNT) {
        p = &OEM_DATABASE_PRESETS[idx];
    } else if (idx < BASE_PRESET_COUNT + s_customCount) {
        p = &s_customSlots[idx - BASE_PRESET_COUNT];
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

void PageDashboard::_drawRpmBar(uint32_t activeRpm, bool fullRedraw) {
    constexpr uint32_t MAX_SCALE_RPM = 12000;
    int32_t targetW = (int32_t)(((uint64_t)activeRpm * 444) / MAX_SCALE_RPM);
    if (targetW > 444) targetW = 444;

    if (fullRedraw) {
        _gfx->fillRoundRect(16, 126, 448, 20, 4, 0x0841);
        _gfx->drawRoundRect(16, 126, 448, 20, 4, 0x52AA);
        _lastBarW = 0;
        for (int32_t x = 0; x < targetW; ++x) {
            _gfx->drawFastVLine(18 + x, 128, 16, getRpmGradientColor(x, 444));
        }
        _lastBarW = targetW;
        return;
    }

    if (targetW == _lastBarW) return;

    int32_t startX = 18;
    int32_t startY = 128;
    int32_t barH = 16;

    if (targetW > _lastBarW) {
        for (int32_t x = _lastBarW; x < targetW; ++x) {
            _gfx->drawFastVLine(startX + x, startY, barH, getRpmGradientColor(x, 444));
        }
    } else if (targetW < _lastBarW) {
        _gfx->fillRect(startX + targetW, startY, _lastBarW - targetW, barH, 0x0841);
    }

    _lastBarW = targetW;
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
        _canvas.render(wheel, cam, 16, 48);

        _drawRpmBar(activeRpm, true);

        _gfx->fillRoundRect(16, 150, 220, 76, 6, 0x0841);
        _gfx->drawRoundRect(16, 150, 220, 76, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("TARGET RPM MESIN:", 26, 156);

        _gfx->fillRoundRect(244, 150, 220, 76, 6, 0x0841);
        _gfx->drawRoundRect(244, 150, 220, 76, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("SIMULATION MODE:", 254, 156);

        _gfx->fillRoundRect(16, 230, 448, 76, 6, 0x0841);
        _gfx->drawRoundRect(16, 230, 448, 76, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("POLA RODA & PROFIL MESIN AKTIF:", 26, 236);

        _lastRpm = 0xFFFFFFFF; _lastMode = 0xFF;
        _lastIsRunning = !state.isRunning; _lastIsEditMode = !isEditMode;
        _lastEditRow = 0xFF; _lastTotalTeeth = 0xFFFF;
    }

    bool isRunningChanged = (state.isRunning != _lastIsRunning);
    bool rpmChanged = (activeRpm != _lastRpm);
    bool modeChanged = (curMode != _lastMode);
    bool editChanged = (editRow != _lastEditRow);

    if (rpmChanged || isRunningChanged || fullRedraw) {
        _drawRpmBar(activeRpm, fullRedraw);
        _gfx->fillRect(24, 172, 134, 38, 0x0841);
        char rpmStr[8]; snprintf(rpmStr, sizeof(rpmStr), "%04u", (unsigned)activeRpm);
        _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(4); _gfx->drawString(rpmStr, 26, 174);
        _gfx->setTextSize(2); _gfx->setTextColor(0x07FF, 0x0841); _gfx->drawString("RPM", 162, 186);
    }

    if (modeChanged || isRunningChanged || fullRedraw) {
        const char* modeNames[] = { "FIX", "SWEEP", "CRK>FIX", "CRK>SWP" };
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize((curMode >= 2) ? 2 : 3);
        _gfx->drawString("          ", 254, 178);
        _gfx->drawString(modeNames[curMode % 4], 254, (curMode >= 2) ? 182 : 178);

        if (state.isRunning) {
            _gfx->fillRoundRect(354, 166, 98, 44, 4, 0x03E0); _gfx->drawRoundRect(354, 166, 98, 44, 4, 0x07E0);
            _gfx->setTextColor(0x07E0, 0x03E0); _gfx->setTextSize(2); _gfx->drawCenterString("RUNNING", 403, 180);
        } else {
            _gfx->fillRoundRect(354, 166, 98, 44, 4, 0xF800); _gfx->drawRoundRect(354, 166, 98, 44, 4, 0xF800);
            _gfx->setTextColor(TFT_WHITE, 0xF800); _gfx->setTextSize(2); _gfx->drawCenterString("STOPPED", 403, 180);
        }
    }

    if (wheel.totalTeeth != _lastTotalTeeth || wheel.missingTeeth != _lastMissingTeeth || fullRedraw) {
        const char* name = (state.activeWheelName[0] != '\0') ? state.activeWheelName : "Pola Kustom";
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString("                             ", 26, 254);
        _gfx->drawString(name, 26, 254);

        char detailBuf[48];
        snprintf(detailBuf, sizeof(detailBuf), "(%u-%u CKP | %u Pulsa Cam)          ",
                 (unsigned)wheel.totalTeeth, (unsigned)wheel.missingTeeth, (unsigned)cam.getEventCount());
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString(detailBuf, 26, 280);

        _lastTotalTeeth = wheel.totalTeeth; _lastMissingTeeth = wheel.missingTeeth;
        if (!fullRedraw) _canvas.render(wheel, cam, 16, 48);
    }

    if (editChanged || isRunningChanged || fullRedraw) {
        _drawEditFrames(isEditMode, editRow, state.isRunning, wheel);
    }

    _lastRpm = activeRpm; _lastIsRunning = state.isRunning; _lastMode = curMode;
    _lastIsEditMode = isEditMode; _lastEditRow = editRow;
}

void PageDashboard::_drawEditFrames(bool isEditMode, uint8_t editRow, bool isRunning, const EcuEngine::ParametricWheel& wheel) {
    _gfx->drawRoundRect(16, 150, 220, 76, 6, (editRow == 0) ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(17, 151, 218, 74, 5, (editRow == 0) ? 0xFFE0 : 0x0841);

    _gfx->drawRoundRect(244, 150, 220, 76, 6, (editRow == 1) ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(245, 151, 218, 74, 5, (editRow == 1) ? 0xFFE0 : 0x0841);

    _gfx->drawRoundRect(16, 230, 448, 76, 6, (editRow == 2) ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(17, 231, 446, 74, 5, (editRow == 2) ? 0xFFE0 : 0x0841);
}

void PageDashboard::onEncoderTurn(int32_t delta, uint8_t editRow,
                                  EcuEngine::EngineRuntimeState& state,
                                  EcuEngine::ParametricWheel& wheel,
                                  EcuEngine::CamEventTable& cam) {
    if (editRow == 0) {
        uint32_t step = (state.rpmStep > 0) ? state.rpmStep : 50;
        int32_t newRpm = static_cast<int32_t>(state.targetRpm) + (delta * (int32_t)step);
        state.targetRpm = constrain(newRpm, 0, 12000);
    } else if (editRow == 1) {
        int32_t m = static_cast<int32_t>(state.runMode) + (delta > 0 ? 1 : -1);
        if (m < 0) m = 3;
        if (m > 3) m = 0;
        state.runMode = static_cast<EcuEngine::EngineRunMode>(m);
    } else if (editRow == 2) {
        size_t count = BASE_PRESET_COUNT + s_customCount;
        int32_t nextIdx = _activePresetIdx + (delta > 0 ? 1 : -1);
        if (nextIdx < 0) nextIdx = count - 1;
        if (nextIdx >= (int32_t)count) nextIdx = 0;
        _activePresetIdx = nextIdx;
        _applyPreset(_activePresetIdx, wheel, cam);
        const char* pName = (_activePresetIdx < (int32_t)BASE_PRESET_COUNT) ? OEM_DATABASE_PRESETS[_activePresetIdx].name : s_customSlots[_activePresetIdx - BASE_PRESET_COUNT].name;
        strncpy(state.activeWheelName, pName, sizeof(state.activeWheelName));
    }
}

} // namespace EcuUi
