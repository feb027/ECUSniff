#pragma once
#include <stdint.h>
#include "pin_config.h"

namespace EcuHal {

class EncoderDriver {
public:
    EncoderDriver();
    void init();
    void read();
    int32_t getDelta();
    int32_t getValue();
    void setValue(int32_t value);
    bool isButtonPressed();

private:
    int32_t _accumulatedDelta{0};
    int32_t _totalValue{0};
};

} // namespace EcuHal
