#include "eps_controller.h"
#include <Preferences.h>

namespace EcuEngine {

static const EpsPresetData PRESET_DATABASE[] = {
    {
        "Toyota / Daihatsu",
        "Avanza, Xenia, Rush, Terios, Vios (Denso)",
        2548.0f, // ~2.548 Hz per km/h (4 pulses/wheel rev)
        2,       // 2 pulses / crank rev (4-cyl Tach)
        2.500f,  // TRQ Center 2.500V
        1.500f,  // TRQ Span 1.500V (1.000V - 4.000V)
        40.0f,   // Default Speed 40 km/h (28.3 Hz)
        1200     // Default RPM 1200 (40.0 Hz)
    },
    {
        "Suzuki Karimun",
        "Karimun Wagon R, Estilo (NSK/Koyo)",
        4000.0f, // 4000 pulses/km (Wagon R cluster pulse)
        2,
        2.500f,
        1.100f,  // Tighter TRQ Span 1.100V (1.400V - 3.600V)
        40.0f,   // Default Speed 40 km/h (44.4 Hz)
        1100     // Default RPM 1100 (36.7 Hz)
    },
    {
        "Suzuki Ertiga",
        "Ertiga, Splash, Ignis (Mitsubishi)",
        4000.0f,
        2,
        2.500f,
        1.350f,  // TRQ Span 1.350V (1.150V - 3.850V)
        50.0f,   // Default Speed 50 km/h (55.6 Hz)
        1400     // Default RPM 1400 (46.7 Hz)
    },
    {
        "Suzuki Swift",
        "Swift 2006 (Active Amp 2.50V / Pin E52-16)",
        4000.0f, // 4000 pulses/km
        2,       // 2 pulses / rev
        2.500f,  // TRQ Center 2.500V
        1.350f,  // TRQ Span 1.350V (1.150V - 3.850V)
        40.0f,   // Default Speed 40 km/h (44.4 Hz)
        1200     // Default RPM 1200 (40.0 Hz)
    },
    {
        "Honda Jazz / Brio",
        "Jazz GD3/GE8, Brio, Mobilio (Showa DC)",
        4100.0f, // 4100 pulses/km (Honda VSS gear)
        2,
        2.430f,  // Proven Exact Neutral: 2.430V
        0.100f,  // Micro Steer Span: +-0.100V (2.330V - 2.530V)
        40.0f,   // Default Speed 40 km/h (45.6 Hz)
        1200     // Default RPM 1200 (40.0 Hz)
    },
    {
        "Honda City (Ind)",
        "City GM2/GM6, Civic (Showa 7.3 Ohm Coil)",
        4100.0f, // 4100 pulses/km
        2,
        2.500f,  // Demodulated Center 2.500V
        0.080f,  // Micro-Trim Span: +-0.080V (Inductive Helper)
        40.0f,   // Default Speed 40 km/h (45.6 Hz)
        1200     // Default RPM 1200 (40.0 Hz)
    },
    {
        "Retrofit / Swap",
        "Bypass Standalone (Jimny/Kijang/Taft)",
        2548.0f,
        2,
        2.500f,
        1.500f,
        15.0f,   // Speed 15 km/h (10.6 Hz - Max Assist Enteng!)
        950      // RPM 950 (31.7 Hz - Engine Running relay ON)
    },
    {
        "Custom Parametric",
        "Manual Tuning Bebas Parameter",
        2548.0f,
        2,
        2.500f,
        1.500f,
        40.0f,
        1200
    }
};

EpsController::EpsController() {
    init();
}

void EpsController::init() {
    _config.preset = EpsOemPreset::ToyotaAvanza;
    _config.speedKmh = 40.0f;
    _config.targetRpm = 1200;
    _config.vssPulsePerKm = 2548.0f;
    _config.rpmPulsesPerRev = 2;
    _config.steerTorque = 0.0f;
    _config.trqCenterVoltage = 2.500f;
    _config.trqVoltageSpan = 1.500f;
    _config.autoSweep = false;
    _config.sweepMinSpeed = 0.0f;
    _config.sweepMaxSpeed = 120.0f;
    _config.sweepStep = 2.0f;
    _config.trq1AdcScale = 2.000f;
    _config.trq1AdcOffset = 0.000f;
    _config.trq2AdcScale = 2.000f;
    _config.trq2AdcOffset = 0.000f;

    _state.isRunning = false;
    _state.currentSpeedKmh = _config.speedKmh;
    _state.currentRpm = _config.targetRpm;
    _state.sweepDirectionUp = true;
    _state.trq1FeedbackVoltage = 0.0f;
    _state.trq2FeedbackVoltage = 0.0f;
    _state.adcFound = false;

    loadCalibration();
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
        _config.trqCenterVoltage = data.defaultTrqVoltage;
        _config.trqVoltageSpan = data.trqVoltageSpan;
        _config.speedKmh = data.defaultSpeedKmh;
        _state.currentSpeedKmh = data.defaultSpeedKmh;
        _config.targetRpm = data.defaultRpm;
        _state.currentRpm = data.defaultRpm;
        _config.steerTorque = 0.0f; // Auto-Zero Neutral Startup
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

void EpsController::setCenterVoltage(float volts) {
    if (volts < 0.500f) volts = 0.500f;
    if (volts > 4.500f) volts = 4.500f;
    _config.trqCenterVoltage = volts;
    _recalculateFrequencies();
}

void EpsController::setSpanVoltage(float volts) {
    if (volts < 0.020f) volts = 0.020f;
    if (volts > 2.000f) volts = 2.000f;
    _config.trqVoltageSpan = volts;
    _recalculateFrequencies();
}

void EpsController::setAutoSweep(bool enabled) {
    _config.autoSweep = enabled;
}

void EpsController::setRunning(bool running) {
    _state.isRunning = running;
    if (running) {
        // Auto-zero steer torque on start if outside deadband
        // so ECU doesn't detect pre-loaded torque upon power-up
    }
    _recalculateFrequencies();
}

void EpsController::toggleRunning() {
    setRunning(!_state.isRunning);
}

void EpsController::setVssPulsePerKm(float pulses) {
    if (pulses < 100.0f) pulses = 100.0f;
    if (pulses > 10000.0f) pulses = 10000.0f;
    _config.vssPulsePerKm = pulses;
    _recalculateFrequencies();
}

void EpsController::setRpmPulsesPerRev(uint8_t pulses) {
    if (pulses < 1) pulses = 1;
    if (pulses > 8) pulses = 8;
    _config.rpmPulsesPerRev = pulses;
    _recalculateFrequencies();
}

void EpsController::setSweepLimits(float minKmh, float maxKmh, float step) {
    if (minKmh < 0.0f) minKmh = 0.0f;
    if (maxKmh > 250.0f) maxKmh = 250.0f;
    if (minKmh >= maxKmh) minKmh = maxKmh - 10.0f;
    if (step < 0.5f) step = 0.5f;
    _config.sweepMinSpeed = minKmh;
    _config.sweepMaxSpeed = maxKmh;
    _config.sweepStep = step;
}

void EpsController::update(float dtSeconds) {
    if (!_state.isRunning) {
        _state.vssFreqHz = 0.0f;
        _state.rpmFreqHz = 0.0f;
        _recalculateFrequencies();
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
    // TRQ1 and TRQ2 using dedicated calibrated center and span (always active)
    _state.trq1Voltage = _config.trqCenterVoltage + (_config.trqVoltageSpan * _config.steerTorque);
    _state.trq2Voltage = _config.trqCenterVoltage - (_config.trqVoltageSpan * _config.steerTorque);

    if (_state.trq1Voltage < 0.050f) _state.trq1Voltage = 0.050f;
    if (_state.trq1Voltage > 4.950f) _state.trq1Voltage = 4.950f;
    if (_state.trq2Voltage < 0.050f) _state.trq2Voltage = 0.050f;
    if (_state.trq2Voltage > 4.950f) _state.trq2Voltage = 4.950f;

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

void EpsController::setFeedbackVoltages(float fb1, float fb2, bool adcFound) {
    _state.trq1FeedbackVoltage = fb1;
    _state.trq2FeedbackVoltage = fb2;
    _state.adcFound = adcFound;
}

void EpsController::setTrq1Scale(float scale) {
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 5.0f) scale = 5.0f;
    _config.trq1AdcScale = scale;
}

void EpsController::setTrq1Offset(float offset) {
    if (offset < -1.0f) offset = -1.0f;
    if (offset > 1.0f) offset = 1.0f;
    _config.trq1AdcOffset = offset;
}

void EpsController::setTrq2Scale(float scale) {
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 5.0f) scale = 5.0f;
    _config.trq2AdcScale = scale;
}

void EpsController::setTrq2Offset(float offset) {
    if (offset < -1.0f) offset = -1.0f;
    if (offset > 1.0f) offset = 1.0f;
    _config.trq2AdcOffset = offset;
}

void EpsController::saveCalibration() {
    Preferences p;
    p.begin("eps_conf", false);
    p.putFloat("t1_scale", _config.trq1AdcScale);
    p.putFloat("t1_off", _config.trq1AdcOffset);
    p.putFloat("t2_scale", _config.trq2AdcScale);
    p.putFloat("t2_off", _config.trq2AdcOffset);
    p.end();
}

void EpsController::loadCalibration() {
    Preferences p;
    p.begin("eps_conf", true);
    _config.trq1AdcScale = p.getFloat("t1_scale", 2.000f);
    _config.trq1AdcOffset = p.getFloat("t1_off", 0.000f);
    _config.trq2AdcScale = p.getFloat("t2_scale", 2.000f);
    _config.trq2AdcOffset = p.getFloat("t2_off", 0.000f);
    p.end();
}

} // namespace EcuEngine
