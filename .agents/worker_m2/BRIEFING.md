# BRIEFING — 2026-09-01T17:16:00+07:00

## Mission
Milestone 2: Upgrade ESP32-S3 RMT multi-channel signal generator and Engine Bit-Array pulse conversion driver to support all 70 ArduStim patterns with high precision, continuous looping, duration slicing (<= 30,000 us), and multi-channel synchronization (CKP, CMP1, CMP2).

## 🔒 My Identity
- Archetype: worker
- Roles: implementer, qa, specialist
- Working directory: g:\semester 7\ECUSniff\.agents\worker_m2
- Original parent: 3238efb3-ee5d-4798-adf9-be5de9e2bccf
- Milestone: M2 - ESP32-S3 RMT Generator & Engine Bit-Array Driver

## 🔒 Key Constraints
- Write ownership: `lib/hal/src/rmt_generator.cpp`, `lib/hal/include/rmt_generator.h`, `lib/engine/src/parametric_pattern.cpp`, `lib/engine/include/parametric_pattern.h`.
- Do not edit UI files or test infrastructure files.
- Zero fake/dummy implementations. Maintain full real state and exact timing math.
- All 987 E2E tests must pass (`python test/run_e2e_tests.py`).
- PlatformIO build must succeed cleanly (`pio run -e esp32s3`).

## Current Parent
- Conversation ID: 3238efb3-ee5d-4798-adf9-be5de9e2bccf
- Updated: 2026-09-01T17:16:00+07:00

## Task Summary
- **What to build**:
  - Implement arbitrary bit-array conversion to RMT symbols via RLE in `RmtGenerator::compileBitArrayToRmt`, `prepareBitArrayCycle`, and `ParametricEngine::generateBitArrayCycle`.
  - Microsecond timing: T_seg = (cycleDegrees * 1,000,000) / (6 * totalEdges * rpm).
  - Duration chunk slicing <= 30,000 us preventing RMT 15-bit counter overflow (down to 10 RPM).
  - EOT zero-terminator `{0, 0, 0, 0}` for seamless hardware continuous loop.
  - Multi-channel output synchronization: CKP (CH0), CMP1 (CH2), CMP2 (CH3).
  - 360-degree crank wheel replication (2x) in 720-degree engine cycle context.
- **Success criteria**: Clean compilation with `pio run -e esp32s3`, 987/987 tests passing.

## Key Decisions Made
- Hardware channel mapping on ESP32-S3: CH0 (CKP, 2 memory blocks = 96 items), CH2 (CMP1, 1 memory block = 48 items), CH3 (CMP2, 1 memory block = 48 items) to avoid hardware memory block overlap.
- Double-buffering ping-pong buffers updated to `MAX_CYCLE_PULSES = 512` supporting all 70 ArduStim patterns down to 10 RPM.
- Integer 64-bit cumulative math ensures 0 microsecond drift over 720° engine cycles.

## Change Tracker
- **Files modified**:
  - `lib/engine/include/parametric_pattern.h`: Added `MAX_CYCLE_PULSES = 512`, `generateBitArrayCycle` declaration.
  - `lib/engine/src/parametric_pattern.cpp`: Implemented `generateBitArrayCycle` with 360-replication and RLE segment generation.
  - `lib/hal/include/rmt_generator.h`: Added `setWheelPattern()`, `prepareBitArrayCycle()`, `compileBitArrayToRmt()`, 3-channel double-buffers.
  - `lib/hal/src/rmt_generator.cpp`: Implemented 3-channel hardware initialization (CH0, CH2, CH3), `compileBitArrayToRmt` with RLE and <= 30,000 us duration slicing, EOT termination, synchronized start/stop/swapBuffer.
- **Build status**: PASS (`pio run -e esp32s3` - SUCCESS in 9.77s)
- **Pending issues**: None

## Quality Status
- **Build/test result**: 987/987 E2E tests passed (100% PASS), 32475 stress assertions passed (100% PASS).
- **Lint status**: Clean
- **Tests added/modified**: `scratch/verify_m2_implementation.py` (all checks passed)

## Loaded Skills
- **Source**: `g:\semester 7\ECUSniff\.agents\skills\ecu-pattern-designer\SKILL.md`
- **Core methodology**: Automotive 0-720 deg timing formulas, missing tooth wheel pitch/duty calculation, RMT continuous loop with EOT terminator, dual-cam phase synchronization.
