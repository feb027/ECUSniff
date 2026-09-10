# Desain Hardware: Regulator Linier Terkontrol I2C (5V - 12V) untuk Suplai Driver Totem-Pole

Dokumen ini menjelaskan rancangan rangkaian **Regulator Tegangan Linier Presisi** yang dikontrol oleh mikrokontroler ESP32-S3 via bus **I2C (MCP4725 DAC)** untuk menyuplai rel tegangan ($V_{\text{RAIL}}$) pada driver output **Totem-Pole (Push-Pull NPN-PNP)**. Rangkaian ini mengendalikan amplitudo pulsa sinyal **CKP, CMP, Tacho RPM, dan VSS Speedometer** secara dinamis dari **5.0V hingga 12.0V**.

---

## 1. Latar Belakang & Masalah Teknis

Pada pengujian ECU mobil, modul EPS (Electric Power Steering), dan Speedometer cluster, variasi tipe sensor di lapangan memerlukan amplitudo pulsa yang berbeda:
1. **Sensor Digital Hall-Effect (5.0V):** Standar pada sensor CKP/CMP mesin modern (Toyota, Honda, Suzuki) dan input mikrokontroler ECU 5V.
2. **Standar Sensor Modul EPS / Eropa (8.0V):** Banyak modul EPS dan sensor kecepatan transmisi mensyaratkan tegangan logic high threshold minimal 6.5V - 8.0V agar assist motor aktif.
3. **Speedometer Analog / Sensor Reluctor Coil (12.0V):** Jarum speedometer analog dan tacho meter koil membutuhkan pulsa ayunan penuh mendekati tegangan baterai/aki (+12V s.d. +14V) untuk menggerakkan kumparan cross-coil.

### Mengapa Harus Regulator Linier (Bukan Buck Converter)?
- **Switching Noise:** Modul step-down DC-DC (seperti LM2596 atau MP1584) menghasilkan switching ripple frekuensi tinggi (100 kHz - 1.5 MHz) sebesar 30 - 100 mVpp. Noise ini dapat merembes ke jalur sinyal pulsa dan memicu *false trigger* atau gangguan sinkronisasi pada input ECU.
- **Pure DC Output:** Kombinasi **Op-Amp LM358 + Transistor Daya BD139** menghasilkan tegangan DC murni dengan ripple $< 1\text{ mV}$, respon transien sangat cepat saat beban pulsa beralih, dan biaya komponen sangat murah ($< \text{Rp } 25.000$).

---

## 2. Diagram Skema Rangkaian

Skema rangkaian lengkap telah dirender dalam format definisi tinggi:
- **File Gambar PNG:** `docs/skema_regulator_linier_i2c_amplitude.png`
- **File Vektor SVG:** `docs/skema_regulator_linier_i2c_amplitude.svg`

```
                       +14V / AKI IN (Supply DC Utama)
                             │
            ┌────────────────┴───────────────┐
            │ Pin 8                          │ Collector
      ┌─────┴─────┐                     ┌────┴────┐
      │  VCC OpAmp│                     │  Q_pass │ BD139 / TIP41
      │   LM358   │                     │  (NPN)  │ (Series Pass)
      │           │    R_base (220Ω)    │         │
      │    Pin 1  ├──────[====]─────────┤ Base    │
      │   (Output)│                     │         │
      │           │                     └────┬────┘
      │           │                          │ Emitter
      │           │                          ├───────────────────────> V_RAIL
      │   Pin 3   │                          │                        (5V - 12V)
      │    (+)    │                          ├───[ 10µF ]───┬─── GND  (Ke Totem-Pole)
      │           │                          ├───[ 100nF]───┤
      │   Pin 2   │                          │              │
      │    (−)    │                          ▼              │
      └─────┬─────┘                   R1 (30kΩ, 1%)         │
            │                                │              │
            ├────────────────────────────────┴──┐           │
            │ (Jalur Umpan Balik / Feedback)    ▼           │
            │                             R2 (10kΩ, 1%)     │
            │                                   │           │
            │                                  GND         GND
   V_DAC    │
 (0 - 3V)   │
    ▲       │
    │       │
┌───┴───────┴──────┐
│  MCP4725 (0x62)  │
│  12-Bit I2C DAC  │
│  VCC = 3.3V      │
└───┬───────┬──────┘
    │ SDA   │ SCL
    ▼       ▼
   ESP32-S3 (Pin 13 & Pin 21)
```

---

## 3. Analisa Rangkaian & Formula Matematis

