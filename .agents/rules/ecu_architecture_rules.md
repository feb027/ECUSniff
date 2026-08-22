---
description: "Arsitektur dan pedoman pengembangan Automotive ECU Test Platform (ECUSniff)"
always_on: true
---

# ECU Test Platform Architecture & Engineering Rules

## 1. Modular Layering Principle (PRD §7)
Kode harus dipisahkan secara ketat menjadi 4 layer:
- **`lib/engine/`**: 100% Hardware-agnostic C++. Berisi model posisi mesin 0–720°, logika pattern crank/cam, kalkulasi sudut-ke-waktu, dan state machine RPM. Layer ini **tidak boleh** memanggil fungsi Arduino/ESP32 (seperti `digitalWrite`, `millis`, `delay`, dll) agar bisa di-unit-test di host.
- **`lib/hal/`**: Hardware Abstraction Layer. Mengimplementasikan adapter untuk peripheral ESP32/ESP32-S3 (RMT driver untuk CKP/CMP, LovyanGFX adapter untuk display TFT 4.0", interrupt handler untuk rotary encoder, storage NVS/LittleFS).
- **`lib/webapi/`**: Async Web Server, WebSocket endpoint, dan REST API. Berjalan sepenuhnya di Core 0.
- **`lib/ui/`**: Manajemen tampilan grafis pada layar TFT (LovyanGFX).
- **`src/main.cpp`**: FreeRTOS task scheduling dan wiring antar-layer.

## 2. Hard Code Limit
- **Maksimal 300 baris per file** (hard rule, idealnya $\le 200$ baris).
- Pecah file besar menjadi sub-komponen atau helper spesifik dengan tanggung jawab tunggal (Single Responsibility Principle).

## 3. Dual-Core FreeRTOS Isolation
- **Core 1 (Real-Time Signal Core):**
  - Khusus menangani Timing Engine / RMT driver / ISR sinyal mesin.
  - **STRICTLY NO heap allocation (`malloc`, `new`, `String`, `std::vector` resize)** saat generator dalam status RUNNING.
  - Bebas dari latensi WiFi/TCP/Display SPI.
- **Core 0 (Networking & UI Core):**
  - Menjalankan WiFi AP/STA, ESPAsyncWebServer, WebSocket broadcast (5–20 Hz), UI rendering LovyanGFX, dan polling encoder/tombol.

## 4. Hardware Pinout Reference
Pastikan seluruh konfigurasi HAL mengacu pada pin berikut:
- **Display 4.0" TFT SPI 480x320 (KMRTM40045-SPI / ILI9488):**
  - `PIN_TFT_CS`: GPIO 32
  - `PIN_TFT_RST`: GPIO 17
  - `PIN_TFT_DC`: GPIO 33
  - `PIN_TFT_MOSI`: GPIO 13
  - `PIN_TFT_SCK`: GPIO 19
  - `PIN_TFT_LED`: GPIO 16 (Backlight)
- **Rotary Encoder:**
  - `PIN_ENC_CLK`: GPIO 14
  - `PIN_ENC_DT`: GPIO 23
  - `PIN_ENC_SW`: GPIO 27
- **Engine Signal Outputs:**
  - `PIN_SIG_CKP`: GPIO 25 (RMT CH0)
  - `PIN_SIG_CMP`: GPIO 26 (RMT CH2)
  - `PIN_SIG_CMP2`: GPIO 18 (RMT CH4)

## 5. Web Assets & Network Constraints
- Perangkat beroperasi default sebagai **WiFi AP mandiri** (`192.168.4.1`) tanpa akses internet.
- **DILARANG menggunakan CDN eksternal** untuk library frontend. Semua file HTML, CSS, dan JS harus di-serve dari LittleFS.

## 6. Display Hardware & True Micro-Partial Rendering
- **Driver Module:** Layar 4.0" KMRTM40045-SPI menggunakan driver `lgfx::Panel_ILI9488` (18-bit color format) pada clock SPI **40 MHz** via `HSPI_HOST`.
- **Anti-Flicker Standard:**
  - **DILARANG** menggambar ulang kotak container background (`fillRoundRect` / `fillRect`) saat nilai parameter berubah.
  - Kotak background dan judul halaman hanya digambar 1 kali (`fullRedraw`).
  - Pembaruan nilai numerik wajib dilakukan secara *in-place* dengan menimpa karakter teks langsung menggunakan background color text (`setTextColor(fg, bg)`) dan fixed-width formatting (`%3u`, `%-3d`).

## 7. Modular UI Architecture (1 File per Tab)
- Setiap tab atau layar antarmuka TFT **WAJIB** dipisahkan ke dalam class dan file mandiri (`page_<name>.h` & `page_<name>.cpp` di `lib/ui/`).
- `menu_manager.cpp` hanya bertindak sebagai koordinator tab bar dan delegasi event encoder.
- Ukuran file UI tidak boleh melebihi batas 300 baris.

## 8. Direct Hardware Encoder ISR Standard
- Driver rotary encoder wajib menggunakan direct GPIO interrupt (`attachInterrupt` pada pin CLK dan DT) dengan Gray Code state table lookup di IRAM, bebas dari polling delay atau library wrapper yang memicu lag.

## 9. LittleFS Standalone Frontend Architecture
- Dilarang menanamkan string literal HTML/CSS/JS besar ke dalam file sumber C++.
- Seluruh aset antarmuka web wajib disimpan sebagai file murni di dalam direktori `data/` (`data/index.html`, `data/css/style.css`, `data/js/*.js`).
- Pustaka pola roda otomotif wajib didefinisikan secara modular di `data/js/wheel_db.js` agar scalable.
- Unggah file web ke memori Flash ESP32 menggunakan perintah LittleFS PlatformIO:
  `pio run -e wemos_d1_r32 -t uploadfs --upload-port <PORT>`

## 10. High-Density Waveform Visualization Standard (Mobile Scope)
- Dilarang memampatkan siklus 720° CKP (36–60 gigi) secara mentah ke dalam layar ponsel sempit (karena akan memadat menjadi balok warna).
- Visualisasi osiloskop pada perangkat bergerak wajib menerapkan arsitektur **Dual-Layer**:
  1. *Minimap Strip 720°* di bagian atas untuk navigasi posisi.
  2. *Main Zoomed Canvas* di bawah (rentang default 60°–180°) yang dilengkapi interaktivitas **Touch Pan/Swipe** dan label penomoran gigi (`T1`, `T2`, `GAP`).

## 11. Zero-Wipe Smooth Tab Transition Standard
- DILARANG memanggil `fillScreen(TFT_BLACK)` saat pengguna berpindah tab di dalam modul yang sama.
- Tab bar atas wajib bersifat persisten (terkunci permanen) dan hanya memperbarui warna tombol tab yang berubah secara delta.
- Halaman konten bawah ($y=48..320$) digambar langsung di atas kanvas konten tanpa menyapu seluruh layar hitam terlebih dahulu.

## 12. High-Contrast Workshop & Senior-Friendly Palette Standard
- Layar TFT SPI (bukan OLED) memiliki sudut pandang terbatas; DILARANG menggunakan warna teks abu-abu redup (`0x8410`, `0x9CD3`, `0x6BC9`).
- Seluruh teks wajib menggunakan palet kontras tinggi:
  - Judul / Label Utama: Putih Terang (`0xFFFF`, Size 2).
  - Deskripsi / Sub-label: Kuning Emas Terang (`0xFFE0`, Size 1) atau Cyan Terang (`0x07FF`, Size 1).
  - Angka Nilai / Output: Kuning Emas Raksasa (`0xFFE0`, Size 3 atau 4).
  - Border Aktif / Seleksi: Hijau Terang (`0x07E0`, 2px) atau Kuning Terang (`0xFFE0`, 2px).

## 13. Real-Time Parameter Delta & Explicit Coordinates
- Seluruh halaman edit (DASH, CKP, CMP) WAJIB menyimpan struct snapshot nilai sebelumnya (`_lastWheel`, `_lastRpm`, dll) untuk mendeteksi perubahan nilai saat encoder diputar di baris yang sama.
- DILARANG menggabungkan `drawString` dengan `printf` berturut-turut tanpa set cursor. Seluruh pencetakan angka wajib diformat ke buffer (`snprintf`) dan digambar menggunakan `drawString(buf, fixedX, fixedY)` dengan koordinat absolut terkunci.

## 14. Full-Height 480x320 Viewport Utilization Standard
- DILARANG menyisakan margin kosong hitam di bawah layar pada display 4.0" 480x320.
- Struktur geometri baku pada layar TFT 480x320:
  - **TabBar Top**: $Y = 0 \dots 42$ (tinggi 42px).
  - **Main Content Card**: $X = 8$, $Y = 44$, Lebar = 464px, Tinggi = 268px (merentang hingga $Y=312$).
  - **Mini Scope Canvas**: Lebar $\ge 440\text{px}$, Tinggi $\ge 95\text{px}$.
  - **Form / Parameter Rows**: Tinggi baris $\ge 40\text{px}$ dengan jarak vertikal $\ge 45\text{px}$.
  - **Bottom Action / Helper Banner**: Terletak presisi di dasar kartu ($Y = 282 \dots 306$).

## 15. Comprehensive Polyglot File Line Limit & Frontend Modularization
- Batas keras maks 300 baris per file (target $\le 220-250$ baris) berlaku mutlak untuk **SEMUA jenis file** (`.cpp`, `.h`, `.hpp`, `.js`, `.html`, `.css`, `.py`).
- Skrip audit baris `check_lines.py` wajib memeriksa seluruh direktori termasuk `data/`.
- Frontend JavaScript wajib dipecah menjadi modul-modul mandiri berdasar domain tanggung jawab tunggal (`wheel_db.js`, `ui_database.js`, `ui_tuner.js`, `scope.js`, `app.js`).

## 16. Full-Duplex Master-Master Real-Time Synchronization Standard
- Seluruh aksi di Web Studio (ganti modul, tab, pilih database roda, slider RPM, form edit) wajib langsung menggerakkan layar fisik TFT dan output GPIO sinyal mesin secara instan.
- Begitu pula putaran knob rotary fisik wajib menyiarkan state terkini (`uiLevel`, `tab`, `ckp`, `cmp`, `rpm`, `mode`) via WebSocket @ 10Hz untuk menggerakkan antarmuka Web secara real-time.
- Sinkronisasi tab wajib menyertakan proteksi fokus halaman (misal: penelusuran katalog database tidak boleh terlempar oleh pembaruan tab fisik).

## 17. Automotive Signal Sniffer & Capture Pipeline Standard
- Input ISR Capture (GPIO 34 & 35) wajib menerapkan Digital Glitch Filter $\ge 5\mu s$ untuk menyaring derau induksi busi/koil di ruang mesin.
- Dekoder sinyal wajib mengintegrasikan modul Auto-Match Database OEM (Toyota, Honda, Bosch, Yamaha, Mazda, Suzuki) beserta persentase kecocokan (*confidence %*).
- Modul web wajib menyediakan endpoint ekspor log rekaman mikrodetik (`/api/export_csv`) untuk analisis spreadsheet dan logic analyzer (Saleae / Sigrok PulseView).

