#pragma once
#include <stdint.h>

namespace EcuHal {

class Mcp23017Driver {
public:
    static constexpr uint8_t DEFAULT_I2C_ADDR = 0x20;

    static constexpr uint8_t PIN_STA  = 0; ///< GPA0: Output STA Crank
    static constexpr uint8_t PIN_CHG  = 1; ///< GPA1: Output CHG Alternator
    static constexpr uint8_t PIN_IGSW = 2; ///< GPA2: Output Relay IGSW Power Cut
    static constexpr uint8_t PIN_MREL = 3; ///< GPA3: Input Monitor Feedback M-REL ECU

    Mcp23017Driver();

    bool init(uint8_t addr = DEFAULT_I2C_ADDR);
    bool isFound() const { return _isFound; }

    void setSta(bool active);
    void setChg(bool lampOn);
    void setIgsw(bool on);
    bool readMrel();

    void writePortA(uint8_t value);
    void writePortB(uint8_t value);
    uint8_t getPortAState() const { return _portAState; }

private:
    uint8_t _i2cAddr{DEFAULT_I2C_ADDR};
    bool    _isFound{false};
    uint8_t _portAState{0x00};
    uint8_t _portBState{0x00};

    static constexpr uint8_t REG_IODIRA = 0x00;
    static constexpr uint8_t REG_IODIRB = 0x01;
    static constexpr uint8_t REG_OLATA  = 0x14;
    static constexpr uint8_t REG_OLATB  = 0x15;
    static constexpr uint8_t REG_GPIOA  = 0x12;
    static constexpr uint8_t REG_GPIOB  = 0x13;

    void _writeRegister(uint8_t reg, uint8_t val);
    uint8_t _readRegister(uint8_t reg);
};

} // namespace EcuHal
