# Panduan Hardware Antarmuka & Skematik EPS Bench Tester (ECUSniff)

Dokumen ini menjelaskan secara teknis karakteristik kelistrikan modul **EPS (Electric Power Steering)** otomotif, analisis penyebab kegagalan sinyal 5V biasa di meja tes, serta menyediakan **skematik sirkuit antarmuka (interface board)** dan **diagram pinout soket EPS** populer.

---

## 1. Analisis Karakteristik Kelistrikan Input ECU EPS

Modul ECU EPS kendaraan (Denso, Showa, Mitsubishi, NSK) dirancang dengan standar kelistrikan otomotif yang ketat:

### A. Sinyal VSS (Speed) & RPM (Tachometer)
* **Karakteristik Asli:** Bertipe **Open-Collector / Active-Low (Switch to GND)**.
* **Mekanisme Otomotif:** Sensor VSS girboks atau output Tacho ECU mesin aslinya adalah sakelar transistor NPN yang hanya bertugas menarik jalur ke **GND (0V)** saat pulsa aktif, lalu membiarkannya mengambang (*floating*) saat pasif.
* **Tegangan Pull-Up Internal:** ECU EPS sudah memiliki resistor pull-up internal ke **5V, 8V, atau 12V**.
* **Mengapa 5V Push-Pull Gagal:**
  Jika pin EPS ditarik ke 12V di dalam modul, sinyal 3.3V atau 5V push-pull hanya mampu menekan tegangan hingga ~4.5V. Rangkaian komparator / Schmitt trigger 12V di dalam ECU membutuhkan ambang batas minimal $0.7 \times 12\text{V} = 8.4\text{V}$. Karena tegangan tidak pernah mencapai 8.4V, ECU EPS menganggap sinyal **selalu LOW** (tidak ada kecepatan/mesin mati).

### B. Sinyal Sensor Torsi (TRQ1 & TRQ2)
* **Karakteristik Asli:** Sinyal analog presisi tinggi ($2.50\text{V}$ netral lurus, $1.0\text{V} - 4.0\text{V}$ saat belok).
* **Mekanisme Otomotif:** Modul EPS menyuplai tegangan referensi ($V_{\text{ref}}$ 5V atau 8V) ke sensor torsi di batang setir. Di dalam ECU terdapat resistor pull-up fail-safe ($4.7\text{ k}\Omega - 10\text{ k}\Omega$).
* **Masalah DAC Langsung:** Jika modul DAC MCP4725 dihubungkan langsung tanpa buffer, arus dari $V_{\text{ref}}$ ECU akan "berkelahi" dengan output DAC, mendistorsi voltase target, atau bahkan merusak IC DAC jika ECU mengeluarkan tegangan 8V.

---

## 2. Skematik Rangkaian Antarmuka (Interface Driver)

Rangkaian ini diletakkan di antara ESP32 dan modul EPS ECU agar alat tes aman, universal, dan kompatibel 100% dengan semua modul EPS (5V, 8V, maupun 12V).

### A. Driver Universal VSS & RPM (Open-Drain N-MOSFET)
Rangkaian ini digunakan untuk **GPIO 38 (VSS)** dan **GPIO 39 (RPM)**:

```text
               +12V (Opsional jika EPS tidak punya pull-up internal)
                │
               [R_pull: 4.7k - 10k] (Bisa dipasang jumper On/Off)
                │
Pin ESP32 ─────[R_gate: 100Ω]───┐
(GPIO 38/39)                    │   Output ke Pin VSS / RPM ECU EPS
                              ├─┴───o ─────[R_seri: 220Ω]─────> Ke Pin EPS
                              │  2N7000 / BS170
                              │  (N-Channel MOSFET)
                              ├─┬───o
                              │ │
                             [10k] [C_filter: 10nF ke GND] (RC Damper)
                              │ │
                             GND GND
```

**Kelebihan:**
1. **Toleran Tegangan Bebas:** Aman untuk ECU EPS bertegangan pull-up 5V, 8V, maupun 12V karena tegangan tinggi terisolasi di Drain MOSFET.
2. **RC Damper Filter ($220\ \Omega + 10\text{ nF}$):** Memperhalus transisi tepi pulsa menjadi ~3–5 mikrodetik, menyerupai keluaran sensor Hall roda/girboks asli dan mencegah *inductive ringing* pemicu error debounce ECU.

---

