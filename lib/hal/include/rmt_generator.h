#pragma once
#include <stdint.h>
#include <stddef.h>
#include <driver/rmt.h>
#include "pin_config.h"
#include "engine_types.h"
#include "parametric_pattern.h"
#include "pattern_types.h"

// Forward declaration of master WheelDefinition
struct WheelDefinition;

namespace EcuHal {

class RmtGenerator {
public:
    RmtGenerator();
    bool init();
    
    // Pattern selection
    void setPattern(const EcuEngine::ParametricWheel& wheel, 
                    const EcuEngine::CamEventTable& cam);
    bool setWheelPattern(const WheelDefinition* wheel);
    void setRpm(uint32_t targetRpm);
    
    void prepareNextCycle();
    void prepareBitArrayCycle();
    void swapBuffer();

    void start();
    void stop();
    bool isRunning() const;
    uint32_t getActiveRpm() const;

    const WheelDefinition* getActiveWheel() const { return _activeWheel; }
    bool isBitArrayMode() const { return _isBitArrayMode; }

    /**
     * @brief Converts an arbitrary bit-array sequence into RMT symbols with RLE compression and duration slicing.
     * @param bitArray Pointer to PROGMEM or RAM bit-array (bit0=CKP, bit1=CMP1, bit2=CMP2).
     * @param totalEdges Number of segments in the bit-array.
     * @param cycleDegrees Engine cycle degrees (360 or 720).
     * @param rpm Engine speed in RPM.
     * @param channelBitMask Bitmask to extract (0x01 for CKP, 0x02 for CMP1, 0x04 for CMP2).
     * @param outItems Output array of rmt_item32_t symbols.
     * @param maxItems Maximum capacity of outItems array.
     * @return Number of rmt_item32_t symbols written (including EOT {0,0,0,0}), or 0 on error.
     */
    static size_t compileBitArrayToRmt(
        const uint8_t* bitArray,
        uint16_t totalEdges,
        uint16_t cycleDegrees,
        uint32_t rpm,
        uint8_t channelBitMask,
        rmt_item32_t* outItems,
        size_t maxItems
    );

    static constexpr uint16_t MAX_RMT_DURATION_CHUNK = 30000; // <= 30,000 us to prevent 15-bit overflow

private:
    bool _running{false};
    uint32_t _activeRpm{850};
    uint32_t _pendingRpm{850};
    bool _needsUpdate{false};
    bool _isBitArrayMode{false};
    uint32_t _cycleStartUs{0};
    uint32_t _activeCycleUs{141176}; // 720 deg @ 850 RPM (~141.176 ms)

    const WheelDefinition*     _activeWheel{nullptr};
    EcuEngine::ParametricWheel _wheel;
    EcuEngine::CamEventTable   _cam;

    // Buffer CKP (Channel 0 / Primary Trigger)
    rmt_item32_t _ckpBufferA[EcuEngine::MAX_CYCLE_PULSES]{};
    rmt_item32_t _ckpBufferB[EcuEngine::MAX_CYCLE_PULSES]{};
    size_t _ckpSizeA{0};
    size_t _ckpSizeB{0};

    // Buffer CMP1 (Channel 2 / Secondary Trigger)
    rmt_item32_t _cmpBufferA[EcuEngine::MAX_CYCLE_PULSES]{};
    rmt_item32_t _cmpBufferB[EcuEngine::MAX_CYCLE_PULSES]{};
    size_t _cmpSizeA{0};
    size_t _cmpSizeB{0};

    // Buffer CMP2 (Channel 3 / Tertiary Trigger / Dual VVT)
    rmt_item32_t _cmp2BufferA[EcuEngine::MAX_CYCLE_PULSES]{};
    rmt_item32_t _cmp2BufferB[EcuEngine::MAX_CYCLE_PULSES]{};
    size_t _cmp2SizeA{0};
    size_t _cmp2SizeB{0};

    uint8_t _activeBufferIdx{0};

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_USB_CDC_ON_BOOT) || defined(ESP32S3)
    static constexpr rmt_channel_t CH_CKP  = RMT_CHANNEL_0;
    static constexpr rmt_channel_t CH_CMP  = RMT_CHANNEL_2;
    static constexpr rmt_channel_t CH_CMP2 = RMT_CHANNEL_3;
    static constexpr uint8_t MEM_BLOCKS_CKP  = 2;
    static constexpr uint8_t MEM_BLOCKS_CMP  = 1;
    static constexpr uint8_t MEM_BLOCKS_CMP2 = 1;
#else
    static constexpr rmt_channel_t CH_CKP  = RMT_CHANNEL_0;
    static constexpr rmt_channel_t CH_CMP  = RMT_CHANNEL_2;
    static constexpr rmt_channel_t CH_CMP2 = RMT_CHANNEL_4;
    static constexpr uint8_t MEM_BLOCKS_CKP  = 2;
    static constexpr uint8_t MEM_BLOCKS_CMP  = 1;
    static constexpr uint8_t MEM_BLOCKS_CMP2 = 1;
#endif

    static constexpr uint8_t RMT_CLK_DIV = 80; // 1 tick = 1 us
};

} // namespace EcuHal
