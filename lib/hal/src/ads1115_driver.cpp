#include "ads1115_driver.h"
#include <Arduino.h>

namespace EcuHal {

bool Ads1115Driver::init(uint8_t i2cAddr) {
    _i2cAddr = i2cAddr;
    Wire.setTimeOut(50);

    Wire.beginTransmission(_i2cAddr);
    if (Wire.endTransmission() != 0) {
        _isFound = false;
        return false;
    }

    _isFound = true;
    _currentChannel = 0;
    _triggerConversion(0); // Start first conversion on AIN0
    return true;
}

void Ads1115Driver::_triggerConversion(uint8_t channel) {
    if (!_isFound) return;

    // MUX:
    // A0 (AIN0 vs GND) = 0x4000
    // A1 (AIN1 vs GND) = 0x5000
    // A2 (AIN2 vs GND) = 0x6000
    uint16_t mux = 0x4000;
    if (channel == 1) mux = 0x5000;
    else if (channel == 2) mux = 0x6000;

    // Config: OS=1 (start conversion), MUX=mux, PGA=+/-4.096V (0x0200),
    // MODE=Single-shot (0x0100), DR=860 SPS (0x00E0), COMP_QUE=Disable (0x0003)
    uint16_t config = 0x8000 | mux | 0x0200 | 0x0100 | 0x00E0 | 0x0003;

    Wire.setTimeOut(50);
    Wire.beginTransmission(_i2cAddr);
    Wire.write(REG_CONFIG);
    Wire.write((uint8_t)(config >> 8));
    Wire.write((uint8_t)(config & 0xFF));
    if (Wire.endTransmission() != 0) {
        _isFound = false;
        return;
    }

    _currentChannel = channel;
    _lastTriggerMs = millis();
}

int16_t Ads1115Driver::_readConversion() {
    if (!_isFound) return 0;

    Wire.setTimeOut(50);
    Wire.beginTransmission(_i2cAddr);
    Wire.write(REG_CONVERSION);
    if (Wire.endTransmission() != 0) {
        _isFound = false;
        return 0;
    }

    size_t len = Wire.requestFrom((int)_i2cAddr, 2);
    if (len >= 2 && Wire.available() >= 2) {
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        int16_t raw = (int16_t)((msb << 8) | lsb);
        if (raw < 0) raw = 0; // Single-ended tegangan positif
        return raw;
    }

    _isFound = false;
    return 0;
}

void Ads1115Driver::update() {
    if (!_isFound) {
        static uint32_t lastRetry = 0;
        uint32_t now = millis();
        if (now - lastRetry < 3000) return;
        lastRetry = now;
        if (!init(_i2cAddr)) return;
    }

    int16_t raw = _readConversion();
    float v = (float)raw * 0.000125f; // FSR 4.096V -> 1 LSB = 0.125 mV

    if (_currentChannel == 0) {
        if (v < 0.0f) v = 0.0f;
        if (v > 3.3f) v = 3.3f;
        _voltageA0 = v;
        _triggerConversion(1); // Next: A1 (TRQ1)
    } else if (_currentChannel == 1) {
        _rawVoltageA1 = v;
        _triggerConversion(2); // Next: A2 (TRQ2)
    } else {
        _rawVoltageA2 = v;
        _triggerConversion(0); // Next: A0 (POT)
    }
}

float Ads1115Driver::getCalibratedVoltageA1() const {
    float v = (_rawVoltageA1 * _cal.trq1Scale) + _cal.trq1Offset;
    if (v < 0.0f) v = 0.0f;
    if (v > 6.0f) v = 6.0f;
    return v;
}

float Ads1115Driver::getCalibratedVoltageA2() const {
    float v = (_rawVoltageA2 * _cal.trq2Scale) + _cal.trq2Offset;
    if (v < 0.0f) v = 0.0f;
    if (v > 6.0f) v = 6.0f;
    return v;
}

} // namespace EcuHal
