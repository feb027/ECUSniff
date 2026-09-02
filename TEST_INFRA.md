# ECUSniff Test Infrastructure & Verification Architecture

**Document ID**: `ECUSNIFF-TEST-INFRA-001`  
**Target Platform**: ESP32-S3 (Wemos D1 R32 / ESP32-S3-DevKitC-1 N8R8) & Portable C++17 Desktop Oracle  
**Test Framework**: Unity (ThrowTheSwitch) + Python E2E Verification Oracle  
**Authoritative Reference Sources**:
1. `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` & `ardustim.ino`
2. `.agents/survey_spec_miner/parsed_wheels.json` & `spec_mining_report.md`
3. `PROJECT.md` Architecture & Interface Contracts

---

## 1. Executive Summary & Test Philosophy

ECUSniff is a mission-critical automotive engine signal simulator and ECU diagnostic tool. A timing error of even $10\ \mu\text{s}$ or an incorrect edge transition can cause an Engine Control Unit (ECU) to lose crankshaft-camshaft synchronization (sync loss), trigger engine check error codes (e.g. P0335/P0340), or fail to fire ignition coils and fuel injectors.

The test infrastructure employs an **Opaque-Box 4-Tier Strategy** with strict mathematical and program-oracle derivations:
- **No Mock Illusions**: All test cases validate real pulse train timings, bitmask transitions, and database records against the authoritative ArduStim source definitions.
- **Bi-Directional Verification**: Tests are runnable both on embedded ESP32-S3 firmware harnesses and on desktop test runners with zero external hardware dependencies.
- **Deterministic Oracles**: Every test expectation is mathematically derived from the fundamental engine rotation equation $T_{\text{seg}} = \frac{D \times 10^6}{6 \times E \times RPM}\ \mu\text{s}$.

---

## 2. 4-Tier Opaque-Box Test Architecture

```
+-----------------------------------------------------------------------------+
|                                  ECUSniff                                   |
|                         4-Tier Test Architecture                            |
+-----------------------------------------------------------------------------+
| Tier 1: Feature Coverage (>= 5 cases per feature)                           |
|  - All 70 Presets Accessible (Indices 0..69, ID mapping, count = 70)        |
|  - Friendly Name Exact Matching (Identical string tokens from ArduStim)     |
|  - Cycle Degrees Validity (Strictly 360 deg or 720 deg)                     |
|  - Edge Count Integrity (4 to 1080 segments match PROGMEM arrays)           |
|  - Bitmask Truth Table (CKP=0x01, CMP1=0x02, CMP2=0x04, No invalid bits)    |
+-----------------------------------------------------------------------------+
| Tier 2: Boundary & Corner Cases                                             |
|  - Empty / Null / Unknown Lookups (nullptr safety, gracefully handled)      |
|  - Boundary Edge Extremes (4 edges minimum up to 1080 edges maximum)        |
|  - RPM Range Extremes (10 RPM ultra-low to 12,000 RPM ultra-high, zero RPM) |
|  - Multi-Gap Sync Validation (36-2-2-2 H4/H6, 12-3, Lotus 36-1-1-1-1)       |
+-----------------------------------------------------------------------------+
| Tier 3: Cross-Feature Combinations                                          |
|  - Dual-Cam Sync (BMW N20 simultaneous CKP+CMP1+CMP2, GM LS1 24X + Cam2)    |
|  - 360 deg vs 720 deg Cycle Conversion & 2x Engine Cycle Periodic Replica   |
|  - Multi-Channel Demuxing (CKP / CMP1 / CMP2) & RLE Duration Compression   |
+-----------------------------------------------------------------------------+
| Tier 4: Real-World OEM Application Scenarios                                |
|  - Toyota Avanza 1.5 New Avanza (36-2 with single 365-420 deg Cam pulse)   |
|  - Toyota Avanza 1.3 Old Avanza (36-2 with 3x multi-pulse Cam)              |
|  - Toyota Avanza/Xenia/Terios/Rush (VVT-i dual revolution Cam pulses)       |
|  - Mitsubishi 4G63 4/2 CAS (Unequal 60 deg Crank & 55/90 deg Cam)          |
|  - Mitsubishi 6G72 (6 Crank & 4 Cam teeth with 85 deg Master TDC tooth)     |
|  - Mitsubishi 3A92 (1.2L 3-Cylinder Mirage Multi-Gap)                       |
|  - Honda Jazz / Fit GD3 V1, V2, V3 (12+1 Crank + 4-Cam Variants)            |
|  - Universal Bosch 60-2 (58 Active + 2 Missing Teeth at segments 116-119)   |
+-----------------------------------------------------------------------------+
```

