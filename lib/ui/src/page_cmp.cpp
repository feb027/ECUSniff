#include "page_cmp.h"
#include <stdio.h>
#include <string.h>

namespace EcuUi {

PageCmp::PageCmp(LovyanGFX* gfx) : _gfx(gfx), _canvas(gfx) {}

void PageCmp::init() {
    _canvas.init(448, 76);
}

void PageCmp::_drawPanel(int32_t x, int32_t y, int32_t w, int32_t h, bool isSelected) {
    uint16_t borderCol = isSelected ? 0xFFE0 : 0x52AA;
    _gfx->drawRoundRect(x, y, w, h, 6, borderCol);
    if (isSelected) {
        _gfx->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, 0xFFE0);
    } else {
        _gfx->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, 0x0841);
    }
}

void PageCmp::render(uint8_t activePresetIdx, 
                     const EcuEngine::EngineRuntimeState& state,
                     const EcuEngine::ParametricWheel& wheel,
                     const EcuEngine::CamEventTable& cam, 
                     bool isEditMode, uint8_t selectedItem, bool fullRedraw) {
    bool dbPreset = (activePresetIdx < WheelDatabase::getWheelCount());
    const WheelDefinition* def = dbPreset ? WheelDatabase::getWheel(activePresetIdx) : nullptr;

    uint32_t activeRpm = (state.runMode == EcuEngine::EngineRunMode::Potentiometer) ? 
                         state.potRpm : 
                         (state.isRunning ? state.currentRpm : state.targetRpm);

    int8_t curAdv = state.vvt.currentAdvanceDeg;

    if (fullRedraw) {
        _gfx->fillRect(0, 40, 480, 280, 0x0841);
        _gfx->fillRoundRect(8, 44, 464, 268, 8, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        // Header Waveform Canvas (Live Dual-Track CKP + CMP with sliding animation)
        if (def) _canvas.render(def, 16, 48, curAdv);
        else _canvas.render(wheel, cam, 16, 48, curAdv);

        // Panel 1: Manual Rotary VVT Shifter (W = 448, Y = 128, H = 54)
        _gfx->fillRoundRect(16, 128, 448, 54, 6, 0x0841);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("1. MANUAL VVT ROTARY SHIFTER (PUTAR ROTARY UNTUK GESER FASA):", 24, 133);

        // Panel 2: VVT Status (W = 220, Y = 188, H = 54)
        _gfx->fillRoundRect(16, 188, 220, 54, 6, 0x0841);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("2. SIMULASI VVT-i:", 24, 193);

        // Panel 3: Live RPM (W = 220, Y = 188, H = 54)
        _gfx->fillRoundRect(244, 188, 220, 54, 6, 0x0841);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("3. PUTARAN MESIN (RPM):", 252, 193);

        // Panel 4: Live Alignment & Safety Clearance (W = 448, Y = 248, H = 58)
        _gfx->fillRoundRect(16, 248, 448, 58, 6, 0x0841);
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("4. MONITOR POSISI SUDUT NOKEN AS TERHADAP CELAH KRUK AS:", 24, 253);

        _lastPresetIdx = activePresetIdx;
        _lastDrawnItem = 0xFF;
        _lastVvtAdv = 0x7F;
        _lastRpm = 0xFFFFFFFF;
        _lastVvtEnabled = !state.vvt.enabled;
    }

    // Dynamic Waveform Update when VVT advance shifts
    if (curAdv != _lastVvtAdv && !fullRedraw) {
        if (def) _canvas.render(def, 16, 48, curAdv);
        else _canvas.render(wheel, cam, 16, 48, curAdv);
    }

    // Panel 1 Value: Live VVT Advance Angle
    if (curAdv != _lastVvtAdv || fullRedraw) {
        _gfx->fillRect(24, 147, 432, 28, 0x0841);
        char advStr[48];
        if (curAdv > 0) snprintf(advStr, sizeof(advStr), "+%d DEG (MAJU / ADVANCED)", (int)curAdv);
        else if (curAdv < 0) snprintf(advStr, sizeof(advStr), "%d DEG (MUNDUR / RETARDED)", (int)curAdv);
        else snprintf(advStr, sizeof(advStr), "0 DEG (STANDBY / BASELINE IDLE)");

        _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(advStr, 24, 149);

        // Panel 4 Value: Diagnostic Clearance
        _gfx->fillRect(24, 269, 432, 30, 0x0841);
        char diagStr[64];
        float baseCam = 185.0f;
        float liveCam = baseCam - (float)curAdv;
        float clearance = liveCam - 165.0f; // Distance from gap 2 (165 deg)
        if (clearance < 0) clearance = -clearance;

        snprintf(diagStr, sizeof(diagStr), "Fasa Cam 1: %.1f deg | Jarak ke Celah Gap 2: %.1f deg (AMAN)", liveCam, clearance);
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString(diagStr, 24, 271);
        _gfx->setTextColor(0xCE79, 0x0841);
        _gfx->drawString("Putar knop rotari untuk melihat pulsa hijau noken as bergerak di layar", 24, 287);

        _lastVvtAdv = curAdv;
    }

    // Panel 2 Value: VVT Mode
    if (state.vvt.mode != _lastVvtMode || state.vvt.enabled != _lastVvtEnabled || fullRedraw) {
        _gfx->fillRect(24, 207, 204, 28, 0x0841);
        const char* eStr = !state.vvt.enabled ? "[ NONAKTIF ]" : 
                           (state.vvt.mode == EcuEngine::VvtMode::Manual ? "[ MANUAL ROTARI ]" : "[ AUTO RPM ]");
        uint16_t col = !state.vvt.enabled ? 0xF800 : 
                       (state.vvt.mode == EcuEngine::VvtMode::Manual ? 0xFFE0 : 0x07E0);
        _gfx->setTextColor(col, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(eStr, 24, 209);
        _lastVvtEnabled = state.vvt.enabled;
        _lastVvtMode = state.vvt.mode;
    }

    // Panel 3 Value: Live RPM
    if (activeRpm != _lastRpm || fullRedraw) {
        _gfx->fillRect(252, 207, 204, 28, 0x0841);
        char rpmBuf[24];
        const char* mStr = (state.runMode == EcuEngine::EngineRunMode::Potentiometer) ? "POT" : "FIX";
        snprintf(rpmBuf, sizeof(rpmBuf), "%u RPM (%s)", (unsigned)activeRpm, mStr);
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(rpmBuf, 252, 209);
        _lastRpm = activeRpm;
    }

    // Border highlights on selection change
    if (selectedItem != _lastDrawnItem || fullRedraw) {
        _drawPanel(16, 128, 448, 54, selectedItem == 0);
        _drawPanel(16, 188, 220, 54, selectedItem == 1);
        _drawPanel(244, 188, 220, 54, selectedItem == 2);
        _drawPanel(16, 248, 448, 58, selectedItem == 3);
        _lastDrawnItem = selectedItem;
    }
}

void PageCmp::onEncoderTurn(int32_t delta, uint8_t selectedItem, EcuEngine::EngineRuntimeState& state) {
    if (selectedItem == 0 || selectedItem == 255) {
        // Manual VVT Phase Shifter: langsung ubah derajat pergeseran real-time
        state.vvt.mode = EcuEngine::VvtMode::Manual;
        state.vvt.enabled = true;
        int32_t val = (int32_t)state.vvt.manualAdvanceDeg + (delta * 1);
        state.vvt.manualAdvanceDeg = (int8_t)constrain(val, -45, 55);
        state.vvt.currentAdvanceDeg = state.vvt.manualAdvanceDeg;
        state.vvt.maxAdvanceDeg = state.vvt.manualAdvanceDeg;
    } else if (selectedItem == 1) {
        if (delta != 0) {
            state.vvt.mode = (state.vvt.mode == EcuEngine::VvtMode::Manual) ? EcuEngine::VvtMode::AutoRpm : EcuEngine::VvtMode::Manual;
        }
    } else if (selectedItem == 2) {
        int32_t val = (int32_t)state.vvt.startRpm + (delta * 50);
        state.vvt.startRpm = (uint32_t)constrain(val, 1000, 4500);
    }
}

void PageCmp::onEncoderClick(uint8_t selectedItem, EcuEngine::EngineRuntimeState& state) {
    if (selectedItem == 1 || selectedItem == 0) {
        state.vvt.mode = (state.vvt.mode == EcuEngine::VvtMode::Manual) ? EcuEngine::VvtMode::AutoRpm : EcuEngine::VvtMode::Manual;
    }
}

} // namespace EcuUi