### C. Skema Universal Signal Injector (74HC14 + Transistor + AC/DC Selector)
Untuk pengujian multi-fungsi (VSS, RPM, CKP, dan CMP) yang membutuhkan fleksibilitas antara sinyal Open-Collector murni, pulsa aktif 5V/12V, serta kopling AC kapasitor (Zero-Crossing untuk ECU koil/reluctor), gunakan rangkaian lengkap pada berkas:
* **Skematik Visual:** [skema_driver_universal_revised.png](file:///c:/project/ECUSniff/docs/skema_driver_universal_revised.png)
* **Berkas Vektor:** [skema_driver_universal_revised.svg](file:///c:/project/ECUSniff/docs/skema_driver_universal_revised.svg)

Sirkuit ini dilengkapi:
1. **SW1 (3-Way Selector):** Memilih antara `[1] Open-Collector (Lepas/Floating)`, `[2] Pull-up +5V`, atau `[3] Pull-up +12V`.
2. **SW2 (Coupling Selector):** Memilih antara `[A] Direct DC` (lurus bypass) atau `[B] AC Coupling` (melewati kapasitor $100\text{ nF}$ + sepasang dioda clamping 1N4148 ke GND).

---

### B. Driver Buffer TRQ1 & TRQ2 (Dual Op-Amp Voltage Follower)
Rangkaian ini menggunakan IC Op-Amp Dual **LM358** atau **MCP6002** yang disuplai tegangan 12V (atau 5V jika tipe rail-to-rail):

```text
                +12V (Daya Meja Tes)
                 │
               ┌─┴─┐
               │   │ (Pin 8 LM358)
Output DAC ────┤+  │ 1/2 LM358 (Voltage Follower Buffer)
(MCP4725 VOUT) │   ├──────┬────────[R_out: 47Ω]──────> Ke Pin TRQ1 / TRQ2 ECU
               │-  │      │
               └─┬─┘      │
                 └─── Feedback (Pin 1 ke Pin 2)
                 │        │
                GND      [Paralel ke Pin A1/A2 ADS1115 untuk ADC Feedback]
```

**Kelebihan:**
1. **Impedansi Output Sangat Rendah ($< 1\ \Omega$):** Op-Amp mampu memaksa tegangan tetap presisi ($2.50\text{V}$, $1.2\text{V}$, dll.) tanpa terpengaruh tarikan resistor pull-up di dalam ECU EPS.
2. **Proteksi Arus & Tegangan:** Modul DAC MCP4725 terlindungi di belakang input berimpedansi tinggi Op-Amp. Resistor $47\ \Omega$ melindungi Op-Amp jika kabel tersentuh ground secara tidak sengaja.
3. **Feedback ADC ADS1115:** Titik setelah resistor $47\ \Omega$ dihubungkan ke Channel A1 (TRQ1) dan A2 (TRQ2) ADS1115 untuk pembacaan verifikasi loop tertutup (*closed-loop*).

---

## 3. Skema Common Ground & Catu Daya

Semua titik Ground **wajib disatukan** (*Common Ground*):

```text
[ Catu Daya 12V / Aki Meja Tes ] ─── GND ───┐
[ Modul EPS ECU ]                  ─── GND ───┼─── TITIK COMMON GROUND (GND)
[ ESP32-S3 DevKit ]                ─── GND ───┤
[ Modul MCP4725 & ADS1115 ]        ─── GND ───┘
```

---

## 4. Diagram Pinout Soket EPS Populer

### A. Toyota Avanza / Daihatsu Xenia / Rush / Terios (Denso)
Modul EPS Denso umumnya memiliki 2 soket utama:

1. **Soket Daya Besar (2-Pin):**
   * Pin 1: **+B** (+12V langsung Aki/PSU via Sekring 40A/50A)
   * Pin 2: **GND** (Massa bodi)

2. **Soket Kontrol & Sinyal (8-Pin / 12-Pin):**
   * **IG (+12V):** Dihubungkan ke +12V kontak (bisa via saklar relay).
   * **SPD / VSS:** Ke output driver VSS (GPIO 38 via 2N7000).
   * **TA / TACHO:** Ke output driver RPM (GPIO 39 via 2N7000). *(EPS Avanza butuh RPM > 600 agar assist aktif)*.
   * **TRQ 1 (Sub):** Ke output Buffer LM358 Channel 1 (MCP4725 0x60).
   * **TRQ 2 (Main):** Ke output Buffer LM358 Channel 2 (MCP4725 0x61).
   * **5V Ref / Sensor GND:** Dihubungkan sesuai pinout sensor setir.

### B. Suzuki Karimun Wagon R / Estilo / Ertiga (NSK / Mitsubishi)
* **+B (+12V Aki):** Soket daya tebal.
* **GND (Massa):** Soket daya tebal.
* **IG (+12V Kontak):** Pin kontrol daya.
* **VSS (Speed Pulse):** Frekuensi standar ~2548 pulsa/km (GPIO 38).
* **RPM Engine:** Pulsa Tacho 2 pulse/rev (GPIO 39).
* **TRQ1 & TRQ2:** Netral $2.50\text{V}$, span $\pm 1.50\text{V}$.

### C. Honda Jazz GD3 / GE8 / Brio (Showa)
* **Sinyal Torsi Showa:** Karakteristik DC analog terpusat di $2.43\text{V} - 2.50\text{V}$.
* **VSS & Engine NE:** Memerlukan pulsa VSS dan RPM aktif untuk menghapus error indikator EPS di speedometer.

---

## 5. Prosedur Pengujian Meja Tes Step-by-Step

1. **Persiapan:**
   * Pastikan kabel Common Ground telah terpasang rapat.
   * Nyalakan ESP32 terlebih dahulu. Masuk ke halaman **EPS Tester** dan pilih preset kendaraan yang sesuai (misal: *Toyota Avanza*).
   * Pada posisi awal, perintah torsi otomatis berada pada posisi netral $0\%$ ($2.50\text{V}$).
2. **Pemberian Daya EPS:**
   * Hubungkan daya 12V ke pin **+B** dan **IG** modul EPS.
3. **Mengaktifkan Simulasi Mesin & Kecepatan:**
   * Tekan tombol **RUN** pada ESP32 (Status berubah dari `[OFF]` ke `[RUN]`).
   * Pulsa RPM (~1200 RPM) dan VSS (~40 km/h) akan mulai keluar ke modul EPS.
4. **Pengujian Assist Belok:**
   * Putar knob atau geser joystick ke arah Kiri / Kanan.
   * Nilai TRQ1 dan TRQ2 akan bergeser secara diferensial (misal: TRQ1 naik ke $3.5\text{V}$, TRQ2 turun ke $1.5\text{V}$).
   * Modul EPS akan merespons dengan menyalakan relay motor dan mengalirkan arus bantu putar ke motor EPS.
