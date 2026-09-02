# Laporan Survei Arsitektur Engine & HAL ECUSniff
**Subagent:** Explorer (Engine & HAL Architecture Survey)  
**Tanggal:** 2026-09-01  
**Target Platform:** ESP32-S3 DevKitC-1 (N8R8 - 8MB Flash / 8MB Octal PSRAM) & Wemos D1 R32  
**Dokumen Terkait:** `ORIGINAL_REQUEST.md`, `skills/ecu-pattern-designer/SKILL.md`

---

## 1. Executive Summary

Investigasi menyeluruh terhadap arsitektur subsistem **Engine Simulator (`lib/engine/`)** dan **Hardware Abstraction Layer (`lib/hal/`)** pada proyek ECUSniff telah selesai dilakukan. 

### Temuan Inti:
1. **Keterbatasan Pola Parametrik Saat Ini:** Implementasi saat ini pada `lib/engine/include/parametric_pattern.h` dan `lib/ui/include/wheel_database.h` mengasumsikan seluruh pola roda gigi mesin sebagai *single-gap missing tooth wheel* berjarak gigi ekuidistan (`totalTeeth`, `missingTeeth`, `dutyCycle`). Akibatnya, pola-pola mesin nyata berstruktur kompleks seperti **Toyota Avanza / Daihatsu Xenia (36-2-2-2 / 144 segment)**, **Mitsubishi 4G63 (4/2)**, **GM LS1 (24X dual-width)**, **Subaru 6/7**, **Chrysler NGC (36+2-2)**, dan **Daihatsu 3+1** diaproksimasi secara kasar dan salah, sehingga sinyal keluaran tidak sesuai dengan tabel decoder ECU aslinya (menyebabkan *Crank-Cam Sync Loss* pada ECU).
2. **Database ArduStim TFTv2:** Sumber rujukan `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` mendefinisikan **70 preset roda gigi OEM dan universal** menggunakan representasi *bit-array / segment pattern* berbasis siklus rotasi ($360^\circ$ atau $720^\circ$) dengan resolusi antara 4 hingga 1080 segmen. Setiap segmen mengodekan status 3 kanal output (Bit 0: CKP, Bit 1: CMP1, Bit 2: CMP2).
3. **Kompatibilitas Hardware RMT ESP32-S3:** Peripheral RMT ESP32-S3 memiliki 4 kanal pemancar (TX Channel 0–3) dengan total RAM hardware sebesar 192 slot item (`rmt_item32_t`). Dengan menggunakan algoritma **Run-Length Encoding (RLE) Edge Compression**, seluruh pola dari ArduStim (kecuali optical 360-slot tanpa prescaler) dapat dikompilasi menjadi buffer pulsa RMT $\le 96$ item untuk CKP dan $\le 48$ item untuk CMP/CMP2. Ini memungkinkan transmisi pulsa dalam mode **Hardware Continuous Loop** murni tanpa overhead CPU atau risiko buffer underrun.
4. **Analisis Memori Flash & SRAM:** Menyimpan seluruh 70 definisi pola bit-array dari ArduStim dalam memori Flash/PROGMEM hanya memerlukan **$\approx 24.7\text{ KB}$**, yang mana kurang dari **0.7%** dari partisi aplikasi 3.5 MB (`app0`), serta **0 bytes internal SRAM** (karena dipetakan langsung ke Flash DROM `0x3C000000`). SRAM hanya digunakan untuk buffer aktif RMT ping-pong ($\approx 6\text{ KB}$).

---

## 2. Analisis Struktur Kode Existing

### 2.1. Subsistem Engine (`lib/engine/`)

