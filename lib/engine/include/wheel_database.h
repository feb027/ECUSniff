#pragma once
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
