#pragma once
#include <stdint.h>
#include "esp_timer.h"
#include "speedo_types.h"

namespace EcuHal {

class SpeedoDriver {
public:
    SpeedoDriver();
    ~SpeedoDriver();

    void init();
    void updateOutputs(const EcuEngine::SpeedoConfig& config, const EcuEngine::SpeedoRuntimeState& state);
    void stop();
    void detectDacs(bool& fuelFound, bool& tempFound);

    uint32_t getKmhToggles() const;
    uint32_t getRpmToggles() const;

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

    esp_timer_handle_t _kmhTimer{nullptr};
    esp_timer_handle_t _rpmTimer{nullptr};
    bool               _kmhTimerRunning{false};
    bool               _rpmTimerRunning{false};

    static constexpr uint8_t LEDC_CH_TEMP = 2;
    static constexpr uint8_t LEDC_CH_FUEL = 3;

    static constexpr uint8_t MCP4725_ADDR_FUEL = 0x60;
    static constexpr uint8_t MCP4725_ADDR_TEMP = 0x61;

    void _setKmhFrequency(float freqHz);
    void _setRpmFrequency(float freqHz);
    void _writeDac(uint8_t addr, float volts);
};

} // namespace EcuHal

