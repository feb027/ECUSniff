#include "rpm_controller.h"
#include <algorithm>

namespace EcuEngine {

RpmController::RpmController() {}

void RpmController::reset() {
    _crankStage = CrankingStage::Idle;
    _elapsedCrankMs = 0;
    _sweepAscending = true;
}

void RpmController::startCranking(const CrankingConfig& config) {
    _crankStage = CrankingStage::Cranking;
    _elapsedCrankMs = 0;
    _currentDynamicRpm = config.crankingRpm;
}

uint32_t RpmController::update(EngineRuntimeState& state, uint32_t deltaMs) {
    if (!state.isRunning) {
        reset();
        state.currentRpm = 0;
        return 0;
    }

    switch (state.runMode) {
        case EngineRunMode::FixedRpm: {
            _currentDynamicRpm = state.targetRpm;
            break;
        }

        case EngineRunMode::Cranking: {
            _elapsedCrankMs += deltaMs;
            if (_crankStage == CrankingStage::Cranking) {
                _currentDynamicRpm = state.cranking.crankingRpm;
                if (_elapsedCrankMs >= state.cranking.crankDurationMs) {
                    _crankStage = CrankingStage::Ramping;
                    _elapsedCrankMs = 0;
                }
            } else if (_crankStage == CrankingStage::Ramping) {
                float progress = static_cast<float>(_elapsedCrankMs) / static_cast<float>(state.cranking.rampDurationMs);
                if (progress >= 1.0f) {
                    _crankStage = CrankingStage::Running;
                    _currentDynamicRpm = state.cranking.runRpm;
                } else {
                    int32_t diff = static_cast<int32_t>(state.cranking.runRpm) - static_cast<int32_t>(state.cranking.crankingRpm);
                    _currentDynamicRpm = state.cranking.crankingRpm + static_cast<uint32_t>(diff * progress);
                }
            } else {
                _currentDynamicRpm = state.cranking.runRpm;
            }
            break;
        }

        case EngineRunMode::AutoSweep: {
            uint32_t step = (state.sweep.sweepRateRpmPerSec * deltaMs) / 1000;
            if (step == 0) step = 1;

            if (_sweepAscending) {
                _currentDynamicRpm += step;
                if (_currentDynamicRpm >= state.sweep.maxRpm) {
                    _currentDynamicRpm = state.sweep.maxRpm;
                    _sweepAscending = false;
                }
            } else {
                if (_currentDynamicRpm <= state.sweep.minRpm + step) {
                    _currentDynamicRpm = state.sweep.minRpm;
                    _sweepAscending = true;
                } else {
                    _currentDynamicRpm -= step;
                }
            }
            break;
        }
    }

    state.currentRpm = _currentDynamicRpm;
    return _currentDynamicRpm;
}

} // namespace EcuEngine
