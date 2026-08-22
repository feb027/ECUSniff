# PRD — Automotive ECU Test Platform

**Project Type:** Prototype / Engineering Tool
**Status:** Draft v0.1
**Target MCU Prototype:** ESP32
**Prototype Display:** OLED 1.3 inch
**Primary Focus:** Programmable CKP/CMP/CMP2 Signal Generator
**Future Scope:** Sensor Simulator, Waveform Capture/Analyzer, CAN/F-CAN, B-CAN, LIN, K-Line, ECU Validation

---

## 1. Ringkasan Produk

Automotive ECU Test Platform adalah alat bench-test modular untuk membantu pekerjaan diagnosis dan pengujian ECU/instrument cluster kendaraan.

Tahap awal difokuskan pada pembuatan **programmable engine signal generator** yang mampu menghasilkan CKP, CMP, dan CMP2 berdasarkan parameter teknis, bukan hanya berdasarkan nama atau model kendaraan.

Konsep utama alat:

> **Engine position → Signal pattern → Electrical interface → ECU → Validation**

Alat ditujukan agar pattern baru dapat dibuat, diedit, disimpan, dan direproduksi tanpa harus menunggu firmware khusus untuk setiap kendaraan.

---

## 2. Masalah yang Ingin Diselesaikan

Generator CKP/CMP yang hanya menyediakan pattern baku seperti `36-1`, `60-2`, atau preset berdasarkan model kendaraan memiliki keterbatasan ketika:

- ECU yang diuji belum tersedia di database alat.
- CKP sama tetapi posisi reference/gap berbeda.
- CMP memiliki phase yang berbeda.
- CMP mempunyai lebih dari satu event dalam 720°.
- ECU dapat hidup dan mengeluarkan injector/ignition tetapi masih menyimpan DTC CKP/CMP atau correlation error.
- Jenis electrical interface berbeda walaupun pattern mekanisnya terlihat serupa.
- Dibutuhkan reproduksi waveform berdasarkan hasil capture kendaraan sehat.

Platform ini ditujukan untuk menguji **validitas penerimaan ECU terhadap posisi crank/cam**, bukan sekadar memastikan ECU mengeluarkan spark atau injector.

---

## 3. Tujuan Produk

### 3.1 Tujuan Utama

Membuat platform yang mampu:

1. Menghasilkan CKP secara programmable.
2. Menghasilkan CMP secara programmable.
3. Menghasilkan CMP2 secara programmable.
4. Menjalankan pattern berdasarkan engine angle 0–720°.
5. Mengatur RPM secara real-time.
6. Mensimulasikan cranking dan running.
7. Mengatur rising/falling edge CMP.
8. Menentukan phase CKP-CMP-CMP2.
9. Mendukung electrical interface yang berbeda.
10. Menyimpan pattern sebagai data teknis.
11. Menambahkan custom pattern hasil capture.
12. Memantau respons ECU untuk validasi.

### 3.2 Tujuan Jangka Panjang

Platform dikembangkan menjadi **Automotive ECU Test Platform** yang dapat mencakup:

- Sensor simulator.
- Waveform capture dan analyzer.
- VSS simulator.
- Analog sensor simulation.
- Resistive sensor simulation.
- CAN/F-CAN.
- B-CAN.
- LIN.
- K-Line.
- CAN replay/scenario.
- ECU/module validation.

---

## 4. Prinsip Arsitektur

Sistem dibagi menjadi beberapa lapisan agar hardware dan software tidak saling mengunci.

```text
                    AUTOMOTIVE ECU TEST PLATFORM
                              │
            ┌─────────────────┼─────────────────┐
            │                 │                 │
            ▼                 ▼                 ▼
    ENGINE SIGNAL         SENSOR I/O      COMMUNICATION
       ENGINE                 │                 │
            │                 │                 │
      ┌─────┼─────┐      Analog/PWM/       CAN/LIN/
      │     │     │      Frequency/         K-Line
     CKP   CMP   CMP2    Resistance
            │
            ▼
     ELECTRICAL DRIVER
            │
            ▼
           ECU
            │
            ▼
       VALIDATION
```

### Lapisan 1 — Mechanical / Engine Position

