#include "app_settings.h"
#include "page_dashboard.h"

namespace EcuApp {

static Preferences pref;

void saveSettings(const EcuEngine::EngineRuntimeState& engineState,
                  const EcuEngine::ParametricWheel& wheelCfg,
                  const EcuEngine::CamEventTable& camCfg) {
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
        char k1[8], k2[8];
        snprintf(k1, sizeof(k1), "ca%u", i);
        snprintf(k2, sizeof(k2), "ch%u", i);
        pref.putFloat(k1, evs[i].angleDeg);
        pref.putBool(k2, evs[i].levelHigh);
    }

    uint8_t cCnt = EcuUi::PageDashboard::getCustomCount();
    pref.putUChar("c_cnt", cCnt);
    for (uint8_t s = 0; s < cCnt && s < EcuUi::PageDashboard::MAX_CUSTOM_PRESETS; ++s) {
        const auto* p = EcuUi::PageDashboard::getCustomPreset(s);
        if (!p) continue;
        char k[16];
        snprintf(k, sizeof(k), "s%u_nm", s); pref.putString(k, p->name);
        snprintf(k, sizeof(k), "s%u_t", s);  pref.putUShort(k, p->totalTeeth);
        snprintf(k, sizeof(k), "s%u_m", s);  pref.putUChar(k, p->missingTeeth);
        snprintf(k, sizeof(k), "s%u_d", s);  pref.putFloat(k, p->dutyCycle);
        snprintf(k, sizeof(k), "s%u_c", s);  pref.putUChar(k, p->camCount);
        for (uint8_t i = 0; i < p->camCount && i < 4; ++i) {
            char ka[16], kh[16];
            snprintf(ka, sizeof(ka), "s%u_ca%u", s, i);
            snprintf(kh, sizeof(kh), "s%u_ch%u", s, i);
            pref.putFloat(ka, p->camAngles[i]);
            pref.putBool(kh, p->camHighs[i]);
        }
    }
    pref.end();
}

void loadSettings(EcuEngine::EngineRuntimeState& engineState,
                  EcuEngine::ParametricWheel& wheelCfg,
                  EcuEngine::CamEventTable& camCfg) {
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
            char k1[8], k2[8];
            snprintf(k1, sizeof(k1), "ca%u", i);
            snprintf(k2, sizeof(k2), "ch%u", i);
            camCfg.addEvent(pref.getFloat(k1, 120.0f), pref.getBool(k2, true));
        }
    } else {
        camCfg.addEvent(120.0f, true);  camCfg.addEvent(180.0f, false);
        camCfg.addEvent(420.0f, true);  camCfg.addEvent(470.0f, false);
    }

    uint8_t cCnt = pref.getUChar("c_cnt", 0);
    EcuUi::PageDashboard::clearAllCustom();
    for (uint8_t s = 0; s < cCnt && s < EcuUi::PageDashboard::MAX_CUSTOM_PRESETS; ++s) {
        char k[16];
        snprintf(k, sizeof(k), "s%u_nm", s);
        if (!pref.isKey(k)) continue;
        EcuUi::WheelPresetItem item{};
        String nm = pref.getString(k, "Capture");
        strncpy(item.name, nm.c_str(), sizeof(item.name) - 1);
        snprintf(k, sizeof(k), "s%u_t", s); item.totalTeeth = pref.getUShort(k, 36);
        snprintf(k, sizeof(k), "s%u_m", s); item.missingTeeth = pref.getUChar(k, 1);
        snprintf(k, sizeof(k), "s%u_d", s); item.dutyCycle = pref.getFloat(k, 0.5f);
        snprintf(k, sizeof(k), "s%u_c", s); item.camCount = pref.getUChar(k, 0);
        for (uint8_t i = 0; i < item.camCount && i < 4; ++i) {
            char ka[16], kh[16];
            snprintf(ka, sizeof(ka), "s%u_ca%u", s, i);
            snprintf(kh, sizeof(kh), "s%u_ch%u", s, i);
            item.camAngles[i] = pref.getFloat(ka, 0.0f);
            item.camHighs[i] = pref.getBool(kh, true);
        }
        EcuUi::PageDashboard::setCustomSlot(s, item);
    }
    pref.end();
}

} // namespace EcuApp
