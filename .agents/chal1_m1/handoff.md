# Milestone 1 Challenger 1 Report: Wheel Database Empirical Verification

## 1. Observation

Direct empirical verification was conducted by creating and executing an independent oracle script (`scratch/empirical_challenge_m1.py`) and running the full project compilation and test suites:

### 1.1 Byte-for-Byte Array Verification
- **Target File**: `lib/engine/src/wheel_database.cpp` (Lines 10..1088)
- **Reference File**: `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` & `external/ardustim-tftv2-touchscreen/ardustim/ardustim.ino`
- **Result**:
  - Total presets evaluated: **70** (indices 0 .. 69).
  - Total array bytes compared: **15,429 bytes (~15.07 KB)**.
  - Byte differences found: **0**.
  - All 70 bit arrays match ArduStim `wheel_defs.h` byte-for-byte with 100% fidelity.
  - Array lengths match `totalEdges` and `wheel_max_edges` across all patterns (from min 4 edges on `dizzy_four_cylinder` up to 1080 edges on `audi_135_with_cam`).

### 1.2 Friendly Names & Cycle Degrees Verification
- **Friendly Names**: All 70 strings in `s_wheelDatabase[].friendlyName` match verbatim against ArduStim definitions, including edge-case whitespace preservation (e.g. Preset 20 `"Toyota Avanza/Xenia/Terios/Rush "` with trailing space).
- **Cycle Degrees**:
  - Exactly 17 patterns mapped to `WheelCycleDegrees::CRANK_360` (360°).
  - Exactly 53 patterns mapped to `WheelCycleDegrees::ENGINE_720` (720°).
  - 100% matched against `Wheels[].wheel_degrees` in `ardustim.ino`.

### 1.3 Brand Category Distribution & `getWheelsByCategory`
- Category breakdown across all 8 `BrandCategory` enums:
  - `BrandCategory::TOYOTA_DAIHATSU`: 8 presets (Old Avanza, New Avanza, Rush/Terios, 36-2-2-2 K3-3SZ, Daihatsu 3+1, 4A-GE CAS, 4A-GZE, 36-2+1T 2JZ)
  - `BrandCategory::HONDA`: 5 presets (Honda RC51, Honda D17, Honda Jazz/Fit 04-08 V1, V2, V3)
  - `BrandCategory::MITSUBISHI`: 4 presets (6G72, 4G63 4/2, DSM 420A, 3A92 3-Cyl)
  - `BrandCategory::NISSAN`: 2 presets (Nissan Livina Juke, Nissan 360 CAS)
  - `BrandCategory::EURO_US`: 23 presets (GM, Ford, Chrysler NGC, BMW N20, Audi, Fiat, Volvo, Viper, etc.)
  - `BrandCategory::UNIVERSAL`: 28 presets (60-2, 36-1, 24-1, 12-1, 8-1, 4-1, distributors, Suzuki Swift, Subaru 6/7, Mazda, Yamaha, etc.)
  - `BrandCategory::CUSTOM`: 0 presets (reserved for runtime sniffer patterns)
  - **Sum of all brand categories**: $8 + 5 + 4 + 2 + 23 + 28 + 0 = \mathbf{70\text{ presets}}$.
  - `WheelDatabase::getWheelsByCategory(BrandCategory::ALL, ...)` retrieves all 70 presets.

### 1.4 Channel Presence Flags
- `hasCmp1`: Active on 50 patterns (matching bitmask 0x02).
- `hasCmp2`: Active on 2 dual-cam patterns:
  - Preset 27 (`GM LS1 crank and cam`)
  - Preset 66 (`BMW N20`)
- 100% consistent with array bit values.

### 1.5 Build & Automated Test Execution
- **PlatformIO Compilation**: `pio run -e esp32s3`
  - Output: `RAM: 22.1% (72332 / 327680 bytes)`, `Flash: 28.2% (1033821 / 3670016 bytes)` -> `[SUCCESS] Took 8.29 seconds`.
- **E2E Test Suite**: `python test/run_e2e_tests.py`
  - Output: `TOTAL: 987/987 passed (0 failed)`.

---

## 2. Logic Chain

1. **Byte-Level Integrity**: Comparing raw byte arrays from `lib/engine/src/wheel_database.cpp` directly against the AST/tokenized arrays from `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` proved that no data corruption, truncation, or indexing shift occurred during the porting of all 70 trigger wheels (Observation 1.1).
2. **Metadata Consistency**: All metadata properties (`friendlyName`, `totalEdges`, `cycleDegrees`, `hasCmp1`, `hasCmp2`) in `s_wheelDatabase[70]` are perfectly aligned with both ArduStim source definitions and hardware signal requirements (Observations 1.2 & 1.4).
3. **Partition Completeness**: The 8 `BrandCategory` enums form a complete, disjoint partition of the database where the sum of category subsets equals the master count of 70 (Observation 1.3).
4. **Firmware & Test Pass**: Clean compilation on `esp32s3` target and 100% pass on 987 automated test assertions prove interface contract conformance and functional stability (Observation 1.5).

---

## 3. Caveats

- **Minor UI Note on `shortName` for Preset 55**:
  - Preset 11 (`"12-1 crank with cam"`, 12-1 missing tooth) and Preset 55 (`"12/1 (12 crank with cam)"`, 12 crank + 1 cam) both currently have `shortName` set to `"12-1 CKP+CMP"`.
  - While this does not affect `id`, `friendlyName`, `bitArray`, or `cycleDegrees` correctness, in M3 UI development it is recommended to set Preset 55 `shortName` to `"12/1 CKP+CMP"` for distinct compact display.
- **RMT Pulse Train Generation**: Physical microsecond RMT pulse compilation and hardware channel streaming are scoped to Milestone 2.

---

## 4. Conclusion

**VERDICT: APPROVE**

Milestone 1 satisfies all requirements set forth in `ORIGINAL_REQUEST.md` (§R1) and `PROJECT.md` (§4.1):
- All 70 ArduStim wheel bit-arrays (15,429 bytes) are ported with 100% byte-for-byte fidelity.
- Friendly names, edge counts, and cycle degrees (360°/720°) match ArduStim verbatim.
- Brand category classification cleanly partitions all 70 patterns across 8 categories.
- PlatformIO compilation builds cleanly with 0 errors.

---

## 5. Verification Method

To independently reproduce and verify this assessment:

1. **Run Oracle Verification Harness**:
   ```bash
   python scratch/empirical_challenge_m1.py
   ```
   *Expected*: `[PASS]` on all 70 bit-arrays, names, cycle degrees, channel presence, and category counts (sum = 70).

2. **Run E2E Test Suite**:
   ```bash
   python test/run_e2e_tests.py
   ```
   *Expected*: `TOTAL: 987/987 passed (0 failed)`.

3. **Compile Firmware on ESP32-S3 Target**:
   ```bash
   pio run -e esp32s3
   ```
   *Expected*: `========================= [SUCCESS] =========================`.
