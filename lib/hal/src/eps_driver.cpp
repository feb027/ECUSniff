#include "eps_driver.h"
#include <Arduino.h>
#include <Wire.h>
#include "driver/gpio.h"
#include "pin_config.h"

namespace EcuHal {

// High-precision non-blocking phase timers for VSS and RPM
static volatile uint32_t s_vssHalfPeriodUs = 0;
static volatile uint8_t  s_vssLevel = 0;
static volatile bool     s_vssActive = false;
static esp_timer_handle_t s_vssTimerHandle = nullptr;

static volatile uint32_t s_rpmHalfPeriodUs = 0;
static volatile uint8_t  s_rpmLevel = 0;
static volatile bool     s_rpmActive = false;
static esp_timer_handle_t s_rpmTimerHandle = nullptr;

// Self-rearming callback for VSS (0.2 Hz - 1,000 Hz seamless square wave)
static void IRAM_ATTR vssTimerCallback(void* arg) {
    uint32_t hp = s_vssHalfPeriodUs;
    if (hp == 0 || !s_vssActive) {
        s_vssLevel = 0;
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 0);
        return;
    }
    s_vssLevel ^= 1;
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), s_vssLevel);
    esp_timer_start_once(s_vssTimerHandle, hp);
}

// Self-rearming callback for RPM (0.2 Hz - 1,000 Hz seamless square wave)
static void IRAM_ATTR rpmTimerCallback(void* arg) {
    uint32_t hp = s_rpmHalfPeriodUs;
    if (hp == 0 || !s_rpmActive) {
        s_rpmLevel = 0;
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 0);
        return;
    }
    s_rpmLevel ^= 1;
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), s_rpmLevel);
    esp_timer_start_once(s_rpmTimerHandle, hp);
}

EpsDriver::EpsDriver() = default;

EpsDriver::~EpsDriver() {
    stop();
    if (_vssTimer) {
        esp_timer_delete(_vssTimer);
        _vssTimer = nullptr;
        s_vssTimerHandle = nullptr;
    }
    if (_rpmTimer) {
        esp_timer_delete(_rpmTimer);
        _rpmTimer = nullptr;
        s_rpmTimerHandle = nullptr;
    }
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

    // 1. Setup clean direct GPIO output for VSS & RPM using native ESP-IDF driver
    gpio_reset_pin(static_cast<gpio_num_t>(PinConfig::EPS_VSS));
    gpio_set_direction(static_cast<gpio_num_t>(PinConfig::EPS_VSS), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 0);

    gpio_reset_pin(static_cast<gpio_num_t>(PinConfig::EPS_RPM));
    gpio_set_direction(static_cast<gpio_num_t>(PinConfig::EPS_RPM), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 0);

    // 2. Setup high-precision esp_timer for VSS (Vehicle Speed Sensor)
    esp_timer_create_args_t vss_args{};
    vss_args.callback = &vssTimerCallback;
    vss_args.name = "eps_vss_timer";
    vss_args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_create(&vss_args, &_vssTimer);
    s_vssTimerHandle = _vssTimer;

    // 3. Setup high-precision esp_timer for RPM (Engine Speed Tachometer)
    esp_timer_create_args_t rpm_args{};
    rpm_args.callback = &rpmTimerCallback;
    rpm_args.name = "eps_rpm_timer";
    rpm_args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_create(&rpm_args, &_rpmTimer);
    s_rpmTimerHandle = _rpmTimer;

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
    if (freqHz > 0.2f) {
        uint32_t newHalfPeriod = static_cast<uint32_t>(500000.0f / freqHz);
        if (newHalfPeriod < 250) newHalfPeriod = 250; // Cap at 2,000 Hz
        s_vssHalfPeriodUs = newHalfPeriod;

        if (!s_vssActive) {
            s_vssActive = true;
            s_vssLevel = 1;
            gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 1);
            if (s_vssTimerHandle) {
                esp_timer_stop(s_vssTimerHandle);
                esp_timer_start_once(s_vssTimerHandle, newHalfPeriod);
            }
        }
        // Seamless: when already active, s_vssHalfPeriodUs is picked up at next toggle!
    } else {
        if (s_vssActive) {
            s_vssActive = false;
            s_vssHalfPeriodUs = 0;
            if (s_vssTimerHandle) {
                esp_timer_stop(s_vssTimerHandle);
            }
            s_vssLevel = 0;
            gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 0);
        }
    }
}

void EpsDriver::_setRpmFrequency(float freqHz) {
    if (freqHz > 0.2f) {
        uint32_t newHalfPeriod = static_cast<uint32_t>(500000.0f / freqHz);
        if (newHalfPeriod < 250) newHalfPeriod = 250; // Cap at 2,000 Hz
        s_rpmHalfPeriodUs = newHalfPeriod;

        if (!s_rpmActive) {
            s_rpmActive = true;
            s_rpmLevel = 1;
            gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 1);
            if (s_rpmTimerHandle) {
                esp_timer_stop(s_rpmTimerHandle);
                esp_timer_start_once(s_rpmTimerHandle, newHalfPeriod);
            }
        }
    } else {
        if (s_rpmActive) {
            s_rpmActive = false;
            s_rpmHalfPeriodUs = 0;
            if (s_rpmTimerHandle) {
                esp_timer_stop(s_rpmTimerHandle);
            }
            s_rpmLevel = 0;
            gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 0);
        }
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
