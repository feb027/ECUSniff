#include "page_speedo_tester.h"

namespace EcuUi {

static const float PPK_PRESETS[] = { 2548.0f, 4000.0f, 8000.0f, 23333.0f, 30000.0f };
static const float PPR_PRESETS[] = { 1.0f, 2.0f, 3.0f, 4.0f, 0.5f };
static const char* ROUTE_NAMES[] = { "Dual PWM", "Single Fuel", "Single Temp", "Dual MCP4725" };

PageSpeedoTester::PageSpeedoTester(LovyanGFX* gfx) : _gfx(gfx) {}

void PageSpeedoTester::init() {
    _lastTab = 0xFF; _lastEditRow = 0xFF;
    _lastKmh = -1; _lastRpm = -1; _lastTemp = -1; _lastFuel = -1;
    _lastHzKmh = -1.0f; _lastHzRpm = -1.0f; _lastVoltTemp = -1.0f; _lastVoltFuel = -1.0f;
    _lastRunning = false; _lastSweep = false;
    _lastEnKmh = false; _lastEnRpm = false; _lastEnTemp = false; _lastEnFuel = false;
}

void PageSpeedoTester::_drawPanelFrame(int32_t x, int32_t y, int32_t w, int32_t h, bool isSelected) {
    _gfx->drawRoundRect(x - 1, y - 1, w + 2, h + 2, 6, isSelected ? 0xFFE0 : TFT_BLACK);
    _gfx->drawRoundRect(x, y, w, h, 6, isSelected ? 0xFFE0 : 0x52AA);
}

void PageSpeedoTester::render(uint8_t currentTab, bool fullRedraw, uint8_t editRow,
                             const EcuEngine::SpeedoController& controller) {
    if (currentTab != _lastTab || fullRedraw) {
        _gfx->fillRect(0, 42, 480, 278, TFT_BLACK);
        _lastTab = currentTab; fullRedraw = true;
        _lastEditRow = 0xFF; _lastKmh = -1; _lastRpm = -1; _lastTemp = -1; _lastFuel = -1;
    }

    if (currentTab == 1) _renderTabCockpit(fullRedraw, editRow, controller);
    else if (currentTab == 2) _renderTabCalibration(fullRedraw, editRow, controller);
    else if (currentTab == 3) _renderTabHardware(fullRedraw, editRow, controller);

    _lastEditRow = editRow;
}

void PageSpeedoTester::_renderTabCockpit(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    const auto& st = controller.getState();

    int32_t curKmh  = st.isRunning ? static_cast<int32_t>(st.currentKmh) : cfg.speedoKmh;
    int32_t curRpm  = st.isRunning ? static_cast<int32_t>(st.currentRpm) : cfg.speedoRpm;
    int32_t curTemp = st.isRunning ? static_cast<int32_t>(st.currentTemp) : cfg.speedoTempPercent;
    int32_t curFuel = st.isRunning ? static_cast<int32_t>(st.currentFuel) : cfg.speedoFuelPercent;

    if (fullRedraw) {
        _gfx->fillRoundRect(8, 46, 228, 126, 6, 0x0841);
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1); _gfx->drawString("SPEEDOMETER", 18, 54);
        _gfx->drawRoundRect(18, 156, 208, 8, 3, 0x31A6);

        _gfx->fillRoundRect(244, 46, 228, 126, 6, 0x0841);
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(1); _gfx->drawString("TACHOMETER", 254, 54);
        _gfx->drawRoundRect(254, 156, 208, 8, 3, 0x31A6);

        _gfx->fillRoundRect(8, 178, 228, 78, 6, 0x0841);
        _gfx->setTextColor(0xFD20, 0x0841); _gfx->setTextSize(1); _gfx->drawString("SUHU MESIN (ECT)", 18, 186);
        _gfx->drawRoundRect(18, 238, 208, 10, 3, 0x31A6);

        _gfx->fillRoundRect(244, 178, 228, 78, 6, 0x0841);
        _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(1); _gfx->drawString("KETINGGIAN BENSIN", 254, 186);
        _gfx->drawRoundRect(254, 238, 208, 10, 3, 0x31A6);

        _gfx->fillRoundRect(8, 262, 464, 52, 6, 0x0841);
    }