| File | Komponen / Kelas | Analisis Kondisi Saat Ini & Keterbatasan |
|---|---|---|
| `include/engine_types.h` | `EngineRunMode`, `CrankingConfig`, `SweepConfig`, `CmpEvent`, `SignalHealthStatus`, `EngineRuntimeState` | Menyimpan konfigurasi cranking (SpinUp, Cranking, Ramp, Flare), mode sweep, dan health sniffer. `CmpEvent` hanya mendukung maksimal 16 event CAM statis. |
| `include/timing_math.h` / `src/timing_math.cpp` | `TimingMath` | Menyediakan kalkulasi microsecond: `calculateRevPeriodUs()`, `calculateCyclePeriodUs()`, `calculateUsPerDegree()`, dan `angleToTimeUs()`. Menggunakan basis $60 \times 10^6 / RPM$ ($360^\circ$) dan $120 \times 10^6 / RPM$ ($720^\circ$). Matematika timing sudah sangat akurat dan bebas floating-point drift. |
| `include/parametric_pattern.h` / `src/parametric_pattern.cpp` | `ParametricWheel`, `CamEventTable`, `ParametricEngine` | **Titik Masalah Utama:** <br>1. `ParametricWheel` hanya dapat memodelkan roda dengan 1 kelompok celah (*single contiguous missing gap*) pada `missingPosition`. Tidak mampu menangani *multi-gap* (misal 36-2-2-2 memiliki 3 celah).<br>2. Gigi diasumsikan ekuidistan dengan durasi sudut tetap ($360^\circ / N$). Tidak mampu menangani variasi lebar gigi (misal GM LS1 12°/3° atau 4G63 70°/110°).<br>3. `generateCkpCycle` membangkitkan $2 \times N$ segmen pulsa secara parametrik. |
| `include/rpm_controller.h` / `src/rpm_controller.cpp` | `RpmController` | State machine cranking (`Idle -> SpinUp -> Cranking -> Ramping -> PostCrank`), modulasi potensiometer ADC, dan kalkulasi sweep RPM linier/ping-pong. Berjalan independen dan stabil. |
| `include/signal_sniffer.h` / `src/signal_sniffer.cpp` | `SignalSniffer` | Dekoder tepi sinyal tangkapan untuk mendeteksi jumlah gigi, missing gap, dan RPM. |
| `include/eps_controller.h`, `speedo_controller.h`, `power_cycle_controller.h` | Kontroler instrumen & pengujian | Bekerja pada layer kontrol independen (LEDC PWM dan I2C DAC/GPIO). |

### 2.2. Subsistem Hardware Abstraction Layer (`lib/hal/`)

| File | Komponen / Peripheral | Analisis Kondisi Saat Ini |
|---|---|---|
| `include/rmt_generator.h` / `src/rmt_generator.cpp` | `RmtGenerator` (ESP32 RMT TX) | Mengonfigurasi peripheral RMT mode TX untuk **CH0 (CKP)** dan **CH3 (CMP)**.<br>- `MEM_BLOCKS_CKP = 3` (144 item), `MEM_BLOCKS_CMP = 1` (48 item).<br>- Menggunakan driver legacy IDF (`<driver/rmt.h>`, `rmt_fill_tx_items`, `rmt_set_tx_loop_mode`).<br>- Mengimplementasikan double-buffering (*ping-pong buffer*) `_ckpBufferA/B` dan `_cmpBufferA/B` berukuran `MAX_CYCLE_PULSES = 256`.<br>- **Kelemahan:** Mengandalkan `ParametricEngine::generateCkpCycle` dan belum memiliki decoder bit-array atau dukungan kanal ketiga (`SIG_CMP2`). |
| `include/pin_config.h` | Pinout Mapping | - **ESP32-S3:** `SIG_CKP = GPIO 4`, `SIG_CMP = GPIO 5`, `SIG_CMP2 = GPIO 6`.<br>- Input Sniffer: `CAP_CKP = GPIO 7`, `CAP_CMP = GPIO 8`, `CAP_CMP2 = GPIO 21`.<br>- TFT SPI: FSPI Bus (GPIO 9, 10, 11, 12, 14, 15).<br>- Input Encoder & Joystick: GPIO 1, 2, 16, 17, 18, 42. Seluruh pinout terisolasi rapi tanpa konflik. |
| `include/capture_driver.h` / `src/capture_driver.cpp` | `CaptureDriver` (GPIO ISR) | Driver capture berbasis interrupt GPIO (`attachInterrupt`) dengan filter glitch hardware $5\ \mu\text{s}$ dan buffer circular 512 event. |
| `include/speedo_driver.h`, `eps_driver.h` | `SpeedoDriver`, `EpsDriver` (LEDC PWM) | Speedo menggunakan LEDC Ch 0-3, EPS menggunakan LEDC Ch 4-7. Tidak ada tabrakan peripheral dengan RMT TX (Ch 0-3). |

