#include "parametric_pattern.h"
#include "timing_math.h"
#include <algorithm>

namespace EcuEngine {

float ParametricWheel::getPitchAngleDeg() const {
    if (totalTeeth == 0) return 0.0f;
    return 360.0f / static_cast<float>(totalTeeth);
}

uint16_t ParametricWheel::getActiveTeethCount() const {
    if (missingTeeth >= totalTeeth) return 0;
    return totalTeeth - missingTeeth;
}

bool ParametricWheel::isValid() const {
    if (totalTeeth < 1 || totalTeeth > 360) return false;
    if (missingTeeth >= totalTeeth) return false;
    if (missingPosition >= totalTeeth) return false;
    if (dutyCycle < 0.05f || dutyCycle > 0.95f) return false;
    return true;
}

CamEventTable::CamEventTable() {
    clear();
}

bool CamEventTable::addEvent(float angleDeg, bool levelHigh) {
    if (_eventCount >= MAX_CMP_EVENTS || angleDeg < 0.0f || angleDeg > 720.0f) {
        return false;
    }
    _events[_eventCount].angleDeg = angleDeg;
    _events[_eventCount].levelHigh = levelHigh;
    _eventCount++;
    return true;
}

void CamEventTable::clear() {
    _eventCount = 0;
    for (size_t i = 0; i < MAX_CMP_EVENTS; ++i) {
        _events[i] = CmpEvent(0.0f, false);
    }
}

uint8_t CamEventTable::getEventCount() const {
    return _eventCount;
}

const CmpEvent* CamEventTable::getEvents() const {
    return _events;
}

bool CamEventTable::validate() const {
    if (_eventCount == 0) return true;
    for (uint8_t i = 1; i < _eventCount; ++i) {
        if (_events[i].angleDeg <= _events[i - 1].angleDeg) {
            return false;
        }
    }
    return true;
}

ParametricEngine::ParametricEngine() {}

size_t ParametricEngine::generateCkpCycle(const ParametricWheel& wheel, 
                                          uint32_t rpm, 
                                          PulseSegment* outSegments, 
                                          size_t maxSegments) {
    if (!wheel.isValid() || rpm == 0 || outSegments == nullptr) {
        return 0;
    }

    size_t totalCycleTeeth = static_cast<size_t>(wheel.totalTeeth) * 2;
    if (totalCycleTeeth > maxSegments) {
        return 0;
    }

    uint32_t cycleTotalUs = TimingMath::calculateCyclePeriodUs(rpm);

    size_t outIdx = 0;
    for (size_t t = 0; t < totalCycleTeeth; ++t) {
        uint32_t tStartUs = (uint32_t)(((uint64_t)t * cycleTotalUs) / totalCycleTeeth);
        uint32_t tEndUs   = (uint32_t)(((uint64_t)(t + 1) * cycleTotalUs) / totalCycleTeeth);
        uint32_t toothPeriodUs = tEndUs - tStartUs;
        uint32_t highUs = static_cast<uint32_t>(toothPeriodUs * wheel.dutyCycle);
        uint32_t lowUs  = toothPeriodUs - highUs;

        uint16_t toothInRev = t % wheel.totalTeeth;
        bool isMissing = (toothInRev >= wheel.missingPosition && 
                          toothInRev < (wheel.missingPosition + wheel.missingTeeth));

        if (isMissing) {
            outSegments[outIdx].duration0Us = toothPeriodUs;
            outSegments[outIdx].level0     = wheel.inverted ? 1 : 0;
            outSegments[outIdx].duration1Us = 0;
            outSegments[outIdx].level1     = wheel.inverted ? 1 : 0;
        } else {
            outSegments[outIdx].duration0Us = highUs;
            outSegments[outIdx].level0     = wheel.inverted ? 0 : 1;
            outSegments[outIdx].duration1Us = lowUs;
            outSegments[outIdx].level1     = wheel.inverted ? 1 : 0;
        }
        outIdx++;
    }

    return outIdx;
}

size_t ParametricEngine::generateCmpCycle(const CamEventTable& cam, 
                                          uint32_t rpm, 
                                          PulseSegment* outSegments, 
                                          size_t maxSegments) {
    uint8_t eventCount = cam.getEventCount();
    if (eventCount == 0 || rpm == 0 || outSegments == nullptr || !cam.validate()) {
        return 0;
    }

    uint32_t cycleTotalUs = TimingMath::calculateCyclePeriodUs(rpm);
    const CmpEvent* events = cam.getEvents();

    size_t outIdx = 0;
    for (uint8_t i = 0; i < eventCount; ++i) {
        if (outIdx >= maxSegments) break;

        float currentAngle = events[i].angleDeg;
        float nextAngle    = (i + 1 < eventCount) ? events[i + 1].angleDeg : 720.0f;

        uint32_t startUs = TimingMath::angleToTimeUs(currentAngle, rpm);
        uint32_t endUs   = TimingMath::angleToTimeUs(nextAngle, rpm);

        if (endUs > startUs) {
            outSegments[outIdx].duration0Us = endUs - startUs;
            outSegments[outIdx].level0     = events[i].levelHigh ? 1 : 0;
            outSegments[outIdx].duration1Us = 0;
            outSegments[outIdx].level1     = events[i].levelHigh ? 1 : 0;
            outIdx++;
        }
    }

    return outIdx;
}

} // namespace EcuEngine