---

## 3. Directory Layout & Test Suite Artifacts

```
g:/semester 7/ECUSniff/
|-- test/
|   |-- test_tier1_features.cpp       # Tier 1: Feature Coverage Unity Test Suite
|   |-- test_tier2_boundary.cpp       # Tier 2: Boundary & Corner Cases Unity Test Suite
|   |-- test_tier3_combinations.cpp   # Tier 3: Cross-Feature Combinations Unity Test Suite
|   |-- test_tier4_realworld.cpp      # Tier 4: Real-World OEM Scenarios Unity Test Suite
|   |-- test_main.cpp                 # Unified Runner executing all 4 Tiers
|   |-- test_wheel_data_oracle.h      # Embedded test fixture containing 70 presets data
|   |-- run_e2e_tests.py              # Automated Python E2E verification test suite
|-- TEST_INFRA.md                     # Test infrastructure specification (this document)
|-- TEST_READY.md                     # Test execution instructions & verification report
```

---

## 4. Mathematical Timing Oracle & Expected Value Derivation

### 4.1 Fundamental Time-Base Formula
For an engine running at target $RPM$, trigger wheel having total segments $E$, spanning cycle rotation $D$ degrees ($D \in \{360^\circ, 720^\circ\}$):

$$T_{\text{seg}} = \frac{D \times 10^6}{6 \times E \times RPM}\quad [\mu\text{s}]$$

### 4.2 Run-Length Encoding (RLE) Pulse Duration
When $N$ contiguous segments share identical channel state $L \in \{0, 1\}$:
$$T_{\text{pulse}} = \sum_{k=0}^{N-1} T_{\text{seg}} = N \times \frac{D \times 10^6}{6 \times E \times RPM}\quad [\mu\text{s}]$$

For hardware RMT transmission, any pulse exceeding maximum hardware chunk duration ($T_{\text{chunk\_max}} = 30,000\ \mu\text{s}$) is partitioned into chunks of $\le 30,000\ \mu\text{s}$ with identical level before transmitting the next transition.

### 4.3 Theoretical Verification Table:
| Engine Preset | Segments ($E$) | Cycle ($D$) | RPM | $T_{\text{seg}}$ ($\mu\text{s}$) | Crank High Pulse ($N=1$) | Missing Tooth Gap |
|---|---|---|---|---|---|---|
| **60-2 Crank Only** | 120 | 360° | 1,000 | 500.00 $\mu\text{s}$ | 500 $\mu\text{s}$ | 2,000 $\mu\text{s}$ (4 segs) |
| **60-2 Crank Only** | 120 | 360° | 6,000 | 83.33 $\mu\text{s}$ | 83.33 $\mu\text{s}$ | 333.33 $\mu\text{s}$ |
| **Toyota New Avanza** | 144 | 720° | 850 | 980.39 $\mu\text{s}$ | 980.39 $\mu\text{s}$ | 1,960.78 $\mu\text{s}$ (2 segs) |
| **Toyota New Avanza** | 144 | 720° | 3,000 | 277.78 $\mu\text{s}$ | 277.78 $\mu\text{s}$ | 555.56 $\mu\text{s}$ |
| **Mitsubishi 4G63** | 144 | 720° | 1,000 | 833.33 $\mu\text{s}$ | 10,000.0 $\mu\text{s}$ (12 segs = 60°) | N/A |
| **Audi 135-Tooth** | 1080 | 720° | 6,000 | 18.52 $\mu\text{s}$ | 18.52 $\mu\text{s}$ | N/A |

---

## 5. Test Execution Instructions

### 5.1 Python E2E Test Suite Execution
To run the automated E2E test suite covering all 70 patterns and all 4 tiers:
```bash
python test/run_e2e_tests.py
```

### 5.2 Embedded PlatformIO Compilation Verification
To verify firmware and test suite compilation for ESP32-S3:
```bash
pio run -e esp32s3
```

---

## 6. Pass Criteria & Quality Gates

1. **Preset Coverage**: 100% (70/70 presets tested and validated).
2. **Timing Fidelity**: Pulse durations must match theoretical calculations within $\pm 1\ \mu\text{s}$ rounding tolerance.
3. **Bitmask Compliance**: 100% of bit-array elements contain only bits `0x01` (CKP), `0x02` (CMP1), `0x04` (CMP2).
4. **PlatformIO Build**: Clean compilation on `esp32s3` environment with 0 errors.
