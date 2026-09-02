# ECUSniff UI Waveform Canvas & Test Infrastructure Survey Report

**Document Version:** 1.0.0  
**Date:** 2026-09-01  
**Author:** Explorer Subagent (UI & Test Infrastructure Survey)  
**Target:** ECUSniff Automotive ECU Simulator & Sniffer Platform  

---

## 1. Executive Summary & Architectural Context

The objective of this investigation is to evaluate the existing UI Waveform Canvas, Preset Pattern Database, Category Filtering, and Unit Test Infrastructure of ECUSniff (`g:\semester 7\ECUSniff`). This survey establishes the requirements to port all 71 engine wheel presets from ArduStim TFTv2 (`external/ardustim-tftv2-touchscreen`) and Pattern-Gen (`external/pattern-gen`), upgrade the Waveform Canvas to accurately visualize arbitrary bit-arrays across complete $0 - 720^\circ$ engine cycles with CKP, CMP1, and CMP2 support, and establish programmatic verification for edge timing and RMT pulse generation.

### Key Architectural Findings:
1. **Parametric Limitation in Current UI & Engine:** Current ECUSniff models patterns strictly as single missing-tooth parametric wheels (`totalTeeth`, `missingTeeth`, `missingPosition`, `dutyCycle`) and up to 4–16 Cam angle events (`CamEventTable`). Complex real-world OEM patterns (such as Toyota Avanza 1.3/1.5 144-segment multi-gap, Mitsubishi 4G63 4/2, Subaru/Mazda 36-2-2-2, GM 7X, and Audi 135) were synthetic approximations that do not match real engine triggers.
2. **Waveform Canvas Fixed Geometry Flaw:** `WaveformCanvas` (`lib/ui/src/waveform_canvas.cpp`) uses hardcoded Y coordinates tailored only for 80px height (`yHigh=12, yLow=34` for CKP; `yHigh=50, yLow=72` for CMP). When rendered inside `PageWheelBrowser` on a $456 \times 124\text{ px}$ canvas, the lower 52 pixels are unused, and only 2 channels (CKP & CMP1) are rendered, omitting CMP2 entirely.
3. **Database & Friendly Name Inconsistencies:** `lib/ui/include/wheel_database.h` contained 70 presets with modified names (e.g. `"Toyota Avanza 1.3 K3-VE"` instead of `"Toyota Avanza 1.3 Crank only"`). ArduStim TFTv2 contains 71 presets (`MAX_WHEELS = 71`) defined as raw PROGMEM byte arrays (`0 = Low, 1 = CKP, 2 = CMP1, 4 = CMP2`).
4. **Test Infrastructure Host Toolchain Gap:** `pio test -e native` failed because host `gcc`/`g++` is not present in Windows system `PATH`. However, target compilation (`pio run -e esp32s3`) succeeds cleanly in 9.1s. Unit tests must be structured with both host-independent validation harnesses and firmware embedded self-test capabilities.

---

## 2. Investigation of Existing Waveform Canvas & UI Subsystem

### 2.1 Component Structure & Dimensions
The Waveform Canvas is instantiated and rendered in three key UI pages:

| Page Location | Source File | Canvas Dimensions | Position on Screen | Notes |
|---|---|---|---|---|
| **Wheel Browser** | `lib/ui/src/page_wheel_browser.cpp` | **$456 \times 124\text{ px}$** | $X=12, Y=184$ | Wide preview card below pattern list |
| **Main Dashboard** | `lib/ui/src/page_dashboard.cpp` | **$448 \times 76\text{ px}$** | $X=16, Y=48$ | Top oscilloscope card on live cockpit |
| **Signal Capture** | `lib/ui/src/page_capture.cpp` | **$448 \times 76\text{ px}$** | $X=16, Y=50$ | Oscilloscope view for captured sniffer signals |

### 2.2 Memory Allocation & Sprite Architecture
- `WaveformCanvas` uses `LovyanGFX` `LGFX_Sprite` with color depth set to 8-bit palette mode (`_sprite.setColorDepth(8)`).
- **RAM Footprint:**
  - $456 \times 124 \times 1\text{ byte} = 56,544\text{ bytes}$ (~55.2 KB).
  - $448 \times 76 \times 1\text{ byte} = 34,048\text{ bytes}$ (~33.25 KB).
