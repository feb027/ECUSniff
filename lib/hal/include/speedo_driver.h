#pragma once
#include <stdint.h>
#include "speedo_types.h"

namespace EcuHal {

class SpeedoDriver {
public:
    SpeedoDriver();

    void init();
    void updateOutputs(const EcuEngine::SpeedoConfig& config, const EcuEngine::SpeedoRuntimeState& state);
    void stop();
    void detectDacs(bool& fuelFound, bool& tempFound);

private:
    bool     _initialized{false};
    float    _lastHzKmh{-1.0f};
    float    _lastHzRpm{-1.0f};
    float    _lastDutyTemp{-1.0f};
    float    _lastDutyFuel{-1.0f};
    int32_t  _lastPwmFreq{-1};
    float    _lastDacFuelVolt{-1.0f};
    float    _lastDacTempVolt{-1.0f};
    bool     _lastRunning{false};
    bool     _dacFuelFound{false};
    bool     _dacTempFound{false};
    uint32_t _lastDacPollMs{0};

    static constexpr uint8_t LEDC_CH_RPM  = 1;
    static constexpr uint8_t LEDC_CH_KMH  = 2;
    static constexpr uint8_t LEDC_CH_TEMP = 4;
    static constexpr uint8_t LEDC_CH_FUEL = 5;

    static constexpr uint8_t MCP4725_ADDR_FUEL = 0x60;
    static constexpr uint8_t MCP4725_ADDR_TEMP = 0x61;

    void _writeDac(uint8_t addr, float volts);
};

} // namespace EcuHal
