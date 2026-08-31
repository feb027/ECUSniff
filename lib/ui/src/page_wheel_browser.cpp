#include "page_wheel_browser.h"
#include "wheel_database.h"
#include "page_dashboard.h"
#include <string.h>

namespace EcuUi {

static const char* CAT_NAMES[] = { "ALL", "TOYOTA", "HONDA", "MITSU", "EURO/US", "UNIV", "CUSTOM" };
static const uint16_t CAT_WIDTHS[] = { 54, 62, 58, 56, 66, 54, 66 };

PageWheelBrowser::PageWheelBrowser(LovyanGFX* gfx) : _gfx(gfx), _canvas(gfx) {}

void PageWheelBrowser::init() { _canvas.init(180, 72); }

void PageWheelBrowser::open(uint16_t initialGlobalIdx) {
    _isOpen = true; _category = WheelCategory::All; _buildFilteredList(); _cursorIdx = 0;
    for (uint16_t i = 0; i < _filteredCount; ++i) {
        if (_filteredIndices[i] == initialGlobalIdx) { _cursorIdx = i; break; }
    }
    _lastCursorIdx = -1; _lastCategory = 0xFF; _needsListRedraw = true;
}

void PageWheelBrowser::close() { _isOpen = false; }

bool PageWheelBrowser::_matchesCategory(uint16_t globalIdx, WheelCategory cat) {
    if (cat == WheelCategory::All) return true;
    if (globalIdx >= OEM_DATABASE_COUNT) return (cat == WheelCategory::Custom);
    if (cat == WheelCategory::Custom) return false;

    const char* name = OEM_DATABASE_PRESETS[globalIdx].name;
    if (cat == WheelCategory::ToyotaDaihatsu) return (strstr(name, "Toyota") || strstr(name, "Daihatsu") || strstr(name, "4AG") || strstr(name, "Avanza") || strstr(name, "Xenia") || strstr(name, "Rush") || strstr(name, "Terios"));
    if (cat == WheelCategory::HondaSuzuki) return (strstr(name, "Honda") || strstr(name, "Suzuki") || strstr(name, "Yamaha") || strstr(name, "RC51") || strstr(name, "D17") || strstr(name, "R1") || strstr(name, "Jazz") || strstr(name, "Fit"));
    if (cat == WheelCategory::MitsuNissanMazda) return (strstr(name, "Mitsubishi") || strstr(name, "Nissan") || strstr(name, "Mazda") || strstr(name, "Subaru") || strstr(name, "Miata") || strstr(name, "4g63") || strstr(name, "6g72") || strstr(name, "3A92") || strstr(name, "323") || strstr(name, "Livina") || strstr(name, "Juke"));
    if (cat == WheelCategory::EuroAmerika) return (strstr(name, "BMW") || strstr(name, "Audi") || strstr(name, "Fiat") || strstr(name, "Volvo") || strstr(name, "GM") || strstr(name, "Ford") || strstr(name, "Chrysler") || strstr(name, "Jeep") || strstr(name, "Dodge") || strstr(name, "Viper") || strstr(name, "Lotus") || strstr(name, "Weber") || strstr(name, "Buell") || strstr(name, "DSM"));
    if (cat == WheelCategory::Universal) return (strstr(name, "60-2") || strstr(name, "36-1") || strstr(name, "24-1") || strstr(name, "12-1") || strstr(name, "8-1") || strstr(name, "4-1") || strstr(name, "40-1") || strstr(name, "dizzy") || strstr(name, "12/1") || strstr(name, "24/1") || strstr(name, "12-3") || strstr(name, "odd") || strstr(name, "36-2"));
    return false;
}

void PageWheelBrowser::_buildFilteredList() {
    _filteredCount = 0;
    uint16_t total = OEM_DATABASE_COUNT + PageDashboard::getCustomCount();
    for (uint16_t i = 0; i < total && _filteredCount < 128; ++i) {
        if (_matchesCategory(i, _category)) _filteredIndices[_filteredCount++] = i;
    }
    if (_cursorIdx >= _filteredCount) _cursorIdx = (_filteredCount > 0) ? (_filteredCount - 1) : 0;
}

uint16_t PageWheelBrowser::getSelectedGlobalIndex() const {
    return (_filteredCount == 0 || _cursorIdx >= _filteredCount) ? 0 : _filteredIndices[_cursorIdx];
}

void PageWheelBrowser::render(bool fullRedraw) {
    if (!_isOpen) return;
    if (fullRedraw) {
        _gfx->fillScreen(TFT_BLACK); _drawHeader(); _drawList(true); _drawPreview();
        _lastCategory = static_cast<uint8_t>(_category); _lastCursorIdx = _cursorIdx; return;
    }
    if (static_cast<uint8_t>(_category) != _lastCategory) {
        _drawHeader(); _drawList(true); _drawPreview();
        _lastCategory = static_cast<uint8_t>(_category); _lastCursorIdx = _cursorIdx; return;
    }
    if (_cursorIdx != _lastCursorIdx || _needsListRedraw) {
        _drawList(false); _drawPreview(); _lastCursorIdx = _cursorIdx; _needsListRedraw = false;
    }
}

void PageWheelBrowser::_drawHeader() {
    _gfx->fillRect(0, 0, 480, 42, 0x0841); _gfx->drawFastHLine(0, 42, 480, 0x52AA);
    uint8_t catIdx = static_cast<uint8_t>(_category); int32_t xPos = 20;
    for (uint8_t c = 0; c < 7; ++c) {
        int32_t w = CAT_WIDTHS[c]; bool active = (c == catIdx);
        _gfx->fillRoundRect(xPos, 5, w, 32, 5, active ? 0x07E0 : 0x18C3);
        if (active) _gfx->drawRoundRect(xPos, 5, w, 32, 5, 0xFFE0);
        else _gfx->drawRoundRect(xPos, 5, w, 32, 5, 0x31A6);
        _gfx->setTextColor(active ? TFT_BLACK : TFT_WHITE, active ? 0x07E0 : 0x18C3);
        _gfx->setTextSize(1); _gfx->drawCenterString(CAT_NAMES[c], xPos + (w / 2), 16);
        xPos += (w + 4);
    }
}

void PageWheelBrowser::_drawList(bool forceAll) {
    int16_t startSlot = (_cursorIdx / 3) * 3; int32_t yBase = 46;
    if (forceAll) { _gfx->fillRect(6, 44, 276, 272, 0x0841); _gfx->drawRoundRect(6, 44, 276, 272, 6, 0x52AA); }

    for (uint8_t s = 0; s < 3; ++s) {
        int16_t itemIdx = startSlot + s; int32_t y = yBase + (s * 82); bool isSel = (itemIdx == _cursorIdx);
        if (itemIdx < _filteredCount) {
            uint16_t gIdx = _filteredIndices[itemIdx];
            const char* name = (gIdx < OEM_DATABASE_COUNT) ? OEM_DATABASE_PRESETS[gIdx].name : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT)->name;
            uint8_t teeth = (gIdx < OEM_DATABASE_COUNT) ? OEM_DATABASE_PRESETS[gIdx].totalTeeth : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT)->totalTeeth;
            uint8_t mTeeth = (gIdx < OEM_DATABASE_COUNT) ? OEM_DATABASE_PRESETS[gIdx].missingTeeth : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT)->missingTeeth;
            uint8_t cams = (gIdx < OEM_DATABASE_COUNT) ? OEM_DATABASE_PRESETS[gIdx].camCount : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT)->camCount;

            uint32_t bg = isSel ? 0x18C3 : 0x10A2;
            _gfx->fillRoundRect(12, y + 2, 264, 76, 5, bg);
            _gfx->drawRoundRect(12, y + 2, 264, 76, 5, isSel ? 0xFFE0 : 0x31A6);
            if (isSel) _gfx->drawRoundRect(13, y + 3, 262, 74, 4, 0xFFE0);

            char titleLine1[28]{}, titleLine2[36]{}, fullTitle[64];
            snprintf(fullTitle, sizeof(fullTitle), "%d. %s", itemIdx + 1, name);
            bool isMultiLine = (strlen(fullTitle) > 19);
            if (!isMultiLine) strncpy(titleLine1, fullTitle, sizeof(titleLine1) - 1);
            else {
                int split = 18; while (split > 3 && fullTitle[split] != ' ') split--;
                if (split <= 3) split = 18;
                strncpy(titleLine1, fullTitle, split); titleLine1[split] = '\0';
                strncpy(titleLine2, fullTitle + split + (fullTitle[split] == ' ' ? 1 : 0), sizeof(titleLine2) - 1);
            }

            _gfx->setTextColor(isSel ? 0xFFE0 : TFT_WHITE, bg); _gfx->setTextSize(2);
            _gfx->drawString(titleLine1, 18, y + 6);
            if (isMultiLine) {
                _gfx->drawString(titleLine2, 18, y + 26);
                _gfx->setTextSize(1); _gfx->setTextColor(isSel ? 0x07FF : 0x07E0, bg);
                char subBuf[40]; snprintf(subBuf, sizeof(subBuf), "CKP:%u-%u (50%%) | CMP:%u Evs", teeth, mTeeth, cams);
                _gfx->drawString(subBuf, 18, y + 54);
            } else {
                _gfx->setTextSize(1); _gfx->setTextColor(isSel ? 0x07FF : 0x07E0, bg);
                char subBuf[40]; snprintf(subBuf, sizeof(subBuf), "Pola CKP: %u-%u Gigi (Duty 50%%)", teeth, mTeeth);
                _gfx->drawString(subBuf, 18, y + 34);
                _gfx->setTextColor(isSel ? 0xFFE0 : 0x07FF, bg);
                char camBuf[40]; snprintf(camBuf, sizeof(camBuf), "Sinyal CMP: %u Event Fase Cam", cams);
                _gfx->drawString(camBuf, 18, y + 54);
            }
        } else { _gfx->fillRect(12, y + 2, 264, 76, 0x0841); }
    }
    _gfx->fillRect(12, 294, 264, 20, 0x0841); _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
    char pBuf[36]; snprintf(pBuf, sizeof(pBuf), "Hal %d/%d (Total %d Pola)", (_cursorIdx / 3) + 1, ((_filteredCount + 2) / 3), _filteredCount);
    _gfx->drawString(pBuf, 18, 298);
}

