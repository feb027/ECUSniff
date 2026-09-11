#include "speedo_driver.h"
#include <Arduino.h>
#include <Wire.h>
#include "driver/gpio.h"
#include "pin_config.h"

namespace EcuHal {

static volatile uint8_t  s_speedoKmhLevel = 0;
static volatile bool     s_speedoKmhActive = false;
static volatile uint32_t s_speedoKmhToggleCount = 0;

static volatile uint8_t  s_speedoRpmLevel = 0;
static volatile bool     s_speedoRpmActive = false;
static volatile uint32_t s_speedoRpmToggleCount = 0;

static void IRAM_ATTR speedoKmhTimerCallback(void* arg) {
    if (!s_speedoKmhActive) return;
    s_speedoKmhLevel ^= 1;
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::SPEEDO_KMH), s_speedoKmhLevel);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), s_speedoKmhLevel);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::SIG_CKP), s_speedoKmhLevel);
    s_speedoKmhToggleCount++;
}

static void IRAM_ATTR speedoRpmTimerCallback(void* arg) {
    if (!s_speedoRpmActive) return;
    s_speedoRpmLevel ^= 1;
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::SPEEDO_RPM), s_speedoRpmLevel);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), s_speedoRpmLevel);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::SIG_CMP), s_speedoRpmLevel);
    s_speedoRpmToggleCount++;
}

uint32_t SpeedoDriver::getKmhToggles() const { return s_speedoKmhToggleCount; }
uint32_t SpeedoDriver::getRpmToggles() const { return s_speedoRpmToggleCount; }

SpeedoDriver::SpeedoDriver() = default;

SpeedoDriver::~SpeedoDriver() {
    stop();
    if (_kmhTimer) {
        esp_timer_delete(_kmhTimer);
        _kmhTimer = nullptr;
    }
    if (_rpmTimer) {
        esp_timer_delete(_rpmTimer);
        _rpmTimer = nullptr;
    }
}

void SpeedoDriver::detectDacs(bool& fuelFound, bool& tempFound) {
    Wire.setTimeOut(50);
    Wire.beginTransmission(MCP4725_ADDR_FUEL);
    _dacFuelFound = (Wire.endTransmission() == 0);

    Wire.beginTransmission(MCP4725_ADDR_TEMP);
    _dacTempFound = (Wire.endTransmission() == 0);

    fuelFound = _dacFuelFound;
    tempFound = _dacTempFound;
}