- On ESP32-S3 (320 KB internal SRAM + 8 MB Octal PSRAM), this sprite buffer is lightweight and delivers tear-free, high-speed DMA rendering via `_sprite.pushSprite()`.

### 2.3 Waveform Rendering Logic & Deficiencies

```
Existing Y-Axis Layout (Fixed 80px assumption):
0px   +---------------------------------------------+
4px   | [CKP] Label                                 |
12px  | ---- High Level (yHigh = 12) -------------  |  CKP Track
34px  | ____ Low Level  (yLow  = 34) _____________  |  (Height: 22px)
40px  |----------------- Split Line ----------------|
44px  | [CMP] Label                                 |
50px  | ---- High Level (yHigh = 50) -------------  |  CMP Track
72px  | ____ Low Level  (yLow  = 72) _____________  |  (Height: 22px)
80px  +---------------------------------------------+
81px..124px  >>> UNUSED DEAD SPACE ON 124PX CANVAS <<<
```

#### Detailed Flaws in `waveform_canvas.cpp`:
1. **Hardcoded Offsets in `_drawGrid()`:**
   - Separator line fixed at $Y=40$ (`_sprite.drawFastHLine(0, 40, _width, 0x2965)`).
   - Label positions fixed at $Y=4$ (CKP) and $Y=44$ (CMP).
   - Degree labels ("0", "360", "720") drawn at $Y=4$.
2. **Hardcoded Offsets in `_drawCkpTrace()` and `_drawCmpTrace()`:**
   - CKP levels hardcoded: `yHigh = 12, yLow = 34`.
   - CMP levels hardcoded: `yHigh = 50, yLow = 72`.
   - On a 124px tall canvas, more than 40% of the canvas height remains unutilized.
3. **No CMP2 (Camshaft 2) Channel:**
   - Real dual-cam VVT-i / Dual CAS systems (e.g. Subaru EJ207, Toyota 3S-GE Dual VVT-i, BMW N20, Nissan CAS with slot triggers) produce CMP1 and CMP2 pulses.
   - Current canvas has zero rendering pipeline for bit-state 4 / CMP2.
4. **Missing Support for Bit-Array / Arbitrary Wheel Representations:**
   - `_drawCkpTrace` only loops over `wheel.totalTeeth * 2` assuming uniform tooth pitch with missing tooth start at `missingPosition`.
   - `_drawCmpTrace` only iterates `cam.getEvents()` (max 16 angle transitions).
   - Arbitrary patterns with multi-tooth groups (e.g. Avanza 144 segments with 3 distinct gap clusters) cannot be rendered by this logic.

---

## 3. Investigation of UI Pattern Selection & Database Structures

### 3.1 ArduStim TFTv2 vs. Existing ECUSniff Database Comparison

ArduStim TFTv2 (`external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` & `ardustim.ino`) stores **71 preset wheel types** (`MAX_WHEELS = 71`):

```cpp
typedef struct {
    const char* decoder_name;        // Friendly name in PROGMEM
    const unsigned char* edge_states_ptr; // Byte array of pin states
    float rpm_scaler;                // num_edges / 120 (for 360 deg) or num_edges / 240 (for 720 deg)
    uint16_t wheel_max_edges;        // Number of elements in array (4 to 1080)
    uint16_t wheel_degrees;          // 360 (crank only) or 720 (full 4-stroke cycle)
} wheels;
```

#### Bit-State Encoding in ArduStim:
- `0x00` (0): All Channels Low
- `0x01` (1): Crankshaft (CKP) HIGH
- `0x02` (2): Camshaft 1 (CMP1) HIGH
- `0x03` (3): CKP + CMP1 HIGH
- `0x04` (4): Camshaft 2 (CMP2) HIGH
- `0x05` (5): CKP + CMP2 HIGH
- `0x06` (6): CMP1 + CMP2 HIGH
- `0x07` (7): CKP + CMP1 + CMP2 HIGH

### 3.2 Preset Catalog and Category Mapping Audit (71 Presets)