### A. Alokasi Alamat I2C
- Bus I2C default ESP32-S3: **SDA = GPIO 13**, **SCL = GPIO 21**.
- Modul DAC EPS menggunakan alamat `0x60` (TRQ1) dan `0x61` (TRQ2).
- Modul DAC Amplitude Regulator dikonfigurasi ke alamat **`0x62`** (MCP4725A1) atau `0x63`, sehingga tidak terjadi tabrakan alamat (*bus collision*).

### B. Rumus Penguatan Umpan Balik (Negative Feedback)
Tegangan referensi $V_{\text{ref}}$ dihasilkan oleh output MCP4725 ($V_{\text{DAC}}$).
Op-Amp LM358 berfungsi sebagai *Error Amplifier* dengan loop tertutup:

$$V_{\text{fb}} = V_{\text{RAIL}} \times \frac{R_2}{R_1 + R_2}$$

Karena Op-Amp menjaga $V_+ = V_-$ pada kondisi setimbang ($V_{\text{ref}} = V_{\text{fb}}$):

$$V_{\text{RAIL}} = V_{\text{DAC}} \times \left(1 + \frac{R_1}{R_2}\right)$$

Dengan memilih $R_1 = 30\text{ k}\Omega$ (atau $20\text{k} + 10\text{k}$ seri 1%) dan $R_2 = 10\text{ k}\Omega$ (1%):

$$\text{Faktor Penguatan (Gain)} = 1 + \frac{30\text{ k}\Omega}{10\text{ k}\Omega} = 4.0\times$$

Maka relasi output terhadap DAC adalah:

$$\mathbf{V_{\text{RAIL}} = 4.0 \times V_{\text{DAC}}}$$

### C. Tabel Preset Nilai I2C DAC (MCP4725 VCC = 3.3V)

Resolusi DAC 12-bit: $\text{Step} = \frac{3.3\text{V}}{4095} \approx 0.8058\text{ mV}$.
Karena penguatan adalah $4.0\times$, resolusi tegangan $V_{\text{RAIL}}$ adalah:
$$\Delta V_{\text{RAIL}} = 0.8058\text{ mV} \times 4.0 \approx \mathbf{3.22\text{ mV / step}}$$

| Target $V_{\text{RAIL}}$ | Kebutuhan $V_{\text{DAC}}$ | Nilai DAC (Desimal) | Nilai DAC (Hex) | Peruntukan Sinyal Otomotif |
|---|---|---|---|---|
| **5.00 V** | $1.250\text{ V}$ | **1551** | `0x060F` | Sensor Digital Hall-Effect (CKP, CMP ECU 5V) |
| **8.00 V** | $2.000\text{ V}$ | **2482** | `0x09B2` | Modul EPS OEM & Sensor Transmisi logic 8V |
| **10.00 V**| $2.500\text{ V}$ | **3102** | `0x0C1E` | Cluster Speedometer Jepang / Universal |
| **12.00 V**| $3.000\text{ V}$ | **3723** | `0x0E8B` | Speedo Jarum Analog & Koil Tacho Reluctor 12V |

---

## 4. Analisa Termal & Pemilihan Komponen

1. **Transistor Daya $Q_{\text{pass}}$ (BD139):**
   - Tegangan input kolektor: $+14\text{V}$ (dari Aki / Power Supply).
   - Arus beban rata-rata driver Totem-Pole: $I_{\text{load}} \approx 80\text{ mA} - 150\text{ mA}$.
   - Kondisi terburuk (Worst-case disipasi daya pada $V_{\text{RAIL}} = 5.0\text{V}$):
     $$P_D = (V_{\text{IN}} - V_{\text{OUT}}) \times I_{\text{load}} = (14\text{V} - 5\text{V}) \times 0.12\text{A} = \mathbf{1.08\text{ Watt}}$$
   - **Rekomendasi:** Gunakan heatsink aluminium kecil (tipe U-clip TO-126) pada BD139. Transistor akan tetap dingin-hangat ($< 45^\circ\text{C}$). Jika menggunakan beban lebih dari 500 mA, ganti dengan TIP41C (TO-220).

2. **Op-Amp (LM358):**
   - LM358 disuplai langsung dari $+14\text{V}$ (Pin 8).
   - Tegangan saturasi maksimum output LM358 adalah $V_{\text{CC}} - 1.5\text{V} = 12.5\text{V}$.
   - Untuk menghasilkan $V_{\text{RAIL}} = 12.0\text{V}$, base BD139 membutuhkan $12.0\text{V} + 0.7\text{V} = 12.7\text{V}$. Jika tegangan aki adalah $14.4\text{V}$ (kondisi mesin hidup/charger), saturasi LM358 mencapai $12.9\text{V}$, cukup untuk $12.0\text{V}$ murni.

