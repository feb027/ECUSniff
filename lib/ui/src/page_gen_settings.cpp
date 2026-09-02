#include "page_gen_settings.h"
#include <stdio.h>

namespace EcuUi {

static const uint32_t STEP_PRESETS[] = { 1, 5, 10, 25, 50, 100, 250, 500, 1000 };
static constexpr uint8_t TOTAL_STEP_PRESETS = 9;

static uint32_t cycleStepPreset(uint32_t currentStep, int32_t delta) {
    int idx = 4; // default 50
    for (int i = 0; i < TOTAL_STEP_PRESETS; ++i) {
        if (STEP_PRESETS[i] == currentStep) { idx = i; break; }
    }
    idx = idx + (delta > 0 ? 1 : -1);
    if (idx < 0) idx = TOTAL_STEP_PRESETS - 1;
    if (idx >= TOTAL_STEP_PRESETS) idx = 0;
    return STEP_PRESETS[idx];
}

PageGenSettings::PageGenSettings(LovyanGFX* gfx) : _gfx(gfx) {}

void PageGenSettings::init() {
    _subCategory = 0;
    _lastSubCategory = 0xFF;
    _lastEditRow = 0xFF;
    _lastRpmStep = 0;
    _lastEncMin = 0xFFFFFFFF;
    _lastEncMax = 0xFFFFFFFF;
    _lastPotStep = 0;
    _lastPotMin = 0xFFFFFFFF;
    _lastPotMax = 0xFFFFFFFF;
    _lastSweepStep = 0;
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

    const char* catNames[5] = { "1. CRANK", "2. FIX ENC", "3. POTENSIO", "4. SWEEP", "5. HARDWARE" };
    int32_t tabW = 86;
    for (uint8_t i = 0; i < 5; ++i) {
        int32_t x = 16 + (i * 90);
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
        _gfx->fillRect(0, 40, 480, 280, 0x0841);
        _gfx->fillRoundRect(8, 44, 464, 268, 8, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        _drawSubNav(true);

        _lastEditRow = 0xFF;
        _lastRpmStep = 0; _lastEncMin = 0xFFFFFFFF; _lastEncMax = 0xFFFFFFFF;
        _lastPotStep = 0; _lastPotMin = 0xFFFFFFFF; _lastPotMax = 0xFFFFFFFF;
        _lastSweepStep = 0; _lastSweepMin = 0; _lastSweepMax = 0; _lastSweepRate = 0;
        _lastCrankRpm = 0; _lastCrankDur = 0; _lastSpinUpDur = 0; _lastRampDur = 0;
    } else {
        _drawSubNav(false);
    }

    if (_subCategory == 0) {
        if (needFull) {
            _gfx->fillRoundRect(16, 78, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("1. CRANK START RPM:", 24, 83);

            _gfx->fillRoundRect(244, 78, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("2. CRANK DURATION:", 252, 83);

            _gfx->fillRoundRect(16, 154, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("3. 0 -> CRANK SPIN-UP:", 24, 159);

            _gfx->fillRoundRect(244, 154, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("4. CRANK TRANSITION:", 252, 159);

            _gfx->fillRoundRect(16, 230, 448, 72, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("5. GRADUAL RAMP DURATION (SETELAH CRANK SELESAI):", 24, 235);
        }

        if (state.cranking.crankingRpm != _lastCrankRpm || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM    ", (unsigned)state.cranking.crankingRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 101);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("RPM Awal Starter Mesin", 24, 127);
            _lastCrankRpm = state.cranking.crankingRpm;
        }

        if (state.cranking.crankDurationMs != _lastCrankDur || needFull) {
            char buf[16]; float sec = state.cranking.crankDurationMs / 1000.0f;
            snprintf(buf, sizeof(buf), "%.1f DETIK   ", sec);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 252, 101);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Lama Tahan Starter", 252, 127);
            _lastCrankDur = state.cranking.crankDurationMs;
        }

        if (state.cranking.spinUpDurationMs != _lastSpinUpDur || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u MS     ", (unsigned)state.cranking.spinUpDurationMs);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 177);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Waktu dari 0 ke Crank RPM", 24, 203);
            _lastSpinUpDur = state.cranking.spinUpDurationMs;
        }

        if (state.cranking.fastFlare != _lastFastFlare || needFull) {
            const char* tStr = state.cranking.fastFlare ? "MELESAT (Direct)" : "GRADUAL (Ramp)  ";
            uint16_t col = state.cranking.fastFlare ? 0x07E0 : 0xFFE0;
            _gfx->setTextColor(col, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(tStr, 252, 177);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString(state.cranking.fastFlare ? "Langsung loncat (0ms ke Fix)" : "Naik halus bertahap", 252, 203);
            _lastFastFlare = state.cranking.fastFlare;
        }

        if (state.cranking.rampDurationMs != _lastRampDur || state.cranking.fastFlare != _lastFastFlare || needFull) {
            char buf[32]; float sec = state.cranking.rampDurationMs / 1000.0f;
            snprintf(buf, sizeof(buf), "%.2f DETIK (%u ms)       ", sec, (unsigned)state.cranking.rampDurationMs);
            uint16_t valCol = state.cranking.fastFlare ? 0x8410 : 0x07E0;
            _gfx->setTextColor(valCol, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 253);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString(state.cranking.fastFlare ? "Nonaktif pada transisi Melesat (Hanya saat GRADUAL) " : 
                                                      "Lama akselerasi dari Crank RPM ke running RPM      ", 24, 279);
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
        if (needFull) {
            _gfx->fillRoundRect(16, 78, 448, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("1. RPM STEP (KENAIKAN PER KLIK ENCODER):", 24, 83);

            _gfx->fillRoundRect(16, 154, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("2. MIN RPM (BATAS BAWAH ENCODER):", 24, 159);

            _gfx->fillRoundRect(244, 154, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("3. MAX RPM (BATAS ATAS ENCODER):", 252, 159);
        }

        uint32_t stepVal = (state.fixEnc.rpmStep > 0) ? state.fixEnc.rpmStep : 50;
        if (stepVal != _lastRpmStep || needFull) {
            char buf[20]; snprintf(buf, sizeof(buf), "+/-%u RPM        ", (unsigned)stepVal);
            _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 101);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Pilihan: 1, 5, 10, 25, 50, 100, 250, 500, 1000 RPM", 24, 127);
            _lastRpmStep = stepVal;
        }

        if (state.fixEnc.minRpm != _lastEncMin || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM    ", (unsigned)state.fixEnc.minRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 177);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Batas Bawah Mode FIX", 24, 203);
            _lastEncMin = state.fixEnc.minRpm;
        }

        if (state.fixEnc.maxRpm != _lastEncMax || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM    ", (unsigned)state.fixEnc.maxRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 252, 177);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Batas Atas Mode FIX", 252, 203);
            _lastEncMax = state.fixEnc.maxRpm;
        }

        if (editRow != _lastEditRow || needFull) {
            _drawPanel(16, 78, 448, 70, editRow == 0);
            _drawPanel(16, 154, 220, 70, editRow == 1);
            _drawPanel(244, 154, 220, 70, editRow == 2);
            _lastEditRow = editRow;
        }
    } else if (_subCategory == 2) {
        if (needFull) {
            _gfx->fillRoundRect(16, 78, 448, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("1. POT RPM STEP (RESOLUSI / STEP QUANTIZATION):", 24, 83);

            _gfx->fillRoundRect(16, 154, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("2. MIN RPM (MENTOK KIRI / 0%):", 24, 159);

            _gfx->fillRoundRect(244, 154, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("3. MAX RPM (MENTOK KANAN / 100%):", 252, 159);

            _gfx->fillRoundRect(16, 230, 448, 72, 6, 0x0841);
            _gfx->drawRoundRect(16, 230, 448, 72, 6, 0x07E0);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("KARAKTER MODE POTENSIOMETER (FINE-TUNING):", 24, 236);
        }

        uint32_t potStepVal = (state.potCfg.rpmStep > 0) ? state.potCfg.rpmStep : 1;
        if (potStepVal != _lastPotStep || needFull) {
            char buf[32];
            if (potStepVal <= 1) {
                snprintf(buf, sizeof(buf), "1 RPM (KONTINU / SUPER HALUS)  ");
            } else {
                snprintf(buf, sizeof(buf), "+/-%u RPM (TERKUNCI KELIPATAN) ", (unsigned)potStepVal);
            }
            _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 101);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Pilihan: 1 (Halus), 5, 10, 25, 50, 100, 250, 500, 1000 RPM", 24, 127);
            _lastPotStep = potStepVal;
        }

        if (state.potCfg.minRpm != _lastPotMin || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM    ", (unsigned)state.potCfg.minRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 177);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("RPM saat knob di kiri (0V)", 24, 203);
            _lastPotMin = state.potCfg.minRpm;
        }

        if (state.potCfg.maxRpm != _lastPotMax || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM    ", (unsigned)state.potCfg.maxRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 252, 177);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("RPM saat knob di kanan (3.3V)", 252, 203);
            _lastPotMax = state.potCfg.maxRpm;
        }

        if (state.potCfg.minRpm != _lastPotMin || state.potCfg.maxRpm != _lastPotMax || needFull) {
            uint32_t span = (state.potCfg.maxRpm > state.potCfg.minRpm) ? (state.potCfg.maxRpm - state.potCfg.minRpm) : 0;
            float degPerRpm = (span > 0) ? (270.0f / (float)span) : 0.0f;
            char infoBuf[64];
            snprintf(infoBuf, sizeof(infoBuf), "Total Span Rentang: %u RPM (1 RPM = %.2f deg)           ", (unsigned)span, degPerRpm * 10.0f);
            _gfx->fillRect(24, 254, 432, 42, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString(infoBuf, 24, 254);
            _gfx->setTextColor(0x07FF, 0x0841);
            _gfx->drawString("Schmidt Lock aktif (100% diam total saat potensiometer diam).", 24, 272);
        }

        if (editRow != _lastEditRow || needFull) {
            _drawPanel(16, 78, 448, 70, editRow == 0);
            _drawPanel(16, 154, 220, 70, editRow == 1);
            _drawPanel(244, 154, 220, 70, editRow == 2);
            _lastEditRow = editRow;
        }
    } else if (_subCategory == 3) {
        if (needFull) {
            _gfx->fillRoundRect(16, 78, 448, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("1. SWEEP STEP (KENAIKAN / STEP KECEPATAN):", 24, 83);

            _gfx->fillRoundRect(16, 154, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("2. SWEEP MIN RPM (BATAS BAWAH):", 24, 159);

            _gfx->fillRoundRect(244, 154, 220, 70, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("3. SWEEP MAX RPM (BATAS ATAS):", 252, 159);

            _gfx->fillRoundRect(16, 230, 448, 72, 6, 0x0841);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("4. SWEEP RATE (KECEPATAN NAIK/TURUN OTOMATIS):", 24, 235);
        }

        uint32_t swStepVal = (state.sweep.rpmStep > 0) ? state.sweep.rpmStep : 50;
        if (swStepVal != _lastSweepStep || needFull) {
            char buf[20]; snprintf(buf, sizeof(buf), "+/-%u RPM        ", (unsigned)swStepVal);
            _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 101);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Pilihan: 1, 5, 10, 25, 50, 100, 250, 500, 1000 RPM", 24, 127);
            _lastSweepStep = swStepVal;
        }

        if (state.sweep.minRpm != _lastSweepMin || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM    ", (unsigned)state.sweep.minRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 177);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Titik putar balik bawah", 24, 203);
            _lastSweepMin = state.sweep.minRpm;
        }

        if (state.sweep.maxRpm != _lastSweepMax || needFull) {
            char buf[16]; snprintf(buf, sizeof(buf), "%u RPM    ", (unsigned)state.sweep.maxRpm);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 252, 177);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Titik putar balik atas", 252, 203);
            _lastSweepMax = state.sweep.maxRpm;
        }

        if (state.sweep.sweepRateRpmPerSec != _lastSweepRate || needFull) {
            char buf[24]; snprintf(buf, sizeof(buf), "%u RPM/DETIK      ", (unsigned)state.sweep.sweepRateRpmPerSec);
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString(buf, 24, 253);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Kecepatan sapuan naik & turun putaran RPM per detik", 24, 279);
            _lastSweepRate = state.sweep.sweepRateRpmPerSec;
        }

        if (editRow != _lastEditRow || needFull) {
            _drawPanel(16, 78, 448, 70, editRow == 0);
            _drawPanel(16, 154, 220, 70, editRow == 1);
            _drawPanel(244, 154, 220, 70, editRow == 2);
            _drawPanel(16, 230, 448, 72, editRow == 3);
            _lastEditRow = editRow;
        }
    } else {
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
        if (editRow == 0) {
            state.fixEnc.rpmStep = cycleStepPreset(state.fixEnc.rpmStep, delta);
            state.rpmStep = state.fixEnc.rpmStep;
        } else if (editRow == 1) {
            uint32_t step = (state.fixEnc.rpmStep > 0) ? state.fixEnc.rpmStep : 50;
            int32_t val = (int32_t)state.fixEnc.minRpm + (delta * (int32_t)step);
            state.fixEnc.minRpm = (uint32_t)constrain(val, 0, 6000);
            if (state.fixEnc.maxRpm <= state.fixEnc.minRpm) {
                state.fixEnc.maxRpm = state.fixEnc.minRpm + step;
            }
        } else if (editRow == 2) {
            uint32_t step = (state.fixEnc.rpmStep > 0) ? state.fixEnc.rpmStep : 50;
            int32_t val = (int32_t)state.fixEnc.maxRpm + (delta * (int32_t)step);
            uint32_t minAllowed = state.fixEnc.minRpm + step;
            state.fixEnc.maxRpm = (uint32_t)constrain(val, (int32_t)minAllowed, 12000);
        }
    } else if (_subCategory == 2) {
        if (editRow == 0) {
            state.potCfg.rpmStep = cycleStepPreset(state.potCfg.rpmStep, delta);
        } else if (editRow == 1) {
            uint32_t step = (state.potCfg.rpmStep > 1) ? state.potCfg.rpmStep : 50;
            int32_t val = (int32_t)state.potCfg.minRpm + (delta * (int32_t)step);
            state.potCfg.minRpm = (uint32_t)constrain(val, 0, 6000);
            if (state.potCfg.maxRpm <= state.potCfg.minRpm) {
                state.potCfg.maxRpm = state.potCfg.minRpm + step;
            }
        } else if (editRow == 2) {
            uint32_t step = (state.potCfg.rpmStep > 1) ? state.potCfg.rpmStep : 50;
            int32_t val = (int32_t)state.potCfg.maxRpm + (delta * (int32_t)step);
            uint32_t minAllowed = state.potCfg.minRpm + step;
            state.potCfg.maxRpm = (uint32_t)constrain(val, (int32_t)minAllowed, 12000);
        }
    } else if (_subCategory == 3) {
        if (editRow == 0) {
            state.sweep.rpmStep = cycleStepPreset(state.sweep.rpmStep, delta);
        } else if (editRow == 1) {
            uint32_t step = (state.sweep.rpmStep > 0) ? state.sweep.rpmStep : 50;
            int32_t val = (int32_t)state.sweep.minRpm + (delta * (int32_t)step);
            state.sweep.minRpm = (uint32_t)constrain(val, 0, 6000);
            if (state.sweep.maxRpm <= state.sweep.minRpm) {
                state.sweep.maxRpm = state.sweep.minRpm + step;
            }
        } else if (editRow == 2) {
            uint32_t step = (state.sweep.rpmStep > 0) ? state.sweep.rpmStep : 50;
            int32_t val = (int32_t)state.sweep.maxRpm + (delta * (int32_t)step);
            uint32_t minAllowed = state.sweep.minRpm + step;
            state.sweep.maxRpm = (uint32_t)constrain(val, (int32_t)minAllowed, 12000);
        } else if (editRow == 3) {
            uint32_t step = (state.sweep.rpmStep > 0) ? state.sweep.rpmStep : 50;
            int32_t val = (int32_t)state.sweep.sweepRateRpmPerSec + (delta * (int32_t)step);
            state.sweep.sweepRateRpmPerSec = (uint32_t)constrain(val, 50, 3000);
        }
    } else {
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
    } else if (_subCategory == 2) {
        if (editRow == 0) {
            onEncoderTurn(1, editRow, state, wheel);
        }
    } else if (_subCategory == 3) {
        if (editRow == 0) {
            onEncoderTurn(1, editRow, state, wheel);
        }
    } else if (_subCategory == 4) {
        if (editRow == 0) {
            wheel.inverted = !wheel.inverted;
        }
    }
}

} // namespace EcuUi
