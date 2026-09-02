#include "page_gen_settings.h"
#include <stdio.h>

namespace EcuUi {

static const uint32_t STEP_PRESETS[] = { 1, 10, 50, 100, 250, 500, 1000 };
static constexpr uint8_t TOTAL_STEP_PRESETS = 7;

PageGenSettings::PageGenSettings(LovyanGFX* gfx) : _gfx(gfx) {}

void PageGenSettings::init() {
    _subCategory = 0;
    _lastSubCategory = 0xFF;
    _lastEditRow = 0xFF;
    _lastRpmStep = 0;
    _lastSweepMin = 0;
    _lastSweepMax = 0;
    _lastSweepRate = 0;
    _lastCrankRpm = 0;
    _lastCrankDur = 0;
    _lastSpinUpDur = 0;
    _lastRampDur = 0;
    _lastFastFlare = false;
    _lastInverted = false;
}

void PageGenSettings::_drawPanel(int32_t x, int32_t y, int32_t w, int32_t h, bool isSel) {
    _gfx->drawRoundRect(x, y, w, h, 6, isSel ? 0xFFE0 : 0x52AA);
    _gfx->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, isSel ? 0xFFE0 : 0x0841);
}

void PageGenSettings::_drawSubNav(bool fullRedraw) {
    if (_subCategory == _lastSubCategory && _focusSubNav == _lastFocusSubNav && !fullRedraw) return;

    // Sub-menu category selector strip
    const char* catNames[3] = { "1. CRANKING & STARTER", "2. SWEEP & STEP", "3. HARDWARE & SIGNAL" };
    int32_t tabW = 146;
    for (uint8_t i = 0; i < 3; ++i) {
        int32_t x = 16 + (i * 152);
        int32_t y = 48;
        bool isActive = (_subCategory == i);
        uint16_t bg = isActive ? 0x2945 : 0x0841;
        uint16_t border = isActive ? (_focusSubNav ? 0xFFE0 : 0x07E0) : 0x3186;

        _gfx->fillRoundRect(x, y, tabW, 26, 4, bg);
        _gfx->drawRoundRect(x, y, tabW, 26, 4, border);
        if (isActive) {
            _gfx->drawRoundRect(x + 1, y + 1, tabW - 2, 24, 3, border);
        }

        _gfx->setTextColor(isActive ? (_focusSubNav ? 0xFFE0 : 0x07E0) : 0x8410, bg);
        _gfx->setTextSize(1);
        int32_t tw = _gfx->textWidth(catNames[i]);
        _gfx->drawString(catNames[i], x + (tabW - tw) / 2, y + 9);
    }
    _lastSubCategory = _subCategory;
    _lastFocusSubNav = _focusSubNav;
}

