#include <Arduino.h>
#include "pin_config.h"
#include "engine_types.h"
#include "timing_math.h"
#include "parametric_pattern.h"
#include "rpm_controller.h"
#include "display_driver.h"
#include "encoder_driver.h"
#include "rmt_generator.h"
#include "capture_driver.h"
#include "signal_sniffer.h"
#include "web_server_manager.h"
#include "menu_manager.h"

// Hardware & Controller instances
static EcuHal::DisplayDriver        display;
static EcuHal::EncoderDriver        encoder;
static EcuHal::RmtGenerator         signalGen;
static EcuHal::CaptureDriver        captureDriver;
static EcuEngine::SignalSniffer     sniffer;
static EcuWebApi::WebServerManager  webManager;
static EcuEngine::RpmController     rpmController;
static EcuUi::MenuManager*          menuMgr = nullptr;

// Engine State & Parametric Configuration
static EcuEngine::EngineRuntimeState engineState;
static EcuEngine::ParametricWheel    wheelCfg;
static EcuEngine::CamEventTable      camCfg;

// Task Core 0: Web, Multi-Screen UI Rendering, and Encoder Polling
void taskCore0UiWeb(void *pvParameters) {
    uint32_t lastWebUpdate = 0;
    uint32_t lastRpmUpdate = 0;
    uint32_t lastMenuRender = 0;
    
    uint32_t btnPressTime = 0;
    uint32_t lastReleaseTime = 0;
    uint8_t  clickCount = 0;
    bool     btnWasDown = false;
    bool     longPressHandled = false;
    bool     superLongPressHandled = false;

    for (;;) {
        uint32_t now = millis();

        // 1. Polling Rotary Encoder Turn
        encoder.read();
        int32_t delta = encoder.getDelta();
        if (delta != 0 && menuMgr) {
            menuMgr->onEncoderTurn(delta, engineState, wheelCfg, camCfg);
            signalGen.setPattern(wheelCfg, camCfg);
            signalGen.setRpm(engineState.targetRpm);
        }

        // 2. Button State Machine
        bool btnIsDown = (digitalRead(PinConfig::ENC_SW) == LOW);
        if (btnIsDown && !btnWasDown) {
            btnPressTime = now;
            btnWasDown = true;
            longPressHandled = false;
        } else if (btnIsDown && btnWasDown) {
            uint32_t holdTime = now - btnPressTime;
            if (!longPressHandled && holdTime >= 600) {
                // Long Press (>= 600ms): Reliable START / STOP Toggle (Never exit to menu)
                longPressHandled = true;
                clickCount = 0;
                engineState.isRunning = !engineState.isRunning;
                if (engineState.isRunning) {
                    if (engineState.runMode == EcuEngine::EngineRunMode::Cranking) {
                        rpmController.startCranking(engineState.cranking);
                    }
                    signalGen.setPattern(wheelCfg, camCfg);
                    signalGen.prepareNextCycle();
                    signalGen.swapBuffer();
                    signalGen.start();
                } else {
                    signalGen.stop();
                }
            }
        } else if (!btnIsDown && btnWasDown) {
            if (!longPressHandled) {
                clickCount++;
                lastReleaseTime = now;
            }
            btnWasDown = false;
        }

        // Single vs Double Click
        if (clickCount > 0 && !btnIsDown && (now - lastReleaseTime >= 300)) {
            if (clickCount == 1 && menuMgr) {
                menuMgr->onEncoderClick();
            } else if (clickCount >= 2 && menuMgr) {
                menuMgr->onEncoderDoubleClick(wheelCfg, camCfg);
            }
            clickCount = 0;
        }

        // 3. Update RPM Controller (Fixed, Cranking, Sweep)
        captureDriver.update();
        if (now - lastRpmUpdate >= 20) {
            uint32_t dt = now - lastRpmUpdate;
            lastRpmUpdate = now;
            if (engineState.isRunning) {
                uint32_t activeRpm = rpmController.update(engineState, dt);
                signalGen.setRpm(activeRpm);
                signalGen.prepareNextCycle();
                signalGen.swapBuffer();
            }
        }

        // 4. Live Telemetry Web Push (~10 Hz)
        if (now - lastWebUpdate >= 100) {
            lastWebUpdate = now;
            engineState.ckpActive = engineState.isRunning;
            engineState.cmp1Active = engineState.isRunning;
            if (menuMgr) {
                engineState.uiLevel = menuMgr->getUiLevel();
                engineState.activeTab = menuMgr->getActiveTab();
                engineState.captureState = static_cast<uint8_t>(captureDriver.getState());
                const auto& capRes = menuMgr->getPageCapture().getLastResult();
                if (capRes.success) {
                    engineState.captureRpm = capRes.detectedRpm;
                    strncpy(engineState.matchedVehicle, capRes.matchedVehicle, sizeof(engineState.matchedVehicle));
                }
            }
            webManager.updateLiveTelemetry(engineState, wheelCfg, camCfg);
        }

        // 5. Multi-Screen Menu Rendering (~20 Hz)
        if (now - lastMenuRender >= 50 && menuMgr) {
            lastMenuRender = now;
            menuMgr->render(engineState, wheelCfg, camCfg);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    Serial.begin(115200);
    delay(800);
    Serial.println("\n==========================================");
    Serial.println(" AUTOMOTIVE ECU TEST PLATFORM (Master Sync)");
    Serial.println("==========================================");

    webManager.setCommandCallback([](const JsonDocument& doc) {
        String cmd = doc["cmd"] | "";
        uint32_t val = doc["val"] | 0;

        if (cmd == "start") {
            engineState.isRunning = true;
            if (engineState.runMode == EcuEngine::EngineRunMode::Cranking) {
                rpmController.startCranking(engineState.cranking);
            }
            signalGen.setPattern(wheelCfg, camCfg);
            signalGen.prepareNextCycle();
            signalGen.swapBuffer();
            signalGen.start();
        } else if (cmd == "stop") {
            engineState.isRunning = false;
            signalGen.stop();
        } else if (cmd == "set_rpm") {
            if (val >= 100 && val <= 12000) {
                engineState.targetRpm = val;
                signalGen.setRpm(val);
            }
        } else if (cmd == "set_mode") {
            if (val <= 2) {
                engineState.runMode = static_cast<EcuEngine::EngineRunMode>(val);
            }
        } else if (cmd == "set_ui_level") {
            if (menuMgr) menuMgr->setUiLevel(static_cast<EcuUi::UiLevel>(val));
        } else if (cmd == "set_tab") {
            if (menuMgr) menuMgr->setGenTab(val);
        } else if (cmd == "set_pattern") {
            if (doc["ckp"].is<JsonObjectConst>()) {
                JsonObjectConst ckp = doc["ckp"];
                wheelCfg.totalTeeth = ckp["totalTeeth"] | 36;
                wheelCfg.missingTeeth = ckp["missingTeeth"] | 1;
                wheelCfg.missingPosition = ckp["missingPosition"] | 0;
                wheelCfg.dutyCycle = ckp["dutyCycle"] | 0.5f;
                wheelCfg.inverted = ckp["inverted"] | false;
            }
            if (doc["cmp"].is<JsonArrayConst>()) {
                camCfg.clear();
                JsonArrayConst cmpArr = doc["cmp"];
                for (JsonObjectConst ev : cmpArr) {
                    float angle = ev["angle"] | 0.0f;
                    bool high = ev["high"] | false;
                    camCfg.addEvent(angle, high);
                }
            }
            signalGen.setPattern(wheelCfg, camCfg);
            if (menuMgr) menuMgr->markNeedsRedraw();
        } else if (cmd == "arm_capture") {
            captureDriver.arm(256);
        }
    });
    webManager.setCaptureDriver(&captureDriver);
    webManager.init();

    wheelCfg.totalTeeth = 36;
    wheelCfg.missingTeeth = 1;
    wheelCfg.missingPosition = 0;
    wheelCfg.dutyCycle = 0.5f;

    camCfg.clear();
    camCfg.addEvent(120.0f, true);
    camCfg.addEvent(180.0f, false);
    camCfg.addEvent(420.0f, true);
    camCfg.addEvent(470.0f, false);

    engineState.targetRpm = 850;
    engineState.isRunning = false;
    engineState.runMode = EcuEngine::EngineRunMode::FixedRpm;

    encoder.init();
    captureDriver.init();
    signalGen.init();
    signalGen.setPattern(wheelCfg, camCfg);
    signalGen.setRpm(850);
    signalGen.stop();

    display.init();
    menuMgr = new EcuUi::MenuManager(&display.getGfx());
    menuMgr->init(&captureDriver, &sniffer);

    xTaskCreatePinnedToCore(
        taskCore0UiWeb,
        "UiWebTask",
        8192,
        NULL,
        1,
        NULL,
        0
    );

    Serial.println("[INIT] Full-Duplex Master-Master Sync Ready at http://192.168.4.1");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
