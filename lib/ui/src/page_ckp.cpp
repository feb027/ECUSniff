#include "page_ckp.h"
#include <stdio.h>
#include <string.h>

namespace EcuUi {

PageCkp::PageCkp(LovyanGFX* gfx) : _gfx(gfx) {}

void PageCkp::_renderRow(uint8_t idx, const char* label, const char* value, bool isSelected) {
    int32_t y = 80 + (idx * 45);
    uint16_t bg = isSelected ? 0x1165 : 0x0841;
    uint16_t border = isSelected ? 0xFFE0 : 0x52AA;
    uint16_t textCol = isSelected ? TFT_WHITE : 0xCE79;
    uint16_t valCol = isSelected ? 0x07E0 : 0x07FF;

    // Erase bounding background to prevent sticking border
    _gfx->fillRoundRect(16, y - 6, 448, 44, 6, 0x10A2);

    _gfx->fillRoundRect(18, y - 4, 444, 40, 5, bg);
    _gfx->drawRoundRect(18, y - 4, 444, 40, 5, border);
    if (isSelected) {
        _gfx->drawRoundRect(17, y - 5, 446, 42, 6, 0xFFE0);
    }

    _gfx->setTextSize(2);
    _gfx->setTextColor(textCol, bg);
    _gfx->drawString(label, 26, y + 6);

    _gfx->setTextColor(valCol, bg);
    _gfx->drawString(value, 230, y + 6);
}

void PageCkp::render(uint8_t activePresetIdx, const EcuEngine::ParametricWheel& wheel, 
                     bool isEditMode, uint8_t selectedItem, bool fullRedraw) {
    bool dbPreset = (activePresetIdx < WheelDatabase::getWheelCount());
    const WheelDefinition* def = dbPreset ? WheelDatabase::getWheel(activePresetIdx) : nullptr;

    if (fullRedraw) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        _gfx->setTextSize(2);
        _gfx->setTextColor(0xFFE0, 0x10A2);
        _gfx->drawString("PROFIL TRIGGER CRANKSHAFT (CKP)", 24, 54);

        for (uint8_t i = 0; i < 5; ++i) {
            char labelBuf[32]{};
            char valBuf[36]{};

            if (def) {
                switch (i) {
                    case 0: {
                        strncpy(labelBuf, "Pola Mesin  :", sizeof(labelBuf));
                        char cleanName[22]{};
                        strncpy(cleanName, def->shortName ? def->shortName : def->friendlyName, sizeof(cleanName) - 1);
                        snprintf(valBuf, sizeof(valBuf), "%s", cleanName);
                        break;
                    }
                    case 1:
                        strncpy(labelBuf, "Siklus Rotasi:", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%s", (def->cycleDegrees == WheelCycleDegrees::CRANK_360) ? "360d (Crank)" : "720d (4-Tak)");
                        break;
                    case 2:
                        strncpy(labelBuf, "Resolusi Seg :", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%u Segmen", def->totalEdges);
                        break;
                    case 3:
                        strncpy(labelBuf, "Pin Output   :", sizeof(labelBuf));
                        strncpy(valBuf, "GPIO 4 (CKP)", sizeof(valBuf));
                        break;
                    case 4:
                        strncpy(labelBuf, "Trigger Cam  :", sizeof(labelBuf));
                        strncpy(valBuf, def->hasCmp2 ? "CKP+CMP1+CMP2" : (def->hasCmp1 ? "CKP+CMP1" : "Crank Only"), sizeof(valBuf));
                        break;
                }
            } else {
                switch (i) {
                    case 0:
                        strncpy(labelBuf, "Total Gigi N :", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%-3u gigi", (unsigned)wheel.totalTeeth);
                        break;
                    case 1:
                        strncpy(labelBuf, "Missing Gap M:", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%-3u gigi", (unsigned)wheel.missingTeeth);
                        break;
                    case 2:
                        strncpy(labelBuf, "Posisi Gap   :", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%-3u     ", (unsigned)wheel.missingPosition);
                        break;
                    case 3:
                        strncpy(labelBuf, "Duty Cycle   :", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%-3d %%   ", static_cast<int>(wheel.dutyCycle * 100));
                        break;
                    case 4:
                        strncpy(labelBuf, "Polaritas    :", sizeof(labelBuf));
                        strncpy(valBuf, wheel.inverted ? "INVERTED" : "NORMAL  ", sizeof(valBuf));
                        break;
                }
            }
            _renderRow(i, labelBuf, valBuf, (selectedItem == i));
        }

        _lastDrawnItem = selectedItem;
        _lastEditMode = isEditMode;
        _lastPresetIdx = activePresetIdx;
        _lastWheel = wheel;
        return;
    }

    bool wheelChanged = (_lastWheel.totalTeeth != wheel.totalTeeth ||
                         _lastWheel.missingTeeth != wheel.missingTeeth ||
                         _lastWheel.missingPosition != wheel.missingPosition ||
                         _lastWheel.dutyCycle != wheel.dutyCycle ||
                         _lastWheel.inverted != wheel.inverted);

    if (_lastEditMode != isEditMode || _lastDrawnItem != selectedItem || _lastPresetIdx != activePresetIdx || wheelChanged) {
        for (uint8_t i = 0; i < 5; ++i) {
            char labelBuf[32]{};
            char valBuf[36]{};

            if (def) {
                switch (i) {
                    case 0: {
                        strncpy(labelBuf, "Pola Mesin  :", sizeof(labelBuf));
                        char cleanName[22]{};
                        strncpy(cleanName, def->shortName ? def->shortName : def->friendlyName, sizeof(cleanName) - 1);
                        snprintf(valBuf, sizeof(valBuf), "%s", cleanName);
                        break;
                    }
                    case 1:
                        strncpy(labelBuf, "Siklus Rotasi:", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%s", (def->cycleDegrees == WheelCycleDegrees::CRANK_360) ? "360d (Crank)" : "720d (4-Tak)");
                        break;
                    case 2:
                        strncpy(labelBuf, "Resolusi Seg :", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%u Segmen", def->totalEdges);
                        break;
                    case 3:
                        strncpy(labelBuf, "Pin Output   :", sizeof(labelBuf));
                        strncpy(valBuf, "GPIO 4 (CKP)", sizeof(valBuf));
                        break;
                    case 4:
                        strncpy(labelBuf, "Trigger Cam  :", sizeof(labelBuf));
                        strncpy(valBuf, def->hasCmp2 ? "CKP+CMP1+CMP2" : (def->hasCmp1 ? "CKP+CMP1" : "Crank Only"), sizeof(valBuf));
                        break;
                }
            } else {
                switch (i) {
                    case 0:
                        strncpy(labelBuf, "Total Gigi N :", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%-3u gigi", (unsigned)wheel.totalTeeth);
                        break;
                    case 1:
                        strncpy(labelBuf, "Missing Gap M:", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%-3u gigi", (unsigned)wheel.missingTeeth);
                        break;
                    case 2:
                        strncpy(labelBuf, "Posisi Gap   :", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%-3u     ", (unsigned)wheel.missingPosition);
                        break;
                    case 3:
                        strncpy(labelBuf, "Duty Cycle   :", sizeof(labelBuf));
                        snprintf(valBuf, sizeof(valBuf), "%-3d %%   ", static_cast<int>(wheel.dutyCycle * 100));
                        break;
                    case 4:
                        strncpy(labelBuf, "Polaritas    :", sizeof(labelBuf));
                        strncpy(valBuf, wheel.inverted ? "INVERTED" : "NORMAL  ", sizeof(valBuf));
                        break;
                }
            }
            _renderRow(i, labelBuf, valBuf, (selectedItem == i));
        }
        _lastDrawnItem = selectedItem;
        _lastEditMode = isEditMode;
        _lastPresetIdx = activePresetIdx;
        _lastWheel = wheel;
    }
}

} // namespace EcuUi
