#include "signal_sniffer.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace EcuEngine {

struct CamCluster {
    float sumAngle;
    uint16_t count;
    bool isHigh;
};

struct TempCamEvent {
    float angle;
    bool high;
};

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
    } else if (N == 0) {
        strncpy(res.matchedVehicle, "Standalone Camshaft (CMP)", sizeof(res.matchedVehicle));
        res.matchConfidence = 97.0f;
    } else {
        snprintf(res.matchedVehicle, sizeof(res.matchedVehicle), "Pola Kustom (%u-%u)", N, M);
        res.matchConfidence = 95.0f;
    }
}

void SignalSniffer::_clusterCamEvents(const RawSignalEdge* events, size_t count,
                                     uint32_t syncRefUs, uint32_t cycle720Us,
                                     CamEventTable& outCam, float tolDeg) {
    if (cycle720Us == 0 || !events) return;

    CamCluster clusters[16];
    size_t clusterCount = 0;

    for (size_t i = 0; i < count; ++i) {
        if (events[i].channel == 1) {
            int32_t deltaT = (int32_t)(events[i].timestampUs - syncRefUs);
            while (deltaT < 0) deltaT += cycle720Us;
            float rawAngle = ((float)(deltaT % cycle720Us) / (float)cycle720Us) * 720.0f;
            bool isHigh = (events[i].level == 1);

            int matchIdx = -1;
            for (size_t k = 0; k < clusterCount; ++k) {
                if (clusters[k].isHigh == isHigh) {
                    float diff = fabsf((clusters[k].sumAngle / (float)clusters[k].count) - rawAngle);
                    if (diff > 360.0f) diff = 720.0f - diff;
                    if (diff < tolDeg) {
                        matchIdx = (int)k;
                        break;
                    }
                }
            }

            if (matchIdx >= 0) {
                clusters[matchIdx].sumAngle += rawAngle;
                clusters[matchIdx].count++;
            } else if (clusterCount < 16) {
                clusters[clusterCount++] = { rawAngle, 1, isHigh };
            }
        }
    }

    TempCamEvent tempEvs[16];
    size_t tempCount = clusterCount;
    for (size_t i = 0; i < tempCount; ++i) {
        tempEvs[i] = { clusters[i].sumAngle / (float)clusters[i].count, clusters[i].isHigh };
    }
    for (size_t i = 0; i < tempCount; ++i) {
        for (size_t j = i + 1; j < tempCount; ++j) {
            if (tempEvs[j].angle < tempEvs[i].angle) {
                TempCamEvent tmp = tempEvs[i];
                tempEvs[i] = tempEvs[j];
                tempEvs[j] = tmp;
            }
        }
    }

    outCam.clear();
    for (size_t i = 0; i < tempCount; ++i) {
        outCam.addEvent(tempEvs[i].angle, tempEvs[i].high);
    }
}

SnifferResult SignalSniffer::decode(const RawSignalEdge* events, size_t eventCount) {
    SnifferResult res;
    res.success = false;
    if (eventCount < 8) return res;

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

    // MODE A: STANDALONE CMP (Tanpa Sinyal CKP)
    if (risingCount < 6) {
        size_t cmpRising = 0;
        int8_t lastLvl = -1;
        for (size_t i = 0; i < eventCount; ++i) {
            if (events[i].channel == 1) {
                if (events[i].level == 1 && lastLvl != 1) {
                    if (cmpRising < 384) _ckpRising[cmpRising++] = events[i].timestampUs;
                    lastLvl = 1;
                } else if (events[i].level == 0) {
                    lastLvl = 0;
                }
            }
        }
        if (cmpRising < 2) return res;

        uint32_t camPeriodUs = (_ckpRising[cmpRising - 1] - _ckpRising[0]) / (cmpRising - 1);
        if (camPeriodUs < 1000) return res;

        res.detectedRpm = (uint32_t)(120000000ULL / camPeriodUs);
        if (res.detectedRpm < 50 || res.detectedRpm > 20000) return res;

        res.wheel.totalTeeth = 0;
        res.wheel.missingTeeth = 0;
        res.wheel.dutyCycle = 0.5f;

        _clusterCamEvents(events, eventCount, _ckpRising[0], camPeriodUs, res.cam, 15.0f);
        _matchVehicleProfile(res);
        snprintf(res.summary, sizeof(res.summary), "CMP Standalone: %u Pulsa @ %u RPM",
                 (unsigned)res.cam.getEventCount(), (unsigned)res.detectedRpm);
        res.success = true;
        return res;
    }

    // MODE B: DUAL CKP + CMP CAPTURE
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

    if (revPeriodUs < 1000) return res;

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

    uint32_t syncRefUs = (gapCount > 0) ? _ckpRising[gapIndices[0]] : _ckpRising[0];
    uint32_t cycle720Us = revPeriodUs * 2;
    _clusterCamEvents(events, eventCount, syncRefUs, cycle720Us, res.cam, 12.0f);

    _matchVehicleProfile(res);
    snprintf(res.summary, sizeof(res.summary), "%u-%u CKP @ %u RPM (%u Pulsa Cam)",
             res.wheel.totalTeeth, res.wheel.missingTeeth, (unsigned)res.detectedRpm,
             (unsigned)res.cam.getEventCount());
    res.success = true;
    return res;
}

} // namespace EcuEngine
