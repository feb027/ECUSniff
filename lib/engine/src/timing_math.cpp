#include "timing_math.h"

namespace EcuEngine {

uint32_t TimingMath::calculateRevPeriodUs(uint32_t rpm) {
    if (rpm == 0) {
        return 0;
    }
    return static_cast<uint32_t>(60000000ULL / rpm);
}

uint32_t TimingMath::calculateCyclePeriodUs(uint32_t rpm) {
    if (rpm == 0) {
        return 0;
    }
    return static_cast<uint32_t>(120000000ULL / rpm);
}

float TimingMath::calculateUsPerDegree(uint32_t rpm) {
    if (rpm == 0) {
        return 0.0f;
    }
    return 166666.6667f / static_cast<float>(rpm);
}

uint32_t TimingMath::angleToTimeUs(float angleDeg, uint32_t rpm) {
    if (rpm == 0 || angleDeg <= 0.0f) {
        return 0;
    }
    float usPerDeg = calculateUsPerDegree(rpm);
    return static_cast<uint32_t>(angleDeg * usPerDeg);
}

float TimingMath::calculateToothPitchAngle(uint16_t totalTeeth) {
    if (totalTeeth == 0) {
        return 0.0f;
    }
    return 360.0f / static_cast<float>(totalTeeth);
}

} // namespace EcuEngine
