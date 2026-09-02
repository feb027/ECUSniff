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
    _lastRunning = false; _lastMode = 0xFF;
    _lastEnKmh = false; _lastEnRpm = false; _lastEnTemp = false; _lastEnFuel = false;
    for (uint8_t i = 0; i < 6; ++i) _lastCalVals[i] = -99;
    _lastPpk = -1.0f; _lastPpr = -1.0f; _lastRouting = 0xFF; _lastCurve = 0xFF;
    _lastSweepTime = -1.0f; _lastDac1 = false; _lastDac2 = false;
}

void PageSpeedoTester::_drawPanelFrame(int32_t x, int32_t y, int32_t w, int32_t h, bool isSelected) {
    _gfx->drawRoundRect(x - 1, y - 1, w + 2, h + 2, 6, isSelected ? 0xFFE0 : TFT_BLACK);
    _gfx->drawRoundRect(x, y, w, h, 6, isSelected ? 0xFFE0 : 0x52AA);
}

void PageSpeedoTester::render(uint8_t currentTab, bool fullRedraw, uint8_t editRow,
                             const EcuEngine::SpeedoController& controller) {
    if (currentTab != _lastTab || fullRedraw) {
        _gfx->fillRect(0, 40, 480, 280, TFT_BLACK);
        _lastTab = currentTab; fullRedraw = true;
        _lastEditRow = 0xFF; _lastKmh = -1; _lastRpm = -1; _lastTemp = -1; _lastFuel = -1;
        for (uint8_t i = 0; i < 6; ++i) _lastCalVals[i] = -99;
        _lastPpk = -1.0f; _lastPpr = -1.0f; _lastRouting = 0xFF; _lastCurve = 0xFF;
    }

    if (currentTab == 0) {
        if (fullRedraw) {
            _gfx->fillRoundRect(40, 100, 400, 130, 8, 0x0841);
            _gfx->drawRoundRect(40, 100, 400, 130, 8, 0xF800);
            _gfx->setTextColor(0xF800, 0x0841); _gfx->setTextSize(2);
            _gfx->drawCenterString("< KELUAR KE MENU UTAMA", 240, 125);
            _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
            _gfx->drawCenterString("Tekan / Klik Knob untuk Keluar", 240, 165);
            _gfx->setTextColor(0x07E0, 0x0841);
            _gfx->drawCenterString("Geser Joystick ke Kanan (>) untuk Batal", 240, 190);
        }
        return;
    }

    if (currentTab == 1) _renderTabCockpit(fullRedraw, editRow, controller);
    else if (currentTab == 2) _renderTabCalibration(fullRedraw, editRow, controller);
    else if (currentTab == 3) _renderTabHardware(fullRedraw, editRow, controller);
    _lastEditRow = editRow;
}

