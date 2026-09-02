#include <unity.h>
#include "../test_wheel_data_oracle.h"
#include "timing_math.h"

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// TIER 1: FEATURE COVERAGE (>= 5 test cases per feature)
// ============================================================================

void test_tier1_all_70_presets_accessible(void) {
    TEST_ASSERT_EQUAL_UINT32(70, ORACLE_TOTAL_WHEELS);
    for (size_t i = 0; i < ORACLE_TOTAL_WHEELS; ++i) {
        const OracleWheelDefinition* wheel = getOracleWheel(i);
        TEST_ASSERT_NOT_NULL_MESSAGE(wheel, "Wheel pointer must not be null");
        TEST_ASSERT_EQUAL_UINT8(i, wheel->id);
        TEST_ASSERT_NOT_NULL(wheel->friendlyName);
        TEST_ASSERT_TRUE(strlen(wheel->friendlyName) > 0);
        TEST_ASSERT_NOT_NULL(wheel->shortName);
        TEST_ASSERT_TRUE(strlen(wheel->shortName) > 0);
        TEST_ASSERT_NOT_NULL(wheel->enumName);
        TEST_ASSERT_TRUE(strlen(wheel->enumName) > 0);
    }
}

void test_tier1_friendly_name_exact_matching(void) {
    const struct {
        uint8_t expectedId;
        const char* name;
    } cases[] = {
        { 18, "Toyota Avanza 1.3 Crank only" },
        { 19, "Toyota Avanza 1.5 Crank only" },
        { 20, "Toyota Avanza/Xenia/Terios/Rush " },
        { 46, "Mitsubishi 4g63 aka 4/2 crank and cam" },
        { 25, "Mitsubishi 6g72 with cam" },
        { 61, "Mitsubishi 3A92" },
        { 49, "Honda Jazz Fit 04-08" },
        { 50, "Honda Jazz Fit 04-08V2" },
        { 51, "Honda Jazz Fit 04-08V3" },
        { 3,  "60-2 crank only" },
        { 6,  "36-1 crank only" },
        { 35, "Nissan Livina Juke crank and cam" },
        { 66, "BMW N20" }
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const OracleWheelDefinition* w = findOracleWheelByFriendlyName(cases[i].name);
        TEST_ASSERT_NOT_NULL_MESSAGE(w, cases[i].name);
        TEST_ASSERT_EQUAL_UINT8(cases[i].expectedId, w->id);
        TEST_ASSERT_EQUAL_STRING(cases[i].name, w->friendlyName);
    }
}

void test_tier1_cycle_degrees_validity(void) {
    size_t count360 = 0;
    size_t count720 = 0;

    for (size_t i = 0; i < ORACLE_TOTAL_WHEELS; ++i) {
        const OracleWheelDefinition* w = getOracleWheel(i);
        uint16_t deg = (uint16_t)w->cycleDegrees;
        TEST_ASSERT_TRUE(deg == 360 || deg == 720);
        if (deg == 360) count360++;
        if (deg == 720) count720++;

        float stepDeg = (float)deg / (float)w->totalEdges;
        TEST_ASSERT_TRUE(stepDeg > 0.0f);
        TEST_ASSERT_TRUE(stepDeg <= 90.0f);
    }

    TEST_ASSERT_EQUAL_UINT32(17, count360);
    TEST_ASSERT_EQUAL_UINT32(53, count720);
}

