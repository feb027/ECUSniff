#pragma once
#include <stdint.h>
#include <stddef.h>

namespace EcuHal {

enum class CaptureState : uint8_t {
    Idle = 0,
    Armed = 1,
    Recording = 2,
    Done = 3
};

struct CaptureEvent {
    uint32_t timestampUs;
    uint8_t  channel; // 0: CKP, 1: CMP, 2: CMP2
    uint8_t  level;   // 0: LOW, 1: HIGH
};

class CaptureDriver {
public:
    static constexpr size_t   MAX_CAPTURE_EVENTS = 512;
    static constexpr uint32_t GLITCH_FILTER_US   = 5; // Rejects spikes < 5us

    CaptureDriver();
    void init();
    void arm(uint16_t targetEvents = 256);
    void stop();
    void update();

    bool isDone() const { return _state == CaptureState::Done; }
    CaptureState getState() const { return _state; }
    uint16_t getEventCount() const { return _eventCount; }
    const CaptureEvent* getBuffer() const { return _buffer; }

    static void isrCkpHandler();
    static void isrCmpHandler();

private:
    static volatile CaptureState _state;
    static volatile uint16_t     _eventCount;
    static volatile uint16_t     _targetEvents;
    static volatile uint32_t     _lastCkpUs;
    static volatile uint32_t     _lastCmpUs;
    static CaptureEvent          _buffer[MAX_CAPTURE_EVENTS];
};

} // namespace EcuHal
