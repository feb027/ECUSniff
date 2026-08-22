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
    if (totalTeeth < 4 || totalTeeth > 120) return false;
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

    size_t totalTeeth = static_cast<size_t>(wheel.totalTeeth); // 1 Crank Revolution (360 deg)
    if (totalTeeth > maxSegments) {
        return 0;
    }

    uint32_t revPeriodUs = TimingMath::calculateRevPeriodUs(rpm);
    uint32_t slotUs = revPeriodUs / wheel.totalTeeth;
    uint32_t highUs = static_cast<uint32_t>(slotUs * wheel.dutyCycle);
    uint32_t lowUs = slotUs - highUs;

    size_t outIdx = 0;
    for (uint16_t t = 0; t < wheel.totalTeeth; ++t) {
        PulseSegment seg{};
        bool isMissing = (t >= wheel.missingPosition && 
                          t < (wheel.missingPosition + wheel.missingTeeth));

        if (isMissing) {
            // Missing tooth gap: level 0 sepanjang slot waktu gigi
            seg.duration0Us = highUs;
            seg.level0 = 0;
            seg.duration1Us = lowUs;
            seg.level1 = 0;
        } else {
            // Normal tooth: Square wave
            seg.duration0Us = highUs;
            seg.level0 = wheel.inverted ? 0 : 1;
            seg.duration1Us = lowUs;
            seg.level1 = wheel.inverted ? 1 : 0;
        }
        outSegments[outIdx++] = seg;
    }

    return outIdx;
}

size_t ParametricEngine::generateCmpCycle(const CamEventTable& cam, 
                                          uint32_t rpm, 
                                          PulseSegment* outSegments, 
                                          size_t maxSegments) {
    if (rpm == 0 || outSegments == nullptr || cam.getEventCount() == 0) {
        return 0;
    }

    uint32_t cycleTotalUs = TimingMath::calculateCyclePeriodUs(rpm);
    uint8_t count = cam.getEventCount();
    const CmpEvent* ev = cam.getEvents();

    size_t outIdx = 0;
    uint32_t lastTimeUs = 0;
    bool currentLevel = !ev[0].levelHigh;

    for (uint8_t i = 0; i < count && outIdx < maxSegments; ++i) {
        uint32_t eventTimeUs = TimingMath::angleToTimeUs(ev[i].angleDeg, rpm);
        if (eventTimeUs > lastTimeUs) {
            PulseSegment seg{};
            seg.duration0Us = eventTimeUs - lastTimeUs;
            seg.level0 = currentLevel ? 1 : 0;
            seg.duration1Us = 0;
            seg.level1 = 0;
            outSegments[outIdx++] = seg;
            lastTimeUs = eventTimeUs;
        }
        currentLevel = ev[i].levelHigh;
    }

    if (lastTimeUs < cycleTotalUs && outIdx < maxSegments) {
        PulseSegment seg{};
        seg.duration0Us = cycleTotalUs - lastTimeUs;
        seg.level0 = currentLevel ? 1 : 0;
        seg.duration1Us = 0;
        seg.level1 = 0;
        outSegments[outIdx++] = seg;
    }

    return outIdx;
}

} // namespace EcuEngine