void PageSpeedoTester::_renderTabCockpit(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig(); const auto& st = controller.getState();
    int32_t curKmh  = st.isRunning ? static_cast<int32_t>(st.currentKmh) : cfg.speedoKmh;
    int32_t curRpm  = st.isRunning ? static_cast<int32_t>(st.currentRpm) : cfg.speedoRpm;
    int32_t curTemp = st.isRunning ? static_cast<int32_t>(st.currentTemp) : cfg.speedoTempPercent;
    int32_t curFuel = st.isRunning ? static_cast<int32_t>(st.currentFuel) : cfg.speedoFuelPercent;
    uint8_t curMode = static_cast<uint8_t>(cfg.runMode);

    if (fullRedraw) {
        _gfx->fillRoundRect(8, 46, 228, 126, 6, 0x0841); _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1); _gfx->drawString("SPEEDOMETER", 18, 54);
        _gfx->drawRoundRect(18, 156, 208, 8, 3, 0x31A6);
        _gfx->fillRoundRect(244, 46, 228, 126, 6, 0x0841); _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(1); _gfx->drawString("TACHOMETER", 254, 54);
        _gfx->drawRoundRect(254, 156, 208, 8, 3, 0x31A6);
        _gfx->fillRoundRect(8, 178, 228, 78, 6, 0x0841); _gfx->setTextColor(0xFD20, 0x0841); _gfx->setTextSize(1); _gfx->drawString("SUHU MESIN (ECT)", 18, 186);
        _gfx->drawRoundRect(18, 238, 208, 10, 3, 0x31A6);
        _gfx->fillRoundRect(244, 178, 228, 78, 6, 0x0841); _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(1); _gfx->drawString("KETINGGIAN BENSIN", 254, 186);
        _gfx->drawRoundRect(254, 238, 208, 10, 3, 0x31A6);
        _gfx->fillRoundRect(8, 262, 464, 52, 6, 0x0841);
    }
    if (fullRedraw || editRow != _lastEditRow) {
        _drawPanelFrame(8, 46, 228, 126, editRow == 0); _drawPanelFrame(244, 46, 228, 126, editRow == 1);
        _drawPanelFrame(8, 178, 228, 78, editRow == 2); _drawPanelFrame(244, 178, 228, 78, editRow == 3);
        _drawPanelFrame(8, 262, 464, 52, editRow == 4);
    }

    if (fullRedraw || curKmh != _lastKmh || cfg.speedoEnableKmh != _lastEnKmh || st.hzKmh != _lastHzKmh) {
        _gfx->fillRoundRect(162, 52, 66, 18, 3, cfg.speedoEnableKmh ? 0x03E0 : 0x7800);
        _gfx->setTextColor(cfg.speedoEnableKmh ? 0x07E0 : 0xF800, cfg.speedoEnableKmh ? 0x03E0 : 0x7800);
        _gfx->setTextSize(1); _gfx->drawCenterString(cfg.speedoEnableKmh ? "CH1: ON" : "CH1: OFF", 195, 57);
        _gfx->fillRect(18, 72, 208, 42, 0x0841);
        char sBuf[8]; snprintf(sBuf, sizeof(sBuf), "%3d", curKmh);
        _gfx->setTextColor(cfg.speedoEnableKmh ? 0x07FF : 0x7BEF, 0x0841); _gfx->setTextSize(4); _gfx->drawString(sBuf, 28, 76);
        _gfx->setTextSize(2); _gfx->drawString("KM/H", 136, 92);
        _gfx->fillRect(18, 122, 208, 24, 0x0841); _gfx->setTextColor(0x52AA, 0x0841); _gfx->setTextSize(1);
        char sub[32]; snprintf(sub, sizeof(sub), "Pulsa: %.1f Hz | %.0f P/KM", st.hzKmh, cfg.pulsePerKm); _gfx->drawString(sub, 20, 128);
        int32_t barW = constrain((curKmh * 204) / 300, 0, 204);
        _gfx->fillRect(20, 158, 204, 4, 0x10A2); if (barW > 0) _gfx->fillRect(20, 158, barW, 4, 0x07FF);
        _lastKmh = curKmh; _lastEnKmh = cfg.speedoEnableKmh; _lastHzKmh = st.hzKmh;
    }

    if (fullRedraw || curRpm != _lastRpm || cfg.speedoEnableRpm != _lastEnRpm || st.hzRpm != _lastHzRpm) {
        _gfx->fillRoundRect(398, 52, 66, 18, 3, cfg.speedoEnableRpm ? 0x03E0 : 0x7800);
        _gfx->setTextColor(cfg.speedoEnableRpm ? 0x07E0 : 0xF800, cfg.speedoEnableRpm ? 0x03E0 : 0x7800);
        _gfx->setTextSize(1); _gfx->drawCenterString(cfg.speedoEnableRpm ? "CH2: ON" : "CH2: OFF", 431, 57);
        _gfx->fillRect(254, 72, 208, 42, 0x0841);
        char rBuf[8]; snprintf(rBuf, sizeof(rBuf), "%5d", curRpm);
        _gfx->setTextColor(cfg.speedoEnableRpm ? 0x07E0 : 0x7BEF, 0x0841); _gfx->setTextSize(4); _gfx->drawString(rBuf, 258, 76);
        _gfx->setTextSize(2); _gfx->drawString("RPM", 398, 92);
        _gfx->fillRect(254, 122, 208, 24, 0x0841); _gfx->setTextColor(0x52AA, 0x0841); _gfx->setTextSize(1);
        char sub[32]; snprintf(sub, sizeof(sub), "Pulsa: %.1f Hz | %.1f PPR", st.hzRpm, cfg.speedoTachoPpr); _gfx->drawString(sub, 256, 128);
        int32_t barW = constrain((curRpm * 204) / cfg.speedoMaxRpm, 0, 204);
        _gfx->fillRect(256, 158, 204, 4, 0x10A2); if (barW > 0) _gfx->fillRect(256, 158, barW, 4, 0x07E0);
        _lastRpm = curRpm; _lastEnRpm = cfg.speedoEnableRpm; _lastHzRpm = st.hzRpm;
    }

    if (fullRedraw || curTemp != _lastTemp || cfg.speedoEnableTemp != _lastEnTemp || st.voltTemp != _lastVoltTemp) {
        _gfx->fillRoundRect(162, 184, 66, 16, 3, cfg.speedoEnableTemp ? 0x03E0 : 0x7800);
        _gfx->setTextColor(cfg.speedoEnableTemp ? 0x07E0 : 0xF800, cfg.speedoEnableTemp ? 0x03E0 : 0x7800);
        _gfx->setTextSize(1); _gfx->drawCenterString(cfg.speedoEnableTemp ? "CH3: ON" : "CH3: OFF", 195, 188);
        _gfx->fillRect(18, 204, 208, 28, 0x0841);
        char tBuf[32]; snprintf(tBuf, sizeof(tBuf), "%3d %% (%.2fV)", curTemp, st.voltTemp);
        _gfx->setTextColor(cfg.speedoEnableTemp ? 0xFD20 : 0x7BEF, 0x0841); _gfx->setTextSize(2); _gfx->drawString(tBuf, 18, 208);
        int32_t barW = constrain((curTemp * 204) / 100, 0, 204);
        _gfx->fillRect(20, 240, 204, 6, 0x10A2); if (barW > 0) _gfx->fillRect(20, 240, barW, 6, (curTemp > 85) ? 0xF800 : ((curTemp < 30) ? 0x07FF : 0x07E0));
        _lastTemp = curTemp; _lastEnTemp = cfg.speedoEnableTemp; _lastVoltTemp = st.voltTemp;
    }

    if (fullRedraw || curFuel != _lastFuel || cfg.speedoEnableFuel != _lastEnFuel || st.voltFuel != _lastVoltFuel) {
        _gfx->fillRoundRect(398, 184, 66, 16, 3, cfg.speedoEnableFuel ? 0x03E0 : 0x7800);
        _gfx->setTextColor(cfg.speedoEnableFuel ? 0x07E0 : 0xF800, cfg.speedoEnableFuel ? 0x03E0 : 0x7800);
        _gfx->setTextSize(1); _gfx->drawCenterString(cfg.speedoEnableFuel ? "CH4: ON" : "CH4: OFF", 431, 188);
        _gfx->fillRect(254, 204, 208, 28, 0x0841);
        char fBuf[32]; snprintf(fBuf, sizeof(fBuf), "%3d %% (%.2fV)", curFuel, st.voltFuel);
        _gfx->setTextColor(cfg.speedoEnableFuel ? 0xFFE0 : 0x7BEF, 0x0841); _gfx->setTextSize(2); _gfx->drawString(fBuf, 254, 208);
        int32_t barW = constrain((curFuel * 204) / 100, 0, 204);
        _gfx->fillRect(256, 240, 204, 6, 0x10A2); if (barW > 0) _gfx->fillRect(256, 240, barW, 6, (curFuel < 20) ? 0xF800 : 0xFFE0);
        _lastFuel = curFuel; _lastEnFuel = cfg.speedoEnableFuel; _lastVoltFuel = st.voltFuel;
    }

    if (fullRedraw || st.isRunning != _lastRunning || curMode != _lastMode) {
        _gfx->fillRect(16, 270, 448, 36, 0x0841);
        if (st.isRunning) {
            _gfx->fillRoundRect(20, 272, 110, 32, 4, 0x03E0); _gfx->drawRoundRect(20, 272, 110, 32, 4, 0x07E0);
            _gfx->setTextColor(0x07E0, 0x03E0); _gfx->setTextSize(2); _gfx->drawCenterString("RUNNING", 75, 280);
        } else {
            _gfx->fillRoundRect(20, 272, 110, 32, 4, 0xF800); _gfx->drawRoundRect(20, 272, 110, 32, 4, 0xF800);
            _gfx->setTextColor(TFT_WHITE, 0xF800); _gfx->setTextSize(2); _gfx->drawCenterString("STOPPED", 75, 280);
        }

        _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(2);
        if (cfg.runMode == EcuEngine::SpeedoRunMode::AutoSweep) {
            char swBuf[32]; snprintf(swBuf, sizeof(swBuf), "MODE: SWEEP (%.0fs)", cfg.sweepTimeSec); _gfx->drawString(swBuf, 145, 274);
        } else if (cfg.runMode == EcuEngine::SpeedoRunMode::StepCalib) {
            char stBuf[32]; snprintf(stBuf, sizeof(stBuf), "MODE: 4-PT STEP (%d%%)", st.stepIndex * 25); _gfx->drawString(stBuf, 145, 274);
        } else if (cfg.runMode == EcuEngine::SpeedoRunMode::OdometerRun) {
            char odoBuf[32]; snprintf(odoBuf, sizeof(odoBuf), "MODE: ODO (%.2f km)", st.totalDistanceKm); _gfx->drawString(odoBuf, 145, 274);
        } else {
            _gfx->drawString("MODE: MANUAL (FIX)", 145, 274);
        }

        _gfx->setTextColor(0x52AA, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("Putar di Baris Mode: Ganti Mode | Klik: RUN / STOP", 145, 294);
        _lastRunning = st.isRunning; _lastMode = curMode;
    }
}