void SpeedoDriver::_writeDac(uint8_t addr, float volts) {
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

void SpeedoDriver::init() {
    if (_initialized) return;

    // 1. Setup GPIO output pins for KMH & RPM with maximum drive capability
    pinMode(PinConfig::SPEEDO_KMH, OUTPUT);
    pinMode(PinConfig::SPEEDO_RPM, OUTPUT);
    pinMode(PinConfig::EPS_VSS, OUTPUT);
    pinMode(PinConfig::EPS_RPM, OUTPUT);
    pinMode(PinConfig::SIG_CKP, OUTPUT);
    pinMode(PinConfig::SIG_CMP, OUTPUT);

    gpio_set_drive_capability(static_cast<gpio_num_t>(PinConfig::SPEEDO_KMH), GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability(static_cast<gpio_num_t>(PinConfig::SPEEDO_RPM), GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability(static_cast<gpio_num_t>(PinConfig::EPS_VSS), GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability(static_cast<gpio_num_t>(PinConfig::EPS_RPM), GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability(static_cast<gpio_num_t>(PinConfig::SIG_CKP), GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability(static_cast<gpio_num_t>(PinConfig::SIG_CMP), GPIO_DRIVE_CAP_3);

    gpio_set_level(static_cast<gpio_num_t>(PinConfig::SPEEDO_KMH), 0);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::SPEEDO_RPM), 0);

    // 2. Setup periodic esp_timer for KM/H (Speedometer pulse)
    esp_timer_create_args_t kmh_args{};
    kmh_args.callback = &speedoKmhTimerCallback;
    kmh_args.name = "speedo_kmh_timer";
    kmh_args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_create(&kmh_args, &_kmhTimer);

    // 3. Setup periodic esp_timer for RPM (Tachometer pulse)
    esp_timer_create_args_t rpm_args{};
    rpm_args.callback = &speedoRpmTimerCallback;
    rpm_args.name = "speedo_rpm_timer";
    rpm_args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_create(&rpm_args, &_rpmTimer);

    // 4. PWM Temp & Fuel channels on Timer 1 (5 kHz, 8-bit)
    ledcSetup(LEDC_CH_TEMP, 5000, 8);
    ledcAttachPin(PinConfig::SPEEDO_TEMP, LEDC_CH_TEMP);
    ledcWrite(LEDC_CH_TEMP, 0);

    ledcSetup(LEDC_CH_FUEL, 5000, 8);
    ledcAttachPin(PinConfig::SPEEDO_FUEL, LEDC_CH_FUEL);
    ledcWrite(LEDC_CH_FUEL, 0);

    // 5. Detect MCP4725 DACs
    bool f, t;
    detectDacs(f, t);
    if (_dacFuelFound) _writeDac(MCP4725_ADDR_FUEL, 0.0f);
    if (_dacTempFound) _writeDac(MCP4725_ADDR_TEMP, 0.0f);

    _initialized = true;
}

void SpeedoDriver::_setKmhFrequency(float freqHz) {
    if (freqHz > 0.2f && _kmhTimer) {
        uint64_t newHalfPeriod = static_cast<uint64_t>(500000.0f / freqHz);
        if (newHalfPeriod < 250) newHalfPeriod = 250; // Max 2,000 Hz

        if (_kmhTimerRunning) {
            esp_timer_stop(_kmhTimer);
        }
        s_speedoKmhActive = true;
        s_speedoKmhLevel = 1;
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::SPEEDO_KMH), 1);
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 1);
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::SIG_CKP), 1);
        esp_timer_start_periodic(_kmhTimer, newHalfPeriod);
        _kmhTimerRunning = true;
    } else {
        if (_kmhTimerRunning && _kmhTimer) {
            esp_timer_stop(_kmhTimer);
            _kmhTimerRunning = false;
        }
        s_speedoKmhActive = false;
        s_speedoKmhLevel = 0;
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::SPEEDO_KMH), 0);
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 0);
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::SIG_CKP), 0);
    }
}

void SpeedoDriver::_setRpmFrequency(float freqHz) {
    if (freqHz > 0.2f && _rpmTimer) {
        uint64_t newHalfPeriod = static_cast<uint64_t>(500000.0f / freqHz);
        if (newHalfPeriod < 250) newHalfPeriod = 250; // Max 2,000 Hz

        if (_rpmTimerRunning) {
            esp_timer_stop(_rpmTimer);
        }
        s_speedoRpmActive = true;
        s_speedoRpmLevel = 1;
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::SPEEDO_RPM), 1);
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 1);
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::SIG_CMP), 1);
        esp_timer_start_periodic(_rpmTimer, newHalfPeriod);
        _rpmTimerRunning = true;
    } else {
        if (_rpmTimerRunning && _rpmTimer) {
            esp_timer_stop(_rpmTimer);
            _rpmTimerRunning = false;
        }
        s_speedoRpmActive = false;
        s_speedoRpmLevel = 0;
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::SPEEDO_RPM), 0);
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 0);
        gpio_set_level(static_cast<gpio_num_t>(PinConfig::SIG_CMP), 0);
    }
}

