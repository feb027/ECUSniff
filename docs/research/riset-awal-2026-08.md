# Riset Awal — Automotive ECU Test Platform

- **Tanggal:** 2026-08-14
- **Status:** Research baseline (Phase 0). Dasar sebelum implementasi.
- **PRD acuan:** `docs/prd/Automotive_ECU_Test_Platform_PRD.md`
- **Keputusan kunci:** ESP32 + PlatformIO (Arduino framework), timing via peripheral **RMT**, UI OLED via **U8g2**, kontrol HP via **WiFi AP + web UI (WebSocket)**, arsitektur 4 lapis (`engine` → `hal` → `webapi`/`ui` → `main`), hard rule ≤300 baris/file.

---

## 1. Ringkasan

Proyek sejenis sudah banyak dan mapan di komunitas (Ardu-Stim, ArduinoJimStim, dll). Arsitektur PRD (engine position 0–720° → pattern → electrical → ECU → validation) sudah sesuai pola industri. Kelemahan referensi lama: semua memakai AVR (timer ISR, jitter, UI serial). ESP32 punya peripheral **RMT** yang didesain tepat untuk ini: menghasilkan pulse train presisi dari RAM tanpa CPU, loop di hardware, sinkron antar channel, dan RX timestamp — yang sekaligus jadi Capture Engine di fase nanti.

## 2. Riset open source

| Proyek | Stack | Status/catatan | Yang diambil |
|---|---|---|---|
| **speeduino/Ardu-Stim** (fork LibreEMS `ardu-stim`, David Andruczyk) | Arduino Nano/Uno/Mega, PlatformIO; GUI Electron | Aktif, 173★, rilisan terbaru v1.2.1 (2024-05) | Standar de-facto stim: pattern library `wheel_defs.h`, 3 output (crank/cam/2nd cam) = CKP/CMP/CMP2. Kelemahan: timing ISR 8-bit, UI serial |
| **pggood/ArduinoJimStim** | Arduino + KiCad/gerber | Open source, 44 pattern | Format pattern degree-based 360°/720° (`wheels_default.h`) cocok dengan model engine position PRD §8; acuan PCB |
| **LucasStraps/ESP32-CKP-Signal-Generator** | **ESP-IDF**, ESP32 + LCD HD44780 + 3 tombol + pot | Mit (2025-01), 8★ | Satu-satunya yang persis kasus: CKP/CMP di ESP32. Struct parametrik `syncTable[] {nama, totalTeeth, totalMissing, cmpTeeth[], cmpCount}`; skema 12V→LM7805→5V. Kelemahan: ESP-IDF, watchdog diabaikan |
| **alpauna/ESP-ECU** | ESP32-S3, arsitektur dual-core | Aktif | **Acuan arsitektur terbaik**: Core 1 = real-time timing tanpa heap/WiFi, Core 0 = app/networking; per-fungsi per-file `.cpp`; ESPAsyncWebServer + WebSocket + ArduinoJson config; OTA; safe mode. Arah kebalikan (decode CKP/CMP) tapi pola layer-nya persis yang dibutuhkan |
| **askrejans/speeduino-serial-sim** | PlatformIO, ESP32/ESP8266/AVR | Aktif | Telemetry web ~20 Hz + REST API + AP/STA fallback + mDNS — acuan pola kontrol remote |
| **elia179/OpenTurbine** | ESP32 + browser dashboard | Aktif | UX "kontrol dari HP": AP self-contained, embedded assets, WebSocket ~3 Hz, captive portal, konfigurasi tanpa compile ulang |
| **ccritix/esp32_odb2** | ESP32 + CAN transceiver | — | Bukti pola AP `192.168.4.1` + browser + REST minimal |
| **andrewrevill.co.uk — Crank & Cam Simulator** | Hardware murni (ROM 8192 sampel + DAC AD558 + transformator) | Blog teknis | Simulasi **VR/inductive analog** yang benar: voltase VR ~10Vpp idle → ~70Vpp redline; butuh DAC + buffer + transformator + isolasi. **Jangan di V0.1** (Phase 5 PRD) |
| **EcmSpy CPS Signal Generator** | AVR timer CTC, toggle per 1° crank | — | Validasi model angle→time sederhana |



### Catatan web/HP
- Asset web (HTML/CSS/JS) **di-embed di firmware/LittleFS — dilarang CDN**: HP yang connect ke AP alat tidak punya internet.
- WebSocket push live RPM/angle ~5–10 Hz; REST untuk command; serial CLI sebagai fallback.
- WiFi: mode AP default (`192.168.4.1`), STA optional (mDNS `ecu-test.local`).

## 5. Arsitektur modular (map PRD §7; ≤300 baris/file)

```text
lib/
├── engine/            # 100% hardware-agnostic, host-testable
│   ├── position_engine/    # 0-720°, TDC ref, direction
│   ├── pattern_engine/     # 36-1, 60-2, single/multi tooth, irregular
│   ├── event_scheduler/    # angle table → time table (skala RPM)
│   ├── rpm_engine/         # stop/cranking/fixed/accel/rev-limit modes
│   ├── capture_engine/     # RMT RX edge → pattern recognition (fase 4)
│   └── electrical_engine/  # push-pull/OC/OD/polarity config
├── hal/               # adapter peripheral; ganti display = 1 file
│   ├── rmt_tx/             # RMT TX CKP/CMP/CMP2
│   ├── rmt_rx/             # capture
│   ├── display/            # U8g2 wrapper
│   ├── storage/            # NVS + LittleFS
│   └── input/              # rotary + button
├── webapi/            # server, WS push, REST command layer
└── ui/                # menu app (OLED) + web assets
src/main.cpp           # wiring FreeRTOS: task timing + task UI/net
```

Pola terbukti dari ESP-ECU: logic murni (angle→time math) di lib bebas-hardware → bisa diuji di host (`pio test -e native`) seperti `eps_frame_math.hpp` di proyek CRV EPS.


## 9. Sumber

- https://github.com/speeduino/Ardu-Stim (dan fork asli https://gitlab.com/libreems-suite/ardu-stim)
- https://github.com/pggood/ArduinoJimStim
- https://github.com/LucasStraps/ESP32-CKP-Signal-Generator
- https://github.com/alpauna/ESP-ECU
- https://github.com/askrejans/speeduino-serial-sim
- https://github.com/elia179/OpenTurbine ; https://github.com/ccritix/esp32_odb2 ; https://github.com/speeduino/Airbear
- https://andrewrevill.co.uk/CrankCamSimulator.htm ; https://www.ecmspy.com/cps_signal.shtml
- ESP32 RMT: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/rmt.html ; Arduino core RMT API: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/rmt.html
- ESP32 MCPWM sync: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/mcpwm.html
- Library: https://github.com/ESP32Async/ESPAsyncWebServer (+ AsyncTCP, org resmi pasca me-no-dev archive) ; https://github.com/mathertel/RotaryEncoder ; https://github.com/igorantolic/AiEsp32RotaryEncoder ; https://github.com/olikraus/u8g2 ; https://github.com/bblanchon/ArduinoJson
- GUI comparison: docs.waveshare.com ESP32-Peripheral-Tutorials (LVGL vs u8g2 vs Arduino_GFX)
