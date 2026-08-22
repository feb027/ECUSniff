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

    // 1. Extract CKP Rising Edges
    uint32_t ckpTimes[256];
    size_t ckpCount = 0;

    for (size_t i = 0; i < eventCount && ckpCount < 256; ++i) {
        if (events[i].channel == 0 && events[i].level == 1) {
            ckpTimes[ckpCount++] = events[i].timestampUs;
        }
    }

    if (ckpCount < 10) return res;

    // 2. Calculate Inter-Tooth Intervals
    uint32_t intervals[256];
    uint32_t sortIntervals[256];
    size_t intervalCount = ckpCount - 1;

    for (size_t i = 0; i < intervalCount; ++i) {
        intervals[i] = ckpTimes[i + 1] - ckpTimes[i];
        sortIntervals[i] = intervals[i];
    }

    uint32_t nominalPeriod = _findMedian(sortIntervals, intervalCount);
    if (nominalPeriod < 10) return res;

    // 3. Locate Missing Tooth Gaps
    size_t gapIndices[16];
    size_t gapCount = 0;

    for (size_t i = 0; i < intervalCount && gapCount < 16; ++i) {
        if (intervals[i] > (nominalPeriod * 3 / 2)) {
            gapIndices[gapCount++] = i;
        }
    }

    // 4. Calculate Total Teeth (N) and Missing Teeth (M)
    uint16_t totalTeeth = 36;
    uint8_t missingTeeth = 1;
    uint32_t revPeriodUs = 0;

    if (gapCount >= 2) {
        size_t teethInRev = gapIndices[1] - gapIndices[0];
        float gapRatio = (float)intervals[gapIndices[0]] / (float)nominalPeriod;

        missingTeeth = (gapRatio >= 2.5f) ? 2 : 1;
        totalTeeth = teethInRev + missingTeeth;

        revPeriodUs = ckpTimes[gapIndices[1]] - ckpTimes[gapIndices[0]];
    } else if (gapCount == 1) {
        float gapRatio = (float)intervals[gapIndices[0]] / (float)nominalPeriod;
        missingTeeth = (gapRatio >= 2.5f) ? 2 : 1;
        totalTeeth = (intervalCount > 30 && intervalCount < 45) ? 36 : (intervalCount >= 45 ? 60 : 36);
        revPeriodUs = nominalPeriod * totalTeeth;
    } else {
        missingTeeth = 0;
        totalTeeth = intervalCount;
        revPeriodUs = nominalPeriod * totalTeeth;
    }

    if (revPeriodUs == 0) return res;

    // 5. Calculate Detected RPM
    res.detectedRpm = (uint32_t)(60000000ULL / revPeriodUs);
    if (res.detectedRpm < 50 || res.detectedRpm > 20000) return res;

    res.wheel.totalTeeth = totalTeeth;
    res.wheel.missingTeeth = missingTeeth;
    res.wheel.missingPosition = 0;
    res.wheel.dutyCycle = 0.50f;
    res.wheel.inverted = false;

    // 6. Map Camshaft (CMP) Events
    res.cam.clear();
    uint32_t syncRefUs = (gapCount > 0) ? ckpTimes[gapIndices[0]] : ckpTimes[0];

    for (size_t i = 0; i < eventCount; ++i) {
        if (events[i].channel == 1) {
            int32_t deltaT = (int32_t)(events[i].timestampUs - syncRefUs);
            if (deltaT >= 0) {
                float angle = ((float)(deltaT % (revPeriodUs * 2)) / (float)(revPeriodUs * 2)) * 720.0f;
                res.cam.addEvent(angle, events[i].level == 1);
            }
        }
    }

    _matchVehicleProfile(res);

    snprintf(res.summary, sizeof(res.summary), "%u-%u CKP @ %u RPM, %u Cam",
             res.wheel.totalTeeth, res.wheel.missingTeeth, (unsigned)res.detectedRpm, (unsigned)res.cam.getEventCount());
    res.success = true;
    return res;
}

} // namespace EcuEngine
