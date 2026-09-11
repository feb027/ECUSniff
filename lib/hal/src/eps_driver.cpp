#include "eps_driver.h"
#include <Arduino.h>
#include <Wire.h>
#include "driver/gpio.h"
#include "pin_config.h"

namespace EcuHal {

static volatile uint8_t s_vssLevel = 0;
static volatile bool    s_vssActive = false;
static volatile uint32_t s_vssToggleCount = 0;

static volatile uint8_t s_rpmLevel = 0;
static volatile bool    s_rpmActive = false;
static volatile uint32_t s_rpmToggleCount = 0;

// Periodic callbacks for continuous, non-blocking square wave pulse train
static void IRAM_ATTR vssTimerCallback(void* arg) {
    if (!s_vssActive) return;
    s_vssLevel ^= 1;
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), s_vssLevel);
    s_vssToggleCount++;
}

static void IRAM_ATTR rpmTimerCallback(void* arg) {
    if (!s_rpmActive) return;
    s_rpmLevel ^= 1;
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), s_rpmLevel);
    s_rpmToggleCount++;
}

uint32_t EpsDriver::getVssToggles() const { return s_vssToggleCount; }
uint32_t EpsDriver::getRpmToggles() const { return s_rpmToggleCount; }

EpsDriver::EpsDriver() = default;

EpsDriver::~EpsDriver() {
    stop();
    if (_vssTimer) {
        esp_timer_delete(_vssTimer);
        _vssTimer = nullptr;
    }
    if (_rpmTimer) {
        esp_timer_delete(_rpmTimer);
        _rpmTimer = nullptr;
    }
}

void EpsDriver::detectDacs(bool& trq1Found, bool& trq2Found) {
    Wire.setTimeOut(50);

    // Check default addresses 0x60 & 0x61
    Wire.beginTransmission(MCP4725_ADDR_TRQ1);
    _dacTrq1Found = (Wire.endTransmission() == 0);
    if (_dacTrq1Found) _trq1Addr = MCP4725_ADDR_TRQ1;

    Wire.beginTransmission(MCP4725_ADDR_TRQ2);
    _dacTrq2Found = (Wire.endTransmission() == 0);
    if (_dacTrq2Found) _trq2Addr = MCP4725_ADDR_TRQ2;

    // Fallback: Check 0x62 & 0x63 (MCP4725A1 chips)
    if (!_dacTrq1Found) {
        Wire.beginTransmission(0x62);
        if (Wire.endTransmission() == 0) {
            _dacTrq1Found = true;
            _trq1Addr = 0x62;
        }
    }
    if (!_dacTrq2Found) {
        Wire.beginTransmission(0x63);
        if (Wire.endTransmission() == 0) {
            _dacTrq2Found = true;
            _trq2Addr = 0x63;
        }
    }

    trq1Found = _dacTrq1Found;
    trq2Found = _dacTrq2Found;
}

void EpsDriver::_writeDac(uint8_t addr, float volts) {
    if (volts < 0.0f) volts = 0.0f;
    if (volts > 5.0f) volts = 5.0f;
    uint16_t dacValue = static_cast<uint16_t>((volts / 5.0f) * 4095.0f);
    if (dacValue > 4095) dacValue = 4095;

    Wire.setTimeOut(50);
    Wire.beginTransmission(addr);
    Wire.write(static_cast<uint8_t>((dacValue >> 8) & 0x0F));
    Wire.write(static_cast<uint8_t>(dacValue & 0xFF));
    Wire.endTransmission();
}