    if (fullRedraw || editRow != _lastEditRow) {
        _drawPanelFrame(8, 46, 228, 126, editRow == 0);
        _drawPanelFrame(244, 46, 228, 126, editRow == 1);
        _drawPanelFrame(8, 178, 228, 78, editRow == 2);
        _drawPanelFrame(244, 178, 228, 78, editRow == 3);
        _drawPanelFrame(8, 262, 464, 52, editRow == 4);
    }

    // 1. SPEED PANEL
    if (fullRedraw || curKmh != _lastKmh || cfg.speedoEnableKmh != _lastEnKmh || st.hzKmh != _lastHzKmh) {
        _gfx->fillRoundRect(162, 52, 66, 18, 3, cfg.speedoEnableKmh ? 0x03E0 : 0x7800);
        _gfx->setTextColor(cfg.speedoEnableKmh ? 0x07E0 : 0xF800, cfg.speedoEnableKmh ? 0x03E0 : 0x7800);
        _gfx->setTextSize(1); _gfx->drawCenterString(cfg.speedoEnableKmh ? "CH1: ON" : "CH1: OFF", 195, 57);

        _gfx->fillRect(18, 72, 208, 42, 0x0841);
        char sBuf[8]; snprintf(sBuf, sizeof(sBuf), "%3d", curKmh);
        _gfx->setTextColor(cfg.speedoEnableKmh ? 0x07FF : 0x7BEF, 0x0841);
        _gfx->setTextSize(4); _gfx->drawString(sBuf, 28, 76);
        _gfx->setTextSize(2); _gfx->drawString("KM/H", 136, 92);

        _gfx->fillRect(18, 122, 208, 24, 0x0841);
        _gfx->setTextColor(0x52AA, 0x0841); _gfx->setTextSize(1);
        char sub[32]; snprintf(sub, sizeof(sub), "Pulsa: %.1f Hz | %.0f P/KM", st.hzKmh, cfg.pulsePerKm);
        _gfx->drawString(sub, 20, 128);

        int32_t barW = constrain((curKmh * 204) / 300, 0, 204);
        _gfx->fillRect(20, 158, 204, 4, 0x10A2);
        if (barW > 0) _gfx->fillRect(20, 158, barW, 4, 0x07FF);

        _lastKmh = curKmh; _lastEnKmh = cfg.speedoEnableKmh; _lastHzKmh = st.hzKmh;
    }

    // 2. RPM PANEL
    if (fullRedraw || curRpm != _lastRpm || cfg.speedoEnableRpm != _lastEnRpm || st.hzRpm != _lastHzRpm) {
        _gfx->fillRoundRect(398, 52, 66, 18, 3, cfg.speedoEnableRpm ? 0x03E0 : 0x7800);
        _gfx->setTextColor(cfg.speedoEnableRpm ? 0x07E0 : 0xF800, cfg.speedoEnableRpm ? 0x03E0 : 0x7800);
        _gfx->setTextSize(1); _gfx->drawCenterString(cfg.speedoEnableRpm ? "CH2: ON" : "CH2: OFF", 431, 57);

        _gfx->fillRect(254, 72, 208, 42, 0x0841);
        char rBuf[8]; snprintf(rBuf, sizeof(rBuf), "%5d", curRpm);
        _gfx->setTextColor(cfg.speedoEnableRpm ? 0x07E0 : 0x7BEF, 0x0841);
        _gfx->setTextSize(4); _gfx->drawString(rBuf, 258, 76);
        _gfx->setTextSize(2); _gfx->drawString("RPM", 398, 92);

        _gfx->fillRect(254, 122, 208, 24, 0x0841);
        _gfx->setTextColor(0x52AA, 0x0841); _gfx->setTextSize(1);
        char sub[32]; snprintf(sub, sizeof(sub), "Pulsa: %.1f Hz | %.1f PPR", st.hzRpm, cfg.speedoTachoPpr);
        _gfx->drawString(sub, 256, 128);

        int32_t barW = constrain((curRpm * 204) / cfg.speedoMaxRpm, 0, 204);
        _gfx->fillRect(256, 158, 204, 4, 0x10A2);
        if (barW > 0) _gfx->fillRect(256, 158, barW, 4, 0x07E0);

        _lastRpm = curRpm; _lastEnRpm = cfg.speedoEnableRpm; _lastHzRpm = st.hzRpm;
    }

