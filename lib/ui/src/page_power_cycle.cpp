#include "page_power_cycle.h"

namespace EcuUi {

PagePowerCycle::PagePowerCycle(LovyanGFX* gfx) : _gfx(gfx) {}

void PagePowerCycle::init() {}

void PagePowerCycle::_drawPanel(int32_t x, int32_t y, int32_t w, int32_t h, bool selected) {
    uint16_t border = selected ? 0xFFE0 : 0x52AA;
    _gfx->drawRoundRect(x, y, w, h, 6, border);
    if (selected) {
        _gfx->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 5, 0xFFE0);
    }
}

void PagePowerCycle::render(bool fullRedraw, uint8_t editRow,
                            const EcuEngine::PowerCycleConfig& config,
                            const EcuEngine::PowerCycleState& state,
                            bool mcpFound) {
    if (fullRedraw) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        // Header Modul
        _gfx->fillRect(16, 50, 448, 24, 0x0841);
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("POWER CYCLE / STRESS TESTER (IGSW ON/OFF CYCLING)", 24, 56);
        _gfx->setTextColor(mcpFound ? 0x07E0 : 0xF800, 0x0841);
        _gfx->drawRightString(mcpFound ? "[MCP23017: OK]" : "[MCP23017: DISCONNECTED]", 456, 56);

        // Frame Kiri: Parameter Uji (X: 16, Y: 80, W: 220, H: 224)
        _gfx->fillRoundRect(16, 80, 220, 224, 6, 0x0841);
        _gfx->drawRoundRect(16, 80, 220, 224, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("--- PARAMETER SIKLUS ---", 24, 88);

        // Frame Kanan: Live Monitor (X: 244, Y: 80, W: 220, H: 224)
        _gfx->fillRoundRect(244, 80, 220, 224, 6, 0x0841);
        _gfx->drawRoundRect(244, 80, 220, 224, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("--- LIVE MONITOR & COUNTER ---", 252, 88);

        _lastOnMs = 0xFFFFFFFF; _lastOffMs = 0xFFFFFFFF;
        _lastTarget = 0xFFFFFFFF; _lastGenPulse = !config.genPulseDuringOn;
        _lastRunning = !state.isRunning; _lastCycle = 0xFFFFFFFF;
        _lastIgsw = !state.igswState; _lastMrel = !state.mrelDetected;
        _lastBootSuccess = 0xFFFFFFFF; _lastBootFail = 0xFFFFFFFF;
        _lastEditRow = 0xFF;
    }

    // ========================================================================
    // PANEL KIRI: PARAMETER INPUT
    // ========================================================================
    if (config.onDurationMs != _lastOnMs || fullRedraw) {
        char buf[32]; snprintf(buf, sizeof(buf), "%u ms (%.2fs)    ", (unsigned)config.onDurationMs, config.onDurationMs / 1000.0f);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("1. DURASI IGSW ON:", 24, 108);
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(buf, 24, 120);
        _lastOnMs = config.onDurationMs;
    }

    if (config.offDurationMs != _lastOffMs || fullRedraw) {
        char buf[32]; snprintf(buf, sizeof(buf), "%u ms (%.2fs)    ", (unsigned)config.offDurationMs, config.offDurationMs / 1000.0f);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("2. DURASI IGSW OFF:", 24, 144);
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(buf, 24, 156);
        _lastOffMs = config.offDurationMs;
    }

    if (config.targetCycles != _lastTarget || fullRedraw) {
        char buf[32];
        if (config.targetCycles == 0) snprintf(buf, sizeof(buf), "INFINITE (Loop) ");
        else snprintf(buf, sizeof(buf), "%u SIKLUS       ", (unsigned)config.targetCycles);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("3. TARGET TOTAL SIKLUS:", 24, 180);
        _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(buf, 24, 192);
        _lastTarget = config.targetCycles;
    }

    if (config.genPulseDuringOn != _lastGenPulse || fullRedraw) {
        const char* pStr = config.genPulseDuringOn ? "AKTIF (RUN CKP/CMP)" : "MATI (HANYA DAYA)  ";
        uint16_t pCol = config.genPulseDuringOn ? 0x07E0 : 0x8410;
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("4. PULSA CKP/CMP SAAT ON:", 24, 216);
        _gfx->setTextColor(pCol, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString(pStr, 24, 230);
        _lastGenPulse = config.genPulseDuringOn;
    }

    // Tombol Start / Stop & Reset
    if (state.isRunning != _lastRunning || fullRedraw) {
        uint16_t btnBg = state.isRunning ? 0xF800 : 0x03E0;
        uint16_t btnBorder = state.isRunning ? 0xFDE0 : 0x07E0;
        _gfx->fillRoundRect(24, 252, 100, 36, 4, btnBg);
        _gfx->drawRoundRect(24, 252, 100, 36, 4, btnBorder);
        _gfx->setTextColor(TFT_WHITE, btnBg); _gfx->setTextSize(2);
        _gfx->drawCenterString(state.isRunning ? "STOP" : "START", 74, 262);

        _gfx->fillRoundRect(130, 252, 98, 36, 4, 0x18C3);
        _gfx->drawRoundRect(130, 252, 98, 36, 4, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x18C3); _gfx->setTextSize(1);
        _gfx->drawCenterString("RESET STATS", 179, 264);

        _lastRunning = state.isRunning;
    }

    // ========================================================================
    // PANEL KANAN: LIVE MONITOR
    // ========================================================================
    if (state.currentCycle != _lastCycle || fullRedraw) {
        char buf[32];
        if (config.targetCycles > 0) {
            snprintf(buf, sizeof(buf), "%04u / %04u  ", (unsigned)state.currentCycle, (unsigned)config.targetCycles);
        } else {
            snprintf(buf, sizeof(buf), "%05u SIKLUS ", (unsigned)state.currentCycle);
        }
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("SIKLUS BERJALAN:", 252, 108);
        _gfx->setTextColor(0xFFE0, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(buf, 252, 120);
        _lastCycle = state.currentCycle;
    }

    if (state.igswState != _lastIgsw || fullRedraw) {
        const char* igStr = state.igswState ? "IGSW: ON (+12V)" : "IGSW: OFF (0V) ";
        uint16_t igCol = state.igswState ? 0x07E0 : 0x8410;
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("STATUS DAYA KONTAK:", 252, 144);
        _gfx->setTextColor(igCol, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString(igStr, 252, 156);
        _lastIgsw = state.igswState;
    }

    if (state.mrelDetected != _lastMrel || state.bootSuccessCount != _lastBootSuccess || 
        state.bootFailCount != _lastBootFail || fullRedraw) {
        
        char bootBuf[32];
        snprintf(bootBuf, sizeof(bootBuf), "OK:%u | GAGAL:%u    ", 
                 (unsigned)state.bootSuccessCount, (unsigned)state.bootFailCount);
        _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("FEEDBACK M-REL ECU (GPA3):", 252, 180);
        _gfx->setTextColor((state.bootFailCount > 0) ? 0xF800 : 0x07E0, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString(bootBuf, 252, 194);

        const char* stDesc = state.mrelDetected ? "[ECU BOOT OK - LATCHED]  " : "[MENUNGGU BOOT M-REL]  ";
        uint16_t stCol = state.mrelDetected ? 0x07E0 : 0xFFE0;
        _gfx->setTextColor(stCol, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString(stDesc, 252, 208);

        _lastMrel = state.mrelDetected;
        _lastBootSuccess = state.bootSuccessCount;
        _lastBootFail = state.bootFailCount;
    }

    // Informasi Pin Hardware
    if (fullRedraw) {
        _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
        _gfx->drawString("PIN HARDWARE MCP23017:", 252, 230);
        _gfx->drawString("• GPA2: Output Relay IGSW (+12V)", 252, 244);
        _gfx->drawString("• GPA3: Input Monitor M-REL ECU", 252, 256);
        _gfx->drawString("• GPA0/1: Sinyal STA & CHG Auto", 252, 268);
    }

    // Border seleksi editRow
    if (editRow != _lastEditRow || fullRedraw) {
        _drawPanel(20, 104, 212, 34, editRow == ROW_ON_TIME);
        _drawPanel(20, 140, 212, 34, editRow == ROW_OFF_TIME);
        _drawPanel(20, 176, 212, 34, editRow == ROW_TARGET);
        _drawPanel(20, 212, 212, 32, editRow == ROW_GEN_PULSE);
        _drawPanel(22, 250, 104, 40, editRow == ROW_START_STOP);
        _drawPanel(128, 250, 102, 40, editRow == ROW_RESET_STATS);
        _lastEditRow = editRow;
    }
}

void PagePowerCycle::onEncoderTurn(int32_t delta, uint8_t editRow,
                                   EcuEngine::PowerCycleConfig& config) {
    if (editRow == ROW_ON_TIME) {
        int32_t val = (int32_t)config.onDurationMs + (delta * 100);
        config.onDurationMs = constrain(val, 100, 10000);
    } else if (editRow == ROW_OFF_TIME) {
        int32_t val = (int32_t)config.offDurationMs + (delta * 100);
        config.offDurationMs = constrain(val, 100, 10000);
    } else if (editRow == ROW_TARGET) {
        int32_t val = (int32_t)config.targetCycles + (delta * 50);
        if (val < 0) val = 0;
        if (val > 10000) val = 10000;
        config.targetCycles = (uint32_t)val;
    } else if (editRow == ROW_GEN_PULSE) {
        config.genPulseDuringOn = !config.genPulseDuringOn;
    }
}

void PagePowerCycle::onEncoderClick(uint8_t editRow,
                                    EcuEngine::PowerCycleController& controller) {
    if (editRow == ROW_START_STOP) {
        if (controller.getState().isRunning) {
            controller.stop();
        } else {
            controller.start();
        }
    } else if (editRow == ROW_RESET_STATS) {
        controller.resetStats();
    } else if (editRow == ROW_GEN_PULSE) {
        controller.getConfig().genPulseDuringOn = !controller.getConfig().genPulseDuringOn;
    }
}

} // namespace EcuUi
