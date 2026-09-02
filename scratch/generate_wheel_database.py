import re
import json
import os

# Load parsed wheels metadata
with open(".agents/survey_spec_miner/parsed_wheels.json", "r", encoding="utf-8") as f:
    wheels = json.load(f)

# Load raw wheel_defs.h to extract exact array contents
with open("external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h", "r", encoding="utf-8") as f:
    defs_text = f.read()

# Clean comments
clean_text = re.sub(r'/\*.*?\*/', '', defs_text, flags=re.DOTALL)
clean_text = re.sub(r'//.*', '', clean_text)

extracted_arrays = {}
for w in wheels:
    arr_name = w["array_name"]
    pattern = rf'(?:const\s+(?:unsigned\s+char|uint8_t)\s+{arr_name}\s*(?:\[\s*\d*\s*\])?\s*PROGMEM\s*=\s*\{{)([^\}}]+)\}};'
    m = re.search(pattern, clean_text)
    if not m:
        pattern = rf'{arr_name}\s*(?:\[\s*\d*\s*\])?\s*PROGMEM\s*=\s*\{{([^\}}]+)\}};'
        m = re.search(pattern, clean_text)
    if not m:
        raise Exception(f"Failed to find array {arr_name}")
    nums = [int(x.strip()) for x in m.group(1).split(',') if x.strip()]
    if len(nums) != w["spec_edges"]:
        raise Exception(f"Length mismatch for {arr_name}: {len(nums)} vs {w['spec_edges']}")
    extracted_arrays[arr_name] = nums

print(f"Successfully extracted all {len(extracted_arrays)} arrays.")

# Category mapper
def get_brand_category(w):
    idx = w["index"]
    brand = w["brand"]
    name = w["friendly_name"]
    
    # Specific mappings
    if idx in [18, 19, 20, 22, 53, 62, 63, 68]:
        return "BrandCategory::TOYOTA_DAIHATSU"
    if idx in [30, 48, 49, 50, 51]:
        return "BrandCategory::HONDA"
    if idx in [25, 46, 59, 61]:
        return "BrandCategory::MITSUBISHI"
    if idx in [35, 38]:
        return "BrandCategory::NISSAN"
    if idx in [12, 15, 23, 26, 27, 28, 29, 32, 33, 34, 36, 37, 41, 42, 43, 44, 47, 58, 60, 65, 66, 67, 69]:
        return "BrandCategory::EURO_US"
    return "BrandCategory::UNIVERSAL"

# 1. Generate pattern_types.h
pattern_types_code = """#pragma once
#include <stdint.h>
#include <stddef.h>

namespace EcuEngine {

/**
 * @brief Signal channel bitmasks for trigger wheel bit-arrays.
 * Each byte in the PROGMEM pattern array encodes simultaneous logic states:
 * - Bit 0 (0x01): Crankshaft (CKP / Primary Trigger)
 * - Bit 1 (0x02): Camshaft 1 (CMP1 / Secondary Trigger)
 * - Bit 2 (0x04): Camshaft 2 (CMP2 / Tertiary Trigger / Dual VVT)
 * - Bit 3 (0x08): Auxiliary / Knock Trigger (Reserved)
 */
constexpr uint8_t SIGNAL_BIT_CKP   = 0x01;
constexpr uint8_t SIGNAL_BIT_CMP1  = 0x02;
constexpr uint8_t SIGNAL_BIT_CMP2  = 0x04;
constexpr uint8_t SIGNAL_BIT_KNOCK = 0x08;

/**
 * @brief Represents a single pulse transition duration (in microseconds) and logic level.
 */
struct PulseTransition {
    uint32_t durationUs{0};
    bool     level{false};
};

} // namespace EcuEngine
"""

with open("lib/engine/include/pattern_types.h", "w", encoding="utf-8") as f:
    f.write(pattern_types_code)
print("Wrote lib/engine/include/pattern_types.h")

