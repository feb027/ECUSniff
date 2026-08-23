#include <Arduino.h>
#include <Preferences.h>
#include "pin_config.h"
#include "engine_types.h"
#include "timing_math.h"
#include "parametric_pattern.h"
#include "rpm_controller.h"
#include "display_driver.h"
#include "encoder_driver.h"
#include "joystick_driver.h"
#include "rmt_generator.h"
#include "capture_driver.h"
#include "signal_sniffer.h"
#include "web_server_manager.h"
#include "menu_manager.h"

static EcuHal::DisplayDriver        display;
static EcuHal::EncoderDriver        encoder;
static EcuHal::JoystickDriver       joystick;
static EcuHal::RmtGenerator         signalGen;
static EcuHal::CaptureDriver        captureDriver;
static EcuEngine::SignalSniffer     sniffer;
static EcuWebApi::WebServerManager  webManager;
static EcuEngine::RpmController     rpmController;
static EcuUi::MenuManager*          menuMgr = nullptr;

static EcuEngine::EngineRuntimeState engineState;
static EcuEngine::ParametricWheel    wheelCfg;
static EcuEngine::CamEventTable      camCfg;
static Preferences                   pref;

static void saveSettings() {
    pref.begin("ecu_conf", false);
    pref.putUInt("rpm", engineState.targetRpm);
    pref.putString("wname", engineState.activeWheelName);
    pref.putUShort("teeth", wheelCfg.totalTeeth);
    pref.putUChar("mteeth", wheelCfg.missingTeeth);
    pref.putUChar("mpos", wheelCfg.missingPosition);
    pref.putFloat("duty", wheelCfg.dutyCycle);
    pref.putBool("inv", wheelCfg.inverted);
    uint8_t c = camCfg.getEventCount();
    pref.putUChar("ccnt", c);
    const auto* evs = camCfg.getEvents();
    for (uint8_t i = 0; i < c && i < 8; ++i) {
        char k1[8], k2[8]; snprintf(k1, sizeof(k1), "ca%u", i); snprintf(k2, sizeof(k2), "ch%u", i);
        pref.putFloat(k1, evs[i].angleDeg); pref.putBool(k2, evs[i].levelHigh);
    }
    if (EcuUi::PageDashboard::hasCapturedPreset()) {
        pref.putUShort("cap_teeth", wheelCfg.totalTeeth);
        pref.putUChar("cap_mteeth", wheelCfg.missingTeeth);
        pref.putFloat("cap_duty", wheelCfg.dutyCycle);
        pref.putString("cap_name", engineState.activeWheelName);
        pref.putUChar("cap_ccnt", c);
        for (uint8_t i = 0; i < c && i < 4; ++i) {
            char k1[12], k2[12]; snprintf(k1, sizeof(k1), "cap_ca%u", i); snprintf(k2, sizeof(k2), "cap_ch%u", i);
            pref.putFloat(k1, evs[i].angleDeg); pref.putBool(k2, evs[i].levelHigh);
        }
    }
    pref.end();
}

static void loadSettings() {
    pref.begin("ecu_conf", true);
    engineState.targetRpm = pref.getUInt("rpm", 850);
    String wn = pref.getString("wname", "Honda / Ford 36-1");
    strncpy(engineState.activeWheelName, wn.c_str(), sizeof(engineState.activeWheelName));
    wheelCfg.totalTeeth = pref.getUShort("teeth", 36);
    wheelCfg.missingTeeth = pref.getUChar("mteeth", 1);
    wheelCfg.missingPosition = pref.getUChar("mpos", 0);
    wheelCfg.dutyCycle = pref.getFloat("duty", 0.5f);
    wheelCfg.inverted = pref.getBool("inv", false);
    uint8_t c = pref.getUChar("ccnt", 4);
    camCfg.clear();
    if (c > 0 && c <= 8 && pref.isKey("ca0")) {
        for (uint8_t i = 0; i < c; ++i) {
            char k1[8], k2[8]; snprintf(k1, sizeof(k1), "ca%u", i); snprintf(k2, sizeof(k2), "ch%u", i);
            camCfg.addEvent(pref.getFloat(k1, 120.0f), pref.getBool(k2, true));
        }
    } else {
        camCfg.addEvent(120.0f, true); camCfg.addEvent(180.0f, false);
        camCfg.addEvent(420.0f, true); camCfg.addEvent(470.0f, false);
    }
    if (pref.isKey("cap_teeth")) {
        EcuEngine::ParametricWheel capWheel;
        capWheel.totalTeeth = pref.getUShort("cap_teeth", 36);
        capWheel.missingTeeth = pref.getUChar("cap_mteeth", 1);
        capWheel.missingPosition = 0;
        capWheel.dutyCycle = pref.getFloat("cap_duty", 0.5f);
        capWheel.inverted = false;
        EcuEngine::CamEventTable capCam;
        uint8_t capC = pref.getUChar("cap_ccnt", 0);
        for (uint8_t i = 0; i < capC && i < 4; ++i) {
            char k1[12], k2[12]; snprintf(k1, sizeof(k1), "cap_ca%u", i); snprintf(k2, sizeof(k2), "cap_ch%u", i);
            capCam.addEvent(pref.getFloat(k1, 0.0f), pref.getBool(k2, true));
        }
        String capName = pref.getString("cap_name", "Captured: Pola Mobil");
        EcuUi::PageDashboard::setCapturedPreset(capName.c_str(), capWheel, capCam);
    }
    pref.end();
}

