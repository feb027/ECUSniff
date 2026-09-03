#pragma once
#include <LovyanGFX.hpp>
#include "../../engine/include/wheel_database.h"
#include "parametric_pattern.h"

namespace EcuUi {

/**
 * @brief Dynamic Multi-Trace Oscilloscope Canvas for visualising CKP (Yellow),
 * CMP1 (Green), and CMP2 (Cyan) pulses over 0 - 720 degree cycles.
 */
class WaveformCanvas {
public:
    explicit WaveformCanvas(LovyanGFX* gfx);
    ~WaveformCanvas();

    bool init(int32_t width = 440, int32_t height = 80);

    // Multi-channel rendering directly from PROGMEM bit-array WheelDefinition
    void render(const WheelDefinition* wheel, int8_t vvtAdvanceDeg = 0, int32_t screenX = 12, int32_t screenY = 184);

    // Backward-compatible render method for ParametricWheel + CamEventTable
    void render(const EcuEngine::ParametricWheel& wheel, 
                const EcuEngine::CamEventTable& cam,
                int8_t vvtAdvanceDeg = 0,
                int32_t screenX = 20, int32_t screenY = 125);

    void clear();

private:
    struct TrackGeometry {
        int32_t yTop;
        int32_t yHigh;
        int32_t yLow;
    };

    LovyanGFX*  _gfx;
    LGFX_Sprite _sprite;
    int32_t     _width{440};
    int32_t     _height{80};
    bool        _initialized{false};

    void _calculateTrackGeometry(uint8_t numTracks, TrackGeometry* tracks);
    void _drawGrid(uint8_t numTracks, bool hasCmp2, const TrackGeometry* tracks);
    void _drawBitArrayTrace(const uint8_t* bitArray, uint16_t totalEdges, uint16_t cycleDegrees,
                            uint8_t bitMask, uint16_t color, int32_t yHigh, int32_t yLow,
                            int32_t xOffset, int32_t availableW, int16_t phaseAdvanceDeg = 0);
    void _drawCkpTraceParametric(const EcuEngine::ParametricWheel& wheel, int32_t yHigh, int32_t yLow,
                                int32_t xOffset, int32_t availableW);
    void _drawCmpTraceParametric(const EcuEngine::CamEventTable& cam, int32_t yHigh, int32_t yLow,
                                int32_t xOffset, int32_t availableW, int16_t phaseAdvanceDeg = 0);
};

} // namespace EcuUi
