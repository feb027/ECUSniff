#include <Arduino.h>
#include "pin_config.h"
#include "app_settings.h"
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
#include "eps_controller.h"
#include "eps_driver.h"
#include "speedo_controller.h"
#include "speedo_driver.h"
#include "web_server_manager.h"
#include "menu_manager.h"

static EcuHal::DisplayDriver        display;
static EcuHal::EncoderDriver        encoder;
static EcuHal::JoystickDriver       joystick;
static EcuHal::RmtGenerator         signalGen;
static EcuHal::CaptureDriver        captureDriver;
static EcuEngine::SignalSniffer     sniffer;
static EcuEngine::EpsController     epsController;
static EcuHal::EpsDriver            epsDriver;
static EcuEngine::SpeedoController  speedoController;
static EcuHal::SpeedoDriver         speedoDriver;
static EcuWebApi::WebServerManager  webManager;
static EcuEngine::RpmController     rpmController;
static EcuUi::MenuManager*          menuMgr = nullptr;

static EcuEngine::EngineRuntimeState engineState;
static EcuEngine::ParametricWheel    wheelCfg;
static EcuEngine::CamEventTable      camCfg;

void taskCore0UiWeb(void *pvParameters) {
    uint32_t lastWeb = 0, lastRpm = 0, lastRender = 0, lastEps = 0, lastSpeedo = 0;
    uint32_t btnTime = 0, relTime = 0;
    uint8_t clickCount = 0; bool btnDown = false, longHandled = false;

    for (;;) {
        uint32_t now = millis();
        EcuHal::JoyAction joyAct = joystick.update();
        if (joyAct != EcuHal::JoyAction::None && menuMgr) {
            menuMgr->onJoystickAction(joyAct, engineState, wheelCfg, camCfg);
            signalGen.setPattern(wheelCfg, camCfg);
            if (engineState.isRunning) { signalGen.prepareNextCycle(); signalGen.swapBuffer(); }
        }

        encoder.read();
        int32_t delta = encoder.getDelta();
        if (delta != 0 && menuMgr) {
            menuMgr->onEncoderTurn(delta, engineState, wheelCfg, camCfg);
            signalGen.setPattern(wheelCfg, camCfg); signalGen.setRpm(engineState.targetRpm);
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

        if (clickCount > 0 && !isDown && (now - relTime >= 180)) {
            if (clickCount == 1 && menuMgr) {
                menuMgr->onEncoderClick(engineState, wheelCfg, camCfg);
                signalGen.setPattern(wheelCfg, camCfg);
                if (engineState.isRunning) { signalGen.prepareNextCycle(); signalGen.swapBuffer(); }
                EcuApp::saveSettings(engineState, wheelCfg, camCfg);
            } else if (clickCount >= 2 && menuMgr) {
                menuMgr->onEncoderDoubleClick(wheelCfg, camCfg);
                const auto& cr = menuMgr->getPageCapture().getLastResult();
                char slotName[32];
                uint8_t nextNum = EcuUi::PageDashboard::getCustomCount() + 1;
                if (cr.success && strstr(cr.matchedVehicle, "Belum Terdeteksi") == nullptr) {
                    snprintf(slotName, sizeof(slotName), "Cap %u: %s", (unsigned)nextNum, cr.matchedVehicle);
                } else {
                    snprintf(slotName, sizeof(slotName), "Capture %u (%u-%u)", (unsigned)nextNum, (unsigned)wheelCfg.totalTeeth, (unsigned)wheelCfg.missingTeeth);
                }
                uint8_t slot = EcuUi::PageDashboard::addCapturedPreset(slotName, wheelCfg, camCfg);
                const auto* saved = EcuUi::PageDashboard::getCustomPreset(slot);
                if (saved) strncpy(engineState.activeWheelName, saved->name, sizeof(engineState.activeWheelName));
                EcuApp::saveSettings(engineState, wheelCfg, camCfg);
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

        if (now - lastEps >= 20) {
            float dtSec = (now - lastEps) / 1000.0f; lastEps = now;
            epsController.update(dtSec);
            epsDriver.updateOutputs(epsController.getState());
        }

        if (now - lastSpeedo >= 20) {
            float dtSec = (now - lastSpeedo) / 1000.0f; lastSpeedo = now;
            speedoController.update(dtSec);
            speedoDriver.updateOutputs(speedoController.getConfig(), speedoController.getState());
        }

        if (now - lastWeb >= 100) {
            lastWeb = now;
            EcuHal::LiveSignalMetrics m{}; captureDriver.getLiveMetrics(m);
            engineState.health = sniffer.evaluateHealth(m.ckpActive, m.cmpActive, m.cmp2Active, m.revPeriodUs, m.nominalPeriodUs, m.lastGapUs);
            if (m.teethPerRev > 0) engineState.health.liveTeeth = m.teethPerRev;
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

static void handleWebCommand(const JsonDocument& doc) {
    String cmd = doc["cmd"] | ""; uint32_t val = doc["val"] | 0;
    if (cmd == "start") {
        engineState.isRunning = true;
        if (engineState.runMode == EcuEngine::EngineRunMode::Cranking) rpmController.startCranking(engineState.cranking);
        signalGen.setPattern(wheelCfg, camCfg); signalGen.prepareNextCycle(); signalGen.swapBuffer(); signalGen.start();
    } else if (cmd == "stop") {
        engineState.isRunning = false; signalGen.stop();
    } else if (cmd == "set_rpm" && val >= 100 && val <= 12000) {
        engineState.targetRpm = val; signalGen.setRpm(val);
        EcuApp::saveSettings(engineState, wheelCfg, camCfg);
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
        EcuApp::saveSettings(engineState, wheelCfg, camCfg);
    } else if (cmd == "rename_preset") {
        uint8_t slot = doc["slot"] | 0; String newName = doc["name"] | "";
        if (newName.length() > 0 && EcuUi::PageDashboard::renameCustomPreset(slot, newName.c_str())) {
            EcuApp::saveSettings(engineState, wheelCfg, camCfg); if (menuMgr) menuMgr->markNeedsRedraw();
        }
    } else if (cmd == "delete_preset") {
        uint8_t slot = doc["slot"] | 0;
        if (EcuUi::PageDashboard::deleteCustomPreset(slot)) {
            EcuApp::saveSettings(engineState, wheelCfg, camCfg); if (menuMgr) menuMgr->markNeedsRedraw();
        }
    } else if (cmd == "set_mode" && val <= 2) {
        engineState.runMode = static_cast<EcuEngine::EngineRunMode>(val);
        if (engineState.isRunning && engineState.runMode == EcuEngine::EngineRunMode::Cranking) rpmController.startCranking(engineState.cranking);
        if (menuMgr) menuMgr->markNeedsRedraw();
    } else if (cmd == "set_ui_level") {
        if (menuMgr) menuMgr->setUiLevel(static_cast<EcuUi::UiLevel>(val));
    } else if (cmd == "set_tab") {
        if (menuMgr) menuMgr->setGenTab(val);
    } else if (cmd == "arm_capture") {
        captureDriver.arm(512);
    } else if (cmd == "eps_toggle") {
        epsController.toggleRunning();
    } else if (cmd == "eps_set") {
        if (doc["speed"].is<float>()) epsController.setSpeed(doc["speed"].as<float>());
        if (doc["rpm"].is<uint32_t>()) epsController.setRpm(doc["rpm"].as<uint32_t>());
        if (doc["steer"].is<float>()) epsController.setSteerTorque(doc["steer"].as<float>());
    } else if (cmd == "eps_preset") {
        epsController.setPreset(static_cast<EcuEngine::EpsOemPreset>(val));
    } else if (cmd == "eps_sweep") {
        epsController.setAutoSweep(val != 0);
    } else if (cmd == "speedo_toggle") {
        speedoController.toggleRunning();
    } else if (cmd == "speedo_set") {
        if (doc["kmh"].is<int32_t>()) speedoController.setKmh(doc["kmh"].as<int32_t>());
        if (doc["rpm"].is<int32_t>()) speedoController.setRpm(doc["rpm"].as<int32_t>());
        if (doc["temp"].is<int32_t>()) speedoController.setTemp(doc["temp"].as<int32_t>());
        if (doc["fuel"].is<int32_t>()) speedoController.setFuel(doc["fuel"].as<int32_t>());
    } else if (cmd == "speedo_set_ch") {
        String ch = doc["ch"] | ""; bool en = doc["en"] | true;
        if (ch == "kmh") speedoController.setChannelEnable(0, en);
        else if (ch == "rpm") speedoController.setChannelEnable(1, en);
        else if (ch == "temp") speedoController.setChannelEnable(2, en);
        else if (ch == "fuel") speedoController.setChannelEnable(3, en);
    } else if (cmd == "speedo_set_ppk" && doc["val"].is<float>()) {
        speedoController.setPulsePerKm(doc["val"].as<float>());
    } else if (cmd == "speedo_set_ppr" && doc["val"].is<float>()) {
        speedoController.setTachoPpr(doc["val"].as<float>());
    } else if (cmd == "speedo_set_max_rpm" && doc["val"].is<int32_t>()) {
        speedoController.setMaxRpm(doc["val"].as<int32_t>());
    } else if (cmd == "speedo_set_temp_cal") {
        speedoController.setTempCal(doc["min"] | 0, doc["mid"] | 50, doc["max"] | 100);
    } else if (cmd == "speedo_set_fuel_cal") {
        speedoController.setFuelCal(doc["min"] | 0, doc["mid"] | 50, doc["max"] | 100);
    } else if (cmd == "speedo_set_curve") {
        speedoController.setGaugeCurve(static_cast<EcuEngine::SpeedoGaugeCurve>(val));
    } else if (cmd == "speedo_set_dac_routing") {
        speedoController.setDacRouting(static_cast<EcuEngine::SpeedoDacRouting>(val));
    } else if (cmd == "speedo_set_sweep") {
        speedoController.setAutoSweep(val != 0);
    } else if (cmd == "speedo_set_sweep_time" && doc["val"].is<float>()) {
        speedoController.setSweepTimeSec(doc["val"].as<float>());
    } else if (cmd == "speedo_set_mode" && val <= 3) {
        speedoController.setRunMode(static_cast<EcuEngine::SpeedoRunMode>(val));
    }
}

void setup() {
    Serial.begin(115200);
    delay(800);
    EcuApp::loadSettings(engineState, wheelCfg, camCfg);

    webManager.setCommandCallback(handleWebCommand);
    webManager.setCaptureDriver(&captureDriver);
    webManager.setEpsController(&epsController);
    webManager.setSpeedoController(&speedoController);
    webManager.init();

    encoder.init(); joystick.init(); captureDriver.init(); signalGen.init();
    epsDriver.init(); speedoDriver.init();
    bool dacFuel = false, dacTemp = false;
    speedoDriver.detectDacs(dacFuel, dacTemp);
    speedoController.setDacFound(dacFuel, dacTemp);

    signalGen.setPattern(wheelCfg, camCfg); signalGen.setRpm(engineState.targetRpm); signalGen.stop();
    display.init();
    menuMgr = new EcuUi::MenuManager(&display.getGfx());
    menuMgr->init(&captureDriver, &sniffer, &epsController, &speedoController);

    xTaskCreatePinnedToCore(taskCore0UiWeb, "UiWebTask", 12288, NULL, 1, NULL, 0);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