void PageSpeedoTester::_renderTabCalibration(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig(); static constexpr uint16_t RY[] = {80, 134, 188};
    if (fullRedraw) {
        _gfx->fillRoundRect(8, 46, 228, 206, 6, 0x0841); _gfx->setTextColor(0xFD20, 0x0841); _gfx->setTextSize(1); _gfx->drawString("SUHU ECT (3-TITIK)", 18, 56);
        _gfx->fillRoundRect(244, 46, 228, 206, 6, 0x0841); _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(1); _gfx->drawString("BENSIN FUEL (3-TITIK)", 254, 56);
        _gfx->fillRoundRect(8, 262, 464, 52, 6, 0x0841); _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("Joy-X/Y: Baris | Putar: +/-1% Offset Kalibrasi | Klik: Reset 0/50/100", 20, 280);
        const char* L_LBL[] = { "MIN (COLD  0%)", "MID (NORM 50%)", "MAX (HOT 100%)" };
        const char* R_LBL[] = { "MIN (EMPTY 0%)", "MID (HALF 50%)", "MAX (FULL100%)" };
        for (uint8_t i = 0; i < 3; ++i) {
            _gfx->fillRoundRect(16, RY[i], 212, 46, 4, 0x10A2); _gfx->setTextColor(TFT_WHITE, 0x10A2); _gfx->setTextSize(1); _gfx->drawString(L_LBL[i], 24, RY[i] + 8);
            _gfx->fillRoundRect(252, RY[i], 212, 46, 4, 0x10A2); _gfx->setTextColor(TFT_WHITE, 0x10A2); _gfx->setTextSize(1); _gfx->drawString(R_LBL[i], 260, RY[i] + 8);
        }
    }
    if (fullRedraw || editRow != _lastEditRow) {
        for (uint8_t i = 0; i < 3; ++i) { _drawPanelFrame(16, RY[i], 212, 46, editRow == i); _drawPanelFrame(252, RY[i], 212, 46, editRow == (i + 3)); }
    }
    int32_t curVals[] = { cfg.tempCalMin, cfg.tempCalMid, cfg.tempCalMax, cfg.fuelCalMin, cfg.fuelCalMid, cfg.fuelCalMax };
    uint32_t colors[] = { 0x07FF, 0x07E0, 0xF800, 0xF800, 0xFFE0, 0x07E0 };
    for (uint8_t i = 0; i < 6; ++i) {
        if (fullRedraw || curVals[i] != _lastCalVals[i]) {
            int32_t bx = (i < 3) ? 16 : 252, by = RY[i % 3];
            _gfx->fillRect(bx + 110, by + 18, 96, 24, 0x10A2); _gfx->setTextColor(colors[i], 0x10A2); _gfx->setTextSize(2);
            char cBuf[16]; snprintf(cBuf, sizeof(cBuf), "%3d %%", curVals[i]); _gfx->drawString(cBuf, bx + 120, by + 22);
            _lastCalVals[i] = curVals[i];
        }
    }
}

