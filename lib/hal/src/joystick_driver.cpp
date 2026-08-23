#include "joystick_driver.h"
#include <Arduino.h>

namespace EcuHal {

JoystickDriver::JoystickDriver() {}

void JoystickDriver::init() {
    pinMode(PinConfig::JOY_SW, INPUT_PULLUP);
    pinMode(PinConfig::JOY_VRX, INPUT);
    pinMode(PinConfig::JOY_VRY, INPUT);

    // Auto-calibration: Rata-rata 16 sample posisi istirahat joystick
    int32_t sumX = 0;
    int32_t sumY = 0;
    for (int i = 0; i < 16; ++i) {
        sumX += analogRead(PinConfig::JOY_VRX);
        sumY += analogRead(PinConfig::JOY_VRY);
        delay(5);
    }
    _centerX = sumX / 16;
    _centerY = sumY / 16;

    // Deteksi apakah joystick terpasang (nilai normal potentiometer 500 - 3500)
    // Jika pin terbuka/floating atau 0V (tidak dicolok), otomatis dinonaktifkan
    if (_centerX >= 400 && _centerX <= 3700 && _centerY >= 400 && _centerY <= 3700) {
        _isEnabled = true;
        Serial.printf("[JOYSTICK] HW-504 Calibrated Center (X:%d, Y:%d) -> ONLINE\n", _centerX, _centerY);
    } else {
        _isEnabled = false;
        Serial.printf("[JOYSTICK] Pins floating or disconnected (X:%d, Y:%d) -> DISABLED\n", _centerX, _centerY);
    }
}

JoyAction JoystickDriver::update() {
    bool btnDown = (digitalRead(PinConfig::JOY_SW) == LOW);
    if (btnDown && !_btnWasDown) {
        _btnWasDown = true;
        return JoyAction::Click;
    } else if (!btnDown && _btnWasDown) {
        _btnWasDown = false;
    }

    if (!_isEnabled) return JoyAction::None;

    int xVal = analogRead(PinConfig::JOY_VRX);
    int yVal = analogRead(PinConfig::JOY_VRY);
    uint32_t nowMs = millis();

    int dx = xVal - _centerX;
    int dy = yVal - _centerY;

    JoyAction rawDir = JoyAction::None;
    if (abs(dx) > abs(dy)) {
        if (dx < -ADC_THRESHOLD) rawDir = JoyAction::Left;
        else if (dx > ADC_THRESHOLD) rawDir = JoyAction::Right;
    } else {
        if (dy < -ADC_THRESHOLD) rawDir = JoyAction::Up;
        else if (dy > ADC_THRESHOLD) rawDir = JoyAction::Down;
    }

    if (rawDir == JoyAction::None) {
        _dirHeld = false;
        return JoyAction::None;
    }

    if (!_dirHeld) {
        _dirHeld = true;
        _lastActionTimeMs = nowMs;
        return rawDir;
    } else {
        if ((nowMs - _lastActionTimeMs) >= REPEAT_DELAY_MS) {
            _lastActionTimeMs = nowMs - (REPEAT_DELAY_MS - REPEAT_RATE_MS);
            return rawDir;
        }
    }
    return JoyAction::None;
}

} // namespace EcuHal
