#pragma once
#include <stdint.h>
#include <stddef.h>

namespace EcuEngine {

enum class EngineRunMode : uint8_t {
    FixedRpm = 0,       // Mode Digital Encoder (FIX)
    Potentiometer = 1,  // Mode Analog Potensio (POT)
    AutoSweep = 2,      // Mode Auto Sweep (SWEEP)
    CrankToFix = 3,     // Mode Cranking ke Digital Fix (CRK>FIX)
    CrankToSweep = 4    // Mode Cranking ke Sweep (CRK>SWP)
};

struct CrankingConfig {
    uint32_t crankingRpm{200};
    uint32_t runRpm{850};
    uint32_t crankDurationMs{3000};
    uint32_t rampDurationMs{2500};    // Durasi gradual ramp (500 - 5000 ms)
    uint32_t spinUpDurationMs{400};   // Durasi 0 -> crank start (100 - 2000 ms)
    bool     fastFlare{false};        // true: MELESAT (instant 0ms ke fix, ~150ms ke sweep), false: GRADUAL
};

struct SweepConfig {
    uint32_t minRpm{800};
    uint32_t maxRpm{6000};
    uint32_t sweepRateRpmPerSec{500};
};

constexpr size_t MAX_CMP_EVENTS = 16;

struct CmpEvent {
    float angleDeg;
    bool  levelHigh;

    constexpr CmpEvent() : angleDeg(0.0f), levelHigh(false) {}
    constexpr CmpEvent(float angle, bool high) : angleDeg(angle), levelHigh(high) {}
};

enum class SignalQuality : uint8_t {
    NoSignal = 0,
    Noisy = 1,
    Syncing = 2,
    PhaseLocked = 3
};

struct SignalHealthStatus {
    SignalQuality quality{SignalQuality::NoSignal};
    bool     ckpOk{false};
    bool     cmp1Ok{false};
    bool     cmp2Ok{false};
    uint32_t liveRpm{0};
    uint16_t liveTeeth{0};
    uint8_t  liveMissing{0};
    uint8_t  liveCamCount{0};
    float    jitterPercent{0.0f};
    char     diagnosticMsg[48]{"Menunggu Sinyal..."};
};

struct EngineRuntimeState {
    EngineRunMode runMode{EngineRunMode::FixedRpm};
    uint32_t targetRpm{850};
    uint32_t currentRpm{0};
    float currentAngleDeg{0.0f};
    bool isRunning{false};
    bool ckpActive{false};
    bool cmp1Active{false};
    bool cmp2Active{false};
    
    CrankingConfig cranking{};
    SweepConfig    sweep{};
    uint32_t       rpmStep{50};

    // Bi-directional UI & Sniffer sync fields
    char     activeWheelName[48]{"Honda / Ford 36-1"};
    uint8_t  uiLevel{0};      // 0: MainHub, 1: Generator, 2: Capture
    uint8_t  activeTab{1};    // 0: Menu, 1: Dash/Live, 2: Ckp/Data, 3: Cmp/Cam
    uint8_t  captureState{0}; // 0: Idle, 1: Armed, 2: Recording, 3: Done
    uint32_t captureRpm{0};
    char     matchedVehicle[48]{"Belum Terdeteksi"};
    uint16_t capTotalTeeth{0};
    uint8_t  capMissingTeeth{0};
    float    capDutyCycle{0.5f};
    uint8_t  capCamCount{0};
    float    capCamAngles[8]{0};
    bool     capCamHighs[8]{false};

    // Live Pre-Flight Health Status
    SignalHealthStatus health{};

    // Hardware I2C MCP23017 Outputs
    bool     mcpFound{false};   // Chip MCP23017 terdeteksi pada bus I2C (0x20)
    bool     staActive{false};  // Sinyal Crank STA (+12V) aktif ke ECU
    bool     chgLampOn{true};   // Sinyal Indikator Pengisian Alternator (true = Lampu Aki ON / Ground aktif)

    // Hardware I2C ADS1115 ADC (Potentiometer)
    bool     adsFound{false};   // Chip ADS1115 ADC terdeteksi pada bus I2C (0x48)
    uint32_t potRpm{0};         // Nilai RPM hasil pembacaan potensiometer (0 - 10000 RPM)
};

} // namespace EcuEngine