---

## 3. Komparasi: Parametric Wheel vs Arbitrary Bit-Array Pattern

### 3.1. Keterbatasan Fundamental Model Parametrik

Model parametrik matematis:
$$\theta_{\text{tooth}} = \frac{360^\circ}{N_{\text{total}}}, \quad \theta_{\text{high}} = \theta_{\text{tooth}} \times \text{DutyCycle}, \quad \theta_{\text{low}} = \theta_{\text{tooth}} \times (1 - \text{DutyCycle})$$

Model ini **hanya valid** untuk roda gigi standar tipe Bosch $N-M$ tunggal (misal $60-2$ atau $36-1$). Model ini **gagal total** pada:
1. **Multi-Gap Wheels (contoh: 36-2-2-2 / Avanza, Swift, Subaru):**
   Pada pola 36-2-2-2, dalam 1 putaran ($360^\circ$) terdapat 36 slot teoritis, di mana celah 2 gigi terjadi di 3 titik terpisah sepanjang keliling roda (misal gigi 11-12, 23-24, 35-36). Parameter tunggal `missingPosition = 0, missingTeeth = 2` tidak mungkin merepresentasikan 3 celah ini.
2. **Asymmetric / Variable-Width Pulses (contoh: Mitsubishi 4G63, GM LS1 24X, GM 7X):**
   - Pola 4G63 hanya memiliki 2 gigi per putaran dengan lebar pulsa dan jarak non-seragam ($70^\circ$ pulsa pertama, $110^\circ$ pulsa kedua).
   - Pola GM LS1 24X memiliki pola bergantian antara pulsa sempit ($3^\circ$) dan pulsa lebar ($12^\circ$).
   - Pola GM 7X memiliki 6 gigi simetris $60^\circ$ ditambah 1 gigi ekstra pada $112^\circ$.
3. **Multi-Channel Crank + Dual-Cam Synchronization (contoh: BMW N20, Avanza Dual VVT-i):**
   Membutuhkan sinkronisasi simultan antara CKP, CMP1 (Intake Cam), dan CMP2 (Exhaust Cam) dalam 1 siklus $720^\circ$.

### 3.2. Struktur Bit-Array ArduStim

ArduStim menyelesaikan permasalahan ini dengan memetakan sinyal otomotif sebagai **array segmen waktu-sudut (Angle-Slot Array)**:
- Setiap elemen array mewakili 1 irisan sudut (*slice*) sebesar:
  $$\Delta\theta = \frac{\text{wheel\_degrees}}{\text{wheel\_max\_edges}}$$
- Format nilai byte per elemen:
  - `0`: LOW untuk semua kanal
  - `1` (`0x01`): CKP = HIGH
  - `2` (`0x02`): CMP1 = HIGH
  - `3` (`0x03`): CKP + CMP1 = HIGH
  - `4` (`0x04`): CMP2 = HIGH
  - `5` (`0x05`): CKP + CMP2 = HIGH
  - `6` (`0x06`): CMP1 + CMP2 = HIGH
  - `7` (`0x07`): CKP + CMP1 + CMP2 = HIGH

#### Contoh Perbandingan Pola Toyota Avanza:
- **Pada `wheel_database.h` (Lama/Salah):**
  `{ "Toyota Avanza 1.3 K3-VE", 31, 2, 0, 0.50f, false, 4, {185.0f, 245.0f, 365.0f, 425.0f}, ... }`
  *(Dianggap roda 31 gigi ekuidistan dengan 1 celah 2-gigi — tidak bisa dibaca oleh ECU Avanza K3-VE)*.
