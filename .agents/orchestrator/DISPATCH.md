# Dispatch Log

## 2026-09-01T09:51:44Z
Execute the comprehensive research and porting of all (~70) crankshaft and camshaft tooth patterns from ArduStim TFTv2 Touchscreen (external/ardustim-tftv2-touchscreen) and Pattern Gen (external/pattern-gen) into the ECUSniff simulator engine.
Requirements:
1. R1: Synchronize database and ensure identical naming with ArduStim TFTv2 (~70 wheel defs), mapping brand categories properly.
2. R2: Arbitrary / bit-array pattern signal generation support in ESP32-S3 RMT generator (lib/hal/src/rmt_generator.cpp and lib/engine/) for 0-720 deg cycles without distortion or buffer underrun.
3. R3: Waveform Canvas rendering for arbitrary bit-array patterns (lib/ui/src/waveform_canvas.cpp) across CKP, CMP1, and CMP2.
4. R4: Automated unit tests in test/ validating edge transitions and timing against ArduStim source arrays (especially New Avanza, Old Avanza, Avanza/Xenia/Terios/Rush, 4G63, 60-2). Ensure PlatformIO compilation and tests pass cleanly.
