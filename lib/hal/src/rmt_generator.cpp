#include "rmt_generator.h"
#include "../../engine/include/wheel_database.h"
#include <Arduino.h>
#include <string.h>

namespace EcuHal {

struct FlatPhase {
    uint16_t duration;
    uint8_t  level;
};

static EcuEngine::PulseSegment s_segments[EcuEngine::MAX_CYCLE_PULSES];
static FlatPhase s_phases[EcuEngine::MAX_CYCLE_PULSES * 2];
static uint8_t s_replicated360[2048];

RmtGenerator::RmtGenerator() {
    _wheel.totalTeeth = 36;
    _wheel.missingTeeth = 1;
    _wheel.missingPosition = 0;
    _wheel.dutyCycle = 0.5f;
    _running = false;
    _isBitArrayMode = false;
    _activeWheel = nullptr;
}

bool RmtGenerator::init() {
    Serial.println("[RMT] Initializing RMT multi-channel hardware on ESP32-S3 (CKP, CMP1, CMP2)...");

    // 1. Configure CKP (Channel 0 / Primary Crank Trigger)
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

    // 2. Configure CMP1 (Channel 2 / Secondary Cam Trigger)
    rmt_config_t config_cmp1{};
    config_cmp1.rmt_mode = RMT_MODE_TX;
    config_cmp1.channel = CH_CMP;
    config_cmp1.gpio_num = static_cast<gpio_num_t>(PinConfig::SIG_CMP);
    config_cmp1.mem_block_num = MEM_BLOCKS_CMP;
    config_cmp1.clk_div = RMT_CLK_DIV;
    config_cmp1.tx_config.loop_en = true;
    config_cmp1.tx_config.carrier_en = false;
    config_cmp1.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    config_cmp1.tx_config.idle_output_en = true;

    // 3. Configure CMP2 (Channel 3 / Tertiary Cam Trigger / Dual VVT)
    rmt_config_t config_cmp2{};
    config_cmp2.rmt_mode = RMT_MODE_TX;
    config_cmp2.channel = CH_CMP2;
    config_cmp2.gpio_num = static_cast<gpio_num_t>(PinConfig::SIG_CMP2);
    config_cmp2.mem_block_num = MEM_BLOCKS_CMP2;
    config_cmp2.clk_div = RMT_CLK_DIV;
    config_cmp2.tx_config.loop_en = true;
    config_cmp2.tx_config.carrier_en = false;
    config_cmp2.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    config_cmp2.tx_config.idle_output_en = true;

    rmt_config(&config_ckp);
    rmt_driver_install(CH_CKP, 0, 0);

    rmt_config(&config_cmp1);
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
    Serial.println("[RMT] Multi-channel RMT Hardware Ready (CH0 CKP + CH2 CMP1 + CH3 CMP2).");
    return true;
}

void RmtGenerator::setPattern(const EcuEngine::ParametricWheel& wheel, 
                             const EcuEngine::CamEventTable& cam) {
    if (!_isBitArrayMode && _activeWheel == nullptr &&
        _wheel.totalTeeth == wheel.totalTeeth &&
        _wheel.missingTeeth == wheel.missingTeeth &&
        _wheel.missingPosition == wheel.missingPosition &&
        _wheel.dutyCycle == wheel.dutyCycle) {
        return; // Pola parametrik sama, tidak perlu reload RMT
    }
    _wheel = wheel;
    _cam = cam;
    _activeWheel = nullptr;
    _isBitArrayMode = false;
    _patternChanged = true;
    _needsUpdate = true;
}

bool RmtGenerator::setWheelPattern(const WheelDefinition* wheel) {
    if (wheel == nullptr) return false;
    if (_isBitArrayMode && _activeWheel == wheel) {
        return true; // Pola preset sama persis, jangan reset RMT
    }
    _activeWheel = wheel;
    _isBitArrayMode = true;
    _patternChanged = true;
    _needsUpdate = true;
    return true;
}

void RmtGenerator::setChannelEnables(bool ckp, bool cmp1, bool cmp2, bool inverted) {
    if (_ckpEnabled != ckp || _cmp1Enabled != cmp1 || _cmp2Enabled != cmp2 || _inverted != inverted) {
        _ckpEnabled = ckp;
        _cmp1Enabled = cmp1;
        _cmp2Enabled = cmp2;
        _inverted = inverted;
        _patternChanged = true;
        _needsUpdate = true;
    }
}

void RmtGenerator::setRpm(uint32_t targetRpm) {
    if (targetRpm != _pendingRpm) {
        _pendingRpm = targetRpm;
        _needsUpdate = true;
    }
}

void RmtGenerator::setVvtConfig(const EcuEngine::VvtConfig& config) {
    if (_vvtConfig.enabled != config.enabled ||
        _vvtConfig.mode != config.mode ||
        _vvtConfig.manualAdvanceDeg != config.manualAdvanceDeg ||
        _vvtConfig.startRpm != config.startRpm ||
        _vvtConfig.fullRpm != config.fullRpm ||
        _vvtConfig.maxAdvanceDeg != config.maxAdvanceDeg) {
        _vvtConfig = config;
        _needsUpdate = true;
    }
}

size_t RmtGenerator::compileBitArrayToRmt(
    const uint8_t* bitArray,
    uint16_t totalEdges,
    uint16_t cycleDegrees,
    uint32_t rpm,
    uint8_t channelBitMask,
    bool isEnabled,
    bool isInverted,
    int16_t phaseAdvanceDeg,
    rmt_item32_t* outItems,
    size_t maxItems
) {
    if (bitArray == nullptr || totalEdges == 0 || rpm == 0 || outItems == nullptr || maxItems < 2) {
        return 0;
    }

    if (cycleDegrees != 360 && cycleDegrees != 720) {
        cycleDegrees = (cycleDegrees == 0) ? 720 : cycleDegrees;
    }

    // Microsecond duration of 1 full cycle
    // T_cycle = (cycleDegrees * 1,000,000) / (6 * rpm)
    uint64_t cycleTotalUs = ((uint64_t)cycleDegrees * 1000000ULL) / (6ULL * (uint64_t)rpm);
    if (cycleTotalUs == 0) {
        cycleTotalUs = 1;
    }

    if (!isEnabled) {
        uint32_t rem = (uint32_t)cycleTotalUs;
        size_t count = 0;
        while (rem > MAX_RMT_DURATION_CHUNK && count < (maxItems - 2)) {
            outItems[count++] = rmt_item32_t{MAX_RMT_DURATION_CHUNK, 0, 0, 0};
            rem -= MAX_RMT_DURATION_CHUNK;
        }
        outItems[count++] = rmt_item32_t{static_cast<uint16_t>(rem), 0, 0, 0};
        outItems[count++] = rmt_item32_t{0, 0, 0, 0};
        return count + 1;
    }

    // Calculate segment advance offset
    int32_t advSegs = 0;
    if (phaseAdvanceDeg != 0 && totalEdges > 0 && cycleDegrees > 0) {
        advSegs = ((int32_t)phaseAdvanceDeg * (int32_t)totalEdges) / (int32_t)cycleDegrees;
    }

    auto getLvlAt = [&](uint16_t s) -> uint8_t {
        int32_t srcIdx = (int32_t)s + advSegs;
        while (srcIdx < 0) srcIdx += totalEdges;
        srcIdx = srcIdx % totalEdges;
        uint8_t lvl = (bitArray[srcIdx] & channelBitMask) ? 1 : 0;
        return isInverted ? (1 - lvl) : lvl;
    };

    // Temporary flat phase buffer for sliced pulses
    const size_t maxPhases = (maxItems > 1) ? (maxItems - 1) * 2 : 0;
    static FlatPhase s_compilePhases[EcuEngine::MAX_CYCLE_PULSES * 2];
    size_t phaseCount = 0;

    uint8_t currLvl = getLvlAt(0);
    uint16_t runStartSeg = 0;

    for (uint16_t s = 1; s < totalEdges; ++s) {
        uint8_t lvl = getLvlAt(s);
        if (lvl != currLvl) {
            uint64_t tStartUs = ((uint64_t)runStartSeg * cycleTotalUs) / (uint64_t)totalEdges;
            uint64_t tEndUs   = ((uint64_t)s * cycleTotalUs) / (uint64_t)totalEdges;
            uint32_t runDurUs = (uint32_t)(tEndUs - tStartUs);

            // Slicing for RMT 15-bit counter limit (<= 30,000 us per chunk)
            uint32_t rem = runDurUs;
            while (rem > MAX_RMT_DURATION_CHUNK && phaseCount < maxPhases && phaseCount < sizeof(s_compilePhases)/sizeof(s_compilePhases[0])) {
                s_compilePhases[phaseCount++] = { MAX_RMT_DURATION_CHUNK, currLvl };
                rem -= MAX_RMT_DURATION_CHUNK;
            }
            if (rem > 0 && phaseCount < maxPhases && phaseCount < sizeof(s_compilePhases)/sizeof(s_compilePhases[0])) {
                s_compilePhases[phaseCount++] = { static_cast<uint16_t>(rem), currLvl };
            }

            currLvl = lvl;
            runStartSeg = s;
        }
    }

    // Final run to end of cycle
    {
        uint64_t tStartUs = ((uint64_t)runStartSeg * cycleTotalUs) / (uint64_t)totalEdges;
        uint64_t tEndUs   = cycleTotalUs;
        uint32_t runDurUs = (uint32_t)(tEndUs - tStartUs);

        uint32_t rem = runDurUs;
        while (rem > MAX_RMT_DURATION_CHUNK && phaseCount < maxPhases && phaseCount < sizeof(s_compilePhases)/sizeof(s_compilePhases[0])) {
            s_compilePhases[phaseCount++] = { MAX_RMT_DURATION_CHUNK, currLvl };
            rem -= MAX_RMT_DURATION_CHUNK;
        }
        if (rem > 0 && phaseCount < maxPhases && phaseCount < sizeof(s_compilePhases)/sizeof(s_compilePhases[0])) {
            s_compilePhases[phaseCount++] = { static_cast<uint16_t>(rem), currLvl };
        }
    }

    // Pack sliced phases into rmt_item32_t pairs
    size_t outCount = 0;
    for (size_t i = 0; i < phaseCount && outCount < (maxItems - 1); i += 2) {
        rmt_item32_t item{};
        item.duration0 = s_compilePhases[i].duration;
        item.level0    = s_compilePhases[i].level;
        if (i + 1 < phaseCount) {
            item.duration1 = s_compilePhases[i + 1].duration;
            item.level1    = s_compilePhases[i + 1].level;
        } else {
            item.duration1 = 0;
            item.level1    = 0;
        }
        outItems[outCount++] = item;
    }

    // Append zero-terminator EOT item for continuous hardware looping
    outItems[outCount++] = rmt_item32_t{0, 0, 0, 0};
    return outCount;
}

void RmtGenerator::prepareBitArrayCycle() {
    if (_activeWheel == nullptr || _pendingRpm == 0) return;

    // Calculate dynamic VVT advance based on manual mode or pending RPM
    if (!_vvtConfig.enabled) {
        _currentVvtAdvance = 0;
    } else if (_vvtConfig.mode == EcuEngine::VvtMode::Manual) {
        _currentVvtAdvance = _vvtConfig.manualAdvanceDeg;
    } else {
        if (_pendingRpm > _vvtConfig.startRpm) {
            if (_pendingRpm >= _vvtConfig.fullRpm) {
                _currentVvtAdvance = _vvtConfig.maxAdvanceDeg;
            } else {
                uint32_t span = _vvtConfig.fullRpm - _vvtConfig.startRpm;
                uint32_t diff = _pendingRpm - _vvtConfig.startRpm;
                int32_t calcAdv = ((int32_t)diff * (int32_t)_vvtConfig.maxAdvanceDeg) / (int32_t)span;
                _currentVvtAdvance = (int8_t)calcAdv;
            }
        } else {
            _currentVvtAdvance = 0;
        }
    }

    rmt_item32_t* targetCkp  = (_activeBufferIdx == 0) ? _ckpBufferB  : _ckpBufferA;
    rmt_item32_t* targetCmp1 = (_activeBufferIdx == 0) ? _cmpBufferB  : _cmpBufferA;
    rmt_item32_t* targetCmp2 = (_activeBufferIdx == 0) ? _cmp2BufferB : _cmp2BufferA;

    size_t ckpSize = 0;
    size_t cmp1Size = 0;
    size_t cmp2Size = 0;

    const uint8_t* bitArray = _activeWheel->bitArray;
    uint16_t totalEdges = _activeWheel->totalEdges;
    uint16_t cycleDegrees = static_cast<uint16_t>(_activeWheel->cycleDegrees);

    // Replicate 360-degree crank patterns seamlessly (2x) when running in 720-degree engine cycle context
    if (_activeWheel->cycleDegrees == WheelCycleDegrees::CRANK_360 && (totalEdges * 2) <= sizeof(s_replicated360)) {
        memcpy(s_replicated360, bitArray, totalEdges);
        memcpy(s_replicated360 + totalEdges, bitArray, totalEdges);
        bitArray = s_replicated360;
        totalEdges = totalEdges * 2;
        cycleDegrees = 720;
    }

    // 1. Compile CKP (Channel 0 / Primary Trigger - No Phase Shift)
    ckpSize = compileBitArrayToRmt(
        bitArray, totalEdges, cycleDegrees, _pendingRpm,
        EcuEngine::SIGNAL_BIT_CKP, _ckpEnabled, _inverted, 0, targetCkp, EcuEngine::MAX_CYCLE_PULSES
    );

    // 2. Compile CMP1 (Channel 2 / Secondary Trigger - VVT Intake Cam Advance)
    if (_activeWheel->hasCmp1) {
        cmp1Size = compileBitArrayToRmt(
            bitArray, totalEdges, cycleDegrees, _pendingRpm,
            EcuEngine::SIGNAL_BIT_CMP1, _cmp1Enabled, _inverted, _currentVvtAdvance, targetCmp1, EcuEngine::MAX_CYCLE_PULSES
        );
    } else {
        targetCmp1[0] = rmt_item32_t{0, 0, 0, 0};
        cmp1Size = 0;
    }

    // 3. Compile CMP2 (Channel 3 / Tertiary Trigger / Dual VVT Exhaust Cam Retard)
    if (_activeWheel->hasCmp2) {
        int16_t cmp2Shift = -(int16_t)((_currentVvtAdvance * 5) / 8); // Retard on exhaust cam
        cmp2Size = compileBitArrayToRmt(
            bitArray, totalEdges, cycleDegrees, _pendingRpm,
            EcuEngine::SIGNAL_BIT_CMP2, _cmp2Enabled, _inverted, cmp2Shift, targetCmp2, EcuEngine::MAX_CYCLE_PULSES
        );
    } else {
        targetCmp2[0] = rmt_item32_t{0, 0, 0, 0};
        cmp2Size = 0;
    }

    if (_activeBufferIdx == 0) {
        _ckpSizeB  = ckpSize;
        _cmpSizeB  = cmp1Size;
        _cmp2SizeB = cmp2Size;
    } else {
        _ckpSizeA  = ckpSize;
        _cmpSizeA  = cmp1Size;
        _cmp2SizeA = cmp2Size;
    }
}

void RmtGenerator::prepareNextCycle() {
    if (!_needsUpdate || _pendingRpm == 0) return;

    if (_isBitArrayMode && _activeWheel != nullptr) {
        prepareBitArrayCycle();
        return;
    }

    // Parametric fallback mode
    // 1. Generate CKP
    {
        size_t ckpCount = EcuEngine::ParametricEngine::generateCkpCycle(
            _wheel, _pendingRpm, s_segments, EcuEngine::MAX_CYCLE_PULSES);
        size_t phaseCount = 0;
        for (size_t i = 0; i < ckpCount && phaseCount < (EcuEngine::MAX_CYCLE_PULSES * 2 - 4); ++i) {
            uint32_t rem0 = s_segments[i].duration0Us;
            uint8_t lvl0 = s_segments[i].level0;
            while (rem0 > 0 && phaseCount < (EcuEngine::MAX_CYCLE_PULSES * 2 - 4)) {
                uint16_t d = (rem0 > MAX_RMT_DURATION_CHUNK) ? MAX_RMT_DURATION_CHUNK : static_cast<uint16_t>(rem0);
                s_phases[phaseCount++] = { d, lvl0 };
                rem0 -= d;
            }
            uint32_t rem1 = s_segments[i].duration1Us;
            uint8_t lvl1 = s_segments[i].level1;
            while (rem1 > 0 && phaseCount < (EcuEngine::MAX_CYCLE_PULSES * 2 - 4)) {
                uint16_t d = (rem1 > MAX_RMT_DURATION_CHUNK) ? MAX_RMT_DURATION_CHUNK : static_cast<uint16_t>(rem1);
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

    // 2. Generate CMP (720 deg) with VVT phase shift
    EcuEngine::CamEventTable shiftedCam = _cam;
    if (!_vvtConfig.enabled) {
        _currentVvtAdvance = 0;
    } else if (_vvtConfig.mode == EcuEngine::VvtMode::Manual) {
        _currentVvtAdvance = _vvtConfig.manualAdvanceDeg;
    } else {
        if (_pendingRpm > _vvtConfig.startRpm) {
            if (_pendingRpm >= _vvtConfig.fullRpm) {
                _currentVvtAdvance = _vvtConfig.maxAdvanceDeg;
            } else {
                uint32_t span = _vvtConfig.fullRpm - _vvtConfig.startRpm;
                uint32_t diff = _pendingRpm - _vvtConfig.startRpm;
                int32_t calcAdv = ((int32_t)diff * (int32_t)_vvtConfig.maxAdvanceDeg) / (int32_t)span;
                _currentVvtAdvance = (int8_t)calcAdv;
            }
        } else {
            _currentVvtAdvance = 0;
        }
    }
    if (_currentVvtAdvance != 0) {
        shiftedCam.clear();
        uint8_t count = _cam.getEventCount();
        const EcuEngine::CmpEvent* ev = _cam.getEvents();
        for (uint8_t i = 0; i < count; ++i) {
            float shiftedAngle = ev[i].angleDeg - (float)_currentVvtAdvance;
            while (shiftedAngle < 0.0f) shiftedAngle += 720.0f;
            while (shiftedAngle >= 720.0f) shiftedAngle -= 720.0f;
            shiftedCam.addEvent(shiftedAngle, ev[i].levelHigh);
        }
    }

    size_t cmpCount = EcuEngine::ParametricEngine::generateCmpCycle(
        shiftedCam, _pendingRpm, s_segments, EcuEngine::MAX_CYCLE_PULSES);

    if (cmpCount > 0) {
        size_t phaseCount = 0;
        for (size_t i = 0; i < cmpCount && phaseCount < (EcuEngine::MAX_CYCLE_PULSES * 2 - 4); ++i) {
            uint32_t rem = s_segments[i].duration0Us;
            uint8_t lvl = s_segments[i].level0;
            while (rem > 0 && phaseCount < (EcuEngine::MAX_CYCLE_PULSES * 2 - 4)) {
                uint16_t d = (rem > MAX_RMT_DURATION_CHUNK) ? MAX_RMT_DURATION_CHUNK : static_cast<uint16_t>(rem);
                s_phases[phaseCount++] = { d, lvl };
                rem -= d;
            }
            if (s_segments[i].duration1Us > 0) {
                rem = s_segments[i].duration1Us;
                lvl = s_segments[i].level1;
                while (rem > 0 && phaseCount < (EcuEngine::MAX_CYCLE_PULSES * 2 - 4)) {
                    uint16_t d = (rem > MAX_RMT_DURATION_CHUNK) ? MAX_RMT_DURATION_CHUNK : static_cast<uint16_t>(rem);
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
    } else {
        if (_activeBufferIdx == 0) _cmpSizeB = 0;
        else _cmpSizeA = 0;
    }

    // CMP2 is unused in parametric mode
    if (_activeBufferIdx == 0) _cmp2SizeB = 0;
    else _cmp2SizeA = 0;
}

void RmtGenerator::swapBuffer() {
    if (!_needsUpdate) return;

    uint8_t nextIdx = (_activeBufferIdx == 0) ? 1 : 0;
    rmt_item32_t* activeCkp  = (nextIdx == 1) ? _ckpBufferB  : _ckpBufferA;
    size_t activeCkpSize    = (nextIdx == 1) ? _ckpSizeB    : _ckpSizeA;

    rmt_item32_t* activeCmp1 = (nextIdx == 1) ? _cmpBufferB  : _cmpBufferA;
    size_t activeCmp1Size   = (nextIdx == 1) ? _cmpSizeB    : _cmpSizeA;

    rmt_item32_t* activeCmp2 = (nextIdx == 1) ? _cmp2BufferB : _cmp2BufferA;
    size_t activeCmp2Size   = (nextIdx == 1) ? _cmp2SizeB   : _cmp2SizeA;

    if (_running) {
        if (_patternChanged) {
            // Pola preset/tipe roda baru dipilih: restart kanal hardware secara bersih
            rmt_tx_stop(CH_CKP);
            rmt_tx_stop(CH_CMP);
            rmt_tx_stop(CH_CMP2);

            if (activeCkpSize > 0) {
                rmt_fill_tx_items(CH_CKP, activeCkp, activeCkpSize, 0);
                rmt_set_tx_loop_mode(CH_CKP, true);
            }
            if (activeCmp1Size > 0) {
                rmt_fill_tx_items(CH_CMP, activeCmp1, activeCmp1Size, 0);
                rmt_set_tx_loop_mode(CH_CMP, true);
            }
            if (activeCmp2Size > 0) {
                rmt_fill_tx_items(CH_CMP2, activeCmp2, activeCmp2Size, 0);
                rmt_set_tx_loop_mode(CH_CMP2, true);
            }

            if (activeCkpSize > 0)  rmt_tx_start(CH_CKP, true);
            if (activeCmp1Size > 0) rmt_tx_start(CH_CMP, true);
            if (activeCmp2Size > 0) rmt_tx_start(CH_CMP2, true);
            _patternChanged = false;
        } else {
            // Pembaruan RPM/VVT halus tanpa menghentikan pulsa crank
            if (activeCkpSize > 0) {
                rmt_fill_tx_items(CH_CKP, activeCkp, activeCkpSize, 0);
            }
            if (activeCmp1Size > 0) {
                rmt_fill_tx_items(CH_CMP, activeCmp1, activeCmp1Size, 0);
            }
            if (activeCmp2Size > 0) {
                rmt_fill_tx_items(CH_CMP2, activeCmp2, activeCmp2Size, 0);
            }
        }

        _activeBufferIdx = nextIdx;
        _activeRpm = _pendingRpm;
        _cycleStartUs = micros();
        _activeCycleUs = (_activeRpm > 0) ? (120000000ULL / _activeRpm) : 120000;
        _needsUpdate = false;
    } else {
        _activeBufferIdx = nextIdx;
        _activeRpm = _pendingRpm;
        _activeCycleUs = (_activeRpm > 0) ? (120000000ULL / _activeRpm) : 120000;
        _needsUpdate = false;
        _patternChanged = false;
    }
}

void RmtGenerator::start() {
    uint8_t currentIdx = _activeBufferIdx;
    rmt_item32_t* activeCkp  = (currentIdx == 1) ? _ckpBufferB  : _ckpBufferA;
    size_t activeCkpSize    = (currentIdx == 1) ? _ckpSizeB    : _ckpSizeA;

    rmt_item32_t* activeCmp1 = (currentIdx == 1) ? _cmpBufferB  : _cmpBufferA;
    size_t activeCmp1Size   = (currentIdx == 1) ? _cmpSizeB    : _cmpSizeA;

    rmt_item32_t* activeCmp2 = (currentIdx == 1) ? _cmp2BufferB : _cmp2BufferA;
    size_t activeCmp2Size   = (currentIdx == 1) ? _cmp2SizeB   : _cmp2SizeA;

    rmt_tx_stop(CH_CKP);
    rmt_tx_stop(CH_CMP);
    rmt_tx_stop(CH_CMP2);

    if (activeCkpSize > 0) {
        rmt_fill_tx_items(CH_CKP, activeCkp, activeCkpSize, 0);
        rmt_set_tx_loop_mode(CH_CKP, true);
    }
    if (activeCmp1Size > 0) {
        rmt_fill_tx_items(CH_CMP, activeCmp1, activeCmp1Size, 0);
        rmt_set_tx_loop_mode(CH_CMP, true);
    }
    if (activeCmp2Size > 0) {
        rmt_fill_tx_items(CH_CMP2, activeCmp2, activeCmp2Size, 0);
        rmt_set_tx_loop_mode(CH_CMP2, true);
    }

    if (activeCkpSize > 0)  rmt_tx_start(CH_CKP, true);
    if (activeCmp1Size > 0) rmt_tx_start(CH_CMP, true);
    if (activeCmp2Size > 0) rmt_tx_start(CH_CMP2, true);

    _running = true;
    _cycleStartUs = micros();
    _activeCycleUs = (_activeRpm > 0) ? (120000000ULL / _activeRpm) : 120000;
    _needsUpdate = false;
    Serial.println("[RMT] Multi-channel continuous loop STARTED (CKP/CMP1/CMP2).");
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
