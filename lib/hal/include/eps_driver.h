#pragma once
#include <stdint.h>
#include "eps_types.h"

namespace EcuHal {

class EpsDriver {
public:
    EpsDriver();

    void init();
    void updateOutputs(const EcuEngine::EpsRuntimeState& state);
    void stop();

private:
    bool     _initialized{false};
    float    _lastVssFreq{-1.0f};
    float    _lastRpmFreq{-1.0f};
    float    _lastTrq1Volt{-1.0f};
    float    _lastTrq2Volt{-1.0f};
    bool     _lastRunning{false};

    static constexpr uint8_t LEDC_CH_VSS  = 4;
    static constexpr uint8_t LEDC_CH_RPM  = 5;
    static constexpr uint8_t LEDC_CH_TRQ1 = 6;
    static constexpr uint8_t LEDC_CH_TRQ2 = 7;
};

} // namespace EcuHal