void EpsDriver::init() {
    if (_initialized) return;

    // 1. Reset pins to strip any lingering JTAG / IO MUX peripheral mapping
    gpio_reset_pin(static_cast<gpio_num_t>(PinConfig::EPS_VSS));
    gpio_reset_pin(static_cast<gpio_num_t>(PinConfig::EPS_RPM));

    // 2. Configure dedicated VSS & RPM pins with Input/Output mode and pull-down
    gpio_config_t io_conf{};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PinConfig::EPS_VSS) | (1ULL << PinConfig::EPS_RPM);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_drive_capability(static_cast<gpio_num_t>(PinConfig::EPS_VSS), GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability(static_cast<gpio_num_t>(PinConfig::EPS_RPM), GPIO_DRIVE_CAP_3);

    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 0);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 0);

    // 3. Hardware Short-Circuit & Pin Crosstalk Self-Test
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 1);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 0);
    delayMicroseconds(50);
    int readVss1 = gpio_get_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS));
    int readRpm1 = gpio_get_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM));

    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 0);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 1);
    delayMicroseconds(50);
    int readVss2 = gpio_get_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS));
    int readRpm2 = gpio_get_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM));

    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 0);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 0);

    if (readRpm1 == 1 || readVss2 == 1) {
        _hardwareShort = true;
        Serial.printf("\n[EPS DRIVER ALERT] !!! KONSLET TERDETEKSI: Pin %d (VSS) & Pin %d (RPM) TERHUBUNG FISIK DI LUAR CHIP! (R1=%d, R2=%d) !!!\n\n",
                      PinConfig::EPS_VSS, PinConfig::EPS_RPM, readRpm1, readVss2);
    } else {
        _hardwareShort = false;
        Serial.printf("[EPS DRIVER] Self-Test OK: Pin %d (VSS) & Pin %d (RPM) 100%% independen & bebas konslet.\n",
                      PinConfig::EPS_VSS, PinConfig::EPS_RPM);
    }

    // 4. Setup periodic esp_timer for VSS (Vehicle Speed Sensor)
    esp_timer_create_args_t vss_args{};
    vss_args.callback = &vssTimerCallback;
    vss_args.name = "eps_vss_timer";
    vss_args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_create(&vss_args, &_vssTimer);

    // 5. Setup periodic esp_timer for RPM (Engine Speed Tachometer)
    esp_timer_create_args_t rpm_args{};
    rpm_args.callback = &rpmTimerCallback;
    rpm_args.name = "eps_rpm_timer";
    rpm_args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_create(&rpm_args, &_rpmTimer);

    // 6. Setup PWM on GPIO 40 (TRQ1) & 41 (TRQ2) as 20kHz RC-filter DAC fallback
    gpio_reset_pin(static_cast<gpio_num_t>(PinConfig::EPS_TRQ1));
    gpio_reset_pin(static_cast<gpio_num_t>(PinConfig::EPS_TRQ2));
    ledcSetup(LEDC_CH_TRQ1, 20000, 8);
    ledcAttachPin(PinConfig::EPS_TRQ1, LEDC_CH_TRQ1);
    ledcWrite(LEDC_CH_TRQ1, 128); // 50% = 2.5V center

    ledcSetup(LEDC_CH_TRQ2, 20000, 8);
    ledcAttachPin(PinConfig::EPS_TRQ2, LEDC_CH_TRQ2);
    ledcWrite(LEDC_CH_TRQ2, 128);

    // 7. Detect I2C Dual MCP4725
    bool d1 = false, d2 = false;
    detectDacs(d1, d2);
    if (_dacTrq1Found) _writeDac(_trq1Addr, 2.50f);
    if (_dacTrq2Found) _writeDac(_trq2Addr, 2.50f);

    _initialized = true;
}

void EpsDriver::_setVssFrequency(float freqHz) {
    if (freqHz > 0.2f && _vssTimer) {
        uint64_t newHalfPeriod = static_cast<uint64_t>(500000.0f / freqHz);
        if (newHalfPeriod < 250) newHalfPeriod = 250; // Cap at 2,000 Hz

        if (_vssTimerRunning) {
            esp_timer_stop(_vssTimer);
        }
        s_vssActive = true;
        s_vssLevel = 1;
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 1);
        esp_timer_start_periodic(_vssTimer, newHalfPeriod);
        _vssTimerRunning = true;
    } else {
        if (_vssTimerRunning && _vssTimer) {
            esp_timer_stop(_vssTimer);
            _vssTimerRunning = false;
        }
        s_vssActive = false;
        s_vssLevel = 0;
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 0);
    }
}

void EpsDriver::_setRpmFrequency(float freqHz) {
    if (freqHz > 0.2f && _rpmTimer) {
        uint64_t newHalfPeriod = static_cast<uint64_t>(500000.0f / freqHz);
        if (newHalfPeriod < 250) newHalfPeriod = 250; // Cap at 2,000 Hz

        if (_rpmTimerRunning) {
            esp_timer_stop(_rpmTimer);
        }
        s_rpmActive = true;
        s_rpmLevel = 1;
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 1);
        esp_timer_start_periodic(_rpmTimer, newHalfPeriod);
        _rpmTimerRunning = true;
    } else {
        if (_rpmTimerRunning && _rpmTimer) {
            esp_timer_stop(_rpmTimer);
            _rpmTimerRunning = false;
        }
        s_rpmActive = false;
        s_rpmLevel = 0;
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 0);
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

    // 3. Update TRQ1 (MCP4725 + PWM GPIO 40) - ALWAYS ACTIVE
    if (state.trq1Voltage != _lastTrq1Volt) {
        if (_dacTrq1Found) {
            _writeDac(_trq1Addr, state.trq1Voltage);
        }
        // Simultaneous PWM on GPIO 40
        float norm = (state.trq1Voltage / 5.0f) * 255.0f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 255.0f) norm = 255.0f;
        ledcWrite(LEDC_CH_TRQ1, static_cast<uint32_t>(norm));
        _lastTrq1Volt = state.trq1Voltage;
    }

    // 4. Update TRQ2 (MCP4725 + PWM GPIO 41) - ALWAYS ACTIVE
    if (state.trq2Voltage != _lastTrq2Volt) {
        if (_dacTrq2Found) {
            _writeDac(_trq2Addr, state.trq2Voltage);
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

    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 0);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 0);

    _lastVssFreq = -1.0f;
    _lastRpmFreq = -1.0f;
    _lastRunning = false;
}

} // namespace EcuHal
