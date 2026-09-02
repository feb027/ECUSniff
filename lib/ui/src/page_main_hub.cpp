#include "page_main_hub.h"

namespace EcuUi {

static constexpr uint8_t TOTAL_MODULES = 5;

static const char* CARD_TITLES[TOTAL_MODULES] = {
    "1. ENGINE SIGNAL GENERATOR",
    "2. SIGNAL CAPTURE / SNIFFER",
    "3. EPS & VSS BENCH TESTER",
    "4. SPEEDOMETER CLUSTER TESTER",
    "5. POWER CYCLE / STRESS TESTER"
};

static const char* CARD_DESCS[TOTAL_MODULES] = {
    "Pembangkit Sinyal Mobil: CKP & CMP (FIX/CRANK/SWEEP)",
    "Perekam & Penganalisis Sinyal ECU Mobil 720-deg",
    "Simulasi EPS: VSS Speed, RPM Tach, Steer TRQ & Sweep",
    "Tester Speedo, Tacho, Suhu ECT, Bensin & Sweep",
    "Uji Daya IGSW ON/OFF Otomatis & Monitor M-REL ECU"
};

PageMainHub::PageMainHub(LovyanGFX* gfx) : _gfx(gfx) {}

void PageMainHub::render(bool fullRedraw, uint8_t selectedIndex) {
    if (selectedIndex >= TOTAL_MODULES) selectedIndex = 0;

    // Window scroll 4 kartu terlihat sekaligus (index 0..3 atau 1..4)
    if (selectedIndex < _scrollOffset) {
        _scrollOffset = selectedIndex;
    } else if (selectedIndex >= _scrollOffset + 4) {
        _scrollOffset = selectedIndex - 3;
    }

    bool scrollChanged = (_scrollOffset != _lastScrollOffset);

    if (fullRedraw || scrollChanged) {
        if (fullRedraw) {
            _gfx->fillScreen(TFT_BLACK);
            // Header bar (Hitam Pekat Murni)
            _gfx->fillRect(0, 0, 480, 42, TFT_BLACK);
            _gfx->drawFastHLine(0, 42, 480, 0x52AA);
            _gfx->setTextColor(TFT_WHITE, TFT_BLACK);
            _gfx->setTextSize(2);
            _gfx->drawString("ECUSniff — MASTER HUB", 16, 12);

            // Bottom status bar
            _gfx->drawFastHLine(0, 286, 480, 0x52AA);
            _gfx->fillRect(0, 287, 480, 33, TFT_BLACK);
            _gfx->setTextColor(0x8410, TFT_BLACK);
            _gfx->setTextSize(1);
            _gfx->drawString("Putar / Joy-Y: Pilih Modul  |  Klik: Masuk Modul", 70, 298);
        }

        // Hapus area kartu (Y: 44 s/d 285) dengan warna hitam pekat
        _gfx->fillRect(0, 44, 480, 241, TFT_BLACK);

        // Render 4 kartu yang masuk dalam jendela scroll
        for (uint8_t i = 0; i < 4; ++i) {
            uint8_t modIdx = _scrollOffset + i;
            if (modIdx < TOTAL_MODULES) {
                int32_t y = 48 + (i * 58);
                _drawCard(modIdx, y, modIdx == selectedIndex);
            }
        }

        _lastSelectedIndex = selectedIndex;
        _lastScrollOffset = _scrollOffset;
        return;
    }

    // Jika scroll tidak berubah, update hanya kartu yang aktif dan nonaktif
    if (selectedIndex != _lastSelectedIndex) {
        for (uint8_t i = 0; i < 4; ++i) {
            uint8_t modIdx = _scrollOffset + i;
            if (modIdx == _lastSelectedIndex) {
                int32_t y = 48 + (i * 58);
                _drawCard(modIdx, y, false);
            } else if (modIdx == selectedIndex) {
                int32_t y = 48 + (i * 58);
                _drawCard(modIdx, y, true);
            }
        }
        _lastSelectedIndex = selectedIndex;
    }
}

void PageMainHub::_drawCard(uint8_t moduleIndex, int32_t y, bool isSelected) {
    if (moduleIndex >= TOTAL_MODULES) return;

    // ========================================================================
    // TEMA WARNA OTOMOTIF BERBEDA UNTUK TIAP MODUL MENU (BEBAS DARI CYAN/BIRU)
    // 0: Kuning Emas (Engine Signal Generator)
    // 1: Hijau Lime Neon (Signal Capture / Sniffer)
    // 2: Oranye Terang (EPS & VSS Bench Tester)
    // 3: Merah Crimson (Speedometer Cluster Tester)
    // 4: Magenta / Ungu Terang (Power Cycle / Stress Tester)
    // ========================================================================
    static constexpr uint16_t THEME_COLORS[TOTAL_MODULES] = {
        0xFFE0, // 1. Kuning Emas
        0x07E0, // 2. Hijau Lime Neon
        0xFD20, // 3. Oranye Terang
        0xF800, // 4. Merah Crimson
        0xF81F  // 5. Magenta / Ungu Terang
    };

    uint16_t themeColor = THEME_COLORS[moduleIndex];
    uint32_t bgColor    = TFT_BLACK;

    // Saat tidak dipilih: border warna tema tipis, teks judul warna tema, teks sub-deskripsi putih bersih
    // Saat dipilih: border ganda warna tema tebal, teks judul dan deskripsi warna tema terang
    uint16_t borderColor = themeColor;
    uint16_t titleColor   = isSelected ? themeColor : TFT_WHITE;
    uint16_t descColor    = isSelected ? themeColor : 0xD6BA; // Abu-abu perak terang saat normal

    // Ukuran kartu besar (Tinggi 52px, Lebar 456px)
    _gfx->fillRoundRect(12, y, 456, 52, 6, bgColor);
    _gfx->drawRoundRect(12, y, 456, 52, 6, borderColor);
    if (isSelected) {
        _gfx->drawRoundRect(13, y + 1, 454, 50, 5, borderColor);
        _gfx->drawRoundRect(14, y + 2, 452, 48, 4, borderColor);
    }

    // Teks Judul Besar (TextSize 2 tebal)
    _gfx->setTextColor(titleColor, bgColor);
    _gfx->setTextSize(2);
    _gfx->drawString(CARD_TITLES[moduleIndex], 24, y + 8);

    // Teks Deskripsi Jelas
    _gfx->setTextColor(descColor, bgColor);
    _gfx->setTextSize(1);
    _gfx->drawString(CARD_DESCS[moduleIndex], 24, y + 32);
}

void PageMainHub::onEncoderTurn(int32_t delta, uint8_t& selectedIndex) {
    int32_t next = static_cast<int32_t>(selectedIndex) + (delta > 0 ? 1 : -1);
    if (next < 0) next = TOTAL_MODULES - 1;
    if (next >= TOTAL_MODULES) next = 0;
    selectedIndex = static_cast<uint8_t>(next);
}

} // namespace EcuUi
