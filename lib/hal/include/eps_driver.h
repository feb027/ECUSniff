#pragma once
#include <stdint.h>
#include "esp_timer.h"
#include "eps_types.h"

namespace EcuHal {

class EpsDriver {
public:
    EpsDriver();
    ~EpsDriver();

    void init();
    void detectDacs(bool& trq1Found, bool& trq2Found);
    void updateOutputs(const EcuEngine::EpsRuntimeState& state);
    void stop();

    bool isDacTrq1Found() const { return _dacTrq1Found; }
    bool isDacTrq2Found() const { return _dacTrq2Found; }
    uint32_t getVssToggles() const;
    uint32_t getRpmToggles() const;

private:
    bool     _initialized{false};
    bool     _dacTrq1Found{false};
    bool     _dacTrq2Found{false};
    uint8_t  _trq1Addr{0x60};
    uint8_t  _trq2Addr{0x61};

    float    _lastVssFreq{-1.0f};
    float    _lastRpmFreq{-1.0f};
    float    _lastTrq1Volt{-1.0f};
    float    _lastTrq2Volt{-1.0f};
    bool     _lastRunning{false};
    bool     _vssTimerRunning{false};
    bool     _rpmTimerRunning{false};

    esp_timer_handle_t _vssTimer{nullptr};
    esp_timer_handle_t _rpmTimer{nullptr};

    static constexpr uint8_t MCP4725_ADDR_TRQ1 = 0x60;
    static constexpr uint8_t MCP4725_ADDR_TRQ2 = 0x61;

    static constexpr uint8_t LEDC_CH_TRQ1 = 6;
    static constexpr uint8_t LEDC_CH_TRQ2 = 5; // Channel 5 (Channel 7 reserved for Display Backlight)

    void _writeDac(uint8_t addr, float volts);
    void _setVssFrequency(float freqHz);
    void _setRpmFrequency(float freqHz);
};

} // namespace EcuHal
