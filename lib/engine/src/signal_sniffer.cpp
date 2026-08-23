#include "signal_sniffer.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace EcuEngine {

template<typename T>
static inline T clampVal(T v, T minV, T maxV) {
    return (v < minV) ? minV : ((v > maxV) ? maxV : v);
}

struct CamCluster { float sumAngle; uint16_t count; bool isHigh; };
struct TempCamEvent { float angle; bool high; };

SignalSniffer::SignalSniffer() {}

uint32_t SignalSniffer::_findMedian(uint32_t* arr, size_t n) {
    if (n == 0) return 0;
    for (size_t i = 0; i < n - 1; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if (arr[j] < arr[i]) { uint32_t t = arr[i]; arr[i] = arr[j]; arr[j] = t; }
        }
    }
    return arr[n / 2];
}

void SignalSniffer::_matchVehicleProfile(SnifferResult& res) {
    uint16_t N = res.wheel.totalTeeth;
    uint8_t  M = res.wheel.missingTeeth;

    struct Preset { uint16_t n; uint8_t m; const char* name; float conf; };
    static const Preset PRESETS[] = {
        { 36, 1, "Ford / Honda / Universal 36-1", 99.8f },
        { 36, 2, "Toyota 1NZ/2NZ/1ZZ (36-2)",     99.6f },
        { 60, 2, "Bosch / VW / BMW (60-2)",       99.9f },
        {  4, 1, "Yamaha NMAX / Aerox (4-1)",      99.2f },
        { 24, 2, "Mazda Miata / BP (24-2)",        98.9f },
        { 12, 1, "Suzuki / Daihatsu (12-1)",       98.5f },
        {  0, 0, "Standalone Camshaft (CMP)",      97.0f }
    };

    for (const auto& p : PRESETS) {
        if (N == p.n && (N == 0 || M == p.m)) {
            strncpy(res.matchedVehicle, p.name, sizeof(res.matchedVehicle));
            res.matchConfidence = p.conf;
            return;
        }
    }
    snprintf(res.matchedVehicle, sizeof(res.matchedVehicle), "Pola Kustom (%u-%u)", N, M);
    res.matchConfidence = 95.0f;
}

uint32_t SignalSniffer::_calcPhaseLockOffset(const RawSignalEdge* events, size_t count,
                                            uint32_t gap0Us, uint32_t revUs, uint32_t cycle720Us) {
    uint32_t widthA = 0, widthB = 0;
    int32_t lastHighUs = -1;

    for (size_t i = 0; i < count; ++i) {
        if (events[i].channel == 1 && events[i].timestampUs >= gap0Us) {
            uint32_t offset = (events[i].timestampUs - gap0Us) % cycle720Us;
            if (events[i].level == 1) {
                lastHighUs = (int32_t)offset;
            } else if (events[i].level == 0 && lastHighUs >= 0) {
                uint32_t w = (offset > (uint32_t)lastHighUs) ? (offset - lastHighUs) : (cycle720Us - lastHighUs + offset);
                if (lastHighUs < (int32_t)revUs) { if (w > widthA) widthA = w; }
                else { if (w > widthB) widthB = w; }
                lastHighUs = -1;
            }
        }
    }
    return (widthB > widthA) ? (gap0Us + revUs) : gap0Us;
}

void SignalSniffer::_clusterCamEvents(const RawSignalEdge* events, size_t count,
                                     uint32_t syncRefUs, uint32_t cycle720Us,
                                     CamEventTable& outCam, float tolDeg,
                                     float toothPitchDeg) {
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
                    if (diff < tolDeg) { matchIdx = (int)k; break; }
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
        float avgA = clusters[i].sumAngle / (float)clusters[i].count;
        float cleanA = avgA;
        if (toothPitchDeg > 0.1f) {
            float toothIdx = roundf(avgA / toothPitchDeg);
            float snapped = toothIdx * toothPitchDeg;
            float halfIdx = roundf(avgA / (toothPitchDeg * 0.5f));
            float halfSnapped = halfIdx * toothPitchDeg * 0.5f;
            cleanA = (fabsf(avgA - halfSnapped) < (toothPitchDeg * 0.25f)) ? halfSnapped : snapped;
        } else {
            cleanA = roundf(avgA * 2.0f) / 2.0f;
        }
        while (cleanA >= 720.0f) cleanA -= 720.0f;
        while (cleanA < 0.0f) cleanA += 720.0f;
        tempEvs[i] = { cleanA, clusters[i].isHigh };
    }
    for (size_t i = 0; i < tempCount; ++i) {
        for (size_t j = i + 1; j < tempCount; ++j) {
            if (tempEvs[j].angle < tempEvs[i].angle) {
                TempCamEvent tmp = tempEvs[i]; tempEvs[i] = tempEvs[j]; tempEvs[j] = tmp;
            }
        }
    }
    outCam.clear();
    for (size_t i = 0; i < tempCount; ++i) outCam.addEvent(tempEvs[i].angle, tempEvs[i].high);
}