void PageGenSettings::render(bool fullRedraw, uint8_t editRow,
                            const EcuEngine::EngineRuntimeState& state,
                            const EcuEngine::ParametricWheel& wheel) {
    bool needFull = fullRedraw || (_subCategory != _lastSubCategory);

    if (needFull) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        _drawSubNav(true);

        _lastEditRow = 0xFF;
        _lastRpmStep = 0; _lastSweepMin = 0; _lastSweepMax = 0; _lastSweepRate = 0;
        _lastCrankRpm = 0; _lastCrankDur = 0; _lastSpinUpDur = 0; _lastRampDur = 0;
    } else {
        _drawSubNav(false);
    }

    if (_subCategory == 0) {
        // ====================================================================
        // KATEGORI 0: CRANKING & STARTER (5 ITEM)
        // ====================================================================
        if (needFull) {
            // Row 0: Crank RPM & Crank Duration
            _gfx->fillRoundRect(16, 78, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("1. CRANK START RPM:", 24, 83);

            _gfx->fillRoundRect(244, 78, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("2. CRANK DURATION:", 252, 83);

            // Row 1: Spin-Up Duration & Crank Transition
            _gfx->fillRoundRect(16, 154, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("3. 0 -> CRANK SPIN-UP:", 24, 159);

            _gfx->fillRoundRect(244, 154, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("4. CRANK TRANSITION:", 252, 159);

            // Row 2: Gradual Ramp Duration (Full Width)
            _gfx->fillRoundRect(16, 230, 448, 72, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("5. GRADUAL RAMP DURATION (SETELAH CRANK SELESAI):", 24, 235);
        }

        // Item 0: Crank RPM
        if (state.cranking.crankingRpm != _lastCrankRpm || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM    ", (unsigned)state.cranking.crankingRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 101);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("RPM Awal Starter Mesin", 24, 127);
            _lastCrankRpm = state.cranking.crankingRpm;
        }

        // Item 1: Crank Duration
        if (state.cranking.crankDurationMs != _lastCrankDur || needFull) {
            char buf[16]; float sec = state.cranking.crankDurationMs / 1000.0f;
            snprintf(buf, sizeof(buf), "%.1f DETIK   ", sec);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 252, 101);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Lama Tahan Starter", 252, 127);
            _lastCrankDur = state.cranking.crankDurationMs;
        }

        // Item 2: Spin-Up Duration
        if (state.cranking.spinUpDurationMs != _lastSpinUpDur || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u MS     ", (unsigned)state.cranking.spinUpDurationMs);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 177);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Waktu dari 0 ke Crank RPM", 24, 203);
            _lastSpinUpDur = state.cranking.spinUpDurationMs;
        }

        // Item 3: Crank Transition (Melesat vs Gradual)
        if (state.cranking.fastFlare != _lastFastFlare || needFull) {
            const char* tStr = state.cranking.fastFlare ? "MELESAT (Direct)" : "GRADUAL (Ramp)  ";
            uint16_t col = state.cranking.fastFlare ? 0x07E0 : 0xFFE0;
            _gfx->setTextColor(col, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(tStr, 252, 177);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString(state.cranking.fastFlare ? "Langsung loncat (0ms ke Fix)" : "Naik halus bertahap", 252, 203);
            _lastFastFlare = state.cranking.fastFlare;
        }

        // Item 4: Gradual Ramp Duration
        if (state.cranking.rampDurationMs != _lastRampDur || state.cranking.fastFlare != _lastFastFlare || needFull) {
            char buf[32]; float sec = state.cranking.rampDurationMs / 1000.0f;
            snprintf(buf, sizeof(buf), "%.2f DETIK (%u ms)       ", sec, (unsigned)state.cranking.rampDurationMs);
            uint16_t valCol = state.cranking.fastFlare ? 0x8410 : 0x07E0;
            _gfx->setTextColor(valCol, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 253);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            if (state.cranking.fastFlare) {
                _gfx->drawString("Nonaktif pada transisi Melesat (Hanya aktif saat GRADUAL)       ", 24, 279);
            } else {
                _gfx->drawString("Lama akselerasi dari Crank RPM ke Target RPM running            ", 24, 279);
            }
            _lastRampDur = state.cranking.rampDurationMs;
        }

        if (editRow != _lastEditRow || needFull) {
            _drawPanel(16, 78, 220, 70, editRow == 0);
            _drawPanel(244, 78, 220, 70, editRow == 1);
            _drawPanel(16, 154, 220, 70, editRow == 2);
            _drawPanel(244, 154, 220, 70, editRow == 3);
            _drawPanel(16, 230, 448, 72, editRow == 4);
            _lastEditRow = editRow;
        }
    } else if (_subCategory == 1) {
        // ====================================================================
        // KATEGORI 1: AUTO SWEEP & STEP (4 ITEM)
        // ====================================================================
        if (needFull) {
            _gfx->fillRoundRect(16, 78, 220, 95, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("1. RPM STEP (ENCODER):", 24, 85);

            _gfx->fillRoundRect(244, 78, 220, 95, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("2. SWEEP MIN RPM:", 252, 85);

            _gfx->fillRoundRect(16, 185, 220, 95, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("3. SWEEP MAX RPM:", 24, 192);

            _gfx->fillRoundRect(244, 185, 220, 95, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("4. SWEEP RATE (SPEED):", 252, 192);
        }

        uint32_t stepVal = (state.rpmStep > 0) ? state.rpmStep : 50;
        if (stepVal != _lastRpmStep || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "+/-%u RPM  ", (unsigned)stepVal);
            _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 110);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Kenaikan per step knob", 24, 142);
            _lastRpmStep = stepVal;
        }

        if (state.sweep.minRpm != _lastSweepMin || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM    ", (unsigned)state.sweep.minRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 252, 110);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Batas Bawah Auto Sweep", 252, 142);
            _lastSweepMin = state.sweep.minRpm;
        }

        if (state.sweep.maxRpm != _lastSweepMax || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM    ", (unsigned)state.sweep.maxRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 217);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Batas Atas Auto Sweep", 24, 249);
            _lastSweepMax = state.sweep.maxRpm;
        }

        if (state.sweep.sweepRateRpmPerSec != _lastSweepRate || needFull) {
            char buf[20]; snprintf(buf, sizeof(buf), "%u RPM/S   ", (unsigned)state.sweep.sweepRateRpmPerSec);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 252, 217);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Kecepatan Sapuan RPM", 252, 249);
            _lastSweepRate = state.sweep.sweepRateRpmPerSec;
        }

        if (editRow != _lastEditRow || needFull) {
            _drawPanel(16, 78, 220, 95, editRow == 0);
            _drawPanel(244, 78, 220, 95, editRow == 1);
            _drawPanel(16, 185, 220, 95, editRow == 2);
            _drawPanel(244, 185, 220, 95, editRow == 3);
            _lastEditRow = editRow;
        }
    } else {
        // ====================================================================
        // KATEGORI 2: HARDWARE & SIGNAL (1 ITEM BESAR)
        // ====================================================================
        if (needFull) {
            _gfx->fillRoundRect(16, 78, 448, 140, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("1. CKP OUTPUT POLARITY:", 26, 90);
        }

        if (wheel.inverted != _lastInverted || needFull) {
            const char* pStr = wheel.inverted ? "INVERTED (Active Low)" : "NORMAL (Active High) ";
            uint32_t pCol = wheel.inverted ? 0xF800 : 0x07E0;
            _gfx->setTextColor(pCol, 0x0841); _gfx->setTextSize(3);
            _gfx->drawString(pStr, 26, 120);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Menentukan fase logika sinyal sensor crank (Hall Effect / Reluctor sim)", 26, 165);
            _lastInverted = wheel.inverted;
        }

        if (editRow != _lastEditRow || needFull) {
            _drawPanel(16, 78, 448, 140, editRow == 0);
            _lastEditRow = editRow;
        }
    }
}

void PageGenSettings::onEncoderTurn(int32_t delta, uint8_t editRow,
                                   EcuEngine::EngineRuntimeState& state,
                                   EcuEngine::ParametricWheel& wheel) {
    if (_subCategory == 0) {
        // Kategori Cranking & Starter
        if (editRow == 0) {
            int32_t val = (int32_t)state.cranking.crankingRpm + (delta * 25);
            state.cranking.crankingRpm = (uint32_t)constrain(val, 50, 1000);
        } else if (editRow == 1) {
            int32_t val = (int32_t)state.cranking.crankDurationMs + (delta * 500);
            state.cranking.crankDurationMs = (uint32_t)constrain(val, 500, 10000);
        } else if (editRow == 2) {
            int32_t val = (int32_t)state.cranking.spinUpDurationMs + (delta * 50);
            state.cranking.spinUpDurationMs = (uint32_t)constrain(val, 100, 2000);
        } else if (editRow == 3 && delta != 0) {
            state.cranking.fastFlare = !state.cranking.fastFlare;
        } else if (editRow == 4) {
            int32_t val = (int32_t)state.cranking.rampDurationMs + (delta * 100);
            state.cranking.rampDurationMs = (uint32_t)constrain(val, 500, 5000);
        }
    } else if (_subCategory == 1) {
        // Kategori Auto Sweep & Step
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
        }
    } else {
        // Kategori Hardware
        if (editRow == 0 && delta != 0) {
            wheel.inverted = !wheel.inverted;
        }
    }
}

void PageGenSettings::onEncoderClick(uint8_t editRow,
                                    EcuEngine::EngineRuntimeState& state,
                                    EcuEngine::ParametricWheel& wheel) {
    if (_subCategory == 0) {
        if (editRow == 3) {
            state.cranking.fastFlare = !state.cranking.fastFlare;
        }
    } else if (_subCategory == 1) {
        if (editRow == 0) {
            onEncoderTurn(1, editRow, state, wheel);
        }
    } else {
        if (editRow == 0) {
            wheel.inverted = !wheel.inverted;
        }
    }
}

} // namespace EcuUi
