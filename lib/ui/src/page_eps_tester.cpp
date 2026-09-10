#include "page_eps_tester.h"
#include "pin_config.h"

namespace EcuUi {

static constexpr uint16_t TAB2_ROW_Y[] = {44, 81, 118, 155, 192, 231, 270};
static constexpr uint8_t  TAB2_TOTAL_ROWS = 7;

static const char* PRESET_NAMES[] = {
    "Toyota / Daihatsu",
    "Suzuki Karimun",
    "Suzuki Ertiga",
    "Suzuki Swift",
    "Honda Jazz / Brio",
    "Honda City (Ind)",
    "Universal LVDT",
    "Retrofit / Swap",
    "Custom Tuning"
};

static const char* PRESET_MODELS[] = {
    "Avanza, Xenia, Rush, Terios, Vios (Denso)",
    "Karimun Wagon R, Estilo (NSK/Koyo)",
    "Ertiga, Splash, Ignis (Mitsubishi)",
    "Swift 2006 (Active Amp 2.50V / Pin E52-16)",
    "Jazz GD3/GE8, Brio (Showa DC 2.430V)",
    "City GM2/GM6 (Showa 7.9 Ohm Inductive)",
    "LVDT TRQ1:13.6R 2.35mH / TRQ2:13.7R 2.38mH",
    "Bypass Standalone (Jimny/Kijang/Taft)",
    "Manual Parametric Tuning Mode"
};

PageEpsTester::PageEpsTester(LovyanGFX* gfx) : _gfx(gfx) {}

void PageEpsTester::init() {
    _lastTab = 0xFF;
    _lastEditRow = 0xFF;

    _lastRunning = false;
    _lastSpeed = -1.0f;
    _lastRpm = 0xFFFFFFFF;
    _lastTorque = -99.0f;
    _lastTrq1Mv = -99999;
    _lastTrq2Mv = -99999;
    _lastTrq1FbMv = -99999;
    _lastTrq2FbMv = -99999;
    _lastDelta1Mv = -99999;
    _lastDelta2Mv = -99999;
    _lastAdcFound = false;
    _lastSweep = false;
    _lastVssFreq = -1.0f;
    _lastRpmFreq = -1.0f;

    _lastPreset = 0xFF;
    _trqCenterFocus = 0;
    _trqSpanFocus = 0;
    _lastT1Center = -1.0f;
    _lastT2Center = -1.0f;
    _lastT1Span = -1.0f;
    _lastT2Span = -1.0f;
    _lastCenterFocus = 0xFF;
    _lastSpanFocus = 0xFF;
    _lastVssPulses = -1.0f;
    _lastRpmPulses = 0xFF;
    _lastT1Scale = -1.0f;
    _lastT1Offset = -99.0f;
    _lastT2Scale = -1.0f;
    _lastT2Offset = -99.0f;
    _lastT1FbPreviewMv = -99999;
    _lastT2FbPreviewMv = -99999;
    _lastT1TargetMv = -99999;
    _lastT2TargetMv = -99999;
    _savedNoticeUntilMs = 0;
    _lastSavedNoticeActive = false;
}

void PageEpsTester::render(uint8_t currentTab, bool fullRedraw, uint8_t editRow,
                          const EcuEngine::EpsController& controller) {
    bool tabChanged = (currentTab != _lastTab);
    if (fullRedraw || tabChanged) {
        _lastTab = currentTab;
        _lastEditRow = 0xFF;

        if (currentTab == 1) {
            _drawStaticLayoutTab1(controller);
            _lastRunning = !controller.getState().isRunning;
            _lastSpeed = -1.0f;
            _lastRpm = 0xFFFFFFFF;
            _lastTorque = -99.0f;
            _lastTrq1Mv = -99999;
            _lastTrq2Mv = -99999;
            _lastTrq1FbMv = -99999;
            _lastTrq2FbMv = -99999;
            _lastDelta1Mv = -99999;
            _lastDelta2Mv = -99999;
            _lastAdcFound = !controller.getState().adcFound;
            _lastSweep = !controller.getConfig().autoSweep;
            _lastVssFreq = -1.0f;
            _lastRpmFreq = -1.0f;
            _lastPreset = 0xFF;
            for (uint8_t i = 0; i <= 4; ++i) {
                _drawBoxBorderTab1(i, i == editRow);
            }
        } else {
            _drawStaticLayoutTab2();
            _lastPreset = 0xFF;
            _lastT1Center = -1.0f;
            _lastT2Center = -1.0f;
            _lastT1Span = -1.0f;
            _lastT2Span = -1.0f;
            _lastCenterFocus = 0xFF;
            _lastSpanFocus = 0xFF;
            _lastVssPulses = -1.0f;
            _lastRpmPulses = 0xFF;
            _lastT1Scale = -1.0f;
            _lastT1Offset = -99.0f;
            _lastT2Scale = -1.0f;
            _lastT2Offset = -99.0f;
            _lastT1FbPreviewMv = -99999;
            _lastT2FbPreviewMv = -99999;
            _lastT1TargetMv = -99999;
            _lastT2TargetMv = -99999;
            _lastSavedNoticeActive = false;
            for (uint8_t i = 0; i < TAB2_TOTAL_ROWS; ++i) {
                _drawRowHighlightTab2(i, i == editRow);
            }
        }
        _lastEditRow = editRow;
    } else if (editRow != _lastEditRow) {
        if (currentTab == 1) {
            if (_lastEditRow <= 4) _drawBoxBorderTab1(_lastEditRow, false);
            if (editRow <= 4) _drawBoxBorderTab1(editRow, true);
        } else {
            if (_lastEditRow < TAB2_TOTAL_ROWS) _drawRowHighlightTab2(_lastEditRow, false);
            if (editRow < TAB2_TOTAL_ROWS) _drawRowHighlightTab2(editRow, true);
        }
        _lastEditRow = editRow;
    }

    if (currentTab == 1) {
        _renderValuesTab1(controller);
    } else {
        _renderValuesTab2(controller);
    }
}

// ============================================================================
// TAB 1: KONTROL UTAMA
// ============================================================================

void PageEpsTester::_drawStaticLayoutTab1(const EcuEngine::EpsController& controller) {
    _gfx->fillRect(0, 42, 480, 278, TFT_BLACK);

    // 1. Zona 1: Top Status Header (Y: 44, H: 24, W: 468)
    _gfx->fillRoundRect(6, 44, 468, 24, 4, 0x10A2);
    _gfx->drawRoundRect(6, 44, 468, 24, 4, 0x31A6);

    // 2. Zona 2: The Core Cockpit - Dual-Channel TRQ Box (Y: 72, H: 120, W: 468)
    _gfx->fillRoundRect(6, 72, 468, 120, 6, 0x0841);
    _gfx->drawRoundRect(6, 72, 468, 120, 6, 0x31A6);

    // Inner TRQ1 Pod (X: 12, Y: 76, W: 224, H: 76)
    _gfx->fillRoundRect(12, 76, 224, 76, 4, 0x10A2);
    _gfx->drawRoundRect(12, 76, 224, 76, 4, 0x31A6);
    _gfx->setTextColor(0xCE79, 0x10A2);
    _gfx->setTextSize(1);
    char trq1Title[48];
    snprintf(trq1Title, sizeof(trq1Title), "● TRQ1 (MCP 0x60 | PWM:GPIO %d)", PinConfig::EPS_TRQ1);
    _gfx->drawString(trq1Title, 18, 80);

    // Static labels for TRQ1
    _gfx->setTextColor(TFT_WHITE, 0x10A2);
    _gfx->drawString("SET :", 18, 97);
    _gfx->setTextColor(0x07FF, 0x10A2);
    _gfx->drawString("READ:", 18, 117);

    // Inner TRQ2 Pod (X: 244, Y: 76, W: 224, H: 76)
    _gfx->fillRoundRect(244, 76, 224, 76, 4, 0x10A2);
    _gfx->drawRoundRect(244, 76, 224, 76, 4, 0x31A6);
    _gfx->setTextColor(0xCE79, 0x10A2);
    _gfx->setTextSize(1);
    char trq2Title[48];
    snprintf(trq2Title, sizeof(trq2Title), "● TRQ2 (MCP 0x61 | PWM:GPIO %d)", PinConfig::EPS_TRQ2);
    _gfx->drawString(trq2Title, 250, 80);

    // Static labels for TRQ2
    _gfx->setTextColor(TFT_WHITE, 0x10A2);
    _gfx->drawString("SET :", 250, 97);
    _gfx->setTextColor(0x07FF, 0x10A2);
    _gfx->drawString("READ:", 250, 117);

    // Dynamic Delta / Verification error tags
    _gfx->setTextColor(0xCE79, 0x10A2);
    _gfx->drawString("Diff:", 112, 137);
    _gfx->drawString("Diff:", 344, 137);

    // --- Center Steering Torque Deflection Indicator ---
    _gfx->setTextColor(0xCE79, 0x0841);
    _gfx->setTextSize(1);
    _gfx->drawString("STEERING SENSOR TORQUE COMMAND", 14, 161);

    int32_t barX = 30;
    int32_t barY = 173;
    int32_t barW = 420;
    int32_t barH = 7;
    _gfx->fillRect(barX, barY, barW, barH, 0x0841);
    _gfx->drawRect(barX, barY, barW, barH, 0x52AA);
    _gfx->drawFastVLine(barX + (barW / 2), barY - 1, barH + 2, TFT_WHITE);

    // 3. Zona 3: Bottom Instrument Cluster
    // Speedometer Pod (Row 0) (X: 6, Y: 196, W: 232, H: 54)
    _gfx->fillRoundRect(6, 196, 232, 54, 4, 0x10A2);
    _gfx->drawRoundRect(6, 196, 232, 54, 4, 0x31A6);
    _gfx->setTextColor(0xCE79, 0x10A2);
    _gfx->setTextSize(1);
    char vssTitle[32];
    snprintf(vssTitle, sizeof(vssTitle), "VSS OUT [GPIO %d]", PinConfig::EPS_VSS);
    _gfx->drawString(vssTitle, 14, 201);

    // Tachometer Pod (Row 1) (X: 242, Y: 196, W: 232, H: 54)
    _gfx->fillRoundRect(242, 196, 232, 54, 4, 0x10A2);
    _gfx->drawRoundRect(242, 196, 232, 54, 4, 0x31A6);
    _gfx->setTextColor(0xCE79, 0x10A2);
    _gfx->setTextSize(1);
    char rpmTitle[32];
    snprintf(rpmTitle, sizeof(rpmTitle), "RPM TACHO [GPIO %d]", PinConfig::EPS_RPM);
    _gfx->drawString(rpmTitle, 250, 201);

    // Auto Sweep Pod (Row 3) (X: 6, Y: 254, W: 180, H: 54)
    _gfx->fillRoundRect(6, 254, 180, 54, 4, 0x10A2);
    _gfx->drawRoundRect(6, 254, 180, 54, 4, 0x31A6);
    _gfx->setTextColor(0xCE79, 0x10A2);
    _gfx->setTextSize(1);
    _gfx->drawString("AUTO SPEED SWEEP", 14, 259);

    // Master Action Pod RUN/STOP (Row 4) (X: 190, Y: 254, W: 284, H: 54)
    _gfx->fillRoundRect(190, 254, 284, 54, 4, 0x10A2);
    _gfx->drawRoundRect(190, 254, 284, 54, 4, 0x31A6);
}

void PageEpsTester::_drawBoxBorderTab1(uint8_t boxIdx, bool selected) {
    uint16_t outerColor = selected ? 0xFFE0 : 0x31A6;
    uint16_t innerColor = selected ? 0xFFE0 : ((boxIdx == 2) ? 0x0841 : 0x10A2);

    switch (boxIdx) {
        case 2: // Steer TRQ Cockpit
            _gfx->drawRoundRect(6, 72, 468, 120, 6, outerColor);
            _gfx->drawRoundRect(7, 73, 466, 118, 5, innerColor);
            break;
        case 0: // Speedometer
            _gfx->drawRoundRect(6, 196, 232, 54, 4, outerColor);
            _gfx->drawRoundRect(7, 197, 230, 52, 3, innerColor);
            break;
        case 1: // Tachometer
            _gfx->drawRoundRect(242, 196, 232, 54, 4, outerColor);
            _gfx->drawRoundRect(243, 197, 230, 52, 3, innerColor);
            break;
        case 3: // Auto Sweep
            _gfx->drawRoundRect(6, 254, 180, 54, 4, outerColor);
            _gfx->drawRoundRect(7, 255, 178, 52, 3, innerColor);
            break;
        case 4: // RUN/STOP
            _gfx->drawRoundRect(190, 254, 284, 54, 4, outerColor);
            _gfx->drawRoundRect(191, 255, 282, 52, 3, innerColor);
            break;
        default:
            break;
    }
}

void PageEpsTester::_renderValuesTab1(const EcuEngine::EpsController& controller) {
    const auto& cfg = controller.getConfig();
    const auto& state = controller.getState();

    // 1. Zona 1: Top Status Bar
    uint8_t curPreset = static_cast<uint8_t>(cfg.preset);
    bool runningChanged = (state.isRunning != _lastRunning);
    if (curPreset != _lastPreset || runningChanged || state.adcFound != _lastAdcFound) {
        // Left Preset Badge
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->setTextPadding(220);
        char presetBuf[64];
        const char* pName = (curPreset < 9) ? PRESET_NAMES[curPreset] : "Custom Tuning";
        snprintf(presetBuf, sizeof(presetBuf), "PRESET: %s", pName);
        _gfx->drawString(presetBuf, 12, 52);

        // Hardware Status
        char hwBuf[48];
        const char* d1 = state.dacTrq1Found ? "OK" : "PWM";
        const char* d2 = state.dacTrq2Found ? "OK" : "PWM";
        const char* ad = state.adcFound ? "OK" : "NO";
        snprintf(hwBuf, sizeof(hwBuf), "DAC:[%s,%s] ADC:[%s]", d1, d2, ad);
        _gfx->setTextColor(0xCE79, 0x10A2);
        _gfx->setTextPadding(150);
        _gfx->drawString(hwBuf, 235, 52);
        _gfx->setTextPadding(0);

        // Right State Badge: ONLY show when RUNNING (no standby text)
        if (state.isRunning) {
            _gfx->fillRoundRect(390, 47, 78, 18, 3, 0x07E0);
            _gfx->setTextColor(TFT_BLACK, 0x07E0);
            _gfx->setTextSize(1);
            _gfx->drawCenterString("● RUN", 429, 52);
        } else {
            _gfx->fillRect(390, 47, 78, 18, 0x10A2);
        }
        _lastPreset = curPreset;
        _lastAdcFound = state.adcFound;
    }

    // 2. Zona 2: TRQ Cockpit Dual Meter (Target vs Realtime Feedback)
    int32_t t1Mv = static_cast<int32_t>(lroundf(state.trq1Voltage * 1000.0f));
    if (t1Mv != _lastTrq1Mv) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.3f V", state.trq1Voltage);
        _gfx->setTextColor(TFT_WHITE, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->setTextPadding(95);
        _gfx->drawString(buf, 55, 95);
        _gfx->setTextPadding(0);
        _lastTrq1Mv = t1Mv;
    }

    int32_t fb1Mv = static_cast<int32_t>(lroundf(state.trq1FeedbackVoltage * 1000.0f));
    if (fb1Mv != _lastTrq1FbMv) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.3f V", state.trq1FeedbackVoltage);
        _gfx->setTextColor(0x07FF, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->setTextPadding(95);
        _gfx->drawString(buf, 55, 115);
        _gfx->setTextPadding(0);
        _lastTrq1FbMv = fb1Mv;
    }

    int32_t delta1 = fb1Mv - t1Mv;
    if (delta1 != _lastDelta1Mv) {
        _gfx->setTextSize(1);
        if (abs(delta1) <= 15) {
            _gfx->setTextColor(0x07E0, 0x10A2); // Green within 15mV
        } else if (abs(delta1) <= 50) {
            _gfx->setTextColor(0xFFE0, 0x10A2); // Yellow
        } else {
            _gfx->setTextColor(0xFD20, 0x10A2); // Orange
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "Err: %+d mV", (int)delta1);
        _gfx->setTextPadding(85);
        _gfx->drawString(buf, 150, 137);
        _gfx->setTextPadding(0);
        _lastDelta1Mv = delta1;
    }

    int32_t t2Mv = static_cast<int32_t>(lroundf(state.trq2Voltage * 1000.0f));
    if (t2Mv != _lastTrq2Mv) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.3f V", state.trq2Voltage);
        _gfx->setTextColor(TFT_WHITE, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->setTextPadding(95);
        _gfx->drawString(buf, 287, 95);
        _gfx->setTextPadding(0);
        _lastTrq2Mv = t2Mv;
    }

    int32_t fb2Mv = static_cast<int32_t>(lroundf(state.trq2FeedbackVoltage * 1000.0f));
    if (fb2Mv != _lastTrq2FbMv) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.3f V", state.trq2FeedbackVoltage);
        _gfx->setTextColor(0x07FF, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->setTextPadding(95);
        _gfx->drawString(buf, 287, 115);
        _gfx->setTextPadding(0);
        _lastTrq2FbMv = fb2Mv;
    }

    int32_t delta2 = fb2Mv - t2Mv;
    if (delta2 != _lastDelta2Mv) {
        _gfx->setTextSize(1);
        if (abs(delta2) <= 15) {
            _gfx->setTextColor(0x07E0, 0x10A2);
        } else if (abs(delta2) <= 50) {
            _gfx->setTextColor(0xFFE0, 0x10A2);
        } else {
            _gfx->setTextColor(0xFD20, 0x10A2);
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "Err: %+d mV", (int)delta2);
        _gfx->setTextPadding(85);
        _gfx->drawString(buf, 382, 137);
        _gfx->setTextPadding(0);
        _lastDelta2Mv = delta2;
    }

    // --- Steering Balance Bar (Only redraws when steer torque changes) ---
    if (cfg.steerTorque != _lastTorque) {
        float pct = cfg.steerTorque * 100.0f;
        const char* dir = (pct > 2.0f) ? "KANAN" : ((pct < -2.0f) ? "KIRI" : "LURUS");
        char buf[32];
        snprintf(buf, sizeof(buf), "%+.0f%% [%s]", pct, dir);
        _gfx->setTextSize(1);
        _gfx->setTextColor(0xFD20, 0x10A2);
        _gfx->setTextPadding(150);
        _gfx->drawCenterString(buf, 240, 161);
        _gfx->setTextPadding(0);

        int32_t barX = 30;
        int32_t barY = 173;
        int32_t barW = 420;
        int32_t barH = 7;
        _gfx->fillRect(barX + 1, barY + 1, barW - 2, barH - 2, 0x0841);
        int32_t midX = barX + (barW / 2);
        _gfx->drawFastVLine(midX, barY - 1, barH + 2, TFT_WHITE);

        int32_t deflW = static_cast<int32_t>(cfg.steerTorque * ((barW / 2) - 4));
        if (deflW > 0) {
            _gfx->fillRect(midX + 1, barY + 1, deflW, barH - 2, 0x07E0); // Green right
        } else if (deflW < 0) {
            _gfx->fillRect(midX + deflW, barY + 1, -deflW, barH - 2, 0x07FF); // Cyan left
        }
        _lastTorque = cfg.steerTorque;
    }

    // 3. Zona 3: Speedometer Pod (Row 0)
    if (state.currentSpeedKmh != _lastSpeed || state.vssFreqHz != _lastVssFreq) {
        // Speed Value (Size 3 - Large digits)
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f", state.currentSpeedKmh);
        _gfx->setTextColor(0x07FF, 0x10A2);
        _gfx->setTextSize(3);
        _gfx->setTextPadding(65);
        _gfx->drawString(buf, 14, 217);

        // Speed Unit "km/h" (Size 2)
        _gfx->setTextSize(2);
        _gfx->setTextPadding(55);
        _gfx->drawString("km/h", 80, 224);

        // Realtime Frequency without "Pin" (Size 2 - Yellow)
        char freqBuf[24];
        snprintf(freqBuf, sizeof(freqBuf), "%.1f Hz", state.vssFreqHz);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextPadding(85);
        _gfx->drawRightString(freqBuf, 226, 224);
        _gfx->setTextPadding(0);

        _lastSpeed = state.currentSpeedKmh;
        _lastVssFreq = state.vssFreqHz;
    }

    // 4. Zona 3: Tachometer Pod (Row 1)
    if (state.currentRpm != _lastRpm || state.rpmFreqHz != _lastRpmFreq) {
        // RPM Value (Size 3 - Large digits)
        char buf[16];
        snprintf(buf, sizeof(buf), "%u", (unsigned)state.currentRpm);
        _gfx->setTextColor(0x07E0, 0x10A2);
        _gfx->setTextSize(3);
        _gfx->setTextPadding(80);
        _gfx->drawString(buf, 250, 217);

        // Unit "RPM" (Size 2)
        _gfx->setTextSize(2);
        _gfx->setTextPadding(45);
        _gfx->drawString("RPM", 335, 224);

        // Realtime Frequency without "Pin" (Size 2 - Yellow)
        char freqBuf[24];
        snprintf(freqBuf, sizeof(freqBuf), "%.1f Hz", state.rpmFreqHz);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextPadding(80);
        _gfx->drawRightString(freqBuf, 466, 224);
        _gfx->setTextPadding(0);

        _lastRpm = state.currentRpm;
        _lastRpmFreq = state.rpmFreqHz;
    }

    // 5. Zona 3: Auto Sweep Pod (Row 3)
    if (cfg.autoSweep != _lastSweep) {
        _gfx->setTextSize(2);
        _gfx->setTextPadding(165);
        if (cfg.autoSweep) {
            _gfx->setTextColor(0x07E0, 0x10A2);
            _gfx->drawString("AKTIF (0-120)", 14, 277);
        } else {
            _gfx->setTextColor(0xCE79, 0x10A2);
            _gfx->drawString("TETAP (OFF)", 14, 277);
        }
        _gfx->setTextPadding(0);
        _lastSweep = cfg.autoSweep;
    }

    // 6. Zona 3: RUN/STOP Master Action Pod (Row 4)
    if (runningChanged) {
        if (state.isRunning) {
            // Prominent full-width crimson active banner
            _gfx->fillRoundRect(194, 258, 276, 46, 5, 0x9000);
            _gfx->drawRoundRect(194, 258, 276, 46, 5, 0xF800);
            _gfx->drawRoundRect(195, 259, 274, 44, 4, 0xF800);
            _gfx->setTextColor(TFT_WHITE, 0x9000);
            _gfx->setTextSize(2);
            _gfx->drawCenterString("■  STOP PENGUJIAN", 332, 273);
        } else {
            // Prominent full-width emerald start banner (NO STANDBY TEXT)
            _gfx->fillRoundRect(194, 258, 276, 46, 5, 0x02E0);
            _gfx->drawRoundRect(194, 258, 276, 46, 5, 0x07E0);
            _gfx->drawRoundRect(195, 259, 274, 44, 4, 0x07E0);
            _gfx->setTextColor(TFT_WHITE, 0x02E0);
            _gfx->setTextSize(2);
            _gfx->drawCenterString("▶  MULAI TES EPS", 332, 273);
        }
        _lastRunning = state.isRunning;
    }
}

// ============================================================================
// TAB 2: PENGATURAN OEM & KALIBRASI PRESISI
// ============================================================================

void PageEpsTester::_drawStaticLayoutTab2() {
    _gfx->fillRect(0, 42, 480, 278, TFT_BLACK);

    static const char* LABELS[] = {
        "PRESET OEM  :",
        "CENTER TRQ  :",
        "SPAN BELOK  :",
        "PULSA VSS   :",
        "CAL A1 TRQ1 :",
        "CAL A2 TRQ2 :",
        "SIMPAN NVS  :"
    };

    for (uint8_t i = 0; i < TAB2_TOTAL_ROWS; ++i) {
        int32_t y = TAB2_ROW_Y[i];
        int32_t h = (i == 4 || i == 5) ? 36 : 34;
        _gfx->fillRoundRect(10, y, 460, h, 4, 0x10A2);
        _gfx->setTextColor(TFT_WHITE, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->drawString(LABELS[i], 18, y + 8);
    }

    // Static subtitles drawn once here to avoid any runtime redraw flicker
    _gfx->setTextColor(0xCE79, 0x10A2);
    _gfx->setTextSize(1);
    _gfx->drawString("Kalibrasi Rasio Pulsa Speedo VSS (Putar: +/- 50)", 125, TAB2_ROW_Y[3] + 20);
    _gfx->drawString("Menyimpan rasio pembagi tegangan permanen ke Flash", 125, TAB2_ROW_Y[6] + 20);
}

void PageEpsTester::_drawRowHighlightTab2(uint8_t row, bool selected) {
    if (row >= TAB2_TOTAL_ROWS) return;
    int32_t y = TAB2_ROW_Y[row];
    int32_t h = (row == 4 || row == 5) ? 36 : 34;
    uint32_t borderColor = selected ? 0xFFE0 : 0x31A6;

    _gfx->drawRoundRect(9, y - 1, 462, h + 2, 5, selected ? 0xFFE0 : TFT_BLACK);
    _gfx->drawRoundRect(10, y, 460, h, 4, borderColor);
}

void PageEpsTester::_renderValuesTab2(const EcuEngine::EpsController& controller) {
    const auto& cfg = controller.getConfig();
    const auto& state = controller.getState();

    // Row 0: Preset
    uint8_t curPreset = static_cast<uint8_t>(cfg.preset);
    if (curPreset != _lastPreset) {
        const char* pName = (curPreset < 9) ? PRESET_NAMES[curPreset] : "Custom";
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->setTextPadding(335);
        _gfx->drawString(pName, 125, TAB2_ROW_Y[0] + 4);

        const char* pModel = (curPreset < 9) ? PRESET_MODELS[curPreset] : "Manual Tuning";
        _gfx->setTextColor(0xCE79, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->setTextPadding(335);
        _gfx->drawString(pModel, 125, TAB2_ROW_Y[0] + 20);
        _gfx->setTextPadding(0);
        _lastPreset = curPreset;
    }

    // Row 1: Independent Center Voltage (TRQ1 & TRQ2)
    if (cfg.trq1CenterVoltage != _lastT1Center || cfg.trq2CenterVoltage != _lastT2Center || _trqCenterFocus != _lastCenterFocus) {
        char buf[64];
        if (_trqCenterFocus == 0) {
            snprintf(buf, sizeof(buf), "[T1]:%.3fV   T2:%.3fV", cfg.trq1CenterVoltage, cfg.trq2CenterVoltage);
        } else {
            snprintf(buf, sizeof(buf), " T1:%.3fV  [T2]:%.3fV", cfg.trq1CenterVoltage, cfg.trq2CenterVoltage);
        }
        _gfx->setTextColor(0x07FF, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->setTextPadding(335);
        _gfx->drawString(buf, 125, TAB2_ROW_Y[1] + 4);

        char subBuf[64];
        const char* focStr = (_trqCenterFocus == 0) ? "Fokus TRQ1" : "Fokus TRQ2";
        snprintf(subBuf, sizeof(subBuf), "[%s] Knob: +/-10mV | Klik: Pindah | Joy: +/-5mV", focStr);
        _gfx->setTextColor(0xCE79, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->setTextPadding(335);
        _gfx->drawString(subBuf, 125, TAB2_ROW_Y[1] + 20);
        _gfx->setTextPadding(0);

        _lastT1Center = cfg.trq1CenterVoltage;
        _lastT2Center = cfg.trq2CenterVoltage;
        _lastCenterFocus = _trqCenterFocus;
    }

    // Row 2: Independent Span Voltage (TRQ1 & TRQ2)
    if (cfg.trq1VoltageSpan != _lastT1Span || cfg.trq2VoltageSpan != _lastT2Span || _trqSpanFocus != _lastSpanFocus) {
        char buf[64];
        if (_trqSpanFocus == 0) {
            snprintf(buf, sizeof(buf), "[T1]:+/-%.3fV  T2:+/-%.3fV", cfg.trq1VoltageSpan, cfg.trq2VoltageSpan);
        } else {
            snprintf(buf, sizeof(buf), " T1:+/-%.3fV [T2]:+/-%.3fV", cfg.trq1VoltageSpan, cfg.trq2VoltageSpan);
        }
        _gfx->setTextColor(0xFD20, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->setTextPadding(335);
        _gfx->drawString(buf, 125, TAB2_ROW_Y[2] + 4);

        char subBuf[64];
        const char* focStr = (_trqSpanFocus == 0) ? "Fokus TRQ1" : "Fokus TRQ2";
        snprintf(subBuf, sizeof(subBuf), "[%s] Knob: +/-10mV | Klik: Pindah | Joy: +/-5mV", focStr);
        _gfx->setTextColor(0xCE79, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->setTextPadding(335);
        _gfx->drawString(subBuf, 125, TAB2_ROW_Y[2] + 20);
        _gfx->setTextPadding(0);

        _lastT1Span = cfg.trq1VoltageSpan;
        _lastT2Span = cfg.trq2VoltageSpan;
        _lastSpanFocus = _trqSpanFocus;
    }

    // Row 3: Pulsa VSS per km
    if (cfg.vssPulsePerKm != _lastVssPulses) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%.0f Pulsa/km", cfg.vssPulsePerKm);
        _gfx->setTextColor(0x07E0, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->setTextPadding(335);
        _gfx->drawString(buf, 125, TAB2_ROW_Y[3] + 4);
        _gfx->setTextPadding(0);
        _lastVssPulses = cfg.vssPulsePerKm;
    }

    // Row 4: Kalibrasi A1 (TRQ1) Multiplier & Offset + Live Preview
    if (cfg.trq1AdcScale != _lastT1Scale || cfg.trq1AdcOffset != _lastT1Offset) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Scale: %.3fx (Knob) | Off: %+.3fV (Joy)", cfg.trq1AdcScale, cfg.trq1AdcOffset);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->setTextPadding(335);
        _gfx->drawString(buf, 125, TAB2_ROW_Y[4] + 4);
        _gfx->setTextPadding(0);
        _lastT1Scale = cfg.trq1AdcScale;
        _lastT1Offset = cfg.trq1AdcOffset;
    }

    int32_t fb1PreviewMv = static_cast<int32_t>(lroundf(state.trq1FeedbackVoltage * 1000.0f));
    if (fb1PreviewMv != _lastT1FbPreviewMv) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Live: %.3f V", state.trq1FeedbackVoltage);
        _gfx->setTextColor(0x07FF, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->setTextPadding(170);
        _gfx->drawString(buf, 125, TAB2_ROW_Y[4] + 16);
        _gfx->setTextPadding(0);
        _lastT1FbPreviewMv = fb1PreviewMv;
    }

    int32_t t1TargetMv = static_cast<int32_t>(lroundf(state.trq1Voltage * 1000.0f));
    if (t1TargetMv != _lastT1TargetMv) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Target: %.3fV", state.trq1Voltage);
        _gfx->setTextColor(0xCE79, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->setTextPadding(140);
        _gfx->drawString(buf, 315, TAB2_ROW_Y[4] + 20);
        _gfx->setTextPadding(0);
        _lastT1TargetMv = t1TargetMv;
    }

    // Row 5: Kalibrasi A2 (TRQ2) Multiplier & Offset + Live Preview
    if (cfg.trq2AdcScale != _lastT2Scale || cfg.trq2AdcOffset != _lastT2Offset) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Scale: %.3fx (Knob) | Off: %+.3fV (Joy)", cfg.trq2AdcScale, cfg.trq2AdcOffset);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->setTextPadding(335);
        _gfx->drawString(buf, 125, TAB2_ROW_Y[5] + 4);
        _gfx->setTextPadding(0);
        _lastT2Scale = cfg.trq2AdcScale;
        _lastT2Offset = cfg.trq2AdcOffset;
    }

    int32_t fb2PreviewMv = static_cast<int32_t>(lroundf(state.trq2FeedbackVoltage * 1000.0f));
    if (fb2PreviewMv != _lastT2FbPreviewMv) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Live: %.3f V", state.trq2FeedbackVoltage);
        _gfx->setTextColor(0x07FF, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->setTextPadding(170);
        _gfx->drawString(buf, 125, TAB2_ROW_Y[5] + 16);
        _gfx->setTextPadding(0);
        _lastT2FbPreviewMv = fb2PreviewMv;
    }

    int32_t t2TargetMv = static_cast<int32_t>(lroundf(state.trq2Voltage * 1000.0f));
    if (t2TargetMv != _lastT2TargetMv) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Target: %.3fV", state.trq2Voltage);
        _gfx->setTextColor(0xCE79, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->setTextPadding(140);
        _gfx->drawString(buf, 315, TAB2_ROW_Y[5] + 20);
        _gfx->setTextPadding(0);
        _lastT2TargetMv = t2TargetMv;
    }

    // Row 6: Simpan ke EEPROM / NVS
    bool savedActive = (millis() < _savedNoticeUntilMs);
    if (savedActive != _lastSavedNoticeActive) {
        _gfx->setTextPadding(335);
        if (savedActive) {
            _gfx->setTextColor(0x07E0, 0x10A2);
            _gfx->setTextSize(2);
            _gfx->drawString("[V] TERSIMPAN KE EEPROM!", 125, TAB2_ROW_Y[6] + 6);
        } else {
            _gfx->setTextColor(0xFFE0, 0x10A2);
            _gfx->setTextSize(2);
            _gfx->drawString("[ KLIK KNOB ] Simpan Kalibrasi", 125, TAB2_ROW_Y[6] + 4);
        }
        _gfx->setTextPadding(0);
        _lastSavedNoticeActive = savedActive;
    }
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void PageEpsTester::onEncoderTurn(uint8_t currentTab, int32_t delta, uint8_t editRow,
                                 EcuEngine::EpsController& controller) {
    const auto& cfg = controller.getConfig();
    if (currentTab == 1) { // Tab 1: COCKPIT
        switch (editRow) {
            case 0: controller.setSpeed(cfg.speedKmh + (delta * 5.0f)); break;
            case 1: controller.setRpm(cfg.targetRpm + (delta * 100)); break;
            case 2: {
                // Steer torque moving smoothly in 2% steps (0.02f)
                float newTorque = cfg.steerTorque + (delta * 0.02f);
                if (newTorque < -1.0f) newTorque = -1.0f;
                if (newTorque > 1.0f) newTorque = 1.0f;
                controller.setSteerTorque(newTorque);
                break;
            }
            case 3: controller.setAutoSweep(!cfg.autoSweep); break;
            case 4: controller.toggleRunning(); break;
            default: break;
        }
    } else { // Tab 2: PENGATURAN OEM & KALIBRASI
        switch (editRow) {
            case 0: {
                int32_t p = static_cast<int32_t>(cfg.preset) + (delta > 0 ? 1 : -1);
                if (p < 0) p = 8;
                if (p > 8) p = 0;
                controller.setPreset(static_cast<EcuEngine::EpsOemPreset>(p));
                break;
            }
            case 1: {
                if (_trqCenterFocus == 0) {
                    controller.setTrq1CenterVoltage(cfg.trq1CenterVoltage + (delta * 0.010f));
                } else {
                    controller.setTrq2CenterVoltage(cfg.trq2CenterVoltage + (delta * 0.010f));
                }
                break;
            }
            case 2: {
                if (_trqSpanFocus == 0) {
                    controller.setTrq1SpanVoltage(cfg.trq1VoltageSpan + (delta * 0.010f));
                } else {
                    controller.setTrq2SpanVoltage(cfg.trq2VoltageSpan + (delta * 0.010f));
                }
                break;
            }
            case 3: controller.setVssPulsePerKm(cfg.vssPulsePerKm + (delta * 50.0f)); break;
            case 4: controller.setTrq1Scale(cfg.trq1AdcScale + (delta * 0.002f)); break;
            case 5: controller.setTrq2Scale(cfg.trq2AdcScale + (delta * 0.002f)); break;
            case 6: controller.saveCalibration(); _savedNoticeUntilMs = millis() + 2500; break;
            default: break;
        }
    }
}

void PageEpsTester::onJoystickAction(uint8_t currentTab, EcuHal::JoyAction action,
                                    EcuEngine::EpsController& controller) {
    const auto& cfg = controller.getConfig();
    if (currentTab == 1) {
        if (action == EcuHal::JoyAction::Left) {
            float newTorque = cfg.steerTorque - 0.05f;
            if (newTorque < -1.0f) newTorque = -1.0f;
            controller.setSteerTorque(newTorque);
        } else if (action == EcuHal::JoyAction::Right) {
            float newTorque = cfg.steerTorque + 0.05f;
            if (newTorque > 1.0f) newTorque = 1.0f;
            controller.setSteerTorque(newTorque);
        }
    } else {
        if (action == EcuHal::JoyAction::Left) {
            if (_lastEditRow == 0) {
                int32_t p = static_cast<int32_t>(cfg.preset) - 1;
                if (p < 0) p = 8;
                controller.setPreset(static_cast<EcuEngine::EpsOemPreset>(p));
            } else if (_lastEditRow == 1) {
                if (_trqCenterFocus == 0) controller.setTrq1CenterVoltage(cfg.trq1CenterVoltage - 0.005f);
                else controller.setTrq2CenterVoltage(cfg.trq2CenterVoltage - 0.005f);
            } else if (_lastEditRow == 2) {
                if (_trqSpanFocus == 0) controller.setTrq1SpanVoltage(cfg.trq1VoltageSpan - 0.005f);
                else controller.setTrq2SpanVoltage(cfg.trq2VoltageSpan - 0.005f);
            } else if (_lastEditRow == 4) {
                controller.setTrq1Offset(cfg.trq1AdcOffset - 0.005f); // Trim -5 mV
            } else if (_lastEditRow == 5) {
                controller.setTrq2Offset(cfg.trq2AdcOffset - 0.005f); // Trim -5 mV
            }
        } else if (action == EcuHal::JoyAction::Right) {
            if (_lastEditRow == 0) {
                int32_t p = static_cast<int32_t>(cfg.preset) + 1;
                if (p > 8) p = 0;
                controller.setPreset(static_cast<EcuEngine::EpsOemPreset>(p));
            } else if (_lastEditRow == 1) {
                if (_trqCenterFocus == 0) controller.setTrq1CenterVoltage(cfg.trq1CenterVoltage + 0.005f);
                else controller.setTrq2CenterVoltage(cfg.trq2CenterVoltage + 0.005f);
            } else if (_lastEditRow == 2) {
                if (_trqSpanFocus == 0) controller.setTrq1SpanVoltage(cfg.trq1VoltageSpan + 0.005f);
                else controller.setTrq2SpanVoltage(cfg.trq2VoltageSpan + 0.005f);
            } else if (_lastEditRow == 4) {
                controller.setTrq1Offset(cfg.trq1AdcOffset + 0.005f); // Trim +5 mV
            } else if (_lastEditRow == 5) {
                controller.setTrq2Offset(cfg.trq2AdcOffset + 0.005f); // Trim +5 mV
            }
        }
    }
}

void PageEpsTester::onEncoderClick(uint8_t currentTab, uint8_t editRow,
                                  EcuEngine::EpsController& controller) {
    if (currentTab == 1) {
        if (editRow == 3) {
            controller.setAutoSweep(!controller.getConfig().autoSweep);
        } else if (editRow == 4) {
            controller.toggleRunning();
        } else {
            controller.toggleRunning();
        }
    } else {
        if (editRow == 0) {
            int32_t p = static_cast<int32_t>(controller.getConfig().preset) + 1;
            if (p > 8) p = 0;
            controller.setPreset(static_cast<EcuEngine::EpsOemPreset>(p));
        } else if (editRow == 1) {
            _trqCenterFocus = (_trqCenterFocus == 0) ? 1 : 0;
            _lastCenterFocus = 0xFF;
        } else if (editRow == 2) {
            _trqSpanFocus = (_trqSpanFocus == 0) ? 1 : 0;
            _lastSpanFocus = 0xFF;
        } else if (editRow == 6) {
            controller.saveCalibration();
            _savedNoticeUntilMs = millis() + 2500;
        }
    }
}

} // namespace EcuUi
