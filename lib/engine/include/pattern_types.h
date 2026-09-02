#pragma once
#include <stdint.h>
#include <stddef.h>

namespace EcuEngine {

/**
 * @brief Signal channel bitmasks for trigger wheel bit-arrays.
 * Each byte in the PROGMEM pattern array encodes simultaneous logic states:
 * - Bit 0 (0x01): Crankshaft (CKP / Primary Trigger)
 * - Bit 1 (0x02): Camshaft 1 (CMP1 / Secondary Trigger)
 * - Bit 2 (0x04): Camshaft 2 (CMP2 / Tertiary Trigger / Dual VVT)
 * - Bit 3 (0x08): Auxiliary / Knock Trigger (Reserved)
 */
constexpr uint8_t SIGNAL_BIT_CKP   = 0x01;
constexpr uint8_t SIGNAL_BIT_CMP1  = 0x02;
constexpr uint8_t SIGNAL_BIT_CMP2  = 0x04;
constexpr uint8_t SIGNAL_BIT_KNOCK = 0x08;

/**
 * @brief Represents a single pulse transition duration (in microseconds) and logic level.
 */
struct PulseTransition {
    uint32_t durationUs{0};
    bool     level{false};
};

} // namespace EcuEngine
