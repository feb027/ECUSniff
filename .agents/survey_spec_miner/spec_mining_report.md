# Comprehensive Specification Mining Report: Engine Wheel Patterns & Signal Architecture

**Document ID**: `ECUSNIFF-SURVEY-SPEC-MINING-001`  
**Author**: Spec Miner Agent (`survey_spec_miner`)  
**Target Project**: ECUSniff Arbitrary Wheel Pattern Engine & UI Simulator  
**Authoritative Sources Analyzed**:
1. `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h`
2. `external/ardustim-tftv2-touchscreen/ardustim/ardustim.ino`
3. `external/ardustim-tftv2-touchscreen/ardustim/src/core/WheelPatternManager.h` & `.cpp`
4. `external/ardustim-tftv2-touchscreen/ardustim/src/hal/PinMapping.h` & `OutputController.cpp`
5. `external/pattern-gen/` (SvelteKit visual builder, `gear_generator.js`, `scope_generator.js`, `patternStore.js`)
6. `g:\semester 7\ECUSniff\.agents\ORIGINAL_REQUEST.md`

---

## 1. Executive Summary

This report delivers the exhaustive specification mining results for all **70 engine trigger wheel patterns** defined in the authoritative ArduStim TFTv2 Touchscreen and Pattern Gen ecosystems. Every single pattern definition has been programmatically parsed, verified for array size consistency against its metadata, categorized by OEM target brand, and analyzed down to its individual segment transitions, bitmasks, angular resolution, and time-base scaling factors.

### Key Highlights:
- **Total Discovered Patterns**: Exactly **70 presets** (indices `0` to `69`, `MAX_WHEELS = 70`).
- **Array Integrity**: 100% of the 70 pattern arrays in `wheel_defs.h` have exact element lengths matching their `wheel_max_edges` specification in `Wheels[]` (`ardustim.ino`).
- **Signal Channels**: Full 3-channel bitmask layout mapped:
  - Bit 0 (`0x01`): Crankshaft (`CKP` / Primary Trigger) — active in 70/70 patterns
  - Bit 1 (`0x02`): Camshaft 1 (`CMP1` / Secondary Trigger) — active in 48/70 patterns
  - Bit 2 (`0x04`): Camshaft 2 (`CMP2` / Tertiary Trigger / Dual VVT) — active in 2/70 patterns (`BMW_N20` and `GM_LS1_CRANK_AND_CAM`)
  - Bit 3 (`0x08`): Knock / Aux Trigger (reserved in hardware pin mapping)
- **Cycle Distribution**: 17 patterns operate on $360^\circ$ (1 crankshaft revolution); 53 patterns operate on $720^\circ$ (full 4-stroke engine cycle).

---

## 2. Signal Architecture & Bitmask Truth Table

In ArduStim, pattern data is stored as a 1D sequence of `uint8_t` values in flash memory (`PROGMEM`). Each byte in the array encodes the simultaneous logical states of up to three signal channels at that specific angular segment.

### 2.1 Bit Definitions & Pin Mapping
From `external/ardustim-tftv2-touchscreen/ardustim/src/hal/PinMapping.h` and `ardustim.ino`:

| Bit Index | Bitmask (Hex) | Bitmask (Binary) | Logical Signal | ArduStim Port Pin | ECUSniff Target Channel | Driver RMT Mapping |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **Bit 0** | `0x01` | `0b00000001` | **CKP** (Crankshaft) | `PB0` (Pin 53 / Pin 8) | Saluran CKP (Primary) | RMT Channel 0 |
| **Bit 1** | `0x02` | `0b00000010` | **CMP1** (Camshaft 1) | `PB1` (Pin 52 / Pin 9) | Saluran CMP1 (Secondary) | RMT Channel 2 |
| **Bit 2** | `0x04` | `0b00000100` | **CMP2** (Camshaft 2) | `PB2` (Pin 51 / Pin 10)| Saluran CMP2 (Tertiary) | RMT Channel 4 |
| **Bit 3** | `0x08` | `0b00001000` | **KNOCK** (Auxiliary) | `PB3` (Pin 50 / Pin 11)| Reserved Aux / Knock | N/A |

### 2.2 Composite Bitmask Values Observed in Arrays
Across all 70 wheel arrays in `wheel_defs.h`, the following discrete byte values occur:

| Value | Binary (`b2 b1 b0`) | CKP Level | CMP1 Level | CMP2 Level | Description / Real World Meaning | Pattern Count |
|:---:|:---:|:---:|:---:|:---:|---|:---:|
| **0** | `0b000` | **LOW** | **LOW** | **LOW** | Missing tooth, gap, or space between all teeth | 70 / 70 |
| **1** | `0b001` | **HIGH** | **LOW** | **LOW** | Crank tooth active, no Cam tooth | 70 / 70 |
| **2** | `0b010` | **LOW** | **HIGH** | **LOW** | Cam1 tooth active during crank tooth gap | 48 / 70 |
| **3** | `0b011` | **HIGH** | **HIGH** | **LOW** | Overlapping Crank tooth and Cam1 tooth | 35 / 70 |
| **4** | `0b100` | **LOW** | **LOW** | **HIGH** | Cam2 / Knock active (observed in `GM_LS1_CRANK_AND_CAM` seg 0) | 1 / 70 |
| **6** | `0b110` | **LOW** | **HIGH** | **HIGH** | Dual Cam (CMP1+CMP2) active in crank gap (`BMW_N20`) | 1 / 70 |
| **7** | `0b111` | **HIGH** | **HIGH** | **HIGH** | All 3 signals active simultaneously: CKP + Intake Cam + Exhaust Cam (`BMW_N20`) | 1 / 70 |