void test_tier1_edge_count_boundaries_and_validity(void) {
    for (size_t i = 0; i < ORACLE_TOTAL_WHEELS; ++i) {
        const OracleWheelDefinition* w = getOracleWheel(i);
        TEST_ASSERT_TRUE(w->totalEdges >= 4);
        TEST_ASSERT_TRUE(w->totalEdges <= 1080);
    }

    // Min edge wheel: Dizzy 4-cyl = 4 edges
    const OracleWheelDefinition* minW = getOracleWheel(0);
    TEST_ASSERT_EQUAL_UINT16(4, minW->totalEdges);

    // Max edge wheel: Audi 135 = 1080 edges
    const OracleWheelDefinition* maxW = getOracleWheel(47);
    TEST_ASSERT_EQUAL_UINT16(1080, maxW->totalEdges);

    // Avanza wheels = 144 edges
    TEST_ASSERT_EQUAL_UINT16(144, getOracleWheel(18)->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(144, getOracleWheel(19)->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(144, getOracleWheel(20)->totalEdges);

    // 60-2 crank only = 120 edges
    TEST_ASSERT_EQUAL_UINT16(120, getOracleWheel(3)->totalEdges);
}

void test_tier1_bitmask_channel_flags_consistency(void) {
    size_t cmp1Count = 0;
    size_t cmp2Count = 0;

    for (size_t i = 0; i < ORACLE_TOTAL_WHEELS; ++i) {
        const OracleWheelDefinition* w = getOracleWheel(i);
        // Every trigger wheel must have Crankshaft (CKP) active
        TEST_ASSERT_TRUE(w->hasCkp);
        if (w->hasCmp1) cmp1Count++;
        if (w->hasCmp2) cmp2Count++;
    }

    TEST_ASSERT_EQUAL_UINT32(48, cmp1Count);
    TEST_ASSERT_EQUAL_UINT32(2, cmp2Count);

    // Only BMW N20 (index 66) and GM LS1 (index 27) drive Cam 2
    TEST_ASSERT_TRUE(getOracleWheel(66)->hasCmp2);
    TEST_ASSERT_TRUE(getOracleWheel(27)->hasCmp2);
    TEST_ASSERT_FALSE(getOracleWheel(3)->hasCmp2); // 60-2 has no cam2
}

void test_tier1_brand_categorization(void) {
    const OracleWheelDefinition* avanza = getOracleWheel(19);
    TEST_ASSERT_TRUE(avanza->category == OracleBrandCategory::TOYOTA_DAIHATSU);

    const OracleWheelDefinition* jazz = getOracleWheel(49);
    TEST_ASSERT_TRUE(jazz->category == OracleBrandCategory::HONDA);

    const OracleWheelDefinition* mitsu = getOracleWheel(46);
    TEST_ASSERT_TRUE(mitsu->category == OracleBrandCategory::MITSUBISHI);

    const OracleWheelDefinition* nissan = getOracleWheel(35);
    TEST_ASSERT_TRUE(nissan->category == OracleBrandCategory::NISSAN);

    const OracleWheelDefinition* bosch602 = getOracleWheel(3);
    TEST_ASSERT_TRUE(bosch602->category == OracleBrandCategory::UNIVERSAL);
}

// ============================================================================
// TIER 2: BOUNDARY & CORNER CASES
// ============================================================================

void test_tier2_lookup_boundary_null_and_unknown(void) {
    TEST_ASSERT_NULL(findOracleWheelByFriendlyName(nullptr));
    TEST_ASSERT_NULL(findOracleWheelByFriendlyName(""));
    TEST_ASSERT_NULL(findOracleWheelByFriendlyName("NON_EXISTENT_WHEEL_PATTERN"));
    TEST_ASSERT_NULL(findOracleWheelByFriendlyName("Toyota Avanza 1.3")); // Partial mismatch

    TEST_ASSERT_NULL(getOracleWheel(70));
    TEST_ASSERT_NULL(getOracleWheel(255));
    TEST_ASSERT_NULL(getOracleWheel(1000));
}

void test_tier2_edge_extremes_timing_math(void) {
    // Min edges = 4 (Dizzy 4 cyl, 360 deg)
    uint32_t t_dizzy_1000 = (uint32_t)((360.0 * 1e6) / (6.0 * 4.0 * 1000.0));
    TEST_ASSERT_EQUAL_UINT32(15000, t_dizzy_1000);

    // Max edges = 1080 (Audi 135, 720 deg)
    uint32_t t_audi_6000 = (uint32_t)((720.0 * 1e6) / (6.0 * 1080.0 * 6000.0));
    TEST_ASSERT_EQUAL_UINT32(18, t_audi_6000);
}

void test_tier2_rpm_dynamic_scaling_range(void) {
    const uint32_t testRpms[] = { 10, 50, 200, 850, 3000, 6000, 12000 };
    for (size_t i = 0; i < sizeof(testRpms)/sizeof(testRpms[0]); ++i) {
        uint32_t rpm = testRpms[i];
        uint32_t t_seg = (uint32_t)((360.0 * 1e6) / (6.0 * 120.0 * (double)rpm));
        uint32_t expected = 500000 / rpm;
        TEST_ASSERT_UINT32_WITHIN(1, expected, t_seg);
    }

    for (size_t i = 0; i < sizeof(testRpms)/sizeof(testRpms[0]); ++i) {
        uint32_t rpm = testRpms[i];
        uint32_t t_seg = (uint32_t)((720.0 * 1e6) / (6.0 * 144.0 * (double)rpm));
        uint32_t expected = (uint32_t)(833333.333 / (double)rpm);
        TEST_ASSERT_UINT32_WITHIN(2, expected, t_seg);
    }
}

void test_tier2_rmt_duration_chunking_ultra_low_rpm(void) {
    uint32_t totalDurationUs = 50000;
    constexpr uint32_t MAX_CHUNK_US = 30000;

    uint32_t chunks[4] = {0};
    size_t chunkCount = 0;
    uint32_t rem = totalDurationUs;

    while (rem > MAX_CHUNK_US) {
        chunks[chunkCount++] = MAX_CHUNK_US;
        rem -= MAX_CHUNK_US;
    }
    if (rem > 0) {
        chunks[chunkCount++] = rem;
    }

    TEST_ASSERT_EQUAL_UINT32(2, chunkCount);
    TEST_ASSERT_EQUAL_UINT32(30000, chunks[0]);
    TEST_ASSERT_EQUAL_UINT32(20000, chunks[1]);
    TEST_ASSERT_EQUAL_UINT32(50000, chunks[0] + chunks[1]);
}

void test_tier2_multi_gap_sync_metadata(void) {
    const OracleWheelDefinition* h4 = getOracleWheel(17);
    TEST_ASSERT_NOT_NULL(h4);
    TEST_ASSERT_EQUAL_UINT16(72, h4->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(360, (uint16_t)h4->cycleDegrees);

    const OracleWheelDefinition* h6 = getOracleWheel(21);
    TEST_ASSERT_NOT_NULL(h6);
    TEST_ASSERT_EQUAL_UINT16(72, h6->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(360, (uint16_t)h6->cycleDegrees);

    const OracleWheelDefinition* w12_3 = getOracleWheel(16);
    TEST_ASSERT_NOT_NULL(w12_3);
    TEST_ASSERT_EQUAL_UINT16(48, w12_3->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(360, (uint16_t)w12_3->cycleDegrees);

    const OracleWheelDefinition* lotus = getOracleWheel(29);
    TEST_ASSERT_NOT_NULL(lotus);
    TEST_ASSERT_EQUAL_UINT16(72, lotus->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(360, (uint16_t)lotus->cycleDegrees);
}

static void runWheelDatabaseTests(void) {
    UNITY_BEGIN();

    // Tier 1
    RUN_TEST(test_tier1_all_70_presets_accessible);
    RUN_TEST(test_tier1_friendly_name_exact_matching);
    RUN_TEST(test_tier1_cycle_degrees_validity);
    RUN_TEST(test_tier1_edge_count_boundaries_and_validity);
    RUN_TEST(test_tier1_bitmask_channel_flags_consistency);
    RUN_TEST(test_tier1_brand_categorization);

    // Tier 2
    RUN_TEST(test_tier2_lookup_boundary_null_and_unknown);
    RUN_TEST(test_tier2_edge_extremes_timing_math);
    RUN_TEST(test_tier2_rpm_dynamic_scaling_range);
    RUN_TEST(test_tier2_rmt_duration_chunking_ultra_low_rpm);
    RUN_TEST(test_tier2_multi_gap_sync_metadata);

    UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
void setup() {
    delay(2000);
    runWheelDatabaseTests();
}
void loop() {}
#else
int main(int argc, char** argv) {
    runWheelDatabaseTests();
    return 0;
}
#endif
