#include "page_capture.h"

namespace EcuUi {

PageCapture::PageCapture(LovyanGFX* gfx) : _gfx(gfx), _canvas(gfx) {}

void PageCapture::init(EcuHal::CaptureDriver* driver, EcuEngine::SignalSniffer* sniffer) {
    _driver = driver;
    _sniffer = sniffer;
    _canvas.init(448, 88);
}

void PageCapture::_processCaptureData() {
    if (!_driver || !_sniffer || !_driver->isDone()) return;

    const EcuHal::CaptureEvent* rawBuf = _driver->getBuffer();
    uint16_t count = _driver->getEventCount();

    EcuEngine::RawSignalEdge edges[384];
    size_t copyCount = count > 384 ? 384 : count;
    for (size_t i = 0; i < copyCount; ++i) {
        edges[i] = { rawBuf[i].timestampUs, rawBuf[i].channel, rawBuf[i].level };
    }

    _lastResult = _sniffer->decode(edges, copyCount);
}

void PageCapture::render(uint8_t subTab, bool fullRedraw, bool isEditMode) {
    if (_driver && _driver->isDone() && !_lastResult.success) {
        _processCaptureData();
    }

    switch (subTab) {
        case 1: _renderLiveTab(fullRedraw); break;
        case 2: _renderDataTab(fullRedraw); break;
        case 3: _renderCamTab(fullRedraw); break;
        default: break;
    }
}

void PageCapture::_renderLiveTab(bool fullRedraw) {
    if (fullRedraw) {
        _gfx->fillRect(8, 44, 464, 268, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        // 1. Live Oscilloscope Scope Canvas (448x88)
        if (_lastResult.success) {
            _canvas.render(_lastResult.wheel, _lastResult.cam, 16, 50);
        } else {
            _gfx->fillRoundRect(16, 50, 448, 88, 4, 0x0841);
            _gfx->drawRoundRect(16, 50, 448, 88, 4, 0x52AA);
            _gfx->setTextColor(0x52AA, 0x0841);
            _gfx->setTextSize(2);
            _gfx->drawString("OSILOSKOP: STANDBY MENUNGGU PULSA", 42, 84);
        }

        // 2. Status Card
        _gfx->fillRoundRect(16, 144, 448, 52, 6, 0x0841);
        _gfx->drawRoundRect(16, 144, 448, 52, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841);
        _gfx->setTextSize(1);
        _gfx->drawString("STATUS TRIGGER:", 28, 154);

        // 3. Telemetry Box
        _gfx->fillRoundRect(16, 202, 448, 74, 6, 0x0841);
        _gfx->drawRoundRect(16, 202, 448, 74, 6, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841);
        _gfx->setTextSize(1);
        _gfx->drawString("RPM Terdeteksi :", 28, 212);
        _gfx->drawString("Pola Kendaraan :", 28, 234);
        _gfx->drawString("Event Camshaft :", 28, 256);

        // 4. Helper footer
        _gfx->fillRect(16, 282, 448, 24, 0x0841);
        _gfx->drawRoundRect(16, 282, 448, 24, 4, 0x31A6);
        _gfx->setTextColor(0x07FF, 0x0841);
        _gfx->drawString("Klik: ARM / TRIGGER  |  Double-Click: REPLAY  |  Tab < MENU: Keluar", 26, 289);
        _lastDrawnState = 0xFF;
    }

    uint8_t curState = _driver ? static_cast<uint8_t>(_driver->getState()) : 0;
    if (curState != _lastDrawnState || fullRedraw) {
        _gfx->setTextSize(2);
        if (curState == 0) {
            _gfx->setTextColor(0xFFE0, 0x0841);
            _gfx->drawString("STANDBY (IDLE)       ", 155, 164);
        } else if (curState == 1) {
            _gfx->setTextColor(0xFDA0, 0x0841);
            _gfx->drawString("ARMED / WAITING PULSE", 155, 164);
        } else if (curState == 2) {
            _gfx->setTextColor(0x07FF, 0x0841);
            _gfx->drawString("MEREKAM SINYAL...    ", 155, 164);
        } else {
            if (_lastResult.success) {
                _gfx->setTextColor(0x07E0, 0x0841);
                _gfx->drawString("CAPTURE COMPLETE     ", 155, 164);
                if (!fullRedraw) {
                    _canvas.render(_lastResult.wheel, _lastResult.cam, 16, 50);
                }
            } else {
                _gfx->setTextColor(0xF800, 0x0841);
                _gfx->drawString("TIMEOUT (TIDAK ADA)  ", 155, 164);
                if (!fullRedraw) {
                    _gfx->fillRoundRect(16, 50, 448, 88, 4, 0x0841);
                    _gfx->drawRoundRect(16, 50, 448, 88, 4, 0xF800);
                    _gfx->setTextColor(0xF800, 0x0841);
                    _gfx->setTextSize(2);
                    _gfx->drawString("TIDAK ADA SINYAL MASUK", 100, 84);
                }
            }
        }
        _lastDrawnState = curState;
    }

    _gfx->setTextSize(1);
    char buf[48];
    if (_lastResult.success) {
        _gfx->setTextColor(0x07FF, 0x0841);
        snprintf(buf, sizeof(buf), "%04u RPM            ", (unsigned)_lastResult.detectedRpm);
        _gfx->drawString(buf, 160, 212);

        snprintf(buf, sizeof(buf), "%s (%.1f%%)         ", _lastResult.matchedVehicle, _lastResult.matchConfidence);
        _gfx->drawString(buf, 160, 234);

        snprintf(buf, sizeof(buf), "%u Pulsa Camshaft (CMP)   ", (unsigned)_lastResult.cam.getEventCount());
        _gfx->drawString(buf, 160, 256);
    } else {
        _gfx->setTextColor(0xCE79, 0x0841);
        _gfx->drawString("---- RPM                ", 160, 212);
        _gfx->drawString("Tidak Ada Sinyal Masuk  ", 160, 234);
        _gfx->drawString("0 Pulsa Camshaft (CMP)  ", 160, 256);
    }
}

void PageCapture::_renderDataTab(bool fullRedraw) {
    if (!fullRedraw) return;

    _gfx->fillRect(8, 44, 464, 268, 0x10A2);
    _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

    _gfx->setTextColor(0xFFE0, 0x10A2);
    _gfx->setTextSize(2);
    _gfx->drawString("HASIL DECODE & AUTO-MATCH", 24, 54);

    const char* labels[] = { "Pola Mobil (OEM):", "Total Gigi (N)  :", "Missing Gap (M) :", "Duty Cycle      :" };
    char valBuf[48];

    for (uint8_t i = 0; i < 4; ++i) {
        int32_t y = 84 + (i * 46);
        _gfx->fillRoundRect(18, y - 4, 444, 40, 5, 0x0841);
        _gfx->drawRoundRect(18, y - 4, 444, 40, 5, 0x52AA);
        _gfx->setTextColor(TFT_WHITE, 0x0841);
        _gfx->setTextSize(1);
        _gfx->drawString(labels[i], 28, y + 12);

        if (_lastResult.success) {
            _gfx->setTextSize(1);
            if (i == 0) snprintf(valBuf, sizeof(valBuf), "%s", _lastResult.matchedVehicle);
            else if (i == 1) snprintf(valBuf, sizeof(valBuf), "%-3u gigi (Pitch %.1f deg)", (unsigned)_lastResult.wheel.totalTeeth, _lastResult.wheel.getPitchAngleDeg());
            else if (i == 2) snprintf(valBuf, sizeof(valBuf), "%-3u gigi (TDC Ref #0)", (unsigned)_lastResult.wheel.missingTeeth);
            else if (i == 3) snprintf(valBuf, sizeof(valBuf), "%d %% Pulsa Aktif", (int)(_lastResult.wheel.dutyCycle * 100));
            _gfx->setTextColor(i == 0 ? 0x07E0 : 0xFFE0, 0x0841);
        } else {
            _gfx->setTextSize(1);
            snprintf(valBuf, sizeof(valBuf), "Belum Ada Data Sinyal");
            _gfx->setTextColor(0x52AA, 0x0841);
        }
        _gfx->drawString(valBuf, 160, y + 12);
    }

    _gfx->fillRect(18, 280, 444, 24, 0x0841);
    _gfx->drawRoundRect(18, 280, 444, 24, 4, 0x31A6);
    _gfx->setTextColor(0x07FF, 0x0841);
    _gfx->setTextSize(1);
    _gfx->drawString("Pola dicocokkan otomatis dengan pustaka database ECU", 56, 287);
}

void PageCapture::_renderCamTab(bool fullRedraw) {
    if (!fullRedraw) return;

    _gfx->fillRect(8, 44, 464, 268, 0x10A2);
    _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

    _gfx->setTextColor(0x07E0, 0x10A2);
    _gfx->setTextSize(2);
    _gfx->drawString("HASIL DECODE CAM (CMP)", 24, 54);

    char valBuf[48];
    uint8_t count = _lastResult.success ? _lastResult.cam.getEventCount() : 0;
    const EcuEngine::CmpEvent* evs = _lastResult.success ? _lastResult.cam.getEvents() : nullptr;

    for (uint8_t i = 0; i < 4; ++i) {
        int32_t y = 86 + (i * 46);
        _gfx->fillRoundRect(18, y - 4, 444, 40, 5, 0x0841);
        _gfx->drawRoundRect(18, y - 4, 444, 40, 5, 0x52AA);
        _gfx->setTextSize(2);

        if (_lastResult.success && evs && i < count) {
            snprintf(valBuf, sizeof(valBuf), "Event %u: %.1f deg -> %s",
                     i + 1, evs[i].angleDeg,
                     evs[i].levelHigh ? "HIGH" : "LOW");
            _gfx->setTextColor(TFT_WHITE, 0x0841);
        } else {
            snprintf(valBuf, sizeof(valBuf), "Event %u: --- (Tidak Terdeteksi)", i + 1);
            _gfx->setTextColor(0x52AA, 0x0841);
        }
        _gfx->drawString(valBuf, 32, y + 6);
    }

    _gfx->fillRect(18, 280, 444, 24, 0x0841);
    _gfx->drawRoundRect(18, 280, 444, 24, 4, 0x31A6);
    _gfx->setTextColor(0x07FF, 0x0841);
    _gfx->setTextSize(1);
    _gfx->drawString("Sudut cam dinormalisasi terhadap missing gap 0 deg TDC", 56, 287);
}

void PageCapture::onEncoderTurn(int32_t delta) {}

void PageCapture::onEncoderClick(uint8_t subTab) {
    if (subTab == 1 && _driver) {
        if (_driver->getState() == EcuHal::CaptureState::Armed) {
            _driver->stop();
        } else {
            _lastResult.success = false;
            _driver->arm(384);
        }
    }
}

void PageCapture::onEncoderDoubleClick(EcuEngine::ParametricWheel& wheel, EcuEngine::CamEventTable& cam) {
    if (_lastResult.success) {
        wheel = _lastResult.wheel;
        cam = _lastResult.cam;
        _gfx->fillRect(18, 280, 444, 24, 0x07E0);
        _gfx->setTextColor(TFT_BLACK, 0x07E0);
        _gfx->setTextSize(1);
        _gfx->drawString(">>> POLA CAPTURE BERHASIL DIMUAT KE GENERATOR! <<<", 60, 287);
    }
}

} // namespace EcuUi
