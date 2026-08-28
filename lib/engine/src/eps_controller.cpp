#include "eps_controller.h"

namespace EcuEngine {

static const EpsPresetData PRESET_DATABASE[] = {
    {
        "Toyota / Daihatsu",
        "Avanza, Xenia, Rush, Terios, Vios",
        2548.0f, // ~2.548 Hz per km/h (4 pulses/wheel rev)
        2,       // 2 pulses / crank rev (4-cyl Tach)
        2.50f,   // TRQ Center 2.50V
        1.50f    // TRQ Span 1.50V (1.00V - 4.00V)
    },
    {
        "Suzuki Karimun",
        "Karimun Wagon R, Estilo, Kotak",
        2548.0f,
        2,
        2.50f,
        1.20f    // TRQ Span 1.20V (1.30V - 3.70V)
    },
    {
        "Suzuki Ertiga",
        "Ertiga, Swift, Splash, Ignis",
        2548.0f,
        2,
        2.50f,
        1.50f
    },
    {
        "Honda Jazz",
        "Jazz GD3/GE8, Brio, Mobilio, City",
        2548.0f,
        2,
        2.50f,
        1.50f
    },
    {
        "Custom Parametric",
        "Manual Tuning Mode",
        2548.0f,
        2,
        2.50f,
        1.50f
    }
};

EpsController::EpsController() {
    init();
}

void EpsController::init() {
    _config.preset = EpsOemPreset::ToyotaAvanza;
    _config.speedKmh = 30.0f;
    _config.targetRpm = 1000;
    _config.vssPulsePerKm = 2548.0f;
    _config.rpmPulsesPerRev = 2;
    _config.steerTorque = 0.0f;
    _config.autoSweep = false;
    _config.sweepMinSpeed = 0.0f;
    _config.sweepMaxSpeed = 120.0f;
    _config.sweepStep = 2.0f;

    _state.isRunning = false;
    _state.currentSpeedKmh = _config.speedKmh;
    _state.currentRpm = _config.targetRpm;
    _state.sweepDirectionUp = true;

    setPreset(_config.preset);
    _recalculateFrequencies();
}

void EpsController::setPreset(EpsOemPreset preset) {
    uint8_t idx = static_cast<uint8_t>(preset);
    if (idx >= static_cast<uint8_t>(EpsOemPreset::COUNT)) {
        preset = EpsOemPreset::ToyotaAvanza;
        idx = 0;
    }
    _config.preset = preset;

    const auto& data = PRESET_DATABASE[idx];
    if (preset != EpsOemPreset::CustomParametric) {
        _config.vssPulsePerKm = data.vssPulsePerKm;
        _config.rpmPulsesPerRev = data.rpmPulsesPerRev;
    }
    _recalculateFrequencies();
}

void EpsController::setSpeed(float kmh) {
    if (kmh < 0.0f) kmh = 0.0f;
    if (kmh > 250.0f) kmh = 250.0f;
    _config.speedKmh = kmh;
    _state.currentSpeedKmh = kmh;
    _recalculateFrequencies();
}

void EpsController::setRpm(uint32_t rpm) {
    if (rpm > 9000) rpm = 9000;
    _config.targetRpm = rpm;
    _state.currentRpm = rpm;
    _recalculateFrequencies();
}

void EpsController::setSteerTorque(float torque) {
    if (torque < -1.0f) torque = -1.0f;
    if (torque > 1.0f) torque = 1.0f;
    _config.steerTorque = torque;
    _recalculateFrequencies();
}

void EpsController::setAutoSweep(bool enabled) {
    _config.autoSweep = enabled;
}

void EpsController::setRunning(bool running) {
    _state.isRunning = running;
    _recalculateFrequencies();
}

void EpsController::toggleRunning() {
    setRunning(!_state.isRunning);
}

void EpsController::update(float dtSeconds) {
    if (!_state.isRunning) {
        _state.vssFreqHz = 0.0f;
        _state.rpmFreqHz = 0.0f;
        return;
    }

    if (_config.autoSweep) {
        float speed = _state.currentSpeedKmh;
        if (_state.sweepDirectionUp) {
            speed += _config.sweepStep * (dtSeconds * 10.0f);
            if (speed >= _config.sweepMaxSpeed) {
                speed = _config.sweepMaxSpeed;
                _state.sweepDirectionUp = false;
            }
        } else {
            speed -= _config.sweepStep * (dtSeconds * 10.0f);
            if (speed <= _config.sweepMinSpeed) {
                speed = _config.sweepMinSpeed;
                _state.sweepDirectionUp = true;
            }
        }
        _state.currentSpeedKmh = speed;
    } else {
        _state.currentSpeedKmh = _config.speedKmh;
    }

    _state.currentRpm = _config.targetRpm;
    _recalculateFrequencies();
}

void EpsController::_recalculateFrequencies() {
    if (!_state.isRunning) {
        _state.vssFreqHz = 0.0f;
        _state.rpmFreqHz = 0.0f;
        return;
    }

    // Freq VSS (Hz) = Speed (km/h) * PulsesPerKm / 3600
    if (_state.currentSpeedKmh > 0.1f) {
        _state.vssFreqHz = (_state.currentSpeedKmh * _config.vssPulsePerKm) / 3600.0f;
    } else {
        _state.vssFreqHz = 0.0f;
    }

    // Freq RPM (Hz) = RPM * PulsesPerRev / 60
    if (_state.currentRpm > 50) {
        _state.rpmFreqHz = (_state.currentRpm * static_cast<float>(_config.rpmPulsesPerRev)) / 60.0f;
    } else {
        _state.rpmFreqHz = 0.0f;
    }

    const auto* pData = getPresetData(_config.preset);
    float center = pData ? pData->defaultTrqVoltage : 2.50f;
    float span = pData ? pData->trqVoltageSpan : 1.50f;

    _state.trq1Voltage = center + (span * _config.steerTorque);
    _state.trq2Voltage = center - (span * _config.steerTorque);

    if (_state.trq1Voltage < 0.1f) _state.trq1Voltage = 0.1f;
    if (_state.trq1Voltage > 4.9f) _state.trq1Voltage = 4.9f;
    if (_state.trq2Voltage < 0.1f) _state.trq2Voltage = 0.1f;
    if (_state.trq2Voltage > 4.9f) _state.trq2Voltage = 4.9f;
}

const EpsPresetData* EpsController::getPresetData(EpsOemPreset preset) {
    uint8_t idx = static_cast<uint8_t>(preset);
    if (idx >= static_cast<uint8_t>(EpsOemPreset::COUNT)) {
        idx = 0;
    }
    return &PRESET_DATABASE[idx];
}

const char* EpsController::getPresetName(EpsOemPreset preset) {
    const auto* p = getPresetData(preset);
    return p ? p->name : "Unknown";
}

} // namespace EcuEngine
