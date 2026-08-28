#include "page_eps_tester.h"

namespace EcuUi {

static constexpr uint16_t ROW_Y[] = {52, 100, 148, 196, 244};
static constexpr uint8_t  TOTAL_ROWS = 5;

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

    // Update Row Focus Boxes
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
    _gfx->fillScreen(TFT_BLACK);

    // Top Header Banner
    _gfx->fillRect(0, 0, 480, 42, 0x0841);
    _gfx->drawFastHLine(0, 42, 480, 0x03E0);
    _gfx->setTextColor(TFT_WHITE, 0x0841);
    _gfx->setTextSize(2);
    _gfx->drawString("EPS & VSS BENCH TESTER", 16, 12);

    // Bottom Action / Navigation Bar
    _gfx->drawFastHLine(0, 290, 480, 0x03E0);
    _gfx->setTextColor(0x07FF, TFT_BLACK);
    _gfx->setTextSize(1);
    _gfx->drawString("Knob/Joy-Y: Pilih Baris | Klik: Edit/Tog | Joy-X: Steer | Dbl-Klik: Menu", 16, 302);

    // Static Row Labels
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
        _gfx->setTextSize(2);
        _gfx->drawString(LABELS[i], 24, ROW_Y[i] + 12);
    }
}

void PageEpsTester::_drawRowHighlight(uint8_t row, bool selected, bool isEditMode) {
    if (row >= TOTAL_ROWS) return;
    uint32_t borderColor = TFT_DARKGRAY;
    if (selected) {
        borderColor = isEditMode ? 0x07E0 : 0xFFE0; // Green if editing, Yellow if focused
    }
    _gfx->drawRoundRect(12, ROW_Y[row], 456, 42, 4, borderColor);
    if (selected) {
        _gfx->drawRoundRect(11, ROW_Y[row] - 1, 458, 44, 5, borderColor);
    }
}

void PageEpsTester::_renderValues(const EcuEngine::EpsController& controller) {
    const auto& cfg = controller.getConfig();
    const auto& st = controller.getState();

    // 1. Header Status (RUNNING / STOPPED)
    if (st.isRunning != _lastRunning) {
        _gfx->fillRoundRect(350, 8, 120, 26, 4, st.isRunning ? 0x03E0 : 0x7800);
        _gfx->setTextColor(TFT_WHITE, st.isRunning ? 0x03E0 : 0x7800);
        _gfx->setTextSize(2);
        _gfx->drawString(st.isRunning ? "RUNNING" : "STOPPED", 362, 13);
        _lastRunning = st.isRunning;
    }

    // 2. Row 0: Preset Name
    uint8_t pIdx = static_cast<uint8_t>(cfg.preset);
    if (pIdx != _lastPreset) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%-18s", controller.getPresetName(cfg.preset));
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(buf, 200, ROW_Y[0] + 12);
        _lastPreset = pIdx;
    }

    // 3. Row 1: Speed & VSS Freq
    if (st.currentSpeedKmh != _lastSpeed || st.vssFreqHz != _lastVssFreq) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%3.0f km/h (%5.1f Hz)", st.currentSpeedKmh, st.vssFreqHz);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(buf, 200, ROW_Y[1] + 12);
        _lastSpeed = st.currentSpeedKmh;
        _lastVssFreq = st.vssFreqHz;
    }

    // 4. Row 2: RPM & Tach Freq
    if (st.currentRpm != _lastRpm || st.rpmFreqHz != _lastRpmFreq) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%4u RPM (%5.1f Hz)", st.currentRpm, st.rpmFreqHz);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(buf, 200, ROW_Y[2] + 12);
        _lastRpm = st.currentRpm;
        _lastRpmFreq = st.rpmFreqHz;
    }

    // 5. Row 3: Steer Torque
    if (cfg.steerTorque != _lastTorque) {
        char buf[32];
        if (cfg.steerTorque < -0.05f) {
            snprintf(buf, sizeof(buf), "KIRI  %2.0f%% (T1:%.2fV)", -cfg.steerTorque * 100.0f, st.trq1Voltage);
        } else if (cfg.steerTorque > 0.05f) {
            snprintf(buf, sizeof(buf), "KANAN %2.0f%% (T1:%.2fV)", cfg.steerTorque * 100.0f, st.trq1Voltage);
        } else {
            snprintf(buf, sizeof(buf), "LURUS  0%% (2.50V)    ");
        }
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(buf, 200, ROW_Y[3] + 12);
        _lastTorque = cfg.steerTorque;
    }

    // 6. Row 4: Auto Sweep
    if (cfg.autoSweep != _lastSweep) {
        _gfx->setTextColor(cfg.autoSweep ? 0x07E0 : 0xF800, 0x10A2);
        _gfx->setTextSize(2);
        _gfx->drawString(cfg.autoSweep ? "AKTIF (0-120 km/h) " : "OFF (Manual)       ", 200, ROW_Y[4] + 12);
        _lastSweep = cfg.autoSweep;
    }
}

void PageEpsTester::onEncoderTurn(int32_t delta, uint8_t editRow,
                                 EcuEngine::EpsController& controller) {
    auto& cfg = controller.getConfig();
    switch (editRow) {
        case 0: { // Preset
            int32_t next = static_cast<int32_t>(cfg.preset) + (delta > 0 ? 1 : -1);
            if (next < 0) next = static_cast<int32_t>(EcuEngine::EpsOemPreset::COUNT) - 1;
            if (next >= static_cast<int32_t>(EcuEngine::EpsOemPreset::COUNT)) next = 0;
            controller.setPreset(static_cast<EcuEngine::EpsOemPreset>(next));
            break;
        }
        case 1: { // Speed
            controller.setSpeed(cfg.speedKmh + (delta * 5.0f));
            break;
        }
        case 2: { // RPM
            int32_t rpm = static_cast<int32_t>(cfg.targetRpm) + (delta * 100);
            if (rpm < 0) rpm = 0;
            controller.setRpm(static_cast<uint32_t>(rpm));
            break;
        }
        case 3: { // Steer Torque
            controller.setSteerTorque(cfg.steerTorque + (delta * 0.10f));
            break;
        }
        case 4: { // Sweep
            controller.setAutoSweep(!cfg.autoSweep);
            break;
        }
        default: break;
    }
}

void PageEpsTester::onJoystickAction(EcuHal::JoyAction action,
                                    EcuEngine::EpsController& controller) {
    if (action == EcuHal::JoyAction::Left) {
        controller.setSteerTorque(controller.getConfig().steerTorque - 0.20f);
    } else if (action == EcuHal::JoyAction::Right) {
        controller.setSteerTorque(controller.getConfig().steerTorque + 0.20f);
    }
}

void PageEpsTester::onEncoderClick(EcuEngine::EpsController& controller) {
    controller.toggleRunning();
}

} // namespace EcuUi