void taskCore0UiWeb(void *pvParameters) {
    uint32_t lastWeb = 0, lastRpm = 0, lastRender = 0, btnTime = 0, relTime = 0;
    uint8_t clickCount = 0; bool btnDown = false, longHandled = false;

    for (;;) {
        uint32_t now = millis();
        EcuHal::JoyAction joyAct = joystick.update();
        if (joyAct != EcuHal::JoyAction::None && menuMgr) {
            menuMgr->onJoystickAction(joyAct, engineState, wheelCfg, camCfg);
        }

        encoder.read();
        int32_t delta = encoder.getDelta();
        if (delta != 0 && menuMgr) {
            menuMgr->onEncoderTurn(delta, engineState, wheelCfg, camCfg);
            signalGen.setPattern(wheelCfg, camCfg);
            signalGen.setRpm(engineState.targetRpm);
            if (engineState.isRunning) { signalGen.prepareNextCycle(); signalGen.swapBuffer(); }
        }

        bool isDown = (digitalRead(PinConfig::ENC_SW) == LOW);
        if (isDown && !btnDown) { btnTime = now; btnDown = true; longHandled = false; }
        else if (isDown && btnDown) {
            if (!longHandled && (now - btnTime) >= 600) {
                longHandled = true; clickCount = 0;
                engineState.isRunning = !engineState.isRunning;
                if (engineState.isRunning) {
                    if (engineState.runMode == EcuEngine::EngineRunMode::Cranking) rpmController.startCranking(engineState.cranking);
                    signalGen.setPattern(wheelCfg, camCfg); signalGen.prepareNextCycle(); signalGen.swapBuffer(); signalGen.start();
                } else { signalGen.stop(); }
            }
        } else if (!isDown && btnDown) {
            if (!longHandled) { clickCount++; relTime = now; }
            btnDown = false;
        }

        if (clickCount > 0 && !isDown && (now - relTime >= 300)) {
            if (clickCount == 1 && menuMgr) menuMgr->onEncoderClick();
            else if (clickCount >= 2 && menuMgr) {
                menuMgr->onEncoderDoubleClick(wheelCfg, camCfg);
                const auto& cr = menuMgr->getPageCapture().getLastResult();
                if (cr.success) {
                    snprintf(engineState.activeWheelName, sizeof(engineState.activeWheelName), "Cap: %s", cr.matchedVehicle);
                } else {
                    snprintf(engineState.activeWheelName, sizeof(engineState.activeWheelName), "Captured: %u-%u", wheelCfg.totalTeeth, wheelCfg.missingTeeth);
                }
                EcuUi::PageDashboard::setCapturedPreset(engineState.activeWheelName, wheelCfg, camCfg);
                saveSettings();
            }
            clickCount = 0;
        }

        captureDriver.update();
        if (now - lastRpm >= 20) {
            uint32_t dt = now - lastRpm; lastRpm = now;
            if (engineState.isRunning) {
                signalGen.setRpm(rpmController.update(engineState, dt));
                signalGen.prepareNextCycle(); signalGen.swapBuffer();
            }
        }

        if (now - lastWeb >= 100) {
            lastWeb = now;
            EcuHal::LiveSignalMetrics m{}; captureDriver.getLiveMetrics(m);
            engineState.health = sniffer.evaluateHealth(m.ckpActive, m.cmpActive, m.cmp2Active, m.revPeriodUs, m.nominalPeriodUs, m.lastGapUs);
            engineState.ckpActive = engineState.isRunning; engineState.cmp1Active = engineState.isRunning;
            if (menuMgr) {
                engineState.uiLevel = menuMgr->getUiLevel(); engineState.activeTab = menuMgr->getActiveTab();
                engineState.captureState = static_cast<uint8_t>(captureDriver.getState());
                const auto& cr = menuMgr->getPageCapture().getLastResult();
                if (cr.success) {
                    engineState.captureRpm = cr.detectedRpm;
                    strncpy(engineState.matchedVehicle, cr.matchedVehicle, sizeof(engineState.matchedVehicle));
                    engineState.capTotalTeeth = cr.wheel.totalTeeth; engineState.capMissingTeeth = cr.wheel.missingTeeth;
                    engineState.capDutyCycle = cr.wheel.dutyCycle; engineState.capCamCount = cr.cam.getEventCount();
                    const auto* ev = cr.cam.getEvents();
                    for (uint8_t k = 0; ev && k < engineState.capCamCount && k < 8; ++k) {
                        engineState.capCamAngles[k] = ev[k].angleDeg; engineState.capCamHighs[k] = ev[k].levelHigh;
                    }
                }
            }
            webManager.updateLiveTelemetry(engineState, wheelCfg, camCfg);
        }

        if (now - lastRender >= 50 && menuMgr) {
            lastRender = now;
            menuMgr->render(engineState, wheelCfg, camCfg);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    Serial.begin(115200);
    delay(800);
    loadSettings();

    webManager.setCommandCallback([](const JsonDocument& doc) {
        String cmd = doc["cmd"] | ""; uint32_t val = doc["val"] | 0;
        if (cmd == "start") {
            engineState.isRunning = true;
            if (engineState.runMode == EcuEngine::EngineRunMode::Cranking) rpmController.startCranking(engineState.cranking);
            signalGen.setPattern(wheelCfg, camCfg); signalGen.prepareNextCycle(); signalGen.swapBuffer(); signalGen.start();
        } else if (cmd == "stop") {
            engineState.isRunning = false; signalGen.stop();
        } else if (cmd == "set_rpm" && val >= 100 && val <= 12000) {
            engineState.targetRpm = val; signalGen.setRpm(val); saveSettings();
        } else if (cmd == "set_pattern") {
            String name = doc["name"] | doc["wheelName"] | "";
            if (name.length() > 0) strncpy(engineState.activeWheelName, name.c_str(), sizeof(engineState.activeWheelName));
            if (doc["ckp"].is<JsonObjectConst>()) {
                JsonObjectConst ckp = doc["ckp"];
                wheelCfg.totalTeeth = ckp["totalTeeth"] | 36; wheelCfg.missingTeeth = ckp["missingTeeth"] | 1;
                wheelCfg.missingPosition = ckp["missingPosition"] | 0; wheelCfg.dutyCycle = ckp["dutyCycle"] | 0.5f;
                wheelCfg.inverted = ckp["inverted"] | false;
            }
            if (doc["cmp"].is<JsonArrayConst>()) {
                camCfg.clear();
                for (JsonObjectConst ev : doc["cmp"].as<JsonArrayConst>()) camCfg.addEvent(ev["angle"] | 0.0f, ev["high"] | false);
            }
            signalGen.setPattern(wheelCfg, camCfg);
            if (engineState.isRunning) { signalGen.prepareNextCycle(); signalGen.swapBuffer(); }
            if (menuMgr) menuMgr->markNeedsRedraw();
            saveSettings();
        } else if (cmd == "set_mode") {
            if (val <= 2) {
                engineState.runMode = static_cast<EcuEngine::EngineRunMode>(val);
                if (engineState.isRunning && engineState.runMode == EcuEngine::EngineRunMode::Cranking) {
                    rpmController.startCranking(engineState.cranking);
                }
                if (menuMgr) menuMgr->markNeedsRedraw();
            }
        } else if (cmd == "set_ui_level") {
            if (menuMgr) menuMgr->setUiLevel(static_cast<EcuUi::UiLevel>(val));
        } else if (cmd == "set_tab") {
            if (menuMgr) menuMgr->setGenTab(val);
        } else if (cmd == "arm_capture") {
            captureDriver.arm(512);
        }
    });
    webManager.setCaptureDriver(&captureDriver);
    webManager.init();

    encoder.init(); joystick.init(); captureDriver.init(); signalGen.init();
    signalGen.setPattern(wheelCfg, camCfg); signalGen.setRpm(engineState.targetRpm); signalGen.stop();
    display.init();
    menuMgr = new EcuUi::MenuManager(&display.getGfx());
    menuMgr->init(&captureDriver, &sniffer);

    xTaskCreatePinnedToCore(taskCore0UiWeb, "UiWebTask", 12288, NULL, 1, NULL, 0);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
