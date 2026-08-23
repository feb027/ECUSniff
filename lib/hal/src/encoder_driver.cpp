#include "encoder_driver.h"
#include <Arduino.h>

namespace EcuHal {

static volatile int32_t s_encoderTicks = 0;
static volatile uint8_t s_prevPinState = 0;

static void IRAM_ATTR encoderISR() {
    uint8_t a = digitalRead(PinConfig::ENC_CLK);
    uint8_t b = digitalRead(PinConfig::ENC_DT);
    uint8_t curr = (a << 1) | b;
    
    if (curr != s_prevPinState) {
        static const int8_t DIR_TABLE[16] = {
             0, -1,  1,  0,
             1,  0,  0, -1,
            -1,  0,  0,  1,
             0,  1, -1,  0
        };
        s_encoderTicks += DIR_TABLE[(s_prevPinState << 2) | curr];
        s_prevPinState = curr;
    }
}

EncoderDriver::EncoderDriver() {}

void EncoderDriver::init() {
    pinMode(PinConfig::ENC_CLK, INPUT_PULLUP);
    pinMode(PinConfig::ENC_DT,  INPUT_PULLUP);
    pinMode(PinConfig::ENC_SW,  INPUT_PULLUP);

    uint8_t a = digitalRead(PinConfig::ENC_CLK);
    uint8_t b = digitalRead(PinConfig::ENC_DT);
    s_prevPinState = (a << 1) | b;

    attachInterrupt(digitalPinToInterrupt(PinConfig::ENC_CLK), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PinConfig::ENC_DT),  encoderISR, CHANGE);

    Serial.println("[ENCODER] Direct Hardware ISR Quadrature Decoder initialized.");
}

void EncoderDriver::read() {
    noInterrupts();
    int32_t ticks = s_encoderTicks;
    
    // Setiap 1 klik fisik detent mekanis EC11 = 4 transisi logika kuadratur
    if (ticks >= 4 || ticks <= -4) {
        _accumulatedDelta = ticks / 4;
        s_encoderTicks = ticks % 4;
    } else {
        _accumulatedDelta = 0;
    }
    interrupts();

    _totalValue += _accumulatedDelta;
}

int32_t EncoderDriver::getDelta() {
    return _accumulatedDelta;
}

int32_t EncoderDriver::getValue() {
    return _totalValue;
}

void EncoderDriver::setValue(int32_t value) {
    _totalValue = value;
    _accumulatedDelta = 0;
}

bool EncoderDriver::isButtonPressed() {
    return (digitalRead(PinConfig::ENC_SW) == LOW);
}

} // namespace EcuHal
