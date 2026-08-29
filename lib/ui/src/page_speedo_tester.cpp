#include "page_speedo_tester.h"

namespace EcuUi {

static const float PPK_PRESETS[] = { 2548.0f, 4000.0f, 8000.0f, 23333.0f, 30000.0f };
static const char* PPK_NAMES[]   = { "2548 (JIS)", "4000 (Univ)", "8000 (Euro)", "23333 (Modern)", "30000 (ABS)" };
static const float PPR_PRESETS[] = { 1.0f, 2.0f, 3.0f, 4.0f, 0.5f };
static const char* PPR_NAMES[]   = { "1.0 (1-Cyl/ECU)", "2.0 (4-Cyl)", "3.0 (6-Cyl)", "4.0 (8-Cyl)", "0.5 (Wasted)" };
static const char* ROUTE_NAMES[] = { "Standar Dual PWM", "Single DAC Fuel + PWM Temp", "Single DAC Temp + PWM Fuel", "Dual MCP4725 DAC" };

PageSpeedoTester::PageSpeedoTester(LovyanGFX* gfx) : _gfx(gfx) {}

void PageSpeedoTester::init() {
    _lastTab = 0xFF;
    _lastEditRow = 0xFF;
}

void PageSpeedoTester::_drawRowFrame(int32_t x, int32_t y, int32_t w, int32_t h, bool isSelected) {
    _gfx->drawRoundRect(x - 1, y - 1, w + 2, h + 2, 5, isSelected ? 0xFFE0 : TFT_BLACK);
    _gfx->drawRoundRect(x, y, w, h, 4, isSelected ? 0xFFE0 : 0x52AA);
}

void PageSpeedoTester::render(uint8_t currentTab, bool fullRedraw, uint8_t editRow,
                             const EcuEngine::SpeedoController& controller) {
    bool tabChanged = (currentTab != _lastTab);
    if (tabChanged || fullRedraw) {
        _gfx->fillRect(0, 42, 480, 278, TFT_BLACK);
        _lastTab = currentTab;
        fullRedraw = true;
    }

    if (currentTab == 1) _renderTabCockpit(fullRedraw, editRow, controller);
    else if (currentTab == 2) _renderTabCalibration(fullRedraw, editRow, controller);
    else if (currentTab == 3) _renderTabHardware(fullRedraw, editRow, controller);

    _lastEditRow = editRow;
}

void PageSpeedoTester::_renderTabCockpit(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    const auto& st = controller.getState();

    static constexpr uint16_t RY[] = {48, 96, 144, 192, 240};

    if (fullRedraw) {
        _gfx->drawFastHLine(0, 290, 480, 0x03E0);
        _gfx->setTextColor(0x07FF, TFT_BLACK);
        _gfx->setTextSize(1);
        _gfx->drawString("Joy-Y: Baris | Putar: Langsung Atur | Klik: Toggle CH / Run | Joy-Left: Menu", 10, 302);

        const char* LABELS[] = { "CH 1 SPEED (KM/H) :", "CH 2 TACHO (RPM)  :", "CH 3 SUHU ECT (%) :", "CH 4 FUEL LEVEL(%):", "MASTER RUN & SWEEP:" };
        for (uint8_t i = 0; i < 5; ++i) {
            _gfx->fillRoundRect(12, RY[i], 456, 42, 4, 0x10A2);
            _gfx->setTextColor(TFT_WHITE, 0x10A2);
            _gfx->setTextSize(1);
            _gfx->drawString(LABELS[i], 22, RY[i] + 15);
        }
    }

    for (uint8_t i = 0; i < 5; ++i) _drawRowFrame(12, RY[i], 456, 42, i == editRow);

    // Row 0: Speed
    _gfx->fillRect(168, RY[0] + 4, 294, 34, 0x10A2);
    _gfx->setTextSize(2);
    _gfx->setTextColor(cfg.speedoEnableKmh ? 0x07FF : 0x7BEF, 0x10A2);
    char buf[48];
    snprintf(buf, sizeof(buf), "%3d km/h [%.1fHz] %s", (int)st.currentKmh, st.hzKmh, cfg.speedoEnableKmh ? "ON" : "OFF");
    _gfx->drawString(buf, 172, RY[0] + 10);

    // Row 1: RPM
    _gfx->fillRect(168, RY[1] + 4, 294, 34, 0x10A2);
    _gfx->setTextColor(cfg.speedoEnableRpm ? 0x07E0 : 0x7BEF, 0x10A2);
    snprintf(buf, sizeof(buf), "%5d RPM [%.1fHz] %s", (int)st.currentRpm, st.hzRpm, cfg.speedoEnableRpm ? "ON" : "OFF");
    _gfx->drawString(buf, 172, RY[1] + 10);

    // Row 2: Suhu
    _gfx->fillRect(168, RY[2] + 4, 294, 34, 0x10A2);
    _gfx->setTextColor(cfg.speedoEnableTemp ? 0xFD20 : 0x7BEF, 0x10A2);
    const char* tempModeStr = (cfg.dacRouting == EcuEngine::SpeedoDacRouting::DualMcp4725 || cfg.dacRouting == EcuEngine::SpeedoDacRouting::SingleDacTemp)
                              ? (st.dacTempFound ? "DAC 0x61" : "DAC OFF") : "PWM P45";
    snprintf(buf, sizeof(buf), "%3d%% (%.2fV) [%s] %s", (int)st.currentTemp, st.voltTemp, tempModeStr, cfg.speedoEnableTemp ? "ON" : "OFF");
    _gfx->drawString(buf, 172, RY[2] + 10);

    // Row 3: Fuel
    _gfx->fillRect(168, RY[3] + 4, 294, 34, 0x10A2);
    _gfx->setTextColor(cfg.speedoEnableFuel ? 0xFFE0 : 0x7BEF, 0x10A2);
    const char* fuelModeStr = (cfg.dacRouting == EcuEngine::SpeedoDacRouting::DualMcp4725 || cfg.dacRouting == EcuEngine::SpeedoDacRouting::SingleDacFuel)
                              ? (st.dacFuelFound ? "DAC 0x60" : "DAC OFF") : "PWM P46";
    snprintf(buf, sizeof(buf), "%3d%% (%.2fV) [%s] %s", (int)st.currentFuel, st.voltFuel, fuelModeStr, cfg.speedoEnableFuel ? "ON" : "OFF");
    _gfx->drawString(buf, 172, RY[3] + 10);

    // Row 4: Master Run & Sweep
    _gfx->fillRect(168, RY[4] + 4, 294, 34, 0x10A2);
    if (st.isRunning) {
        _gfx->setTextColor(0x07E0, 0x10A2);
        _gfx->drawString(cfg.autoSweep ? "RUNNING (SWEEP: ON)" : "RUNNING (MANUAL)", 172, RY[4] + 10);
    } else {
        _gfx->setTextColor(0xF800, 0x10A2);
        _gfx->drawString(cfg.autoSweep ? "STOPPED (SWEEP: ON)" : "STOPPED (STANDBY)", 172, RY[4] + 10);
    }
}

void PageSpeedoTester::_renderTabCalibration(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    static constexpr uint16_t RY[] = {48, 88, 128, 168, 208, 248};

    if (fullRedraw) {
        _gfx->drawFastHLine(0, 290, 480, 0x03E0);
        _gfx->setTextColor(0x07FF, TFT_BLACK);
        _gfx->setTextSize(1);
        _gfx->drawString("Joy-Y: Baris | Putar: +/-1% Kalibrasi | Klik: Reset Preset | Joy-L: Menu", 10, 302);

        const char* LABELS[] = { "ECT MIN (COLD 0%) :", "ECT MID (NORM 50%):", "ECT MAX (HOT 100%):", "FUEL MIN (EMPTY 0%):", "FUEL MID (HALF 50%):", "FUEL MAX (FULL100%):" };
        for (uint8_t i = 0; i < 6; ++i) {
            _gfx->fillRoundRect(12, RY[i], 456, 36, 4, 0x10A2);
            _gfx->setTextColor(TFT_WHITE, 0x10A2);
            _gfx->setTextSize(1);
            _gfx->drawString(LABELS[i], 22, RY[i] + 12);
        }
    }

    for (uint8_t i = 0; i < 6; ++i) _drawRowFrame(12, RY[i], 456, 36, i == editRow);

    int32_t vals[] = { cfg.tempCalMin, cfg.tempCalMid, cfg.tempCalMax, cfg.fuelCalMin, cfg.fuelCalMid, cfg.fuelCalMax };
    uint32_t colors[] = { 0xFD20, 0xFD20, 0xFD20, 0xFFE0, 0xFFE0, 0xFFE0 };

    for (uint8_t i = 0; i < 6; ++i) {
        _gfx->fillRect(190, RY[i] + 4, 270, 28, 0x10A2);
        _gfx->setTextColor(colors[i], 0x10A2);
        _gfx->setTextSize(2);
        char buf[32];
        snprintf(buf, sizeof(buf), "%3d %% Cal", vals[i]);
        _gfx->drawString(buf, 195, RY[i] + 8);
    }
}

void PageSpeedoTester::_renderTabHardware(bool fullRedraw, uint8_t editRow, const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    const auto& st = controller.getState();
    static constexpr uint16_t RY[] = {48, 96, 144, 192, 240};

    if (fullRedraw) {
        _gfx->drawFastHLine(0, 290, 480, 0x03E0);
        _gfx->setTextColor(0x07FF, TFT_BLACK);
        _gfx->setTextSize(1);
        _gfx->drawString("Joy-Y: Baris | Putar: Ganti Preset / Routing | Klik: Toggle", 16, 302);

        const char* LABELS[] = { "PPK PULSES/KM :", "TACHO PPR/CYL :", "HARDWARE ROUTE:", "GAUGE CURVE   :", "SWEEP TIME    :" };
        for (uint8_t i = 0; i < 5; ++i) {
            _gfx->fillRoundRect(12, RY[i], 456, 42, 4, 0x10A2);
            _gfx->setTextColor(TFT_WHITE, 0x10A2);
            _gfx->setTextSize(1);
            _gfx->drawString(LABELS[i], 22, RY[i] + 15);
        }
    }

    for (uint8_t i = 0; i < 5; ++i) _drawRowFrame(12, RY[i], 456, 42, i == editRow);

    // Row 0: PPK
    _gfx->fillRect(150, RY[0] + 4, 312, 34, 0x10A2);
    _gfx->setTextColor(0x07FF, 0x10A2);
    _gfx->setTextSize(2);
    char buf[48];
    snprintf(buf, sizeof(buf), "%.0f P/KM", cfg.pulsePerKm);
    _gfx->drawString(buf, 155, RY[0] + 10);

    // Row 1: PPR
    _gfx->fillRect(150, RY[1] + 4, 312, 34, 0x10A2);
    _gfx->setTextColor(0x07E0, 0x10A2);
    snprintf(buf, sizeof(buf), "%.1f PPR", cfg.speedoTachoPpr);
    _gfx->drawString(buf, 155, RY[1] + 10);

    // Row 2: Routing with DAC status badges
    _gfx->fillRect(150, RY[2] + 2, 312, 38, 0x10A2);
    _gfx->setTextColor(0xFFE0, 0x10A2);
    _gfx->setTextSize(1);
    uint8_t rIdx = static_cast<uint8_t>(cfg.dacRouting);
    _gfx->drawString(rIdx < 4 ? ROUTE_NAMES[rIdx] : "Dual DAC", 155, RY[2] + 6);

    _gfx->setTextColor(st.dacFuelFound ? 0x07E0 : 0xF800, 0x10A2);
    _gfx->drawString(st.dacFuelFound ? "DAC1(0x60 FUEL): ON" : "DAC1(0x60): OFFLINE", 155, RY[2] + 22);
    _gfx->setTextColor(st.dacTempFound ? 0x07E0 : 0xF800, 0x10A2);
    _gfx->drawString(st.dacTempFound ? "DAC2(0x61 TEMP): ON" : "DAC2(0x61): OFFLINE", 305, RY[2] + 22);

    // Row 3: Curve
    _gfx->fillRect(150, RY[3] + 4, 312, 34, 0x10A2);
    _gfx->setTextColor(0xFD20, 0x10A2);
    _gfx->setTextSize(2);
    _gfx->drawString(cfg.gaugeCurve == EcuEngine::SpeedoGaugeCurve::SqrtThermal ? "Non-Linier (Sqrt)" : "Linier 1:1", 155, RY[3] + 10);

    // Row 4: Sweep Time
    _gfx->fillRect(150, RY[4] + 4, 312, 34, 0x10A2);
    _gfx->setTextColor(0xD69F, 0x10A2);
    snprintf(buf, sizeof(buf), "%.0f Detik", cfg.sweepTimeSec);
    _gfx->drawString(buf, 155, RY[4] + 10);
}

void PageSpeedoTester::onEncoderTurn(uint8_t currentTab, int32_t delta, uint8_t editRow, EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();

    if (currentTab == 1) { // COCKPIT TAB
        switch (editRow) {
            case 0: controller.setKmh(cfg.speedoKmh + (delta * 10)); break;
            case 1: controller.setRpm(cfg.speedoRpm + (delta * 500)); break;
            case 2: controller.setTemp(cfg.speedoTempPercent + (delta * 5)); break;
            case 3: controller.setFuel(cfg.speedoFuelPercent + (delta * 5)); break;
            case 4: controller.setAutoSweep(!cfg.autoSweep); break;
            default: break;
        }
    } else if (currentTab == 2) { // 3-POINT CALIBRATION TAB
        switch (editRow) {
            case 0: controller.setTempCal(cfg.tempCalMin + delta, cfg.tempCalMid, cfg.tempCalMax); break;
            case 1: controller.setTempCal(cfg.tempCalMin, cfg.tempCalMid + delta, cfg.tempCalMax); break;
            case 2: controller.setTempCal(cfg.tempCalMin, cfg.tempCalMid, cfg.tempCalMax + delta); break;
            case 3: controller.setFuelCal(cfg.fuelCalMin + delta, cfg.fuelCalMid, cfg.fuelCalMax); break;
            case 4: controller.setFuelCal(cfg.fuelCalMin, cfg.fuelCalMid + delta, cfg.fuelCalMax); break;
            case 5: controller.setFuelCal(cfg.fuelCalMin, cfg.fuelCalMid, cfg.fuelCalMax + delta); break;
            default: break;
        }
    } else if (currentTab == 3) { // HARDWARE & ROUTING TAB
        switch (editRow) {
            case 0: {
                int32_t idx = 1;
                for (int i = 0; i < 5; ++i) if (abs(cfg.pulsePerKm - PPK_PRESETS[i]) < 10) idx = i;
                idx = (idx + (delta > 0 ? 1 : -1) + 5) % 5;
                controller.setPulsePerKm(PPK_PRESETS[idx]);
                break;
            }
            case 1: {
                int32_t idx = 1;
                for (int i = 0; i < 5; ++i) if (abs(cfg.speedoTachoPpr - PPR_PRESETS[i]) < 0.1f) idx = i;
                idx = (idx + (delta > 0 ? 1 : -1) + 5) % 5;
                controller.setTachoPpr(PPR_PRESETS[idx]);
                break;
            }
            case 2: {
                int32_t r = (static_cast<int32_t>(cfg.dacRouting) + (delta > 0 ? 1 : -1) + 4) % 4;
                controller.setDacRouting(static_cast<EcuEngine::SpeedoDacRouting>(r));
                break;
            }
            case 3: {
                auto c = (cfg.gaugeCurve == EcuEngine::SpeedoGaugeCurve::SqrtThermal) ? EcuEngine::SpeedoGaugeCurve::Linear : EcuEngine::SpeedoGaugeCurve::SqrtThermal;
                controller.setGaugeCurve(c);
                break;
            }
            case 4: controller.setSweepTimeSec(cfg.sweepTimeSec + delta); break;
            default: break;
        }
    }
}

void PageSpeedoTester::onEncoderClick(uint8_t currentTab, uint8_t editRow, EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    if (currentTab == 1) {
        switch (editRow) {
            case 0: controller.setChannelEnable(0, !cfg.speedoEnableKmh); break;
            case 1: controller.setChannelEnable(1, !cfg.speedoEnableRpm); break;
            case 2: controller.setChannelEnable(2, !cfg.speedoEnableTemp); break;
            case 3: controller.setChannelEnable(3, !cfg.speedoEnableFuel); break;
            case 4: controller.toggleRunning(); break;
            default: break;
        }
    } else if (currentTab == 2) {
        if (editRow <= 2) controller.setTempCal(0, 50, 100);
        else controller.setFuelCal(0, 50, 100);
    } else if (currentTab == 3) {
        if (editRow == 3) {
            auto c = (cfg.gaugeCurve == EcuEngine::SpeedoGaugeCurve::SqrtThermal) ? EcuEngine::SpeedoGaugeCurve::Linear : EcuEngine::SpeedoGaugeCurve::SqrtThermal;
            controller.setGaugeCurve(c);
        }
    }
}

} // namespace EcuUi
