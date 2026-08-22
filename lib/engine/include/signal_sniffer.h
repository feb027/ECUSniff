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

private:
    uint32_t _findMedian(uint32_t* arr, size_t n);
    void     _matchVehicleProfile(SnifferResult& res);
};

} // namespace EcuEngine
