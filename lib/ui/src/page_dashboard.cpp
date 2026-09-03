#include "page_dashboard.h"
#include "wheel_database.h"
#include "page_wheel_browser.h"

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

void PageDashboard::_drawRpmBar(uint32_t activeRpm, uint32_t minRpm, uint32_t maxRpm, bool fullRedraw) {
    if (maxRpm <= minRpm) {
        minRpm = 0;
        maxRpm = 12000;
    }

    int32_t targetW = 0;
    if (activeRpm <= minRpm) {
        targetW = 0;
    } else if (activeRpm >= maxRpm) {
        targetW = 444;
    } else {
        targetW = (int32_t)(((uint64_t)(activeRpm - minRpm) * 444ULL) / (uint64_t)(maxRpm - minRpm));
    }
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
    uint32_t activeRpm = (state.runMode == EcuEngine::EngineRunMode::Potentiometer) ? 
                         state.potRpm : 
                         (state.isRunning ? state.currentRpm : state.targetRpm);
    uint8_t curMode = static_cast<uint8_t>(state.runMode);

    uint32_t modeMinRpm = (state.runMode == EcuEngine::EngineRunMode::Potentiometer) ? state.potCfg.minRpm :
                          ((state.runMode == EcuEngine::EngineRunMode::FixedRpm) ? state.fixEnc.minRpm : state.sweep.minRpm);
    uint32_t modeMaxRpm = (state.runMode == EcuEngine::EngineRunMode::Potentiometer) ? state.potCfg.maxRpm :
                          ((state.runMode == EcuEngine::EngineRunMode::FixedRpm) ? state.fixEnc.maxRpm : state.sweep.maxRpm);

    if (fullRedraw) {
        _gfx->fillRect(0, 40, 480, 280, 0x0841);
        _gfx->fillRoundRect(8, 44, 464, 268, 8, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);
        if (_activePresetIdx < WheelDatabase::getWheelCount()) {
            const WheelDefinition* def = WheelDatabase::getWheel(_activePresetIdx);
            if (def) _canvas.render(def, 16, 48, state.vvt.currentAdvanceDeg);
            else _canvas.render(wheel, cam, 16, 48, state.vvt.currentAdvanceDeg);
        } else {
            _canvas.render(wheel, cam, 16, 48, state.vvt.currentAdvanceDeg);
        }

        _drawRpmBar(activeRpm, modeMinRpm, modeMaxRpm, true);

        // Quadrant 1: Target RPM (Ambil 1/2 Layar Kiri: W = 220 px)
        _gfx->fillRoundRect(16, 148, 220, 78, 6, 0x0841);
        _gfx->drawRoundRect(16, 148, 220, 78, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("TARGET RPM MESIN:", 26, 154);

        // Quadrant 2: Master Control Panel (Ambil sisa 1/2 Layar Kanan: W = 220 px)
        _gfx->fillRoundRect(244, 148, 220, 78, 6, 0x0841);
        _gfx->drawRoundRect(244, 148, 220, 78, 6, 0x52AA);

        // Quadrant 3: Full-Width Pola Roda / Preset Card
        _gfx->fillRoundRect(16, 232, 448, 76, 6, 0x0841);
        _gfx->drawRoundRect(16, 232, 448, 76, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("POLA RODA & PROFIL MESIN AKTIF (KLIK UNTUK BROWSER):", 24, 236);

        _lastRpm = 0xFFFFFFFF; _lastMode = 0xFF;
        _lastIsRunning = !state.isRunning; _lastIsEditMode = !isEditMode;
        _lastEditRow = 0xFF; _lastTotalTeeth = 0xFFFF;
        _lastCkpEn = !state.ckpEnabled; _lastCmp1En = !state.cmp1Enabled;
        _lastCmp2En = !state.cmp2Enabled; _lastInverted = !wheel.inverted;
        _lastVvtAdv = state.vvt.currentAdvanceDeg;
    }

    bool isRunningChanged = (state.isRunning != _lastIsRunning);
    bool rpmChanged = (activeRpm != _lastRpm);
    bool modeChanged = (curMode != _lastMode);
    bool editChanged = (editRow != _lastEditRow);
    bool vvtChanged = (state.vvt.currentAdvanceDeg != _lastVvtAdv);

    if (vvtChanged && !fullRedraw) {
        if (_activePresetIdx < WheelDatabase::getWheelCount()) {
            const WheelDefinition* def = WheelDatabase::getWheel(_activePresetIdx);
            if (def) _canvas.render(def, 16, 48, state.vvt.currentAdvanceDeg);
            else _canvas.render(wheel, cam, 16, 48, state.vvt.currentAdvanceDeg);
        } else {
            _canvas.render(wheel, cam, 16, 48, state.vvt.currentAdvanceDeg);
        }
        _lastVvtAdv = state.vvt.currentAdvanceDeg;
    }

    if (rpmChanged || isRunningChanged || modeChanged || fullRedraw) {
        _drawRpmBar(activeRpm, modeMinRpm, modeMaxRpm, fullRedraw || modeChanged);
        _gfx->fillRect(22, 166, 208, 52, 0x0841);
        char rpmStr[8]; snprintf(rpmStr, sizeof(rpmStr), "%u", (unsigned)activeRpm);
        _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(4); 
        _gfx->drawRightString(rpmStr, 166, 172);
        _gfx->setTextSize(2); _gfx->setTextColor(0x07FF, 0x0841); 
        _gfx->drawString("RPM", 174, 184);
    }

    // ====================================================================
    // PANEL KANAN (1/2 LAYAR): MASTER POT/STOPPED (BESAR) + 4 TOMBOL SINYAL
    // ====================================================================
    bool sigChanged = (state.ckpEnabled != _lastCkpEn || 
                       state.cmp1Enabled != _lastCmp1En || 
                       state.cmp2Enabled != _lastCmp2En || 
                       wheel.inverted != _lastInverted ||
                       modeChanged || isRunningChanged ||
                       editChanged || fullRedraw);

    if (sigChanged) {
        // --- BARIS ATAS (MODE & MASTER RUN/STOP LEBIH BESAR, Y: 154, H: 34) ---
        // 1. Tombol Mode (Row 1, X: 248, Y: 154, W: 82, H: 34)
        bool modeSel = (editRow == 1);
        const char* modeNames[] = { "FIX", "POT", "SWEEP", "CRK FIX", "CRK SWP" };
        const char* curModeName = modeNames[curMode % 5];
        uint16_t modeBg = modeSel ? 0xFFE0 : ((curMode == 1) ? 0x2104 : 0x18C3);
        uint16_t modeBorder = modeSel ? 0xFFE0 : ((curMode == 1) ? 0x07E0 : 0x52AA);
        uint16_t modeFg = modeSel ? TFT_BLACK : ((curMode == 1) ? 0xFFE0 : 0x07E0);
        _gfx->fillRoundRect(248, 154, 82, 34, 5, modeBg);
        _gfx->drawRoundRect(248, 154, 82, 34, 5, modeBorder);
        if (modeSel) _gfx->drawRoundRect(249, 155, 80, 32, 4, 0xFFE0);
        _gfx->setTextColor(modeFg, modeBg);
        if (strlen(curModeName) > 5) {
            _gfx->setTextSize(1);
            _gfx->drawCenterString(curModeName, 289, 167);
        } else {
            _gfx->setTextSize(2);
            _gfx->drawCenterString(curModeName, 289, 163);
        }

        // 2. Tombol Master RUN / STOP (Row 2, X: 334, Y: 154, W: 126, H: 34)
        bool mtrSel = (editRow == 2);
        uint16_t mtrBg = state.isRunning ? 0x03E0 : 0xF800;
        uint16_t mtrBorder = mtrSel ? 0xFFE0 : (state.isRunning ? 0x07E0 : 0xF800);
        _gfx->fillRoundRect(334, 154, 126, 34, 5, mtrBg);
        _gfx->drawRoundRect(334, 154, 126, 34, 5, mtrBorder);
        if (mtrSel) _gfx->drawRoundRect(335, 155, 124, 32, 4, 0xFFE0);
        _gfx->setTextColor(TFT_WHITE, mtrBg); _gfx->setTextSize(2);
        _gfx->drawCenterString(state.isRunning ? "RUNNING" : "STOP", 397, 163);

        // --- BARIS BAWAH: 4 TOMBOL (CKP, CMP, CMP2, POL) DI BAWAH KOTAK MASTER (Y: 194, H: 26) ---
        // 3. Tombol CKP (Row 3, X: 248, Y: 194, W: 50, H: 26)
        bool ckpSel = (editRow == 3);
        uint16_t ckpBg = ckpSel ? 0xFFE0 : (state.ckpEnabled ? 0x03E0 : 0x2104);
        uint16_t ckpBorder = ckpSel ? 0xFFE0 : (state.ckpEnabled ? 0x07E0 : 0x52AA);
        uint16_t ckpFg = ckpSel ? TFT_BLACK : (state.ckpEnabled ? TFT_WHITE : 0x8410);
        _gfx->fillRoundRect(248, 194, 50, 26, 4, ckpBg);
        _gfx->drawRoundRect(248, 194, 50, 26, 4, ckpBorder);
        if (ckpSel) _gfx->drawRoundRect(249, 195, 48, 24, 3, 0xFFE0);
        _gfx->setTextColor(ckpFg, ckpBg); _gfx->setTextSize(1);
        _gfx->drawCenterString(state.ckpEnabled ? "CKP:ON" : "CKP:OFF", 273, 202);

        // 4. Tombol CMP1 (Row 4, X: 301, Y: 194, W: 52, H: 26)
        bool cmp1Sel = (editRow == 4);
        uint16_t cmp1Bg = cmp1Sel ? 0xFFE0 : (state.cmp1Enabled ? 0x03E0 : 0x2104);
        uint16_t cmp1Border = cmp1Sel ? 0xFFE0 : (state.cmp1Enabled ? 0x07E0 : 0x52AA);
        uint16_t cmp1Fg = cmp1Sel ? TFT_BLACK : (state.cmp1Enabled ? TFT_WHITE : 0x8410);
        _gfx->fillRoundRect(301, 194, 52, 26, 4, cmp1Bg);
        _gfx->drawRoundRect(301, 194, 52, 26, 4, cmp1Border);
        if (cmp1Sel) _gfx->drawRoundRect(302, 195, 50, 24, 3, 0xFFE0);
        _gfx->setTextColor(cmp1Fg, cmp1Bg); _gfx->setTextSize(1);
        _gfx->drawCenterString(state.cmp1Enabled ? "CMP:ON" : "CMP:OFF", 327, 202);

        // 5. Tombol CMP2 (Row 5, X: 356, Y: 194, W: 54, H: 26)
        bool cmp2Sel = (editRow == 5);
        uint16_t cmp2Bg = cmp2Sel ? 0xFFE0 : (state.cmp2Enabled ? 0x03E0 : 0x2104);
        uint16_t cmp2Border = cmp2Sel ? 0xFFE0 : (state.cmp2Enabled ? 0x07E0 : 0x52AA);
        uint16_t cmp2Fg = cmp2Sel ? TFT_BLACK : (state.cmp2Enabled ? TFT_WHITE : 0x8410);
        _gfx->fillRoundRect(356, 194, 54, 26, 4, cmp2Bg);
        _gfx->drawRoundRect(356, 194, 54, 26, 4, cmp2Border);
        if (cmp2Sel) _gfx->drawRoundRect(357, 195, 52, 24, 3, 0xFFE0);
        _gfx->setTextColor(cmp2Fg, cmp2Bg); _gfx->setTextSize(1);
        _gfx->drawCenterString(state.cmp2Enabled ? "CM2:ON" : "CM2:OFF", 383, 202);

        // 6. Tombol POL (Row 6, X: 413, Y: 194, W: 47, H: 26)
        bool polSel = (editRow == 6);
        uint16_t polBg = wheel.inverted ? 0xF800 : 0x07E0;
        uint16_t polBorder = polSel ? 0xFFE0 : (wheel.inverted ? 0xFDE0 : 0x03E0);
        _gfx->fillRoundRect(413, 194, 47, 26, 4, polBg);
        _gfx->drawRoundRect(413, 194, 47, 26, 4, polBorder);
        if (polSel) _gfx->drawRoundRect(414, 195, 45, 24, 3, 0xFFE0);
        _gfx->setTextColor(wheel.inverted ? TFT_WHITE : TFT_BLACK, polBg); _gfx->setTextSize(1);
        _gfx->drawCenterString(wheel.inverted ? "INV" : "NORM", 436, 202);

        _lastCkpEn = state.ckpEnabled;
        _lastCmp1En = state.cmp1Enabled;
        _lastCmp2En = state.cmp2Enabled;
        _lastInverted = wheel.inverted;
    }

    // ====================================================================
    // PANEL BAWAH: POLA RODA MEMANJANG & BADGE HARDWARE MEMANJANG
    // ====================================================================
    static bool s_lastSta = false;
    static bool s_lastChg = false;
    static char s_lastWheelName[48]{""};
    static uint8_t s_lastCamCount = 0xFF;
    static uint8_t s_lastVvtAdv = 0xFF;
    static bool s_lastVvtEn = false;

    bool staChanged = (state.staActive != s_lastSta);
    bool chgChanged = (state.chgLampOn != s_lastChg);
    bool nameChanged = (strcmp(state.activeWheelName, s_lastWheelName) != 0);
    bool camChanged = (cam.getEventCount() != s_lastCamCount);
    bool vvtBadgeChanged = (state.vvt.currentAdvanceDeg != s_lastVvtAdv || state.vvt.enabled != s_lastVvtEn);

    s_lastSta = state.staActive;
    s_lastChg = state.chgLampOn;
    if (nameChanged) {
        strncpy(s_lastWheelName, state.activeWheelName, sizeof(s_lastWheelName));
    }
    s_lastCamCount = cam.getEventCount();

    if (wheel.totalTeeth != _lastTotalTeeth || wheel.missingTeeth != _lastMissingTeeth || 
        nameChanged || camChanged || staChanged || chgChanged || vvtBadgeChanged || fullRedraw) {
        const char* name = (state.activeWheelName[0] != '\0') ? state.activeWheelName : "Pola Kustom";
        
        // Hapus area teks nama preset sebelah kiri (X: 22 s/d 362)
        _gfx->fillRect(22, 248, 340, 54, 0x0841);

        char displayTitle[36];
        if (strlen(name) > 28) {
            strncpy(displayTitle, name, 26);
            displayTitle[26] = '.';
            displayTitle[27] = '.';
            displayTitle[28] = '\0';
        } else {
            strncpy(displayTitle, name, sizeof(displayTitle) - 1);
            displayTitle[sizeof(displayTitle) - 1] = '\0';
        }

        _gfx->setTextColor(0x07E0, 0x0841); 
        _gfx->setTextSize(2);
        _gfx->drawString(displayTitle, 24, 252);

        char detailBuf[54];
        if (_activePresetIdx < WheelDatabase::getWheelCount()) {
            const WheelDefinition* def = WheelDatabase::getWheel(_activePresetIdx);
            if (def) {
                const char* channelStr = def->hasCmp2 ? "CKP+CMP1+CMP2" : (def->hasCmp1 ? "CKP+CMP1" : "CKP Only");
                const char* cycleStr = (def->cycleDegrees == WheelCycleDegrees::CRANK_360) ? "360d" : "720d";
                snprintf(detailBuf, sizeof(detailBuf), "(%s | %u Edges | %s)", cycleStr, def->totalEdges, channelStr);
            } else {
                snprintf(detailBuf, sizeof(detailBuf), "(%u-%u CKP | %u Cam)",
                         (unsigned)wheel.totalTeeth, (unsigned)wheel.missingTeeth, (unsigned)cam.getEventCount());
            }
        } else {
            snprintf(detailBuf, sizeof(detailBuf), "(%u-%u CKP | %u Cam)",
                     (unsigned)wheel.totalTeeth, (unsigned)wheel.missingTeeth, (unsigned)cam.getEventCount());
        }
        _gfx->setTextColor(0x07FF, 0x0841); 
        _gfx->setTextSize(1);
        _gfx->drawString(detailBuf, 24, 278);

        // ====================================================================
        // BADGE STATUS HARDWARE DI POJOK KANAN BAWAH (CRK, ALT, & VVT-i)
        // ====================================================================
        // 1. Badge CRK (Starter Crank) X: 366, Y: 248, W: 43, H: 24
        uint16_t staBg = state.staActive ? 0xFFE0 : 0x18C3;
        uint16_t staBorder = state.staActive ? 0xFFE0 : 0x3186;
        uint16_t staFg = state.staActive ? TFT_BLACK : 0x8410;
        _gfx->fillRoundRect(366, 248, 43, 24, 4, staBg);
        _gfx->drawRoundRect(366, 248, 43, 24, 4, staBorder);
        _gfx->setTextColor(staFg, staBg); _gfx->setTextSize(1);
        _gfx->drawCenterString(state.staActive ? "STA" : "CRK", 387, 256);

        // 2. Badge ALT (Alternator Charging) X: 413, Y: 248, W: 43, H: 24
        uint16_t altBg = state.chgLampOn ? 0xF800 : 0x03E0;
        uint16_t altBorder = state.chgLampOn ? 0xFDE0 : 0x07E0;
        uint16_t altFg = state.chgLampOn ? TFT_WHITE : 0x07E0;
        _gfx->fillRoundRect(413, 248, 43, 24, 4, altBg);
        _gfx->drawRoundRect(413, 248, 43, 24, 4, altBorder);
        _gfx->setTextColor(altFg, altBg); _gfx->setTextSize(1);
        _gfx->drawCenterString(state.chgLampOn ? "BAT" : "CHG", 434, 256);

        // 3. Dynamic VVT-i Advance Badge X: 366, Y: 276, W: 90, H: 24
        char vvtBuf[16];
        uint16_t vvtBg, vvtBorder, vvtFg;
        if (!state.vvt.enabled) {
            snprintf(vvtBuf, sizeof(vvtBuf), "VVT: OFF");
            vvtBg = 0x18C3; vvtBorder = 0x3186; vvtFg = 0x8410;
        } else if (state.vvt.currentAdvanceDeg > 0) {
            snprintf(vvtBuf, sizeof(vvtBuf), "VVT: +%udeg", (unsigned)state.vvt.currentAdvanceDeg);
            vvtBg = 0x03E0; vvtBorder = 0xFFE0; vvtFg = 0xFFE0;
        } else {
            snprintf(vvtBuf, sizeof(vvtBuf), "VVT: 0deg");
            vvtBg = 0x18C3; vvtBorder = 0x07E0; vvtFg = 0x07FF;
        }
        _gfx->fillRoundRect(366, 276, 90, 24, 4, vvtBg);
        _gfx->drawRoundRect(366, 276, 90, 24, 4, vvtBorder);
        _gfx->setTextColor(vvtFg, vvtBg); _gfx->setTextSize(1);
        _gfx->drawCenterString(vvtBuf, 411, 284);

        s_lastVvtAdv = state.vvt.currentAdvanceDeg;
        s_lastVvtEn = state.vvt.enabled;

        _lastTotalTeeth = wheel.totalTeeth; _lastMissingTeeth = wheel.missingTeeth;
        if (!fullRedraw) {
            if (_activePresetIdx < WheelDatabase::getWheelCount()) {
                const WheelDefinition* def = WheelDatabase::getWheel(_activePresetIdx);
                if (def) _canvas.render(def, 16, 48, state.vvt.currentAdvanceDeg);
                else _canvas.render(wheel, cam, 16, 48, state.vvt.currentAdvanceDeg);
            } else {
                _canvas.render(wheel, cam, 16, 48, state.vvt.currentAdvanceDeg);
            }
        }
    }

    if (editChanged || isRunningChanged || fullRedraw) {
        _drawEditFrames(isEditMode, editRow, state.isRunning, wheel);
    }

    _lastRpm = activeRpm; _lastIsRunning = state.isRunning; _lastMode = curMode;
    _lastIsEditMode = isEditMode; _lastEditRow = editRow;
}

void PageDashboard::_drawEditFrames(bool isEditMode, uint8_t editRow, bool isRunning, const EcuEngine::ParametricWheel& wheel) {
    // Frame 0: Target RPM Box (1/2 Layar Kiri: W = 220 px)
    _gfx->drawRoundRect(16, 148, 220, 78, 6, (editRow == 0) ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(17, 149, 218, 76, 5, (editRow == 0) ? 0xFFE0 : 0x0841);

    // Frame Outer Master Panel (1/2 Layar Kanan: W = 220 px) - MENYALA BERSAMAAN SAAT EDITROW 1 s/d 6 DIPILIH!
    bool rightSel = (editRow >= 1 && editRow <= 6);
    _gfx->drawRoundRect(244, 148, 220, 78, 6, rightSel ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(245, 149, 218, 76, 5, rightSel ? 0xFFE0 : 0x0841);

    // Frame 1: Mode Button
    bool modeSel = (editRow == 1);
    _gfx->drawRoundRect(248, 154, 82, 34, 5, modeSel ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(249, 155, 80, 32, 4, modeSel ? 0xFFE0 : 0x18C3);

    // Frame 2: Master Button
    bool mtrSel = (editRow == 2);
    _gfx->drawRoundRect(334, 154, 126, 34, 5, mtrSel ? 0xFFE0 : (isRunning ? 0x07E0 : 0xF800));
    _gfx->drawRoundRect(335, 155, 124, 32, 4, mtrSel ? 0xFFE0 : (isRunning ? 0x03E0 : 0x9800));

    // Frame 3: CKP Button
    bool ckpSel = (editRow == 3);
    _gfx->drawRoundRect(248, 194, 50, 26, 4, ckpSel ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(249, 195, 48, 24, 3, ckpSel ? 0xFFE0 : 0x2104);

    // Frame 4: CMP1 Button
    bool cmp1Sel = (editRow == 4);
    _gfx->drawRoundRect(301, 194, 52, 26, 4, cmp1Sel ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(302, 195, 50, 24, 3, cmp1Sel ? 0xFFE0 : 0x2104);

    // Frame 5: CMP2 Button
    bool cmp2Sel = (editRow == 5);
    _gfx->drawRoundRect(356, 194, 54, 26, 4, cmp2Sel ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(357, 195, 52, 24, 3, cmp2Sel ? 0xFFE0 : 0x2104);

    // Frame 6: POL Button
    bool polSel = (editRow == 6);
    _gfx->drawRoundRect(413, 194, 47, 26, 4, polSel ? 0xFFE0 : (wheel.inverted ? 0xF800 : 0x07E0));
    _gfx->drawRoundRect(414, 195, 45, 24, 3, polSel ? 0xFFE0 : (wheel.inverted ? 0x9800 : 0x03E0));

    // Frame 7: Pola Wheel (Full-Width Bottom Card)
    _gfx->drawRoundRect(16, 232, 448, 76, 6, (editRow == 7) ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(17, 233, 446, 74, 5, (editRow == 7) ? 0xFFE0 : 0x0841);
}

void PageDashboard::onEncoderTurn(int32_t delta, uint8_t editRow,
                                  EcuEngine::EngineRuntimeState& state,
                                  EcuEngine::ParametricWheel& wheel,
                                  EcuEngine::CamEventTable& cam) {
    if (editRow == 0) {
        if (state.runMode == EcuEngine::EngineRunMode::Potentiometer) {
            return;
        }
        uint32_t step = (state.fixEnc.rpmStep > 0) ? state.fixEnc.rpmStep : ((state.rpmStep > 0) ? state.rpmStep : 50);
        int32_t newRpm = static_cast<int32_t>(state.targetRpm) + (delta * (int32_t)step);
        uint32_t minLimit = state.fixEnc.minRpm;
        uint32_t maxLimit = (state.fixEnc.maxRpm > minLimit) ? state.fixEnc.maxRpm : (minLimit + 500);
        state.targetRpm = constrain(newRpm, (int32_t)minLimit, (int32_t)maxLimit);
    } else if (editRow == 1) {
        int32_t m = static_cast<int32_t>(state.runMode) + (delta > 0 ? 1 : -1);
        if (m < 0) m = 4;
        if (m > 4) m = 0;
        state.runMode = static_cast<EcuEngine::EngineRunMode>(m);
    } else if (editRow == 2) {
        state.isRunning = !state.isRunning;
    } else if (editRow == 3 && delta != 0) {
        state.ckpEnabled = !state.ckpEnabled;
    } else if (editRow == 4 && delta != 0) {
        state.cmp1Enabled = !state.cmp1Enabled;
    } else if (editRow == 5 && delta != 0) {
        state.cmp2Enabled = !state.cmp2Enabled;
    } else if (editRow == 6 && delta != 0) {
        wheel.inverted = !wheel.inverted;
    } else if (editRow == 7) {
        _activePresetIdx = _getNextPresetInCategory(_activePresetIdx, delta > 0 ? 1 : -1, _activeCategory);
        _applyPreset(_activePresetIdx, wheel, cam);
        const char* pName = (_activePresetIdx < (int32_t)BASE_PRESET_COUNT) ? 
                            OEM_DATABASE_PRESETS[_activePresetIdx].name : 
                            s_customSlots[_activePresetIdx - BASE_PRESET_COUNT].name;
        strncpy(state.activeWheelName, pName, sizeof(state.activeWheelName));
    }
}

int32_t PageDashboard::_getNextPresetInCategory(int32_t currentIdx, int32_t direction, uint8_t category) {
    size_t count = BASE_PRESET_COUNT + s_customCount;
    if (count == 0) return 0;
    if (category == 0) { // All category
        int32_t next = currentIdx + direction;
        if (next < 0) next = count - 1;
        if (next >= (int32_t)count) next = 0;
        return next;
    }

    WheelCategory cat = static_cast<WheelCategory>(category);
    int32_t idx = currentIdx;
    for (size_t i = 0; i < count; ++i) {
        idx += direction;
        if (idx < 0) idx = count - 1;
        if (idx >= (int32_t)count) idx = 0;
        if (PageWheelBrowser::matchesCategory((uint16_t)idx, cat)) {
            return idx;
        }
    }
    return currentIdx;
}

} // namespace EcuUi
