#include "waveform_canvas.h"
#include <stdlib.h>

namespace EcuUi {

WaveformCanvas::WaveformCanvas(LovyanGFX* gfx)
    : _gfx(gfx), _sprite(gfx) {}

WaveformCanvas::~WaveformCanvas() {
    if (_initialized) {
        _sprite.deleteSprite();
    }
}

bool WaveformCanvas::init(int32_t width, int32_t height) {
    _width = width;
    _height = height;
    _sprite.setColorDepth(8);
    if (_sprite.createSprite(_width, _height) == nullptr) {
        return false;
    }
    _initialized = true;
    return true;
}

void WaveformCanvas::clear() {
    if (_initialized) {
        _sprite.fillSprite(0x0000);
    }
}

void WaveformCanvas::_calculateTrackGeometry(uint8_t numTracks, TrackGeometry* tracks) {
    if (numTracks == 0) return;
    int32_t headerH = (_height > 90) ? 14 : 12;
    int32_t usableH = _height - headerH - 4;
    int32_t trackH = usableH / numTracks;
    int32_t marginY = (_height > 90) ? ((trackH > 40) ? 6 : 4) : 3;

    for (uint8_t i = 0; i < numTracks; ++i) {
        tracks[i].yTop = headerH + 2 + (i * trackH);
        tracks[i].yHigh = tracks[i].yTop + marginY;
        tracks[i].yLow = tracks[i].yTop + trackH - marginY - 1;
    }
}

void WaveformCanvas::_drawGrid(uint8_t numTracks, bool hasCmp2, const TrackGeometry* tracks) {
    _sprite.fillSprite(0x0000); // TFT_BLACK

    _sprite.drawRect(0, 0, _width, _height, 0x39E7); // Dark gray border

    int32_t xOffset = 28;
    int32_t xPad = 8;
    int32_t availableW = _width - xOffset - xPad;

    // Vertical quarter markers (dotted lines for 180°, 360°, 540°)
    for (int i = 1; i <= 3; ++i) {
        int32_t x = xOffset + (i * availableW) / 4;
        for (int32_t y = 2; y < _height - 2; y += 4) {
            _sprite.drawPixel(x, y, 0x52AA);
        }
    }

    // Horizontal track separators
    for (uint8_t i = 1; i < numTracks; ++i) {
        _sprite.drawFastHLine(0, tracks[i].yTop - 1, _width, 0x2965);
    }

    // Degree labels at top header
    _sprite.setTextSize(1);
    _sprite.setTextColor(TFT_LIGHTGRAY, 0x0000);
    _sprite.drawString("0", xOffset - 2, 2);
    _sprite.drawString("360", xOffset + (availableW / 2) - 8, 2);
    _sprite.drawString("720", xOffset + availableW - 18, 2);

    // Channel labels on the left margin
    if (numTracks >= 1) {
        _sprite.setTextColor(TFT_YELLOW, 0x0000);
        _sprite.drawString("CKP", 4, tracks[0].yHigh);
    }
    if (numTracks >= 2) {
        _sprite.setTextColor(TFT_GREEN, 0x0000);
        _sprite.drawString(hasCmp2 ? "CM1" : "CMP", 4, tracks[1].yHigh);
    }
    if (numTracks >= 3) {
        _sprite.setTextColor(TFT_CYAN, 0x0000);
        _sprite.drawString("CM2", 4, tracks[2].yHigh);
    }
}

void WaveformCanvas::_drawBitArrayTrace(const uint8_t* bitArray, uint16_t totalEdges, uint16_t cycleDegrees,
                                        uint8_t bitMask, uint16_t color, int32_t yHigh, int32_t yLow,
                                        int32_t xOffset, int32_t availableW, int16_t phaseAdvanceDeg) {
    if (!bitArray || totalEdges == 0 || availableW <= 0) {
        _sprite.drawFastHLine(xOffset, yLow, availableW, color);
        return;
    }

    // Determine total segments across 720 degrees
    size_t numTotalSegments = (cycleDegrees == 360) ? (static_cast<size_t>(totalEdges) * 2) : totalEdges;
    if (numTotalSegments == 0) {
        _sprite.drawFastHLine(xOffset, yLow, availableW, color);
        return;
    }

    int32_t advSegs = 0;
    if (phaseAdvanceDeg != 0 && totalEdges > 0 && cycleDegrees > 0) {
        advSegs = ((int32_t)phaseAdvanceDeg * (int32_t)totalEdges) / (int32_t)cycleDegrees;
    }

    auto getLevel = [&](size_t s) -> uint8_t {
        int32_t baseIdx = (cycleDegrees == 360) ? (int32_t)(s % totalEdges) : (int32_t)s;
        int32_t srcIdx = baseIdx + advSegs;
        while (srcIdx < 0) srcIdx += totalEdges;
        srcIdx = srcIdx % totalEdges;
        return (bitArray[srcIdx] & bitMask) ? 1 : 0;
    };

    // Initial state before column 0
    uint8_t prevEndLevel = getLevel(0);

    for (int32_t x = 0; x < availableW; ++x) {
        size_t segStart = (static_cast<size_t>(x) * numTotalSegments) / availableW;
        size_t segEnd = (static_cast<size_t>(x + 1) * numTotalSegments) / availableW;
        if (segEnd <= segStart) segEnd = segStart + 1;
        if (segEnd > numTotalSegments) segEnd = numTotalSegments;

        bool hasHigh = false;
        bool hasLow = false;
        uint8_t firstLevel = 0;
        uint8_t lastLevel = 0;

        for (size_t s = segStart; s < segEnd; ++s) {
            uint8_t lvl = getLevel(s);
            if (lvl) hasHigh = true; else hasLow = true;
            if (s == segStart) firstLevel = lvl;
            lastLevel = lvl;
        }

        int32_t px = xOffset + x;

        // Transition from previous column to this column
        if (x > 0 && firstLevel != prevEndLevel) {
            int32_t yA = prevEndLevel ? yHigh : yLow;
            int32_t yB = firstLevel ? yHigh : yLow;
            int32_t yMin = (yA < yB) ? yA : yB;
            int32_t yH = abs(yB - yA) + 1;
            _sprite.drawFastVLine(px, yMin, yH, color);
        }

        // Draw within this pixel column
        if (hasHigh && hasLow) {
            int32_t yMin = (yHigh < yLow) ? yHigh : yLow;
            int32_t yH = abs(yLow - yHigh) + 1;
            _sprite.drawFastVLine(px, yMin, yH, color);
        } else if (hasHigh) {
            _sprite.drawPixel(px, yHigh, color);
        } else {
            _sprite.drawPixel(px, yLow, color);
        }

        prevEndLevel = lastLevel;
    }
}

void WaveformCanvas::render(const WheelDefinition* wheel, int32_t screenX, int32_t screenY, int8_t vvtAdvanceDeg) {
    if (!_initialized) return;

    if (!wheel || !wheel->bitArray) {
        _sprite.fillSprite(0x0000);
        _sprite.drawRect(0, 0, _width, _height, 0x39E7);
        _sprite.setTextColor(0xF800, 0x0000);
        _sprite.setTextSize(1);
        _sprite.drawCenterString("No Pattern Data", _width / 2, _height / 2 - 4);
        _sprite.pushSprite(screenX, screenY);
        return;
    }

    uint8_t numTracks = wheel->hasCmp2 ? 3 : 2;
    TrackGeometry tracks[3];
    _calculateTrackGeometry(numTracks, tracks);

    _drawGrid(numTracks, wheel->hasCmp2, tracks);

    int32_t xOffset = 28;
    int32_t xPad = 8;
    int32_t availableW = _width - xOffset - xPad;
    uint16_t deg = static_cast<uint16_t>(wheel->cycleDegrees);

    // Track 0: CKP (Yellow - Bit 0 / Mask 0x01) - Always 0 phase shift
    _drawBitArrayTrace(wheel->bitArray, wheel->totalEdges, deg, 0x01, TFT_YELLOW,
                       tracks[0].yHigh, tracks[0].yLow, xOffset, availableW, 0);

    // Track 1: CMP1 (Green - Bit 1 / Mask 0x02) - Dynamic VVT Advance
    _drawBitArrayTrace(wheel->bitArray, wheel->totalEdges, deg, 0x02, TFT_GREEN,
                       tracks[1].yHigh, tracks[1].yLow, xOffset, availableW, vvtAdvanceDeg);

    // Track 2: CMP2 (Cyan - Bit 2 / Mask 0x04) - Retarded
    if (wheel->hasCmp2) {
        int16_t cmp2Shift = -(int16_t)(((int32_t)vvtAdvanceDeg * 5) / 8);
        _drawBitArrayTrace(wheel->bitArray, wheel->totalEdges, deg, 0x04, TFT_CYAN,
                       tracks[2].yHigh, tracks[2].yLow, xOffset, availableW, cmp2Shift);
    }

    _sprite.pushSprite(screenX, screenY);
}

void WaveformCanvas::render(const EcuEngine::ParametricWheel& wheel, 
                            const EcuEngine::CamEventTable& cam,
                            int32_t screenX, int32_t screenY,
                            int8_t vvtAdvanceDeg) {
    if (!_initialized) return;

    uint8_t numTracks = 2;
    TrackGeometry tracks[2];
    _calculateTrackGeometry(numTracks, tracks);

    _drawGrid(numTracks, false, tracks);

    int32_t xOffset = 28;
    int32_t xPad = 8;
    int32_t availableW = _width - xOffset - xPad;

    _drawCkpTraceParametric(wheel, tracks[0].yHigh, tracks[0].yLow, xOffset, availableW);
    _drawCmpTraceParametric(cam, tracks[1].yHigh, tracks[1].yLow, xOffset, availableW, vvtAdvanceDeg);

    _sprite.pushSprite(screenX, screenY);
}

void WaveformCanvas::_drawCkpTraceParametric(const EcuEngine::ParametricWheel& wheel,
                                             int32_t yHigh, int32_t yLow,
                                             int32_t xOffset, int32_t availableW) {
    if (!wheel.isValid() || wheel.totalTeeth == 0 || availableW <= 0) {
        _sprite.drawFastHLine(xOffset, yLow, availableW, TFT_YELLOW);
        return;
    }

    size_t totalTeeth = static_cast<size_t>(wheel.totalTeeth) * 2; // 720 deg
    float pxPerTooth = static_cast<float>(availableW) / static_cast<float>(totalTeeth);

    int32_t lastX = xOffset;
    int32_t lastY = yLow;

    for (size_t t = 0; t < totalTeeth; ++t) {
        uint16_t toothIdx = t % wheel.totalTeeth;
        bool isMissing = (toothIdx >= wheel.missingPosition && 
                          toothIdx < (wheel.missingPosition + wheel.missingTeeth));

        int32_t xStart = xOffset + static_cast<int32_t>(t * pxPerTooth);
        int32_t xMid   = xStart + static_cast<int32_t>(pxPerTooth * wheel.dutyCycle);
        int32_t xEnd   = xOffset + static_cast<int32_t>((t + 1) * pxPerTooth);

        if (isMissing) {
            if (lastY != yLow) {
                _sprite.drawLine(lastX, lastY, xStart, yLow, TFT_YELLOW);
            } else if (xStart > lastX) {
                _sprite.drawFastHLine(lastX, yLow, xStart - lastX, TFT_YELLOW);
            }
            _sprite.drawFastHLine(xStart, yLow, xEnd - xStart, TFT_YELLOW);
            lastX = xEnd;
            lastY = yLow;
        } else {
            if (lastY != yHigh) {
                _sprite.drawLine(lastX, lastY, xStart, yHigh, TFT_YELLOW);
            } else if (xStart > lastX) {
                _sprite.drawFastHLine(lastX, yHigh, xStart - lastX, TFT_YELLOW);
            }
            _sprite.drawFastHLine(xStart, yHigh, xMid - xStart, TFT_YELLOW);
            _sprite.drawLine(xMid, yHigh, xMid, yLow, TFT_YELLOW);
            _sprite.drawFastHLine(xMid, yLow, xEnd - xMid, TFT_YELLOW);
            lastX = xEnd;
            lastY = yLow;
        }
    }

    if (lastX < xOffset + availableW) {
        _sprite.drawFastHLine(lastX, lastY, (xOffset + availableW) - lastX, TFT_YELLOW);
    }
}

void WaveformCanvas::_drawCmpTraceParametric(const EcuEngine::CamEventTable& cam,
                                             int32_t yHigh, int32_t yLow,
                                             int32_t xOffset, int32_t availableW, int16_t phaseAdvanceDeg) {
    if (availableW <= 0) return;

    uint8_t count = cam.getEventCount();
    const EcuEngine::CmpEvent* ev = cam.getEvents();

    if (count == 0) {
        _sprite.drawFastHLine(xOffset, yLow, availableW, TFT_GREEN);
        return;
    }

    int32_t lastX = xOffset;
    int32_t lastY = ev[0].levelHigh ? yLow : yHigh;

    for (uint8_t i = 0; i < count; ++i) {
        float shiftedAngle = ev[i].angleDeg - (float)phaseAdvanceDeg;
        while (shiftedAngle < 0.0f) shiftedAngle += 720.0f;
        while (shiftedAngle >= 720.0f) shiftedAngle -= 720.0f;

        int32_t eventX = xOffset + static_cast<int32_t>((shiftedAngle / 720.0f) * availableW);
        int32_t nextY = ev[i].levelHigh ? yHigh : yLow;

        if (eventX > lastX) {
            _sprite.drawFastHLine(lastX, lastY, eventX - lastX, TFT_GREEN);
        }
        int32_t topY = (lastY < nextY) ? lastY : nextY;
        int32_t h = abs(nextY - lastY) + 1;
        _sprite.drawFastVLine(eventX, topY, h, TFT_GREEN);

        lastX = eventX;
        lastY = nextY;
    }

    if (lastX < xOffset + availableW) {
        _sprite.drawFastHLine(lastX, lastY, (xOffset + availableW) - lastX, TFT_GREEN);
    }
}

} // namespace EcuUi
