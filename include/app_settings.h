#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "engine_types.h"
#include "parametric_pattern.h"

namespace EcuApp {

void saveSettings(const EcuEngine::EngineRuntimeState& engineState,
                  const EcuEngine::ParametricWheel& wheelCfg,
                  const EcuEngine::CamEventTable& camCfg);

void loadSettings(EcuEngine::EngineRuntimeState& engineState,
                  EcuEngine::ParametricWheel& wheelCfg,
                  EcuEngine::CamEventTable& camCfg);

} // namespace EcuApp
