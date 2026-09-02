# Forensic Audit Report: Milestone 2 & Milestone 3

**Work Product**: Milestone 2 (ESP32-S3 RMT Generator & Engine Bit-Array Driver) & Milestone 3 (UI Waveform Canvas & Live Visualizer Rasterization)  
**Profile**: General Project / Integrity Forensics  
**Verdict**: **CLEAN**

---

## 1. Observation

Direct source inspection, tool execution logs, and compilation checks were performed across all components of Milestone 2 and Milestone 3.

### 1.1 Milestone 2 Source Forensics (`lib/hal/src/rmt_generator.cpp`, `lib/hal/include/rmt_generator.h`)
- **Arbitrary Bit-Array to RMT Pulse Conversion**:
  - `RmtGenerator::compileBitArrayToRmt` (`lib/hal/src/rmt_generator.cpp:112-201`):
    - Computes cycle microsecond timebase dynamically via 64-bit integer formula:
      $$T_{\text{cycle}} = \frac{\text{cycleDegrees} \times 1,000,000}{6 \times \text{rpm}}\ \mu\text{s}$$
    - Calculates start and end timestamps per segment using cumulative integer scaling:
      $$t_{\text{start}}[s] = \frac{s \times T_{\text{cycle}}}{N}, \quad t_{\text{end}}[s] = \frac{(s+1) \times T_{\text{cycle}}}{N}$$
    - Compresses runs of identical states via Run-Length Encoding (RLE).
    - Enforces duration chunk slicing $\le 30,000\ \mu\text{s}$ (`MAX_RMT_DURATION_CHUNK = 30000`) to prevent ESP32 RMT 15-bit hardware counter overflow ($32,767\ \mu\text{s}$) down to 10 RPM.
    - Appends genuine hardware zero-terminator EOT `{duration0=0, level0=0, duration1=0, level1=0}` for glitch-free loopback.
  - Zero hardcoded lookup tables, zero dummy return shortcuts, zero static bypasses found.
- **ESP32-S3 Multi-Channel Hardware Synchronization**:
  - Channel 0 (`CH_CKP` / GPIO 4): 2 Memory Blocks (96 items)
  - Channel 2 (`CH_CMP` / GPIO 5): 1 Memory Block (48 items)
  - Channel 3 (`CH_CMP2` / GPIO 6): 1 Memory Block (48 items)
  - Non-overlapping hardware memory block offsets (Blocks 0..1 for CH0, Block 2 for CH2, Block 3 for CH3) prevent DMA collision on ESP32-S3.
  - Periodic replication: 360° crank patterns duplicate $2\times$ into 720° engine cycles (`prepareBitArrayCycle`, lines 218–226).
  - Atomic double-buffering ping-pong implementation (`_ckpBufferA/B`, `_cmpBufferA/B`, `_cmp2BufferA/B`) allows zero-jitter in-flight RPM and pattern switching.

### 1.2 Milestone 3 Source Forensics (`lib/ui/src/waveform_canvas.cpp`, `lib/ui/include/waveform_canvas.h`, `lib/ui/src/page_wheel_browser.cpp`)
- **Authentic Dynamic Rasterization**:
  - `WaveformCanvas::render(const WheelDefinition* wheel, int32_t screenX, int32_t screenY)` (`lib/ui/src/waveform_canvas.cpp:153-192`):
    - Reads PROGMEM bit-arrays directly from `WheelDefinition`.
    - Computes vertical track geometry dynamically via `_calculateTrackGeometry` for 1, 2, or 3 tracks (`TFT_YELLOW` for CKP, `TFT_GREEN` for CMP1, `TFT_CYAN` for CMP2).
    - Adaptively scales to dynamic canvas heights ($456 \times 124\text{ px}$ in `PageWheelBrowser`, $448 \times 76\text{ px}$ in `PageDashboard`) with zero vertical unpainted gaps.
    - `_drawBitArrayTrace` (`lines 90-151`):
      - Iterates across horizontal pixel columns $x \in [0, W_{\text{avail}}-1]$.
      - Maps column pixel span to segment index range $[s_{\text{start}}, s_{\text{end}})$.
      - Preserves high-frequency transitions via vertical step lines (`drawFastVLine`) during intra-column oscillations or column-to-column level shifts.
  - Zero pre-rendered static bitmap sprites, zero fake oscilloscope screenshot assets.
- **UI Category Navigation**:
  - `PageWheelBrowser` (`lib/ui/src/page_wheel_browser.cpp:9-123`) binds to `BrandCategory` enum (ALL, TOYOTA, HONDA, MITSU, NISSAN, EURO/US, UNIV, CUSTOM) with $O(1)$ category filtering.
  - Displays all 70 ArduStim presets with friendly names, short names, cycle degrees, total edges, and channel badges (`[CKP + CMP1 + CMP2]`, `[CKP + CMP1]`, `[CKP Only]`).

