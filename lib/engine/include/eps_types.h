#pragma once
#include <stdint.h>

namespace EcuEngine {

enum class EpsOemPreset : uint8_t {
    ToyotaAvanza = 0,
    SuzukiKarimun = 1,
    SuzukiErtiga  = 2,
    SuzukiSwift   = 3,
    HondaJazz     = 4,
    HondaCity     = 5,
    RetrofitSwap  = 6,
    CustomParametric = 7,
    COUNT         = 8
};

struct EpsPresetData {
    const char* name;
    const char* vehicleModel;
    float       vssPulsePerKm;    // Pulsa VSS per km tempuh
    uint8_t     rpmPulsesPerRev;  // Pulsa Tachometer per putaran mesin
    float       defaultTrqVoltage;// Tegangan center torque sensor (umumnya 2.50 V)
    float       trqVoltageSpan;   // Rentang tegangan maks deviasi torque
    float       defaultSpeedKmh;  // Kecepatan default saat preset dipilih (km/h)
    uint32_t    defaultRpm;       // RPM default saat preset dipilih
};

struct EpsConfig {
    EpsOemPreset preset{EpsOemPreset::ToyotaAvanza};
    float        speedKmh{40.0f};
    uint32_t     targetRpm{1200};
    float        vssPulsePerKm{2548.0f};
    uint8_t      rpmPulsesPerRev{2};
    float        steerTorque{0.0f};   // -1.0f (Full Kiri) s.d. +1.0f (Full Kanan), 0.0f (Lurus)
    float        trqCenterVoltage{2.500f}; // Tegangan netral
    float        trqVoltageSpan{1.500f};   // Rentang deviasi tegangan maksimal
    bool         autoSweep{false};
    float        sweepMinSpeed{0.0f};
    float        sweepMaxSpeed{120.0f};
    float        sweepStep{2.0f};
    // Kalibrasi ADC Feedback TRQ (Divider Multiplier & Offset)
    float        trq1AdcScale{2.000f};   // Pengali rasio pembagi tegangan (10k:10k = 2.0x)
    float        trq1AdcOffset{0.000f};  // Offset koreksi tegangan A1 (Volt)
    float        trq2AdcScale{2.000f};   // Pengali rasio pembagi tegangan (10k:10k = 2.0x)
    float        trq2AdcOffset{0.000f};  // Offset koreksi tegangan A2 (Volt)
};

struct EpsRuntimeState {
    bool     isRunning{false};
    float    currentSpeedKmh{0.0f};
    uint32_t currentRpm{0};
    float    vssFreqHz{0.0f};
    float    rpmFreqHz{0.0f};
    float    trq1Voltage{2.50f};         // Target DAC output
    float    trq2Voltage{2.50f};         // Target DAC output
    float    trq1FeedbackVoltage{0.0f};  // Tegangan terukur riil dari ADC A1
    float    trq2FeedbackVoltage{0.0f};  // Tegangan terukur riil dari ADC A2
    bool     sweepDirectionUp{true};
    bool     dacTrq1Found{false};
    bool     dacTrq2Found{false};
    bool     adcFound{false};
};

} // namespace EcuEngine
