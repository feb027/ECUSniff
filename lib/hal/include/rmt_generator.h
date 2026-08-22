#pragma once
#include <stdint.h>
#include <driver/rmt.h>
#include "pin_config.h"
#include "engine_types.h"
#include "parametric_pattern.h"

namespace EcuHal {

class RmtGenerator {
public:
    RmtGenerator();
    bool init();
    
    void setPattern(const EcuEngine::ParametricWheel& wheel, 
                    const EcuEngine::CamEventTable& cam);
    void setRpm(uint32_t targetRpm);
    
    void prepareNextCycle();
    void swapBuffer();

    void start();
    void stop();
    bool isRunning() const;
    uint32_t getActiveRpm() const;

private:
    bool _running{false};
    uint32_t _activeRpm{850};
    uint32_t _pendingRpm{850};
    bool _needsUpdate{false};

    EcuEngine::ParametricWheel _wheel;
    EcuEngine::CamEventTable   _cam;

    // Buffer CKP
    rmt_item32_t _ckpBufferA[EcuEngine::MAX_CYCLE_PULSES]{};
    rmt_item32_t _ckpBufferB[EcuEngine::MAX_CYCLE_PULSES]{};
    size_t _ckpSizeA{0};
    size_t _ckpSizeB{0};

    // Buffer CMP
    rmt_item32_t _cmpBufferA[EcuEngine::MAX_CYCLE_PULSES]{};
    rmt_item32_t _cmpBufferB[EcuEngine::MAX_CYCLE_PULSES]{};
    size_t _cmpSizeA{0};
    size_t _cmpSizeB{0};

    uint8_t _activeBufferIdx{0};

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_USB_CDC_ON_BOOT)
    static constexpr rmt_channel_t CH_CKP = RMT_CHANNEL_0;
    static constexpr rmt_channel_t CH_CMP = RMT_CHANNEL_1;
    static constexpr uint8_t MEM_BLOCKS_PER_CH = 2;
#else
    static constexpr rmt_channel_t CH_CKP = RMT_CHANNEL_0;
    static constexpr rmt_channel_t CH_CMP = RMT_CHANNEL_4;
    static constexpr uint8_t MEM_BLOCKS_PER_CH = 4;
#endif

    static constexpr uint8_t RMT_CLK_DIV = 80; // 1 tick = 1 us
};

} // namespace EcuHal