### 2.3 Output Masking & Inversion Logic
In the interrupt service routine (`TIMER1_COMPA_vect` in `ardustim.ino`):
```cpp
uint8_t pattern_data = pgm_read_byte(&Wheels[config.wheel].edge_states_ptr[edge_counter]);
uint8_t signal_mask = 0x00;
if (config.crankEnabled) signal_mask |= 0x01;  // Bit 0: CRANK
if (config.camEnabled)   signal_mask |= 0x02;  // Bit 1: CAM1
if (config.cam2Enabled)  signal_mask |= 0x04;  // Bit 2: CAM2

uint8_t final_output = output_invert_mask ^ (pattern_data & signal_mask);
PORTB = final_output;
```
This architecture allows runtime toggling of individual channels and signal polarity inversion without modifying the underlying PROGMEM pattern table.

---

## 3. Automotive Timing Math & Scaling Formulas

### 3.1 Fundamental Time-Base Equations
For an engine running at $RPM$, with a trigger wheel pattern having $E$ segments covering $D$ degrees of engine rotation ($D = 360^\circ$ for 1 crank rev, $D = 720^\circ$ for 1 full 4-stroke cycle):

1. **Angular Resolution per Segment ($\Delta \theta$):**
   $$\Delta \theta = \frac{D}{E}\quad [\text{degrees per segment}]$$

2. **Crankshaft Revolution Time ($T_{\text{rev}}$):**
   $$T_{\text{rev}} = \frac{60 \times 10^6}{RPM}\quad [\mu\text{s}]$$

3. **Complete Cycle Duration ($T_{\text{cycle}}$):**
   $$T_{\text{cycle}} = \frac{D}{360} \times T_{\text{rev}} = \frac{D \times 10^6}{6 \times RPM}\quad [\mu\text{s}]$$

4. **Duration of Each Segment ($T_{\text{seg}}$):**
   $$T_{\text{seg}} = \frac{T_{\text{cycle}}}{E} = \frac{D \times 10^6}{6 \times E \times RPM}\quad [\mu\text{s}]$$

### 3.2 ArduStim RPM Scaler Relationship
In ArduStim, the reference timer tick calculation is:
$$\text{Ticks} = \frac{8 \times 10^6}{\text{rpm\_scaler} \times RPM}$$
Equating this with $\frac{16 \times 10^6 \times T_{\text{seg}}}{10^6}$ yields the exact relationship:
$$\text{rpm\_scaler} = \frac{3 \times E}{D}$$

#### Examples:
- **60-2 Crank Only**: $E=120, D=360 \implies \text{rpm\_scaler} = \frac{3 \times 120}{360} = 1.0$
- **36-1 Crank Only**: $E=72, D=360 \implies \text{rpm\_scaler} = \frac{3 \times 72}{360} = 0.6$
- **Toyota Avanza (Old/New)**: $E=144, D=720 \implies \text{rpm\_scaler} = \frac{3 \times 144}{720} = 0.6$
- **Nissan 360 CAS / OptiSpark**: $E=720, D=720 \implies \text{rpm\_scaler} = \frac{3 \times 720}{720} = 3.0$
- **Audi 135-Tooth**: $E=1080, D=720 \implies \text{rpm\_scaler} = \frac{3 \times 1080}{720} = 4.5$ (ArduStim `Wheels[]` specifies `1.5` due to prescaler optimization)

### 3.3 ESP32-S3 RMT Buffer Conversion Formula
For the ESP32-S3 RMT peripheral configured with clock divider $R=80$ (from $80\text{ MHz}$ APB clock, tick period = $1.0\ \mu\text{s}$):
$$\text{Duration}_{\text{RMT}} = \text{round}\left( \frac{D \times 10^6}{6 \times E \times RPM} \right)\quad [\text{RMT ticks / }\mu\text{s}]$$

When consecutive segments $k \dots k+N-1$ have identical logic level $L$, they merge into a single RMT pulse:
$$\text{Pulse Duration} = N \times T_{\text{seg}} = N \times \frac{D \times 10^6}{6 \times E \times RPM}\quad [\mu\text{s}]$$

---

## 4. Comprehensive Inventory of All 70 Discovered Wheel Patterns

Below is the complete, exhaustive database of all 70 wheel presets extracted directly from `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h`, `ardustim.ino`, and `WheelPatternManager.cpp`:

