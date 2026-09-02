# Forensic Audit Report: Milestone 1 (Wheel Pattern Database)

**Work Product**: `lib/engine/include/wheel_database.h`, `lib/engine/src/wheel_database.cpp`, `lib/engine/include/pattern_types.h`  
**Profile**: General Project (Forensic Integrity)  
**Integrity Mode**: Development (per `ORIGINAL_REQUEST.md`)  
**Verdict**: **CLEAN** (0 Integrity Violations Detected)

---

## 1. Observation

### 1.1 Direct Source Code Inspection
- **File Checked**: `lib/engine/src/wheel_database.cpp` (1,262 lines, 76,159 bytes).
  - Lines 10–1090: Exactly 70 PROGMEM bit-arrays (`s_pattern_00_...` to `s_pattern_69_...`) storing 15,429 raw bytes of trigger pattern data.
  - Lines 1094–1165: Master database lookup array `static const WheelDefinition s_wheelDatabase[70]` containing metadata for all 70 presets.
  - Lines 1168–1176: Robust case-insensitive string comparator `static bool stringEqualsIgnoreCase(const char* a, const char* b)` with `unsigned char` safety casting.
  - Lines 1178–1261: Public query APIs within namespace `WheelDatabase`:
    - `size_t getWheelCount()`: Returns exact count `TOTAL_WHEELS` (70).
    - `const WheelDefinition* getWheel(size_t index)`: Bounds-checked (`index < TOTAL_WHEELS`), returns valid pointer or `nullptr`.
    - `const WheelDefinition* getWheelById(uint8_t id)`: Bounds-checked (`id < TOTAL_WHEELS`), returns valid pointer or `nullptr`.
    - `const WheelDefinition* findByFriendlyName(const char* name)`: Null-safe, two-pass search (exact `strcmp` then `stringEqualsIgnoreCase`).
    - `const WheelDefinition* findByShortName(const char* name)`: Null-safe, two-pass search (exact `strcmp` then `stringEqualsIgnoreCase`).
    - `size_t getWheelsByCategory(BrandCategory cat, const WheelDefinition** outWheels, size_t maxOut)`: Null-safe destination buffer handling, returns total matching count, strictly respects `maxOut` boundary.
    - `const char* getCategoryName(BrandCategory cat)`: Complete switch statement covering all categories with `default: return "Unknown";`.
- **File Checked**: `lib/engine/include/wheel_database.h` (101 lines).
  - Declares `BrandCategory` enum (8 categories: `ALL`, `TOYOTA_DAIHATSU`, `HONDA`, `MITSUBISHI`, `NISSAN`, `EURO_US`, `UNIVERSAL`, `CUSTOM`, `COUNT`).
  - Declares `WheelCycleDegrees` enum (`CRANK_360 = 360`, `ENGINE_720 = 720`).
  - Declares `WheelDefinition` struct and `WheelDatabase` API functions.
- **File Checked**: `lib/engine/include/pattern_types.h` (29 lines).
  - Declares standard channel bitmasks: `SIGNAL_BIT_CKP = 0x01`, `SIGNAL_BIT_CMP1 = 0x02`, `SIGNAL_BIT_CMP2 = 0x04`, `SIGNAL_BIT_KNOCK = 0x08`.
  - Declares `PulseTransition` struct for RMT microsecond conversions.

### 1.2 Automated Forensic Verification Script Results
Command executed: `python .agents/aud_m1/verify_forensics.py`
```text
================================================================================
                    FORENSIC INTEGRITY AUDIT - MILESTONE 1                      
================================================================================
ArduStim Header Arrays Extracted: 70
ArduStim Wheels[] Entries Extracted: 70
ECUSniff PROGMEM Arrays Extracted: 70
ECUSniff Master Table Entries: 70
[PASS] Check 1: ECUSniff master table contains exactly 70 definitions (0..69).
[PASS] Check 2: ECUSniff contains exactly 70 PROGMEM bit-arrays.
[PASS] Check 3: All 70 bit-arrays match ArduStim byte-for-byte (15429 total bytes verified).
[PASS] Check 4: All 70 friendly names match ArduStim source definitions 100%.
[PASS] Check 5: All 70 cycle degrees and total edge counts match ArduStim table.
[PASS] Check 6: hasCmp1 and hasCmp2 flags accurately reflect bitmasks across all 70 arrays.
Brand Category Distribution: {'UNIVERSAL': 28, 'EURO_US': 23, 'TOYOTA_DAIHATSU': 8, 'MITSUBISHI': 4, 'HONDA': 5, 'NISSAN': 2}
[PASS] Check 7: Brand categories are properly populated with OEM distributions.
[PASS] Check 8: No hardcoded query shortcuts, stubs, TODOs, or bypasses detected in source.
--------------------------------------------------------------------------------
FORENSIC AUDIT SUMMARY: 8/8 CHECKS PASSED
================================================================================
```

