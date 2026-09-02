# ECU Pattern Designer & Timing Math Guide (Local Copy)

## 1. Engine 4-Stroke Cycle (0° – 720°)
- Cycle 1 (0° – 360°): Intake & Compression
- Cycle 2 (360° – 720°): Power & Exhaust
- Camshaft rotates 1:2 to Crankshaft: 1 Cam turn (360°) = 2 Crank turns (720°).

## 2. Fundamental Timing Formulas
1. 1 Crank Rev (360°): $T_{rev} = \frac{60 \times 10^6}{RPM}\ \mu s$
2. 1 Engine 4-Stroke Cycle (720°): $T_{cycle} = 2 \times T_{rev} = \frac{120 \times 10^6}{RPM}\ \mu s$
3. 1 Degree Angle: $T_{deg} = \frac{T_{rev}}{360} = \frac{166666.67}{RPM}\ \mu s/deg$

## 3. Crankshaft Pattern (CKP Missing Tooth)
- $N_{total}$ total theoretical teeth, $M_{missing}$ missing teeth
- Pitch angle: $\theta_{pitch} = 360.0^\circ / N_{total}$
- High duration: $\theta_{high} = \theta_{pitch} \times DutyCycle$
- Low duration: $\theta_{low} = \theta_{pitch} \times (1 - DutyCycle)$

## 4. Camshaft Pattern (CMP / CMP2 Event Table)
- Event Table over 0°–720°: `{ angle_deg, level (HIGH/LOW) }`

## 5. ESP32 RMT Hardware Rules for Looping
1. EOT Zero-Terminator Marker: `{ duration0 = 0, level0 = 0, duration1 = 0, level1 = 0 }`.
2. Clear remaining RAM buffer with zeros.
3. RMT memory allocation:
   - Channel 0 (CKP): 2 Blocks (128 slots)
   - Channel 2 (CMP1): 2 Blocks (128 slots)
   - Channel 4 (CMP2): 2 Blocks (128 slots)