void PageSpeedoTester::_renderTabHardware(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig(); const auto& st = controller.getState();
    if (fullRedraw) {
        _gfx->fillRoundRect(8, 46, 228, 98, 6, 0x0841); _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1); _gfx->drawString("PULSES PER KM (SPEED)", 18, 54);
        _gfx->fillRoundRect(244, 46, 228, 98, 6, 0x0841); _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(1); _gfx->drawString("PULSES PER REV (TACHO)", 254, 54);
        _gfx->fillRoundRect(8, 154, 228, 98, 6, 0x0841); _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(1); _gfx->drawString("JALUR HARDWARE FUEL & TEMP", 18, 162);
        _gfx->fillRoundRect(244, 154, 228, 98, 6, 0x0841); _gfx->setTextColor(0xFD20, 0x0841); _gfx->setTextSize(1); _gfx->drawString("KURVA JARUM & DURASI SWEEP", 254, 162);
        _gfx->fillRoundRect(8, 262, 464, 52, 6, 0x0841); _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("Joy-X/Y: Pilih Card | Putar: Ganti Preset / Nilai | Klik: Toggle", 20, 280);
    }
    if (fullRedraw || editRow != _lastEditRow) {
        _drawPanelFrame(8, 46, 228, 98, editRow == 0); _drawPanelFrame(244, 46, 228, 98, editRow == 1);
        _drawPanelFrame(8, 154, 228, 98, editRow == 2); _drawPanelFrame(244, 154, 228, 98, editRow == 3);
    }

    if (fullRedraw || cfg.pulsePerKm != _lastPpk) {
        _gfx->fillRect(18, 72, 208, 64, 0x0841); _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(3);
        char buf[32]; snprintf(buf, sizeof(buf), "%.0f", cfg.pulsePerKm); _gfx->drawString(buf, 24, 76);
        _gfx->setTextSize(1); _gfx->setTextColor(0x52AA, 0x0841); _gfx->drawString("PULSES / KM (JIS/Euro/Univ)", 24, 114); _lastPpk = cfg.pulsePerKm;
    }
    if (fullRedraw || cfg.speedoTachoPpr != _lastPpr) {
        _gfx->fillRect(254, 72, 208, 64, 0x0841); _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(3);
        char buf[32]; snprintf(buf, sizeof(buf), "%.1f PPR", cfg.speedoTachoPpr); _gfx->drawString(buf, 260, 76);
        _gfx->setTextSize(1); _gfx->setTextColor(0x52AA, 0x0841); _gfx->drawString("1-Cyl / 4-Cyl / 6-Cyl / 8-Cyl", 260, 114); _lastPpr = cfg.speedoTachoPpr;
    }
    uint8_t curR = static_cast<uint8_t>(cfg.dacRouting);
    if (fullRedraw || curR != _lastRouting || st.dacFuelFound != _lastDac1 || st.dacTempFound != _lastDac2) {
        _gfx->fillRect(18, 178, 208, 66, 0x0841); _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(curR < 4 ? ROUTE_NAMES[curR] : "Dual DAC", 24, 182); _gfx->setTextSize(1);
        _gfx->setTextColor(st.dacFuelFound ? 0x07E0 : 0xF800, 0x0841); _gfx->drawString(st.dacFuelFound ? "DAC1(0x60 FUEL): ON" : "DAC1(0x60): OFFLINE", 24, 212);
        _gfx->setTextColor(st.dacTempFound ? 0x07E0 : 0xF800, 0x0841); _gfx->drawString(st.dacTempFound ? "DAC2(0x61 TEMP): ON" : "DAC2(0x61): OFFLINE", 24, 228);
        _lastRouting = curR; _lastDac1 = st.dacFuelFound; _lastDac2 = st.dacTempFound;
    }
    uint8_t curC = static_cast<uint8_t>(cfg.gaugeCurve);
    if (fullRedraw || curC != _lastCurve || cfg.sweepTimeSec != _lastSweepTime) {
        _gfx->fillRect(254, 178, 208, 66, 0x0841); _gfx->setTextColor(0xFD20, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(curC == 0 ? "Non-Linier (Sqrt)" : "Linier 1:1", 260, 182); _gfx->setTextSize(1); _gfx->setTextColor(0xD69F, 0x0841);
        char sBuf[32]; snprintf(sBuf, sizeof(sBuf), "Durasi Sweep: %.0f Detik", cfg.sweepTimeSec); _gfx->drawString(sBuf, 260, 218);
        _lastCurve = curC; _lastSweepTime = cfg.sweepTimeSec;
    }
}

void PageSpeedoTester::onEncoderTurn(uint8_t currentTab, int32_t delta, uint8_t editRow, EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    if (currentTab == 1) {
        switch (editRow) {
            case 0: controller.setKmh(cfg.speedoKmh + (delta * 5)); break;
            case 1: controller.setRpm(cfg.speedoRpm + (delta * 100)); break;
            case 2: controller.setTemp(cfg.speedoTempPercent + (delta * 1)); break;
            case 3: controller.setFuel(cfg.speedoFuelPercent + (delta * 1)); break;
            case 4: {
                int32_t m = (static_cast<int32_t>(cfg.runMode) + (delta > 0 ? 1 : -1) + 4) % 4;
                controller.setRunMode(static_cast<EcuEngine::SpeedoRunMode>(m));
                break;
            }
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
        } else if (editRow == 3) controller.setSweepTimeSec(cfg.sweepTimeSec + delta);
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
