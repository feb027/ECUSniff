---
name: ecu-pattern-designer
description: "Kalkulasi pola sinyal otomotif (0-720 deg cycle), formula missing tooth wheel (36-1, 60-2), sinkronisasi phase CMP/CMP2, dan konversi ke buffer pulse train ESP32 RMT."
---

# ECU Pattern Designer & Timing Math Guide

Panduan teknis dan formula matematis untuk merancang, memvalidasi, dan mengonversi pola sinyal crankshaft (CKP) dan camshaft (CMP/CMP2) untuk simulator ECU.

---

## 1. Konsep Siklus Mesin 4-Tak (0° – 720°)

Sistem internal menggunakan sudut putaran poros engkol (crankshaft) dari **0.0° sampai 720.0°**:
- **Siklus 1 (0° – 360°):** Langkah Hisap (Intake) & Kompresi (Compression)
- **Siklus 2 (360° – 720°):** Langkah Usaha (Power) & Buang (Exhaust)

Poros bubungan (Camshaft) berputar dengan rasio **1:2** terhadap Crankshaft:
- 1 putaran Camshaft (360°) = 2 putaran Crankshaft (720°).

---

## 2. Rumus Dasar Time-Base & RPM

Kalkulasi waktu berbasis microsecond ($\mu\text{s}$) pada target $RPM$:

1. **Waktu 1 Putaran Crankshaft (360°):**
   $$T_{\text{rev}} = \frac{60 \times 10^6}{RPM}\ \mu\text{s}$$

2. **Waktu 1 Siklus Lengkap 4-Tak (720°):**
   $$T_{\text{cycle}} = 2 \times T_{\text{rev}} = \frac{120 \times 10^6}{RPM}\ \mu\text{s}$$

3. **Waktu per 1 Derajat Sudut Mesin:**
   $$T_{\text{deg}} = \frac{T_{\text{rev}}}{360} = \frac{166666.67}{RPM}\ \mu\text{s/deg}$$

### Tabel Contoh Waktu Berdasarkan RPM:
| RPM | 1 Putaran 360° ($T_{\text{rev}}$) | 1 Siklus 720° ($T_{\text{cycle}}$) | Waktu per 1° ($T_{\text{deg}}$) |
|---|---|---|---|
| **200 (Cranking)** | 300,000 $\mu\text{s}$ (300 ms) | 600,000 $\mu\text{s}$ (600 ms) | 833.33 $\mu\text{s}$ |
| **850 (Idle)** | 70,588 $\mu\text{s}$ (70.59 ms) | 141,176 $\mu\text{s}$ (141.18 ms) | 196.08 $\mu\text{s}$ |
| **3,000 (Cruise)** | 20,000 $\mu\text{s}$ (20 ms) | 40,000 $\mu\text{s}$ (40 ms) | 55.56 $\mu\text{s}$ |
| **6,000 (High)** | 10,000 $\mu\text{s}$ (10 ms) | 20,000 $\mu\text{s}$ (20 ms) | 27.78 $\mu\text{s}$ |

---

## 3. Desain Pola Crankshaft (CKP Missing Tooth)

Untuk roda bergigi dengan tipe $N - M$ (misal $36-1$ atau $60-2$):
- $N_{\text{total}}$: Jumlah total gigi teoritis (misal 36 atau 60)
- $M_{\text{missing}}$: Jumlah gigi yang dihilangkan sebagai penanda sinkronisasi (gap)
- $N_{\text{active}} = N_{\text{total}} - M_{\text{missing}}$

### Parameter Gigi:
- **Tooth Pitch Angle:**
  $$\theta_{\text{pitch}} = \frac{360.0^\circ}{N_{\text{total}}}$$
  *(Contoh: Pada 36-1, pitch = $360/36 = 10.0^\circ$)*
- **Tooth High Duration (Angle):**
  $$\theta_{\text{high}} = \theta_{\text{pitch}} \times \text{DutyCycle}$$ (Standar 50% = $5.0^\circ$)
- **Tooth Low Duration (Angle):**
  $$\theta_{\text{low}} = \theta_{\text{pitch}} \times (1 - \text{DutyCycle})$$ (Standar 50% = $5.0^\circ$)

---

## 4. Desain Pola Camshaft (CMP / CMP2 Event Table)

CMP dimodelkan sebagai **Event Table** berbasis sudut 0°–720°:
Setiap event berisi: `{ angle_deg, level (HIGH/LOW) }`.

Contoh Pola CMP 4-Cylinder Universal:
- Event 1: Rising @ 120.0° (Level = HIGH)
- Event 2: Falling @ 180.0° (Level = LOW)
- Event 3: Rising @ 420.0° (Level = HIGH)
- Event 4: Falling @ 470.0° (Level = LOW)

---

## 5. Konversi & Aturan Hardware ESP32 RMT untuk Continuous Looping

1. **EOT Zero-Terminator Marker (Wajib):**
   - Setelah item gigi terakhir (misal item index 35 pada roda 36 gigi), slot berikutnya (index 36) **WAJIB** diisi dengan:
     `{ duration0 = 0, level0 = 0, duration1 = 0, level1 = 0 }`.
   - Tanpa EOT marker ini, hardware RMT akan terus membaca sisa slot RAM yang tidak terinisialisasi sehingga menghasilkan pulsa panjang liar.

2. **Pembersihan Sisa RAM Buffer:**
   - Bersihkan sisa slot dalam buffer array dengan nilai 0 sebelum ditransmisikan.

3. **Pengisian Buffer Tanpa ISR Overhead:**
   - Gunakan `rmt_fill_tx_items(channel, items, count + 1, 0)`.
   - Aktifkan loop kontinu di hardware: `rmt_set_tx_loop_mode(channel, true)` dan `rmt_tx_start(channel, true)`.

4. **Alokasi Blok Memori Bebas Konflik (ESP32):**
   - Channel 0 (CKP): 2 Blocks (Blocks 0, 1 = 128 slot).
   - Channel 2 (CMP): 2 Blocks (Blocks 2, 3 = 128 slot).
   - Channel 4 (CMP2): 2 Blocks (Blocks 4, 5 = 128 slot).
