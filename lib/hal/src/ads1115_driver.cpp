#include "ads1115_driver.h"
#include <Arduino.h>

namespace EcuHal {

bool Ads1115Driver::init(uint8_t i2cAddr) {
    _i2cAddr = i2cAddr;
    Wire.setTimeOut(2);

    Wire.beginTransmission(_i2cAddr);
    if (Wire.endTransmission() != 0) {
        _isFound = false;
        return false;
    }

    // Konfigurasi ADS1115 Continuous Conversion Mode pada AIN0:
    // - MUX: AIN0 vs GND (0x4000)
    // - PGA: +/- 4.096V (0x0200) -> 1 LSB = 0.125 mV (Sangat cocok untuk rentang 0-3.3V)
    // - MODE: Continuous Conversion (0x0000)
    // - DR: 860 SPS (0x00E0)
    // - COMP_QUE: Disable comparator (0x0003)
    // Total Config = 0x42E3
    uint16_t config = 0x42E3;

    Wire.beginTransmission(_i2cAddr);
    Wire.write(REG_CONFIG);
    Wire.write((uint8_t)(config >> 8));
    Wire.write((uint8_t)(config & 0xFF));
    if (Wire.endTransmission() != 0) {
        _isFound = false;
        return false;
    }

    // Set pointer register ke Conversion Register (0x00)
    Wire.beginTransmission(_i2cAddr);
    Wire.write(REG_CONVERSION);
    if (Wire.endTransmission() != 0) {
        _isFound = false;
        return false;
    }

    _isFound = true;
    return true;
}

int16_t Ads1115Driver::readRawA0() {
    if (!_isFound) return 0;

    Wire.setTimeOut(2);
    size_t len = Wire.requestFrom((int)_i2cAddr, 2);
    if (len >= 2 && Wire.available() >= 2) {
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        int16_t raw = (int16_t)((msb << 8) | lsb);
        if (raw < 0) raw = 0; // Single-ended tegangan positif
        return raw;
    }

    // Jika request gagal / NACK, tandai offline agar tidak membebani loop FreeRTOS
    _isFound = false;
    return 0;
}

float Ads1115Driver::readVoltageA0() {
    if (!_isFound) {
        static uint32_t lastRetry = 0;
        uint32_t now = millis();
        if (now - lastRetry < 3000) {
            return 0.0f; // Jeda 3 detik sebelum retry agar CPU tetap lancar 60 FPS
        }
        lastRetry = now;
        if (!init(_i2cAddr)) {
            return 0.0f;
        }
    }

    int16_t raw = readRawA0();
    // FSR +/- 4.096V (15-bit single ended: 32767 = 4.096V -> 1 LSB = 0.000125V)
    float voltage = (float)raw * 0.000125f;
    if (voltage < 0.0f) voltage = 0.0f;
    if (voltage > 3.3f) voltage = 3.3f;
    return voltage;
}

} // namespace EcuHal
