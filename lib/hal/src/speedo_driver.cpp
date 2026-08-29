#include "speedo_driver.h"
#include <Arduino.h>
#include <Wire.h>
#include "pin_config.h"

namespace EcuHal {

SpeedoDriver::SpeedoDriver() = default;

void SpeedoDriver::detectDacs(bool& fuelFound, bool& tempFound) {
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

    Wire.beginTransmission(addr);
    Wire.write(static_cast<uint8_t>((dacValue >> 8) & 0x0F));
    Wire.write(static_cast<uint8_t>(dacValue & 0xFF));
    Wire.endTransmission();
}

void SpeedoDriver::init() {
    if (_initialized) return;

    Wire.begin(PinConfig::I2C_SDA, PinConfig::I2C_SCL);
    bool f, t;
    detectDacs(f, t);

    // 1. PWM Temp & Fuel channels (Timer 2, 5 kHz default, 8-bit)
    ledcSetup(LEDC_CH_TEMP, 5000, 8);
    ledcAttachPin(PinConfig::SPEEDO_TEMP, LEDC_CH_TEMP);
    ledcWrite(LEDC_CH_TEMP, 0);

    ledcSetup(LEDC_CH_FUEL, 5000, 8);
    ledcAttachPin(PinConfig::SPEEDO_FUEL, LEDC_CH_FUEL);
    ledcWrite(LEDC_CH_FUEL, 0);

    // 2. RPM & KMH Frequency Pulse Channels (Timer 0 & 1, 10-bit)
    ledcSetup(LEDC_CH_RPM, 50, 10);
    ledcAttachPin(PinConfig::SPEEDO_RPM, LEDC_CH_RPM);
    ledcWrite(LEDC_CH_RPM, 0);

    ledcSetup(LEDC_CH_KMH, 50, 10);
    ledcAttachPin(PinConfig::SPEEDO_KMH, LEDC_CH_KMH);
    ledcWrite(LEDC_CH_KMH, 0);

    if (_dacFuelFound) _writeDac(MCP4725_ADDR_FUEL, 0.0f);
    if (_dacTempFound) _writeDac(MCP4725_ADDR_TEMP, 0.0f);

    _initialized = true;
}

void SpeedoDriver::updateOutputs(const EcuEngine::SpeedoConfig& config, const EcuEngine::SpeedoRuntimeState& state) {
    if (!_initialized) init();

    uint32_t now = millis();
    if (!state.isRunning && (now - _lastDacPollMs > 3000)) {
        bool f, t;
        detectDacs(f, t);
        _lastDacPollMs = now;
    }

    if (!state.isRunning) {
        if (_lastRunning) {
            stop();
            _lastRunning = false;
        }
        return;
    }
    _lastRunning = true;

    // 1. Reconfigure PWM Frequency if changed
    if (config.speedoPwmFreqHz != _lastPwmFreq && config.speedoPwmFreqHz >= 10 && config.speedoPwmFreqHz <= 20000) {
        ledcSetup(LEDC_CH_TEMP, config.speedoPwmFreqHz, 8);
        ledcSetup(LEDC_CH_FUEL, config.speedoPwmFreqHz, 8);
        _lastPwmFreq = config.speedoPwmFreqHz;
        _lastDutyTemp = -1.0f;
        _lastDutyFuel = -1.0f;
    }

    // 2. Output KM/H Frequency
    if (state.hzKmh != _lastHzKmh) {
        if (state.hzKmh > 0.5f && config.speedoEnableKmh) {
            ledcWriteTone(LEDC_CH_KMH, static_cast<double>(state.hzKmh));
        } else {
            ledcWrite(LEDC_CH_KMH, 0);
        }
        _lastHzKmh = state.hzKmh;
    }

    // 3. Output RPM Frequency
    if (state.hzRpm != _lastHzRpm) {
        if (state.hzRpm > 0.5f && config.speedoEnableRpm) {
            ledcWriteTone(LEDC_CH_RPM, static_cast<double>(state.hzRpm));
        } else {
            ledcWrite(LEDC_CH_RPM, 0);
        }
        _lastHzRpm = state.hzRpm;
    }

    // 4. Output Temp (DAC / PWM)
    bool isTempDac = (config.dacRouting == EcuEngine::SpeedoDacRouting::DualMcp4725 ||
                      config.dacRouting == EcuEngine::SpeedoDacRouting::SingleDacTemp);
    if (isTempDac && _dacTempFound) {
        if (state.voltTemp != _lastDacTempVolt) {
            _writeDac(MCP4725_ADDR_TEMP, config.speedoEnableTemp ? state.voltTemp : 0.0f);
            _lastDacTempVolt = state.voltTemp;
        }
        if (_lastDutyTemp != 0.0f) {
            ledcWrite(LEDC_CH_TEMP, 0);
            _lastDutyTemp = 0.0f;
        }
    } else {
        if (state.dutyTemp != _lastDutyTemp) {
            uint32_t dutyVal = config.speedoEnableTemp ? static_cast<uint32_t>((state.dutyTemp / 100.0f) * 255.0f) : 0;
            if (dutyVal > 255) dutyVal = 255;
            ledcWrite(LEDC_CH_TEMP, dutyVal);
            _lastDutyTemp = state.dutyTemp;
        }
    }

    // 5. Output Fuel (DAC / PWM)
    bool isFuelDac = (config.dacRouting == EcuEngine::SpeedoDacRouting::DualMcp4725 ||
                      config.dacRouting == EcuEngine::SpeedoDacRouting::SingleDacFuel);
    if (isFuelDac && _dacFuelFound) {
        if (state.voltFuel != _lastDacFuelVolt) {
            _writeDac(MCP4725_ADDR_FUEL, config.speedoEnableFuel ? state.voltFuel : 0.0f);
            _lastDacFuelVolt = state.voltFuel;
        }
        if (_lastDutyFuel != 0.0f) {
            ledcWrite(LEDC_CH_FUEL, 0);
            _lastDutyFuel = 0.0f;
        }
    } else {
        if (state.dutyFuel != _lastDutyFuel) {
            uint32_t dutyVal = config.speedoEnableFuel ? static_cast<uint32_t>((state.dutyFuel / 100.0f) * 255.0f) : 0;
            if (dutyVal > 255) dutyVal = 255;
            ledcWrite(LEDC_CH_FUEL, dutyVal);
            _lastDutyFuel = state.dutyFuel;
        }
    }
}

void SpeedoDriver::stop() {
    if (!_initialized) return;
    ledcWrite(LEDC_CH_KMH, 0);
    ledcWrite(LEDC_CH_RPM, 0);
    ledcWrite(LEDC_CH_TEMP, 0);
    ledcWrite(LEDC_CH_FUEL, 0);
    if (_dacFuelFound) _writeDac(MCP4725_ADDR_FUEL, 0.0f);
    if (_dacTempFound) _writeDac(MCP4725_ADDR_TEMP, 0.0f);
    _lastHzKmh = -1.0f;
    _lastHzRpm = -1.0f;
    _lastDutyTemp = -1.0f;
    _lastDutyFuel = -1.0f;
    _lastDacFuelVolt = -1.0f;
    _lastDacTempVolt = -1.0f;
}

} // namespace EcuHal
