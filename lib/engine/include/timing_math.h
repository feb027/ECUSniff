#pragma once
#include <stdint.h>
#include "engine_types.h"

namespace EcuEngine {

class TimingMath {
public:
    /**
     * @brief Menghitung durasi satu putaran poros engkol (360 derajat) dalam mikrodetik (us).
     * @param rpm Kecepatan putaran mesin (RPM).
     * @return Waktu putaran dalam mikrodetik. Mengembalikan 0 jika rpm == 0.
     */
    static uint32_t calculateRevPeriodUs(uint32_t rpm);

    /**
     * @brief Menghitung durasi satu siklus 4-tak (720 derajat) dalam mikrodetik (us).
     * @param rpm Kecepatan putaran mesin (RPM).
     * @return Waktu 720 derajat dalam mikrodetik.
     */
    static uint32_t calculateCyclePeriodUs(uint32_t rpm);

    /**
     * @brief Menghitung durasi waktu per satu derajat rotasi mesin.
     * @param rpm Kecepatan putaran mesin (RPM).
     * @return Waktu mikrodetik per derajat.
     */
    static float calculateUsPerDegree(uint32_t rpm);

    /**
     * @brief Mengonversi sudut mesin (derajat) menjadi durasi waktu (us) relatif terhadap awal siklus.
     * @param angleDeg Sudut mesin (0 - 720 derajat).
     * @param rpm Kecepatan putaran mesin (RPM).
     * @return Offset waktu dalam mikrodetik.
     */
    static uint32_t angleToTimeUs(float angleDeg, uint32_t rpm);

    /**
     * @brief Menghitung sudut pitch antar gigi pada roda CKP.
     * @param totalTeeth Jumlah total gigi teoritis.
     * @return Derajat per gigi (e.g. 10.0 derajat untuk 36 gigi).
     */
    static float calculateToothPitchAngle(uint16_t totalTeeth);
};

} // namespace EcuEngine
