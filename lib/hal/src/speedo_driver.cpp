#include "speedo_driver.h"
#include <Arduino.h>
#include <Wire.h>
#include "pin_config.h"

namespace EcuHal {

SpeedoDriver::SpeedoDriver() = default;

void SpeedoDriver::detectDacs(bool& fuelFound, bool& tempFound) {
    Wire.setTimeOut(2);
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

    Wire.setTimeOut(2);
    Wire.beginTransmission(addr);
    Wire.write(static_cast<uint8_t>((dacValue >> 8) & 0x0F));
    Wire.write(static_cast<uint8_t>(dacValue & 0xFF));
    Wire.endTransmission();
}

void SpeedoDriver::init() {
    if (_initialized) return;

    Wire.setTimeOut(2);
    bool f, t;
    detectDacs(f, t);

    // 1. PWM Temp & Fuel channels (Timer 2, 5 kHz default, 8-bit)
    ledcSetup(LEDC_CH_TEMP, 5000, 8);
    ledcAttachPin(PinConfig::SPEEDO_TEMP, LEDC_CH_TEMP);
    ledcWrite(LEDC_CH_TEMP, 0);

    ledcSetup(LEDC_CH_FUEL, 5000, 8);
    ledcAttachPin(PinConfig::SPEEDO_FUEL, LEDC_CH_FUEL);
    ledcWrite(LEDC_CH_FUEL, 0);

    // 2. RPM & KMH Frequency Pulse Channels (Timer 0 & 1, 8-bit, 100 Hz default)
    ledcSetup(LEDC_CH_RPM, 100, 8);
    ledcAttachPin(PinConfig::SPEEDO_RPM, LEDC_CH_RPM);
    ledcWrite(LEDC_CH_RPM, 0);

    ledcSetup(LEDC_CH_KMH, 100, 8);
    ledcAttachPin(PinConfig::SPEEDO_KMH, LEDC_CH_KMH);
    ledcWrite(LEDC_CH_KMH, 0);

    if (_dacFuelFound) _writeDac(MCP4725_ADDR_FUEL, 0.0f);
    if (_dacTempFound) _writeDac(MCP4725_ADDR_TEMP, 0.0f);

    _initialized = true;
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

    // 1. Output KM/H Frequency
    if (state.hzKmh != _lastHzKmh) {
        if (config.speedoEnableKmh && state.hzKmh > 0.5f) {
            ledcWriteTone(LEDC_CH_KMH, static_cast<double>(state.hzKmh));
        } else {
            ledcWrite(LEDC_CH_KMH, 0);
        }
        _lastHzKmh = state.hzKmh;
    }

    // 2. Output RPM Frequency
    if (state.hzRpm != _lastHzRpm) {
        if (config.speedoEnableRpm && state.hzRpm > 0.5f) {
            ledcWriteTone(LEDC_CH_RPM, static_cast<double>(state.hzRpm));
        } else {
            ledcWrite(LEDC_CH_RPM, 0);
        }
        _lastHzRpm = state.hzRpm;
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
