#pragma once
#include "eps_types.h"

namespace EcuEngine {

class EpsController {
public:
    EpsController();

    void init();
    void setPreset(EpsOemPreset preset);
    void setSpeed(float kmh);
    void setRpm(uint32_t rpm);
    void setSteerTorque(float torque); // -1.0f (Left) to +1.0f (Right)
    void setAutoSweep(bool enabled);
    void setRunning(bool running);
    void toggleRunning();

    void update(float dtSeconds);

    const EpsConfig& getConfig() const { return _config; }
    EpsConfig& getConfig() { return _config; }
    const EpsRuntimeState& getState() const { return _state; }

    static const EpsPresetData* getPresetData(EpsOemPreset preset);
    static const char* getPresetName(EpsOemPreset preset);

private:
    EpsConfig       _config;
    EpsRuntimeState _state;

    void _recalculateFrequencies();
};

} // namespace EcuEngine
