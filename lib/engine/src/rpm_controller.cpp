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
    _crankStage = CrankingStage::SpinUp;
    _elapsedCrankMs = 0;
    _currentDynamicRpm = 0;
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

        case EngineRunMode::CrankToFix:
        case EngineRunMode::CrankToSweep: {
            _elapsedCrankMs += deltaMs;
            constexpr uint32_t SPINUP_MS = 400;
            uint32_t crankHoldMs = (state.cranking.crankDurationMs > 0) ? state.cranking.crankDurationMs : 3000;
            uint32_t rampUpMs = state.cranking.fastFlare ? 500 : 2500;

            if (_crankStage == CrankingStage::SpinUp) {
                float prog = (float)_elapsedCrankMs / (float)SPINUP_MS;
                if (prog >= 1.0f) {
                    _currentDynamicRpm = state.cranking.crankingRpm;
                    _crankStage = CrankingStage::Cranking;
                    _elapsedCrankMs = 0;
                } else {
                    _currentDynamicRpm = (uint32_t)(state.cranking.crankingRpm * prog);
                }
            } else if (_crankStage == CrankingStage::Cranking) {
                _currentDynamicRpm = state.cranking.crankingRpm;
                if (_elapsedCrankMs >= crankHoldMs) {
                    _crankStage = CrankingStage::Ramping;
                    _elapsedCrankMs = 0;
                }
            } else if (_crankStage == CrankingStage::Ramping) {
                uint32_t destRpm = (state.runMode == EngineRunMode::CrankToFix) ? state.targetRpm : state.sweep.minRpm;
                float prog = (float)_elapsedCrankMs / (float)rampUpMs;
                if (prog >= 1.0f) {
                    _currentDynamicRpm = destRpm;
                    _crankStage = CrankingStage::PostCrank;
                    _sweepAscending = true;
                } else {
                    int32_t diff = (int32_t)destRpm - (int32_t)state.cranking.crankingRpm;
                    _currentDynamicRpm = state.cranking.crankingRpm + (uint32_t)(diff * prog);
                }
            } else {
                if (state.runMode == EngineRunMode::CrankToFix) {
                    _currentDynamicRpm = state.targetRpm;
                } else {
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
                }
            }
            break;
        }
    }

    state.currentRpm = _currentDynamicRpm;
    return _currentDynamicRpm;
}

} // namespace EcuEngine
