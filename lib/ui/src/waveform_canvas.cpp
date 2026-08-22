#include "waveform_canvas.h"

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
    _sprite.setColorDepth(8); // 8-bit color depth menghemat RAM (440x80 = 35KB RAM)
    if (_sprite.createSprite(_width, _height) == nullptr) {
        return false;
    }
    _initialized = true;
    return true;
}

void WaveformCanvas::render(const EcuEngine::ParametricWheel& wheel, 
                            const EcuEngine::CamEventTable& cam,
                            int32_t screenX, int32_t screenY) {
    if (!_initialized) return;

    _drawGrid();
    _drawCkpTrace(wheel);
    _drawCmpTrace(cam);

    _sprite.pushSprite(screenX, screenY);
}

void WaveformCanvas::_drawGrid() {
    _sprite.fillSprite(0x00); // Black

    // Outer border
    _sprite.drawRect(0, 0, _width, _height, 0x39E7); // Dark gray border

    // Vertical angle divider lines (0, 180, 360, 540, 720 deg)
    for (int i = 1; i <= 3; ++i) {
        int32_t x = (i * _width) / 4;
        for (int32_t y = 2; y < _height - 2; y += 4) {
            _sprite.drawPixel(x, y, 0x52AA); // Dotted gridline
        }
    }

    // Mid divider between CKP & CMP
    _sprite.drawFastHLine(0, 40, _width, 0x2965);

    // Channel labels
    _sprite.setTextSize(1);
    _sprite.setTextColor(TFT_YELLOW, 0x00);
    _sprite.drawString("CKP", 4, 4);

    _sprite.setTextColor(TFT_GREEN, 0x00);
    _sprite.drawString("CMP", 4, 44);

    // Angle text markers
    _sprite.setTextColor(TFT_LIGHTGRAY, 0x00);
    _sprite.drawString("0", 25, 4);
    _sprite.drawString("360", (_width / 2) - 8, 4);
    _sprite.drawString("720", _width - 24, 4);
}

void WaveformCanvas::_drawCkpTrace(const EcuEngine::ParametricWheel& wheel) {
    if (!wheel.isValid()) return;

    int32_t yHigh = 12;
    int32_t yLow  = 34;
    size_t totalTeeth = static_cast<size_t>(wheel.totalTeeth) * 2; // 720 deg
    float pxPerTooth = static_cast<float>(_width - 30) / static_cast<float>(totalTeeth);
    int32_t xOffset = 25;

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
            _sprite.drawLine(lastX, lastY, xStart, yLow, TFT_YELLOW);
            _sprite.drawFastHLine(xStart, yLow, xEnd - xStart, TFT_YELLOW);
            lastX = xEnd;
            lastY = yLow;
        } else {
            // Rising
            _sprite.drawLine(lastX, lastY, xStart, yHigh, TFT_YELLOW);
            // High level
            _sprite.drawFastHLine(xStart, yHigh, xMid - xStart, TFT_YELLOW);
            // Falling
            _sprite.drawLine(xMid, yHigh, xMid, yLow, TFT_YELLOW);
            // Low level
            _sprite.drawFastHLine(xMid, yLow, xEnd - xMid, TFT_YELLOW);
            lastX = xEnd;
            lastY = yLow;
        }
    }
}

void WaveformCanvas::_drawCmpTrace(const EcuEngine::CamEventTable& cam) {
    int32_t yHigh = 50;
    int32_t yLow  = 72;
    int32_t xOffset = 25;
    int32_t availableW = _width - 30;

    uint8_t count = cam.getEventCount();
    const EcuEngine::CmpEvent* ev = cam.getEvents();

    if (count == 0) {
        _sprite.drawFastHLine(xOffset, yLow, availableW, TFT_GREEN);
        return;
    }

    int32_t lastX = xOffset;
    int32_t lastY = ev[0].levelHigh ? yLow : yHigh;

    for (uint8_t i = 0; i < count; ++i) {
        int32_t eventX = xOffset + static_cast<int32_t>((ev[i].angleDeg / 720.0f) * availableW);
        int32_t nextY = ev[i].levelHigh ? yHigh : yLow;

        _sprite.drawLine(lastX, lastY, eventX, lastY, TFT_GREEN);
        _sprite.drawLine(eventX, lastY, eventX, nextY, TFT_GREEN);
        lastX = eventX;
        lastY = nextY;
    }

    if (lastX < xOffset + availableW) {
        _sprite.drawFastHLine(lastX, lastY, (xOffset + availableW) - lastX, TFT_GREEN);
    }
}

} // namespace EcuUi