Mendefinisikan posisi crankshaft/camshaft secara konseptual:

- TDC.
- BDC.
- 360° crank revolution.
- 720° four-stroke engine cycle.
- CKP tooth pattern.
- Missing tooth.
- CMP phase.

### Lapisan 2 — Signal Pattern

Mendefinisikan kapan signal berubah:

- Tooth event.
- Missing event.
- Rising edge.
- Falling edge.
- Pulse width.
- Event table.
- Custom pattern.

### Lapisan 3 — Electrical Interface

Mendefinisikan bagaimana signal terlihat secara listrik:

- Push-pull 0–5 V.
- Push-pull 0–12 V.
- Open collector.
- Open drain.
- Hall/digital.
- VR/inductive.
- Polarity.
- Amplitude.
- Rise/fall characteristic.

### Lapisan 4 — Communication

Future expansion:

- CAN.
- F-CAN.
- B-CAN.
- CAN FD.
- LIN.
- K-Line / KWP / ISO 9141 related interfaces.

---

## 5. Target Prototype V0.1

Prototype pertama **tidak perlu langsung universal secara hardware**. Fokusnya adalah membuktikan arsitektur software dan timing engine.

### Hardware Minimum

- ESP32.
- OLED 1.3 inch.
- Rotary encoder.
- Tombol Back/Cancel bila diperlukan.
- 3 output digital prototype:
  - CKP.
  - CMP.
  - CMP2.
- Minimal 3 input capture prototype:
  - CKP IN.
  - CMP IN.
  - CMP2 IN.
- Jalur komunikasi internal untuk ekspansi.
- Catu daya 12 V ke 5 V/3.3 V yang terproteksi.

### Belum wajib pada V0.1

- VR driver final.
- Automotive-grade transient protection final.
- CAN/LIN/K-Line physical layer.
- Resistive sensor simulator.
- Injector/ignition load driver.

---

## 6. Konsep Interface Terisolasi

Karena perangkat akan sering digunakan untuk bench testing dan berpotensi menghadapi salah sambung atau tegangan balik, desain harus mengutamakan perlindungan MCU.

Konsep awal:

```text
ESP32
  │
  ▼
Isolation / Buffer
  │
  ▼
Signal Driver
  │
  ▼
Automotive / ECU Side
```

Optocoupler dapat digunakan pada jalur yang sesuai, tetapi isolasi tidak dianggap sebagai satu-satunya proteksi.

Lapisan proteksi yang perlu disiapkan dalam desain lanjutan:

- Current limiting.
- Clamp/protection.
- Reverse polarity protection.
- TVS/transient protection.
- Short-circuit tolerance.
- Pemisahan jalur ground dan return current yang terkontrol.

---

## 7. Arsitektur Software

Software harus dipisahkan menjadi engine yang independen dari jenis display.

```text
APPLICATION
│
├── Menu / UI
├── Pattern Database
├── Test Scenario
└── Validation

ENGINE LAYER
│
├── Engine Position Engine
├── Pattern Engine
├── Event Scheduler
├── RPM / Time Base Engine
├── Capture Engine
└── Electrical Configuration

HAL / DRIVER
│
├── GPIO / Timer
├── Capture Peripheral
├── Display Driver
├── Storage
└── Future CAN/LIN/K-Line Driver
```

OLED hanya menjadi **Display Layer**, sehingga nantinya dapat diganti dengan TFT SPI 2.4/3.0/4.0 inch tanpa mengubah logic generator.

---

## 8. Core Engine — Engine Position

Ini adalah bagian inti proyek.

Sistem internal menggunakan posisi mesin:

```text
0° → 720°
```

untuk engine 4-stroke.

### Konsep

- CKP dapat memiliki pattern yang berulang setiap 360°.
- CMP/CMP2 dapat memiliki event yang ditempatkan sepanjang 720°.
- RPM menentukan kecepatan eksekusi posisi tersebut.
- Pattern tidak berubah ketika RPM berubah; time base yang berubah.

### Contoh

```text
CKP = 36-1
CMP rising = event pada sudut tertentu
CMP falling = event pada sudut tertentu
RPM = 800
```

