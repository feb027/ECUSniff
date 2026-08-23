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
    bool isConnected() const { return _isEnabled; }

private:
    uint32_t _lastActionTimeMs{0};
    int32_t  _centerX{2048};
    int32_t  _centerY{2048};
    bool     _btnWasDown{false};
    bool     _dirHeld{false};
    bool     _isEnabled{false};

    static constexpr int32_t ADC_THRESHOLD   = 1000;
    static constexpr uint32_t REPEAT_DELAY_MS = 400;
    static constexpr uint32_t REPEAT_RATE_MS  = 200;
};

} // namespace EcuHal
