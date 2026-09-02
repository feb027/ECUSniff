#pragma once
#include <stdint.h>

namespace EcuEngine {

enum class CyclePhase : uint8_t {
    Stopped = 0,
    PhaseOn  = 1,
    PhaseOff = 2,
    Finished = 3
};

struct PowerCycleConfig {
    uint32_t onDurationMs{2000};       // Lama kontak IGSW ON (100 - 10000 ms)
    uint32_t offDurationMs{2000};      // Lama kontak IGSW OFF (100 - 10000 ms)
    uint32_t targetCycles{500};        // Target total siklus (0 = Tak terhingga / Infinite)
    bool     genPulseDuringOn{true};   // Hidupkan pulsa CKP/CMP saat fase ON
    bool     readMrelFeedback{true};   // Pantau pin M-REL untuk verifikasi boot ECU
};

struct PowerCycleState {
    bool        isRunning{false};
    CyclePhase  phase{CyclePhase::Stopped};
    uint32_t    currentCycle{0};
    uint32_t    elapsedPhaseMs{0};
    bool        igswState{false};      // Output aktual IGSW (+12V aktif jika true)
    bool        mrelDetected{false};   // Respon M-REL terbaca pada siklus ini
    uint32_t    bootSuccessCount{0};   // Jumlah siklus M-REL berhasil aktif
    uint32_t    bootFailCount{0};      // Jumlah siklus M-REL gagal aktif
};

} // namespace EcuEngine
