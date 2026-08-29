#include "speedo_controller.h"
#include <cmath>

namespace EcuEngine {

SpeedoController::SpeedoController() {
    init();
}

void SpeedoController::init() {
    _config.speedoKmh = 120;
    _config.speedoRpm = 4000;
    _config.speedoMaxRpm = 16000;
    _config.speedoTempPercent = 50;
    _config.speedoFuelPercent = 50;
    _config.speedoEnableKmh = true;
    _config.speedoEnableRpm = true;
    _config.speedoEnableTemp = true;
    _config.speedoEnableFuel = true;
    _config.pulsePerKm = 4000.0f;
    _config.speedoTachoPpr = 2.0f;
    _config.speedoPwmFreqHz = 5000;
    _config.gaugeCurve = SpeedoGaugeCurve::SqrtThermal;
    _config.dacRouting = SpeedoDacRouting::DualMcp4725;
    _config.tempCalMin = 0;
    _config.tempCalMid = 50;
    _config.tempCalMax = 100;
    _config.fuelCalMin = 0;
    _config.fuelCalMid = 50;
    _config.fuelCalMax = 100;
    _config.autoSweep = false;
    _config.sweepTimeSec = 5.0f;

    _state.isRunning = false;
    _state.currentKmh = 0.0f;
    _state.currentRpm = 0.0f;
    _state.currentTemp = 0.0f;
    _state.currentFuel = 0.0f;
    _state.sweepUp = true;
    _state.sweepProgress = 0.0f;
    _state.dacFuelFound = false;
    _state.dacTempFound = false;

    _recalculate();
}

void SpeedoController::setRunning(bool running) {
    _state.isRunning = running;
    _recalculate();
}

void SpeedoController::toggleRunning() {
    setRunning(!_state.isRunning);
}

void SpeedoController::setKmh(int32_t kmh) {
    if (kmh < 0) kmh = 0;
    if (kmh > 350) kmh = 350;
    _config.speedoKmh = kmh;
    _recalculate();
}

void SpeedoController::setRpm(int32_t rpm) {
    if (rpm < 0) rpm = 0;
    if (rpm > 20000) rpm = 20000;
    _config.speedoRpm = rpm;
    _recalculate();
}

void SpeedoController::setTemp(int32_t tempPercent) {
    if (tempPercent < 0) tempPercent = 0;
    if (tempPercent > 100) tempPercent = 100;
    _config.speedoTempPercent = tempPercent;
    _recalculate();
}

void SpeedoController::setFuel(int32_t fuelPercent) {
    if (fuelPercent < 0) fuelPercent = 0;
    if (fuelPercent > 100) fuelPercent = 100;
    _config.speedoFuelPercent = fuelPercent;
    _recalculate();
}

void SpeedoController::setPulsePerKm(float ppk) {
    if (ppk < 100.0f) ppk = 100.0f;
    if (ppk > 100000.0f) ppk = 100000.0f;
    _config.pulsePerKm = ppk;
    _recalculate();
}

void SpeedoController::setTachoPpr(float ppr) {
    if (ppr < 0.1f) ppr = 0.1f;
    if (ppr > 12.0f) ppr = 12.0f;
    _config.speedoTachoPpr = ppr;
    _recalculate();
}

void SpeedoController::setMaxRpm(int32_t maxRpm) {
    if (maxRpm < 1000) maxRpm = 1000;
    if (maxRpm > 25000) maxRpm = 25000;
    _config.speedoMaxRpm = maxRpm;
}

void SpeedoController::setPwmFreqHz(int32_t freqHz) {
    if (freqHz < 10) freqHz = 10;
    if (freqHz > 20000) freqHz = 20000;
    _config.speedoPwmFreqHz = freqHz;
}

void SpeedoController::setGaugeCurve(SpeedoGaugeCurve curve) {
    _config.gaugeCurve = curve;
    _recalculate();
}

void SpeedoController::setDacRouting(SpeedoDacRouting routing) {
    _config.dacRouting = routing;
    _recalculate();
}

void SpeedoController::setAutoSweep(bool enabled) {
    _config.autoSweep = enabled;
}

void SpeedoController::setSweepTimeSec(float sec) {
    if (sec < 0.5f) sec = 0.5f;
    if (sec > 60.0f) sec = 60.0f;
    _config.sweepTimeSec = sec;
}

void SpeedoController::setChannelEnable(uint8_t ch, bool enable) {
    if (ch == 0) _config.speedoEnableKmh = enable;
    else if (ch == 1) _config.speedoEnableRpm = enable;
    else if (ch == 2) _config.speedoEnableTemp = enable;
    else if (ch == 3) _config.speedoEnableFuel = enable;
    _recalculate();
}

void SpeedoController::setTempCal(int32_t minVal, int32_t midVal, int32_t maxVal) {
    _config.tempCalMin = minVal;
    _config.tempCalMid = midVal;
    _config.tempCalMax = maxVal;
    _recalculate();
}

void SpeedoController::setFuelCal(int32_t minVal, int32_t midVal, int32_t maxVal) {
    _config.fuelCalMin = minVal;
    _config.fuelCalMid = midVal;
    _config.fuelCalMax = maxVal;
    _recalculate();
}

void SpeedoController::setDacFound(bool fuelFound, bool tempFound) {
    _state.dacFuelFound = fuelFound;
    _state.dacTempFound = tempFound;
}

float SpeedoController::_apply3PointCal(float rawPercent, int32_t minVal, int32_t midVal, int32_t maxVal) {
    if (rawPercent <= 0.0f) return static_cast<float>(minVal);
    if (rawPercent >= 100.0f) return static_cast<float>(maxVal);
    if (rawPercent <= 50.0f) {
        float f = rawPercent / 50.0f;
        return minVal + (f * (midVal - minVal));
    } else {
        float f = (rawPercent - 50.0f) / 50.0f;
        return midVal + (f * (maxVal - midVal));
    }
}

float SpeedoController::_applyCurve(float calibratedPercent, SpeedoGaugeCurve curve) {
    float frac = calibratedPercent / 100.0f;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    if (curve == SpeedoGaugeCurve::SqrtThermal) {
        return std::sqrt(frac);
    }
    return frac;
}

void SpeedoController::update(float dtSeconds) {
    if (!_state.isRunning) {
        _state.currentKmh = 0.0f;
        _state.currentRpm = 0.0f;
        _state.currentTemp = 0.0f;
        _state.currentFuel = 0.0f;
        _state.hzKmh = 0.0f;
        _state.hzRpm = 0.0f;
        _state.dutyTemp = 0.0f;
        _state.dutyFuel = 0.0f;
        _state.voltTemp = 0.0f;
        _state.voltFuel = 0.0f;
        return;
    }

    if (_config.autoSweep) {
        float speed = 1.0f / (_config.sweepTimeSec > 0.1f ? _config.sweepTimeSec : 5.0f);
        if (_state.sweepUp) {
            _state.sweepProgress += speed * dtSeconds;
            if (_state.sweepProgress >= 1.0f) {
                _state.sweepProgress = 1.0f;
                _state.sweepUp = false;
            }
        } else {
            _state.sweepProgress -= speed * dtSeconds;
            if (_state.sweepProgress <= 0.0f) {
                _state.sweepProgress = 0.0f;
                _state.sweepUp = true;
            }
        }

        _state.currentKmh = _config.speedoEnableKmh ? (_state.sweepProgress * _config.speedoKmh) : 0.0f;
        _state.currentRpm = _config.speedoEnableRpm ? (_state.sweepProgress * _config.speedoRpm) : 0.0f;
        _state.currentTemp = _config.speedoEnableTemp ? (_state.sweepProgress * _config.speedoTempPercent) : 0.0f;
        _state.currentFuel = _config.speedoEnableFuel ? (_state.sweepProgress * _config.speedoFuelPercent) : 0.0f;
    } else {
        _state.currentKmh = _config.speedoEnableKmh ? static_cast<float>(_config.speedoKmh) : 0.0f;
        _state.currentRpm = _config.speedoEnableRpm ? static_cast<float>(_config.speedoRpm) : 0.0f;
        _state.currentTemp = _config.speedoEnableTemp ? static_cast<float>(_config.speedoTempPercent) : 0.0f;
        _state.currentFuel = _config.speedoEnableFuel ? static_cast<float>(_config.speedoFuelPercent) : 0.0f;
    }

    _recalculate();
}

void SpeedoController::_recalculate() {
    if (!_state.isRunning) {
        _state.hzKmh = 0.0f;
        _state.hzRpm = 0.0f;
        _state.dutyTemp = 0.0f;
        _state.dutyFuel = 0.0f;
        _state.voltTemp = 0.0f;
        _state.voltFuel = 0.0f;
        return;
    }

    // 1. KMH Frequency
    if (_config.speedoEnableKmh && _state.currentKmh > 0.05f) {
        _state.hzKmh = (_state.currentKmh * _config.pulsePerKm) / 3600.0f;
    } else {
        _state.hzKmh = 0.0f;
    }

    // 2. RPM Frequency
    if (_config.speedoEnableRpm && _state.currentRpm > 10.0f) {
        _state.hzRpm = (_state.currentRpm * _config.speedoTachoPpr) / 60.0f;
    } else {
        _state.hzRpm = 0.0f;
    }

    // 3. Temp Duty & Voltage
    if (_config.speedoEnableTemp) {
        float calTemp = _apply3PointCal(_state.currentTemp, _config.tempCalMin, _config.tempCalMid, _config.tempCalMax);
        float frac = _applyCurve(calTemp, _config.gaugeCurve);
        _state.dutyTemp = frac * 100.0f;
        _state.voltTemp = frac * 5.0f;
    } else {
        _state.dutyTemp = 0.0f;
        _state.voltTemp = 0.0f;
    }

    // 4. Fuel Duty & Voltage
    if (_config.speedoEnableFuel) {
        float calFuel = _apply3PointCal(_state.currentFuel, _config.fuelCalMin, _config.fuelCalMid, _config.fuelCalMax);
        float frac = _applyCurve(calFuel, _config.gaugeCurve);
        _state.dutyFuel = frac * 100.0f;
        _state.voltFuel = frac * 5.0f;
    } else {
        _state.dutyFuel = 0.0f;
        _state.voltFuel = 0.0f;
    }
}

} // namespace EcuEngine
