#include "capture_driver.h"
#include <Arduino.h>
#include "pin_config.h"

namespace EcuHal {

volatile CaptureState CaptureDriver::_state = CaptureState::Idle;
volatile uint16_t     CaptureDriver::_eventCount = 0;
volatile uint16_t     CaptureDriver::_targetEvents = 384;
volatile uint32_t     CaptureDriver::_armTimeMs = 0;
volatile uint32_t     CaptureDriver::_lastCkpUs = 0;
volatile uint32_t     CaptureDriver::_lastCmpUs = 0;
volatile uint32_t     CaptureDriver::_lastCkpRisingUs = 0;
volatile uint32_t     CaptureDriver::_liveLastGapUs = 0;
volatile uint32_t     CaptureDriver::_liveRevPeriodUs = 0;
volatile uint32_t     CaptureDriver::_liveNominalUs = 0;
volatile uint16_t     CaptureDriver::_liveTeethCount = 0;
volatile uint16_t     CaptureDriver::_liveCkpEdgesSec = 0;
volatile uint16_t     CaptureDriver::_liveCmpEdgesSec = 0;
volatile uint32_t     CaptureDriver::_lastRateCheckMs = 0;
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
    _armTimeMs = millis();
    _state = CaptureState::Armed;
}

void CaptureDriver::stop() {
    _state = CaptureState::Idle;
}

void CaptureDriver::update() {
    uint32_t nowMs = millis();
    if (_state == CaptureState::Armed) {
        if ((nowMs - _armTimeMs) > 2500) {
            _state = CaptureState::Done;
        }
    } else if (_state == CaptureState::Recording) {
        uint32_t nowUs = micros();
        uint32_t lastPulseUs = (_lastCkpUs > _lastCmpUs) ? _lastCkpUs : _lastCmpUs;
        if (lastPulseUs != 0 && (nowUs - lastPulseUs) > 300000) {
            _state = CaptureState::Done;
        }
    }
}

void CaptureDriver::getLiveMetrics(LiveSignalMetrics& outMetrics) {
    uint32_t nowUs = micros();
    outMetrics.ckpActive = (_lastCkpUs != 0 && (nowUs - _lastCkpUs) < 150000);
    outMetrics.cmpActive = (_lastCmpUs != 0 && (nowUs - _lastCmpUs) < 300000);
    outMetrics.cmp2Active = false;
    outMetrics.ckpRateHz = _liveCkpEdgesSec;
    outMetrics.cmpRateHz = _liveCmpEdgesSec;
    outMetrics.lastGapUs = _liveLastGapUs;
    outMetrics.revPeriodUs = _liveRevPeriodUs;
    outMetrics.nominalPeriodUs = _liveNominalUs;
    outMetrics.teethPerRev = _liveTeethCount;
}

void IRAM_ATTR CaptureDriver::isrCkpHandler() {
    uint32_t now = micros();
    if (_lastCkpUs != 0 && (now - _lastCkpUs) < GLITCH_FILTER_US) return;
    _lastCkpUs = now;

    if (_state == CaptureState::Idle || _state == CaptureState::Done) {
        return; // ZERO overhead when not recording
    }

    uint8_t lvl = (REG_READ(GPIO_IN1_REG) >> (PinConfig::CAP_CKP - 32)) & 0x01;

    if (_state == CaptureState::Armed) {
        _state = CaptureState::Recording;
    }

    if (_state == CaptureState::Recording) {
        if (_eventCount < MAX_CAPTURE_EVENTS) {
            _buffer[_eventCount++] = { now, 0, lvl };
            if (_eventCount >= _targetEvents) _state = CaptureState::Done;
        }
    }
}

void IRAM_ATTR CaptureDriver::isrCmpHandler() {
    uint32_t now = micros();
    if (_lastCmpUs != 0 && (now - _lastCmpUs) < GLITCH_FILTER_US) return;
    _lastCmpUs = now;

    if (_state == CaptureState::Idle || _state == CaptureState::Done) {
        return; // ZERO overhead when not recording
    }

    uint8_t lvl = (REG_READ(GPIO_IN1_REG) >> (PinConfig::CAP_CMP - 32)) & 0x01;

    if (_state == CaptureState::Armed) {
        _state = CaptureState::Recording;
    }

    if (_state == CaptureState::Recording) {
        if (_eventCount < MAX_CAPTURE_EVENTS) {
            _buffer[_eventCount++] = { now, 1, lvl };
            if (_eventCount >= _targetEvents) _state = CaptureState::Done;
        }
    }
}

} // namespace EcuHal