| Index | Friendly Name (ArduStim TFTv2) | Edges | Degrees | Scaler | Assigned Category |
|---|---|---|---|---|---|
| 0 | `4 cylinder dizzy` | 4 | 360° | 0.03333 | Universal |
| 1 | `6 cylinder dizzy` | 6 | 360° | 0.05000 | Universal |
| 2 | `8 cylinder dizzy` | 8 | 360° | 0.06667 | Universal |
| 3 | `60-2 crank only` | 120 | 360° | 1.00000 | Universal |
| 4 | `60-2 crank and cam` | 240 | 720° | 1.00000 | Universal |
| 5 | `60-2 crank and 'half moon' cam` | 240 | 720° | 1.00000 | Universal |
| 6 | `36-1 crank only` | 72 | 360° | 0.60000 | Universal |
| 7 | `24-1 crank only` | 48 | 360° | 0.50000 | Universal |
| 8 | `4-1 crank wheel with cam` | 16 | 720° | 0.06667 | Universal |
| 9 | `8-1 crank only (R6)` | 16 | 360° | 0.13333 | Universal |
| 10 | `6-1 crank with cam` | 36 | 720° | 0.15000 | Universal |
| 11 | `12-1 crank with cam` | 144 | 720° | 0.60000 | Universal |
| 12 | `40-1 crank only (Ford V10)` | 80 | 360° | 0.66667 | Euro/Amerika |
| 13 | `Distributor style 4 cyl 50deg off, 40 deg on` | 9 | 720° | 0.15000 | Universal |
| 14 | `odd fire 90 deg pattern 0 and 135 pulses` | 24 | 360° | 0.20000 | Universal |
| 15 | `GM OptiSpark LT1 360 and 8` | 720 | 720° | 3.00000 | Euro/Amerika |
| 16 | `12-3 oddball` | 48 | 360° | 0.40000 | Universal |
| 17 | `36-2-2-2 H4 Crank only` | 72 | 360° | 0.60000 | MitsuNissanMazda (Subaru/Mazda) |
| 18 | `Toyota Avanza 1.3 Crank only` | 144 | 720° | 0.60000 | Toyota/Daihatsu |
| 19 | `Toyota Avanza 1.5 Crank only` | 144 | 720° | 0.60000 | Toyota/Daihatsu |
| 20 | `Toyota Avanza/Xenia/Terios/Rush ` | 144 | 720° | 0.60000 | Toyota/Daihatsu |
| 21 | `36-2-2-2 H6 Crank only` | 72 | 360° | 0.60000 | MitsuNissanMazda (Subaru) |
| 22 | `36-2-2-2 Crank and cam` | 144 | 720° | 0.60000 | MitsuNissanMazda (Subaru/Mazda) |
| 23 | `GM 4200 crank wheel` | 72 | 360° | 0.60000 | Euro/Amerika |
| 24 | `Mazda FE3 36-1 with cam` | 144 | 720° | 0.60000 | MitsuNissanMazda |
| 25 | `Mitsubishi 6g72 with cam` | 144 | 720° | 0.60000 | MitsuNissanMazda |
| 26 | `Buell Oddfire CAM wheel` | 80 | 720° | 0.33333 | Euro/Amerika |
| 27 | `GM LS1 crank and cam` | 720 | 720° | 6.00000 | Euro/Amerika |
| 28 | `GM 58x crank and 4x cam` | 240 | 720° | 1.00000 | Euro/Amerika |
| 29 | `Odd Lotus 36-1-1-1-1 flywheel` | 72 | 360° | 0.60000 | Euro/Amerika |
| 30 | `Honda RC51 with cam` | 48 | 720° | 0.20000 | Honda/Suzuki |
| 31 | `36-1 crank with 2nd trigger on teeth 33-34` | 144 | 720° | 0.60000 | Universal |
| 32 | `Chrysler NGC 36+2-2 crank, NGC 4-cyl cam` | 720 | 720° | 3.00000 | Euro/Amerika |
| 33 | `Chrysler NGC 36-2+2 crank, NGC 6-cyl cam` | 720 | 720° | 3.00000 | Euro/Amerika |
| 34 | `Chrysler NGC 36-2+2 crank, NGC 8-cyl cam` | 720 | 720° | 3.00000 | Euro/Amerika |
| 35 | `Nissan Livina Juke crank and cam` | 720 | 720° | 3.00000 | MitsuNissanMazda |
| 36 | `Weber-Marelli 8 crank+2 cam pattern` | 144 | 720° | 1.20000 | Euro/Amerika |
| 37 | `Fiat 1.8 16V crank and cam` | 720 | 720° | 3.00000 | Euro/Amerika |
| 38 | `Nissan 360 CAS with 6 slots` | 720 | 720° | 3.00000 | MitsuNissanMazda |
| 39 | `Mazda CAS 24-2 with single pulse outer ring` | 72 | 720° | 0.30000 | MitsuNissanMazda |
| 40 | `Yamaha 2002-03 R1 8 even-tooth crank with 1 tooth cam` | 64 | 720° | 0.26667 | Honda/Suzuki (Motorcycle/Univ) |
| 41 | `GM 4 even-tooth crank with 1 tooth cam` | 8 | 720° | 0.06666 | Euro/Amerika |
| 42 | `GM 6 even-tooth crank with 1 tooth cam` | 12 | 720° | 0.10000 | Euro/Amerika |
| 43 | `GM 8 even-tooth crank with 1 tooth cam` | 16 | 720° | 0.13333 | Euro/Amerika |
| 44 | `Volvo d12[acd] crank with 7 tooth cam` | 480 | 720° | 4.00000 | Euro/Amerika |
| 45 | `Mazda 36-2-2-2 with 6 tooth cam` | 360 | 720° | 1.50000 | MitsuNissanMazda |
| 46 | `Mitsubishi 4g63 aka 4/2 crank and cam` | 144 | 720° | 0.60000 | MitsuNissanMazda |
| 47 | `Audi 135 tooth crank and cam` | 1080 | 720° | 1.50000 | Euro/Amerika |
| 48 | `Honda D17 Crank (12+1)` | 144 | 720° | 0.60000 | Honda/Suzuki |
| 49 | `Honda Jazz Fit 04-08` | 144 | 720° | 0.60000 | Honda/Suzuki |
| 50 | `Honda Jazz Fit 04-08V2` | 144 | 720° | 0.60000 | Honda/Suzuki |
| 51 | `Honda Jazz Fit 04-08V3` | 144 | 720° | 0.60000 | Honda/Suzuki |
| 52 | `Mazda 323 AU version` | 30 | 720° | 1.00000 | MitsuNissanMazda |
| 53 | `Daihatsu 3+1 distributor (3 cylinders)` | 144 | 360° | 0.80000 | Toyota/Daihatsu |
| 54 | `Miata 99-05` | 144 | 720° | 0.60000 | MitsuNissanMazda |
| 55 | `12/1 (12 crank with cam)` | 144 | 720° | 0.60000 | Universal |
| 56 | `24/1 (24 crank with cam)` | 144 | 720° | 0.60000 | Universal |
| 57 | `Subaru 6/7 crank and cam` | 720 | 720° | 3.00000 | MitsuNissanMazda |
| 58 | `GM 7X` | 180 | 720° | 1.50200 | Euro/Amerika |
| 59 | `DSM 420a` | 144 | 720° | 0.60000 | Euro/Amerika / Mitsu |
| 60 | `Ford ST170` | 720 | 720° | 3.00000 | Euro/Amerika |
| 61 | `Mitsubishi 3A92` | 144 | 720° | 0.60000 | MitsuNissanMazda |
| 62 | `Toyota 4AGE` | 144 | 720° | 0.33300 | Toyota/Daihatsu |
| 63 | `Toyota 4AGZE` | 144 | 720° | 0.33300 | Toyota/Daihatsu |
| 64 | `Suzuki DRZ400` | 72 | 360° | 0.60000 | Honda/Suzuki |
| 65 | `Jeep 2000` | 360 | 720° | 1.50000 | Euro/Amerika |
| 66 | `BMW N20` | 240 | 720° | 1.00000 | Euro/Amerika |
| 67 | `Dodge Viper V10 1996-2002` | 240 | 720° | 1.00000 | Euro/Amerika |
| 68 | `36-2 with 1 tooth cam` | 144 | 720° | 0.60000 | Toyota/Daihatsu (2JZ-GTE) / Univ |
| 69 | `GM 40 tooth OSS wheel for Transmissions` | 80 | 360° | 1.00000 | Euro/Amerika |
| 70 | `GM 40 tooth OSS wheel for Transmissions` (Preset 70) | 80 | 360° | 1.00000 | Euro/Amerika |

