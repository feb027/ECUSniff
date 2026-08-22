#pragma once
#include <stdint.h>
#include <stddef.h>
#include "engine_types.h"

namespace EcuEngine {

/**
 * @brief Definisi Roda Parametrik CKP (Missing-Tooth Wheel)
 */
struct ParametricWheel {
    uint16_t totalTeeth{36};      ///< Jumlah total gigi teoritis dalam 360 derajat
    uint8_t  missingTeeth{1};     ///< Jumlah gigi yang dihilangkan (gap)
    uint8_t  missingPosition{0};  ///< Indeks gigi awal gap (0 sampai totalTeeth - 1)
    float    dutyCycle{0.5f};      ///< Rasio High / Tooth Period (0.1 - 0.9)
    bool     inverted{false};      ///< Polaritas pulsa terbalik

    float getPitchAngleDeg() const;
    uint16_t getActiveTeethCount() const;
    bool isValid() const;
};

/**
 * @brief Tabel Event Camshaft (CMP/CMP2) sepanjang siklus 4-tak (0 - 720 derajat)
 */
class CamEventTable {
public:
    CamEventTable();
    bool addEvent(float angleDeg, bool levelHigh);
    void clear();
    uint8_t getEventCount() const;
    const CmpEvent* getEvents() const;
    bool validate() const;

private:
    CmpEvent _events[MAX_CMP_EVENTS];
    uint8_t  _eventCount{0};
};

/**
 * @brief Representasi segmen pulsa waktu untuk 1 siklus penuh mesin (720 derajat)
 */
struct PulseSegment {
    uint32_t duration0Us{0};
    uint8_t  level0{0};
    uint32_t duration1Us{0};
    uint8_t  level1{0};
};

constexpr size_t MAX_CYCLE_PULSES = 256;

class ParametricEngine {
public:
    ParametricEngine();

    /**
     * @brief Mengompilasi pulsa CKP untuk siklus 720 derajat (2 putaran) pada RPM tertentu.
     * @param wheel Konfigurasi roda gigi.
     * @param rpm Kecepatan putaran mesin.
     * @param outSegments Buffer output array segmen pulsa.
     * @param maxSegments Kapasitas maksimum buffer output.
     * @return Jumlah segmen pulsa yang dihasilkan.
     */
    static size_t generateCkpCycle(const ParametricWheel& wheel, 
                                  uint32_t rpm, 
                                  PulseSegment* outSegments, 
                                  size_t maxSegments);

    /**
     * @brief Mengompilasi pulsa CMP untuk siklus 720 derajat pada RPM tertentu.
     * @param cam Konfigurasi tabel event cam.
     * @param rpm Kecepatan putaran mesin.
     * @param outSegments Buffer output array segmen pulsa.
     * @param maxSegments Kapasitas maksimum buffer output.
     * @return Jumlah segmen pulsa yang dihasilkan.
     */
    static size_t generateCmpCycle(const CamEventTable& cam, 
                                  uint32_t rpm, 
                                  PulseSegment* outSegments, 
                                  size_t maxSegments);
};

} // namespace EcuEngine
