#include "page_wheel_browser.h"
#include "wheel_database.h"
#include "page_dashboard.h"
#include <string.h>
#include <stdio.h>

namespace EcuUi {

static const char* CAT_NAMES[] = { "ALL", "TOYOTA", "HONDA", "MITSU", "NISSAN", "EURO/US", "UNIV", "CUSTOM" };
static const uint16_t CAT_WIDTHS[] = { 46, 58, 54, 52, 56, 64, 48, 62 };

PageWheelBrowser::PageWheelBrowser(LovyanGFX* gfx) : _gfx(gfx), _canvas(gfx) {}

void PageWheelBrowser::init() {
    // Waveform Canvas spans full width (456 x 124 px)
    _canvas.init(456, 124);
}

void PageWheelBrowser::open(uint16_t initialGlobalIdx, BrandCategory initialCategory) {
    _isOpen = true;
    _category = initialCategory;
    _buildFilteredList();
    _cursorIdx = 0;
    for (uint16_t i = 0; i < _filteredCount; ++i) {
        if (_filteredIndices[i] == initialGlobalIdx) {
            _cursorIdx = i;
            break;
        }
    }
    _lastCursorIdx = -1;
    _lastCategory = 0xFF;
    _needsListRedraw = true;
}

void PageWheelBrowser::close() {
    _isOpen = false;
}

bool PageWheelBrowser::matchesCategory(uint16_t globalIdx, BrandCategory cat) {
    if (cat == BrandCategory::ALL) return true;
    size_t dbCount = WheelDatabase::getWheelCount();
    if (globalIdx < dbCount) {
        if (cat == BrandCategory::CUSTOM) return false;
        const WheelDefinition* def = WheelDatabase::getWheel(globalIdx);
        return (def != nullptr && def->category == cat);
    }
    return (cat == BrandCategory::CUSTOM);
}

void PageWheelBrowser::_buildFilteredList() {
    _filteredCount = 0;
    size_t dbCount = WheelDatabase::getWheelCount();
    uint16_t customCount = PageDashboard::getCustomCount();
    uint16_t total = dbCount + customCount;

    for (uint16_t i = 0; i < total && _filteredCount < 128; ++i) {
        if (matchesCategory(i, _category)) {
            _filteredIndices[_filteredCount++] = i;
        }
    }
    if (_cursorIdx >= _filteredCount) {
        _cursorIdx = (_filteredCount > 0) ? (_filteredCount - 1) : 0;
    }
}

uint16_t PageWheelBrowser::getSelectedGlobalIndex() const {
    return (_filteredCount == 0 || _cursorIdx >= _filteredCount) ? 0 : _filteredIndices[_cursorIdx];
}

void PageWheelBrowser::render(bool fullRedraw) {
    if (!_isOpen) return;

    if (fullRedraw) {
        _gfx->fillScreen(TFT_BLACK);
        _drawHeader();
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
    int32_t xPos = 6;

    for (uint8_t c = 0; c < 8; ++c) {
        int32_t w = CAT_WIDTHS[c];
        bool active = (c == catIdx);

        uint16_t bg = active ? 0x07E0 : 0x18C3;
        uint16_t border = active ? 0xFFE0 : 0x31A6;
        uint16_t fg = active ? TFT_BLACK : TFT_WHITE;

        _gfx->fillRoundRect(xPos, 5, w, 32, 5, bg);
        _gfx->drawRoundRect(xPos, 5, w, 32, 5, border);
        if (active) {
            _gfx->drawRoundRect(xPos + 1, 6, w - 2, 30, 4, 0xFFE0);
        }

        _gfx->setTextColor(fg, bg);
        _gfx->setTextSize(1);
        _gfx->drawCenterString(CAT_NAMES[c], xPos + (w / 2), 16);
        xPos += (w + 4);
    }
}

void PageWheelBrowser::_drawList(bool forceAll) {
    int16_t startSlot = (_cursorIdx / 2) * 2;
    int32_t yBase = 46;

    for (uint8_t s = 0; s < 2; ++s) {
        int16_t itemIdx = startSlot + s;
        int32_t y = yBase + (s * 68);
        bool isSel = (itemIdx == _cursorIdx);

        if (itemIdx < _filteredCount) {
            uint16_t gIdx = _filteredIndices[itemIdx];
            size_t dbCount = WheelDatabase::getWheelCount();

            const char* titleName = nullptr;
            const char* shortName = nullptr;
            char specBuf[96];

            if (gIdx < dbCount) {
                const WheelDefinition* wheel = WheelDatabase::getWheel(gIdx);
                if (wheel) {
                    titleName = wheel->friendlyName;
                    shortName = wheel->shortName;
                    const char* cycleStr = (wheel->cycleDegrees == WheelCycleDegrees::CRANK_360) ? "360 deg" : "720 deg";
                    const char* channelStr = wheel->hasCmp2 ? "CKP+CMP1+CMP2" : (wheel->hasCmp1 ? "CKP+CMP1" : "CKP Only");
                    snprintf(specBuf, sizeof(specBuf), "%s | %s | %u Edges | [%s]", 
                             shortName ? shortName : "", cycleStr, wheel->totalEdges, channelStr);
                }
            } else {
                const WheelPresetItem* p = PageDashboard::getCustomPreset(gIdx - dbCount);
                if (p) {
                    titleName = p->name;
                    snprintf(specBuf, sizeof(specBuf), "Custom | %u-%u (%u Teeth) | %u Cam",
                             p->totalTeeth, p->missingTeeth, p->totalTeeth, p->camCount);
                }
            }

            if (!titleName) titleName = "Unknown Pattern";

            // Latar belakang bersih seragam 0x0841 tanpa blocking biru
            uint16_t bg = 0x0841;
            uint16_t border = isSel ? 0xFFE0 : 0x31A6;

            // Full width pattern card (456 x 64 px)
            _gfx->fillRoundRect(12, y, 456, 64, 6, bg);
            _gfx->drawRoundRect(12, y, 456, 64, 6, border);
            if (isSel) {
                _gfx->drawRoundRect(13, y + 1, 454, 62, 5, 0xFFE0);
            }

            // Pattern title (Font Size 2)
            char fullTitle[80];
            snprintf(fullTitle, sizeof(fullTitle), "%d. %s", itemIdx + 1, titleName);
            _gfx->setTextColor(isSel ? 0xFFE0 : TFT_WHITE, bg);
            _gfx->setTextSize(2);
            _gfx->drawString(fullTitle, 22, y + 10);

            // Pattern metadata & channel badges (Font Size 1)
            _gfx->setTextColor(isSel ? 0x07E0 : 0x8410, bg);
            _gfx->setTextSize(1);
            _gfx->drawString(specBuf, 22, y + 38);
        } else {
            _gfx->fillRect(12, y, 456, 64, TFT_BLACK);
        }
    }
}

void PageWheelBrowser::_drawPreview() {
    if (_filteredCount == 0 || _cursorIdx >= _filteredCount) {
        _gfx->fillRect(12, 184, 456, 124, 0x0841);
        _gfx->drawRoundRect(12, 184, 456, 124, 6, 0x52AA);
        _gfx->setTextColor(0xF800, 0x0841);
        _gfx->setTextSize(2);
        _gfx->drawCenterString("Tidak Ada Pola Dalam Kategori Ini", 240, 236);
        return;
    }

    uint16_t gIdx = _filteredIndices[_cursorIdx];
    size_t dbCount = WheelDatabase::getWheelCount();

    if (gIdx < dbCount) {
        const WheelDefinition* wheelDef = WheelDatabase::getWheel(gIdx);
        if (wheelDef) {
            _canvas.render(wheelDef, 12, 184);
        }
    } else {
        const WheelPresetItem* p = PageDashboard::getCustomPreset(gIdx - dbCount);
        if (p) {
            EcuEngine::ParametricWheel wheel{};
            wheel.totalTeeth = p->totalTeeth;
            wheel.missingTeeth = p->missingTeeth;
            wheel.missingPosition = p->missingPosition;
            wheel.dutyCycle = p->dutyCycle;
            wheel.inverted = p->inverted;

            EcuEngine::CamEventTable cam{};
            for (uint8_t i = 0; i < p->camCount; ++i) {
                cam.addEvent(p->camAngles[i], p->camHighs[i]);
            }

            _canvas.render(wheel, cam, 12, 184);
        }
    }
}

void PageWheelBrowser::onEncoderTurn(int32_t delta) {
    if (!_isOpen || _filteredCount == 0) return;
    int16_t next = _cursorIdx + delta;
    if (next < 0) next = 0;
    if (next >= _filteredCount) next = _filteredCount - 1;
    if (next != _cursorIdx) {
        _cursorIdx = next;
        _needsListRedraw = true;
    }
}

void PageWheelBrowser::onJoystickAction(EcuHal::JoyAction action) {
    if (!_isOpen) return;

    if (action == EcuHal::JoyAction::Up && _cursorIdx > 0) {
        _cursorIdx--;
        _needsListRedraw = true;
    } else if (action == EcuHal::JoyAction::Down && _cursorIdx + 1 < _filteredCount) {
        _cursorIdx++;
        _needsListRedraw = true;
    } else if (action == EcuHal::JoyAction::Left) {
        uint8_t c = static_cast<uint8_t>(_category);
        _category = static_cast<BrandCategory>((c > 0) ? (c - 1) : 7);
        _buildFilteredList();
        _cursorIdx = 0;
        _needsListRedraw = true;
    } else if (action == EcuHal::JoyAction::Right) {
        uint8_t c = static_cast<uint8_t>(_category);
        _category = static_cast<BrandCategory>((c + 1) % 8);
        _buildFilteredList();
        _cursorIdx = 0;
        _needsListRedraw = true;
    }
}

bool PageWheelBrowser::onEncoderClick(EcuEngine::ParametricWheel& outWheel,
                                     EcuEngine::CamEventTable& outCam,
                                     char* outName, size_t maxNameLen) {
    if (!_isOpen || _filteredCount == 0 || _cursorIdx >= _filteredCount) return false;
    uint16_t gIdx = _filteredIndices[_cursorIdx];
    size_t dbCount = WheelDatabase::getWheelCount();

    if (gIdx < dbCount) {
        const WheelDefinition* def = WheelDatabase::getWheel(gIdx);
        if (!def) return false;

        outWheel.totalTeeth = def->totalEdges / 2;
        outWheel.missingTeeth = 0;
        outWheel.missingPosition = 0;
        outWheel.dutyCycle = 0.50f;
        outWheel.inverted = false;

        outCam.clear();

        if (outName && maxNameLen > 0) {
            strncpy(outName, def->friendlyName, maxNameLen - 1);
            outName[maxNameLen - 1] = '\0';
        }
    } else {
        const WheelPresetItem* p = PageDashboard::getCustomPreset(gIdx - dbCount);
        if (!p) return false;

        outWheel.totalTeeth = p->totalTeeth;
        outWheel.missingTeeth = p->missingTeeth;
        outWheel.missingPosition = p->missingPosition;
        outWheel.dutyCycle = p->dutyCycle;
        outWheel.inverted = p->inverted;

        outCam.clear();
        for (uint8_t i = 0; i < p->camCount; ++i) {
            outCam.addEvent(p->camAngles[i], p->camHighs[i]);
        }

        if (outName && maxNameLen > 0) {
            strncpy(outName, p->name, maxNameLen - 1);
            outName[maxNameLen - 1] = '\0';
        }
    }

    _isOpen = false;
    return true;
}

} // namespace EcuUi
