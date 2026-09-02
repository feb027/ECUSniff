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
            // Mode Potensio merespon langsung putaran fisik tanpa delay inersia lambat
            _slewedRpm = (float)state.potRpm;
            _currentDynamicRpm = state.potRpm;
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
    uint32_t minLimit = state.potCfg.minRpm;
    uint32_t maxLimit = (state.potCfg.maxRpm > minLimit) ? state.potCfg.maxRpm : (minLimit + 500);

    // 1. Zero-Clamp: Di bawah 0.04V (< 1.2% dari 3.3V), kunci pada batas bawah (minLimit)
    if (voltage < 0.04f) {
        _smoothedPotRpm = (float)minLimit;
        _lastRawPotRpm = minLimit;
        state.potRpm = minLimit;
        return;
    }

    // 2. Mapping Tegangan 0.04V - 3.25V ke rentang batas [minLimit s/d maxLimit]
    // Langkah fisik potensiometer menjadi sangat lebar, halus, dan presisi
    float ratio = (voltage - 0.04f) / (3.25f - 0.04f);
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    float targetRawRpm = (float)minLimit + (ratio * (float)(maxLimit - minLimit));

    // 3. Adaptive Dual-Rate Filter:
    // Jika putaran bergerak cepat (|diff| > 60 RPM): respons INSTAN (alpha = 0.90) tanpa delay
    // Jika putaran lambat/diam: filter lembut (alpha = 0.20) untuk meredam noise ADC
    float diff = fabsf(targetRawRpm - _smoothedPotRpm);
    float alpha = (diff > 60.0f) ? 0.90f : 0.20f;

    _smoothedPotRpm = (alpha * targetRawRpm) + ((1.0f - alpha) * _smoothedPotRpm);

    // 4. Schmidt-Trigger / Hysteresis Lock Window:
    float hystWindow = (float)(maxLimit - minLimit) * 0.003f; // ~0.3% span
    if (hystWindow < 6.0f) hystWindow = 6.0f;
    if (hystWindow > 20.0f) hystWindow = 20.0f;

    uint32_t potStep = (state.potCfg.rpmStep > 0) ? state.potCfg.rpmStep : 1;
    float halfStep = (float)potStep * 0.5f;

    if (state.potRpm < minLimit || state.potRpm > maxLimit) {
        uint32_t initRpm = (potStep <= 1) ? 
                           static_cast<uint32_t>(_smoothedPotRpm + 0.5f) :
                           (static_cast<uint32_t>((_smoothedPotRpm + halfStep) / potStep) * potStep);
        state.potRpm = (initRpm < minLimit) ? minLimit : (initRpm > maxLimit ? maxLimit : initRpm);
    } else {
        float deltaFromLocked = fabsf(_smoothedPotRpm - static_cast<float>(state.potRpm));
        float minHyst = (potStep <= 1) ? hystWindow : (float)potStep * 0.5f;
        if (deltaFromLocked >= minHyst) {
            uint32_t newRpm = (potStep <= 1) ? 
                              static_cast<uint32_t>(_smoothedPotRpm + 0.5f) :
                              (static_cast<uint32_t>((_smoothedPotRpm + halfStep) / potStep) * potStep);
            state.potRpm = (newRpm < minLimit) ? minLimit : (newRpm > maxLimit ? maxLimit : newRpm);
        }
    }
}

} // namespace EcuEngine
