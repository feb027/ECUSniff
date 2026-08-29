#include "page_speedo_tester.h"

namespace EcuUi {

static constexpr uint16_t ROW_Y[] = {48, 96, 144, 192, 240};
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
    _gfx->fillRect(0, 42, 480, 278, TFT_BLACK);

    _gfx->drawFastHLine(0, 290, 480, 0x03E0);
    _gfx->setTextColor(0x07FF, TFT_BLACK);
    _gfx->setTextSize(1);
    _gfx->drawString("Knob/Joy-Y: Baris | Klik: Edit | Joy-Left / Tab [< MENU]: Keluar", 20, 302);

    static const char* LABELS[] = {
        "SPEED (KM/H):",
        "TACHO (RPM) :",
        "SUHU (ECT)  :",
        "BENSIN(FUEL):",
        "AUTO SWEEP  :"
    };

    for (uint8_t i = 0; i < TOTAL_ROWS; ++i) {
        _gfx->fillRoundRect(12, ROW_Y[i], 456, 42, 4, 0x10A2);
        _gfx->setTextColor(TFT_WHITE, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->drawString(LABELS[i], 24, ROW_Y[i] + 14);
    }
}

void PageSpeedoTester::_drawRowHighlight(uint8_t row, bool isSelected, bool isEditing) {
    if (row >= TOTAL_ROWS) return;
    int32_t y = ROW_Y[row];
    uint32_t borderColor = isEditing ? 0xF800 : (isSelected ? 0xFFE0 : 0x52AA);

    _gfx->drawRoundRect(12, y, 456, 42, 4, borderColor);
    if (isSelected || isEditing) {
        _gfx->drawRoundRect(11, y - 1, 458, 44, 5, borderColor);
    }
}

void PageSpeedoTester::_renderValues(const EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    const auto& state = controller.getState();

    // Row 0: Speed KM/H
    if (state.currentKmh != _lastKmh || state.hzKmh != _lastHzKmh) {
        _gfx->fillRect(140, ROW_Y[0] + 4, 320, 34, 0x10A2);
        _gfx->setTextColor(0x07FF, 0x10A2);
        _gfx->setTextSize(2);
        char buf[48];
        snprintf(buf, sizeof(buf), "%.0f km/h (%.1f Hz)", state.currentKmh, state.hzKmh);
        _gfx->drawString(buf, 145, ROW_Y[0] + 10);
        _lastKmh = state.currentKmh;
        _lastHzKmh = state.hzKmh;
    }

    // Row 1: RPM
    if (state.currentRpm != _lastRpm || state.hzRpm != _lastHzRpm) {
        _gfx->fillRect(140, ROW_Y[1] + 4, 320, 34, 0x10A2);
        _gfx->setTextColor(0x07E0, 0x10A2);
        _gfx->setTextSize(2);
        char buf[48];
        snprintf(buf, sizeof(buf), "%.0f RPM (%.1f Hz)", state.currentRpm, state.hzRpm);
        _gfx->drawString(buf, 145, ROW_Y[1] + 10);
        _lastRpm = state.currentRpm;
        _lastHzRpm = state.hzRpm;
    }

    // Row 2: Temp
    if (state.currentTemp != _lastTemp) {
        _gfx->fillRect(140, ROW_Y[2] + 4, 320, 34, 0x10A2);
        _gfx->setTextColor(0xFD20, 0x10A2);
        _gfx->setTextSize(2);
        char buf[48];
        snprintf(buf, sizeof(buf), "%.0f %%  (%.2f V)", state.currentTemp, state.voltTemp);
        _gfx->drawString(buf, 145, ROW_Y[2] + 10);
        _lastTemp = state.currentTemp;
    }

    // Row 3: Fuel
    if (state.currentFuel != _lastFuel) {
        _gfx->fillRect(140, ROW_Y[3] + 4, 320, 34, 0x10A2);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        char buf[48];
        snprintf(buf, sizeof(buf), "%.0f %%  (%.2f V)", state.currentFuel, state.voltFuel);
        _gfx->drawString(buf, 145, ROW_Y[3] + 10);
        _lastFuel = state.currentFuel;
    }

    // Row 4: Auto Sweep & Running
    if (cfg.autoSweep != _lastSweep || state.isRunning != _lastRunning) {
        _gfx->fillRect(140, ROW_Y[4] + 4, 320, 34, 0x10A2);
        _gfx->setTextSize(2);
        if (state.isRunning) {
            _gfx->setTextColor(0x07E0, 0x10A2);
            _gfx->drawString(cfg.autoSweep ? "SWEEP AKTIF (RUN)" : "MANUAL (RUN)", 145, ROW_Y[4] + 10);
        } else {
            _gfx->setTextColor(0xF800, 0x10A2);
            _gfx->drawString(cfg.autoSweep ? "SWEEP (STOPPED)" : "OFF (STOPPED)", 145, ROW_Y[4] + 10);
        }
        _lastSweep = cfg.autoSweep;
        _lastRunning = state.isRunning;
    }
}

void PageSpeedoTester::onEncoderTurn(int32_t delta, uint8_t editRow, EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    switch (editRow) {
        case 0: controller.setKmh(cfg.speedoKmh + (delta * 10)); break;
        case 1: controller.setRpm(cfg.speedoRpm + (delta * 500)); break;
        case 2: controller.setTemp(cfg.speedoTempPercent + (delta * 5)); break;
        case 3: controller.setFuel(cfg.speedoFuelPercent + (delta * 5)); break;
        case 4: controller.setAutoSweep(!cfg.autoSweep); break;
        default: break;
    }
}

void PageSpeedoTester::onJoystickAction(EcuHal::JoyAction action, EcuEngine::SpeedoController& controller) {
    const auto& cfg = controller.getConfig();
    if (action == EcuHal::JoyAction::Left) {
        controller.setKmh(cfg.speedoKmh - 10);
    } else if (action == EcuHal::JoyAction::Right) {
        controller.setKmh(cfg.speedoKmh + 10);
    }
}

} // namespace EcuUi
