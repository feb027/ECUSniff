# Milestone 2 & Milestone 3 Adversarial Challenge Report

## 1. Observation
- **Test Harness Implemented**:
  - `test/test_adversarial_m2_m3.py`: Comprehensive adversarial stress harness covering 5 challenge dimensions across all 70 ArduStim wheel patterns.
  - `test/run_e2e_tests.py`: 4-Tier verification suite (T1: Feature coverage, T2: Boundary/corner, T3: Combinations, T4: OEM scenarios).
- **Tool Commands & Verification Results**:
  1. `python test/test_adversarial_m2_m3.py`:
     - Executed 679,355 discrete verification checks with **0 failures** (100% PASS).
     - **Challenge 1 (RMT Compilation Gamut)**: 1,830 multi-channel pulse compilations executed across 15 RPM steps ($10, 15, 25, 50, 100, 250, 500, 850, 1000, 2500, 4000, 6000, 8000, 10000, 12000\text{ RPM}$). All generated items fit within `MAX_CYCLE_PULSES = 512`, duration chunks $\le 30,000\ \mu\text{s}$, and valid EOT `{0, 0, 0, 0}` termination.
     - **Challenge 2 (Pulse Slicing & Conservation)**: 11,713 multi-chunk sliced pulses verified at low RPM ($10 - 100\text{ RPM}$). Sliced sub-phases strictly preserve logic level and sum precisely to the unsliced duration.
     - **Challenge 3 (Zero Cumulative Drift)**: Verified across all 70 presets and 7 RPM steps. Exact equation $\sum (d_0 + d_1) \equiv T_{\text{cycle}}$ holds with **$0\ \mu\text{s}$ drift**.
     - **Challenge 4 (Multi-Channel Phase Alignment)**: Time-series reconstruction on BMW N20 (Index 66) and GM LS1 (Index 27) demonstrated 100% exact microsecond segment phase alignment across CKP, CMP1, and CMP2.
     - **Challenge 5 (Waveform Canvas Geometry & Pixel Bounds)**: Tested across 6 screen resolutions ($456 \times 124$, $448 \times 76$, $400 \times 100$, $440 \times 80$, $320 \times 60$, $480 \times 240$). 420 rendering cycles executed with **0 out-of-bounds pixel writes** and non-overlapping track corridors.
  2. `python test/run_e2e_tests.py`:
     - Result: 987/987 passed (Tier 1: 791/791, Tier 2: 25/25, Tier 3: 15/15, Tier 4: 156/156, 0 failed).
  3. `pio run -e esp32s3`:
     - Result: `[SUCCESS]` in 10.80s.
     - RAM: 28.6% (93,852 bytes / 327,680 bytes)
     - Flash: 28.6% (1,049,313 bytes / 3,670,016 bytes)

## 2. Logic Chain
1. **RMT 15-Bit Limit & Chunk Slicing ($\le 30,000\ \mu\text{s}$)**:
   - Observation: ESP32-S3 RMT tick counter is 15-bit ($32,767\ \mu\text{s}$ max at $1\ \mu\text{s}$ resolution).
   - Logic: `RmtGenerator::compileBitArrayToRmt` enforces `MAX_RMT_DURATION_CHUNK = 30000`. By slicing runs $> 30,000\ \mu\text{s}$ into identical-level sub-phases, the hardware counter never overflows down to $10\text{ RPM}$, while preserving pulse edge placement.
2. **Zero Cumulative Drift via 64-Bit Integer Timestamps**:
   - Observation: $t_{\text{start}}[s] = \lfloor \frac{s \times T_{\text{cycle}}}{N} \rfloor$ and $t_{\text{end}}[s] = \lfloor \frac{(s+1) \times T_{\text{cycle}}}{N} \rfloor$.
   - Logic: Because segment durations are derived from absolute cycle start/end timestamps rather than accumulating rounded delta fractions, summing all pulse durations telescopes identically to $T_{\text{cycle}} - 0 = T_{\text{cycle}}$, guaranteeing $0\ \mu\text{s}$ drift across unlimited engine cycles.
3. **Multi-Channel Phase Synchronization**:
   - Observation: Dual-cam patterns (e.g. BMW N20, GM LS1) define bit 0 (CKP), bit 1 (CMP1), bit 2 (CMP2) in a single unified PROGMEM array.
   - Logic: Compiling all 3 channels using identical $T_{\text{cycle}}$ and segment counts guarantees that hardware TX start on CH0, CH2, and CH3 locks phase relations in real time.
4. **Waveform Canvas Dynamic Geometry & Bounds Protection**:
   - Observation: `WaveformCanvas` calculates dynamic header height ($14\text{ px}$ vs $12\text{ px}$), track height ($\lfloor \text{usableH} / N \rfloor$), and margins based on sprite height.
   - Logic: Pixel columns evaluate segments $s \in [\lfloor \frac{x \times N}{W} \rfloor, \lfloor \frac{(x+1) \times N}{W} \rfloor)$ and draw vertical step lines for high-density edge transitions. All drawing coordinates $(x, y)$ strictly satisfy $0 \le x < W$ and $0 \le y < H$.

## 3. Caveats
- No caveats. All 70 presets compile into RMT symbols and render on Waveform Canvas without timing drift, buffer overflow, or display artifacts.

## 4. Conclusion
Milestones M2 and M3 are **empirically validated and APPROVED**.
- RMT generator correctly compiles all 70 presets across 10 to 12,000 RPM.
- Slicing strictly guarantees $\le 30,000\ \mu\text{s}$ with zero cumulative drift ($0\ \mu\text{s}$ error).
- Synchronized multi-channel generation verified for CKP, CMP1, and CMP2.
- Waveform Canvas renders cleanly across all tested resolutions with 0 OOB writes.
- PlatformIO firmware compiles cleanly (`[SUCCESS]` in 10.80s).

**Verdict**: **APPROVE**

## 5. Verification Method
Execute the following verification commands to independently reproduce the empirical results:
```bash
# 1. Run the comprehensive adversarial stress harness (679,355 checks)
python test/test_adversarial_m2_m3.py

# 2. Run the 4-tier E2E regression test suite (987 checks)
python test/run_e2e_tests.py

# 3. Compile ESP32-S3 firmware build
pio run -e esp32s3
```
