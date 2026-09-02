## 2026-09-01T10:09:32Z
Task (Milestone 2: ESP32-S3 RMT Generator & Engine Bit-Array Driver):
1. Upgrade `lib/hal/include/rmt_generator.h` and `lib/hal/src/rmt_generator.cpp` (and `lib/engine/src/parametric_pattern.cpp` / `lib/engine/include/parametric_pattern.h`):
   - Add support for arbitrary bit-array / segment-based patterns via `setWheelPattern(const WheelDefinition* wheel)` and `prepareBitArrayCycle`.
   - Calculate microsecond segment duration: T_seg = (cycleDegrees * 1,000,000) / (6 * totalEdges * rpm).
   - Convert bit-array (Bit 0: CKP, Bit 1: CMP1, Bit 2: CMP2) into RMT symbols (`rmt_item32_t`) via Run-Length Encoding (RLE).
   - Implement duration slicing (splitting pulses > 30,000 us into chunks <= 30,000 us) to prevent RMT 15-bit overflow at low RPM (down to 10 RPM).
   - Ensure RMT continuous looping with zero-terminator EOT `{0, 0, 0, 0}`.
   - Synchronize multi-channel outputs: CKP (CH0), CMP1 (CH3 or CH1), and CMP2 (CH2 for BMW N20 and GM LS1).
   - Replicate 360-degree crank patterns seamlessly (2x) when running in 720-degree engine cycle context.
2. Compile and verify with `pio run -e esp32s3`.
3. Run `python test/run_e2e_tests.py` to ensure all 987 tests pass.

Write ownership:
You own `lib/hal/src/rmt_generator.cpp`, `lib/hal/include/rmt_generator.h`, `lib/engine/src/parametric_pattern.cpp`, `lib/engine/include/parametric_pattern.h`. Do not edit UI files or test infrastructure files.
