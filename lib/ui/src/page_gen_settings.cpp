#include "page_gen_settings.h"
#include <stdio.h>

namespace EcuUi {

static const uint32_t STEP_PRESETS[] = { 1, 10, 50, 100, 250, 500, 1000 };
static constexpr uint8_t TOTAL_STEP_PRESETS = 7;

PageGenSettings::PageGenSettings(LovyanGFX* gfx) : _gfx(gfx) {}

void PageGenSettings::init() {
    _currentPage = 0;
    _lastDrawnPage = 0xFF;
    _lastEditRow = 0xFF;
    _lastRpmStep = 0;
    _lastCrankDur = 0;
    _lastCrankRpm = 0;
    _lastSweepRate = 0;
    _lastSweepMin = 0;
    _lastSweepMax = 0;
    _lastInverted = false;
}

void PageGenSettings::_drawPanel(int32_t x, int32_t y, int32_t w, int32_t h, bool isSel) {
    _gfx->drawRoundRect(x, y, w, h, 6, isSel ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, isSel ? 0xFFE0 : 0x0841);
}

void PageGenSettings::render(bool fullRedraw, uint8_t editRow,
                            const EcuEngine::EngineRuntimeState& state,
                            const EcuEngine::ParametricWheel& wheel) {
    uint8_t page = (editRow >= 5 && editRow <= 9) ? 1 : 0;
    _currentPage = page;
    bool pageChanged = (_currentPage != _lastDrawnPage);

    if (fullRedraw || pageChanged) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        if (_currentPage == 0) {
            _gfx->fillRoundRect(16, 50, 220, 94, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("1. RPM STEP (ENCODER):", 24, 58);

            _gfx->fillRoundRect(244, 50, 220, 94, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("2. SWEEP MIN RPM:", 252, 58);

            _gfx->fillRoundRect(16, 152, 220, 94, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("3. SWEEP MAX RPM:", 24, 160);

            _gfx->fillRoundRect(244, 152, 220, 94, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("4. SWEEP RATE (SPEED):", 252, 160);
        } else {
            _gfx->fillRoundRect(16, 50, 220, 94, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("5. CRANKING START RPM:", 24, 58);

            _gfx->fillRoundRect(244, 50, 220, 94, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("6. CRANK DURATION (WAKTU):", 252, 58);

            _gfx->fillRoundRect(16, 152, 220, 94, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("7. CKP OUTPUT POLARITY:", 24, 160);

            _gfx->fillRoundRect(244, 152, 220, 94, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("8. RESET FACTORY DEFAULTS:", 252, 160);
        }

        _lastEditRow = 0xFF; _lastRpmStep = 0; _lastCrankDur = 0;
        _lastCrankRpm = 0; _lastSweepRate = 0; _lastSweepMin = 0;
        _lastSweepMax = 0; _lastDrawnPage = _currentPage;
    }

    if (_currentPage == 0) {
        uint32_t stepVal = (state.rpmStep > 0) ? state.rpmStep : 50;
        if (stepVal != _lastRpmStep || fullRedraw || pageChanged) {
            char buf[16]; snprintf(buf, sizeof(buf), "+/-%u RPM ", (unsigned)stepVal);
            _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(3);
            _gfx->drawString(buf, 24, 82);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Kenaikan per step putaran knob", 24, 122);
            _lastRpmStep = stepVal;
        }

        if (state.sweep.minRpm != _lastSweepMin || fullRedraw || pageChanged) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM   ", (unsigned)state.sweep.minRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(3);
            _gfx->drawString(buf, 252, 82);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Batas Bawah Putaran Auto Sweep", 252, 122);
            _lastSweepMin = state.sweep.minRpm;
        }

        if (state.sweep.maxRpm != _lastSweepMax || fullRedraw || pageChanged) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM   ", (unsigned)state.sweep.maxRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(3);
            _gfx->drawString(buf, 24, 184);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Batas Atas Putaran Auto Sweep", 24, 224);
            _lastSweepMax = state.sweep.maxRpm;
        }

        if (state.sweep.sweepRateRpmPerSec != _lastSweepRate || fullRedraw || pageChanged) {
            char buf[20]; snprintf(buf, sizeof(buf), "%u RPM/S  ", (unsigned)state.sweep.sweepRateRpmPerSec);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(3);
            _gfx->drawString(buf, 252, 184);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Kecepatan Sapuan RPM Mesin", 252, 224);
            _lastSweepRate = state.sweep.sweepRateRpmPerSec;
        }
    } else {
        if (state.cranking.crankingRpm != _lastCrankRpm || fullRedraw || pageChanged) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM   ", (unsigned)state.cranking.crankingRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(3);
            _gfx->drawString(buf, 24, 82);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("RPM Awal Dinamo Starter", 24, 122);
            _lastCrankRpm = state.cranking.crankingRpm;
        }

        if (state.cranking.crankDurationMs != _lastCrankDur || fullRedraw || pageChanged) {
            char buf[16]; float sec = state.cranking.crankDurationMs / 1000.0f;
            snprintf(buf, sizeof(buf), "%.1f DETIK ", sec);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(3);
            _gfx->drawString(buf, 252, 82);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Waktu Tahan Starter Mesin", 252, 122);
            _lastCrankDur = state.cranking.crankDurationMs;
        }

        if (wheel.inverted != _lastInverted || fullRedraw || pageChanged) {
            const char* pStr = wheel.inverted ? "INVERTED (Low)" : "NORMAL (High) ";
            uint32_t pCol = wheel.inverted ? 0xF800 : 0x07E0;
            _gfx->setTextColor(pCol, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(pStr, 24, 188);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Logika Polaritas Sinyal CKP", 24, 224);
            _lastInverted = wheel.inverted;
        }

        if (fullRedraw || pageChanged) {
            _gfx->setTextColor(0xF800, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString("[ KLIK RESET ]", 252, 188);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Kembalikan Semua Default", 252, 224);
        }
    }

    if (editRow != _lastEditRow || fullRedraw || pageChanged) {
        if (_currentPage == 0) {
            _drawPanel(16, 50, 220, 94, editRow == 0);
            _drawPanel(244, 50, 220, 94, editRow == 1);
            _drawPanel(16, 152, 220, 94, editRow == 2);
            _drawPanel(244, 152, 220, 94, editRow == 3);

            _gfx->fillRoundRect(16, 256, 220, 48, 6, 0x07E0);
            _gfx->setTextColor(TFT_BLACK, 0x07E0); _gfx->setTextSize(2);
            _gfx->drawCenterString("<< HALAMAN 1", 126, 270);

            _gfx->fillRoundRect(244, 256, 220, 48, 6, 0x18C3);
            _gfx->drawRoundRect(244, 256, 220, 48, 6, (editRow == 4) ? 0xFFE0 : 0x52AA);
            if (editRow == 4) _gfx->drawRoundRect(245, 257, 218, 46, 5, 0xFFE0);
            _gfx->setTextColor(TFT_WHITE, 0x18C3); _gfx->setTextSize(2);
            _gfx->drawCenterString("HALAMAN 2 >>", 354, 270);
        } else {
            _drawPanel(16, 50, 220, 94, editRow == 5);
            _drawPanel(244, 50, 220, 94, editRow == 6);
            _drawPanel(16, 152, 220, 94, editRow == 7);
            _drawPanel(244, 152, 220, 94, editRow == 8);

            _gfx->fillRoundRect(16, 256, 220, 48, 6, 0x18C3);
            _gfx->drawRoundRect(16, 256, 220, 48, 6, (editRow == 9) ? 0xFFE0 : 0x52AA);
            if (editRow == 9) _gfx->drawRoundRect(17, 257, 218, 46, 5, 0xFFE0);
            _gfx->setTextColor(TFT_WHITE, 0x18C3); _gfx->setTextSize(2);
            _gfx->drawCenterString("<< HALAMAN 1", 126, 270);

            _gfx->fillRoundRect(244, 256, 220, 48, 6, 0x07E0);
            _gfx->setTextColor(TFT_BLACK, 0x07E0); _gfx->setTextSize(2);
            _gfx->drawCenterString("HALAMAN 2 >>", 354, 270);
        }
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
    } else if (editRow == 5) {
        int32_t val = (int32_t)state.cranking.crankingRpm + (delta * 25);
        state.cranking.crankingRpm = (uint32_t)constrain(val, 50, 1000);
    } else if (editRow == 6) {
        int32_t val = (int32_t)state.cranking.crankDurationMs + (delta * 500);
        state.cranking.crankDurationMs = (uint32_t)constrain(val, 500, 10000);
    } else if (editRow == 7 && delta != 0) {
        wheel.inverted = !wheel.inverted;
    }
}

void PageGenSettings::onEncoderClick(uint8_t editRow,
                                    EcuEngine::EngineRuntimeState& state,
                                    EcuEngine::ParametricWheel& wheel) {
    if (editRow == 0) {
        onEncoderTurn(1, editRow, state, wheel);
    } else if (editRow == 7) {
        wheel.inverted = !wheel.inverted;
    } else if (editRow == 8) {
        state.rpmStep = 50;
        state.sweep.minRpm = 800;
        state.sweep.maxRpm = 6000;
        state.sweep.sweepRateRpmPerSec = 500;
        state.cranking.crankingRpm = 200;
        state.cranking.crankDurationMs = 3000;
        wheel.inverted = false;
    }
}

} // namespace EcuUi