3. **Kapasitor Output ($10\mu\text{F} + 100\text{nF}$):**
   - Kapasitor $10\mu\text{F}$ menjaga kestabilan loop kontrol saat transistor totem-pole beralih (switching).
   - Kapasitor keramik $100\text{nF}$ membuang transient switching frekuensi tinggi ke ground.

---

## 5. Implementasi Software C++ (ESP32)

### Driver Kelas `AmplitudeDriver`

```cpp
#pragma once
#include <Arduino.h>
#include <Wire.h>

class AmplitudeDriver {
public:
    static constexpr uint8_t MCP4725_ADDR_AMP = 0x62; // A0 tied to VCC on MCP4725A1
    static constexpr float OPAMP_GAIN = 4.0f;          // 1 + 30k/10k
    static constexpr float DAC_VREF   = 3.3f;          // ESP32 3V3 Rail

    static bool init() {
        Wire.beginTransmission(MCP4725_ADDR_AMP);
        bool found = (Wire.endTransmission() == 0);
        if (found) {
            setVoltage(5.0f); // Default safe 5.0V on startup
        }
        return found;
    }

    static void setVoltage(float railVolts) {
        if (railVolts < 1.0f)  railVolts = 1.0f;
        if (railVolts > 12.5f) railVolts = 12.5f;

        // Hitung V_DAC yang dibutuhkan: V_DAC = V_RAIL / GAIN
        float vDac = railVolts / OPAMP_GAIN;
        if (vDac > DAC_VREF) vDac = DAC_VREF;

        uint16_t dacCode = static_cast<uint16_t>((vDac / DAC_VREF) * 4095.0f);
        if (dacCode > 4095) dacCode = 4095;

        // Fast Write MCP4725 (2 bytes)
        Wire.beginTransmission(MCP4725_ADDR_AMP);
        Wire.write(static_cast<uint8_t>((dacCode >> 8) & 0x0F));
        Wire.write(static_cast<uint8_t>(dacCode & 0xFF));
        Wire.endTransmission();
    }
};
```

### Integrasi Otomatis pada Halaman UI

Saat pengguna membuka halaman menu tertentu, mikrokontroler otomatis memanggil `setVoltage()` sesuai standar beban, dengan opsi fine-tuning via Rotary Encoder:

1. **Halaman CKP / CMP Wheel (`PageCkp`, `PageCmp`):**
   ```cpp
   // Saat memasuki menu CKP/CMP:
   AmplitudeDriver::setVoltage(5.0f); // Default 5.0V untuk Hall Sensor ECU
   ```
2. **Halaman Speedometer Tester (`PageSpeedoTester`):**
   ```cpp
   // Saat memasuki menu Speedo Tester:
   AmplitudeDriver::setVoltage(12.0f); // 12V untuk jarum speedometer & cluster meter
   ```
3. **Halaman EPS Bench Tester (`PageEpsTester`):**
   ```cpp
   // Saat memasuki menu EPS Tester:
   AmplitudeDriver::setVoltage(8.0f);  // 8.0V atau 12.0V sesuai tipe modul EPS
   ```

---

## 6. Daftar Komponen (BOM)

| No | Komponen | Tipe / Nilai | Keterangan |
|---|---|---|---|
| 1 | Modul DAC | **MCP4725 Breakout** | I2C 12-Bit (Address `0x62`) |
| 2 | IC Dual Op-Amp | **LM358P** (DIP-8) | Single-supply up to 32V |
| 3 | Transistor Daya | **BD139** (NPN TO-126) | $V_{CE}=80\text{V}$, $I_C=1.5\text{A}$ + Heatsink mini |
| 4 | Resistor Base | **$220\ \Omega$** (1/4W) | Pembatas arus base |
| 5 | Resistor Feedback R1 | **$30\text{ k}\Omega$** (1/4W, 1%) | Atau $20\text{k} + 10\text{k}$ seri |
| 6 | Resistor Feedback R2 | **$10\text{ k}\Omega$** (1/4W, 1%) | Divider ke Ground |
| 7 | Kapasitor Filter DAC | **$100\text{ nF}$** keramik | Peredam noise referensi DAC |
| 8 | Kapasitor Output | **$10\ \mu\text{F}$ / 25V** (Elco) | Stabilisator beban transien pulsa |
| 9 | Kapasitor Bypass | **$100\text{ nF}$** keramik | Bypass HF pada rel $V_{\text{RAIL}}$ |
