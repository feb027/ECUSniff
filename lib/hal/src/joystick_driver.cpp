#include "joystick_driver.h"
#include <Arduino.h>

namespace EcuHal {

JoystickDriver::JoystickDriver() = default;

void JoystickDriver::init() {
    pinMode(PinConfig::JOY_SW, INPUT_PULLUP);
    pinMode(PinConfig::JOY_VRX, INPUT);
    pinMode(PinConfig::JOY_VRY, INPUT);

    int32_t sumX = 0, sumY = 0;
    for (uint8_t i = 0; i < 16; ++i) {
        sumX += analogRead(PinConfig::JOY_VRX);
        sumY += analogRead(PinConfig::JOY_VRY);
        delayMicroseconds(500);
    }
    _centerX = sumX / 16;
    _centerY = sumY / 16;
    if (_centerX < 500 || _centerX > 3500) _centerX = 2048;
    if (_centerY < 500 || _centerY > 3500) _centerY = 2048;
    _isEnabled = true;
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

    // 2. Analog X/Y Reading with Auto-Calibrated Deadzone
    int xVal = analogRead(PinConfig::JOY_VRX);
    int yVal = analogRead(PinConfig::JOY_VRY);
    uint32_t nowMs = millis();

    int dx = xVal - _centerX;
    int dy = yVal - _centerY;
    constexpr int32_t DEADZONE = 800;

    JoyAction rawDir = JoyAction::None;
    if (abs(dx) > abs(dy)) {
        if (dx < -DEADZONE) rawDir = JoyAction::Left;
        else if (dx > DEADZONE) rawDir = JoyAction::Right;
    } else {
        if (dy < -DEADZONE) rawDir = JoyAction::Up;
        else if (dy > DEADZONE) rawDir = JoyAction::Down;
    }

    if (rawDir == JoyAction::None) {
        _dirHeld = false;
        return JoyAction::None;
    }

    constexpr uint32_t FIRST_DELAY_MS = 400;
    constexpr uint32_t REPEAT_RATE_MS = 250;

    if (!_dirHeld) {
        _dirHeld = true;
        _lastActionTimeMs = nowMs;
        return rawDir;
    } else if ((nowMs - _lastActionTimeMs) >= FIRST_DELAY_MS) {
        _lastActionTimeMs = nowMs - (FIRST_DELAY_MS - REPEAT_RATE_MS);
        return rawDir;
    }

    return JoyAction::None;
}

} // namespace EcuHal
