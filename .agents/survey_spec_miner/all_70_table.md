| # | Enum Identifier | Friendly Name (`wheel_defs.h`) | TFT Name (`WheelPatternManager.cpp`) | Category / Brand | Pattern Architecture | Segments ($E$) | Cycle ($D$) | Deg/Seg | RPM Scaler | Signal Channels | Distinct Bitmasks |
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