#include "capture_driver.h"
#include <Arduino.h>
#include "pin_config.h"

namespace EcuHal {

volatile CaptureState CaptureDriver::_state = CaptureState::Idle;
volatile uint16_t     CaptureDriver::_eventCount = 0;
volatile uint16_t     CaptureDriver::_targetEvents = 512;
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

static volatile uint32_t s_lastGapTimestampUs = 0;
static volatile uint32_t s_prevDtUs = 0;
static volatile uint16_t s_runningToothCount = 0;
static volatile bool     s_lastWasGap = false;
CaptureEvent          CaptureDriver::_buffer[CaptureDriver::MAX_CAPTURE_EVENTS];

CaptureDriver::CaptureDriver() {}

void CaptureDriver::init() {
    pinMode(PinConfig::CAP_CKP, INPUT_PULLDOWN);
    pinMode(PinConfig::CAP_CMP, INPUT_PULLDOWN);
    pinMode(PinConfig::CAP_CMP2, INPUT_PULLDOWN);

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
    _lastCkpRisingUs = 0;
    _liveNominalUs = 0;
    s_prevDtUs = 0;
    s_lastGapTimestampUs = 0;
    s_runningToothCount = 0;
    s_lastWasGap = false;
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

    uint8_t lvl = static_cast<uint8_t>(digitalRead(PinConfig::CAP_CKP));

    if (lvl == 1) {
        if (_lastCkpRisingUs != 0) {
            uint32_t dt = now - _lastCkpRisingUs;
            if (dt > 30 && dt < 1200000) {
                if (s_prevDtUs > 0) {
                    if (s_lastWasGap) {
                        _liveNominalUs = dt;
                        s_runningToothCount = 1;
                        s_lastWasGap = false;
                    } else if (dt >= ((s_prevDtUs * 140) / 100)) {
                        _liveLastGapUs = dt;
                        if (s_lastGapTimestampUs != 0) {
                            uint32_t revP = now - s_lastGapTimestampUs;
                            if (revP >= 1000 && revP <= 1200000) {
                                _liveRevPeriodUs = revP;
                            }
                        }
                        s_lastGapTimestampUs = now;
                        uint16_t missingGuess = (dt >= ((s_prevDtUs * 240) / 100)) ? 2 : 1;
                        _liveTeethCount = s_runningToothCount + missingGuess;
                        s_runningToothCount = 0;
                        s_lastWasGap = true;

                        if (_state == CaptureState::Armed) {
                            _state = CaptureState::Recording;
                            _eventCount = 0;
                        }
                    } else {
                        _liveNominalUs = (_liveNominalUs > 0) ? ((_liveNominalUs * 3 + dt) >> 2) : dt;
                        s_runningToothCount++;
                        s_lastWasGap = false;
                        if (s_runningToothCount > 130) {
                            s_runningToothCount = 0;
                            s_lastGapTimestampUs = 0;
                        }
                    }
                }
                s_prevDtUs = dt;
            }
        }
        _lastCkpRisingUs = now;
    }
    _lastCkpUs = now;

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

    uint8_t lvl = static_cast<uint8_t>(digitalRead(PinConfig::CAP_CMP));

    if (_state == CaptureState::Recording) {
        if (_eventCount < MAX_CAPTURE_EVENTS) {
            _buffer[_eventCount++] = { now, 1, lvl };
            if (_eventCount >= _targetEvents) _state = CaptureState::Done;
        }
    }
}

} // namespace EcuHal
