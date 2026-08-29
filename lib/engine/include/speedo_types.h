#pragma once
#include <stdint.h>

namespace EcuEngine {

enum class SpeedoGaugeCurve : uint8_t {
    SqrtThermal = 0, // Non-linear thermal sqrt compensation
    Linear      = 1  // Linear 1:1 response
};

enum class SpeedoDacRouting : uint8_t {
    DualPwm       = 0, // Dual PWM on GPIO Pins
    SingleDacFuel = 1, // MCP4725 Fuel (0x60) + PWM Temp
    SingleDacTemp = 2, // MCP4725 Temp (0x61) + PWM Fuel
    DualMcp4725   = 3  // Dual MCP4725 (0x60 Fuel + 0x61 Temp)
};

struct SpeedoConfig {
    int32_t          speedoKmh{120};
    int32_t          speedoRpm{4000};
    int32_t          speedoMaxRpm{16000};
    int32_t          speedoTempPercent{50};
    int32_t          speedoFuelPercent{50};
    bool             speedoEnableKmh{true};
    bool             speedoEnableRpm{true};
    bool             speedoEnableTemp{true};
    bool             speedoEnableFuel{true};
    float            pulsePerKm{4000.0f};
    float            speedoTachoPpr{2.0f};
    int32_t          speedoPwmFreqHz{5000};
    SpeedoGaugeCurve gaugeCurve{SpeedoGaugeCurve::SqrtThermal};
    SpeedoDacRouting dacRouting{SpeedoDacRouting::DualMcp4725};
    int32_t          tempCalMin{0};
    int32_t          tempCalMid{50};
    int32_t          tempCalMax{100};
    int32_t          fuelCalMin{0};
    int32_t          fuelCalMid{50};
    int32_t          fuelCalMax{100};
    bool             autoSweep{false};
    float            sweepTimeSec{5.0f};
};

struct SpeedoRuntimeState {
    bool  isRunning{false};
    float currentKmh{0.0f};
    float currentRpm{0.0f};
    float currentTemp{0.0f};
    float currentFuel{0.0f};
    float hzKmh{0.0f};
    float hzRpm{0.0f};
    float dutyTemp{0.0f}; // 0.0 - 100.0 %
    float dutyFuel{0.0f}; // 0.0 - 100.0 %
    float voltTemp{0.0f}; // 0.0 - 5.0 V
    float voltFuel{0.0f}; // 0.0 - 5.0 V
    bool  dacFuelFound{false};
    bool  dacTempFound{false};
    bool  sweepUp{true};
    float sweepProgress{0.0f};
};

} // namespace EcuEngine