### 1.3 Empirical Build and Test Execution Output
1. **PlatformIO Firmware Compilation (`pio run -e esp32s3`)**:
   - Environment: `esp32s3` (ESP32-S3 240MHz, 320KB RAM, 8MB Flash)
   - Status: `[SUCCESS]` in 9.69 seconds
   - RAM: 28.6% (used 93,852 bytes / 327,680 bytes)
   - Flash: 28.6% (used 1,049,313 bytes / 3,670,016 bytes)
2. **4-Tier E2E Regression Test Suite (`python test/run_e2e_tests.py`)**:
   - Tier 1 (Feature Coverage): 791 / 791 passed `[PASS]`
   - Tier 2 (Boundary & Corner Cases): 25 / 25 passed `[PASS]`
   - Tier 3 (Cross-Feature Combinations): 15 / 15 passed `[PASS]`
   - Tier 4 (Real-World OEM Scenarios): 156 / 156 passed `[PASS]`
   - Total: **987 / 987 passed (0 failed)**
3. **M2 Slicing & Timing Verification (`python scratch/verify_m2_implementation.py`)**:
   - Test 1 (Duration Slicing $\le 30,000\ \mu\text{s}$ across 10–12,000 RPM): `PASSED`
   - Test 2 (Microsecond Conservation & Zero Drift): `PASSED`
   - Test 3 (360° to 720° Periodic Replication): `PASSED`
   - Test 4 (Dual-Cam Multi-Channel Synchronization): `PASSED`
4. **M3 Geometric & Category Verification (`python scratch/verify_m3_implementation.py`)**:
   - Canvas Track Geometry & Decimation across 4 resolutions: `PASSED`
   - Brand Category Classification & Mapping: `PASSED`

---

## 2. Logic Chain

1. **No Hardcoded Output Cheats**:
   Inspection of `RmtGenerator::compileBitArrayToRmt` reveals pure algorithmic Run-Length Encoding and 64-bit microsecond scaling. No hardcoded pulse tables or bypass branches exist.
2. **No Pre-Rendered Visual Cheats**:
   Inspection of `WaveformCanvas` demonstrates that step waveforms are rasterized in real time directly from the bit-arrays in Flash PROGMEM (`WheelDefinition::bitArray`), correctly mapping $0..720^\circ$ into horizontal pixel coordinates with decimation edge preservation.
3. **Zero Cumulative Drift**:
   Using cumulative time calculation $t[s] = \frac{s \times T_{\text{cycle}}}{N}$ rather than incremental additions guarantees that the sum of all segment durations $\sum D_i \equiv T_{\text{cycle}}$ exactly equals the cycle period down to 0 microsecond error across unlimited engine cycles.
4. **Hardware Safety**:
   Slicing durations into sub-chunks of $\le 30,000\ \mu\text{s}$ guarantees that ESP32 RMT 15-bit hardware counter overflow ($> 32,767\ \mu\text{s}$) cannot occur even at 10 RPM. Memory block isolation (CH0: 2 blocks, CH2: 1 block, CH3: 1 block) prevents TX RAM buffer collision on ESP32-S3.
5. **Contract Compliance**:
   All structures, enums (`BrandCategory`, `WheelCycleDegrees`), methods (`compileBitArrayToRmt`, `render`, `matchesCategory`), and pin definitions match `PROJECT.md` and `ORIGINAL_REQUEST.md`.

---

## 3. Caveats

- Native C++ unit tests (`pio test -e native`) require host GCC/G++ in PATH; verification was executed via the 4-tier Python oracle test suite (`test/run_e2e_tests.py`), M2/M3 verification scripts, and direct ESP32-S3 cross-compilation (`pio run -e esp32s3`).

---

## 4. Conclusion

**Verdict: CLEAN**
- Milestone 2 and Milestone 3 implementations are authentic, complete, and free of shortcuts, dummy mocks, or integrity violations.
- Real RMT pulse compilation from bit-arrays with RLE and duration chunk slicing is fully verified.
- Real-time oscilloscope waveform rasterization from `WheelDefinition` bit-arrays is fully verified.
- Firmware compiles cleanly (`pio run -e esp32s3` - SUCCESS) and passes all 987 E2E tests (100%).

---

## 5. Verification Method

To independently reproduce the forensic verification:

```bash
# 1. Clean Firmware Compilation for ESP32-S3
pio run -e esp32s3

# 2. Comprehensive 4-Tier E2E Test Suite
python test/run_e2e_tests.py

# 3. M2 Slicing, Conservation, and Multi-Channel Verification
python scratch/verify_m2_implementation.py

# 4. M3 Canvas Geometry and Brand Category Verification
python scratch/verify_m3_implementation.py
```
