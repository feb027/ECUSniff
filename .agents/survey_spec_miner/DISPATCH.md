## 2026-09-01T09:52:29Z

You are the Spec Miner for ECUSniff Wheel Patterns Survey.
Your working directory is: g:\semester 7\ECUSniff\.agents\survey_spec_miner
Read ORIGINAL_REQUEST.md at: g:\semester 7\ECUSniff\.agents\ORIGINAL_REQUEST.md

Task:
Perform exhaustive analysis of the external sources:
1. `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` and any other files in `external/ardustim-tftv2-touchscreen/`
2. `external/pattern-gen/` (including any python/c generators or wheel data)
3. Enumerate ALL (~70) wheel definitions:
   - Index, friendly name (exact string in ArduStim), short name/identifier
   - Target brand category (Toyota, Honda, Mitsubishi, Nissan, Euro/US, Universal, Custom, etc.)
   - Pattern type (e.g. missing tooth, bit-array/segment array, multi-gap, dual-wheel, CAS)
   - Number of segments/teeth, cycle length (360 deg or 720 deg), CKP/CMP1/CMP2 bit definitions and layout
   - Detail critical patterns: New Avanza, Old Avanza, Avanza/Xenia/Terios/Rush, 4G63 (4/2), 6g72, 3A92, 36-2-2-2 (Crank+Cam, H4, H6), Honda Jazz/Fit (V1, V2, V3), D17, RC51, Nissan Livina/Juke, Nissan 360 CAS, 60-2, 36-1, etc.
4. Analyze how bitmasks/values work in ArduStim (e.g., bit 0: LOW, 1: CKP, 2: CMP1, 3: CKP+CMP1, 4: CMP2, etc. or specific bit definitions).

Output:
Write a comprehensive report to `g:\semester 7\ECUSniff\.agents\survey_spec_miner\spec_mining_report.md` and `handoff.md`.
Report when complete via send_message to parent.
