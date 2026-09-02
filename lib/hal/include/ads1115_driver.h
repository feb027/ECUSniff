#pragma once
#include <stdint.h>
#include <Wire.h>

namespace EcuHal {

/**
 * @brief Driver ringan I2C untuk ADS1115 (16-bit) / ADS1015 (12-bit) ADC.
 * Default I2C address: 0x48 (ADDR pin ke GND).
 */
class Ads1115Driver {
public:
    static constexpr uint8_t DEFAULT_I2C_ADDR = 0x48;

    Ads1115Driver() = default;

    /**
     * @brief Inisialisasi komunikasi I2C dengan ADS1115.
     * @param i2cAddr Alamat I2C ADS1115 (default 0x48).
     * @return true jika chip terdeteksi, false jika tidak ada respon.
     */
    bool init(uint8_t i2cAddr = DEFAULT_I2C_ADDR);

    /**
     * @brief Cek apakah chip ADS1115 terdeteksi di bus I2C.
     */
    bool isFound() const { return _isFound; }

    /**
     * @brief Membaca nilai raw ADC single-ended pada Channel A0.
     * Konfigurasi: Single-ended AIN0 ke GND, FSR ±4.096V, 860 SPS.
     * @return int16_t Nilai raw ADC (0..32767 untuk tegangan positif).
     */
    int16_t readRawA0();

    /**
     * @brief Membaca tegangan pada Channel A0 dalam satuan Volt.
     * @return float Tegangan (0.0V - 3.3V).
     */
    float readVoltageA0();

private:
    uint8_t _i2cAddr{DEFAULT_I2C_ADDR};
    bool    _isFound{false};

    static constexpr uint8_t REG_CONVERSION = 0x00;
    static constexpr uint8_t REG_CONFIG     = 0x01;
};

} // namespace EcuHal
