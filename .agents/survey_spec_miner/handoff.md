# Handoff Report: Specification Mining for Engine Wheel Patterns

**To**: Parent Agent / Orchestrator (`orchestrator`)  
**From**: Spec Miner Agent (`survey_spec_miner`)  
**Timestamp**: `2026-09-01T09:56:00Z`  
**Report Artifact**: `g:\semester 7\ECUSniff\.agents\survey_spec_miner\spec_mining_report.md`  
**Data Artifact**: `g:\semester 7\ECUSniff\.agents\survey_spec_miner\parsed_wheels.json`

---

### 1. Observation
- Inspected `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` (lines 75–147, `WheelType` enum), `ardustim.ino` (lines 155–227, `Wheels[MAX_WHEELS]` table), and `WheelPatternManager.cpp` (lines 99–170, `friendlyNames[]` table).
- Verified that `MAX_WHEELS` equals **70** (indices 0 to 69).
- Extracted all 70 raw arrays and executed automated verification script (`analyze_patterns.py`). All 70 arrays have lengths matching their `wheel_max_edges` declaration (from 4 edges up to 1080 edges).
- Pin mapping in `PinMapping.h` (lines 18–48) and `ardustim.ino` (lines 379–389) confirms:
  - Bit 0 (`0x01`): Crankshaft signal (`CKP` / PB0 / Pin 53 / Pin 8)
  - Bit 1 (`0x02`): Camshaft 1 signal (`CMP1` / PB1 / Pin 52 / Pin 9)
  - Bit 2 (`0x04`): Camshaft 2 signal (`CMP2` / PB2 / Pin 51 / Pin 10)
  - Bit 3 (`0x08`): Knock signal (`KNOCK` / PB3 / Pin 50 / Pin 11)
- Observed discrete array values: `0` (all low), `1` (CKP), `2` (CMP1), `3` (CKP+CMP1), `4` (CMP2 in LS1 seg 0), `6` (CMP1+CMP2 in N20), `7` (CKP+CMP1+CMP2 in N20).
- Cycle lengths: 17 patterns are $360^\circ$ (crank revolution); 53 patterns are $720^\circ$ (full 4-stroke cycle).

---

### 2. Logic Chain
1. **Database Parity**: For ECUSniff to achieve 100% parity with ArduStim TFTv2, every enum index (0–69) in `WheelType` must be mirrored, retaining both the standard friendly name (e.g. `Toyota Avanza 1.3 Crank only`) and the compact TFT name (e.g. `Old Avanza`).
2. **Arbitrary Bit-Array Support**: Critical patterns like Avanza (Old, New, Avanza/Xenia/Terios/Rush), 4G63, 6G72, 3A92, and Honda Jazz V1–V3 are non-equidistant, multi-pulse arrays. Attempting to generate them via equidistant missing-tooth formulas will fail. They must be generated directly from the 1D segment arrays.
3. **ESP32 RMT Microsecond Scaling**: Given array length $E$ and cycle degrees $D$, each segment represents $\Delta \theta = \frac{D}{E}$ degrees. The duration in microseconds at speed $RPM$ is:
   $$T_{\text{seg}} = \frac{D \times 10^6}{6 \times E \times RPM}\ \mu\text{s}$$
   Consecutive identical values merge into single RMT pulse train items of length $N \times T_{\text{seg}}$, followed by an EOT zero-terminator `{0, 0, 0, 0}` for hardware looping.
4. **Dual Cam Support**: `BMW_N20` and `GM_LS1_CRANK_AND_CAM` require `CMP2` (bit 2 / RMT channel 4) to be actively supported alongside `CKP` (channel 0) and `CMP1` (channel 2).

---

### 3. Caveats
- `avanza_xenia_terios_rush_friendly_name` in ArduStim has a trailing space (`"Toyota Avanza/Xenia/Terios/Rush "`). ECUSniff's lookup functions should handle both exact string and trimmed string.
- `GM_LS1_CRANK_AND_CAM` has `rpm_scaler = 6.0` (doubled in ArduStim for AVR timer limits); ECUSniff should calculate microsecond pulse width using physical formula $\frac{D \times 10^6}{6 \times E \times RPM}$ rather than the legacy scaler.
- Flash storage: all 70 pattern arrays take ~15 KB in total. Storing them in flash (`const uint8_t[]`) rather than allocating dynamic heap on boot ensures zero RAM fragmentation on ESP32-S3.

---

### 4. Conclusion
The specification mining for the engine wheel pattern database is complete. All 70 presets, their metadata, exact friendly names, brand categories, bitmask truth tables, timing math, and critical pattern details are fully documented in `spec_mining_report.md` and structured in `parsed_wheels.json`.

---

### 5. Verification Method
1. Run verification script:
   ```bash
   python .agents/survey_spec_miner/analyze_patterns.py
   ```
2. Verify all 70 entries against `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` and `ardustim.ino`.
3. Check `spec_mining_report.md` for full breakdown tables.
