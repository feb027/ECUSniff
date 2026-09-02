# Project: ECUSniff Wheel Pattern Simulator & ArduStim Porting

## Architecture
ECUSniff is an automotive ECU diagnostic and signal generator platform based on ESP32-S3 (with 8MB Octal PSRAM and 4.0" 480x480 TFT Display).
The architecture comprises four tightly coupled subsystems:
1. **Engine Pattern Database (`lib/engine/`)**: High-performance, memory-efficient storage in Flash PROGMEM holding the complete catalog of 70 ArduStim TFTv2 / Pattern-Gen crankshaft and camshaft wheel bit-arrays (CKP, CMP1, CMP2) across $360^\circ$ and $720^\circ$ cycles, with brand categorization, exact friendly names, and fast query interfaces.
2. **HAL RMT Signal Generator (`lib/hal/`)**: Multi-channel ESP32-S3 RMT peripheral driver converting segment/bit-arrays into microsecond pulse trains via Run-Length Encoding (RLE), duration chunk slicing ($\le 30,000\ \mu\text{s}$), and hardware loopback with zero phase jitter or buffer underrun across 10–12,000 RPM.
3. **UI & Waveform Canvas (`lib/ui/`)**: TFT display subsystem rendering crisp virtual oscilloscope traces on dynamic canvas heights ($456 \times 124\text{ px}$ on Wheel Browser, $448 \times 76\text{ px}$ on Dashboard) for CKP (Yellow), CMP1 (Green), and CMP2 (Cyan) with $0-720^\circ$ horizontal mapping and anti-aliased step waveforms.
4. **Automated Test & Verification Suite (`test/`)**: Embedded ESP32-S3 and portable desktop test fixtures executing comparative timing and edge-transition verification against ArduStim source definitions.

## Code Layout & Write Ownership
| Module / Path | Description | Milestone Owner |
|---|---|---|
| `lib/engine/include/wheel_database.h`, `lib/engine/src/wheel_database.cpp` | 70 ArduStim PROGMEM bit-arrays, metadata structs, categories, lookups | M1 |
| `lib/engine/include/pattern_types.h`, `lib/engine/include/parametric_pattern.h`, `lib/engine/src/parametric_pattern.cpp` | Engine pattern data structures & pulse conversion logic | M1 / M2 |
| `lib/hal/include/rmt_generator.h`, `lib/hal/src/rmt_generator.cpp` | ESP32-S3 RMT multi-channel pulse generator (CKP, CMP1, CMP2) | M2 |
| `lib/ui/include/waveform_canvas.h`, `lib/ui/src/waveform_canvas.cpp` | Dynamic Waveform Canvas rendering for arbitrary bit-arrays | M3 |
| `lib/ui/include/page_wheel_browser.h`, `lib/ui/src/page_wheel_browser.cpp` | Wheel selection UI & brand filtering | M3 |
| `test/test_wheel_patterns/`, `test/test_rmt_timing/` | Comparative edge and timing validation test suites | M4 & E2E Track |

## Feature Inventory
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | All 70 ArduStim Wheel Bit-Arrays | Complete PROGMEM storage of all 70 ArduStim wheel patterns ($360^\circ$ and $720^\circ$) with exact edge counts | M1 | ORIGINAL_REQUEST §R1 |
| 2 | Identical Friendly Naming & Metadata | Exact ArduStim TFTv2 string names, indices 0-69, compact names, wheel types, cycle degrees | M1 | ORIGINAL_REQUEST §R1 |
| 3 | Brand Category Classification | Clean enum-based categorization (Toyota, Honda, Mitsubishi, Nissan, Euro/US, Universal, Custom) | M1 | ORIGINAL_REQUEST §R1 |
| 4 | Arbitrary Bit-Array to RMT Conversion | RLE compression converting bitmask sequences (CKP, CMP1, CMP2) into microsecond RMT pulse items | M2 | ORIGINAL_REQUEST §R2 |
| 5 | Dynamic RPM Microsecond Scaling | Accurate calculation $T_{\text{seg}} = \frac{D \times 10^6}{6 \times E \times RPM}$ with pulse chunking ($\le 30,000\ \mu\text{s}$) | M2 | ORIGINAL_REQUEST §R2 |
| 6 | Multi-Channel Sync (CKP, CMP1, CMP2) | Synchronized RMT channel outputs for dual-cam OEM patterns (e.g. BMW N20, GM LS1) | M2 | ORIGINAL_REQUEST §R2 |
| 7 | Zero Buffer Underrun & Continuous Looping | Hardware RMT continuous loop mode with EOT `{0,0,0,0}` termination and atomic double-buffering | M2 | ORIGINAL_REQUEST §R2 |
| 8 | Dynamic Canvas Height Partitioning | WaveformCanvas support for $124\text{ px}$ and $76\text{ px}$ heights without unpainted blank gaps | M3 | ORIGINAL_REQUEST §R3 |
| 9 | Multi-Trace Oscilloscope Visualization | Simultaneous CKP (Yellow), CMP1 (Green), and CMP2 (Cyan) rendering from bit-arrays | M3 | ORIGINAL_REQUEST §R3 |
| 10 | $0-720^\circ$ Horizontal Normalization | Seamless display scaling spanning full 4-stroke cycle with $2\times$ replication for $360^\circ$ wheels | M3 | ORIGINAL_REQUEST §R3 |
| 11 | UI Brand Category Browser Sync | Update `PageWheelBrowser` with dropdown/tab brand filtering using clean category enums | M3 | ORIGINAL_REQUEST §R1/R3 |
| 12 | Critical OEM Comparative Tests | Automated unit tests verifying edge transitions for Avanza Old, Avanza New, Rush/Terios, 4G63, 60-2 | M4 | ORIGINAL_REQUEST §R4 |
| 13 | Multi-Tier Test Suite & Pass (Tiers 1-4) | Comprehensive Category-Partition, BVA, Pairwise, and Application-level tests | M4 / E2E | ORIGINAL_REQUEST §R4 |
| 14 | Clean PlatformIO Firmware Build | Zero compiler errors or warnings on `esp32s3` environment | M4 / E2E | ORIGINAL_REQUEST §AC |

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Wheel Database & Data Structures | Port 70 PROGMEM bit-arrays, metadata structs, exact friendly names, brand categories | none | DONE |
| M2 | ESP32-S3 RMT Generator & Engine | Bit-array RLE converter, microsecond timing math, multi-channel RMT driver (CKP, CMP1, CMP2) | M1 | DONE |
| M3 | UI Waveform Canvas & Browser Sync | Dynamic height rendering, 3-trace bit-array visualization ($0-720^\circ$), category filtering UI | M1 | DONE |
| M4 | E2E Testing & Timing Verification | Comprehensive test harness, ArduStim comparative timing tests, PlatformIO build verification | M1, M2, M3 | DONE |

## Interface Contracts

### 1. Database (`lib/engine/include/wheel_database.h`)
```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>

enum class BrandCategory : uint8_t {
    ALL = 0,
    TOYOTA_DAIHATSU,
    HONDA,
    MITSUBISHI,
    NISSAN,
    EURO_US,
    UNIVERSAL,
    CUSTOM,
    COUNT
};

enum class WheelCycleDegrees : uint16_t {
    CRANK_360 = 360,
    ENGINE_720 = 720
};

struct WheelDefinition {
    uint8_t id;                       // 0 .. 69 matching ArduStim index
    const char* friendlyName;         // Exact string name from ArduStim
    const char* shortName;            // Compact display name
    BrandCategory category;           // Brand categorization
    WheelCycleDegrees cycleDegrees;   // 360 or 720
    uint16_t totalEdges;              // Number of segments (e.g. 144, 720, 1080)
    const uint8_t* bitArray;          // PROGMEM array: bit0=CKP, bit1=CMP1, bit2=CMP2
    bool hasCmp1;                     // True if pattern contains Cam 1 pulses
    bool hasCmp2;                     // True if pattern contains Cam 2 pulses
};

namespace WheelDatabase {
    size_t getWheelCount();
    const WheelDefinition* getWheel(size_t index);
    const WheelDefinition* getWheelById(uint8_t id);
    const WheelDefinition* findByFriendlyName(const char* name);
    size_t getWheelsByCategory(BrandCategory cat, const WheelDefinition** outWheels, size_t maxOut);
    const char* getCategoryName(BrandCategory cat);
}
```

### 2. Engine & RMT Driver (`lib/hal/include/rmt_generator.h` & `lib/engine/`)
```cpp
#pragma once
#include <stdint.h>
#include "wheel_database.h"
#include <driver/rmt.h>

struct PulseTransition {
    uint32_t durationUs;
    bool level;
};

class RmtGenerator {
public:
    static bool init();
    static bool setWheelPattern(const WheelDefinition* wheel);
    static bool setRpm(uint32_t rpm);
    static bool start();
    static bool stop();
    static bool isRunning();
    
    // Internal conversion utility for test validation & buffer filling
    static size_t compileBitArrayToRmt(
        const uint8_t* bitArray,
        uint16_t totalEdges,
        uint16_t cycleDegrees,
        uint32_t rpm,
        uint8_t channelBitMask, // 0x01 for CKP, 0x02 for CMP1, 0x04 for CMP2
        rmt_item32_t* outItems,
        size_t maxItems
    );
};
```

### 3. Waveform Canvas (`lib/ui/include/waveform_canvas.h`)
```cpp
#pragma once
#include <TFT_eSPI.h>
#include "wheel_database.h"

class WaveformCanvas {
public:
    WaveformCanvas() = default;
    ~WaveformCanvas();

    bool init(int32_t width, int32_t height);
    void render(const WheelDefinition* wheel, int32_t screenX, int32_t screenY);
    void clear();

private:
    int32_t _width = 0;
    int32_t _height = 0;
    TFT_eSprite _sprite{&tft}; // or internal framebuffer
    
    void _drawGrid(bool showCmp1, bool showCmp2);
    void _drawBitArrayTrace(const uint8_t* bitArray, uint16_t totalEdges, uint16_t cycleDegrees, uint8_t bitMask, uint16_t color, int32_t yTop, int32_t yHeight);
};
```