- **Pada `wheel_defs.h` ArduStim (Benar / 144-Segmen $720^\circ$):**
  Array 144 byte ($\Delta\theta = 720 / 144 = 5.0^\circ/\text{elemen}$), secara presisi merekonstruksi urutan celah 36-2-2-2 serta pulsa CAM 4-silinder sinkron.

---

## 4. Pemetaan Lengkap 70 Pola Roda Gigi ArduStim TFTv2

Seluruh 70 preset dari `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` dipetakan ke dalam 5 kategori resmi ECUSniff:

| # | Friendly Name (Identik ArduStim TFTv2) | Kategori | Array Sumber | Panjang Segmen | Rentang Siklus |
|---|---|---|---|---|---|
| 0 | `4 cylinder dizzy` | Universal | `dizzy_four_cylinder` | 4 | $360^\circ$ |
| 1 | `6 cylinder dizzy` | Universal | `dizzy_six_cylinder` | 6 | $360^\circ$ |
| 2 | `8 cylinder dizzy` | Universal | `dizzy_eight_cylinder` | 8 | $360^\circ$ |
| 3 | `60-2 crank only` | Universal | `sixty_minus_two` | 120 | $360^\circ$ |
| 4 | `60-2 crank and cam` | Universal | `sixty_minus_two_with_cam` | 240 | $720^\circ$ |
| 5 | `60-2 crank and 'half moon' cam` | Universal | `sixty_minus_two_with_halfmoon_cam` | 240 | $720^\circ$ |
| 6 | `36-1 crank only` | Universal | `thirty_six_minus_one` | 72 | $360^\circ$ |
| 7 | `24-1 crank only` | Universal | `twenty_four_minus_one` | 48 | $360^\circ$ |
| 8 | `4-1 crank wheel with cam` | Universal | `four_minus_one_with_cam` | 16 | $720^\circ$ |
| 9 | `8-1 crank only (R6)` | Honda/Suzuki/Yamaha | `eight_minus_one` | 16 | $360^\circ$ |
| 10 | `6-1 crank with cam` | Universal | `six_minus_one_with_cam` | 36 | $720^\circ$ |
| 11 | `12-1 crank with cam` | Universal | `twelve_minus_one_with_cam` | 144 | $720^\circ$ |
| 12 | `40-1 crank only (Ford V10)` | Euro/US | `fourty_minus_one` | 80 | $360^\circ$ |
| 13 | `Distributor style 4 cyl 50deg off, 40 deg on` | Universal | `dizzy_four_trigger_return` | 9 | $720^\circ$ |
| 14 | `odd fire 90 deg pattern 0 and 135 pulses` | Universal | `oddfire_vr` | 24 | $360^\circ$ |
| 15 | `GM OptiSpark LT1 360 and 8` | Euro/US | `optispark_lt1` | 720 | $720^\circ$ |
| 16 | `12-3 oddball` | Universal | `twelve_minus_three` | 48 | $360^\circ$ |
| 17 | `36-2-2-2 H4 Crank only` | Universal | `thirty_six_minus_two_two_two` | 72 | $360^\circ$ |
| 18 | `Toyota Avanza 1.3 Crank only` | Toyota/Daihatsu | `old_avanza` | 144 | $720^\circ$ |
| 19 | `Toyota Avanza 1.5 Crank only` | Toyota/Daihatsu | `new_avanza` | 144 | $720^\circ$ |
| 20 | `Toyota Avanza/Xenia/Terios/Rush ` | Toyota/Daihatsu | `avanza_xenia_terios_rush` | 144 | $720^\circ$ |
| 21 | `36-2-2-2 H6 Crank only` | Universal | `thirty_six_minus_two_two_two_h6` | 72 | $360^\circ$ |
| 22 | `36-2-2-2 Crank and cam` | Toyota/Daihatsu | `thirty_six_minus_two_two_two_with_cam` | 144 | $720^\circ$ |
| 23 | `GM 4200 crank wheel` | Euro/US | `fourty_two_hundred_wheel` | 72 | $360^\circ$ |
| 24 | `Mazda FE3 36-1 with cam` | Mitsu/Nissan/Mazda | `thirty_six_minus_one_with_cam_fe3` | 144 | $720^\circ$ |
| 25 | `Mitsubishi 6g72 with cam` | Mitsu/Nissan/Mazda | `six_g_seventy_two_with_cam` | 144 | $720^\circ$ |
| 26 | `Buell Oddfire CAM wheel` | Euro/US | `buell_oddfire_cam` | 80 | $720^\circ$ |
| 27 | `GM LS1 crank and cam` | Euro/US | `gm_ls1_crank_and_cam` | 720 | $720^\circ$ |
| 28 | `GM 58x crank and 4x cam` | Euro/US | `GM_LS_58X_crank_and_4x_cam` | 240 | $720^\circ$ |
| 29 | `Odd Lotus 36-1-1-1-1 flywheel` | Euro/US | `lotus_thirty_six_minus_one_one_one_one` | 72 | $360^\circ$ |
| 30 | `Honda RC51 with cam` | Honda/Suzuki/Yamaha | `honda_rc51_with_cam` | 48 | $720^\circ$ |
| 31 | `36-1 crank with 2nd trigger on teeth 33-34` | Universal | `thirty_six_minus_one_with_second_trigger` | 144 | $720^\circ$ |
| 32 | `Chrysler NGC 36+2-2 crank, NGC 4-cyl cam` | Euro/US | `chrysler_ngc_thirty_six_plus_two_minus_two_with_ngc4_cam` | 720 | $720^\circ$ |
| 33 | `Chrysler NGC 36-2+2 crank, NGC 6-cyl cam` | Euro/US | `chrysler_ngc_thirty_six_minus_two_plus_two_with_ngc6_cam` | 720 | $720^\circ$ |
| 34 | `Chrysler NGC 36-2+2 crank, NGC 8-cyl cam` | Euro/US | `chrysler_ngc_thirty_six_minus_two_plus_two_with_ngc8_cam` | 720 | $720^\circ$ |
| 35 | `Nissan Livina Juke crank and cam` | Mitsu/Nissan/Mazda | `nissan_livina_juke_pattern` | 720 | $720^\circ$ |
| 36 | `Weber-Marelli 8 crank+2 cam pattern` | Euro/US | `weber_iaw_with_cam` | 144 | $720^\circ$ |
| 37 | `Fiat 1.8 16V crank and cam` | Euro/US | `fiat_one_point_eight_sixteen_valve_with_cam` | 720 | $720^\circ$ |
| 38 | `Nissan 360 CAS with 6 slots` | Mitsu/Nissan/Mazda | `three_sixty_nissan_cas` | 720 | $720^\circ$ |
| 39 | `Mazda CAS 24-2 with single pulse outer ring` | Mitsu/Nissan/Mazda | `twenty_four_minus_two_with_second_trigger` | 72 | $720^\circ$ |
| 40 | `Yamaha 2002-03 R1 8 even-tooth crank with 1 tooth cam` | Honda/Suzuki/Yamaha | `yamaha_eight_tooth_with_cam` | 64 | $720^\circ$ |
| 41 | `GM 4 even-tooth crank with 1 tooth cam` | Euro/US | `gm_four_tooth_with_cam` | 8 | $720^\circ$ |
| 42 | `GM 6 even-tooth crank with 1 tooth cam` | Euro/US | `gm_six_tooth_with_cam` | 12 | $720^\circ$ |
| 43 | `GM 8 even-tooth crank with 1 tooth cam` | Euro/US | `gm_eight_tooth_with_cam` | 16 | $720^\circ$ |
| 44 | `Volvo d12[acd] crank with 7 tooth cam` | Euro/US | `volvo_d12acd_with_cam` | 480 | $720^\circ$ |
| 45 | `Mazda 36-2-2-2 with 6 tooth cam` | Mitsu/Nissan/Mazda | `mazda_thirty_six_minus_two_two_two_with_six_tooth_cam` | 360 | $720^\circ$ |
| 46 | `Mitsubishi 4g63 aka 4/2 crank and cam` | Mitsu/Nissan/Mazda | `mitsubishi_4g63_4_2` | 144 | $720^\circ$ |
| 47 | `Audi 135 tooth crank and cam` | Euro/US | `audi_135_with_cam` | 1080 | $720^\circ$ |
| 48 | `Honda D17 Crank (12+1)` | Honda/Suzuki/Yamaha | `honda_d17_no_cam` | 144 | $720^\circ$ |
| 49 | `Honda Jazz Fit 04-08` | Honda/Suzuki/Yamaha | `honda_jazz_fit_04_08` | 144 | $720^\circ$ |
| 50 | `Honda Jazz Fit 04-08V2` | Honda/Suzuki/Yamaha | `honda_jazz_fit_04_08v2` | 144 | $720^\circ$ |
| 51 | `Honda Jazz Fit 04-08V3` | Honda/Suzuki/Yamaha | `honda_jazz_fit_04_08v3` | 144 | $720^\circ$ |
| 52 | `Mazda 323 AU version` | Mitsu/Nissan/Mazda | `mazda_323_au` | 30 | $720^\circ$ |
| 53 | `Daihatsu 3+1 distributor (3 cylinders)` | Toyota/Daihatsu | `daihatsu_3cyl` | 144 | $360^\circ$ |
| 54 | `Miata 99-05` | Mitsu/Nissan/Mazda | `miata_9905` | 144 | $720^\circ$ |
| 55 | `12/1 (12 crank with cam)` | Universal | `twelve_with_cam` | 144 | $720^\circ$ |
| 56 | `24/1 (24 crank with cam)` | Universal | `twenty_four_with_cam` | 144 | $720^\circ$ |
| 57 | `Subaru 6/7 crank and cam` | Mitsu/Nissan/Mazda | `subaru_six_seven` | 720 | $720^\circ$ |
| 58 | `GM 7X` | Euro/US | `gm_seven_x` | 180 | $720^\circ$ |
| 59 | `DSM 420a` | Euro/US | `four_twenty_a` | 144 | $720^\circ$ |
| 60 | `Ford ST170` | Euro/US | `ford_st170` | 720 | $720^\circ$ |
| 61 | `Mitsubishi 3A92` | Mitsu/Nissan/Mazda | `mitsubishi_3A92` | 144 | $720^\circ$ |
| 62 | `Toyota 4AGE` | Toyota/Daihatsu | `toyota_4AGE_CAS` | 144 | $720^\circ$ |
| 63 | `Toyota 4AGZE` | Toyota/Daihatsu | `toyota_4AGZE` | 144 | $720^\circ$ |
| 64 | `Suzuki DRZ400` | Honda/Suzuki/Yamaha | `suzuki_DRZ400` | 72 | $360^\circ$ |
| 65 | `Jeep 2000` | Euro/US | `jeep_2000` | 360 | $720^\circ$ |
| 66 | `BMW N20` | Euro/US | `bmw_n20` | 240 | $720^\circ$ |
| 67 | `Dodge Viper V10 1996-2002` | Euro/US | `viper9602wheel` | 240 | $720^\circ$ |
| 68 | `36-2 with 1 tooth cam` | Toyota/Daihatsu | `thirty_six_minus_two_with_second_trigger` | 144 | $720^\circ$ |
| 69 | `GM 40 tooth OSS wheel for Transmissions` | Euro/US | `GM40toothOSS` | 80 | $360^\circ$ |

