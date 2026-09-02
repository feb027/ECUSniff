# Milestone 1 Handoff Report: Wheel Pattern Database & Data Structures Porting

## 1. Observation
- **Files Created / Modified**:
  1. `lib/engine/include/wheel_database.h`: Master trigger wheel definitions and database querying interface. Defines `BrandCategory` (8 categories), `WheelCycleDegrees` (360° / 720°), `WheelDefinition` struct, and `WheelDatabase` namespace methods.
  2. `lib/engine/src/wheel_database.cpp`: Stores all **70 ArduStim wheel patterns (indices 0..69)** as raw `const uint8_t[] PROGMEM` bit-arrays, the master lookup table `s_wheelDatabase[70]`, and full implementations for:
     - `size_t getWheelCount()`
     - `const WheelDefinition* getWheel(size_t index)`
     - `const WheelDefinition* getWheelById(uint8_t id)`
     - `const WheelDefinition* findByFriendlyName(const char* name)`
     - `const WheelDefinition* findByShortName(const char* name)`
     - `size_t getWheelsByCategory(BrandCategory cat, const WheelDefinition** outWheels, size_t maxOut)`
     - `const char* getCategoryName(BrandCategory cat)`
  3. `lib/engine/include/pattern_types.h`: Defines standard signal channel bitmasks (`SIGNAL_BIT_CKP = 0x01`, `SIGNAL_BIT_CMP1 = 0x02`, `SIGNAL_BIT_CMP2 = 0x04`, `SIGNAL_BIT_KNOCK = 0x08`) and `PulseTransition` struct for timing conversions.
- **Pattern Completeness**: Exactly 70 presets (0..69) matching ArduStim TFTv2 Touchscreen (`external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` and `Wheels[]`).
- **Memory Footprint**: Total raw bit-array data stored in Flash PROGMEM is **15,429 bytes (~15.07 KB)** across all 70 presets.

## 2. Logic Chain
1. **Flash PROGMEM Optimization**:
   Trigger wheel patterns in ArduStim are discrete bit-arrays where each element represents the logic state of all channels (Bit 0: CKP, Bit 1: CMP1, Bit 2: CMP2). Placing all 70 arrays in Flash PROGMEM (`const uint8_t ... PROGMEM`) ensures 0 bytes of RAM overhead when idle.
2. **Exact Presets & Friendly Names Matching**:
   All 70 presets maintain verbatim string matching against ArduStim source definitions (e.g. `"Toyota Avanza 1.3 Crank only"`, `"Toyota Avanza 1.5 Crank only"`, `"Toyota Avanza/Xenia/Terios/Rush "`, `"Mitsubishi 4g63 aka 4/2 crank and cam"`, `"60-2 crank only"`, `"GM LS1 crank and cam"`, etc.).
3. **Brand Category Classification**:
   Preset categorization cleanly groups all 70 patterns into 8 enums:
   - `TOYOTA_DAIHATSU`: 8 presets (Old Avanza, New Avanza, Rush/Terios, 36-2-2-2 K3-3SZ, Daihatsu 3+1, 4A-GE CAS, 4A-GZE, 36-2+1T 2JZ).
   - `HONDA`: 5 presets (Honda RC51, Honda D17 12+1, Honda Jazz/Fit 04-08 V1, V2, V3).
   - `MITSUBISHI`: 4 presets (6G72, 4G63 4/2, DSM 420A, 3A92 3-Cyl).
   - `NISSAN`: 2 presets (Nissan Livina/Juke, Nissan 360 CAS).
   - `EURO_US`: 23 presets (GM, Ford, Chrysler NGC4/NGC6/NGC8, BMW N20, Audi 135T, Fiat 1.8 16V, Volvo D12, Weber-Marelli, Viper, etc.).
   - `UNIVERSAL`: 28 presets (60-2, 36-1, 24-1, 12-1, 8-1, 4-1, distributors, Suzuki Swift H4, Subaru 6/7, Mazda CAS, Yamaha R1, etc.).
   - `CUSTOM`: Reserved for user-created / sniffer-captured patterns.
4. **Fast Lookup & Case-Insensitive Fallback**:
   `findByFriendlyName()` and `findByShortName()` perform fast exact matching first (`strcmp`), falling back to case-insensitive matching (`stringEqualsIgnoreCase`) to maximize resilience when queried by UI or serial commands.

## 3. Caveats
- Pattern index 20 has a trailing space in the original ArduStim source string (`"Toyota Avanza/Xenia/Terios/Rush "`). This trailing space is preserved in `friendlyName` for 100% byte-for-byte fidelity with ArduStim, while `shortName` is cleanly set to `"Avanza/Xenia/Terios/Rush"`.
- Dual-cam patterns: `GM_LS1_CRANK_AND_CAM` (index 27) and `BMW_N20` (index 66) have `hasCmp2 = true` due to active Bit 2 (0x04) pulses.

## 4. Conclusion
Milestone 1 (Wheel Pattern Database & Data Structures Porting) is **100% complete and fully verified**. All 70 presets with exact ArduStim friendly names, short display names, OEM brand categories, cycle degrees, and PROGMEM bit-arrays are compiled and operational.

## 5. Verification Method & Output

### 5.1 PlatformIO Compilation Command
```bash
pio run -e esp32s3
```
**Compilation Result**:
```text
Processing esp32s3 (platform: espressif32; framework: arduino; board: esp32-s3-devkitc-1)
--------------------------------------------------------------------------------
HARDWARE: ESP32S3 240MHz, 320KB RAM, 8MB Flash
...
Compiling .pio\build\esp32s3\lib71b\engine\wheel_database.cpp.o
Archiving .pio\build\esp32s3\lib71b\libengine.a
Linking .pio\build\esp32s3\firmware.elf
Checking size .pio\build\esp32s3\firmware.elf
RAM:   [==        ]  22.1% (used 72332 bytes from 327680 bytes)
Flash: [===       ]  28.2% (used 1033821 bytes from 3670016 bytes)
Building .pio\build\esp32s3\firmware.bin
Merged 2 ELF sections
Successfully created esp32s3 image.
========================= [SUCCESS] Took 44.67 seconds =========================
```

### 5.2 Python Structural & Content Verification
```bash
python scratch/verify_m1_implementation.py
```
**Output**:
```text
--- Starting Worker M1 Comprehensive Verification ---
[PASS] All required header and source files exist.
[PASS] pattern_types.h contains all required bitmasks and PulseTransition struct.
[PASS] wheel_database.h exports full WheelDefinition struct and WheelDatabase API.
[PASS] All 70 PROGMEM arrays verified (15429 bytes).
[PASS] All 70 friendly names verified in master database table.
[PASS] All brand categories correctly mapped and utilized.
--- ALL VERIFICATIONS PASSED 100% ---
```

### 5.3 4-Tier E2E Verification Suite
```bash
python test/run_e2e_tests.py
```
**Output**:
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
