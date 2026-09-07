#include "eps_driver.h"
#include <Arduino.h>
#include <Wire.h>
#include "pin_config.h"

namespace EcuHal {

static volatile uint8_t s_vssLevel = 0;
static volatile uint8_t s_rpmLevel = 0;

static void IRAM_ATTR vssTimerCallback(void* arg) {
    s_vssLevel ^= 1;
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), s_vssLevel);
}

static void IRAM_ATTR rpmTimerCallback(void* arg) {
    s_rpmLevel ^= 1;
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), s_rpmLevel);
}

EpsDriver::EpsDriver() = default;

EpsDriver::~EpsDriver() {
    stop();
    if (_vssTimer) esp_timer_delete(_vssTimer);
    if (_rpmTimer) esp_timer_delete(_rpmTimer);
}

void EpsDriver::detectDacs(bool& trq1Found, bool& trq2Found) {
    Wire.setTimeOut(2);
    Wire.beginTransmission(MCP4725_ADDR_TRQ1);
    _dacTrq1Found = (Wire.endTransmission() == 0);

    Wire.beginTransmission(MCP4725_ADDR_TRQ2);
    _dacTrq2Found = (Wire.endTransmission() == 0);

    trq1Found = _dacTrq1Found;
    trq2Found = _dacTrq2Found;
}

void EpsDriver::_writeDac(uint8_t addr, float volts) {
    if (volts < 0.0f) volts = 0.0f;
    if (volts > 5.0f) volts = 5.0f;
    uint16_t dacValue = static_cast<uint16_t>((volts / 5.0f) * 4095.0f);
    if (dacValue > 4095) dacValue = 4095;

    Wire.setTimeOut(2);
    Wire.beginTransmission(addr);
    Wire.write(static_cast<uint8_t>((dacValue >> 8) & 0x0F));
    Wire.write(static_cast<uint8_t>(dacValue & 0xFF));
    Wire.endTransmission();
}

void EpsDriver::init() {
    if (_initialized) return;

    // 1. Setup GPIO output for VSS & RPM
    pinMode(PinConfig::EPS_VSS, OUTPUT);
    digitalWrite(PinConfig::EPS_VSS, LOW);
    pinMode(PinConfig::EPS_RPM, OUTPUT);
    digitalWrite(PinConfig::EPS_RPM, LOW);

    // 2. Setup esp_timer for VSS
    esp_timer_create_args_t vss_args{};
    vss_args.callback = &vssTimerCallback;
    vss_args.name = "eps_vss_timer";
    vss_args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_create(&vss_args, &_vssTimer);

    // 3. Setup esp_timer for RPM
    esp_timer_create_args_t rpm_args{};
    rpm_args.callback = &rpmTimerCallback;
    rpm_args.name = "eps_rpm_timer";
    rpm_args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_create(&rpm_args, &_rpmTimer);

    // 4. Setup PWM on GPIO 40 (TRQ1) & 41 (TRQ2) as 20kHz RC-filter DAC fallback
    ledcSetup(LEDC_CH_TRQ1, 20000, 8);
    ledcAttachPin(PinConfig::EPS_TRQ1, LEDC_CH_TRQ1);
    ledcWrite(LEDC_CH_TRQ1, 128); // 50% = 2.5V center

    ledcSetup(LEDC_CH_TRQ2, 20000, 8);
    ledcAttachPin(PinConfig::EPS_TRQ2, LEDC_CH_TRQ2);
    ledcWrite(LEDC_CH_TRQ2, 128);

    // 5. Detect I2C Dual MCP4725
    bool d1 = false, d2 = false;
    detectDacs(d1, d2);
    if (_dacTrq1Found) _writeDac(MCP4725_ADDR_TRQ1, 2.50f);
    if (_dacTrq2Found) _writeDac(MCP4725_ADDR_TRQ2, 2.50f);

    _initialized = true;
}

