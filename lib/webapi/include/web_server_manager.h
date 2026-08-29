#pragma once
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "engine_types.h"
#include "parametric_pattern.h"
#include "capture_driver.h"
#include "eps_controller.h"
#include "speedo_controller.h"

namespace EcuWebApi {

typedef std::function<void(const JsonDocument& doc)> WebCommandJsonCallback;

class WebServerManager {
public:
    WebServerManager();
    void init();
    void setCommandCallback(WebCommandJsonCallback cb);
    void setCaptureDriver(const EcuHal::CaptureDriver* capDriver);
    void setEpsController(EcuEngine::EpsController* epsController) { _epsController = epsController; }
    void setSpeedoController(EcuEngine::SpeedoController* speedoController) { _speedoController = speedoController; }
    void updateLiveTelemetry(const EcuEngine::EngineRuntimeState& state,
                            const EcuEngine::ParametricWheel& wheel,
                            const EcuEngine::CamEventTable& cam);

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    WebCommandJsonCallback _cmdCallback;
    const EcuHal::CaptureDriver* _capDriver{nullptr};
    EcuEngine::EpsController*    _epsController{nullptr};
    EcuEngine::SpeedoController* _speedoController{nullptr};

    void _onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                    AwsEventType type, void *arg, uint8_t *data, size_t len);
    void _handleWsMessage(AsyncWebSocketClient *client, uint8_t *data, size_t len);
};

} // namespace EcuWebApi
