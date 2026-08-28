#include "eps_driver.h"
#include <Arduino.h>
#include "pin_config.h"

namespace EcuHal {

EpsDriver::EpsDriver() = default;

void EpsDriver::init() {
    if (_initialized) return;

    // 1. Channel VSS (LEDC Timer for dynamic frequency, 50% duty square wave)
    ledcSetup(LEDC_CH_VSS, 100, 8); // 8-bit resolution
    ledcAttachPin(PinConfig::EPS_VSS, LEDC_CH_VSS);
    ledcWrite(LEDC_CH_VSS, 0);

    // 2. Channel RPM Tachometer
    ledcSetup(LEDC_CH_RPM, 100, 8);
    ledcAttachPin(PinConfig::EPS_RPM, LEDC_CH_RPM);
    ledcWrite(LEDC_CH_RPM, 0);

    // 3. Channel TRQ1 (High-frequency PWM 20kHz for smooth analog DAC filtering)
    ledcSetup(LEDC_CH_TRQ1, 20000, 8);
    ledcAttachPin(PinConfig::EPS_TRQ1, LEDC_CH_TRQ1);
    ledcWrite(LEDC_CH_TRQ1, 128); // Center 50% duty (~1.65V raw / 2.5V filtered)

    // 4. Channel TRQ2
    ledcSetup(LEDC_CH_TRQ2, 20000, 8);
    ledcAttachPin(PinConfig::EPS_TRQ2, LEDC_CH_TRQ2);
    ledcWrite(LEDC_CH_TRQ2, 128);

    _initialized = true;
}

void EpsDriver::updateOutputs(const EcuEngine::EpsRuntimeState& state) {
    if (!_initialized) {
        init();
    }

    if (!state.isRunning) {
        if (_lastRunning) {
            stop();
            _lastRunning = false;
        }
        return;
    }
    _lastRunning = true;

    // 1. Update VSS Frequency
    if (state.vssFreqHz != _lastVssFreq) {
        if (state.vssFreqHz > 0.5f) {
            ledcWriteTone(LEDC_CH_VSS, static_cast<double>(state.vssFreqHz));
        } else {
            ledcWrite(LEDC_CH_VSS, 0);
        }
        _lastVssFreq = state.vssFreqHz;
    }

    // 2. Update RPM Frequency
    if (state.rpmFreqHz != _lastRpmFreq) {
        if (state.rpmFreqHz > 0.5f) {
            ledcWriteTone(LEDC_CH_RPM, static_cast<double>(state.rpmFreqHz));
        } else {
            ledcWrite(LEDC_CH_RPM, 0);
        }
        _lastRpmFreq = state.rpmFreqHz;
    }

    // 3. Update TRQ1 PWM Duty
    if (state.trq1Voltage != _lastTrq1Volt) {
        // Map 0.0V - 3.3V (MCU level) to 0 - 255
        float norm = (state.trq1Voltage / 3.30f) * 255.0f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 255.0f) norm = 255.0f;
        ledcWrite(LEDC_CH_TRQ1, static_cast<uint32_t>(norm));
        _lastTrq1Volt = state.trq1Voltage;
    }

    // 4. Update TRQ2 PWM Duty
    if (state.trq2Voltage != _lastTrq2Volt) {
        float norm = (state.trq2Voltage / 3.30f) * 255.0f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 255.0f) norm = 255.0f;
        ledcWrite(LEDC_CH_TRQ2, static_cast<uint32_t>(norm));
        _lastTrq2Volt = state.trq2Voltage;
    }
}

void EpsDriver::stop() {
    if (!_initialized) return;
    ledcWrite(LEDC_CH_VSS, 0);
    ledcWrite(LEDC_CH_RPM, 0);
    _lastVssFreq = -1.0f;
    _lastRpmFreq = -1.0f;
}

} // namespace EcuHal
