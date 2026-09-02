#pragma once
#include <stdint.h>
#include "power_cycle_types.h"

namespace EcuEngine {

class PowerCycleController {
public:
    PowerCycleController();

    void init();
    void setConfig(const PowerCycleConfig& cfg);
    const PowerCycleConfig& getConfig() const { return _config; }
    PowerCycleConfig& getConfig() { return _config; }

    const PowerCycleState& getState() const { return _state; }
    PowerCycleState& getState() { return _state; }

    void start();
    void stop();
    void resetStats();

    /**
     * @brief Update mesin siklus timing setiap 20ms
     * @param deltaMs Selang waktu dalam milidetik
     * @param mrelActive Status logika pin M-REL yang dibaca dari hardware
     */
    void update(uint32_t deltaMs, bool mrelActive);

private:
    PowerCycleConfig _config{};
    PowerCycleState  _state{};
    bool             _mrelLatchedThisCycle{false};
};

} // namespace EcuEngine
