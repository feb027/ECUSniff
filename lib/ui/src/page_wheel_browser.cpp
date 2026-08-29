#include "page_wheel_browser.h"
#include "wheel_database.h"
#include "page_dashboard.h"
#include <string.h>

namespace EcuUi {

static const char* CAT_NAMES[] = {
    "ALL", "TOYOTA", "HONDA", "MITSU", "EURO/US", "UNIV", "CUSTOM"
};

static const uint16_t CAT_WIDTHS[] = {
    54, 62, 58, 56, 66, 54, 66
};

PageWheelBrowser::PageWheelBrowser(LovyanGFX* gfx) : _gfx(gfx), _canvas(gfx) {}

void PageWheelBrowser::init() {
    _canvas.init(198, 70);
}

void PageWheelBrowser::open(uint16_t initialGlobalIdx) {
    _isOpen = true;
    _category = WheelCategory::All;
    _buildFilteredList();
    _cursorIdx = 0;
    for (uint16_t i = 0; i < _filteredCount; ++i) {
        if (_filteredIndices[i] == initialGlobalIdx) {
            _cursorIdx = i; break;
        }
    }
    _lastCursorIdx = -1;
    _lastCategory = 0xFF;
    _needsListRedraw = true;
}

void PageWheelBrowser::close() {
    _isOpen = false;
}

bool PageWheelBrowser::_matchesCategory(uint16_t globalIdx, WheelCategory cat) {
    if (cat == WheelCategory::All) return true;
    if (globalIdx >= OEM_DATABASE_COUNT) return (cat == WheelCategory::Custom);
    if (cat == WheelCategory::Custom) return false;

    const char* name = OEM_DATABASE_PRESETS[globalIdx].name;
    if (cat == WheelCategory::ToyotaDaihatsu) {
        return (strstr(name, "Toyota") || strstr(name, "Daihatsu") || strstr(name, "4AG"));
    } else if (cat == WheelCategory::HondaSuzuki) {
        return (strstr(name, "Honda") || strstr(name, "Suzuki") || strstr(name, "Yamaha") || strstr(name, "RC51") || strstr(name, "D17") || strstr(name, "R1"));
    } else if (cat == WheelCategory::MitsuNissanMazda) {
        return (strstr(name, "Mitsubishi") || strstr(name, "Nissan") || strstr(name, "Mazda") || strstr(name, "Subaru") || strstr(name, "Miata") || strstr(name, "4g63") || strstr(name, "6g72") || strstr(name, "3A92") || strstr(name, "323"));
    } else if (cat == WheelCategory::EuroAmerika) {
        return (strstr(name, "BMW") || strstr(name, "Audi") || strstr(name, "Fiat") || strstr(name, "Volvo") || strstr(name, "GM") || strstr(name, "Ford") || strstr(name, "Chrysler") || strstr(name, "Jeep") || strstr(name, "Dodge") || strstr(name, "Viper") || strstr(name, "Lotus") || strstr(name, "Weber") || strstr(name, "Buell") || strstr(name, "DSM"));
    } else if (cat == WheelCategory::Universal) {
        return (strstr(name, "60-2") || strstr(name, "36-1") || strstr(name, "24-1") || strstr(name, "12-1") || strstr(name, "8-1") || strstr(name, "4-1") || strstr(name, "40-1") || strstr(name, "dizzy") || strstr(name, "12/1") || strstr(name, "24/1") || strstr(name, "12-3") || strstr(name, "odd"));
    }
    return false;
}

void PageWheelBrowser::_buildFilteredList() {
    _filteredCount = 0;
    uint16_t total = OEM_DATABASE_COUNT + PageDashboard::getCustomCount();
    for (uint16_t i = 0; i < total && _filteredCount < 128; ++i) {
        if (_matchesCategory(i, _category)) {
            _filteredIndices[_filteredCount++] = i;
        }
    }
    if (_cursorIdx >= _filteredCount) _cursorIdx = (_filteredCount > 0) ? (_filteredCount - 1) : 0;
}

uint16_t PageWheelBrowser::getSelectedGlobalIndex() const {
    if (_filteredCount == 0 || _cursorIdx >= _filteredCount) return 0;
    return _filteredIndices[_cursorIdx];
}

void PageWheelBrowser::render(bool fullRedraw) {
    if (!_isOpen) return;

    if (fullRedraw) {
        _gfx->fillScreen(TFT_BLACK);
        _drawHeader();
        _drawFooter();
        _drawList(true);
        _drawPreview();
        _lastCategory = static_cast<uint8_t>(_category);
        _lastCursorIdx = _cursorIdx;
        return;
    }

    if (static_cast<uint8_t>(_category) != _lastCategory) {
        _drawHeader();
        _drawList(true);
        _drawPreview();
        _lastCategory = static_cast<uint8_t>(_category);
        _lastCursorIdx = _cursorIdx;
        return;
    }

    if (_cursorIdx != _lastCursorIdx || _needsListRedraw) {
        _drawList(false);
        _drawPreview();
        _lastCursorIdx = _cursorIdx;
        _needsListRedraw = false;
    }
}

void PageWheelBrowser::_drawHeader() {
    _gfx->fillRect(0, 0, 480, 42, 0x0841);
    _gfx->drawFastHLine(0, 42, 480, 0x52AA);

    uint8_t catIdx = static_cast<uint8_t>(_category);
    int32_t xPos = 20;
    for (uint8_t c = 0; c < 7; ++c) {
        int32_t w = CAT_WIDTHS[c];
        bool active = (c == catIdx);
        _gfx->fillRoundRect(xPos, 5, w, 32, 5, active ? 0x07E0 : 0x18C3);
        if (active) _gfx->drawRoundRect(xPos, 5, w, 32, 5, 0xFFE0);
        else _gfx->drawRoundRect(xPos, 5, w, 32, 5, 0x31A6);

        _gfx->setTextColor(active ? TFT_BLACK : TFT_WHITE, active ? 0x07E0 : 0x18C3);
        _gfx->setTextSize(1);
        _gfx->drawCenterString(CAT_NAMES[c], xPos + (w / 2), 16);
        xPos += (w + 4);
    }
}

void PageWheelBrowser::_drawList(bool forceAll) {
    int16_t startSlot = (_cursorIdx / 4) * 4;
    int32_t yBase = 46;

    if (forceAll) {
        _gfx->fillRect(6, 44, 260, 236, 0x0841);
        _gfx->drawRoundRect(6, 44, 260, 236, 6, 0x52AA);
    }

    for (uint8_t s = 0; s < 4; ++s) {
        int16_t itemIdx = startSlot + s;
        int32_t y = yBase + (s * 52);
        bool isSel = (itemIdx == _cursorIdx);

        if (itemIdx < _filteredCount) {
            uint16_t gIdx = _filteredIndices[itemIdx];
            const char* name = (gIdx < OEM_DATABASE_COUNT) ? OEM_DATABASE_PRESETS[gIdx].name : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT)->name;
            uint8_t teeth = (gIdx < OEM_DATABASE_COUNT) ? OEM_DATABASE_PRESETS[gIdx].totalTeeth : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT)->totalTeeth;
            uint8_t mTeeth = (gIdx < OEM_DATABASE_COUNT) ? OEM_DATABASE_PRESETS[gIdx].missingTeeth : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT)->missingTeeth;
            uint8_t cams = (gIdx < OEM_DATABASE_COUNT) ? OEM_DATABASE_PRESETS[gIdx].camCount : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT)->camCount;

            uint32_t bg = isSel ? 0x18C3 : 0x10A2;
            _gfx->fillRoundRect(12, y + 2, 248, 48, 4, bg);
            _gfx->drawRoundRect(12, y + 2, 248, 48, 4, isSel ? 0xFFE0 : 0x31A6);
            if (isSel) {
                _gfx->drawRoundRect(13, y + 3, 246, 46, 3, 0xFFE0);
            }

            _gfx->setTextColor(isSel ? 0xFFE0 : TFT_WHITE, bg);
            _gfx->setTextSize(1);
            char tBuf[32]; snprintf(tBuf, sizeof(tBuf), "%2d. %-22.22s", itemIdx + 1, name);
            _gfx->drawString(tBuf, 18, y + 8);

            _gfx->setTextColor(isSel ? 0x07FF : 0x07E0, bg);
            char subBuf[32]; snprintf(subBuf, sizeof(subBuf), "Pola: %u-%u CKP | %u Pulsa CAM", teeth, mTeeth, cams);
            _gfx->drawString(subBuf, 28, y + 26);
        } else {
            _gfx->fillRect(12, y + 2, 248, 48, 0x0841);
        }
    }

    _gfx->fillRect(12, 256, 248, 20, 0x0841);
    _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
    char pBuf[32];
    snprintf(pBuf, sizeof(pBuf), "Hal %d/%d (%d Pola)", (_cursorIdx / 4) + 1, ((_filteredCount + 3) / 4), _filteredCount);
    _gfx->drawString(pBuf, 18, 260);
}

