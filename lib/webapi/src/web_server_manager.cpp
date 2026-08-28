#include "web_server_manager.h"
#include <LittleFS.h>
#include <WiFi.h>
#include "page_dashboard.h"

namespace EcuWebApi {

WebServerManager::WebServerManager() : _server(80), _ws("/ws") {}

void WebServerManager::setCommandCallback(WebCommandJsonCallback cb) {
    _cmdCallback = cb;
}

void WebServerManager::setCaptureDriver(const EcuHal::CaptureDriver* capDriver) {
    _capDriver = capDriver;
}

void WebServerManager::init() {
    Serial.println("[WEB] Initializing LittleFS and AP mode...");
    if (!LittleFS.begin(true)) {
        Serial.println("[WEB] LittleFS Mount Failed!");
    } else {
        Serial.println("[WEB] LittleFS mounted successfully.");
    }

    WiFi.softAP("ECUSniff_Lab", "automotive123");
    IPAddress IP = WiFi.softAPIP();
    Serial.printf("[WEB] AP IP: %s\n", IP.toString().c_str());

    _ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                       AwsEventType type, void *arg, uint8_t *data, size_t len) {
        this->_onWsEvent(server, client, type, arg, data, len);
    });
    _server.addHandler(&_ws);

    _server.on("/api/presets", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/js/wheel_db.js", "application/javascript");
        request->send(response);
    });

    _server.on("/api/export_csv", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!_capDriver || !_capDriver->isDone()) {
            request->send(400, "text/plain", "Tidak ada rekaman capture");
            return;
        }
        String csv = "Index,Timestamp_us,Delta_us,Channel,Signal,Level\n";
        const EcuHal::CaptureEvent* buf = _capDriver->getBuffer();
        uint16_t count = _capDriver->getEventCount();
        uint32_t prevT = count > 0 ? buf[0].timestampUs : 0;
        for (uint16_t i = 0; i < count; ++i) {
            uint32_t dt = buf[i].timestampUs - prevT;
            prevT = buf[i].timestampUs;
            csv += String(i) + "," + String(buf[i].timestampUs) + "," + String(dt) + "," +
                   String(buf[i].channel) + "," + (buf[i].channel == 0 ? "CKP" : "CMP") + "," +
                   String(buf[i].level) + "\n";
        }
        AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csv);
        response->addHeader("Content-Disposition", "attachment; filename=\"ecusniff_capture.csv\"");
        request->send(response);
    });

    _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    _server.begin();
    Serial.println("[WEB] Server started on http://192.168.4.1");
}

void WebServerManager::_onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        _handleWsMessage(client, data, len);
    }
}

void WebServerManager::_handleWsMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, data, len);
    if (!err && _cmdCallback) {
        _cmdCallback(doc);
    }
}

void WebServerManager::updateLiveTelemetry(const EcuEngine::EngineRuntimeState& state,
                                         const EcuEngine::ParametricWheel& wheel,
                                         const EcuEngine::CamEventTable& cam) {
    if (_ws.count() == 0) return;
    JsonDocument doc;
    doc["type"] = "telemetry";
    doc["rpm"] = state.isRunning ? state.currentRpm : state.targetRpm;
    doc["targetRpm"] = state.targetRpm;
    doc["running"] = state.isRunning;
    doc["mode"] = static_cast<uint8_t>(state.runMode);
    doc["uiLevel"] = state.uiLevel;
    doc["tab"] = state.activeTab;

    JsonObject ckp = doc["ckp"].to<JsonObject>();
    ckp["totalTeeth"] = wheel.totalTeeth;
    ckp["missingTeeth"] = wheel.missingTeeth;
    ckp["missingPosition"] = wheel.missingPosition;
    ckp["dutyCycle"] = wheel.dutyCycle;
    ckp["inverted"] = wheel.inverted;

    JsonArray cmpArr = doc["cmp"].to<JsonArray>();
    uint8_t evCount = cam.getEventCount();
    const auto* evs = cam.getEvents();
    if (evs) {
        for (uint8_t i = 0; i < evCount && i < 8; ++i) {
            JsonObject evObj = cmpArr.add<JsonObject>();
            evObj["angle"] = evs[i].angleDeg;
            evObj["high"] = evs[i].levelHigh;
        }
    }

    doc["wheelName"] = state.activeWheelName;
    doc["capState"] = state.captureState;
    doc["capRpm"] = state.captureRpm;
    doc["capVehicle"] = state.matchedVehicle;

    if (state.capTotalTeeth > 0) {
        JsonObject capCkp = doc["capCkp"].to<JsonObject>();
        capCkp["totalTeeth"] = state.capTotalTeeth;
        capCkp["missingTeeth"] = state.capMissingTeeth;
        capCkp["dutyCycle"] = state.capDutyCycle;

        JsonArray capCmpArr = doc["capCmp"].to<JsonArray>();
        for (uint8_t i = 0; i < state.capCamCount && i < 8; ++i) {
            JsonObject evObj = capCmpArr.add<JsonObject>();
            evObj["angle"] = state.capCamAngles[i];
            evObj["high"] = state.capCamHighs[i];
        }
    }

    JsonObject hl = doc["health"].to<JsonObject>();
    hl["quality"] = static_cast<uint8_t>(state.health.quality);
    hl["ckpOk"] = state.health.ckpOk;
    hl["cmp1Ok"] = state.health.cmp1Ok;
    hl["cmp2Ok"] = state.health.cmp2Ok;
    hl["liveRpm"] = state.health.liveRpm;
    hl["liveTeeth"] = state.health.liveTeeth;
    hl["jitter"] = state.health.jitterPercent;
    hl["msg"] = state.health.diagnosticMsg;

    JsonArray custArr = doc["customSlots"].to<JsonArray>();
    uint8_t cCnt = EcuUi::PageDashboard::getCustomCount();
    for (uint8_t s = 0; s < cCnt && s < EcuUi::PageDashboard::MAX_CUSTOM_PRESETS; ++s) {
        const auto* p = EcuUi::PageDashboard::getCustomPreset(s);
        if (!p) continue;
        JsonObject sObj = custArr.add<JsonObject>();
        sObj["slot"] = s;
        sObj["name"] = p->name;
        sObj["teeth"] = p->totalTeeth;
        sObj["mteeth"] = p->missingTeeth;
        sObj["duty"] = p->dutyCycle;
    }

    if (_epsController) {
        JsonObject eps = doc["eps"].to<JsonObject>();
        const auto& epsCfg = _epsController->getConfig();
        const auto& epsSt = _epsController->getState();
        eps["running"] = epsSt.isRunning;
        eps["preset"] = static_cast<uint8_t>(epsCfg.preset);
        eps["presetName"] = _epsController->getPresetName(epsCfg.preset);
        eps["speed"] = epsSt.currentSpeedKmh;
        eps["targetSpeed"] = epsCfg.speedKmh;
        eps["rpm"] = epsSt.currentRpm;
        eps["targetRpm"] = epsCfg.targetRpm;
        eps["vssFreq"] = epsSt.vssFreqHz;
        eps["rpmFreq"] = epsSt.rpmFreqHz;
        eps["steer"] = epsCfg.steerTorque;
        eps["trq1"] = epsSt.trq1Voltage;
        eps["trq2"] = epsSt.trq2Voltage;
        eps["sweep"] = epsCfg.autoSweep;
    }

    String out;
    serializeJson(doc, out);
    _ws.textAll(out);
}

} // namespace EcuWebApi
