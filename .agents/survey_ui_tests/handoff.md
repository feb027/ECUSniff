# Handoff Report: ECUSniff UI Waveform Canvas & Test Infrastructure Survey

**Agent:** Explorer Subagent (`survey_ui_tests`)  
**Working Directory:** `g:\semester 7\ECUSniff\.agents\survey_ui_tests`  
**Date:** 2026-09-01  
**Handoff Type:** Hard (Survey Task Complete)  

---

## 1. Observation

1. **Waveform Canvas Layout and Logic (`lib/ui/src/waveform_canvas.cpp`):**
   - Lines 49-62: `_drawGrid()` hardcodes grid lines and labels assuming an 80px high canvas:
     ```cpp
     _sprite.drawFastHLine(0, 40, _width, 0x2965);
     _sprite.drawString("CKP", 4, 4);
     _sprite.drawString("CMP", 4, 44);
     ```
   - Lines 65-66: `_drawCkpTrace()` hardcodes CKP Y levels: `int32_t yHigh = 12; int32_t yLow = 34;`.
   - Lines 107-108: `_drawCmpTrace()` hardcodes CMP Y levels: `int32_t yHigh = 50; int32_t yLow = 72;`.
   - In `lib/ui/src/page_wheel_browser.cpp` lines 14-16 & 245:
     ```cpp
     _canvas.init(456, 124);
     ...
     _canvas.render(wheel, cam, 12, 184);
     ```
     Because the Y levels are hardcoded to $\le 72\text{ px}$, on the 124px canvas in `PageWheelBrowser`, pixels 73 to 124 are completely unpainted/blank.
   - Canvas only supports 2 traces (CKP and CMP1); CMP2 is completely unhandled.

2. **Wheel Database & Preset Discrepancy (`lib/ui/include/wheel_database.h` vs ArduStim `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h`):**
   - `wheel_database.h` had 70 presets modeled with `WheelPresetItem`:
     `{ char name[48]; uint16_t totalTeeth; uint8_t missingTeeth; uint8_t missingPosition; float dutyCycle; bool inverted; uint8_t camCount; float camAngles[4]; bool camHighs[4]; }`
   - Real ArduStim presets in `wheel_defs.h` contain 71 presets (`MAX_WHEELS = 71`) defined as raw bit-arrays (`0 = Low, 1 = CKP, 2 = CMP1, 4 = CMP2`), e.g.:
     - `old_avanza` (144 bytes, 720°): 3-gap sync group + cam pulses.
     - `new_avanza` (144 bytes, 720°): 3-gap sync group with single cam pulse.
     - `mitsubishi_4g63_4_2` (144 bytes, 720°): 4/2 CAS pattern.
     - In `wheel_database.h`, these were artificially approximated as `31-2` teeth with 4 cam angles, which does not produce the real automotive tooth train.

3. **Brand Category Filtering (`lib/ui/src/page_wheel_browser.cpp` lines 38-85):**
   - `matchesCategory` performs runtime `strstr` checks.
   - Presets like `Toyota Avanza/Xenia/Terios/Rush ` have trailing whitespace in ArduStim.
   - Presets like `36-2-2-2 H4` match `"36-2"` and fall into `Universal` instead of `MitsuNissanMazda (Subaru/Mazda)`.

4. **Test Infrastructure & PlatformIO Toolchain (`platformio.ini` & `test/`):**
   - Executing `pio test -e native` failed with:
     ```
     'gcc' is not recognized as an internal or external command,
     operable program or batch file.
     *** [.pio\build\native\unity_config_build\unity_config.o] Error 1
     ```
   - Executing `pio run -e esp32s3` succeeded with 0 errors in 9.14s:
     ```
     RAM:   [==        ]  22.1% (used 72332 bytes from 327680 bytes)
     Flash: [===       ]  28.2% (used 1033821 bytes from 3670016 bytes)
     ========================= [SUCCESS] Took 9.14 seconds =========================
     ```

---

## 2. Logic Chain

1. From Observation 1, because `WaveformCanvas` hardcodes $Y=12..34$ and $Y=50..72$, any canvas with height different from 80px (e.g. $456 \times 124$ in `PageWheelBrowser` or $448 \times 76$ in `PageDashboard`) suffers vertical distortion and unused visual area.
2. From Observation 1 and 2, because `WaveformCanvas` and `RmtGenerator` only consume `ParametricWheel` (uniform pitch + 1 gap) and `CamEventTable` (max 16 angle transitions), neither can accurately synthesize or render the 71 ArduStim OEM bit-arrays.
3. From Observation 2, porting ArduStim TFTv2 requires introducing an `ArbitraryPattern` or direct `const uint8_t*` bit-array interface across the database, RMT generator, and waveform canvas.
4. From Observation 3, explicit category enum metadata in each preset item eliminates brittle `strstr` matching and ensures 100% accurate brand grouping.
5. From Observation 4, because host GCC is absent on the user's Windows environment, automated tests should be delivered with target-executable test runner capabilities on `esp32s3` and modular standard C++ test fixtures that can also run on any host with GCC/Clang/MSVC.

---

## 3. Caveats

- **No Caveats:** All relevant source files (`lib/ui/`, `lib/engine/`, `lib/hal/`, `test/`, `platformio.ini`, `external/ardustim-tftv2-touchscreen/`, `external/pattern-gen/`) were thoroughly inspected and cross-referenced.

---

## 4. Conclusion

1. **Waveform Canvas Upgrade:** `WaveformCanvas` must be refactored with dynamic vertical track partitioning ($H=76\text{ px}$ vs $H=124\text{ px}$), multi-channel rendering (CKP Yellow, CMP1 Green, CMP2 Cyan), horizontal 0–720° mapping with $2\times$ duplication for 360° patterns, and high-density decimation.
2. **Database Synchronization:** The full 71-preset catalog from ArduStim TFTv2 (`external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h`) must be integrated as PROGMEM bit-arrays with exact friendly names and explicit category tags.
3. **RMT Engine & Test Harness:** `RmtGenerator` and unit tests must support converting arbitrary bit-arrays into microsecond pulse trains with duration slicing ($\le 30,000\ \mu\text{s}$) and zero-terminator EOT items, verified by comparative automated unit tests.

Detailed survey report written to `g:\semester 7\ECUSniff\.agents\survey_ui_tests\ui_test_survey.md`.

---

## 5. Verification Method

To independently verify the observations in this report:

1. **Inspect Waveform Canvas Hardcoded Offsets:**
   - File: `g:\semester 7\ECUSniff\lib\ui\src\waveform_canvas.cpp` lines 49, 65, 107.
2. **Inspect Wheel Database Discrepancy:**
   - File: `g:\semester 7\ECUSniff\lib\ui\include\wheel_database.h` vs `g:\semester 7\ECUSniff\external\ardustim-tftv2-touchscreen\ardustim\wheel_defs.h`.
3. **Verify Clean Firmware Build on ESP32-S3:**
   - Run command: `pio run -e esp32s3`
   - Expected result: `[SUCCESS]` in ~9 seconds with ~28% Flash usage.
4. **Verify Native Test Toolchain State:**
   - Run command: `pio test -e native`
   - Expected result: Errored due to missing host `gcc`/`g++`.