    // 3. TEMP PANEL
    if (fullRedraw || curTemp != _lastTemp || cfg.speedoEnableTemp != _lastEnTemp || st.voltTemp != _lastVoltTemp) {
        _gfx->fillRect(18, 204, 208, 28, 0x0841);
        char tBuf[32]; snprintf(tBuf, sizeof(tBuf), "%3d %% (%.2fV)", curTemp, st.voltTemp);
        _gfx->setTextColor(cfg.speedoEnableTemp ? 0xFD20 : 0x7BEF, 0x0841);
        _gfx->setTextSize(2); _gfx->drawString(tBuf, 18, 208);

        int32_t barW = constrain((curTemp * 204) / 100, 0, 204);
        _gfx->fillRect(20, 240, 204, 6, 0x10A2);
        if (barW > 0) _gfx->fillRect(20, 240, barW, 6, (curTemp > 85) ? 0xF800 : ((curTemp < 30) ? 0x07FF : 0x07E0));

        _lastTemp = curTemp; _lastEnTemp = cfg.speedoEnableTemp; _lastVoltTemp = st.voltTemp;
    }

    // 4. FUEL PANEL
    if (fullRedraw || curFuel != _lastFuel || cfg.speedoEnableFuel != _lastEnFuel || st.voltFuel != _lastVoltFuel) {
        _gfx->fillRect(254, 204, 208, 28, 0x0841);
        char fBuf[32]; snprintf(fBuf, sizeof(fBuf), "%3d %% (%.2fV)", curFuel, st.voltFuel);
        _gfx->setTextColor(cfg.speedoEnableFuel ? 0xFFE0 : 0x7BEF, 0x0841);
        _gfx->setTextSize(2); _gfx->drawString(fBuf, 254, 208);

        int32_t barW = constrain((curFuel * 204) / 100, 0, 204);
        _gfx->fillRect(256, 240, 204, 6, 0x10A2);
        if (barW > 0) _gfx->fillRect(256, 240, barW, 6, (curFuel < 20) ? 0xF800 : 0xFFE0);

        _lastFuel = curFuel; _lastEnFuel = cfg.speedoEnableFuel; _lastVoltFuel = st.voltFuel;
    }