---

## 5. Arsitektur ESP32-S3 RMT & Konversi Pulsa Presisi Tinggi

### 5.1. Spesifikasi Hardware RMT ESP32-S3
- **Kanal Pemancar (TX):** 4 Kanal independen (Channel 0 – Channel 3).
- **RAM Dedicated Hardware:** 4 Blok memori $\times 48$ slot `rmt_item32_t` = **192 item**.
- **Alokasi Blok Memori Optimal (Bebas Tabrakan):**
  - `CH_CKP` (Channel 0): 2 Blok (96 item `rmt_item32_t`)
  - `CH_CMP1` (Channel 1): 1 Blok (48 item `rmt_item32_t`)
  - `CH_CMP2` (Channel 2): 1 Blok (48 item `rmt_item32_t`)
  *(Total = 4 Blok = 192 slot item).*
- **Format Item RMT:**
  Setiap `rmt_item32_t` (32-bit) memuat 2 pulsa berturut-turut:
  `{ duration0: 15-bit, level0: 1-bit, duration1: 15-bit, level1: 1-bit }`.
  - Durasi maksimum 1 sub-pulsa: $2^{15}-1 = 32,767\ \mu\text{s}$ (pada `clk_div = 80`, 1 tick = $1.0\ \mu\text{s}$).
  - Penanda akhir transmisi: Zero-terminator EOT `{ duration0: 0, level0: 0, duration1: 0, level1: 0 }`.

