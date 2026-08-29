#include "page_eps_tester.h"

namespace EcuUi {

static constexpr uint16_t ROW_Y[] = {48, 96, 144, 192, 240};
static constexpr uint8_t  TOTAL_ROWS = 5;

static const char* PRESET_NAMES[] = {
    "Toyota / Daihatsu",
    "Suzuki Karimun",
    "Suzuki Ertiga",
    "Honda Jazz / Brio",
    "Custom Tuning"
};

PageEpsTester::PageEpsTester(LovyanGFX* gfx) : _gfx(gfx) {}

void PageEpsTester::init() {
    _lastRunning = false;
    _lastPreset = 0xFF;
    _lastSpeed = -1.0f;
    _lastRpm = 0xFFFFFFFF;
    _lastTorque = -99.0f;
    _lastSweep = false;
    _lastEditMode = false;
    _lastEditRow = 0xFF;
    _lastVssFreq = -1.0f;
    _lastRpmFreq = -1.0f;
}

void PageEpsTester::render(bool fullRedraw, bool isEditMode, uint8_t editRow,
                          const EcuEngine::EpsController& controller) {
    if (fullRedraw) {
        _drawStaticLayout();
        _lastPreset = 0xFF;
        _lastSpeed = -1.0f;
        _lastRpm = 0xFFFFFFFF;
        _lastTorque = -99.0f;
        _lastVssFreq = -1.0f;
        _lastRpmFreq = -1.0f;
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

void PageEpsTester::_drawStaticLayout() {
    _gfx->fillRect(0, 42, 480, 278, TFT_BLACK);

    _gfx->drawFastHLine(0, 290, 480, 0x03E0);
    _gfx->setTextColor(0x07FF, TFT_BLACK);
    _gfx->setTextSize(1);
    _gfx->drawString("Joy-Y: Baris | Putar: Langsung Atur | Klik: Run/Stop | Joy-Left: Menu", 20, 302);

    static const char* LABELS[] = {
        "PRESET OEM :",
        "SPEED (VSS):",
        "ENGINE RPM :",
        "STEER TRQ  :",
        "AUTO SWEEP :"
    };

    for (uint8_t i = 0; i < TOTAL_ROWS; ++i) {
        _gfx->fillRoundRect(12, ROW_Y[i], 456, 42, 4, 0x10A2);
        _gfx->setTextColor(TFT_WHITE, 0x10A2);
        _gfx->setTextSize(1);
        _gfx->drawString(LABELS[i], 24, ROW_Y[i] + 14);
    }
}

void PageEpsTester::_drawRowHighlight(uint8_t row, bool isSelected, bool isEditing) {
    if (row >= TOTAL_ROWS) return;
    int32_t y = ROW_Y[row];
    uint32_t borderColor = isSelected ? 0xFFE0 : 0x52AA;

    _gfx->drawRoundRect(11, y - 1, 458, 44, 5, isSelected ? 0xFFE0 : TFT_BLACK);
    _gfx->drawRoundRect(12, y, 456, 42, 4, borderColor);
}

void PageEpsTester::_renderValues(const EcuEngine::EpsController& controller) {
    const auto& cfg = controller.getConfig();
    const auto& state = controller.getState();

    // Row 0: Preset
    uint8_t curPreset = static_cast<uint8_t>(cfg.preset);
    if (curPreset != _lastPreset) {
        _gfx->fillRect(140, ROW_Y[0] + 4, 320, 34, 0x10A2);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        const char* pName = (curPreset < 5) ? PRESET_NAMES[curPreset] : "Custom";
        _gfx->drawString(pName, 145, ROW_Y[0] + 10);
        _lastPreset = curPreset;
    }

    // Row 1: Speed
    if (state.currentSpeedKmh != _lastSpeed || state.vssFreqHz != _lastVssFreq) {
        _gfx->fillRect(140, ROW_Y[1] + 4, 320, 34, 0x10A2);
        _gfx->setTextColor(0x07FF, 0x10A2);
        _gfx->setTextSize(2);
        char buf[48];
        snprintf(buf, sizeof(buf), "%.0f km/h  (%.1f Hz)", state.currentSpeedKmh, state.vssFreqHz);
        _gfx->drawString(buf, 145, ROW_Y[1] + 10);
        _lastSpeed = state.currentSpeedKmh;
        _lastVssFreq = state.vssFreqHz;
    }

    // Row 2: RPM
    if (state.currentRpm != _lastRpm || state.rpmFreqHz != _lastRpmFreq) {
        _gfx->fillRect(140, ROW_Y[2] + 4, 320, 34, 0x10A2);
        _gfx->setTextColor(0x07E0, 0x10A2);
        _gfx->setTextSize(2);
        char buf[48];
        snprintf(buf, sizeof(buf), "%u RPM  (%.1f Hz)", (unsigned)state.currentRpm, state.rpmFreqHz);
        _gfx->drawString(buf, 145, ROW_Y[2] + 10);
        _lastRpm = state.currentRpm;
        _lastRpmFreq = state.rpmFreqHz;
    }

    // Row 3: Steer Torque
    if (cfg.steerTorque != _lastTorque) {
        _gfx->fillRect(140, ROW_Y[3] + 4, 320, 34, 0x10A2);
        _gfx->setTextColor(0xFD20, 0x10A2);
        _gfx->setTextSize(2);
        char buf[48];
        float pct = cfg.steerTorque * 100.0f;
        const char* dir = (pct > 5.0f) ? "KANAN" : ((pct < -5.0f) ? "KIRI" : "LURUS");
        snprintf(buf, sizeof(buf), "%+.0f%% [%s] (%.2fV)", pct, dir, state.trq1Voltage);
        _gfx->drawString(buf, 145, ROW_Y[3] + 10);
        _lastTorque = cfg.steerTorque;
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

void PageEpsTester::onEncoderTurn(int32_t delta, uint8_t editRow, EcuEngine::EpsController& controller) {
    const auto& cfg = controller.getConfig();
    switch (editRow) {
        case 0: {
            int32_t p = static_cast<int32_t>(cfg.preset) + (delta > 0 ? 1 : -1);
            if (p < 0) p = 4;
            if (p > 4) p = 0;
            controller.setPreset(static_cast<EcuEngine::EpsOemPreset>(p));
            break;
        }
        case 1: controller.setSpeed(cfg.speedKmh + (delta * 5.0f)); break;
        case 2: controller.setRpm(cfg.targetRpm + (delta * 100)); break;
        case 3: controller.setSteerTorque((cfg.steerTorque * 100.0f) + (delta * 5.0f)); break;
        case 4: controller.setAutoSweep(!cfg.autoSweep); break;
        default: break;
    }
}

void PageEpsTester::onJoystickAction(EcuHal::JoyAction action, EcuEngine::EpsController& controller) {
    const auto& cfg = controller.getConfig();
    if (action == EcuHal::JoyAction::Left) {
        controller.setSteerTorque((cfg.steerTorque * 100.0f) - 10.0f);
    } else if (action == EcuHal::JoyAction::Right) {
        controller.setSteerTorque((cfg.steerTorque * 100.0f) + 10.0f);
    }
}

} // namespace EcuUi