void PageWheelBrowser::_drawPreview() {
    _gfx->fillRoundRect(270, 44, 204, 236, 6, 0x0841);
    _gfx->drawRoundRect(270, 44, 204, 236, 6, 0x52AA);

    _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(1);
    _gfx->drawString("PREVIEW POLA GELOMBANG:", 278, 52);

    if (_filteredCount == 0 || _cursorIdx >= _filteredCount) {
        _gfx->setTextColor(0xF800, 0x0841); _gfx->setTextSize(2);
        _gfx->drawString("Tidak Ada Pola", 285, 120); return;
    }

    uint16_t gIdx = _filteredIndices[_cursorIdx];
    const WheelPresetItem* p = (gIdx < OEM_DATABASE_COUNT) ? &OEM_DATABASE_PRESETS[gIdx] : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT);
    if (!p) return;

    EcuEngine::ParametricWheel wheel{};
    wheel.totalTeeth = p->totalTeeth; wheel.missingTeeth = p->missingTeeth;
    wheel.missingPosition = p->missingPosition; wheel.dutyCycle = p->dutyCycle; wheel.inverted = p->inverted;

    EcuEngine::CamEventTable cam{};
    for (uint8_t i = 0; i < p->camCount; ++i) cam.addEvent(p->camAngles[i], p->camHighs[i]);

    _canvas.render(wheel, cam, 273, 68);

    _gfx->fillRect(274, 144, 196, 92, 0x0841);
    _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
    char buf[48];
    snprintf(buf, sizeof(buf), "Nama   : %s", p->name); _gfx->drawString(buf, 278, 150);
    _gfx->setTextColor(0xFFE0, 0x0841);
    snprintf(buf, sizeof(buf), "Teeth  : %u Gigi (Missing %u)", p->totalTeeth, p->missingTeeth); _gfx->drawString(buf, 278, 170);
    _gfx->setTextColor(0x07FF, 0x0841);
    snprintf(buf, sizeof(buf), "Duty   : %.0f %% | Inv: %s", p->dutyCycle * 100.0f, p->inverted ? "YES" : "NO"); _gfx->drawString(buf, 278, 190);
    _gfx->setTextColor(0x07E0, 0x0841);
    snprintf(buf, sizeof(buf), "Cam Evs: %u Transisi (720d)", p->camCount); _gfx->drawString(buf, 278, 210);

    _gfx->fillRoundRect(276, 238, 192, 36, 4, 0x03E0); _gfx->drawRoundRect(276, 238, 192, 36, 4, 0x07E0);
    _gfx->setTextColor(0x07E0, 0x03E0); _gfx->setTextSize(2);
    _gfx->drawCenterString("KLIK TERAPKAN", 372, 248);
}