    // 5. MASTER STATUS STRIP
    if (fullRedraw || st.isRunning != _lastRunning || cfg.autoSweep != _lastSweep) {
        _gfx->fillRect(16, 270, 448, 36, 0x0841);
        if (st.isRunning) {
            _gfx->fillRoundRect(20, 272, 110, 32, 4, 0x03E0); _gfx->drawRoundRect(20, 272, 110, 32, 4, 0x07E0);
            _gfx->setTextColor(0x07E0, 0x03E0); _gfx->setTextSize(2); _gfx->drawCenterString("RUNNING", 75, 280);
        } else {
            _gfx->fillRoundRect(20, 272, 110, 32, 4, 0x3800); _gfx->drawRoundRect(20, 272, 110, 32, 4, 0xF800);
            _gfx->setTextColor(0xF800, 0x3800); _gfx->setTextSize(2); _gfx->drawCenterString("STOPPED", 75, 280);
        }

        _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(2);
        if (cfg.autoSweep) {
            char swBuf[32]; snprintf(swBuf, sizeof(swBuf), "SWEEP: ON (%.0fs)", cfg.sweepTimeSec);
            _gfx->drawString(swBuf, 145, 274);
        } else {
            _gfx->drawString("SWEEP: OFF (MANUAL)", 145, 274);
        }

        _gfx->setTextColor(0x52AA, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("Joy-X/Y: Pilih | Putar: Ubah Nilai | Klik: Run/Stop", 145, 294);
        _lastRunning = st.isRunning; _lastSweep = cfg.autoSweep;
    }
}

void PageSpeedoTester::_renderTabCalibration(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    static constexpr uint16_t RY[] = {48, 88, 128, 168, 208, 248};

    if (fullRedraw) {
        _gfx->drawFastHLine(0, 290, 480, 0x03E0); _gfx->setTextColor(0x07FF, TFT_BLACK); _gfx->setTextSize(1);
        _gfx->drawString("Joy-Y: Baris | Putar: +/-1% Kalibrasi | Klik: Reset | Joy-L: Menu", 10, 302);
        const char* LABELS[] = { "ECT MIN (COLD 0%) :", "ECT MID (NORM 50%):", "ECT MAX (HOT 100%):", "FUEL MIN (EMPTY 0%):", "FUEL MID (HALF 50%):", "FUEL MAX (FULL100%):" };
        for (uint8_t i = 0; i < 6; ++i) {
            _gfx->fillRoundRect(12, RY[i], 456, 36, 4, 0x10A2);
            _gfx->setTextColor(TFT_WHITE, 0x10A2); _gfx->setTextSize(1); _gfx->drawString(LABELS[i], 22, RY[i] + 12);
        }
    }
    for (uint8_t i = 0; i < 6; ++i) _drawPanelFrame(12, RY[i], 456, 36, i == editRow);

    int32_t vals[] = { cfg.tempCalMin, cfg.tempCalMid, cfg.tempCalMax, cfg.fuelCalMin, cfg.fuelCalMid, cfg.fuelCalMax };
    uint32_t colors[] = { 0xFD20, 0xFD20, 0xFD20, 0xFFE0, 0xFFE0, 0xFFE0 };
    for (uint8_t i = 0; i < 6; ++i) {
        _gfx->fillRect(190, RY[i] + 4, 270, 28, 0x10A2);
        _gfx->setTextColor(colors[i], 0x10A2); _gfx->setTextSize(2);
        char buf[32]; snprintf(buf, sizeof(buf), "%3d %% Cal", vals[i]);
        _gfx->drawString(buf, 195, RY[i] + 8);
    }
}

void PageSpeedoTester::_renderTabHardware(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    const auto& st = controller.getState();
    static constexpr uint16_t RY[] = {48, 96, 144, 192, 240};

    if (fullRedraw) {
        _gfx->drawFastHLine(0, 290, 480, 0x03E0); _gfx->setTextColor(0x07FF, TFT_BLACK); _gfx->setTextSize(1);
        _gfx->drawString("Joy-Y: Baris | Putar: Ganti Preset / Routing | Klik: Toggle", 16, 302);
        const char* LABELS[] = { "PPK PULSES/KM :", "TACHO PPR/CYL :", "HARDWARE ROUTE:", "GAUGE CURVE   :", "SWEEP TIME    :" };
        for (uint8_t i = 0; i < 5; ++i) {
            _gfx->fillRoundRect(12, RY[i], 456, 42, 4, 0x10A2);
            _gfx->setTextColor(TFT_WHITE, 0x10A2); _gfx->setTextSize(1); _gfx->drawString(LABELS[i], 22, RY[i] + 15);
        }
    }
    for (uint8_t i = 0; i < 5; ++i) _drawPanelFrame(12, RY[i], 456, 42, i == editRow);

    _gfx->fillRect(150, RY[0] + 4, 312, 34, 0x10A2); _gfx->setTextColor(0x07FF, 0x10A2); _gfx->setTextSize(2);
    char buf[48]; snprintf(buf, sizeof(buf), "%.0f P/KM", cfg.pulsePerKm); _gfx->drawString(buf, 155, RY[0] + 10);

    _gfx->fillRect(150, RY[1] + 4, 312, 34, 0x10A2); _gfx->setTextColor(0x07E0, 0x10A2);
    snprintf(buf, sizeof(buf), "%.1f PPR", cfg.speedoTachoPpr); _gfx->drawString(buf, 155, RY[1] + 10);

    _gfx->fillRect(150, RY[2] + 2, 312, 38, 0x10A2); _gfx->setTextColor(0xFFE0, 0x10A2); _gfx->setTextSize(1);
    uint8_t rIdx = static_cast<uint8_t>(cfg.dacRouting);
    _gfx->drawString(rIdx < 4 ? ROUTE_NAMES[rIdx] : "Dual DAC", 155, RY[2] + 6);
    _gfx->setTextColor(st.dacFuelFound ? 0x07E0 : 0xF800, 0x10A2); _gfx->drawString(st.dacFuelFound ? "DAC1: ON" : "DAC1: OFF", 155, RY[2] + 22);
    _gfx->setTextColor(st.dacTempFound ? 0x07E0 : 0xF800, 0x10A2); _gfx->drawString(st.dacTempFound ? "DAC2: ON" : "DAC2: OFF", 305, RY[2] + 22);

    _gfx->fillRect(150, RY[3] + 4, 312, 34, 0x10A2); _gfx->setTextColor(0xFD20, 0x10A2); _gfx->setTextSize(2);
    _gfx->drawString(cfg.gaugeCurve == EcuEngine::SpeedoGaugeCurve::SqrtThermal ? "Non-Linier" : "Linier 1:1", 155, RY[3] + 10);

    _gfx->fillRect(150, RY[4] + 4, 312, 34, 0x10A2); _gfx->setTextColor(0xD69F, 0x10A2);
    snprintf(buf, sizeof(buf), "%.0f Detik", cfg.sweepTimeSec); _gfx->drawString(buf, 155, RY[4] + 10);
}

void PageSpeedoTester::onEncoderTurn(uint8_t currentTab, int32_t delta, uint8_t editRow, EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    if (currentTab == 1) {
        switch (editRow) {
            case 0: controller.setKmh(cfg.speedoKmh + (delta * 5)); break;
            case 1: controller.setRpm(cfg.speedoRpm + (delta * 100)); break;
            case 2: controller.setTemp(cfg.speedoTempPercent + (delta * 1)); break;
            case 3: controller.setFuel(cfg.speedoFuelPercent + (delta * 1)); break;
            case 4: controller.setAutoSweep(!cfg.autoSweep); break;
        }
    } else if (currentTab == 2) {
        if (editRow == 0) controller.setTempCal(cfg.tempCalMin + delta, cfg.tempCalMid, cfg.tempCalMax);
        else if (editRow == 1) controller.setTempCal(cfg.tempCalMin, cfg.tempCalMid + delta, cfg.tempCalMax);
        else if (editRow == 2) controller.setTempCal(cfg.tempCalMin, cfg.tempCalMid, cfg.tempCalMax + delta);
        else if (editRow == 3) controller.setFuelCal(cfg.fuelCalMin + delta, cfg.fuelCalMid, cfg.fuelCalMax);
        else if (editRow == 4) controller.setFuelCal(cfg.fuelCalMin, cfg.fuelCalMid + delta, cfg.fuelCalMax);
        else if (editRow == 5) controller.setFuelCal(cfg.fuelCalMin, cfg.fuelCalMid, cfg.fuelCalMax + delta);
    } else if (currentTab == 3) {
        if (editRow == 0) {
            int32_t idx = 1; for (int i = 0; i < 5; ++i) if (abs(cfg.pulsePerKm - PPK_PRESETS[i]) < 10) idx = i;
            controller.setPulsePerKm(PPK_PRESETS[(idx + (delta > 0 ? 1 : -1) + 5) % 5]);
        } else if (editRow == 1) {
            int32_t idx = 1; for (int i = 0; i < 5; ++i) if (abs(cfg.speedoTachoPpr - PPR_PRESETS[i]) < 0.1f) idx = i;
            controller.setTachoPpr(PPR_PRESETS[(idx + (delta > 0 ? 1 : -1) + 5) % 5]);
        } else if (editRow == 2) {
            controller.setDacRouting(static_cast<EcuEngine::SpeedoDacRouting>((static_cast<int32_t>(cfg.dacRouting) + (delta > 0 ? 1 : -1) + 4) % 4));
        } else if (editRow == 3) {
            controller.setGaugeCurve(cfg.gaugeCurve == EcuEngine::SpeedoGaugeCurve::SqrtThermal ? EcuEngine::SpeedoGaugeCurve::Linear : EcuEngine::SpeedoGaugeCurve::SqrtThermal);
        } else if (editRow == 4) controller.setSweepTimeSec(cfg.sweepTimeSec + delta);
    }
}

void PageSpeedoTester::onEncoderClick(uint8_t currentTab, uint8_t editRow, EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    if (currentTab == 1) {
        if (editRow == 0) controller.setChannelEnable(0, !cfg.speedoEnableKmh);
        else if (editRow == 1) controller.setChannelEnable(1, !cfg.speedoEnableRpm);
        else if (editRow == 2) controller.setChannelEnable(2, !cfg.speedoEnableTemp);
        else if (editRow == 3) controller.setChannelEnable(3, !cfg.speedoEnableFuel);
        else if (editRow == 4) controller.toggleRunning();
    } else if (currentTab == 2) {
        if (editRow <= 2) controller.setTempCal(0, 50, 100);
        else controller.setFuelCal(0, 50, 100);
    } else if (currentTab == 3 && editRow == 3) {
        controller.setGaugeCurve(cfg.gaugeCurve == EcuEngine::SpeedoGaugeCurve::SqrtThermal ? EcuEngine::SpeedoGaugeCurve::Linear : EcuEngine::SpeedoGaugeCurve::SqrtThermal);
    }
}

} // namespace EcuUi
