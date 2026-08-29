#include "page_speedo_tester.h"

namespace EcuUi {

static constexpr uint16_t ROW_Y[] = {52, 100, 148, 196, 244};
static constexpr uint8_t  TOTAL_ROWS = 5;

PageSpeedoTester::PageSpeedoTester(LovyanGFX* gfx) : _gfx(gfx) {}

void PageSpeedoTester::init() {
    _lastRunning = false;
    _lastKmh = -1.0f;
    _lastRpm = -1.0f;
    _lastTemp = -1.0f;
    _lastFuel = -1.0f;
    _lastHzKmh = -1.0f;
    _lastHzRpm = -1.0f;
    _lastSweep = false;
    _lastEditMode = false;
    _lastEditRow = 0xFF;
    _lastDacFuel = false;
    _lastDacTemp = false;
}

void PageSpeedoTester::render(bool fullRedraw, bool isEditMode, uint8_t editRow,
                             const EcuEngine::SpeedoController& controller) {
    if (fullRedraw) {
        _drawStaticLayout();
        _lastKmh = -1.0f;
        _lastRpm = -1.0f;
        _lastTemp = -1.0f;
        _lastFuel = -1.0f;
        _lastHzKmh = -1.0f;
        _lastHzRpm = -1.0f;
        _lastEditRow = 0xFF;
    }

    if (fullRedraw || isEditMode != _lastEditMode || editRow != _lastEditRow) {
        for (uint8_t i = 0; i < TOTAL_ROWS; ++i) {
            _drawRowHighlight(i, i == editRow, isEditMode && (i == editRow));
        }
        _lastEditMode = isEditMode;
        _lastEditRow = editRow;
    }

    _renderValues(controller);
}

void PageSpeedoTester::_drawStaticLayout() {
    _gfx->fillScreen(TFT_BLACK);

    // Top Header Banner
    _gfx->fillRect(0, 0, 480, 42, 0x0841);
    _gfx->drawFastHLine(0, 42, 480, 0x03E0);
    _gfx->setTextColor(TFT_WHITE, 0x0841);
    _gfx->setTextSize(2);
    _gfx->drawString("SPEEDOMETER CLUSTER TESTER", 16, 12);

    // Bottom Navigation Bar
    _gfx->drawFastHLine(0, 290, 480, 0x03E0);
    _gfx->setTextColor(0x07FF, TFT_BLACK);
    _gfx->setTextSize(1);
    _gfx->drawString("Knob: Nilai | Klik: Edit/Tog | Joy-Y: Baris | Dbl-Klik: Menu Hub", 16, 302);

    static const char* LABELS[] = {
        "1. SPEED (KMH):",
        "2. TACHO (RPM):",
        "3. SUHU (ECT) :",
        "4. BENSIN/FUEL:",
        "5. AUTO SWEEP :"
    };

    for (uint8_t i = 0; i < TOTAL_ROWS; ++i) {
        _gfx->fillRoundRect(12, ROW_Y[i], 456, 42, 4, 0x10A2);
        _gfx->setTextColor(TFT_WHITE, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(LABELS[i], 24, ROW_Y[i] + 12);
    }
}

void PageSpeedoTester::_drawRowHighlight(uint8_t row, bool selected, bool isEditMode) {
    if (row >= TOTAL_ROWS) return;
    uint32_t borderColor = TFT_DARKGRAY;
    if (selected) {
        borderColor = isEditMode ? 0x07E0 : 0xFFE0;
    }
    _gfx->drawRoundRect(12, ROW_Y[row], 456, 42, 4, borderColor);
    if (selected) {
        _gfx->drawRoundRect(11, ROW_Y[row] - 1, 458, 44, 5, borderColor);
    }
}

void PageSpeedoTester::_renderValues(const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    const auto& st = controller.getState();

    // 1. Header Status
    if (st.isRunning != _lastRunning) {
        _gfx->fillRoundRect(350, 8, 120, 26, 4, st.isRunning ? 0x03E0 : 0x7800);
        _gfx->setTextColor(TFT_WHITE, st.isRunning ? 0x03E0 : 0x7800);
        _gfx->setTextSize(2);
        _gfx->drawString(st.isRunning ? "RUNNING" : "STOPPED", 362, 13);
        _lastRunning = st.isRunning;
    }

    // 2. Row 0: Speed (KM/H) & Freq
    if (st.currentKmh != _lastKmh || st.hzKmh != _lastHzKmh) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%3.0f km/h (%5.1f Hz)", st.currentKmh, st.hzKmh);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(buf, 204, ROW_Y[0] + 12);
        _lastKmh = st.currentKmh;
        _lastHzKmh = st.hzKmh;
    }

    // 3. Row 1: Tach (RPM) & Freq
    if (st.currentRpm != _lastRpm || st.hzRpm != _lastHzRpm) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%4.0f RPM (%5.1f Hz)", st.currentRpm, st.hzRpm);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(buf, 204, ROW_Y[1] + 12);
        _lastRpm = st.currentRpm;
        _lastHzRpm = st.hzRpm;
    }

    // 4. Row 2: Temp ECT
    if (st.currentTemp != _lastTemp) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%3.0f%% (PWM:%4.1f%%)", st.currentTemp, st.dutyTemp);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(buf, 204, ROW_Y[2] + 12);
        _lastTemp = st.currentTemp;
    }

    // 5. Row 3: Fuel Level
    if (st.currentFuel != _lastFuel) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%3.0f%% (PWM:%4.1f%%)", st.currentFuel, st.dutyFuel);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(buf, 204, ROW_Y[3] + 12);
        _lastFuel = st.currentFuel;
    }

    // 6. Row 4: Sweep Status
    if (cfg.autoSweep != _lastSweep) {
        _gfx->setTextColor(cfg.autoSweep ? 0x07E0 : 0xF800, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(cfg.autoSweep ? "AKTIF (All Channels) " : "OFF (Manual Dial)    ", 204, ROW_Y[4] + 12);
        _lastSweep = cfg.autoSweep;
    }
}

void PageSpeedoTester::onEncoderTurn(int32_t delta, uint8_t editRow,
                                    EcuEngine::SpeedoController& controller) {
    auto& cfg = controller.getConfig();
    switch (editRow) {
        case 0: controller.setKmh(cfg.speedoKmh + (delta * 5)); break;
        case 1: controller.setRpm(cfg.speedoRpm + (delta * 250)); break;
        case 2: controller.setTemp(cfg.speedoTempPercent + (delta * 5)); break;
        case 3: controller.setFuel(cfg.speedoFuelPercent + (delta * 5)); break;
        case 4: controller.setAutoSweep(!cfg.autoSweep); break;
        default: break;
    }
}

void PageSpeedoTester::onJoystickAction(EcuHal::JoyAction action,
                                       EcuEngine::SpeedoController& controller) {
    if (action == EcuHal::JoyAction::Left) {
        controller.setKmh(controller.getConfig().speedoKmh - 10);
    } else if (action == EcuHal::JoyAction::Right) {
        controller.setKmh(controller.getConfig().speedoKmh + 10);
    }
}

void PageSpeedoTester::onEncoderClick(EcuEngine::SpeedoController& controller) {
    controller.toggleRunning();
}

} // namespace EcuUi
