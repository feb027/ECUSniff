#pragma once
#include <stdint.h>
#include <Wire.h>

namespace EcuHal {

struct AdsTrqCalibration {
    float trq1Scale{2.000f};   // Pengali rasio voltage divider (10k:10k = 2.0x)
    float trq1Offset{0.000f};  // Koreksi tegangan (Offset V)
    float trq2Scale{2.000f};
    float trq2Offset{0.000f};
};

/**
 * @brief Driver multi-channel I2C untuk ADS1115 (16-bit) ADC.
 * Mendukung pembacaan bergiliran (round-robin non-blocking) untuk:
 * - A0: Potensiometer RPM Analog
 * - A1: Sinyal Feedback Realtime TRQ1
 * - A2: Sinyal Feedback Realtime TRQ2
 */
class Ads1115Driver {
public:
    static constexpr uint8_t DEFAULT_I2C_ADDR = 0x48;

    Ads1115Driver() = default;

    bool init(uint8_t i2cAddr = DEFAULT_I2C_ADDR);
    bool isFound() const { return _isFound; }

    /**
     * @brief Update siklus sampling round-robin non-blocking (panggil berkala di loop sensor).
     */
    void update();

    // Nilai pembacaan tegangan
    float readVoltageA0() const { return _voltageA0; }
    float getRawVoltageA1() const { return _rawVoltageA1; }
    float getRawVoltageA2() const { return _rawVoltageA2; }
    float getCalibratedVoltageA1() const;
    float getCalibratedVoltageA2() const;

    // Pengaturan Kalibrasi
    void setCalibration(const AdsTrqCalibration& cal) { _cal = cal; }
    const AdsTrqCalibration& getCalibration() const { return _cal; }

    void setTrq1Scale(float scale) { _cal.trq1Scale = scale; }
    void setTrq1Offset(float offset) { _cal.trq1Offset = offset; }
    void setTrq2Scale(float scale) { _cal.trq2Scale = scale; }
    void setTrq2Offset(float offset) { _cal.trq2Offset = offset; }

private:
    uint8_t _i2cAddr{DEFAULT_I2C_ADDR};
    bool    _isFound{false};

    uint8_t  _currentChannel{0}; // 0: A0, 1: A1, 2: A2
    uint32_t _lastTriggerMs{0};

    float _voltageA0{0.0f};
    float _rawVoltageA1{0.0f};
    float _rawVoltageA2{0.0f};

    AdsTrqCalibration _cal;

    static constexpr uint8_t REG_CONVERSION = 0x00;
    static constexpr uint8_t REG_CONFIG     = 0x01;

    void _triggerConversion(uint8_t channel);
    int16_t _readConversion();
};

} // namespace EcuHal