void PageWheelBrowser::_drawPreview() {
    _gfx->fillRoundRect(286, 44, 188, 272, 6, 0x0841); _gfx->drawRoundRect(286, 44, 188, 272, 6, 0x52AA);
    _gfx->setTextColor(0x07E0, 0x0841); _gfx->setTextSize(1); _gfx->drawString("PREVIEW WAVEFORM:", 292, 52);
    if (_filteredCount == 0 || _cursorIdx >= _filteredCount) {
        _gfx->setTextColor(0xF800, 0x0841); _gfx->setTextSize(2); _gfx->drawString("Tidak Ada Pola", 295, 140); return;
    }
    uint16_t gIdx = _filteredIndices[_cursorIdx];
    const WheelPresetItem* p = (gIdx < OEM_DATABASE_COUNT) ? &OEM_DATABASE_PRESETS[gIdx] : PageDashboard::getCustomPreset(gIdx - OEM_DATABASE_COUNT);
    if (!p) return;

    EcuEngine::ParametricWheel wheel{};
    wheel.totalTeeth = p->totalTeeth; wheel.missingTeeth = p->missingTeeth;
    wheel.missingPosition = p->missingPosition; wheel.dutyCycle = p->dutyCycle; wheel.inverted = p->inverted;
    EcuEngine::CamEventTable cam{};
    for (uint8_t i = 0; i < p->camCount; ++i) cam.addEvent(p->camAngles[i], p->camHighs[i]);
    _canvas.render(wheel, cam, 290, 66);

    _gfx->fillRect(290, 142, 180, 110, 0x0841); _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(2);
    if (strlen(p->name) <= 14) _gfx->drawString(p->name, 292, 144);
    else {
        char line1[24]{}, line2[32]{}; int split = 14;
        while (split > 0 && p->name[split] != ' ') split--;
        if (split <= 0) split = 14;
        strncpy(line1, p->name, split); line1[split] = '\0';
        strncpy(line2, p->name + split + (p->name[split] == ' ' ? 1 : 0), sizeof(line2) - 1);
        _gfx->drawString(line1, 292, 144); _gfx->drawString(line2, 292, 164);
    }
    _gfx->setTextSize(1); int32_t specY = (strlen(p->name) <= 14) ? 172 : 188;
    _gfx->setTextColor(0xFFE0, 0x0841);
    char buf[48]; snprintf(buf, sizeof(buf), "CKP : %u-%u (%u Gigi)", p->totalTeeth, p->missingTeeth, p->totalTeeth); _gfx->drawString(buf, 292, specY);
    _gfx->setTextColor(0x07FF, 0x0841); snprintf(buf, sizeof(buf), "Duty: %.0f%% | %s", p->dutyCycle * 100.0f, p->inverted ? "Inv (Low)" : "Normal High"); _gfx->drawString(buf, 292, specY + 18);
    _gfx->setTextColor(0x07E0, 0x0841); snprintf(buf, sizeof(buf), "CMP : %u Pulsa Cam (0-720d)", p->camCount); _gfx->drawString(buf, 292, specY + 36);

    _gfx->fillRoundRect(290, 258, 180, 52, 6, 0x03E0); _gfx->drawRoundRect(290, 258, 180, 52, 6, 0x07E0);
    _gfx->setTextColor(0x07E0, 0x03E0); _gfx->setTextSize(2); _gfx->drawCenterString("KLIK PILIH", 380, 274);
}

