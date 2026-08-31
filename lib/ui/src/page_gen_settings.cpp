#include "page_gen_settings.h"
#include <stdio.h>

namespace EcuUi {

static const uint32_t STEP_PRESETS[] = { 1, 10, 50, 100, 250, 500, 1000 };
static constexpr uint8_t TOTAL_STEP_PRESETS = 7;

PageGenSettings::PageGenSettings(LovyanGFX* gfx) : _gfx(gfx) {}

void PageGenSettings::init() {
    _lastEditRow = 0xFF;
    _lastRpmStep = 0;
    _lastSweepMin = 0;
    _lastSweepMax = 0;
    _lastSweepRate = 0;
    _lastCrankRpm = 0;
    _lastInverted = false;
}

void PageGenSettings::_drawPanel(int32_t x, int32_t y, int32_t w, int32_t h, bool isSel) {
    _gfx->drawRoundRect(x, y, w, h, 6, isSel ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, isSel ? 0xFFE0 : 0x0841);
}

void PageGenSettings::render(bool fullRedraw, uint8_t editRow,
                            const EcuEngine::EngineRuntimeState& state,
                            const EcuEngine::ParametricWheel& wheel) {
    if (fullRedraw) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        _gfx->fillRoundRect(16, 50, 220, 72, 6, 0x0841);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("1. RPM STEP (ENCODER):", 24, 56);

        _gfx->fillRoundRect(244, 50, 220, 72, 6, 0x0841);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("2. SWEEP MIN RPM:", 252, 56);

        _gfx->fillRoundRect(16, 128, 220, 72, 6, 0x0841);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("3. SWEEP MAX RPM:", 24, 134);

        _gfx->fillRoundRect(244, 128, 220, 72, 6, 0x0841);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("4. SWEEP RATE (SPEED):", 252, 134);

        _gfx->fillRoundRect(16, 206, 220, 72, 6, 0x0841);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("5. CRANKING START RPM:", 24, 212);

        _gfx->fillRoundRect(244, 206, 220, 72, 6, 0x0841);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("6. CKP OUTPUT POLARITY:", 252, 212);

        _lastEditRow = 0xFF; _lastRpmStep = 0; _lastSweepMin = 0;
        _lastSweepMax = 0; _lastSweepRate = 0; _lastCrankRpm = 0;
    }

    uint32_t stepVal = (state.rpmStep > 0) ? state.rpmStep : 50;
    if (stepVal != _lastRpmStep || fullRedraw) {
        char buf[16]; snprintf(buf, sizeof(buf), "+/-%u RPM ", (unsigned)stepVal);
        _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(buf, 24, 76);
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("Kenaikan per step knob", 24, 102);
        _lastRpmStep = stepVal;
    }

    if (state.sweep.minRpm != _lastSweepMin || fullRedraw) {
        char buf[16]; snprintf(buf, sizeof(buf), "%u RPM   ", (unsigned)state.sweep.minRpm);
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(buf, 252, 76);
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("Batas Bawah Auto Sweep", 252, 102);
        _lastSweepMin = state.sweep.minRpm;
    }

    if (state.sweep.maxRpm != _lastSweepMax || fullRedraw) {
        char buf[16]; snprintf(buf, sizeof(buf), "%u RPM   ", (unsigned)state.sweep.maxRpm);
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(buf, 24, 154);
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("Batas Atas Auto Sweep", 24, 180);
        _lastSweepMax = state.sweep.maxRpm;
    }

    if (state.sweep.sweepRateRpmPerSec != _lastSweepRate || fullRedraw) {
        char buf[20]; snprintf(buf, sizeof(buf), "%u RPM/S  ", (unsigned)state.sweep.sweepRateRpmPerSec);
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(buf, 252, 154);
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("Kecepatan Sapuan RPM", 252, 180);
        _lastSweepRate = state.sweep.sweepRateRpmPerSec;
    }

    if (state.cranking.crankingRpm != _lastCrankRpm || fullRedraw) {
        char buf[16]; snprintf(buf, sizeof(buf), "%u RPM   ", (unsigned)state.cranking.crankingRpm);
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(buf, 24, 232);
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("RPM Saat Starter Mesin", 24, 258);
        _lastCrankRpm = state.cranking.crankingRpm;
    }

    if (wheel.inverted != _lastInverted || fullRedraw) {
        const char* pStr = wheel.inverted ? "INVERTED (Low) " : "NORMAL (High)  ";
        uint32_t pCol = wheel.inverted ? 0xF800 : 0x07E0;
        _gfx->setTextColor(pCol, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(pStr, 252, 232);
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("Logika Polaritas CKP", 252, 258);
        _lastInverted = wheel.inverted;
    }

    if (editRow != _lastEditRow || fullRedraw) {
        _drawPanel(16, 50, 220, 72, editRow == 0);
        _drawPanel(244, 50, 220, 72, editRow == 1);
        _drawPanel(16, 128, 220, 72, editRow == 2);
        _drawPanel(244, 128, 220, 72, editRow == 3);
        _drawPanel(16, 206, 220, 72, editRow == 4);
        _drawPanel(244, 206, 220, 72, editRow == 5);

        _gfx->fillRect(16, 284, 448, 22, 0x0841);
        _gfx->drawRoundRect(16, 284, 448, 22, 4, 0x31A6);
        _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("Joy-Y: Pilih Parameter | Putar: Ubah Nilai | Klik: Terapkan/Toggle", 24, 290);
        _lastEditRow = editRow;
    }
}

void PageGenSettings::onEncoderTurn(int32_t delta, uint8_t editRow,
                                   EcuEngine::EngineRuntimeState& state,
                                   EcuEngine::ParametricWheel& wheel) {
    if (editRow == 0) {
        int idx = 2;
        for (int i = 0; i < TOTAL_STEP_PRESETS; ++i) {
            if (STEP_PRESETS[i] == state.rpmStep) { idx = i; break; }
        }
        idx = (idx + (delta > 0 ? 1 : -1));
        if (idx < 0) idx = TOTAL_STEP_PRESETS - 1;
        if (idx >= TOTAL_STEP_PRESETS) idx = 0;
        state.rpmStep = STEP_PRESETS[idx];
    } else if (editRow == 1) {
        int32_t val = (int32_t)state.sweep.minRpm + (delta * 100);
        state.sweep.minRpm = (uint32_t)constrain(val, 0, 6000);
    } else if (editRow == 2) {
        int32_t val = (int32_t)state.sweep.maxRpm + (delta * 100);
        state.sweep.maxRpm = (uint32_t)constrain(val, 500, 12000);
    } else if (editRow == 3) {
        int32_t val = (int32_t)state.sweep.sweepRateRpmPerSec + (delta * 50);
        state.sweep.sweepRateRpmPerSec = (uint32_t)constrain(val, 50, 3000);
    } else if (editRow == 4) {
        int32_t val = (int32_t)state.cranking.crankingRpm + (delta * 25);
        state.cranking.crankingRpm = (uint32_t)constrain(val, 50, 1000);
    } else if (editRow == 5 && delta != 0) {
        wheel.inverted = !wheel.inverted;
    }
}

void PageGenSettings::onEncoderClick(uint8_t editRow,
                                    EcuEngine::EngineRuntimeState& state,
                                    EcuEngine::ParametricWheel& wheel) {
    if (editRow == 0) {
        onEncoderTurn(1, editRow, state, wheel);
    } else if (editRow == 5) {
        wheel.inverted = !wheel.inverted;
    }
}

} // namespace EcuUi