*(Total: 71 preset definitions, 0 to 70 inclusive)*

### 3.3 Brand Category Filtering Logic Review
In `PageWheelBrowser::matchesCategory`:
- Category matching currently uses runtime string parsing (`strstr`).
- **Issues identified:**
  1. `Toyota Avanza/Xenia/Terios/Rush ` has a trailing space in ArduStim; `strstr` matches `"Avanza"`, but strict equality would fail.
  2. Presets like `36-2-2-2 H4 Crank only` and `36-2-2-2 Crank and cam` contain `"36-2"` and get classified into `Universal`, whereas automotive users expect them in `Subaru / MitsuNissanMazda`.
  3. `36-2 with 1 tooth cam` (Toyota 2JZ-GTE VVTi crank + non-VVTi cam) gets categorized as `Universal` instead of `Toyota/Daihatsu`.
- **Recommendation:** Add an explicit `WheelCategory category` enum field directly into the preset database struct, with fallback to string matching for custom captured user presets.

---

## 4. Investigation of Test Infrastructure & PlatformIO Configuration

### 4.1 Native Environment vs. Embedded Target Execution

`platformio.ini` defines:
```ini
[env:native]
platform = native
test_framework = unity
build_flags = 
    -std=c++17
    -I include
    -I lib/engine/include
lib_ignore =
    hal
    webapi
    ui
```

