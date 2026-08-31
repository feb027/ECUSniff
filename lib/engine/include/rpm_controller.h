#pragma once
#include <stdint.h>
#include "engine_types.h"

namespace EcuEngine {

enum class CrankingStage : uint8_t {
    Idle,
    SpinUp,
    Cranking,
    Ramping,
    PostCrank
};

class RpmController {
public:
    RpmController();

    void reset();
    void startCranking(const CrankingConfig& config);
    uint32_t update(EngineRuntimeState& state, uint32_t deltaMs);

    CrankingStage getCrankingStage() const { return _crankStage; }
    bool isSweepAscending() const { return _sweepAscending; }

private:
    CrankingStage _crankStage{CrankingStage::Idle};
    uint32_t      _elapsedCrankMs{0};
    uint32_t      _currentDynamicRpm{0};
    bool          _sweepAscending{true};
};

} // namespace EcuEngine