void PageWheelBrowser::_drawFooter() {
    _gfx->fillRect(0, 284, 480, 36, 0x0841);
    _gfx->drawFastHLine(0, 284, 480, 0x52AA);
    _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
    _gfx->drawString("Joy-X: Kategori | Joy-Y: Pilih Pola | Putar: Scroll | KLIK: Terapkan | 2x KLIK: Batal", 10, 296);
}

void PageWheelBrowser::onEncoderTurn(int32_t delta) {
    if (!_isOpen || _filteredCount == 0) return;
    int16_t next = _cursorIdx + delta;
    if (next < 0) next = 0;
    if (next >= _filteredCount) next = _filteredCount - 1;
    if (next != _cursorIdx) {
        _cursorIdx = next; _needsListRedraw = true;
    }
}

void PageWheelBrowser::onJoystickAction(EcuHal::JoyAction action) {
    if (!_isOpen) return;
    if (action == EcuHal::JoyAction::Up) {
        if (_cursorIdx > 0) { _cursorIdx--; _needsListRedraw = true; }
    } else if (action == EcuHal::JoyAction::Down) {
        if (_cursorIdx + 1 < _filteredCount) { _cursorIdx++; _needsListRedraw = true; }
    } else if (action == EcuHal::JoyAction::Left) {
        uint8_t c = static_cast<uint8_t>(_category);
        _category = static_cast<WheelCategory>((c > 0) ? (c - 1) : 6);
        _buildFilteredList(); _cursorIdx = 0; _needsListRedraw = true;
    } else if (action == EcuHal::JoyAction::Right) {
        uint8_t c = static_cast<uint8_t>(_category);
        _category = static_cast<WheelCategory>((c + 1) % 7);
        _buildFilteredList(); _cursorIdx = 0; _needsListRedraw = true;
    }
}

bool PageWheelBrowser::onEncoderClick(EcuEngine::ParametricWheel& outWheel,
                                     EcuEngine::CamEventTable& outCam,
                                     char* outName, size_t maxNameLen) {
    if (!_isOpen || _filteredCount == 0 || _cursorIdx >= _filteredCount) return false;
    uint16_t gIdx = _filteredIndices[_cursorIdx];
    const WheelPresetItem* p = (gIdx < OEM_DATABASE_COUNT) ? &OEM_DATABASE_PRESETS[gIdx] : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT);
    if (!p) return false;

    outWheel.totalTeeth = p->totalTeeth;
    outWheel.missingTeeth = p->missingTeeth;
    outWheel.missingPosition = p->missingPosition;
    outWheel.dutyCycle = p->dutyCycle;
    outWheel.inverted = p->inverted;

    outCam.clear();
    for (uint8_t i = 0; i < p->camCount; ++i) outCam.addEvent(p->camAngles[i], p->camHighs[i]);
    if (outName && maxNameLen > 0) {
        strncpy(outName, p->name, maxNameLen - 1);
        outName[maxNameLen - 1] = '\0';
    }
    _isOpen = false;
    return true;
}

} // namespace EcuUi
