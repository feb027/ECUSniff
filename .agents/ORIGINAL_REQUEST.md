# Original User Request

## 2026-09-01T09:51:25Z

Riset dan porting komprehensif seluruh definisi pola roda gigi mesin (crankshaft & camshaft tooth patterns) dari repositori ArduStim TFTv2 Touchscreen (`external/ardustim-tftv2-touchscreen`) dan Pattern Gen (`external/pattern-gen`) ke dalam engine simulator ECUSniff, menyelaraskan seluruh penamaan preset persis sama dengan ArduStim TFTv2, serta mengimplementasikan pembangkitan sinyal berbasis bit-array / pola arbitrer (multi-gap, 36-2-2-2, 4G63, dsb) pada driver RMT ESP32-S3 dan Waveform Canvas agar sinyal 100% akurat dan ECU mobil dapat menyala dengan normal.

Working directory: `g:/semester 7/ECUSniff`
Integrity mode: development

## Requirements

### R1. Sinkronisasi Database Pola & Penamaan Identik ArduStim TFTv2
Mengekstrak dan memetakan seluruh (~70) definisi pola roda gigi dari `external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h` ke dalam database pola ECUSniff (`lib/ui/include/wheel_database.h` atau engine database) dengan menggunakan nama ramah (*friendly name*) yang 100% identik dengan ArduStim TFTv2, termasuk:
- `Toyota Avanza 1.3 Crank only` (Old Avanza)
- `Toyota Avanza 1.5 Crank only` (New Avanza)
- `Toyota Avanza/Xenia/Terios/Rush`
- `36-2-2-2 Crank and cam`, `36-2-2-2 H4 Crank only`, `36-2-2-2 H6 Crank only`
- `Mitsubishi 4g63 aka 4/2 crank and cam`, `Mitsubishi 6g72 with cam`, `Mitsubishi 3cylinder 3A92`
- `Honda Jazz/Fit 2004-2008` (V1, V2, V3), `Honda D17`, `Honda RC51 with cam`
- `Nissan Livina Juke`, `Nissan 360 CAS with 6 slots`
- Serta seluruh varian universal dan OEM lainnya dari ArduStim.

### R2. Dukungan Pembangkitan Sinyal Arbitrer / Bit-Array pada Driver RMT ESP32-S3
Memodifikasi struktur pola dan engine RMT (`lib/hal/src/rmt_generator.cpp` dan `lib/engine/`) agar mendukung konversi langsung dari array segmen/bit-array ArduStim (nilai bit 0: Low, 1: CKP, 2: CMP1, 3: CKP+CMP1, 4: CMP2, dsb) menjadi buffer pulsa RMT berdurasi mikrodetik presisi tinggi pada kecepatan RPM berapa pun, tanpa distorsi jarak gigi atau asumsi gigi ekuidistan tunggal.

### R3. Rendering Waveform Canvas untuk Pola Arbitrer
Memperbarui `WaveformCanvas` (`lib/ui/src/waveform_canvas.cpp`) agar mampu merender grafik osiloskop virtual sinyal CKP, CMP1, dan CMP2 sepanjang siklus $0 - 720^\circ$ langsung dari representasi bit-array atau tabel event sudut, sehingga bentuk gelombang pada layar TFT 4.0" persis merefleksikan pulsa riil.

### R4. Pengujian Otomatis & Verifikasi Komparatif Sinyal
Menambahkan test case unit test pada direktori `test/` untuk memvalidasi secara programatis bahwa buffer pulsa RMT yang dibangkitkan untuk pola-pola kritis (khususnya `NEW_AVANZA`, `OLD_AVANZA`, `AVANZA_XENIA_TERIOS_RUSH`, `4G63`, dan `60-2`) memiliki urutan transisi tepi (*rising/falling edge*) dan rasio sudut rotasi yang identik dengan array sumber di ArduStim.

## Acceptance Criteria

### Penamaan & Integritas Database
- [ ] Semua preset di `wheel_defs.h` ArduStim TFTv2 terdaftar pada database ECUSniff dengan penamaan string yang sama persis.
- [ ] Kategori merek (Toyota, Honda, Mitsubishi, Nissan, Euro/US, Universal, Custom) memetakan pola-pola baru dengan benar.

### Akurasi Pembangkitan Sinyal RMT
- [ ] Driver RMT ESP32-S3 berhasil mengonversi bit-array ArduStim (termasuk pola 144-segmen Avanza New/Old) menjadi pulsa berulang $0 - 720^\circ$ tanpa buffer underrun atau pergeseran fase antar-revolusi.
- [ ] Pengujian unit test komparatif timing sinyal pada PlatformIO lulus 100% (`SUCCESS`).

### Tampilan & Pengoperasian
- [ ] Waveform Canvas lebar ($456 \times 124\text{ px}$) di Menu Pilihan Pola dan Dasbor Utama menampilkan visualisasi gelombang CKP dan CMP yang akurat untuk seluruh pola bit-array.
- [ ] Firmware berhasil dikompilasi bersih (`pio run -e esp32s3`) tanpa error atau peringatan memori.