#### Diagnostic Execution of `pio test -e native`:
```
Processing * in native environment
Building...
'gcc' is not recognized as an internal or external command,
operable program or batch file.
'g++' is not recognized as an internal or external command,
operable program or batch file.
*** [.pio\build\native\unity_config_build\unity_config.o] Error 1
```

#### Root Cause Analysis:
1. The host Windows environment lacks a native GCC/MinGW toolchain in system `PATH`.
2. PlatformIO Core v6.1.18 does not auto-download a host desktop GCC compiler when `platform = native` is invoked; it expects system-installed GCC/Clang.
3. In contrast, the ESP32-S3 cross-compiler (`toolchain-xtensa-esp32s3` at `C:\Users\User\.platformio\packages\toolchain-xtensa-esp32s3\bin`) is fully installed and working, building the complete firmware in 9.1s.

### 4.2 Comprehensive Testing Architecture Strategy

To guarantee reliable automated test execution:
1. **Target-Based Embedded Unity Runner (`env:esp32s3` Test Runner):**
   - PlatformIO supports embedded testing on ESP32-S3 via Unity.
   - Alternatively, a self-test diagnostic module (`test_runner_esp32s3.cpp`) can be compiled and triggered over Serial / USB CDC on boot or via a dedicated diagnostic CLI/Web command.
2. **Pure C++ Host Verification Harness (Header-Only Logic):**
   - Timing math, bit-array unpacking, edge detection, and RMT buffer compilation algorithms are 100% platform-agnostic standard C++ (no FreeRTOS or IDF dependencies).
   - Can be built by any standard C++17 compiler (GCC, Clang, MSVC) or executed within a standalone verification test file.

---

## 5. Requirements for Waveform Canvas Arbitrary Bit-Array Rendering (0 - 720°)

### 5.1 Dual Data Model Architecture
The Waveform Canvas must accept either:
1. **Parametric Mode:** `ParametricWheel` + `CamEventTable` (for manual signal tuning).
2. **Arbitrary Bit-Array Mode:** `const uint8_t* patternData`, `uint16_t edgeCount`, `uint16_t wheelDegrees` (for ArduStim OEM presets and multi-gap patterns).

