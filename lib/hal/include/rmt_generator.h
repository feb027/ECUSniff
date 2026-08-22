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

    // Buffer CKP (256 items per channel)
    rmt_item32_t _ckpBufferA[EcuEngine::MAX_CYCLE_PULSES]{};
    rmt_item32_t _ckpBufferB[EcuEngine::MAX_CYCLE_PULSES]{};
    size_t _ckpSizeA{0};
    size_t _ckpSizeB{0};

    // Buffer CMP (256 items per channel)
    rmt_item32_t _cmpBufferA[EcuEngine::MAX_CYCLE_PULSES]{};
    rmt_item32_t _cmpBufferB[EcuEngine::MAX_CYCLE_PULSES]{};
    size_t _cmpSizeA{0};
    size_t _cmpSizeB{0};

    uint8_t _activeBufferIdx{0};

    // Alokasi 4 block memory per channel (256 items kapasitas penuh)
    static constexpr rmt_channel_t CH_CKP = RMT_CHANNEL_0; // Blocks 0, 1, 2, 3 (256 items)
    static constexpr rmt_channel_t CH_CMP = RMT_CHANNEL_4; // Blocks 4, 5, 6, 7 (256 items)

    static constexpr uint8_t RMT_CLK_DIV = 80; // 1 tick = 1 us
};

} // namespace EcuHal
