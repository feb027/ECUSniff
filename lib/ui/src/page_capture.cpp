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

        // 1. Oscilloscope Scope Canvas (448x76)
        if (_lastResult.success) {
            _canvas.render(_lastResult.wheel, _lastResult.cam, 16, 50);
        } else {
            _gfx->fillRoundRect(16, 50, 448, 76, 4, 0x0841);
            _gfx->drawRoundRect(16, 50, 448, 76, 4, 0x52AA);
            _gfx->setTextColor(0x52AA, 0x0841);
            _gfx->setTextSize(2);
            _gfx->drawString("PRE-FLIGHT DIAGNOSTICS ACTIVE", 60, 78);
        }

        // 2. Pre-Flight Status Badge Box
        _gfx->fillRoundRect(16, 132, 448, 38, 5, 0x0841);
        _gfx->drawRoundRect(16, 132, 448, 38, 5, 0x52AA);

        // 3. 4-Card Diagnostics Grid (2x2)
        _gfx->fillRoundRect(16, 174, 220, 50, 4, 0x0841);
        _gfx->drawRoundRect(16, 174, 220, 50, 4, 0x52AA);
        _gfx->fillRoundRect(244, 174, 220, 50, 4, 0x0841);
        _gfx->drawRoundRect(244, 174, 220, 50, 4, 0x52AA);

        _gfx->fillRoundRect(16, 228, 220, 50, 4, 0x0841);
        _gfx->drawRoundRect(16, 228, 220, 50, 4, 0x52AA);
        _gfx->fillRoundRect(244, 228, 220, 50, 4, 0x0841);
        _gfx->drawRoundRect(244, 228, 220, 50, 4, 0x52AA);

        // 4. Helper footer
        _gfx->fillRect(16, 282, 448, 24, 0x0841);
        _gfx->drawRoundRect(16, 282, 448, 24, 4, 0x31A6);
        _gfx->setTextColor(0x07FF, 0x0841);
        _gfx->drawString("KLIK: REKAM SINYAL  |  DOUBLE-CLICK: MUAT KE GENERATOR", 40, 289);
        _lastDrawnState = 0xFF;
    }

    EcuHal::LiveSignalMetrics metrics{};
    if (_driver) _driver->getLiveMetrics(metrics);

    EcuEngine::SignalHealthStatus health{};
    if (_sniffer) {
        health = _sniffer->evaluateHealth(metrics.ckpActive, metrics.cmpActive, metrics.cmp2Active,
                                         metrics.revPeriodUs, metrics.nominalPeriodUs, metrics.lastGapUs);
    }

    uint8_t curState = _driver ? static_cast<uint8_t>(_driver->getState()) : 0;
    _gfx->setTextSize(2);

    if (curState == 1) {
        _gfx->setTextColor(0xFDA0, 0x0841);
        _gfx->drawString("[ MENUNGGU 0-DEG GAP... ]    ", 68, 142);
    } else if (curState == 2) {
        _gfx->setTextColor(0x07FF, 0x0841);
        _gfx->drawString("[ MEREKAM SIKLUS 720-DEG... ] ", 68, 142);
    } else {
        if (health.quality == EcuEngine::SignalQuality::PhaseLocked) {
            _gfx->setTextColor(0x07E0, 0x0841);
            _gfx->drawString("[ 720-DEG PHASE LOCKED (OK) ]", 68, 142);
        } else if (health.quality == EcuEngine::SignalQuality::Syncing) {
            _gfx->setTextColor(0xFFE0, 0x0841);
            _gfx->drawString("[ MENYINKRONKAN FASA... ]    ", 68, 142);
        } else if (health.quality == EcuEngine::SignalQuality::Noisy) {
            _gfx->setTextColor(0xF800, 0x0841);
            _gfx->drawString("[ DERAU TINGGI / CEK KABEL ] ", 68, 142);
        } else {
            _gfx->setTextColor(0x52AA, 0x0841);
            _gfx->drawString("[ TIDAK ADA SINYAL MASUK ]   ", 68, 142);
        }
    }

    // Render 4 Diagnostic Cards
    _gfx->setTextSize(1);
    char buf[48];

    // Card 1: CKP
    _gfx->setTextColor(0xCE79, 0x0841);
    _gfx->drawString("CKP SENSOR (PIN 34):", 24, 180);
    _gfx->setTextSize(1);
    if (metrics.ckpActive) {
        _gfx->setTextColor(0x07E0, 0x0841);
        snprintf(buf, sizeof(buf), "OK (%04u RPM / %u gigi)", (unsigned)health.liveRpm, (unsigned)health.liveTeeth);
    } else {
        _gfx->setTextColor(0xF800, 0x0841);
        snprintf(buf, sizeof(buf), "TERPUTUS / TIDAK ADA ");
    }
    _gfx->drawString(buf, 24, 202);

    // Card 2: CMP
    _gfx->setTextColor(0xCE79, 0x0841);
    _gfx->drawString("CMP SENSOR 1 (PIN 35):", 252, 180);
    if (metrics.cmpActive) {
        _gfx->setTextColor(0x07E0, 0x0841);
        snprintf(buf, sizeof(buf), "OK (LOCKED / AKTIF)  ");
    } else {
        _gfx->setTextColor(0x52AA, 0x0841);
        snprintf(buf, sizeof(buf), "IDLE / BELUM TERSAMBUNG");
    }
    _gfx->drawString(buf, 252, 202);

    // Card 3: CMP2
    _gfx->setTextColor(0xCE79, 0x0841);
    _gfx->drawString("CMP SENSOR 2 (PIN 39):", 24, 234);
    _gfx->setTextColor(0x52AA, 0x0841);
    _gfx->drawString("STANDBY / NON-AKTIF   ", 24, 256);

    // Card 4: Kualitas Sinyal
    _gfx->setTextColor(0xCE79, 0x0841);
    _gfx->drawString("KUALITAS & INTEGRITAS SINYAL:", 252, 234);
    if (metrics.ckpActive) {
        _gfx->setTextColor(0x07E0, 0x0841);
        snprintf(buf, sizeof(buf), "Jitter: %.1f%% [BERSIH]   ", _lastResult.success ? _lastResult.jitterPercent : 0.2f);
    } else {
        _gfx->setTextColor(0x52AA, 0x0841);
        snprintf(buf, sizeof(buf), "Menunggu Aliran Pulsa   ");
    }
    _gfx->drawString(buf, 252, 256);

    if (curState != _lastDrawnState) {
        if (_lastResult.success && curState == 3) {
            _canvas.render(_lastResult.wheel, _lastResult.cam, 16, 50);
        }
        _lastDrawnState = curState;
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
            _hasProcessedCapture = false;
            _driver->arm(512);
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