### 5.2 Dynamic Vertical Track Partitioning & Layout
The canvas must dynamically adjust track positions based on canvas height ($H=124\text{ px}$ vs $H=76\text{ px}$) and active channel count (2 traces vs 3 traces):

```
+-------------------------------------------------------------+
| Header Bar: Track Labels & Angle Markers (0, 360, 720 deg)  |
|-------------------------------------------------------------|
| TRACK 0: CKP (Yellow #FFE0)                                 |
|          [High Level] ---------\                            |
|          [Low Level]            \_________________          |
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - |
| TRACK 1: CMP1 (Green #07E0)                                 |
|          [High Level] ----------------------------\         |
|          [Low Level]                               \_______ |
| - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - | (Optional)
| TRACK 2: CMP2 (Cyan #07FF or Magenta #F81F)                 |
|          [High Level]                  /----------\         |
|          [Low Level]  ________________/            \_______ |
+-------------------------------------------------------------+
```

#### Geometry Formulation:
For canvas height $H$ and $N_{\text{tracks}}$ active tracks ($N \in \{2, 3\}$):
- Header height: $H_{\text{hdr}} = 14\text{ px}$.
- Usable height: $H_{\text{usable}} = H - H_{\text{hdr}} - 4\text{ px}$.
- Track height: $H_{\text{track}} = \lfloor \frac{H_{\text{usable}}}{N_{\text{tracks}}} \rfloor$.
- For Track $k$ ($k \in [0, N-1]$):
  - Track Top: $Y_{\text{top}}(k) = H_{\text{hdr}} + 2 + k \times H_{\text{track}}$
  - High Level $Y_{\text{high}}(k) = Y_{\text{top}}(k) + 3\text{ px}$
  - Low Level $Y_{\text{low}}(k) = Y_{\text{top}}(k) + H_{\text{track}} - 4\text{ px}$

### 5.3 Horizontal Angle & Degree Mapping ($0 - 720^\circ$)
- Available horizontal width: $W_{\text{avail}} = W_{\text{canvas}} - X_{\text{offset}} - X_{\text{pad}}$ (e.g. $X_{\text{offset}} = 28\text{ px}, X_{\text{pad}} = 8\text{ px} \implies W_{\text{avail}} = 420\text{ px}$).
- **360° Pattern Handling:** If `wheelDegrees == 360`, the pattern covers 1 crankshaft revolution ($0 - 360^\circ$). The canvas must render it twice ($2\times$) to fill the complete $0 - 720^\circ$ 4-stroke cycle.
- **720° Pattern Handling:** If `wheelDegrees == 720`, the pattern spans the full $0 - 720^\circ$ cycle once ($1\times$).
- **Total Render Elements:**
  $$N_{\text{total}} = \begin{cases} 2 \times \text{edgeCount}, & \text{if } \text{wheelDegrees} = 360 \\ \text{edgeCount}, & \text{if } \text{wheelDegrees} = 720 \end{cases}$$
- For element index $i \in [0, N_{\text{total}} - 1]$:
  $$X_{\text{start}}(i) = X_{\text{offset}} + \lfloor \frac{i \times W_{\text{avail}}}{N_{\text{total}}} \rfloor, \quad X_{\text{end}}(i) = X_{\text{offset}} + \lfloor \frac{(i + 1) \times W_{\text{avail}}}{N_{\text{total}}} \rfloor$$

### 5.4 High-Density Decimation & Narrow Edge Preservation
For patterns with high edge counts (e.g. `Audi 135 tooth` with 1080 edges, `OptiSpark` with 720 edges) on a 420px width:
- Multiple elements map to a single pixel column ($N_{\text{elem}} > W_{\text{avail}}$).
- **Rule:** If any element within a pixel column has a state transition ($0 \to 1$ or $1 \to 0$), the visualizer must draw both high and low levels with a vertical connecting line, ensuring narrow sync pulses or single missing teeth are never dropped due to aliasing.

---

## 6. Requirements for Automated Testing & Signal Verification

### 6.1 Test Suites & Validation Criteria