void EpsDriver::_setVssFrequency(float freqHz) {
    if (freqHz > 0.2f && _vssTimer) {
        uint64_t halfPeriodUs = static_cast<uint64_t>(500000.0f / freqHz);
        if (halfPeriodUs < 100) halfPeriodUs = 100;
        if (_vssTimerRunning) {
            esp_timer_stop(_vssTimer);
        }
        esp_timer_start_periodic(_vssTimer, halfPeriodUs);
        _vssTimerRunning = true;
    } else {
        if (_vssTimerRunning && _vssTimer) {
            esp_timer_stop(_vssTimer);
            _vssTimerRunning = false;
        }
        s_vssLevel = 0;
        digitalWrite(PinConfig::EPS_VSS, LOW);
    }
}

void EpsDriver::_setRpmFrequency(float freqHz) {
    if (freqHz > 0.2f && _rpmTimer) {
        uint64_t halfPeriodUs = static_cast<uint64_t>(500000.0f / freqHz);
        if (halfPeriodUs < 100) halfPeriodUs = 100;
        if (_rpmTimerRunning) {
            esp_timer_stop(_rpmTimer);
        }
        esp_timer_start_periodic(_rpmTimer, halfPeriodUs);
        _rpmTimerRunning = true;
    } else {
        if (_rpmTimerRunning && _rpmTimer) {
            esp_timer_stop(_rpmTimer);
            _rpmTimerRunning = false;
        }
        s_rpmLevel = 0;
        digitalWrite(PinConfig::EPS_RPM, LOW);
    }
}

void EpsDriver::updateOutputs(const EcuEngine::EpsRuntimeState& state) {
    if (!_initialized) init();

    if (!state.isRunning) {
        if (_lastRunning) {
            _setVssFrequency(0.0f);
            _setRpmFrequency(0.0f);
            _lastVssFreq = -1.0f;
            _lastRpmFreq = -1.0f;
            _lastRunning = false;
        }
    } else {
        _lastRunning = true;

        // 1. Update VSS Pulse Frequency (0.5 Hz - 250 Hz)
        if (state.vssFreqHz != _lastVssFreq) {
            _setVssFrequency(state.vssFreqHz);
            _lastVssFreq = state.vssFreqHz;
        }

        // 2. Update Engine RPM Pulse Frequency (10 Hz - 300 Hz)
        if (state.rpmFreqHz != _lastRpmFreq) {
            _setRpmFrequency(state.rpmFreqHz);
            _lastRpmFreq = state.rpmFreqHz;
        }
    }

    // 3. Update TRQ1 (MCP4725 0x60 + PWM GPIO 40) - ALWAYS ACTIVE
    if (state.trq1Voltage != _lastTrq1Volt) {
        if (_dacTrq1Found) {
            _writeDac(MCP4725_ADDR_TRQ1, state.trq1Voltage);
        }
        // Simultaneous PWM on GPIO 40
        float norm = (state.trq1Voltage / 5.0f) * 255.0f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 255.0f) norm = 255.0f;
        ledcWrite(LEDC_CH_TRQ1, static_cast<uint32_t>(norm));
        _lastTrq1Volt = state.trq1Voltage;
    }

    // 4. Update TRQ2 (MCP4725 0x61 + PWM GPIO 41) - ALWAYS ACTIVE
    if (state.trq2Voltage != _lastTrq2Volt) {
        if (_dacTrq2Found) {
            _writeDac(MCP4725_ADDR_TRQ2, state.trq2Voltage);
        }
        // Simultaneous PWM on GPIO 41
        float norm = (state.trq2Voltage / 5.0f) * 255.0f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 255.0f) norm = 255.0f;
        ledcWrite(LEDC_CH_TRQ2, static_cast<uint32_t>(norm));
        _lastTrq2Volt = state.trq2Voltage;
    }
}

void EpsDriver::stop() {
    if (!_initialized) return;

    _setVssFrequency(0.0f);
    _setRpmFrequency(0.0f);

    _lastVssFreq = -1.0f;
    _lastRpmFreq = -1.0f;
    _lastRunning = false;
}

} // namespace EcuHal