SignalHealthStatus SignalSniffer::evaluateHealth(bool ckpActive, bool cmpActive, bool cmp2Active,
                                                uint32_t revPeriodUs, uint32_t nominalUs, uint32_t lastGapUs) {
    SignalHealthStatus h{};
    h.ckpOk = ckpActive; h.cmp1Ok = cmpActive; h.cmp2Ok = cmp2Active;
    if (!ckpActive && !cmpActive) {
        h.quality = SignalQuality::NoSignal;
        strncpy(h.diagnosticMsg, "Kabel Terputus / Generator Mati", sizeof(h.diagnosticMsg));
        return h;
    }
    if (ckpActive) {
        if (revPeriodUs > 1000 && revPeriodUs < 1200000) {
            h.liveRpm = (uint32_t)(60000000ULL / revPeriodUs);
            if (nominalUs > 0) h.liveTeeth = (uint16_t)roundf((float)revPeriodUs / (float)nominalUs);
            if (h.liveTeeth >= 4 && h.liveTeeth <= 120) {
                h.quality = SignalQuality::PhaseLocked;
                snprintf(h.diagnosticMsg, sizeof(h.diagnosticMsg), cmpActive ? "720-deg Locked (%u RPM)" : "CKP Locked (%u RPM)", (unsigned)h.liveRpm);
            } else {
                h.quality = SignalQuality::Noisy;
                strncpy(h.diagnosticMsg, "Derau CKP / Pola Tidak Dikenal", sizeof(h.diagnosticMsg));
            }
        } else {
            h.quality = SignalQuality::Syncing;
            strncpy(h.diagnosticMsg, "Menyinkronkan Fasa CKP...", sizeof(h.diagnosticMsg));
        }
    } else if (cmpActive) {
        h.quality = SignalQuality::PhaseLocked;
        strncpy(h.diagnosticMsg, "Standalone CMP Mode", sizeof(h.diagnosticMsg));
    }
    return h;
}

