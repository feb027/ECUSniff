#pragma once
#include <stdint.h>
#include <stddef.h>

namespace EcuEngine {

enum class PatternType : uint8_t {
    MissingTooth = 0,  // e.g. 36-1, 60-2, 36-2
    SingleTooth  = 1,
    MultiTooth   = 2,
    Irregular    = 3,
    CustomEvent  = 4
};

enum class EngineRunMode : uint8_t {
    FixedRpm  = 0,
    Cranking  = 1,
    AutoSweep = 2
};

struct CkpConfig {
    PatternType type{PatternType::MissingTooth};
    uint16_t totalTeeth{36};
    uint8_t missingTeeth{1};
    uint8_t missingPosition{0};
    float dutyCycle{0.5f};
    bool invertedPolarity{false};
};

struct CmpEvent {
    float angleDeg{0.0f};
    bool levelHigh{false};

    constexpr CmpEvent() = default;
    constexpr CmpEvent(float angle, bool level) : angleDeg(angle), levelHigh(level) {}
};

constexpr size_t MAX_CMP_EVENTS = 16;

struct CmpConfig {
    bool enabled{true};
    uint8_t eventCount{0};
    CmpEvent events[MAX_CMP_EVENTS]{};
    bool invertedPolarity{false};
};

struct CrankingConfig {
    uint32_t crankingRpm{200};
    uint32_t runRpm{850};
    uint32_t crankDurationMs{2500};
    uint32_t rampDurationMs{1500};
};

struct SweepConfig {
    uint32_t minRpm{800};
    uint32_t maxRpm{6000};
    uint32_t sweepRateRpmPerSec{1000};
    bool sweepUp{true};
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
};

} // namespace EcuEngine
