#include "page_cmp.h"
#include <stdio.h>
#include <string.h>

namespace EcuUi {

PageCmp::PageCmp(LovyanGFX* gfx) : _gfx(gfx) {}

void PageCmp::_renderRow(uint8_t idx, const char* label, bool isSelected) {
    int32_t y = 86 + (idx * 54);
    uint16_t bg = isSelected ? 0x1165 : 0x0841;
    uint16_t border = isSelected ? 0xFFE0 : 0x52AA;
    uint16_t textCol = isSelected ? TFT_WHITE : 0xCE79;

    // Erase bounding background to prevent sticking border
    _gfx->fillRoundRect(16, y - 6, 448, 48, 6, 0x10A2);

    _gfx->fillRoundRect(18, y - 4, 444, 44, 5, bg);
    _gfx->drawRoundRect(18, y - 4, 444, 44, 5, border);
    if (isSelected) {
        _gfx->drawRoundRect(17, y - 5, 446, 46, 6, 0xFFE0);
    }

    _gfx->setTextSize(2);
    _gfx->setTextColor(textCol, bg);
    _gfx->drawString(label, 26, y + 8);
}

void PageCmp::render(uint8_t activePresetIdx, const EcuEngine::CamEventTable& cam, 
                     bool isEditMode, uint8_t selectedItem, bool fullRedraw) {
    bool dbPreset = (activePresetIdx < WheelDatabase::getWheelCount());
    const WheelDefinition* def = dbPreset ? WheelDatabase::getWheel(activePresetIdx) : nullptr;

    char rowLabels[4][64]{};
    uint8_t totalRows = 0;

    if (def) {
        if (!def->hasCmp1 && !def->hasCmp2) {
            strncpy(rowLabels[0], "Tipe Pola: Crank Only (Tanpa CMP)", sizeof(rowLabels[0]));
            strncpy(rowLabels[1], "Pin GPIO 4: Sinyal CKP Aktif", sizeof(rowLabels[1]));
            strncpy(rowLabels[2], "Pin GPIO 5 & 6: Standby / Low", sizeof(rowLabels[2]));
            totalRows = 3;
        } else {
            uint16_t deg = static_cast<uint16_t>(def->cycleDegrees);
            
            // Scan CMP1 pulses (Bit 1 / Mask 0x02)
            bool inPulse = false;
            uint16_t pStart = 0;
            for (uint16_t s = 0; s < def->totalEdges; ++s) {
                bool isHigh = (def->bitArray[s] & 0x02) != 0;
                if (isHigh && !inPulse) {
                    inPulse = true;
                    pStart = s;
                } else if (!isHigh && inPulse) {
                    inPulse = false;
                    if (totalRows < 4) {
                        float sDeg = (static_cast<float>(pStart) * deg) / def->totalEdges;
                        float eDeg = (static_cast<float>(s) * deg) / def->totalEdges;
                        snprintf(rowLabels[totalRows++], sizeof(rowLabels[0]), 
                                 "CMP1 Ev%u: %.1f-%.1f (%.0f deg HIGH)", totalRows, sDeg, eDeg, eDeg - sDeg);
                    }
                }
            }
            if (inPulse && totalRows < 4) {
                float sDeg = (static_cast<float>(pStart) * deg) / def->totalEdges;
                float eDeg = static_cast<float>(deg);
                snprintf(rowLabels[totalRows++], sizeof(rowLabels[0]), 
                         "CMP1 Ev%u: %.1f-%.1f (%.0f deg HIGH)", totalRows, sDeg, eDeg, eDeg - sDeg);
            }

            // Scan CMP2 pulses (Bit 2 / Mask 0x04) if present
            if (def->hasCmp2) {
                inPulse = false;
                pStart = 0;
                for (uint16_t s = 0; s < def->totalEdges; ++s) {
                    bool isHigh = (def->bitArray[s] & 0x04) != 0;
                    if (isHigh && !inPulse) {
                        inPulse = true;
                        pStart = s;
                    } else if (!isHigh && inPulse) {
                        inPulse = false;
                        if (totalRows < 4) {
                            float sDeg = (static_cast<float>(pStart) * deg) / def->totalEdges;
                            float eDeg = (static_cast<float>(s) * deg) / def->totalEdges;
                            snprintf(rowLabels[totalRows++], sizeof(rowLabels[0]), 
                                     "CMP2 Ev%u: %.1f-%.1f (%.0f deg HIGH)", totalRows, sDeg, eDeg, eDeg - sDeg);
                        }
                    }
                }
                if (inPulse && totalRows < 4) {
                    float sDeg = (static_cast<float>(pStart) * deg) / def->totalEdges;
                    float eDeg = static_cast<float>(deg);
                    snprintf(rowLabels[totalRows++], sizeof(rowLabels[0]), 
                             "CMP2 Ev%u: %.1f-%.1f (%.0f deg HIGH)", totalRows, sDeg, eDeg, eDeg - sDeg);
                }
            }
        }
    } else {
        uint8_t count = cam.getEventCount();
        const auto* evs = cam.getEvents();
        if (count == 0) {
            strncpy(rowLabels[0], "Tidak Ada Pulsa Cam (0 Event)", sizeof(rowLabels[0]));
            totalRows = 1;
        } else {
            for (uint8_t i = 0; i < 4 && i < count; ++i) {
                snprintf(rowLabels[i], sizeof(rowLabels[0]), "Event %u: %5.1f deg -> %s",
                         i + 1, evs[i].angleDeg, evs[i].levelHigh ? "HIGH" : "LOW ");
                totalRows++;
            }
        }
    }

    if (fullRedraw) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        _gfx->setTextSize(2);
        _gfx->setTextColor(0x07E0, 0x10A2);
        _gfx->drawString("TABEL FASE & PULSA CAMSHAFT (CMP)", 24, 54);

        for (uint8_t i = 0; i < 4 && i < totalRows; ++i) {
            _renderRow(i, rowLabels[i], (selectedItem == i));
        }
        _lastDrawnItem = selectedItem;
        _lastPresetIdx = activePresetIdx;
        return;
    }

    if (_lastDrawnItem != selectedItem || _lastPresetIdx != activePresetIdx) {
        for (uint8_t i = 0; i < 4 && i < totalRows; ++i) {
            _renderRow(i, rowLabels[i], (selectedItem == i));
        }
        _lastDrawnItem = selectedItem;
        _lastPresetIdx = activePresetIdx;
    }
}

} // namespace EcuUi