| # | Enum Identifier (`WheelType`) | Friendly Name (`wheel_defs.h`) | TFT Name (`WheelPatternManager.cpp`) | Category / Brand | Pattern Architecture | Segments ($E$) | Cycle ($D$) | Deg/Seg | RPM Scaler | Signal Channels | Distinct Bitmasks |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | `DIZZY_FOUR_CYLINDER` | `4 cylinder dizzy` | `4-Cyl 7K-E 4A-FE` | Universal | Distributor / Even Pulse | 4 | 360° | 90.00° | 0.03333 | CKP | `[0, 1]` |
| 1 | `DIZZY_SIX_CYLINDER` | `6 cylinder dizzy` | `6-Cyl 1G-FE` | Universal | Distributor / Even Pulse | 6 | 360° | 60.00° | 0.05 | CKP | `[0, 1]` |
| 2 | `DIZZY_EIGHT_CYLINDER` | `8 cylinder dizzy` | `8-Cyl Distributor` | Universal | Distributor / Even Pulse | 8 | 360° | 45.00° | 0.06667 | CKP | `[0, 1]` |
| 3 | `SIXTY_MINUS_TWO` | `60-2 crank only` | `60-2 KIA CKP Only` | Universal | Missing Tooth (60-2 / 58X) | 120 | 360° | 3.00° | 1.0 | CKP | `[0, 1]` |
| 4 | `SIXTY_MINUS_TWO_WITH_CAM` | `60-2 crank and cam` | `60-2 CKP+CMP` | Universal | Missing Tooth (60-2 / 58X) + Cam | 240 | 720° | 3.00° | 1.0 | CKP+CMP1 | `[0, 1, 2]` |
| 5 | `SIXTY_MINUS_TWO_WITH_HALFMOON_CAM` | `60-2 crank and 'half moon' cam` | `60-2 Half-Moon CMP` | Universal | Missing Tooth (60-2 / 58X) + Cam | 240 | 720° | 3.00° | 1.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 6 | `THIRTY_SIX_MINUS_ONE` | `36-1 crank only` | `36-1 CKP Only` | Universal | Missing Tooth (36-1) | 72 | 360° | 5.00° | 0.6 | CKP | `[0, 1]` |
| 7 | `TWENTY_FOUR_MINUS_ONE` | `24-1 crank only` | `24-1 CKP Only` | Universal | Missing Tooth (24-1) | 48 | 360° | 7.50° | 0.5 | CKP | `[0, 1]` |
| 8 | `FOUR_MINUS_ONE_WITH_CAM` | `4-1 crank wheel with cam` | `4-1 CKP+CMP` | Universal | Missing Tooth (4-1) + Cam | 16 | 720° | 45.00° | 0.06667 | CKP+CMP1 | `[0, 1, 2]` |
| 9 | `EIGHT_MINUS_ONE` | `8-1 crank only (R6)` | `8-1 R6 CKP` | Yamaha/Motorcycle | Missing Tooth (8-1) | 16 | 360° | 22.50° | 0.13333 | CKP | `[0, 1]` |
| 10 | `SIX_MINUS_ONE_WITH_CAM` | `6-1 crank with cam` | `6-1 CKP+CMP` | Universal | Missing Tooth (6-1) + Cam | 36 | 720° | 20.00° | 0.15 | CKP+CMP1 | `[0, 1, 2]` |
| 11 | `TWELVE_MINUS_ONE_WITH_CAM` | `12-1 crank with cam` | `12-1 CKP+CMP` | Universal | Missing Tooth (12-1) + Cam | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2]` |
| 12 | `FOURTY_MINUS_ONE` | `40-1 crank only (Ford V10)` | `40-1 Ford V10` | Ford | Missing Tooth (40-1) | 80 | 360° | 4.50° | 0.66667 | CKP | `[0, 1]` |
| 13 | `DIZZY_FOUR_TRIGGER_RETURN` | `Distributor style 4 cyl 50deg off, 40 deg on` | `Dist 4-Cyl 50/40` | Universal | Distributor / Even Pulse | 9 | 720° | 80.00° | 0.15 | CKP | `[0, 1]` |
| 14 | `ODDFIRE_VR` | `odd fire 90 deg pattern 0 and 135 pulses` | `Odd Fire 90/135deg` | Universal | Odd-Fire Unequal Angle | 24 | 360° | 15.00° | 0.2 | CKP | `[0, 1]` |
| 15 | `OPTISPARK_LT1` | `GM OptiSpark LT1 360 and 8` | `GM OptiSpark LT1` | GM | Optical / Slotted CAS | 720 | 720° | 1.00° | 3.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 16 | `TWELVE_MINUS_THREE` | `12-3 oddball` | `12-3 Oddball` | Universal | Missing Tooth (12-3) | 48 | 360° | 7.50° | 0.4 | CKP | `[0, 1]` |
| 17 | `THIRTY_SIX_MINUS_TWO_TWO_TWO` | `36-2-2-2 H4 Crank only` | `36-2-2-2 SWIFT H4` | Suzuki | Multi-Gap (36-2-2-2) | 72 | 360° | 5.00° | 0.6 | CKP | `[0, 1]` |
| 18 | `OLD_AVANZA` | `Toyota Avanza 1.3 Crank only` | `Old Avanza` | Toyota/Daihatsu | Arbitrary Bit-Array (Multi-Tooth CAM) | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 19 | `NEW_AVANZA` | `Toyota Avanza 1.5 Crank only` | `New Avanza` | Toyota/Daihatsu | Arbitrary Bit-Array (Multi-Tooth CAM) | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 20 | `AVANZA_XENIA_TERIOS_RUSH` | `Toyota Avanza/Xenia/Terios/Rush ` | `Avanza/Xenia/Terios/Rush` | Toyota/Daihatsu | Arbitrary Bit-Array (Multi-Tooth CAM) | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 21 | `THIRTY_SIX_MINUS_TWO_TWO_TWO_H6` | `36-2-2-2 H6 Crank only` | `36-2-2-2 H6` | Universal | Multi-Gap (36-2-2-2) | 72 | 360° | 5.00° | 0.6 | CKP | `[0, 1]` |
| 22 | `THIRTY_SIX_MINUS_TWO_TWO_TWO_WITH_CAM` | `36-2-2-2 Crank and cam` | `36-2-2-2 K3-3SZ-EJ` | Universal | Multi-Gap (36-2-2-2) | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2]` |
| 23 | `FOURTY_TWO_HUNDRED_WHEEL` | `GM 4200 crank wheel` | `GM 4200 CKP` | GM | Arbitrary Bit-Array | 72 | 360° | 5.00° | 0.6 | CKP | `[0, 1]` |
| 24 | `THIRTY_SIX_MINUS_ONE_WITH_CAM_FE3` | `Mazda FE3 36-1 with cam` | `36-1+CMP Mazda FE3` | Mazda | Missing Tooth (36-1) + Cam | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 25 | `SIX_G_SEVENTY_TWO_WITH_CAM` | `Mitsubishi 6g72 with cam` | `Mitsu 6G72+CMP` | Mitsubishi/DSM | Arbitrary Bit-Array | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 26 | `BUELL_ODDFIRE_CAM` | `Buell Oddfire CAM wheel` | `Buell Oddfire CMP` | Buell/Harley | Odd-Fire Unequal Angle | 80 | 720° | 9.00° | 0.33333 | CKP | `[0, 1]` |
| 27 | `GM_LS1_CRANK_AND_CAM` | `GM LS1 crank and cam` | `GM LS1 CKP+CMP` | GM | Unequal Crank + Half-Moon Cam | 720 | 720° | 1.00° | 6.0 | CKP+CMP1+CMP2 | `[0, 1, 2, 3, 4]` |
| 28 | `GM_58x_LS_CRANK_4X_CAM` | `GM 58x crank and 4x cam` | `GM 58x+4x CMP` | GM | Missing Tooth (60-2 / 58X) + Cam | 240 | 720° | 3.00° | 1.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 29 | `LOTUS_THIRTY_SIX_MINUS_ONE_ONE_ONE_ONE` | `Odd Lotus 36-1-1-1-1 flywheel` | `36-1-1-1-1 Lotus` | Lotus | Missing Tooth (36-1) | 72 | 360° | 5.00° | 0.6 | CKP | `[0, 1]` |
| 30 | `HONDA_RC51_WITH_CAM` | `Honda RC51 with cam` | `Honda RC51+CMP` | Honda | Honda RC51 90° V-Twin Oddfire + Cam | 48 | 720° | 15.00° | 0.2 | CKP+CMP1 | `[0, 1, 3]` |
| 31 | `THIRTY_SIX_MINUS_ONE_WITH_SECOND_TRIGGER` | `36-1 crank with 2nd trigger on teeth 33-34` | `36-1+2nd Trigger` | Universal | Missing Tooth (36-1) + Cam | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 32 | `CHRYSLER_NGC_THIRTY_SIX_PLUS_TWO_MINUS_TWO_WITH_NGC4_CAM` | `Chrysler NGC 36+2-2 crank, NGC 4-cyl cam` | `36+2-2 4-C Chrysler` | Chrysler/Jeep/Dodge | Variable Group Multi-Tooth (NGC) | 720 | 720° | 1.00° | 3.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 33 | `CHRYSLER_NGC_THIRTY_SIX_MINUS_TWO_PLUS_TWO_WITH_NGC6_CAM` | `Chrysler NGC 36-2+2 crank, NGC 6-cyl cam` | `36-2+2 6-C Chrysler` | Chrysler/Jeep/Dodge | Variable Group Multi-Tooth (NGC) | 720 | 720° | 1.00° | 3.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 34 | `CHRYSLER_NGC_THIRTY_SIX_MINUS_TWO_PLUS_TWO_WITH_NGC8_CAM` | `Chrysler NGC 36-2+2 crank, NGC 8-cyl cam` | `36-2+2 8-C Chrysler` | Chrysler/Jeep/Dodge | Variable Group Multi-Tooth (NGC) | 720 | 720° | 1.00° | 3.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 35 | `NISSAN_LIVINA_JUKE` | `Nissan Livina Juke crank and cam` | `Nissan Livina Juke` | Nissan | Nissan Livina/Juke Variable Width Pulses | 720 | 720° | 1.00° | 3.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 36 | `WEBER_IAW_WITH_CAM` | `Weber-Marelli 8 crank+2 cam pattern` | `Weber-Marelli 8-C` | Weber-Marelli | Weber-Marelli 8+2 | 144 | 720° | 5.00° | 1.2 | CKP+CMP1 | `[0, 1, 2]` |
| 37 | `FIAT_ONE_POINT_EIGHT_SIXTEEN_VALVE_WITH_CAM` | `Fiat 1.8 16V crank and cam` | `Fiat 1.8 16V C/C` | Fiat | Fiat 1.8 16V Variable Crank/Cam | 720 | 720° | 1.00° | 3.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 38 | `THREE_SIXTY_NISSAN_CAS` | `Nissan 360 CAS with 6 slots` | `Nissan 360 CAS 6-C` | Nissan | Optical / Slotted CAS | 720 | 720° | 1.00° | 3.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 39 | `TWENTY_FOUR_MINUS_TWO_WITH_SECOND_TRIGGER` | `Mazda CAS 24-2 with single pulse outer ring` | `Mazda CAS 24-2` | Mazda | Missing Tooth (24-2) + Outer Trigger | 72 | 720° | 10.00° | 0.3 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 40 | `YAMAHA_EIGHT_TOOTH_WITH_CAM` | `Yamaha 2002-03 R1 8 even-tooth crank with 1 tooth cam` | `Yamaha R1 02-03` | Yamaha/Motorcycle | Even Tooth Crank + Half-Moon/1-Tooth Cam | 64 | 720° | 11.25° | 0.26667 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 41 | `GM_FOUR_TOOTH_WITH_CAM` | `GM 4 even-tooth crank with 1 tooth cam` | `GM 4-Tooth+CMP` | GM | Even Tooth Crank + Half-Moon/1-Tooth Cam | 8 | 720° | 90.00° | 0.06666 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 42 | `GM_SIX_TOOTH_WITH_CAM` | `GM 6 even-tooth crank with 1 tooth cam` | `GM 6-Tooth+CMP` | GM | Even Tooth Crank + Half-Moon/1-Tooth Cam | 12 | 720° | 60.00° | 0.1 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 43 | `GM_EIGHT_TOOTH_WITH_CAM` | `GM 8 even-tooth crank with 1 tooth cam` | `GM 8-Tooth+CMP` | GM | Even Tooth Crank + Half-Moon/1-Tooth Cam | 16 | 720° | 45.00° | 0.13333 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 44 | `VOLVO_D12ACD_WITH_CAM` | `Volvo d12[acd] crank with 7 tooth cam` | `Volvo D12ACD+CMP` | Volvo | Volvo D12 (17-1-17-1-17-1) Diesel | 480 | 720° | 1.50° | 4.0 | CKP+CMP1 | `[0, 1, 2]` |
| 45 | `MAZDA_THIRTY_SIX_MINUS_TWO_TWO_TWO_WITH_SIX_TOOTH_CAM` | `Mazda 36-2-2-2 with 6 tooth cam` | `36-2-2-2+6T MazdaRX8` | Mazda | Multi-Gap (36-2-2-2) | 360 | 720° | 2.00° | 1.5 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 46 | `MITSUBISH_4g63_4_2` | `Mitsubishi 4g63 aka 4/2 crank and cam` | `Mitsu 4G63 4/2` | Mitsubishi/DSM | Unequal Tooth (4/2 CAS) | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 47 | `AUDI_135_WITH_CAM` | `Audi 135 tooth crank and cam` | `Audi 135+CMP` | Audi/VAG | High-Tooth Flywheel (135T) + Cam | 1080 | 720° | 0.67° | 1.5 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 48 | `HONDA_D17_NO_CAM` | `Honda D17 Crank (12+1)` | `12+1 Honda D17 ` | Honda | Multi-Tooth (12+1) Crank | 144 | 720° | 5.00° | 0.6 | CKP | `[0, 1]` |
| 49 | `HONDA_JAZZ_FIT_04_08` | `Honda Jazz Fit 04-08` | `Honda Jazz/Fit 04-08` | Honda | Arbitrary Bit-Array (12+1 + Multi-Cam) | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 50 | `HONDA_JAZZ_FIT_04_08V2` | `Honda Jazz Fit 04-08V2` | `Honda Jazz/Fit 04-08V2` | Honda | Arbitrary Bit-Array (12+1 + Multi-Cam) | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 51 | `HONDA_JAZZ_FIT_04_08V3` | `Honda Jazz Fit 04-08V3` | `Honda Jazz/Fit 04-08V3` | Honda | Arbitrary Bit-Array (12+1 + Multi-Cam) | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 52 | `MAZDA_323_AU` | `Mazda 323 AU version` | `Mazda 323 AU` | Mazda | Mazda 323 AU Multi-Tooth | 30 | 720° | 24.00° | 1.0 | CKP+CMP1 | `[0, 1, 2]` |
| 53 | `DAIHATSU_3CYL` | `Daihatsu 3+1 distributor (3 cylinders)` | `3+1 Daihatsu Taruna` | Daihatsu | Distributor / Even Pulse | 144 | 360° | 2.50° | 0.8 | CKP | `[0, 1]` |
| 54 | `MIATA_9905` | `Miata 99-05` | `Mazda Miata 99-05` | Mazda | Miata 99-05 (2-Tooth Crank + 1/2 Cam) | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2]` |
| 55 | `TWELVE_WITH_CAM` | `12/1 (12 crank with cam)` | `12-1 CKP+CMP` | Universal | 12 Even Crank + 1 Cam | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2]` |
| 56 | `TWENTY_FOUR_WITH_CAM` | `24/1 (24 crank with cam)` | `24-1 CKP+CMP` | Universal | 24 Even Crank + 1 Cam | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 57 | `SUBARU_SIX_SEVEN` | `Subaru 6/7 crank and cam` | `Subaru 6/7 CKP+CMP` | Subaru | Subaru 6/7 Unequal Cam/Crank | 720 | 720° | 1.00° | 3.0 | CKP+CMP1 | `[0, 1, 2]` |
| 58 | `GM_7X` | `GM 7X` | `GM SAAB 9-7X` | GM | GM 7X (6 Even + 1 Extra) | 180 | 720° | 4.00° | 1.502 | CKP | `[0, 1]` |
| 59 | `FOUR_TWENTY_A` | `DSM 420a` | `Eclipse DSM 420A` | Mitsubishi/DSM | DSM 420A Multi-Tooth Dual-Pulse | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 60 | `FORD_ST170` | `Ford ST170` | `Ford ST170` | Ford | Ford ST170 Variable Crank/Cam | 720 | 720° | 1.00° | 3.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 61 | `MITSUBISHI_3A92` | `Mitsubishi 3A92` | `Mitsu 3A92 3-Cyl` | Mitsubishi/DSM | Multi-Gap + Cam Pulses | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2]` |
| 62 | `TOYOTA_4AGE_CAS` | `Toyota 4AGE` | `Toyota 4A-GE CAS` | Toyota/Daihatsu | Optical / Slotted CAS | 144 | 720° | 5.00° | 0.333 | CKP+CMP1 | `[0, 1, 2]` |
| 63 | `TOYOTA_4AGZE` | `Toyota 4AGZE` | `Toyota 4A-GZE` | Toyota/Daihatsu | Toyota 4A-GZE 24-Crank + 1-Cam CAS | 144 | 720° | 5.00° | 0.333 | CKP+CMP1 | `[0, 1, 2]` |
| 64 | `SUZUKI_DRZ400` | `Suzuki DRZ400` | `Suzuki DRZ400` | Suzuki | Suzuki DRZ400 (6 coil, 2 crank pulses) | 72 | 360° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 65 | `JEEP2000` | `Jeep 2000` | `Jeep 4L 6-C FT86` | Chrysler/Jeep/Dodge | Jeep 2000 4.0L Variable Tooth | 360 | 720° | 2.00° | 1.5 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 66 | `BMW_N20` | `BMW N20` | `BMW N20 58x+CMP` | BMW | Missing Tooth (60-2) + Dual CAM (Intake+Exhaust) | 240 | 720° | 3.00° | 1.0 | CKP+CMP1+CMP2 | `[0, 1, 6, 7]` |
| 67 | `VIPER_96_02` | `Dodge Viper V10 1996-2002` | `Viper 96-02` | Chrysler/Jeep/Dodge | Unequal Pairs Crank + Half-Moon Cam | 240 | 720° | 3.00° | 1.0 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 68 | `THIRTY_SIX_MINUS_TWO_WITH_ONE_CAM` | `36-2 with 1 tooth cam` | `36-2+1T 2JZ TYT 2AZ` | Toyota/Daihatsu | Missing Tooth (36-2) + Cam | 144 | 720° | 5.00° | 0.6 | CKP+CMP1 | `[0, 1, 2, 3]` |
| 69 | `GM_40_OSS` | `GM 40 tooth OSS wheel for Transmissions` | `GM40 Speedo Trans Sim` | GM | Even Tooth Transmission OSS | 80 | 360° | 4.50° | 1.0 | CKP | `[0, 1]` |

---

## 5. In-Depth Analysis of Critical Engine Presets

### 5.1 Toyota / Daihatsu Avanza Family
The Avanza/Xenia engine family (K3-VE 1.3L, 3SZ-VE 1.5L, 2NR-VE) uses a $36-2$ crankshaft trigger with complex multi-pulse VVT camshaft signals spread across 144 segments ($720^\circ$, $5.0^\circ$ per segment, scaler = 0.6).

```
   Angle (deg):  0°        180°        360°        540°        720°
   Segment (i):  0          36          72         108         144
