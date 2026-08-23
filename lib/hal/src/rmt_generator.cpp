#include "rmt_generator.h"
#include <Arduino.h>

namespace EcuHal {

struct FlatPhase {
    uint16_t duration;
    uint8_t  level;
};

static EcuEngine::PulseSegment s_segments[EcuEngine::MAX_CYCLE_PULSES];
static FlatPhase s_phases[EcuEngine::MAX_CYCLE_PULSES];

RmtGenerator::RmtGenerator() {
    _wheel.totalTeeth = 36;
    _wheel.missingTeeth = 1;
    _wheel.missingPosition = 0;
    _wheel.dutyCycle = 0.5f;
    _running = false;
}

bool RmtGenerator::init() {
    Serial.println("[RMT] Initializing RMT multi-channel hardware on ESP32-S3...");

    rmt_config_t config_ckp{};
    config_ckp.rmt_mode = RMT_MODE_TX;
    config_ckp.channel = CH_CKP;
    config_ckp.gpio_num = static_cast<gpio_num_t>(PinConfig::SIG_CKP);
    config_ckp.mem_block_num = MEM_BLOCKS_CKP;
    config_ckp.clk_div = RMT_CLK_DIV;
    config_ckp.tx_config.loop_en = true;
    config_ckp.tx_config.carrier_en = false;
    config_ckp.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    config_ckp.tx_config.idle_output_en = true;

    rmt_config_t config_cmp{};
    config_cmp.rmt_mode = RMT_MODE_TX;
    config_cmp.channel = CH_CMP;
    config_cmp.gpio_num = static_cast<gpio_num_t>(PinConfig::SIG_CMP);
    config_cmp.mem_block_num = MEM_BLOCKS_CMP;
    config_cmp.clk_div = RMT_CLK_DIV;
    config_cmp.tx_config.loop_en = true;
    config_cmp.tx_config.carrier_en = false;
    config_cmp.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    config_cmp.tx_config.idle_output_en = true;

    rmt_config(&config_ckp);
    rmt_driver_install(CH_CKP, 0, 0);

    rmt_config(&config_cmp);
    rmt_driver_install(CH_CMP, 0, 0);

    rmt_set_tx_loop_mode(CH_CKP, true);
    rmt_set_tx_loop_mode(CH_CMP, true);

    _needsUpdate = true;
    prepareNextCycle();
    swapBuffer();

    stop();
    Serial.println("[RMT] Multi-channel RMT Hardware Ready (CH0 CKP + CH3 CMP).");
    return true;
}

void RmtGenerator::setPattern(const EcuEngine::ParametricWheel& wheel, 
                             const EcuEngine::CamEventTable& cam) {
    _wheel = wheel;
    _cam = cam;
    _needsUpdate = true;
}

void RmtGenerator::setRpm(uint32_t targetRpm) {
    if (targetRpm != _pendingRpm) {
        _pendingRpm = targetRpm;
        _needsUpdate = true;
    }
}

void RmtGenerator::prepareNextCycle() {
    if (!_needsUpdate || _pendingRpm == 0) return;

    // 1. Generate CKP (720 deg)
    size_t ckpCount = EcuEngine::ParametricEngine::generateCkpCycle(
        _wheel, _pendingRpm, s_segments, EcuEngine::MAX_CYCLE_PULSES);

    if (ckpCount > 0) {
        size_t phaseCount = 0;
        for (size_t i = 0; i < ckpCount && phaseCount < (EcuEngine::MAX_CYCLE_PULSES - 4); ++i) {
            uint32_t rem0 = s_segments[i].duration0Us;
            uint8_t lvl0 = s_segments[i].level0;
            while (rem0 > 0 && phaseCount < (EcuEngine::MAX_CYCLE_PULSES - 4)) {
                uint16_t d = (rem0 > 30000) ? 30000 : static_cast<uint16_t>(rem0);
                s_phases[phaseCount++] = { d, lvl0 };
                rem0 -= d;
            }
            uint32_t rem1 = s_segments[i].duration1Us;
            uint8_t lvl1 = s_segments[i].level1;
            while (rem1 > 0 && phaseCount < (EcuEngine::MAX_CYCLE_PULSES - 4)) {
                uint16_t d = (rem1 > 30000) ? 30000 : static_cast<uint16_t>(rem1);
                s_phases[phaseCount++] = { d, lvl1 };
                rem1 -= d;
            }
        }

        rmt_item32_t* targetCkp = (_activeBufferIdx == 0) ? _ckpBufferB : _ckpBufferA;
        size_t outCkp = 0;
        for (size_t i = 0; i < phaseCount && outCkp < (EcuEngine::MAX_CYCLE_PULSES - 1); i += 2) {
            rmt_item32_t item{};
            item.duration0 = s_phases[i].duration;
            item.level0    = s_phases[i].level;
            if (i + 1 < phaseCount) {
                item.duration1 = s_phases[i + 1].duration;
                item.level1    = s_phases[i + 1].level;
            } else {
                item.duration1 = 0;
                item.level1    = 0;
            }
            targetCkp[outCkp++] = item;
        }
        targetCkp[outCkp] = rmt_item32_t{0, 0, 0, 0}; // Final EOT
        if (_activeBufferIdx == 0) _ckpSizeB = outCkp + 1;
        else _ckpSizeA = outCkp + 1;
    }

    // 2. Generate CMP (720 deg)
    size_t cmpCount = EcuEngine::ParametricEngine::generateCmpCycle(
        _cam, _pendingRpm, s_segments, EcuEngine::MAX_CYCLE_PULSES);

    if (cmpCount > 0) {
        size_t phaseCount = 0;
        for (size_t i = 0; i < cmpCount && phaseCount < (EcuEngine::MAX_CYCLE_PULSES - 4); ++i) {
            uint32_t rem = s_segments[i].duration0Us;
            uint8_t lvl = s_segments[i].level0;
            while (rem > 0 && phaseCount < (EcuEngine::MAX_CYCLE_PULSES - 4)) {
                uint16_t d = (rem > 30000) ? 30000 : static_cast<uint16_t>(rem);
                s_phases[phaseCount++] = { d, lvl };
                rem -= d;
            }
            if (s_segments[i].duration1Us > 0) {
                rem = s_segments[i].duration1Us;
                lvl = s_segments[i].level1;
                while (rem > 0 && phaseCount < (EcuEngine::MAX_CYCLE_PULSES - 4)) {
                    uint16_t d = (rem > 30000) ? 30000 : static_cast<uint16_t>(rem);
                    s_phases[phaseCount++] = { d, lvl };
                    rem -= d;
                }
            }
        }

        rmt_item32_t* targetCmp = (_activeBufferIdx == 0) ? _cmpBufferB : _cmpBufferA;
        size_t outCmp = 0;
        for (size_t i = 0; i < phaseCount && outCmp < (EcuEngine::MAX_CYCLE_PULSES - 1); i += 2) {
            rmt_item32_t item{};
            item.duration0 = s_phases[i].duration;
            item.level0    = s_phases[i].level;
            if (i + 1 < phaseCount) {
                item.duration1 = s_phases[i + 1].duration;
                item.level1    = s_phases[i + 1].level;
            } else {
                item.duration1 = 0;
                item.level1    = 0;
            }
            targetCmp[outCmp++] = item;
        }
        targetCmp[outCmp] = rmt_item32_t{0, 0, 0, 0}; // Final EOT
        if (_activeBufferIdx == 0) _cmpSizeB = outCmp + 1;
        else _cmpSizeA = outCmp + 1;
    }
}

void RmtGenerator::swapBuffer() {
    if (!_needsUpdate) return;

    uint8_t nextIdx = (_activeBufferIdx == 0) ? 1 : 0;
    rmt_item32_t* activeCkp = (nextIdx == 1) ? _ckpBufferB : _ckpBufferA;
    size_t activeCkpSize   = (nextIdx == 1) ? _ckpSizeB   : _ckpSizeA;

    rmt_item32_t* activeCmp = (nextIdx == 1) ? _cmpBufferB : _cmpBufferA;
    size_t activeCmpSize   = (nextIdx == 1) ? _cmpSizeB   : _cmpSizeA;

    if (activeCkpSize > 0) {
        rmt_fill_tx_items(CH_CKP, activeCkp, activeCkpSize, 0);
    }
    if (activeCmpSize > 0) {
        rmt_fill_tx_items(CH_CMP, activeCmp, activeCmpSize, 0);
    }

    _activeBufferIdx = nextIdx;
    _activeRpm = _pendingRpm;
    _needsUpdate = false;
}

void RmtGenerator::start() {
    uint8_t currentIdx = _activeBufferIdx;
    rmt_item32_t* activeCkp = (currentIdx == 1) ? _ckpBufferB : _ckpBufferA;
    size_t activeCkpSize   = (currentIdx == 1) ? _ckpSizeB   : _ckpSizeA;

    rmt_item32_t* activeCmp = (currentIdx == 1) ? _cmpBufferB : _cmpBufferA;
    size_t activeCmpSize   = (currentIdx == 1) ? _cmpSizeB   : _cmpSizeA;

    rmt_tx_stop(CH_CKP);
    rmt_tx_stop(CH_CMP);

    if (activeCkpSize > 0) {
        rmt_fill_tx_items(CH_CKP, activeCkp, activeCkpSize, 0);
        rmt_set_tx_loop_mode(CH_CKP, true);
    }
    if (activeCmpSize > 0) {
        rmt_fill_tx_items(CH_CMP, activeCmp, activeCmpSize, 0);
        rmt_set_tx_loop_mode(CH_CMP, true);
    }

    if (activeCkpSize > 0) rmt_tx_start(CH_CKP, true);
    if (activeCmpSize > 0) rmt_tx_start(CH_CMP, true);

    _running = true;
    Serial.println("[RMT] Multi-channel continuous loop STARTED.");
}

void RmtGenerator::stop() {
    rmt_tx_stop(CH_CKP);
    rmt_tx_stop(CH_CMP);
    _running = false;
    Serial.println("[RMT] Multi-channel continuous loop STOPPED.");
}

bool RmtGenerator::isRunning() const {
    return _running;
}

uint32_t RmtGenerator::getActiveRpm() const {
    return _activeRpm;
}

} // namespace EcuHal