SnifferResult SignalSniffer::decode(const RawSignalEdge* events, size_t eventCount) {
    SnifferResult res;
    res.success = false;
    if (eventCount < 8) return res;

    size_t risingCount = 0, highPulseCount = 0;
    uint64_t totalHighUs = 0;
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

    // MODE A: STANDALONE CMP
    if (risingCount < 6) {
        size_t cmpRising = 0; int8_t lastLvl = -1;
        for (size_t i = 0; i < eventCount; ++i) {
            if (events[i].channel == 1) {
                if (events[i].level == 1 && lastLvl != 1) {
                    if (cmpRising < 384) _ckpRising[cmpRising++] = events[i].timestampUs;
                    lastLvl = 1;
                } else if (events[i].level == 0) lastLvl = 0;
            }
        }
        if (cmpRising < 2) return res;
        uint32_t camPeriodUs = (_ckpRising[cmpRising - 1] - _ckpRising[0]) / (cmpRising - 1);
        if (camPeriodUs < 1000) return res;
        res.detectedRpm = (uint32_t)(120000000ULL / camPeriodUs);
        if (res.detectedRpm < 50 || res.detectedRpm > 20000) return res;
        res.wheel.totalTeeth = 0; res.wheel.missingTeeth = 0; res.wheel.dutyCycle = 0.5f;
        _clusterCamEvents(events, eventCount, _ckpRising[0], camPeriodUs, res.cam, 15.0f, 0.0f);
        _matchVehicleProfile(res);
        snprintf(res.summary, sizeof(res.summary), "CMP Standalone: %u Pulsa @ %u RPM", (unsigned)res.cam.getEventCount(), (unsigned)res.detectedRpm);
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

    size_t gapIndices[16], gapCount = 0, normalToothCount = 0;
    uint64_t totalDeviationUs = 0;
    uint32_t gapThreshold = (uint32_t)(nominalPeriod * 1.42f);

    for (size_t i = 0; i < intervalCount && gapCount < 16; ++i) {
        if (_intervals[i] >= gapThreshold) {
            if (gapCount == 0 || (i - gapIndices[gapCount - 1]) >= 3) gapIndices[gapCount++] = i;
        } else {
            totalDeviationUs += (_intervals[i] > nominalPeriod) ? (_intervals[i] - nominalPeriod) : (nominalPeriod - _intervals[i]);
            normalToothCount++;
        }
    }

    res.jitterPercent = (normalToothCount > 0) ? ((float)totalDeviationUs / (float)normalToothCount / (float)nominalPeriod) * 100.0f : 0.0f;
    uint16_t totalTeeth = 36; uint8_t missingTeeth = 1; uint32_t revPeriodUs = 0;
    if (gapCount >= 2) {
        size_t physicalTeeth = gapIndices[1] - gapIndices[0];
        missingTeeth = clampVal((uint8_t)roundf((float)_intervals[gapIndices[0]] / (float)nominalPeriod - 1.0f), (uint8_t)1, (uint8_t)4);
        totalTeeth = physicalTeeth + missingTeeth;
        revPeriodUs = _ckpRising[gapIndices[1]] - _ckpRising[gapIndices[0]];
    } else if (gapCount == 1) {
        missingTeeth = clampVal((uint8_t)roundf((float)_intervals[gapIndices[0]] / (float)nominalPeriod - 1.0f), (uint8_t)1, (uint8_t)4);
        size_t physicalTeeth = gapIndices[0] + 1;
        totalTeeth = (physicalTeeth >= 3) ? (physicalTeeth + missingTeeth) : ((intervalCount >= 50) ? 60 : 36);
        revPeriodUs = nominalPeriod * totalTeeth;
    } else {
        missingTeeth = 0; totalTeeth = intervalCount; revPeriodUs = nominalPeriod * totalTeeth;
    }

    if (revPeriodUs < 1000) return res;
    res.detectedRpm = (uint32_t)(60000000ULL / revPeriodUs);
    if (res.detectedRpm < 50 || res.detectedRpm > 20000) return res;

    float avgHigh = (highPulseCount > 0) ? ((float)totalHighUs / (float)highPulseCount) : (nominalPeriod * 0.5f);
    res.wheel.totalTeeth = totalTeeth;
    res.wheel.missingTeeth = missingTeeth;
    res.wheel.missingPosition = 0;
    res.wheel.dutyCycle = clampVal(avgHigh / (float)nominalPeriod, 0.10f, 0.90f);
    res.wheel.inverted = false;

    // 720 deg Phase-Locking: Tooth 1 Rising Edge minus 1 Tooth Pitch gives EXACT 0.0 deg!
    uint32_t gap0Us = (gapCount > 0 && (gapIndices[0] + 1) < risingCount) ? (_ckpRising[gapIndices[0] + 1] - nominalPeriod) : _ckpRising[0];
    uint32_t cycle720Us = revPeriodUs * 2;
    size_t cmpEventCount = 0; uint64_t cmpMinPulseUs = 0xFFFFFFFF;

    for (size_t i = 0; i < eventCount; ++i) {
        if (events[i].channel == 1) {
            cmpEventCount++;
            if (i > 0 && events[i - 1].channel == 1) {
                uint64_t dt = events[i].timestampUs - events[i - 1].timestampUs;
                if (dt < cmpMinPulseUs) cmpMinPulseUs = dt;
            }
        }
    }

    if (cmpEventCount >= 4 && cmpMinPulseUs > 50) {
        float pitchDeg = res.wheel.getPitchAngleDeg();
        uint32_t syncRefUs = _calcPhaseLockOffset(events, eventCount, gap0Us, revPeriodUs, cycle720Us);
        _clusterCamEvents(events, eventCount, syncRefUs, cycle720Us, res.cam, 10.0f, pitchDeg);
        if (res.cam.getEventCount() < 2) res.cam.clear();
    } else {
        res.cam.clear();
    }

    _matchVehicleProfile(res);
    snprintf(res.summary, sizeof(res.summary), "%u-%u CKP @ %u RPM (%u Pulsa Cam)", res.wheel.totalTeeth, res.wheel.missingTeeth, (unsigned)res.detectedRpm, (unsigned)res.cam.getEventCount());
    res.success = true;
    return res;
}

} // namespace EcuEngine