# 2. Generate wheel_database.h
wheel_database_h = """#pragma once
#include <stdint.h>
#include <stddef.h>

#ifndef PROGMEM
#define PROGMEM
#endif

/**
 * @brief Brand Category enumeration for filtering and grouping OEM trigger wheels.
 */
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

/**
 * @brief Cycle degrees for trigger wheels (360 degrees for 1 crank rev, 720 degrees for 4-stroke cycle).
 */
enum class WheelCycleDegrees : uint16_t {
    CRANK_360 = 360,
    ENGINE_720 = 720
};

/**
 * @brief Wheel definition holding metadata and PROGMEM bit-array pointer for an ArduStim pattern.
 */
struct WheelDefinition {
    uint8_t             id;           // 0 .. 69 matching ArduStim index
    const char*         friendlyName; // Exact string name from ArduStim TFTv2
    const char*         shortName;    // Compact display name
    BrandCategory       category;     // Brand categorization
    WheelCycleDegrees   cycleDegrees; // 360 or 720 degrees
    uint16_t            totalEdges;   // Number of segments in bit-array
    const uint8_t*      bitArray;     // PROGMEM array: bit0=CKP, bit1=CMP1, bit2=CMP2
    bool                hasCmp1;      // True if pattern contains Cam 1 pulses
    bool                hasCmp2;      // True if pattern contains Cam 2 pulses
};

namespace WheelDatabase {

constexpr size_t TOTAL_WHEELS = 70;

/**
 * @brief Returns the total number of built-in wheel patterns (70).
 */
size_t getWheelCount();

/**
 * @brief Retrieves a wheel definition by index (0 .. 69).
 * @param index Array index (0 .. TOTAL_WHEELS - 1).
 * @return Pointer to WheelDefinition, or nullptr if out of bounds.
 */
const WheelDefinition* getWheel(size_t index);

/**
 * @brief Retrieves a wheel definition by ArduStim ID (0 .. 69).
 * @param id Wheel ID.
 * @return Pointer to WheelDefinition, or nullptr if not found.
 */
const WheelDefinition* getWheelById(uint8_t id);

/**
 * @brief Finds a wheel definition by exact or case-insensitive friendly name.
 * @param name Exact friendly name string (e.g. "Toyota Avanza 1.5 Crank only").
 * @return Pointer to WheelDefinition, or nullptr if not found.
 */
const WheelDefinition* findByFriendlyName(const char* name);

/**
 * @brief Finds a wheel definition by short display name.
 * @param name Short name string (e.g. "New Avanza").
 * @return Pointer to WheelDefinition, or nullptr if not found.
 */
const WheelDefinition* findByShortName(const char* name);

/**
 * @brief Filters and populates an array of wheel definition pointers matching a category.
 * @param cat Brand category to filter by (or BrandCategory::ALL for all).
 * @param outWheels Destination array to store pointers (can be nullptr to just query count).
 * @param maxOut Maximum number of pointers to write into outWheels.
 * @return Number of matching wheels found.
 */
size_t getWheelsByCategory(BrandCategory cat, const WheelDefinition** outWheels, size_t maxOut);

/**
 * @brief Returns the human-readable brand category name string.
 * @param cat Brand category.
 * @return Null-terminated string representation.
 */
const char* getCategoryName(BrandCategory cat);

} // namespace WheelDatabase
"""

with open("lib/engine/include/wheel_database.h", "w", encoding="utf-8") as f:
    f.write(wheel_database_h)
print("Wrote lib/engine/include/wheel_database.h")

# 3. Generate wheel_database.cpp
cpp_lines = [
    '#include "wheel_database.h"',
    '#include <string.h>',
    '#include <ctype.h>',
    '',
    '// ============================================================================',
    '// 70 ArduStim Trigger Wheel PROGMEM Bit-Arrays (Indices 0 .. 69)',
    '// Bit 0 (0x01): CKP, Bit 1 (0x02): CMP1, Bit 2 (0x04): CMP2',
    '// ============================================================================',
    ''
]

for w in wheels:
    idx = w["index"]
    arr_name = w["array_name"]
    nums = extracted_arrays[arr_name]
    var_name = f"s_pattern_{idx:02d}_{arr_name}"
    cpp_lines.append(f"// Pattern [{idx:2d}]: {w['friendly_name']} ({len(nums)} edges, {w['degrees']} deg)")
    cpp_lines.append(f"static const uint8_t {var_name}[{len(nums)}] PROGMEM = {{")
    
    # Format 20 numbers per line
    for chunk_start in range(0, len(nums), 20):
        chunk = nums[chunk_start:chunk_start+20]
        chunk_str = ", ".join(str(n) for n in chunk)
        if chunk_start + 20 < len(nums):
            cpp_lines.append(f"    {chunk_str},")
        else:
            cpp_lines.append(f"    {chunk_str}")
    cpp_lines.append("};\n")