void PageWheelBrowser::onEncoderTurn(int32_t delta) {
    if (!_isOpen || _filteredCount == 0) return;
    int16_t next = _cursorIdx + delta;
    if (next < 0) next = 0;
    if (next >= _filteredCount) next = _filteredCount - 1;
    if (next != _cursorIdx) { _cursorIdx = next; _needsListRedraw = true; }
}

void PageWheelBrowser::onJoystickAction(EcuHal::JoyAction action) {
    if (!_isOpen) return;
    if (action == EcuHal::JoyAction::Up && _cursorIdx > 0) { _cursorIdx--; _needsListRedraw = true; }
    else if (action == EcuHal::JoyAction::Down && _cursorIdx + 1 < _filteredCount) { _cursorIdx++; _needsListRedraw = true; }
    else if (action == EcuHal::JoyAction::Left) {
        uint8_t c = static_cast<uint8_t>(_category); _category = static_cast<WheelCategory>((c > 0) ? (c - 1) : 6);
        _buildFilteredList(); _cursorIdx = 0; _needsListRedraw = true;
    } else if (action == EcuHal::JoyAction::Right) {
        uint8_t c = static_cast<uint8_t>(_category); _category = static_cast<WheelCategory>((c + 1) % 7);
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

    outWheel.totalTeeth = p->totalTeeth; outWheel.missingTeeth = p->missingTeeth;
    outWheel.missingPosition = p->missingPosition; outWheel.dutyCycle = p->dutyCycle; outWheel.inverted = p->inverted;
    outCam.clear();
    for (uint8_t i = 0; i < p->camCount; ++i) outCam.addEvent(p->camAngles[i], p->camHighs[i]);
    if (outName && maxNameLen > 0) { strncpy(outName, p->name, maxNameLen - 1); outName[maxNameLen - 1] = '\0'; }
    _isOpen = false;
    return true;
}

} // namespace EcuUi
