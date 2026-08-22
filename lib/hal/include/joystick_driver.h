#pragma once
#include <stdint.h>
#include "pin_config.h"

namespace EcuHal {

enum class JoyAction : uint8_t {
    None = 0,
    Left = 1,
    Right = 2,
    Up = 3,
    Down = 4,
    Click = 5
};

class JoystickDriver {
public:
    JoystickDriver();
    void init();
    JoyAction update();

private:
    uint32_t _lastActionTimeMs{0};
    bool     _btnWasDown{false};
    bool     _dirHeld{false};

    static constexpr int32_t ADC_CENTER = 2048;
    static constexpr int32_t ADC_THRESHOLD = 900;
    static constexpr uint32_t REPEAT_DELAY_MS = 350;
    static constexpr uint32_t REPEAT_RATE_MS  = 180;
};

} // namespace EcuHal