### 1.3 Adversarial Stress-Testing Results
Command executed: `python .agents/aud_m1/test_adversarial.py`
```text
================================================================================
                 ADVERSARIAL STRESS-TESTING: WHEEL DATABASE                     
================================================================================
[TEST 1] Null / Empty query inputs:
   -> PASS: Null & empty string inputs correctly handled without crash
[TEST 2] Case-insensitive resolution: All 140 uppercase/lowercase queries resolved correctly to unique wheel IDs.
[TEST 3] Category query with maxOut < count returns total count (70) without overflowing output buffer (5 items).
[TEST 4] Out-of-bounds indices [70, 71, 100, 255, 1000] correctly evaluate out-of-range (< 70 check) and safely return nullptr.
[TEST 5] Bogus / non-existent pattern queries ['Ferrari 458 V8', 'Lamborghini V10', 'NonExistentWheel999', '   ', '!@#$%^&*()'] correctly evaluate to nullptr.
--------------------------------------------------------------------------------
ADVERSARIAL TEST SUMMARY: 5/5 PASSED
================================================================================
```

### 1.4 PlatformIO Build Verification
Command executed: `pio run -e esp32s3`
```text
Processing esp32s3 (platform: espressif32; framework: arduino; board: esp32-s3-devkitc-1)
--------------------------------------------------------------------------------
RAM:   [==        ]  22.1% (used 72332 bytes from 327680 bytes)
Flash: [===       ]  28.2% (used 1033821 bytes from 3670016 bytes)
========================= [SUCCESS] Took 8.54 seconds =========================
```

### 1.5 4-Tier E2E Test Suite Results
Command executed: `python test/run_e2e_tests.py`
```text
================================================================================
                        ECUSNIFF E2E TEST SUMMARY
================================================================================
Tier 1: 791/791 tests passed [PASS]
Tier 2: 25/25 tests passed [PASS]
Tier 3: 15/15 tests passed [PASS]
Tier 4: 156/156 tests passed [PASS]
--------------------------------------------------------------------------------
TOTAL: 987/987 passed (0 failed)
================================================================================
```

---

## 2. Logic Chain

1. **PROGMEM Data Authenticity**:
   - Every single byte of all 70 bit-arrays in `lib/engine/src/wheel_database.cpp` was compared against the upstream ArduStim header `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h`.
   - All 15,429 bytes match 100% with 0 byte differences. No placeholder arrays, constant-filled dummy patterns, or shortened sequences exist.
2. **Lookup Function Integrity**:
   - `getWheel`, `getWheelById`, `findByFriendlyName`, `findByShortName`, and `getWheelsByCategory` implement authentic traversal and lookup algorithms over the master database table.
   - There are no hardcoded conditionals intercepting specific test inputs (e.g. `if (strcmp(name, "Toyota Avanza") == 0) return ...;`).
3. **No Facade or Bypass Patterns**:
   - All methods contain genuine computational logic with safety boundary guards.
   - Code inspection for markers such as `TODO`, `FIXME`, `NotImplemented`, or constant-return stubs returned 0 occurrences.
4. **Build and Runtime Safety**:
   - PlatformIO successfully compiled the firmware target `esp32s3` with 0 errors.
   - Stress-testing confirms that null pointers, empty strings, case variations, out-of-bounds indices, and undersized output buffers are handled safely without segmentation faults or buffer overflows.

---

## 3. Caveats
- Pattern index 20 retains a trailing whitespace in its `friendlyName` (`"Toyota Avanza/Xenia/Terios/Rush "`) matching ArduStim TFTv2 source verbatim. Lookup functions handle this seamlessly, and `shortName` provides the clean string `"Avanza/Xenia/Terios/Rush"`.
- Dual-cam patterns (`GM LS1` at index 27 and `BMW N20` at index 66) correctly assert `hasCmp2 = true` as verified by the active 0x04 bitmask check.

---

## 4. Conclusion
The Milestone 1 work product exhibits **100% genuine implementation fidelity**.
- 70/70 PROGMEM bit-arrays are fully authentic.
- Lookup functions are generic, robust, and free of bypasses.
- Clean compilation on PlatformIO `esp32s3`.
- **Verdict**: **CLEAN**.

---

## 5. Verification Method

To independently reproduce this forensic audit, run the following commands:

```bash
# 1. Run Python Bit-by-Bit Comparative Audit against ArduStim Source
python ".agents/aud_m1/verify_forensics.py"

# 2. Run Adversarial Stress-Testing
python ".agents/aud_m1/test_adversarial.py"

# 3. Run E2E 4-Tier Test Suite
python test/run_e2e_tests.py

# 4. Compile on PlatformIO for ESP32-S3
pio run -e esp32s3
```
