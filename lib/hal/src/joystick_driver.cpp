#include "joystick_driver.h"
#include <Arduino.h>

namespace EcuHal {

JoystickDriver::JoystickDriver() {}

void JoystickDriver::init() {
    pinMode(PinConfig::JOY_SW, INPUT_PULLUP);
    pinMode(PinConfig::JOY_VRX, INPUT);
    pinMode(PinConfig::JOY_VRY, INPUT);
    _centerX = 2048;
    _centerY = 2048;
}

JoyAction JoystickDriver::update() {
    // 1. Digital Switch (Click)
    bool btnDown = (digitalRead(PinConfig::JOY_SW) == LOW);
    if (btnDown && !_btnWasDown) {
        _btnWasDown = true;
        return JoyAction::Click;
    } else if (!btnDown && _btnWasDown) {
        _btnWasDown = false;
    }

    // 2. Analog X/Y Reading
    int xVal = analogRead(PinConfig::JOY_VRX);
    int yVal = analogRead(PinConfig::JOY_VRY);
    uint32_t nowMs = millis();

    // Abaikan jika pin floating / tidak tersambung (keduanya mendekati 0 atau 4095)
    if ((xVal >= 3950 && yVal >= 3950) || (xVal <= 100 && yVal <= 100)) {
        _dirHeld = false;
        return JoyAction::None;
    }

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