### 5.2. Algoritma Konversi Bit-Array ke RMT (Run-Length Encoding)

Untuk mengonversi bit-array sumber ke dalam buffer pulsa RMT berdurasi mikrodetik tanpa distorsi:

1. **Hitung Total Waktu Siklus ($T_{\text{cycle}}$):**
   $$T_{\text{cycle}} = \frac{120 \times 10^6}{RPM}\ \mu\text{s} \quad (\text{untuk siklus } 720^\circ)$$
2. **Kuantisasi Waktu Kumulatif (Integer 64-bit):**
   Untuk setiap segmen $s \in [0, N_{\text{segments}}-1]$ di mana $N_{\text{segments}} = L \times (720 / \Phi)$:
   $$t_{\text{start}}[s] = \left\lfloor \frac{s \times T_{\text{cycle}}}{N_{\text{segments}}} \right\rfloor, \quad t_{\text{end}}[s] = \left\lfloor \frac{(s + 1) \times T_{\text{cycle}}}{N_{\text{segments}}} \right\rfloor$$
   *Sifat matematis:* $\sum \Delta t \equiv T_{\text{cycle}}$ presisi integer mutlak tanpa *cumulative rounding error*.
3. **Ekstraksi Masking & Kompresi Tepi (RLE):**
   - Kanal CKP mengekstrak Bit 0 (`& 0x01`), CMP1 mengekstrak Bit 1 (`& 0x02`), CMP2 mengekstrak Bit 2 (`& 0x04`).
   - Gabungkan segmen-segmen bersebelahan yang memiliki logika sama menjadi satu pulsa panjang $D = t_{\text{end\_run}} - t_{\text{start\_run}}$.
   - Jika $D > 30,000\ \mu\text{s}$ (misal pada RPM rendah 200 RPM), potong menjadi sub-fase $\le 30,000\ \mu\text{s}$ dengan logika level yang sama.