void SpeedoDriver::updateOutputs(const EcuEngine::SpeedoConfig& config, const EcuEngine::SpeedoRuntimeState& state) {
    if (!_initialized) init();

    if (!state.isRunning) {
        if (_lastRunning) {
            stop();
            _lastRunning = false;
        }
        return;
    }
    _lastRunning = true;

    // 1. Output KM/H Frequency (Independent esp_timer)
    float targetKmhHz = config.speedoEnableKmh ? state.hzKmh : 0.0f;
    if (targetKmhHz != _lastHzKmh) {
        _setKmhFrequency(targetKmhHz);
        _lastHzKmh = targetKmhHz;
    }

    // 2. Output RPM Frequency (Independent esp_timer)
    float targetRpmHz = config.speedoEnableRpm ? state.hzRpm : 0.0f;
    if (targetRpmHz != _lastHzRpm) {
        _setRpmFrequency(targetRpmHz);
        _lastHzRpm = targetRpmHz;
    }

    // 3. Output Temp (PWM or DAC)
    bool isTempDac = (config.dacRouting == EcuEngine::SpeedoDacRouting::DualMcp4725 ||
                      config.dacRouting == EcuEngine::SpeedoDacRouting::SingleDacTemp);
    if (isTempDac && _dacTempFound) {
        if (state.voltTemp != _lastDacTempVolt) {
            _writeDac(MCP4725_ADDR_TEMP, config.speedoEnableTemp ? state.voltTemp : 0.0f);
            _lastDacTempVolt = state.voltTemp;
        }
    } else {
        if (state.dutyTemp != _lastDutyTemp) {
            uint32_t duty8 = config.speedoEnableTemp ? static_cast<uint32_t>((state.dutyTemp / 100.0f) * 255.0f) : 0;
            if (duty8 > 255) duty8 = 255;
            ledcWrite(LEDC_CH_TEMP, duty8);
            _lastDutyTemp = state.dutyTemp;
        }
    }

    // 4. Output Fuel (PWM or DAC)
    bool isFuelDac = (config.dacRouting == EcuEngine::SpeedoDacRouting::DualMcp4725 ||
                      config.dacRouting == EcuEngine::SpeedoDacRouting::SingleDacFuel);
    if (isFuelDac && _dacFuelFound) {
        if (state.voltFuel != _lastDacFuelVolt) {
            _writeDac(MCP4725_ADDR_FUEL, config.speedoEnableFuel ? state.voltFuel : 0.0f);
            _lastDacFuelVolt = state.voltFuel;
        }
    } else {
        if (state.dutyFuel != _lastDutyFuel) {
            uint32_t duty8 = config.speedoEnableFuel ? static_cast<uint32_t>((state.dutyFuel / 100.0f) * 255.0f) : 0;
            if (duty8 > 255) duty8 = 255;
            ledcWrite(LEDC_CH_FUEL, duty8);
            _lastDutyFuel = state.dutyFuel;
        }
    }
}

void SpeedoDriver::stop() {
    _setKmhFrequency(0.0f);
    _setRpmFrequency(0.0f);

    ledcWrite(LEDC_CH_TEMP, 0);
    ledcWrite(LEDC_CH_FUEL, 0);

    gpio_set_level(static_cast<gpio_num_t>(PinConfig::SPEEDO_KMH), 0);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::SPEEDO_RPM), 0);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_VSS), 0);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::EPS_RPM), 0);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::SIG_CKP), 0);
    gpio_set_level(static_cast<gpio_num_t>(PinConfig::SIG_CMP), 0);

    if (_dacFuelFound) _writeDac(MCP4725_ADDR_FUEL, 0.0f);
    if (_dacTempFound) _writeDac(MCP4725_ADDR_TEMP, 0.0f);

    _lastHzKmh = -1.0f;
    _lastHzRpm = -1.0f;
    _lastDutyTemp = -1.0f;
    _lastDutyFuel = -1.0f;
    _lastDacFuelVolt = -1.0f;
    _lastDacTempVolt = -1.0f;
    _lastRunning = false;
}

} // namespace EcuHal
