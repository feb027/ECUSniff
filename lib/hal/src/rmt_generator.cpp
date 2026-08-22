#include "rmt_generator.h"
#include <Arduino.h>

namespace EcuHal {

RmtGenerator::RmtGenerator() {
    _wheel.totalTeeth = 36;
    _wheel.missingTeeth = 1;
    _wheel.missingPosition = 0;
    _wheel.dutyCycle = 0.5f;
    _running = false;
}

bool RmtGenerator::init() {
    Serial.println("[RMT] Initializing RMT multi-channel hardware (CKP + CMP)...");

    rmt_config_t config_ckp{};
    config_ckp.rmt_mode = RMT_MODE_TX;
    config_ckp.channel = CH_CKP;
    config_ckp.gpio_num = static_cast<gpio_num_t>(PinConfig::SIG_CKP);
    config_ckp.mem_block_num = 2;
    config_ckp.clk_div = RMT_CLK_DIV;
    config_ckp.tx_config.loop_en = true;
    config_ckp.tx_config.carrier_en = false;
    config_ckp.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    config_ckp.tx_config.idle_output_en = true;

    rmt_config_t config_cmp = config_ckp;
    config_cmp.channel = CH_CMP;
    config_cmp.gpio_num = static_cast<gpio_num_t>(PinConfig::SIG_CMP);
    config_cmp.mem_block_num = 2;

    rmt_config_t config_cmp2 = config_ckp;
    config_cmp2.channel = CH_CMP2;
    config_cmp2.gpio_num = static_cast<gpio_num_t>(PinConfig::SIG_CMP2);
    config_cmp2.mem_block_num = 2;

    rmt_config(&config_ckp);
    rmt_driver_install(CH_CKP, 0, 0);

    rmt_config(&config_cmp);
    rmt_driver_install(CH_CMP, 0, 0);

    rmt_config(&config_cmp2);
    rmt_driver_install(CH_CMP2, 0, 0);

    rmt_set_tx_loop_mode(CH_CKP, true);
    rmt_set_tx_loop_mode(CH_CMP, true);
    rmt_set_tx_loop_mode(CH_CMP2, true);

    _needsUpdate = true;
    prepareNextCycle();
    swapBuffer();

    stop();
    Serial.println("[RMT] Multi-channel RMT Initialized (CKP: GPIO 25, CMP: GPIO 26).");
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

    EcuEngine::PulseSegment segments[EcuEngine::MAX_CYCLE_PULSES];

    // 1. Generate CKP (360 deg)
    size_t ckpCount = EcuEngine::ParametricEngine::generateCkpCycle(
        _wheel, _pendingRpm, segments, EcuEngine::MAX_CYCLE_PULSES);

    if (ckpCount > 0) {
        rmt_item32_t* targetCkp = (_activeBufferIdx == 0) ? _ckpBufferB : _ckpBufferA;
        size_t outCkp = 0;
        for (size_t i = 0; i < ckpCount && outCkp < (EcuEngine::MAX_CYCLE_PULSES - 2); ++i) {
            uint32_t d0 = segments[i].duration0Us;
            uint8_t lvl0 = segments[i].level0;
            uint32_t d1 = segments[i].duration1Us;
            uint8_t lvl1 = segments[i].level1;

            if (d0 <= 30000 && d1 <= 30000) {
                targetCkp[outCkp++] = rmt_item32_t{ static_cast<uint16_t>(d0), lvl0, static_cast<uint16_t>(d1), lvl1 };
            } else {
                while (d0 > 0 && outCkp < (EcuEngine::MAX_CYCLE_PULSES - 2)) {
                    uint16_t d = (d0 > 30000) ? 30000 : static_cast<uint16_t>(d0);
                    targetCkp[outCkp++] = rmt_item32_t{ d, lvl0, 0, 0 };
                    d0 -= d;
                }
                while (d1 > 0 && outCkp < (EcuEngine::MAX_CYCLE_PULSES - 2)) {
                    uint16_t d = (d1 > 30000) ? 30000 : static_cast<uint16_t>(d1);
                    targetCkp[outCkp++] = rmt_item32_t{ d, lvl1, 0, 0 };
                    d1 -= d;
                }
            }
        }
        targetCkp[outCkp] = rmt_item32_t{0, 0, 0, 0}; // EOT
        if (_activeBufferIdx == 0) _ckpSizeB = outCkp + 1;
        else _ckpSizeA = outCkp + 1;
    }

    // 2. Generate CMP (720 deg) with 15-bit safe chunking
    size_t cmpCount = EcuEngine::ParametricEngine::generateCmpCycle(
        _cam, _pendingRpm, segments, EcuEngine::MAX_CYCLE_PULSES);

    if (cmpCount > 0) {
        rmt_item32_t* targetCmp = (_activeBufferIdx == 0) ? _cmpBufferB : _cmpBufferA;
        size_t outCmp = 0;
        for (size_t i = 0; i < cmpCount && outCmp < (EcuEngine::MAX_CYCLE_PULSES - 2); ++i) {
            uint32_t rem = segments[i].duration0Us;
            uint8_t lvl = segments[i].level0;
            while (rem > 0 && outCmp < (EcuEngine::MAX_CYCLE_PULSES - 2)) {
                uint16_t d = (rem > 30000) ? 30000 : static_cast<uint16_t>(rem);
                targetCmp[outCmp++] = rmt_item32_t{ d, lvl, 0, 0 };
                rem -= d;
            }
            if (segments[i].duration1Us > 0) {
                rem = segments[i].duration1Us;
                lvl = segments[i].level1;
                while (rem > 0 && outCmp < (EcuEngine::MAX_CYCLE_PULSES - 2)) {
                    uint16_t d = (rem > 30000) ? 30000 : static_cast<uint16_t>(rem);
                    targetCmp[outCmp++] = rmt_item32_t{ d, lvl, 0, 0 };
                    rem -= d;
                }
            }
        }
        targetCmp[outCmp] = rmt_item32_t{0, 0, 0, 0}; // EOT
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

    if (activeCkpSize > 0) {
        rmt_fill_tx_items(CH_CKP, activeCkp, activeCkpSize, 0);
        rmt_set_tx_loop_mode(CH_CKP, true);
        rmt_tx_start(CH_CKP, true);
    }
    if (activeCmpSize > 0) {
        rmt_fill_tx_items(CH_CMP, activeCmp, activeCmpSize, 0);
        rmt_set_tx_loop_mode(CH_CMP, true);
        rmt_tx_start(CH_CMP, true);
    }

    _running = true;
    Serial.println("[RMT] Multi-channel continuous loop STARTED.");
}

void RmtGenerator::stop() {
    rmt_tx_stop(CH_CKP);
    rmt_tx_stop(CH_CMP);
    rmt_tx_stop(CH_CMP2);
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
