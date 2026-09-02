## 2026-09-01T10:09:32Z
Task (Milestone 3: UI Waveform Canvas & Browser Sync):
1. Upgrade `lib/ui/include/waveform_canvas.h` and `lib/ui/src/waveform_canvas.cpp`:
   - Support dynamic canvas heights (e.g. 124px in `PageWheelBrowser`, 76px in `PageDashboard`) by dynamically partitioning track heights for CKP, CMP1, and CMP2.
   - Implement multi-channel rendering from `const WheelDefinition*` bit-arrays: CKP (Yellow), CMP1 (Green), and CMP2 (Cyan).
   - Map 0..720 degrees horizontally across canvas width, duplicating 360-degree wheels 2x so full engine cycle is always displayed.
   - Maintain clean step waveforms with decimation/downsampling for dense patterns (e.g. 1080 edges).
   - Retain backward-compatible render methods for `ParametricWheel` if called.
2. Upgrade `lib/ui/include/page_wheel_browser.h` and `lib/ui/src/page_wheel_browser.cpp`:
   - Consume the 70 ArduStim presets from `WheelDatabase` (`lib/engine/include/wheel_database.h`).
   - Use `BrandCategory` enum for category tab/filter navigation instead of brittle `strstr`.
   - Display friendly names, short names, cycle degrees (360°/720°), and channel badges (CKP, CMP1, CMP2).
   - Update canvas render call: `_canvas.render(wheelDef, 12, 184)`.
3. Update `lib/ui/include/wheel_database.h` to alias or include `lib/engine/include/wheel_database.h`.
4. Compile and verify with `pio run -e esp32s3`.

Write ownership:
You own `lib/ui/src/waveform_canvas.cpp`, `lib/ui/include/waveform_canvas.h`, `lib/ui/src/page_wheel_browser.cpp`, `lib/ui/include/page_wheel_browser.h`, `lib/ui/include/wheel_database.h`. Do not edit HAL RMT files.