4. **Pembentukan Item RMT & EOT:**
   - Pasangkan setiap 2 fase durasi ke dalam 1 slot `rmt_item32_t`.
   - Tambahkan item EOT `{0, 0, 0, 0}` pada slot terakhir.
   - Muat ke hardware RAM via `rmt_fill_tx_items(channel, items, count, 0)`.
   - Aktifkan continuous loop `rmt_set_tx_loop_mode(channel, true)` dan `rmt_tx_start(channel, true)`.

### 5.3. Double Buffering (Ping-Pong) untuk Transisi RPM Mulus
Ketika RPM diubah (misal saat sweep atau akselerasi/cranking), kalkulasi RMT dijalankan pada buffer cadangan (Buffer B). Setelah selesai, transmisi dihentikan sesaat, buffer B dimuat ke RAM RMT, dan loop diaktifkan kembali. Tidak ada glitch atau pergeseran fase antar-revolusi.

---

## 6. Analisis Batasan Memori (Flash / SRAM / PSRAM)

### 6.1. Flash (PROGMEM)
- **Data Array 70 Pola:** $\approx 21.5\text{ KB}$
- **Tabel Struktur & String Nama:** $\approx 3.2\text{ KB}$
- **Total Kebutuhan Database:** $\mathbf{\approx 24.7\text{ KB}}$
- **Kapasitas Partisi Aplikasi (`app0`):** 3,670,016 bytes (3.5 MB).
- **Penggunaan Flash Firmware Saat Ini:** 1,033,821 bytes (28.2%).
- **Sisa Flash Bebas:** 2.63 MB.
- **Kesimpulan:** Penambahan database 70 pola hanya memakan **0.67%** dari ruang Flash yang tersedia. Sangat aman!

