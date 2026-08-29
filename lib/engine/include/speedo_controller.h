#pragma once
#include "speedo_types.h"

namespace EcuEngine {

class SpeedoController {
public:
    SpeedoController();

    void init();
    void setRunning(bool running);
    void toggleRunning();
    void setKmh(int32_t kmh);
    void setRpm(int32_t rpm);
    void setTemp(int32_t tempPercent);
    void setFuel(int32_t fuelPercent);
    void setPulsePerKm(float ppk);
    void setTachoPpr(float ppr);
    void setMaxRpm(int32_t maxRpm);
    void setPwmFreqHz(int32_t freqHz);
    void setGaugeCurve(SpeedoGaugeCurve curve);
    void setDacRouting(SpeedoDacRouting routing);
    void setAutoSweep(bool enabled);
    void setSweepTimeSec(float sec);
    void setChannelEnable(uint8_t ch, bool enable); // 0: KMH, 1: RPM, 2: TEMP, 3: FUEL

    void setTempCal(int32_t minVal, int32_t midVal, int32_t maxVal);
    void setFuelCal(int32_t minVal, int32_t midVal, int32_t maxVal);
    void setDacFound(bool fuelFound, bool tempFound);

    void update(float dtSeconds);

    const SpeedoConfig& getConfig() const { return _config; }
    SpeedoConfig& getConfig() { return _config; }
    const SpeedoRuntimeState& getState() const { return _state; }

private:
    SpeedoConfig       _config;
    SpeedoRuntimeState _state;

    float _apply3PointCal(float rawPercent, int32_t minVal, int32_t midVal, int32_t maxVal);
    float _applyCurve(float calibratedPercent, SpeedoGaugeCurve curve);
    void  _recalculate();
};

} // namespace EcuEngine
