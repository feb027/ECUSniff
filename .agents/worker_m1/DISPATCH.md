## 2026-09-01T09:59:00Z
Task (Milestone 1: Wheel Pattern Database & Data Structures Porting):
1. Create/Update `lib/engine/include/wheel_database.h` and `lib/engine/src/wheel_database.cpp` (and `lib/engine/include/pattern_types.h` if needed).
2. Store all 70 ArduStim wheel patterns (indices 0..69) as PROGMEM arrays (`const uint8_t[]`) containing bit-arrays (Bit 0: CKP, Bit 1: CMP1, Bit 2: CMP2).
3. Implement `WheelDefinition` struct with `id`, `friendlyName` (exact ArduStim string), `shortName`, `category` (BrandCategory enum: ALL, TOYOTA_DAIHATSU, HONDA, MITSUBISHI, NISSAN, EURO_US, UNIVERSAL, CUSTOM), `cycleDegrees` (360 or 720), `totalEdges`, `bitArray`, `hasCmp1`, `hasCmp2`.
4. Provide fast lookup API in `WheelDatabase` namespace: `getWheelCount()`, `getWheel(index)`, `getWheelById(id)`, `findByFriendlyName(name)`, `getWheelsByCategory(category, outArray, maxOut)`, `getCategoryName(category)`.
5. Ensure `lib/ui/include/wheel_database.h` is compatible with or references `lib/engine/include/wheel_database.h`.
6. Verify code compiles cleanly by running `pio run -e esp32s3`. Document memory/flash usage in report.

Write ownership:
You own `lib/engine/include/wheel_database.h`, `lib/engine/src/wheel_database.cpp`, `lib/engine/include/pattern_types.h`. Do not edit `lib/hal/src/rmt_generator.cpp` or UI files yet.
