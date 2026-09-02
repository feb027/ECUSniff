#include "power_cycle_controller.h"

namespace EcuEngine {

PowerCycleController::PowerCycleController() = default;

void PowerCycleController::init() {
    _state = PowerCycleState{};
}

void PowerCycleController::setConfig(const PowerCycleConfig& cfg) {
    _config = cfg;
}

void PowerCycleController::start() {
    _state.isRunning = true;
    _state.phase = CyclePhase::PhaseOn;
    _state.elapsedPhaseMs = 0;
    _state.igswState = true;
    _state.mrelDetected = false;
    _mrelLatchedThisCycle = false;
}

void PowerCycleController::stop() {
    _state.isRunning = false;
    _state.phase = CyclePhase::Stopped;
    _state.elapsedPhaseMs = 0;
    _state.igswState = false;
    _state.mrelDetected = false;
}

void PowerCycleController::resetStats() {
    _state.currentCycle = 0;
    _state.bootSuccessCount = 0;
    _state.bootFailCount = 0;
    _state.elapsedPhaseMs = 0;
    _mrelLatchedThisCycle = false;
}

void PowerCycleController::update(uint32_t deltaMs, bool mrelActive) {
    if (!_state.isRunning) return;

    _state.elapsedPhaseMs += deltaMs;

    if (_state.phase == CyclePhase::PhaseOn) {
        _state.igswState = true;

        // Monitor respon M-REL
        if (mrelActive) {
            _mrelLatchedThisCycle = true;
            _state.mrelDetected = true;
        }

        if (_state.elapsedPhaseMs >= _config.onDurationMs) {
            // Evaluasi respon M-REL siklus ini
            if (_config.readMrelFeedback) {
                if (_mrelLatchedThisCycle) {
                    _state.bootSuccessCount++;
                } else {
                    _state.bootFailCount++;
                }
            }

            // Pindah ke fase OFF
            _state.phase = CyclePhase::PhaseOff;
            _state.elapsedPhaseMs = 0;
            _state.igswState = false;
            _state.mrelDetected = false;
            _mrelLatchedThisCycle = false;
        }
    } else if (_state.phase == CyclePhase::PhaseOff) {
        _state.igswState = false;

        if (_state.elapsedPhaseMs >= _config.offDurationMs) {
            // 1 Siklus penuh selesai
            _state.currentCycle++;

            // Cek apakah mencapai target siklus
            if (_config.targetCycles > 0 && _state.currentCycle >= _config.targetCycles) {
                _state.isRunning = false;
                _state.phase = CyclePhase::Finished;
                _state.igswState = false;
            } else {
                // Lanjut ke siklus berikutnya
                _state.phase = CyclePhase::PhaseOn;
                _state.elapsedPhaseMs = 0;
                _state.igswState = true;
                _state.mrelDetected = false;
                _mrelLatchedThisCycle = false;
            }
        }
    }
}

} // namespace EcuEngine
