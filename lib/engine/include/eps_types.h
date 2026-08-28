#pragma once
#include <stdint.h>

namespace EcuEngine {

enum class EpsOemPreset : uint8_t {
    ToyotaAvanza = 0,
    SuzukiKarimun = 1,
    SuzukiErtiga  = 2,
    HondaJazz     = 3,
    CustomParametric = 4,
    COUNT         = 5
};

struct EpsPresetData {
    const char* name;
    const char* vehicleModel;
    float       vssPulsePerKm;    // Pulsa VSS per km tempuh (~2548 = 4 pulsa/putaran ban standar)
    uint8_t     rpmPulsesPerRev;  // Pulsa Tachometer per putaran mesin (misal: 2 untuk 4-silinder)
    float       defaultTrqVoltage;// Tegangan center torque sensor (umumnya 2.50 V)
    float       trqVoltageSpan;   // Rentang tegangan maks deviasi torque (misal: 1.50 V -> 1.0V - 4.0V)
};

struct EpsConfig {
    EpsOemPreset preset{EpsOemPreset::ToyotaAvanza};
    float        speedKmh{40.0f};
    uint32_t     targetRpm{1200};
    float        vssPulsePerKm{2548.0f};
    uint8_t      rpmPulsesPerRev{2};
    float        steerTorque{0.0f};   // -1.0f (Full Kiri) s.d. +1.0f (Full Kanan), 0.0f (Lurus)
    bool         autoSweep{false};
    float        sweepMinSpeed{0.0f};
    float        sweepMaxSpeed{120.0f};
    float        sweepStep{2.0f};
};

struct EpsRuntimeState {
    bool     isRunning{false};
    float    currentSpeedKmh{0.0f};
    uint32_t currentRpm{0};
    float    vssFreqHz{0.0f};
    float    rpmFreqHz{0.0f};
    float    trq1Voltage{2.50f};
    float    trq2Voltage{2.50f};
    bool     sweepDirectionUp{true};
};

} // namespace EcuEngine