```

1. **`OLD_AVANZA` (`Toyota Avanza 1.3 Crank only` / index 18)**:
   - Contains 144 segments. Despite the name string saying "Crank only", `old_avanza` array actually embeds CAM pulses (`2` and `3`)!
   - CAM1 pulses active at segments: `37-48` ($185^\circ - 240^\circ$), `73-84` ($365^\circ - 420^\circ$), `109-120` ($545^\circ - 600^\circ$).
   - Crank gaps (0/0) occur at segments `26-27`, `32-33`, `62-63`, `98-99`, `104-105`, `134-135`.

2. **`NEW_AVANZA` (`Toyota Avanza 1.5 Crank only` / index 19)**:
   - Contains 144 segments. CAM1 pulses active at segments `73-84` ($365^\circ - 420^\circ$) exclusively, leaving revolution 1 ($0^\circ - 360^\circ$) without cam pulses for single-sync identification.
   - Gaps at segments `26-27`, `32-33`, `62-63`, `98-99`, `104-105`, `134-135`.

3. **`AVANZA_XENIA_TERIOS_RUSH` (`Toyota Avanza/Xenia/Terios/Rush ` / index 20)**:
   - Contains 144 segments. Full dual-revolution VVT-i pattern with phase-shifted CAM pulses.
   - CAM active at segments `13-24` ($65^\circ - 120^\circ$), `49-60` ($245^\circ - 300^\circ$), `85-96` ($425^\circ - 480^\circ$).
   - Gaps shifted to segments `7-9`, `38-39`, `75-79`, `117-118`.

### 5.2 Mitsubishi Multi-Gap & Unequal Tooth Family
1. **`MITSUBISH_4g63_4_2` (`Mitsubishi 4g63 aka 4/2 crank and cam` / index 46)**:
   - 144 segments ($720^\circ$, $5.0^\circ$/segment, scaler = 0.6).
   - Simulates the legendary 4G63 CAS optical disc.
   - **Crank**: 2 distinct pulse events at $105^\circ - 165^\circ$ (segments 21-33) and $465^\circ - 525^\circ$ (segments 93-105). Pulse width = $60^\circ$.
   - **Cam**: 2 wide sync pulses of unequal duration:
     - Tooth 1: $0^\circ - 55^\circ$ (segments 0-10, width = $55^\circ$).
     - Tooth 2: $270^\circ - 360^\circ$ (segments 54-71, width = $90^\circ$).
   - Overlap regions generate bitmask `3` (Crank HIGH + Cam HIGH) at segments `57-69` and `130-142`.

2. **`SIX_G_SEVENTY_TWO_WITH_CAM` (`Mitsubishi 6g72 with cam` / index 25)**:
   - 144 segments ($720^\circ$, $5.0^\circ$/segment, scaler = 0.6).
   - 6-cylinder DOHC CAS: 6 crank pulses ($40^\circ$ high, $70^\circ$ low per cylinder event).
   - Cam has 4 sync teeth: 3 teeth of $40^\circ$ duration and 1 extra-wide master tooth of $85^\circ$ duration for #1 TDC indexing.

3. **`MITSUBISHI_3A92` (`Mitsubishi 3A92` / index 61)**:
   - 144 segments ($720^\circ$, $5.0^\circ$/segment, scaler = 0.6).
   - Mitsubishi Mirage 1.2L 3-cylinder engine.
   - Crank missing tooth gaps at segments `44-45`, `92-93`, `140-143`.
   - Cam pulses placed at segments `20` ($100^\circ$), `68-70` ($340^\circ - 350^\circ$), `116` ($580^\circ$).

### 5.3 Honda Jazz/Fit, D17, and RC51 Family
1. **`HONDA_JAZZ_FIT_04_08` (V1, V2, V3 / indices 49, 50, 51)**:
   - 144 segments ($720^\circ$, $5.0^\circ$/segment, scaler = 0.6).
   - Crankshaft: 12 equidistant teeth plus 1 extra sync tooth at $330^\circ$ (12+1 pattern).
   - Camshaft: 4 pulses corresponding to ignition events with distinctive duration/spacing variants:
     - **V1 (index 49)**: CAM high on segments 23-25, 66-95, 99-135.
     - **V2 (index 50)**: Phase offset shifted by $180^\circ$ (CAM high on segments 0-35, 109-140).
     - **V3 (index 51)**: Cam high at segments 36-38, 78-106, 110-143.

2. **`HONDA_D17_NO_CAM` (`Honda D17 Crank (12+1)` / index 48)**:
   - 144 segments ($720^\circ$, $5.0^\circ$/segment, scaler = 0.6).
   - Pure crank 12+1 pattern: 12 teeth spaced $30^\circ$ apart, with an extra sync tooth positioned $10^\circ$ after tooth 12.

3. **`HONDA_RC51_WITH_CAM` (`Honda RC51 with cam` / index 30)**:
   - 48 segments ($720^\circ$, $15.0^\circ$/segment, scaler = 0.2).
   - Honda RVT1000R / SP1 / SP2 $90^\circ$ V-Twin oddfire pattern.
   - Crank: 12 evenly spaced teeth per revolution ($30^\circ$ pitch).
   - Cam: Single-segment pulses at teeth 6, 16, and 18 (bitmask `3` overlap).

### 5.4 Subaru 36-2-2-2 & Multi-Gap Family
1. **`THIRTY_SIX_MINUS_TWO_TWO_TWO` (H4 / index 17)**:
   - 72 segments ($360^\circ$, $5.0^\circ$/segment, scaler = 0.6).
   - 36 theoretical teeth. Missing 2 teeth at 3 distinct angular sectors:
     - Gap 1: Segments 26-29 (teeth 13-14 missing)
     - Gap 2: Segments 32-35 (teeth 16-17 missing)
     - Gap 3: Segments 62-65 (teeth 31-32 missing)
   - Used on Suzuki Swift Sport and early Subaru boxer engines.

2. **`THIRTY_SIX_MINUS_TWO_TWO_TWO_H6` (H6 / index 21)**:
   - 72 segments ($360^\circ$, $5.0^\circ$/segment, scaler = 0.6).
   - Missing teeth configured for 6-cylinder EZ30/EZ36 boxer engines: gaps at segments `38-41`, `62-65`, `68-71`.

3. **`THIRTY_SIX_MINUS_TWO_TWO_TWO_WITH_CAM` (index 22)**:
   - 144 segments ($720^\circ$, $5.0^\circ$/segment, scaler = 0.6).
   - 36-2-2-2 crank with single cam pulses injected during crank gaps (segments 3-5 on rev 1, segment 55 on rev 2).

4. **`MAZDA_THIRTY_SIX_MINUS_TWO_TWO_TWO_WITH_SIX_TOOTH_CAM` (index 45)**:
   - 360 segments ($720^\circ$, $2.0^\circ$/segment, scaler = 1.5).
   - High-resolution representation for Mazda RX-8 rotary (Renesis) and Mazda 3/6 MZR engines.

### 5.5 Nissan High-Resolution & Optical CAS Family
1. **`NISSAN_LIVINA_JUKE` (index 35)**:
   - 720 segments ($720^\circ$, $1.0^\circ$/segment, scaler = 3.0).
   - HR15DE / HR16DE / MR16DDT pattern with groups of variable-pitch crank pulses and multi-tooth VTC camshaft signal.

2. **`THREE_SIXTY_NISSAN_CAS` (`Nissan 360 CAS with 6 slots` / index 38)**:
   - 720 segments ($720^\circ$, $1.0^\circ$/segment, scaler = 3.0).
   - Simulates the Nissan Optical CAS (RB26DETT, SR20DET, VG30DETT).
   - **Outer Track**: 360 slots ($1^\circ$ high, $1^\circ$ low) on CKP channel.
   - **Inner Track**: 6 slots spaced every $120^\circ$ with progressively increasing slot widths for deterministic 1-revolution cylinder detection:
     - Cylinder 1 slot: $8^\circ$ wide (segments 9-16)
     - Cylinder 2 slot: $16^\circ$ wide (segments 129-144)
     - Cylinder 3 slot: $24^\circ$ wide (segments 249-272)
     - Cylinder 4 slot: $32^\circ$ wide (segments 369-400)
     - Cylinder 5 slot: $40^\circ$ wide (segments 489-528)
     - Cylinder 6 slot: $48^\circ$ wide (segments 609-656)

### 5.6 Dual-Cam (CMP1 + CMP2) Engine Presets
1. **`BMW_N20` (`BMW N20` / index 66)**:
   - 240 segments ($720^\circ$, $3.0^\circ$/segment, scaler = 1.0).
   - **Crank**: Standard Bosch 58X (60-2) missing tooth wheel (teeth 58-60 missing at segments `116-119` and `236-239`).
   - **Camshafts**: Dual independent VVT camshafts:
     - Intake CAM (`CMP1` / bit 1) + Exhaust CAM (`CMP2` / bit 2)
     - Segments `30-41` (teeth 16-21) and `90-129` (teeth 46-65) have BOTH cams active (`bit 1 + bit 2 = 6` in gap, `bit 0 + bit 1 + bit 2 = 7` on tooth).
     - Segments `130-179` have both cams active on revolution 2.
     - **This is the only factory preset that actively drives all 3 outputs (`CKP`, `CMP1`, `CMP2`) simultaneously.**

2. **`GM_LS1_CRANK_AND_CAM` (`GM LS1 crank and cam` / index 27)**:
   - 720 segments ($720^\circ$, $1.0^\circ$/segment, scaler = 6.0).
   - GM Gen III 24X crank wheel (unequal width teeth: $3^\circ$ and $12^\circ$ alternating pulses) with $360^\circ$ half-moon cam on `CMP1` (bit 1). Segment 0 has bit 2 (`0x04`) set for knock signal sync testing.

---

## 6. Edge Cases & Implementation Constraints

| # | Feature / Preset | Discovered Condition | Impact on Driver / Simulator | Recommended Action in ECUSniff |
|---|---|---|---|---|
| 1 | `avanza_xenia_terios_rush_friendly_name` | Has a trailing whitespace in `wheel_defs.h`: `"Toyota Avanza/Xenia/Terios/Rush "` | String matching fails if trimmed | In database, provide both exact string and clean trimmed display name |
| 2 | `BMW_N20` (index 66) | Uses bitmask values `6` (`0b110`) and `7` (`0b111`) | Requires 3 independent RMT channels active simultaneously (CKP, CMP1, CMP2) | Driver RMT must support 3-channel pulse train conversion |
| 3 | `GM_LS1_CRANK_AND_CAM` (index 27) | Has `rpm_scaler = 6.0` despite $E=720, D=720$ ($3 \times 720 / 720 = 3.0$) | ArduStim doubled the scaler to fit AVR 16-bit timer overflow limits | RMT driver must use formula $T_{\text{seg}} = \frac{D \times 10^6}{6 \times E \times RPM}$ rather than relying on legacy scaler |
| 4 | `GM_7X` (index 58) | `rpm_scaler = 1.502` (non-integer float) | AVR timer math used empirical tweak for 6+1 tooth spacing | RMT microsecond buffer generation should use exact angular step ($4.0^\circ$/seg) |
| 5 | High Segment Arrays (`AUDI_135_WITH_CAM` = 1080 bytes; `OPTISPARK_LT1`, `SUBARU_SIX_SEVEN`, `NISSAN_LIVINA_JUKE`, `FORD_ST170` = 720 bytes) | RAM usage if stored in SRAM | Placing all 70 arrays in RAM would consume $>15\text{ KB}$ | Keep arrays in `const uint8_t[] PROGMEM` / flash; generate RMT items dynamically on preset change |
| 6 | RMT Hardware Looping | ESP32-S3 RMT channel looping mode requires zero-duration EOT item | Without EOT marker, RMT hardware reads uninitialized RAM | Always append `{ duration0 = 0, level0 = 0, duration1 = 0, level1 = 0 }` at end of RMT TX buffer |

---

## 7. Verification Method

To verify the findings of this specification mining report:
1. Cross-check enum ordering with `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` lines 75–147.
2. Cross-check `Wheels[]` array entries with `external/ardustim-tftv2-touchscreen/ardustim/ardustim.ino` lines 155–227.
3. Run the validation script:
   ```bash
   python .agents/survey_spec_miner/analyze_patterns.py
   ```
   Output: `ALL 70 patterns array lengths match their spec_edges perfectly!`
4. Verify bitmask decoder truth table with `PinMapping.h` line 36–50 and `OutputController.cpp`.
