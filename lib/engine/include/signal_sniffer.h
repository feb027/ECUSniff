#pragma once
#include <stdint.h>
#include <stddef.h>
#include "engine_types.h"
#include "parametric_pattern.h"

namespace EcuEngine {

struct RawSignalEdge {
    uint32_t timestampUs;
    uint8_t  channel; // 0: CKP, 1: CMP, 2: CMP2
    uint8_t  level;   // 0: LOW, 1: HIGH
};

struct SnifferResult {
    bool             success{false};
    uint32_t         detectedRpm{0};
    ParametricWheel  wheel{};
    CamEventTable    cam{};
    char             summary[64]{0};
    char             matchedVehicle[48]{"Belum Terdeteksi"};
    float            matchConfidence{0.0f};
    float            jitterPercent{0.0f};
};

class SignalSniffer {
public:
    SignalSniffer();

    SnifferResult decode(const RawSignalEdge* events, size_t eventCount);
    SignalHealthStatus evaluateHealth(bool ckpActive, bool cmpActive, bool cmp2Active,
                                      uint32_t revPeriodUs, uint32_t nominalUs, uint32_t lastGapUs);

private:
    uint32_t _ckpRising[384];
    uint32_t _intervals[384];
    uint32_t _sortIntervals[384];

    uint32_t _findMedian(uint32_t* arr, size_t n);
    void     _matchVehicleProfile(SnifferResult& res);
    void     _clusterCamEvents(const RawSignalEdge* events, size_t count,
                               uint32_t syncRefUs, uint32_t cycle720Us,
                               CamEventTable& outCam, float tolDeg,
                               float toothPitchDeg = 0.0f);
    uint32_t _calcPhaseLockOffset(const RawSignalEdge* events, size_t count,
                                  uint32_t gap0Us, uint32_t revUs, uint32_t cycle720Us);
};

} // namespace EcuEngine
