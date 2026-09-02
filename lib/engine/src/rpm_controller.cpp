#include "rpm_controller.h"
#include <algorithm>
#include <math.h>

namespace EcuEngine {

RpmController::RpmController() {}

void RpmController::reset() {
    _crankStage = CrankingStage::Idle;
    _elapsedCrankMs = 0;
    _sweepAscending = true;
    _slewedRpm = 0.0f;
}

void RpmController::startCranking(const CrankingConfig& config) {
    _crankStage = CrankingStage::SpinUp;
    _elapsedCrankMs = 0;
    _currentDynamicRpm = 0;
    _slewedRpm = 0.0f;
}

uint32_t RpmController::update(EngineRuntimeState& state, uint32_t deltaMs) {
    if (!state.isRunning) {
        reset();
        state.currentRpm = 0;
        return 0;
    }

    switch (state.runMode) {
        case EngineRunMode::FixedRpm: {
            float target = (float)state.targetRpm;
            if (_slewedRpm <= 0.0f) _slewedRpm = target;
            float maxStep = (_slewRateRpmPerSec * (float)deltaMs) / 1000.0f;
            if (fabsf(target - _slewedRpm) <= maxStep) {
                _slewedRpm = target;
            } else if (target > _slewedRpm) {
                _slewedRpm += maxStep;
            } else {
                _slewedRpm -= maxStep;
            }
            _currentDynamicRpm = (uint32_t)(_slewedRpm + 0.5f);
            break;
        }

        case EngineRunMode::Potentiometer: {
            float target = (float)state.potRpm;
            if (_slewedRpm <= 0.0f) _slewedRpm = target;
            float maxStep = (_slewRateRpmPerSec * (float)deltaMs) / 1000.0f;
            if (fabsf(target - _slewedRpm) <= maxStep) {
                _slewedRpm = target;
            } else if (target > _slewedRpm) {
                _slewedRpm += maxStep;
            } else {
                _slewedRpm -= maxStep;
            }
            _currentDynamicRpm = (uint32_t)(_slewedRpm + 0.5f);
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
            uint32_t spinUpMs = (state.cranking.spinUpDurationMs > 0) ? state.cranking.spinUpDurationMs : 400;
            uint32_t crankHoldMs = (state.cranking.crankDurationMs > 0) ? state.cranking.crankDurationMs : 3000;
            uint32_t destRpm = (state.runMode == EngineRunMode::CrankToFix) ? state.targetRpm : state.sweep.minRpm;

            if (_crankStage == CrankingStage::SpinUp) {
                float prog = (float)_elapsedCrankMs / (float)spinUpMs;
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
                if (state.cranking.fastFlare) {
                    // Mode MELESAT:
                    // CrankToFix langsung meloncat seketika (0 ms) ke target RPM
                    if (state.runMode == EngineRunMode::CrankToFix) {
                        _currentDynamicRpm = destRpm;
                        _crankStage = CrankingStage::PostCrank;
                        _sweepAscending = true;
                    } else {
                        // CrankToSweep memiliki transisi cepat (~150 ms) ke Min RPM
                        constexpr uint32_t FAST_SWEEP_RAMP_MS = 150;
                        float prog = (float)_elapsedCrankMs / (float)FAST_SWEEP_RAMP_MS;
                        if (prog >= 1.0f) {
                            _currentDynamicRpm = destRpm;
                            _crankStage = CrankingStage::PostCrank;
                            _sweepAscending = true;
                        } else {
                            int32_t diff = (int32_t)destRpm - (int32_t)state.cranking.crankingRpm;
                            _currentDynamicRpm = state.cranking.crankingRpm + (uint32_t)(diff * prog);
                        }
                    }
                } else {
                    // Mode GRADUAL: menggunakan rampDurationMs yang dapat diatur
                    uint32_t rampUpMs = (state.cranking.rampDurationMs > 0) ? state.cranking.rampDurationMs : 2500;
                    float prog = (float)_elapsedCrankMs / (float)rampUpMs;
                    if (prog >= 1.0f) {
                        _currentDynamicRpm = destRpm;
                        _crankStage = CrankingStage::PostCrank;
                        _sweepAscending = true;
                    } else {
                        int32_t diff = (int32_t)destRpm - (int32_t)state.cranking.crankingRpm;
                        _currentDynamicRpm = state.cranking.crankingRpm + (uint32_t)(diff * prog);
                    }
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

void RpmController::updatePotentiometer(float voltage, EngineRuntimeState& state) {
    // 1. Zero-Clamp Threshold: Di bawah 0.03V (< 1% dari 3.3V), kunci solid pada 0 RPM
    if (voltage < 0.03f) {
        _smoothedPotRpm = 0.0f;
        _lastRawPotRpm = 0;
        state.potRpm = 0;
        return;
    }

    // 2. Mapping Tegangan 0.03V - 3.3V ke 50 - 10000 RPM
    float ratio = (voltage - 0.03f) / (3.3f - 0.03f);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    uint32_t rawRpm = (uint32_t)(50.0f + (ratio * 9950.0f));

    // 3. Deadband Filter: Jika perubahan nilai mentah <= 4 RPM, abaikan untuk cegah getaran diam
    int32_t diff = (int32_t)rawRpm - (int32_t)_lastRawPotRpm;
    if (abs(diff) <= 4) {
        rawRpm = _lastRawPotRpm;
    } else {
        _lastRawPotRpm = rawRpm;
    }

    // 4. Exponential Moving Average (EMA) Filter (Alpha = 0.18f untuk kehalusan maksimal)
    _smoothedPotRpm = (0.18f * (float)rawRpm) + (0.82f * _smoothedPotRpm);

    // 5. Simpan ke runtime state
    state.potRpm = (uint32_t)(_smoothedPotRpm + 0.5f);
}

} // namespace EcuEngine
