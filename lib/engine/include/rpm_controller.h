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
    void updatePotentiometer(float voltage, EngineRuntimeState& state);

    CrankingStage getCrankingStage() const { return _crankStage; }
    bool isSweepAscending() const { return _sweepAscending; }

    void setSlewRate(float rpmPerSec) { _slewRateRpmPerSec = (rpmPerSec > 100.0f) ? rpmPerSec : 100.0f; }
    float getSlewRate() const { return _slewRateRpmPerSec; }

private:
    CrankingStage _crankStage{CrankingStage::Idle};
    uint32_t      _elapsedCrankMs{0};
    uint32_t      _currentDynamicRpm{0};
    float         _slewedRpm{0.0f};
    float         _slewRateRpmPerSec{1800.0f}; // 1800 RPM/s for smooth realistic engine inertia
    bool          _sweepAscending{true};
    float         _smoothedPotRpm{0.0f};
    uint32_t      _lastRawPotRpm{0};
};

} // namespace EcuEngine