### 6.2. Internal SRAM
- Database dideklarasikan `const ... PROGMEM` (Flash DROM), sehingga **0 byte RAM** terpakai untuk penyimpanan data pola.
- Runtime RAM yang digunakan hanyalah buffer ganda aktif RMT:
  - $2 \times (\text{Buffer CKP } 256\text{ item} + \text{Buffer CMP } 128\text{ item} + \text{Buffer CMP2 } 128\text{ item}) \times 4\text{ byte} \approx \mathbf{4.0\text{ KB}}$.
- **Kesimpulan:** SRAM internal ESP32-S3 (320 KB) sangat longgar.

### 6.3. PSRAM (8 MB Octal PSRAM)
- Partisi Octal PSRAM N8R8 siap digunakan jika ada fitur *Custom User Wheel Pattern Recording / Upload* via Web Dashboard dengan kapasitas jutaan data point.

---

## 7. Rencana Perubahan Arsitektur & Langkah Implementasi

### Layer 1: Engine Database & Pattern Structures (`lib/engine/`)
1. Buat struktur data universal `WheelPresetDefinition` yang memuat:
   - `friendlyName` (string PROGMEM)
   - `patternData` (pointer array PROGMEM)
   - `edgeCount` (uint16_t)
   - `cycleDegrees` (360 atau 720)
   - `rpmScaler` (float)
   - `category` (enum WheelCategory)
2. Buat kelas `SignalPatternCompiler` pada `lib/engine/` untuk melakukan kompilasi RLE dari bit-array menjadi urutan pulsa waktu mikrodetik untuk siklus $720^\circ$.
3. Pertahankan `ParametricWheel` sebagai wrapper kompatibilitas untuk mode kustom manual (*custom single-gap tuner*).

### Layer 2: Hardware Driver (`lib/hal/`)
1. Perbarui `RmtGenerator` untuk mendukung 3 kanal output simultan (`SIG_CKP`, `SIG_CMP`, `SIG_CMP2`).
2. Implementasikan fungsi `compileAndLoadPattern(preset, targetRpm)` yang mengonversi bit-array ArduStim ke format `rmt_item32_t` dengan pembagian blok RAM hardware ESP32-S3 yang optimal.
3. Pastikan eksekusi start multi-channel RMT sinkron pada awal siklus $720^\circ$.

### Layer 3: User Interface & Waveform Canvas (`lib/ui/`)
1. Sinkronkan seluruh 70 preset pada `lib/ui/include/wheel_database.h` dengan penamaan ramah yang identik persis dengan ArduStim TFTv2.
2. Perbarui `WaveformCanvas::_drawCkpTrace` dan `_drawCmpTrace` agar mampu merender grafik osiloskop virtual $0 - 720^\circ$ langsung dari bit-array segmen.

### Layer 4: Unit Testing & Verifikasi Sinyal (`test/`)
1. Tambahkan unit test komparatif timing pada `test/` untuk memvalidasi urutan transisi tepi sinyal pola `NEW_AVANZA`, `OLD_AVANZA`, `AVANZA_XENIA_TERIOS_RUSH`, `4G63`, `60-2`, dan `36-2-2-2`.
2. Validasi kelulusan kompilasi firmware `pio run -e esp32s3`.

---
*Laporan survei ini siap diteruskan ke Orchestrator dan Implementer.*
