#include "page_capture.h"

namespace EcuUi {

PageCapture::PageCapture(LovyanGFX* gfx) : _gfx(gfx), _canvas(gfx) {}

void PageCapture::init(EcuHal::CaptureDriver* driver, EcuEngine::SignalSniffer* sniffer) {
    _driver = driver;
    _sniffer = sniffer;
    _canvas.init(448, 76);
}

void PageCapture::_processCaptureData() {
    if (!_driver || !_sniffer || !_driver->isDone()) return;

    const EcuHal::CaptureEvent* rawBuf = _driver->getBuffer();
    uint16_t count = _driver->getEventCount();
    if (count == 0 || !rawBuf) return;

    static EcuEngine::RawSignalEdge s_edges[512];
    size_t copyCount = count > 512 ? 512 : count;
    for (size_t i = 0; i < copyCount; ++i) {
        s_edges[i] = { rawBuf[i].timestampUs, rawBuf[i].channel, rawBuf[i].level };
    }

    _lastResult = _sniffer->decode(s_edges, copyCount);
}

void PageCapture::render(uint8_t subTab, bool fullRedraw, bool isEditMode) {
    if (_driver && _driver->isDone() && !_hasProcessedCapture) {
        _hasProcessedCapture = true;
        _processCaptureData();
    }

    switch (subTab) {
        case 0: {
            if (fullRedraw) {
                _gfx->fillRect(8, 44, 464, 268, TFT_BLACK);
                _gfx->fillRoundRect(40, 100, 400, 130, 8, 0x0841);
                _gfx->drawRoundRect(40, 100, 400, 130, 8, 0xF800);
                _gfx->setTextColor(0xF800, 0x0841); _gfx->setTextSize(2);
                _gfx->drawCenterString("< KELUAR KE MENU UTAMA", 240, 125);
                _gfx->setTextColor(TFT_WHITE, 0x0841); _gfx->setTextSize(1);
                _gfx->drawCenterString("Tekan / Klik Knob untuk Keluar", 240, 165);
                _gfx->setTextColor(0x07E0, 0x0841);
                _gfx->drawCenterString("Geser Joystick ke Kanan (>) untuk Batal", 240, 190);
            }
            break;
        }
        case 1: _renderLiveTab(fullRedraw); break;
        case 2: _renderDataTab(fullRedraw); break;
        case 3: _renderCamTab(fullRedraw); break;
        default: break;
    }
}

void PageCapture::_renderLiveTab(bool fullRedraw) {
    if (fullRedraw) {
        _gfx->fillRect(0, 40, 480, 280, 0x0841);
        _gfx->fillRoundRect(8, 44, 464, 268, 8, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);

        if (_lastResult.success) {
            _canvas.render(_lastResult.wheel, _lastResult.cam, 16, 50);
        } else {
            _gfx->fillRoundRect(16, 50, 448, 76, 4, 0x0841);
            _gfx->drawRoundRect(16, 50, 448, 76, 4, 0x52AA);
            _gfx->setTextColor(0x52AA, 0x0841); _gfx->setTextSize(2);
            _gfx->drawString("PRE-FLIGHT DIAGNOSTICS ACTIVE", 60, 78);
        }

        _gfx->fillRoundRect(16, 132, 448, 38, 5, 0x0841);
        _gfx->drawRoundRect(16, 132, 448, 38, 5, 0x52AA);
        _gfx->fillRoundRect(16, 174, 448, 88, 5, 0x0841);
        _gfx->drawRoundRect(16, 174, 448, 88, 5, 0x52AA);
        _gfx->fillRoundRect(16, 266, 448, 40, 5, 0x18C3);
        _gfx->drawRoundRect(16, 266, 448, 40, 5, 0x52AA);

        _lastDrawnState = 0xFF; _lastDrawnRpm = 0xFFFFFFFF;
    }

    uint8_t curState = _driver ? static_cast<uint8_t>(_driver->getState()) : 0;
    if (curState != _lastDrawnState || fullRedraw) {
        _gfx->fillRect(24, 138, 432, 26, 0x0841);
        _gfx->setTextSize(2);
        if (curState == 0) {
            _gfx->setTextColor(0x52AA, 0x0841); _gfx->drawString("[ IDLE / STANDBY ]", 24, 142);
        } else if (curState == 1) {
            _gfx->setTextColor(0xFFE0, 0x0841); _gfx->drawString("[ ARMED - MENUNGGU PULSA ]", 24, 142);
        } else if (curState == 2) {
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->drawString("[ CAPTURING BUFFER... ]", 24, 142);
        } else if (curState == 3) {
            _gfx->setTextColor(0x07E0, 0x0841); _gfx->drawString("[ CAPTURE SELESAI (DONE) ]", 24, 142);
        }
        _lastDrawnState = curState;
    }

    if (fullRedraw || (_driver && _driver->isDone())) {
        _gfx->fillRect(24, 180, 432, 76, 0x0841);
        _gfx->setTextSize(2);
        if (_lastResult.success) {
            _gfx->setTextColor(0x07E0, 0x0841);
            char rpmBuf[32]; snprintf(rpmBuf, sizeof(rpmBuf), "RPM: %u RPM", (unsigned)_lastResult.detectedRpm);
            _gfx->drawString(rpmBuf, 24, 182);

            _gfx->setTextColor(0xFFE0, 0x0841);
            char patBuf[48];
            snprintf(patBuf, sizeof(patBuf), "Pola: %u-%u (Duty: %.0f%%)",
                     (unsigned)_lastResult.wheel.totalTeeth, (unsigned)_lastResult.wheel.missingTeeth,
                     _lastResult.wheel.dutyCycle * 100.0f);
            _gfx->drawString(patBuf, 24, 206);

            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            char matchBuf[64]; snprintf(matchBuf, sizeof(matchBuf), "Deteksi Profil: %s", _lastResult.matchedVehicle);
            _gfx->drawString(matchBuf, 24, 234);
        } else {
            _gfx->setTextColor(0x52AA, 0x0841); _gfx->drawString("Status: Menunggu Sinyal Trigger CKP/CMP...", 24, 196);
            _gfx->setTextColor(0x07FF, 0x0841); _gfx->setTextSize(1);
            _gfx->drawString("Hubungkan probe sniffer ke pin CKP (GPIO 15) & CMP (GPIO 16)", 24, 230);
        }
    }

    if (fullRedraw) {
        _gfx->setTextColor(TFT_WHITE, 0x18C3); _gfx->setTextSize(1);
        _gfx->drawString("1x Klik: ARM Sniffer | 2x Klik: Simpan Custom | Geser Kanan: Data", 24, 280);
    }
}

void PageCapture::_renderDataTab(bool fullRedraw) {
    if (fullRedraw) {
        _gfx->fillRect(0, 40, 480, 280, 0x0841);
        _gfx->fillRoundRect(8, 44, 464, 268, 8, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);
        _gfx->setTextColor(0x07E0, 0x10A2); _gfx->setTextSize(2);
        _gfx->drawString("DECODE DATA TELEMETRY", 20, 52);
        _gfx->drawFastHLine(20, 74, 440, 0x52AA);

        _gfx->setTextColor(TFT_WHITE, 0x10A2); _gfx->setTextSize(1);
        if (_lastResult.success) {
            char b[64];
            snprintf(b, sizeof(b), "Total Gigi Fisik: %u", (unsigned)_lastResult.wheel.totalTeeth); _gfx->drawString(b, 24, 85);
            snprintf(b, sizeof(b), "Missing Teeth   : %u", (unsigned)_lastResult.wheel.missingTeeth); _gfx->drawString(b, 24, 105);
            snprintf(b, sizeof(b), "Duty Cycle      : %.1f %%", _lastResult.wheel.dutyCycle * 100.0f); _gfx->drawString(b, 24, 125);
            snprintf(b, sizeof(b), "Inverted Polar  : %s", _lastResult.wheel.inverted ? "YES (Active Low)" : "NO (Active High)"); _gfx->drawString(b, 24, 145);
            snprintf(b, sizeof(b), "Engine Cycle RPM: %u RPM", (unsigned)_lastResult.detectedRpm); _gfx->drawString(b, 24, 165);
            snprintf(b, sizeof(b), "Matched OEM DB  : %s", _lastResult.matchedVehicle); _gfx->drawString(b, 24, 185);
        } else {
            _gfx->setTextColor(0x52AA, 0x10A2); _gfx->setTextSize(2);
            _gfx->drawString("Tidak ada decode data. Silakan lakukan capture.", 24, 120);
        }
    }
}

void PageCapture::_renderCamTab(bool fullRedraw) {
    if (fullRedraw) {
        _gfx->fillRect(0, 40, 480, 280, 0x0841);
        _gfx->fillRoundRect(8, 44, 464, 268, 8, 0x10A2);
        _gfx->drawRoundRect(8, 44, 464, 268, 8, 0x52AA);
        _gfx->setTextColor(0x07FF, 0x10A2); _gfx->setTextSize(2);
        _gfx->drawString("CAM EVENT DECODER (0-720 DEG)", 20, 52);
        _gfx->drawFastHLine(20, 74, 440, 0x52AA);

        uint8_t camCount = _lastResult.cam.getEventCount();
        const auto* events = _lastResult.cam.getEvents();
        _gfx->setTextColor(TFT_WHITE, 0x10A2); _gfx->setTextSize(1);

        if (_lastResult.success && camCount > 0) {
            char h[48]; snprintf(h, sizeof(h), "Terdeteksi %u event transisi noken as (CMP):", (unsigned)camCount);
            _gfx->drawString(h, 24, 85);
            for (uint8_t i = 0; i < camCount && i < 8; ++i) {
                char evBuf[64];
                snprintf(evBuf, sizeof(evBuf), "Event #%u: Sudut %5.1f deg -> Polar: %s",
                         (unsigned)(i + 1), events[i].angleDeg, events[i].levelHigh ? "HIGH [1]" : "LOW  [0]");
                _gfx->drawString(evBuf, 24, 110 + (i * 18));
            }
        } else {
            _gfx->setTextColor(0x52AA, 0x10A2); _gfx->setTextSize(2);
            _gfx->drawString("Tidak ada sinyal CAM terdeteksi.", 24, 120);
        }
    }
}

void PageCapture::onEncoderClick(uint8_t subTab) {
    if (subTab == 1 && _driver) {
        _hasProcessedCapture = false;
        _driver->arm(512);
    }
}

void PageCapture::onEncoderDoubleClick(EcuEngine::ParametricWheel& outWheel, EcuEngine::CamEventTable& outCam) {
    if (_lastResult.success) {
        outWheel = _lastResult.wheel;
        outCam = _lastResult.cam;
    }
}

} // namespace EcuUi
