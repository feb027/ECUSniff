#include "web_server_manager.h"
#include <LittleFS.h>
#include <WiFi.h>

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

    // CSV Diagnostic Log Export Endpoint
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

    doc["capState"] = state.captureState;
    doc["capRpm"] = state.captureRpm;
    doc["capVehicle"] = state.matchedVehicle;

    String out;
    serializeJson(doc, out);
    _ws.textAll(out);
}

} // namespace EcuWebApi
