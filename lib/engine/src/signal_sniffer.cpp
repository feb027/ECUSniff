#include "signal_sniffer.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace EcuEngine {

SignalSniffer::SignalSniffer() {}

uint32_t SignalSniffer::_findMedian(uint32_t* arr, size_t n) {
    if (n == 0) return 0;
    for (size_t i = 0; i < n - 1; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if (arr[j] < arr[i]) {
                uint32_t tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
    return arr[n / 2];
}

void SignalSniffer::_matchVehicleProfile(SnifferResult& res) {
    uint16_t N = res.wheel.totalTeeth;
    uint8_t  M = res.wheel.missingTeeth;

    if (N == 36 && M == 1) {
        strncpy(res.matchedVehicle, "Ford / Honda / Universal 36-1", sizeof(res.matchedVehicle));
        res.matchConfidence = 99.8f;
    } else if (N == 36 && M == 2) {
        strncpy(res.matchedVehicle, "Toyota 1NZ/2NZ/1ZZ (36-2)", sizeof(res.matchedVehicle));
        res.matchConfidence = 99.6f;
    } else if (N == 60 && M == 2) {
        strncpy(res.matchedVehicle, "Bosch / VW / BMW (60-2)", sizeof(res.matchedVehicle));
        res.matchConfidence = 99.9f;
    } else if (N == 4 && M == 1) {
        strncpy(res.matchedVehicle, "Yamaha NMAX / Aerox (4-1)", sizeof(res.matchedVehicle));
        res.matchConfidence = 99.2f;
    } else if (N == 24 && M == 2) {
        strncpy(res.matchedVehicle, "Mazda Miata / BP (24-2)", sizeof(res.matchedVehicle));
        res.matchConfidence = 98.9f;
    } else if (N == 12 && M == 1) {
        strncpy(res.matchedVehicle, "Suzuki / Daihatsu (12-1)", sizeof(res.matchedVehicle));
        res.matchConfidence = 98.5f;
    } else {
        snprintf(res.matchedVehicle, sizeof(res.matchedVehicle), "Pola Kustom (%u-%u)", N, M);
        res.matchConfidence = 95.0f;
    }
}

SnifferResult SignalSniffer::decode(const RawSignalEdge* events, size_t eventCount) {
    SnifferResult res;
    res.success = false;
    if (eventCount < 16) return res;

    size_t risingCount = 0;
    uint64_t totalHighUs = 0;
    size_t highPulseCount = 0;

    int8_t lastCkpLvl = -1;
    for (size_t i = 0; i < eventCount; ++i) {
        if (events[i].channel == 0) {
            if (events[i].level == 1 && lastCkpLvl != 1) {
                if (risingCount < 384) _ckpRising[risingCount++] = events[i].timestampUs;
                lastCkpLvl = 1;
            } else if (events[i].level == 0 && lastCkpLvl != 0) {
                if (risingCount > 0 && events[i].timestampUs > _ckpRising[risingCount - 1]) {
                    totalHighUs += (events[i].timestampUs - _ckpRising[risingCount - 1]);
                    highPulseCount++;
                }
                lastCkpLvl = 0;
            }
        }
    }
    if (risingCount < 10) return res;

    size_t intervalCount = risingCount - 1;
    for (size_t i = 0; i < intervalCount; ++i) {
        _intervals[i] = _ckpRising[i + 1] - _ckpRising[i];
        _sortIntervals[i] = _intervals[i];
    }
    uint32_t nominalPeriod = _findMedian(_sortIntervals, intervalCount);
    if (nominalPeriod < 20) return res;

    size_t gapIndices[16];
    size_t gapCount = 0;
    uint64_t totalDeviationUs = 0;
    size_t normalToothCount = 0;
    uint32_t gapThreshold = (uint32_t)(nominalPeriod * 1.55f);

    for (size_t i = 0; i < intervalCount && gapCount < 16; ++i) {
        if (_intervals[i] >= gapThreshold) {
            if (gapCount == 0 || (i - gapIndices[gapCount - 1]) >= 3) {
                gapIndices[gapCount++] = i;
            }
        } else {
            totalDeviationUs += (_intervals[i] > nominalPeriod) ? (_intervals[i] - nominalPeriod) : (nominalPeriod - _intervals[i]);
            normalToothCount++;
        }
    }

    res.jitterPercent = (normalToothCount > 0) 
        ? ((float)totalDeviationUs / (float)normalToothCount / (float)nominalPeriod) * 100.0f 
        : 0.0f;

    uint16_t totalTeeth = 36;
    uint8_t missingTeeth = 1;
    uint32_t revPeriodUs = 0;

    if (gapCount >= 2) {
        size_t physicalTeethInRev = gapIndices[1] - gapIndices[0];
        float gapRatio = (float)_intervals[gapIndices[0]] / (float)nominalPeriod;
        missingTeeth = (uint8_t)roundf(gapRatio - 1.0f);
        if (missingTeeth < 1) missingTeeth = 1;
        if (missingTeeth > 4) missingTeeth = 4;
        totalTeeth = physicalTeethInRev + missingTeeth;
        revPeriodUs = _ckpRising[gapIndices[1]] - _ckpRising[gapIndices[0]];
    } else if (gapCount == 1) {
        float gapRatio = (float)_intervals[gapIndices[0]] / (float)nominalPeriod;
        missingTeeth = (uint8_t)roundf(gapRatio - 1.0f);
        if (missingTeeth < 1) missingTeeth = 1;
        if (missingTeeth > 4) missingTeeth = 4;
        
        if (intervalCount >= 50) totalTeeth = 60;
        else if (intervalCount >= 30) totalTeeth = 36;
        else if (intervalCount >= 20) totalTeeth = 24;
        else if (intervalCount >= 10) totalTeeth = 12;
        else totalTeeth = 4;

        revPeriodUs = nominalPeriod * totalTeeth;
    } else {
        missingTeeth = 0;
        totalTeeth = intervalCount;
        revPeriodUs = nominalPeriod * totalTeeth;
    }

    if (revPeriodUs < 1000) return res; // Guard against divide by zero

    res.detectedRpm = (uint32_t)(60000000ULL / revPeriodUs);
    if (res.detectedRpm < 50 || res.detectedRpm > 20000) return res;

    float avgHigh = (highPulseCount > 0) ? ((float)totalHighUs / (float)highPulseCount) : (nominalPeriod * 0.5f);
    float measuredDuty = avgHigh / (float)nominalPeriod;
    if (measuredDuty < 0.10f) measuredDuty = 0.10f;
    if (measuredDuty > 0.90f) measuredDuty = 0.90f;

    res.wheel.totalTeeth = totalTeeth;
    res.wheel.missingTeeth = missingTeeth;
    res.wheel.missingPosition = 0;
    res.wheel.dutyCycle = measuredDuty;
    res.wheel.inverted = false;

    // 6. Map & Cluster Camshaft (CMP) Events (0 - 720 deg)
    res.cam.clear();
    uint32_t syncRefUs = (gapCount > 0) ? _ckpRising[gapIndices[0]] : _ckpRising[0];
    uint32_t cycle720Us = revPeriodUs * 2;
    if (cycle720Us == 0) return res; // Guard against modulo by zero

    for (size_t i = 0; i < eventCount; ++i) {
        if (events[i].channel == 1) {
            int32_t deltaT = (int32_t)(events[i].timestampUs - syncRefUs);
            while (deltaT < 0) deltaT += cycle720Us;
            float rawAngle = ((float)(deltaT % cycle720Us) / (float)cycle720Us) * 720.0f;
            bool isHigh = (events[i].level == 1);
            
            bool foundClose = false;
            const CmpEvent* existing = res.cam.getEvents();
            for (size_t k = 0; k < res.cam.getEventCount(); ++k) {
                if (existing[k].levelHigh == isHigh && fabsf(existing[k].angleDeg - rawAngle) < 5.0f) {
                    foundClose = true;
                    break;
                }
            }
            if (!foundClose && res.cam.getEventCount() < 16) {
                res.cam.addEvent(rawAngle, isHigh);
            }
        }
    }

    _matchVehicleProfile(res);
    snprintf(res.summary, sizeof(res.summary), "%u-%u CKP @ %u RPM, Duty %.0f%%, Jitter %.1f%%",
             res.wheel.totalTeeth, res.wheel.missingTeeth, (unsigned)res.detectedRpm,
             res.wheel.dutyCycle * 100.0f, res.jitterPercent);
    res.success = true;
    return res;
}

} // namespace EcuEngine