Engine position kemudian diterjemahkan oleh scheduler menjadi timestamp/timer event.

---

## 9. CKP Parameter

### Basic

- Pattern Type.
- Total Tooth/Slot.
- Missing Tooth Count.
- Missing Position.
- Tooth Pitch.
- Tooth Width/Duty.
- CKP Offset.
- Reference.
- Direction.
- Polarity.

### Pattern Type

Minimal:

- Missing Tooth.
- Single Tooth.
- Multi Tooth.
- Irregular.
- Custom/Event Table.

### Future

- Non-uniform tooth spacing.
- Captured waveform replay.
- Pattern morphing/phase correction.

---

## 10. CMP / CMP2 Parameter

Setiap channel CMP harus dapat dikonfigurasi secara independen.

### Parameter

- Enable/Disable.
- Event Count.
- Rising Events.
- Falling Events.
- Start Angle.
- End Angle.
- Pulse Width.
- Phase Reference.
- Polarity.
- Electrical Level.
- Output Mode.

### Event Model

Untuk universal mode, CMP tidak hanya disimpan sebagai satu pulse.

Contoh:

```text
Event 1 = Rising  @ 120°
Event 2 = Falling @ 180°
Event 3 = Rising  @ 420°
Event 4 = Falling @ 470°
```

Model event table diprioritaskan untuk pattern kompleks.

---

## 11. RPM / Run Mode

### Mode

- STOP.
- CRANKING.
- FIXED RPM.
- IDLE.
- ACCELERATION.
- DECELERATION.
- REV LIMIT.
- CUSTOM RPM PROFILE.

### Parameter

- Cranking RPM.
- Running RPM.
- Target RPM.
- Acceleration rate.
- Deceleration rate.
- RPM step.
- RPM limit.

Tujuannya agar ECU tidak hanya melihat kondisi mesin yang langsung berada pada RPM tetap, tetapi dapat diuji dalam kondisi cranking → start → idle → perubahan RPM.

---

## 12. Electrical Interface Engine

Setiap output channel harus memiliki konfigurasi electrical terpisah dari pattern.

### Mode Target

```text
0–5 V Push-Pull
0–12 V Push-Pull
Open Collector
Open Drain
VR / Inductive
```

### Parameter

- Voltage High.
- Voltage Low.
- Polarity.
- Source/Sink behavior.
- Pulse width.
- Edge characteristic.
- Amplitude untuk mode analog/VR.

Catatan: electrical interface harus mengikuti karakter input ECU yang sebenarnya; pattern mekanis yang sama tidak otomatis berarti interface listrik yang sama.

---

## 13. Capture Engine

Capture Engine bertujuan mengambil waveform dari kendaraan sehat kemudian mengubahnya menjadi pattern yang bisa direproduksi.

### Input Target

- CKP.
- CMP.
- CMP2.

### Data yang Dicapture

- Timestamp rising edge.
- Timestamp falling edge.
- Interval antar edge.
- Pulse width.
- Amplitude.
- Polarity.
- Missing tooth/gap.
- Hubungan CKP ↔ CMP.
- Hubungan CKP ↔ CMP2.

### Alur

```text
Vehicle Healthy
      ↓
Capture
      ↓
Edge Detection
      ↓
Interval Analysis
      ↓
Pattern Recognition
      ↓
Angle Normalization
      ↓
Pattern Database
      ↓
Replay / Generator
```

### Dua Mode Pattern

**Parametric Pattern**

Contoh: `36-1 + CMP phase`.

**Captured Pattern**

Pattern berasal dari hasil pengukuran kendaraan dan direpresentasikan sebagai event/timing data.

---

## 14. Pattern Database

Database tidak boleh bergantung penuh pada nama kendaraan.

### Struktur konsep

```text
Pattern
├── Metadata
├── Mechanical
│   ├── Engine cycle
│   ├── CKP pattern
│   ├── Reference
│   └── Phase
├── Events
│   ├── CKP
│   ├── CMP
│   └── CMP2
├── Electrical
└── Validation Notes
```

Nama kendaraan hanya metadata tambahan.

Contoh:

```text
PATTERN_ID: CUSTOM_001
LABEL: Vehicle A / ECU X
CKP: 36-1
REFERENCE: TDC
CMP_EVENTS: custom
ELECTRICAL: Hall 5V
```

---

## 15. Validation Engine

Tujuan validation adalah mengubah hasil test dari sekadar “ECU hidup” menjadi pengujian yang lebih terukur.

### Input/Monitoring yang direncanakan

- ECU RPM.
- Ignition trigger.
- Injector trigger.
- Sync status bila tersedia.
- CKP/CMP output monitor.
- DTC melalui komunikasi pada fase lanjutan.

### Contoh hasil

```text
CKP              PASS
CMP              PASS
CMP2             OFF
ECU RPM          798
SYNC             PASS
IGNITION         PASS
INJECTOR         PASS
CKP DTC          NONE
CMP DTC          NONE
CORRELATION      PASS
```

Validation tidak boleh menyatakan ECU “100% sehat” hanya dari satu pengujian; hasil harus dianggap sebagai **bench validation berdasarkan skenario yang dijalankan**.

---

## 16. Arsitektur Menu Prototype

```text
ENGINE SIGNAL GENERATOR
│
├── ENGINE SETUP
├── CKP
├── CMP
├── CMP2
├── RPM / RUN MODE
├── ELECTRICAL
├── CUSTOM PATTERN
├── CAPTURE
├── VALIDATION
├── PATTERN MEMORY
├── SENSOR SIMULATOR     [FUTURE]
├── COMMUNICATION        [FUTURE]
│   ├── CAN
│   ├── LIN
│   └── K-LINE
└── SYSTEM
```

---

## 17. Prototype OLED UI

Target OLED 1.3 inch dipakai untuk menguji arsitektur menu sebelum menggunakan TFT yang lebih besar.

### Main Screen

```text
ENGINE SIGNAL GEN

CKP   36-1      ON
CMP   CUSTOM    ON
CMP2  OFF

RPM   0850
ANGLE 012.5°

OUT   5V HALL
RUN   YES
```

### CKP Screen

```text
CKP SETUP

Pattern : 36-1
Teeth   : 36
Missing : 1
RPM     : 850
Polarity: NORMAL

> EDIT
```

### CMP Screen

```text
CMP SETUP

Events  : 2
Rising  : 120.0°
Falling : 180.0°
Phase   : CUSTOM
Polarity: NORMAL

> EVENT TABLE
```

### Capture Result

```text
CAPTURE RESULT

Teeth      36
Missing     1
RPM       742

CMP Rising 118.4°
CMP Fall   302.1°

> SAVE
> ANALYZE
```

---

## 18. Future Communication Architecture

Platform dirancang mempunyai expansion bus agar komunikasi kendaraan dapat ditambahkan tanpa mengganti core engine.

### CAN

Target kemampuan:

- CAN monitor.
- CAN transmit.
- CAN receive.
- CAN replay.
- Periodic frame.
- Scenario/test sequence.
- F-CAN/B-CAN sebagai profile/network classification, bukan protokol yang berbeda.
- Future CAN FD.

### LIN

Target kemampuan:

- Master mode.
- Slave simulation pada skenario tertentu.
- Header/response handling.
- Schedule based test.

### K-Line

Target kemampuan:

- Physical layer interface.
- UART-based framing.
- ISO 9141 / KWP family support sesuai kebutuhan implementasi.
- Diagnostic request/response.

### Prinsip

Communication Engine harus terpisah dari Engine Signal Engine.

---

## 19. Roadmap

### Phase 1 — Foundation

- ESP32.
- OLED 1.3 inch.
- Menu framework.
- Rotary encoder.
- Engine position model 0–720°.
- CKP 36-1 generator.
- Basic CMP generator.
- CMP2 architecture.

### Phase 2 — Timing Engine

- Hardware timer/event scheduler.
- RPM engine.
- Cranking mode.
- RPM profile.
- Precise rising/falling event.
- Pattern parameterization.

### Phase 3 — Electrical Interface

- 0–5 V output.
- 0–12 V output.
- Open collector/open drain.
- Protection layer.
- Output monitor.

### Phase 4 — Capture

