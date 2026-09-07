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
    void setCenterVoltage(float volts);
    void setSpanVoltage(float volts);
    void setVssPulsePerKm(float pulses);
    void setRpmPulsesPerRev(uint8_t pulses);
    void setSweepLimits(float minKmh, float maxKmh, float step);

    void update(float dtSeconds);

    void setFeedbackVoltages(float fb1, float fb2, bool adcFound = true);
    void setTrq1Scale(float scale);
    void setTrq1Offset(float offset);
    void setTrq2Scale(float scale);
    void setTrq2Offset(float offset);
    void saveCalibration();
    void loadCalibration();

    const EpsConfig& getConfig() const { return _config; }
    EpsConfig& getConfig() { return _config; }
    const EpsRuntimeState& getState() const { return _state; }
    void setDacFound(bool trq1, bool trq2) { _state.dacTrq1Found = trq1; _state.dacTrq2Found = trq2; }

    static const EpsPresetData* getPresetData(EpsOemPreset preset);
    static const char* getPresetName(EpsOemPreset preset);

private:
    EpsConfig       _config;
    EpsRuntimeState _state;

    void _recalculateFrequencies();
};

} // namespace EcuEngine