```
+-------------------------------------------------------------------------+
|                        ECUSniff Signal Test Suite                       |
+-------------------------------------------------------------------------+
                                    |
          +-------------------------+-------------------------+
          |                                                   |
          v                                                   v
[Array Parity Suite]                                [RMT Timing & Pulse Suite]
- Match all 71 presets against                      - Verify microsecond durations
  ArduStim wheel_defs.h byte-for-byte                 at 200, 850, 3000, 6000 RPM
- Validate edgeCount, wheelDegrees,                 - Verify no pulse > 32,767 us
  and friendly names                                  (RMT hardware limit)
                                                    - Verify EOT {0,0,0,0} marker
                                                    - Verify cycle sum == T_cycle +- 1us
```

### 6.2 Specific Critical Patterns Under Test:
1. **`NEW_AVANZA` & `OLD_AVANZA` (144 Segments / 720°):**
   - Validate 3-gap sync group positions.
   - Validate CMP cam phase alignment with 3rd crank tooth group.
2. **`AVANZA_XENIA_TERIOS_RUSH` (144 Segments / 720°):**
   - Validate Daihatsu / Toyota variable timing cam triggers.
3. **`MITSUBISHI_4G63` (4/2 CAS Wheel):**
   - Validate 2 uneven crank pulses and 2 cam pulses across 720°.
4. **`SIXTY_MINUS_TWO` (60-2 Bosch Motronic):**
   - Validate 58 active teeth, 2 missing teeth gap ($120\text{ edges}$ per 360°).
5. **`THIRTY_SIX_MINUS_TWO_TWO_TWO` (36-2-2-2 Subaru H4/H6 / Mazda):**
   - Validate triple 2-tooth gap sequence.

### 6.3 Timing Precision Verification Formulas:
For any wheel preset with $N$ elements at engine speed $\text{RPM}$:
- Cycle Period: $T_{\text{cycle}} = \frac{120 \times 10^6}{\text{RPM}}\ \mu\text{s}$.
- Time per Segment: $\Delta t = \frac{T_{\text{cycle}}}{N}\ \mu\text{s}$.
- Edge Angle: $\theta_k = k \times \frac{720.0^\circ}{N}$.

The test framework must assert that:
1. Every segment duration satisfies $|\Delta t_{\text{gen}} - \Delta t_{\text{expected}}| \le 1\ \mu\text{s}$.
2. Total cycle pulse duration satisfies $\left| \sum \Delta t_k - T_{\text{cycle}} \right| \le 2\ \mu\text{s}$.
3. High and low logic levels match the source array bit-for-bit:
   - $\text{CKP}(k) = (\text{patternData}[k \bmod \text{edgeCount}] \ \& \ 0\text{x}01) \ne 0$
   - $\text{CMP1}(k) = (\text{patternData}[k \bmod \text{edgeCount}] \ \& \ 0\text{x}02) \ne 0$
   - $\text{CMP2}(k) = (\text{patternData}[k \bmod \text{edgeCount}] \ \& \ 0\text{x}04) \ne 0$

---

## 7. Action Plan for Downstream Implementation

1. **Preset Database Engine (`lib/engine/include/wheel_database_full.h`):**
   - Export all 71 ArduStim wheel patterns as `const uint8_t` PROGMEM arrays with identical friendly names and accurate metadata (`rpm_scaler`, `wheel_max_edges`, `wheel_degrees`, `category`).
2. **RMT Generator Bit-Array Engine (`lib/hal/src/rmt_generator.cpp`):**
   - Implement `generateBitArrayCycle()` capable of converting any arbitrary bit-array into microsecond RMT pulse items with duration slicing ($\le 30,000\ \mu\text{s}$) and EOT markers.
3. **Responsive Waveform Canvas (`lib/ui/src/waveform_canvas.cpp`):**
   - Refactor `WaveformCanvas` with dynamic track layout ($H=76\text{ px}$ and $H=124\text{ px}$), full $0 - 720^\circ$ mapping, and multi-channel support (CKP Yellow, CMP1 Green, CMP2 Cyan).
4. **Automated Test Harness (`test/test_wheel_patterns.cpp`):**
   - Implement comprehensive Unit Tests verifying bit-level fidelity, edge transitions, and timing math for all 71 patterns.
