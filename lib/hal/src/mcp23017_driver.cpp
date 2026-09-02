#include "mcp23017_driver.h"
#include <Arduino.h>
#include <Wire.h>

namespace EcuHal {

Mcp23017Driver::Mcp23017Driver() = default;

bool Mcp23017Driver::init(uint8_t addr) {
    _i2cAddr = addr;
    
    // Uji koneksi I2C
    Wire.setTimeOut(5);
    Wire.beginTransmission(_i2cAddr);
    if (Wire.endTransmission() != 0) {
        _isFound = false;
        return false;
    }
    _isFound = true;

    // Set pin Port A:
    // GPA0 (STA) = Output (0)
    // GPA1 (CHG) = Output (0)
    // GPA2 (IGSW) = Output (0)
    // GPA3 (MREL) = Input (1)
    // GPA4-GPA7 = Output (0)
    // Register IODIRA: 0b00001000 = 0x08
    _writeRegister(REG_IODIRA, 0x08);
    _writeRegister(REG_IODIRB, 0x00); // Port B semua output

    // Kondisi awal aman:
    // STA (GPA0) = LOW (0V / Tidak Crank)
    // CHG (GPA1) = HIGH (Tarik ke GND / Lampu Aki Menyala saat kontak ON)
    // IGSW (GPA2) = LOW (0V / Kontak OFF awal)
    _portAState = (1 << PIN_CHG); 
    _portBState = 0x00;

    _writeRegister(REG_GPIOA, _portAState);
    _writeRegister(REG_GPIOB, _portBState);

    return true;
}

void Mcp23017Driver::_writeRegister(uint8_t reg, uint8_t val) {
    if (!_isFound) return;
    Wire.setTimeOut(5);
    Wire.beginTransmission(_i2cAddr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

uint8_t Mcp23017Driver::_readRegister(uint8_t reg) {
    if (!_isFound) return 0;
    Wire.setTimeOut(5);
    Wire.beginTransmission(_i2cAddr);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) return 0;
    
    Wire.requestFrom((int)_i2cAddr, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

void Mcp23017Driver::setSta(bool active) {
    if (active) {
        _portAState |= (1 << PIN_STA);
    } else {
        _portAState &= ~(1 << PIN_STA);
    }
    _writeRegister(REG_GPIOA, _portAState);
}

void Mcp23017Driver::setChg(bool lampOn) {
    if (lampOn) {
        _portAState |= (1 << PIN_CHG);
    } else {
        _portAState &= ~(1 << PIN_CHG);
    }
    _writeRegister(REG_GPIOA, _portAState);
}

void Mcp23017Driver::setIgsw(bool on) {
    if (on) {
        _portAState |= (1 << PIN_IGSW);
    } else {
        _portAState &= ~(1 << PIN_IGSW);
    }
    _writeRegister(REG_GPIOA, _portAState);
}

bool Mcp23017Driver::readMrel() {
    uint8_t val = _readRegister(REG_GPIOA);
    return (val & (1 << PIN_MREL)) != 0;
}

void Mcp23017Driver::writePortA(uint8_t value) {
    _portAState = value;
    _writeRegister(REG_GPIOA, _portAState);
}

void Mcp23017Driver::writePortB(uint8_t value) {
    _portBState = value;
    _writeRegister(REG_GPIOB, _portBState);
}

} // namespace EcuHal
