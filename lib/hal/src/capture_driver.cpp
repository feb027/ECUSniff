#include "capture_driver.h"
#include <Arduino.h>
#include "pin_config.h"

namespace EcuHal {

volatile CaptureState CaptureDriver::_state = CaptureState::Idle;
volatile uint16_t     CaptureDriver::_eventCount = 0;
volatile uint16_t     CaptureDriver::_targetEvents = 256;
volatile uint32_t     CaptureDriver::_lastCkpUs = 0;
volatile uint32_t     CaptureDriver::_lastCmpUs = 0;
CaptureEvent          CaptureDriver::_buffer[CaptureDriver::MAX_CAPTURE_EVENTS];

CaptureDriver::CaptureDriver() {}

void CaptureDriver::init() {
    pinMode(PinConfig::CAP_CKP, INPUT);
    pinMode(PinConfig::CAP_CMP, INPUT);

    attachInterrupt(digitalPinToInterrupt(PinConfig::CAP_CKP), isrCkpHandler, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PinConfig::CAP_CMP), isrCmpHandler, CHANGE);
    _state = CaptureState::Idle;
    _eventCount = 0;
}

void CaptureDriver::arm(uint16_t targetEvents) {
    if (targetEvents > MAX_CAPTURE_EVENTS) targetEvents = MAX_CAPTURE_EVENTS;
    _targetEvents = targetEvents;
    _eventCount = 0;
    _lastCkpUs = 0;
    _lastCmpUs = 0;
    _state = CaptureState::Armed;
}

void CaptureDriver::stop() {
    _state = CaptureState::Idle;
}

void IRAM_ATTR CaptureDriver::isrCkpHandler() {
    if (_state == CaptureState::Idle || _state == CaptureState::Done) return;

    uint32_t now = micros();
    if (_lastCkpUs != 0 && (now - _lastCkpUs) < GLITCH_FILTER_US) return; // Glitch filter
    _lastCkpUs = now;

    uint8_t lvl = digitalRead(PinConfig::CAP_CKP);

    if (_state == CaptureState::Armed) {
        _state = CaptureState::Recording;
    }

    if (_state == CaptureState::Recording) {
        if (_eventCount < MAX_CAPTURE_EVENTS) {
            _buffer[_eventCount] = { now, 0, lvl };
            _eventCount++;
            if (_eventCount >= _targetEvents) {
                _state = CaptureState::Done;
            }
        }
    }
}

void IRAM_ATTR CaptureDriver::isrCmpHandler() {
    if (_state != CaptureState::Recording) return;

    uint32_t now = micros();
    if (_lastCmpUs != 0 && (now - _lastCmpUs) < GLITCH_FILTER_US) return; // Glitch filter
    _lastCmpUs = now;

    uint8_t lvl = digitalRead(PinConfig::CAP_CMP);

    if (_eventCount < MAX_CAPTURE_EVENTS) {
        _buffer[_eventCount] = { now, 1, lvl };
        _eventCount++;
        if (_eventCount >= _targetEvents) {
            _state = CaptureState::Done;
        }
    }
}

} // namespace EcuHal
