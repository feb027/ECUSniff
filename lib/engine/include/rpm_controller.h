#pragma once
#include <stdint.h>
#include "engine_types.h"

namespace EcuEngine {

enum class CrankingStage : uint8_t {
    Idle = 0,
    Cranking,
    Ramping,
    Running
};

class RpmController {
public:
    RpmController();

    void reset();
    void startCranking(const CrankingConfig& config);
    uint32_t update(EngineRuntimeState& state, uint32_t deltaMs);

private:
    CrankingStage _crankStage{CrankingStage::Idle};
    uint32_t _elapsedCrankMs{0};
    uint32_t _currentDynamicRpm{850};
    bool     _sweepAscending{true};
};

} // namespace EcuEngine
