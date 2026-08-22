#pragma once
#include <LovyanGFX.hpp>
#include "parametric_pattern.h"

namespace EcuUi {

/**
 * @brief Dual-Trace Oscilloscope Canvas untuk memvisualisasikan pulsa CKP (Kuning)
 * dan CMP (Hijau) sepanjang siklus 0 - 720 derajat pada layar TFT.
 */
class WaveformCanvas {
public:
    WaveformCanvas(LovyanGFX* gfx);
    ~WaveformCanvas();

    bool init(int32_t width = 440, int32_t height = 80);
    void render(const EcuEngine::ParametricWheel& wheel, 
                const EcuEngine::CamEventTable& cam,
                int32_t screenX = 20, int32_t screenY = 125);

private:
    LovyanGFX* _gfx;
    LGFX_Sprite _sprite;
    int32_t _width{440};
    int32_t _height{80};
    bool _initialized{false};

    void _drawGrid();
    void _drawCkpTrace(const EcuEngine::ParametricWheel& wheel);
    void _drawCmpTrace(const EcuEngine::CamEventTable& cam);
};

} // namespace EcuUi