# Array of WheelDefinition
cpp_lines.append('// ============================================================================')
cpp_lines.append(f'// Master Wheel Definitions Table ({len(wheels)} Presets)')
cpp_lines.append('// ============================================================================')
cpp_lines.append(f'static const WheelDefinition s_wheelDatabase[{len(wheels)}] = {{')

for w in wheels:
    idx = w["index"]
    arr_name = w["array_name"]
    var_name = f"s_pattern_{idx:02d}_{arr_name}"
    friendly = w["friendly_name"].replace('"', '\\"')
    short_name = w["tft_name"].replace('"', '\\"')
    cat = get_brand_category(w)
    deg = "WheelCycleDegrees::CRANK_360" if w["degrees"] == 360 else "WheelCycleDegrees::ENGINE_720"
    edges = w["spec_edges"]
    cmp1 = "true" if w["has_cam1"] else "false"
    cmp2 = "true" if w["has_cam2"] else "false"
    
    cpp_lines.append(f'    {{ {idx:2d}, "{friendly}", "{short_name}", {cat}, {deg}, {edges}, {var_name}, {cmp1}, {cmp2} }},')

cpp_lines.append('};\n')

# Lookup API implementation
api_impl = """// Helper for case-insensitive string comparison
static bool stringEqualsIgnoreCase(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++;
        b++;
    }
    return (*a == '\\0' && *b == '\\0');
}

namespace WheelDatabase {

size_t getWheelCount() {
    return TOTAL_WHEELS;
}

const WheelDefinition* getWheel(size_t index) {
    if (index < TOTAL_WHEELS) {
        return &s_wheelDatabase[index];
    }
    return nullptr;
}

const WheelDefinition* getWheelById(uint8_t id) {
    if (id < TOTAL_WHEELS) {
        return &s_wheelDatabase[id];
    }
    return nullptr;
}

const WheelDefinition* findByFriendlyName(const char* name) {
    if (!name || name[0] == '\\0') return nullptr;
    
    // First pass: exact match
    for (size_t i = 0; i < TOTAL_WHEELS; ++i) {
        if (strcmp(s_wheelDatabase[i].friendlyName, name) == 0) {
            return &s_wheelDatabase[i];
        }
    }
    // Second pass: case-insensitive match
    for (size_t i = 0; i < TOTAL_WHEELS; ++i) {
        if (stringEqualsIgnoreCase(s_wheelDatabase[i].friendlyName, name)) {
            return &s_wheelDatabase[i];
        }
    }
    return nullptr;
}

const WheelDefinition* findByShortName(const char* name) {
    if (!name || name[0] == '\\0') return nullptr;
    
    // First pass: exact match
    for (size_t i = 0; i < TOTAL_WHEELS; ++i) {
        if (strcmp(s_wheelDatabase[i].shortName, name) == 0) {
            return &s_wheelDatabase[i];
        }
    }
    // Second pass: case-insensitive match
    for (size_t i = 0; i < TOTAL_WHEELS; ++i) {
        if (stringEqualsIgnoreCase(s_wheelDatabase[i].shortName, name)) {
            return &s_wheelDatabase[i];
        }
    }
    return nullptr;
}

size_t getWheelsByCategory(BrandCategory cat, const WheelDefinition** outWheels, size_t maxOut) {
    size_t count = 0;
    for (size_t i = 0; i < TOTAL_WHEELS; ++i) {
        if (cat == BrandCategory::ALL || s_wheelDatabase[i].category == cat) {
            if (outWheels && count < maxOut) {
                outWheels[count] = &s_wheelDatabase[i];
            }
            count++;
        }
    }
    return count;
}

const char* getCategoryName(BrandCategory cat) {
    switch (cat) {
        case BrandCategory::ALL:              return "ALL";
        case BrandCategory::TOYOTA_DAIHATSU:  return "Toyota / Daihatsu";
        case BrandCategory::HONDA:            return "Honda";
        case BrandCategory::MITSUBISHI:       return "Mitsubishi";
        case BrandCategory::NISSAN:           return "Nissan";
        case BrandCategory::EURO_US:          return "Euro / US";
        case BrandCategory::UNIVERSAL:        return "Universal";
        case BrandCategory::CUSTOM:           return "Custom";
        default:                              return "Unknown";
    }
}

} // namespace WheelDatabase
"""

cpp_lines.append(api_impl)

with open("lib/engine/src/wheel_database.cpp", "w", encoding="utf-8") as f:
    f.write("\n".join(cpp_lines))
print("Wrote lib/engine/src/wheel_database.cpp")