- 3-channel capture.
- Timestamp.
- Edge detection.
- Missing tooth detection.
- Angle normalization.
- Pattern analyzer.
- Pattern save/replay.

### Phase 5 — Advanced Signal

- VR CKP/CMP simulation.
- Complex/custom pattern.
- Disturbance injection.
- Jitter/noise/dropout testing.

### Phase 6 — Sensor Simulator

- VSS.
- Analog voltage sensors.
- PWM.
- Resistance simulation.
- Fuel/temperature.

### Phase 7 — Communication

- CAN.
- F-CAN/B-CAN profiles.
- LIN.
- K-Line.
- CAN replay/scenario.

### Phase 8 — ECU Validation Platform

- Test scenario.
- Automated validation.
- DTC acquisition.
- Measurement logging.
- Pass/fail report.

---

## 20. Non-Functional Requirements

### Safety / Robustness

- ESP32 tidak boleh terhubung langsung ke interface otomotif berisiko.
- Output harus memiliki current limiting/protection yang sesuai.
- Input capture harus memiliki protection dan conditioning.
- Salah sambung 12 V/ground fault harus diminimalkan dampaknya ke MCU.
- Hardware output final harus diuji menggunakan dummy load sebelum dihubungkan ke ECU.

### Timing

- Generator tidak boleh bergantung pada software delay sederhana untuk timing final.
- Timing engine menggunakan hardware timer/peripheral yang sesuai.
- Scheduler harus mampu mengatur beberapa channel secara sinkron.

### Maintainability

- Pattern engine tidak bergantung pada UI.
- Display driver dipisahkan dari application logic.
- Electrical driver dipisahkan dari pattern logic.
- Communication protocol dipisahkan dari core timing engine.

### Expandability

- Minimal menyediakan internal expansion interface berbasis SPI/UART/I2C/GPIO sesuai kebutuhan modul.
- Slot/modul masa depan tidak boleh memaksa redesign total motherboard.

---

## 21. Acceptance Criteria Prototype V0.1

Prototype dianggap berhasil apabila:

- Menu dapat dinavigasi melalui OLED.
- Parameter CKP dapat diedit dan disimpan.
- CKP 36-1 dapat dijalankan pada RPM yang dapat diubah.
- CMP dapat disinkronkan terhadap engine position.
- CMP2 dapat diaktifkan dan diberi event terpisah.
- Rising/falling event dapat diuji pada posisi sudut yang ditentukan.
- RUN/STOP dapat dilakukan dengan aman.
- Waveform dapat diverifikasi menggunakan oscilloscope/logic analyzer.
- Display tidak bergantung pada logic generator.
- Struktur data pattern dapat disimpan untuk pengembangan capture/replay.

---

## 22. Keputusan Desain Awal

| Item | Keputusan V0.1 |
|---|---|
| MCU | ESP32 |
| Display | OLED 1.3 inch |
| UI | Rotary encoder + tombol |
| Engine position | 0–720° |
| CKP | Programmable |
| CMP | Programmable |
| CMP2 | Programmable |
| Timing | Hardware timer/event scheduler |
| Pattern | Parametric + future captured |
| Storage | ESP32 Flash, future microSD |
| Isolation | Dipersiapkan sejak awal |
| Electrical output | Modular, dimulai dari digital |
| Capture | Dirancang sejak arsitektur awal |
| CAN/LIN/K-Line | Future expansion |

---

## 23. Prinsip Utama Proyek

> **Jangan membuat generator berdasarkan “nama mobil”. Buat engine yang memahami posisi, pattern, electrical interface, dan komunikasi. Nama kendaraan hanya menjadi metadata.**

> **ECU bisa hidup bukan berarti ECU sudah tervalidasi. Target platform adalah membuat kondisi bench sedekat mungkin dengan kondisi kendaraan yang sebenarnya.**

> **Prototype boleh sederhana, tetapi arsitekturnya tidak boleh buntu.**

---

## 24. Status Dokumen

Dokumen ini merupakan **PRD awal / living document**. Parameter teknis hardware, jenis transceiver, topologi output, spesifikasi VR, dan protokol komunikasi final akan ditentukan setelah fondasi software dan electrical interface dipahami serta diuji secara bertahap.
