#include <unity.h>
#include "../test_wheel_data_oracle.h"
#include "timing_math.h"

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// TIER 3: CROSS-FEATURE COMBINATIONS
// ============================================================================

void test_tier3_dual_cam_sync_bmw_n20(void) {
    const OracleWheelDefinition* bmw = getOracleWheel(66);
    TEST_ASSERT_NOT_NULL(bmw);
    TEST_ASSERT_EQUAL_STRING("BMW_N20", bmw->enumName);
    TEST_ASSERT_EQUAL_UINT16(240, bmw->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)bmw->cycleDegrees);
    TEST_ASSERT_TRUE(bmw->hasCkp);
    TEST_ASSERT_TRUE(bmw->hasCmp1);
    TEST_ASSERT_TRUE(bmw->hasCmp2);
}

void test_tier3_dual_cam_sync_gm_ls1(void) {
    const OracleWheelDefinition* ls1 = getOracleWheel(27);
    TEST_ASSERT_NOT_NULL(ls1);
    TEST_ASSERT_EQUAL_STRING("GM_LS1_CRANK_AND_CAM", ls1->enumName);
    TEST_ASSERT_EQUAL_UINT16(720, ls1->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)ls1->cycleDegrees);
    TEST_ASSERT_TRUE(ls1->hasCkp);
    TEST_ASSERT_TRUE(ls1->hasCmp1);
    TEST_ASSERT_TRUE(ls1->hasCmp2);
}

void test_tier3_cycle_conversion_360_to_720(void) {
    const OracleWheelDefinition* w60_2 = getOracleWheel(3);
    const OracleWheelDefinition* w60_2_cam = getOracleWheel(4);

    TEST_ASSERT_EQUAL_UINT16(120, w60_2->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(360, (uint16_t)w60_2->cycleDegrees);

    TEST_ASSERT_EQUAL_UINT16(240, w60_2_cam->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)w60_2_cam->cycleDegrees);

    float step60_2 = (float)w60_2->cycleDegrees / (float)w60_2->totalEdges;
    float step60_2_cam = (float)w60_2_cam->cycleDegrees / (float)w60_2_cam->totalEdges;
    TEST_ASSERT_EQUAL_FLOAT(3.0f, step60_2);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, step60_2_cam);

    uint32_t t_seg_360 = (uint32_t)((360.0 * 1e6) / (6.0 * 120.0 * 3000.0));
    uint32_t t_seg_720 = (uint32_t)((720.0 * 1e6) / (6.0 * 240.0 * 3000.0));
    TEST_ASSERT_EQUAL_UINT32(166, t_seg_360);
    TEST_ASSERT_EQUAL_UINT32(166, t_seg_720);
}

void test_tier3_rle_duration_conservation(void) {
    const uint32_t rpms[] = { 600, 1000, 3000, 6000 };
    for (size_t r = 0; r < sizeof(rpms)/sizeof(rpms[0]); ++r) {
        uint32_t rpm = rpms[r];
        
        uint32_t t_cycle_360 = (uint32_t)((360.0 * 1e6) / (6.0 * (double)rpm));
        uint32_t expected_360 = 60000000 / rpm;
        TEST_ASSERT_UINT32_WITHIN(1, expected_360, t_cycle_360);

        uint32_t t_cycle_720 = (uint32_t)((720.0 * 1e6) / (6.0 * (double)rpm));
        uint32_t expected_720 = 120000000 / rpm;
        TEST_ASSERT_UINT32_WITHIN(1, expected_720, t_cycle_720);
    }
}

void test_tier3_channel_bitmask_demuxing(void) {
    uint8_t sampleValues[] = { 0, 1, 2, 3, 4, 6, 7 };
    bool expCkp[]  = { false, true,  false, true,  false, false, true  };
    bool expCmp1[] = { false, false, true,  true,  false, true,  true  };
    bool expCmp2[] = { false, false, false, false, true,  true,  true  };

    for (size_t i = 0; i < sizeof(sampleValues); ++i) {
        uint8_t v = sampleValues[i];
        bool ckp = (v & 0x01) != 0;
        bool cmp1 = (v & 0x02) != 0;
        bool cmp2 = (v & 0x04) != 0;

        TEST_ASSERT_EQUAL_INT(expCkp[i], ckp);
        TEST_ASSERT_EQUAL_INT(expCmp1[i], cmp1);
        TEST_ASSERT_EQUAL_INT(expCmp2[i], cmp2);
    }
}

// ============================================================================
// TIER 4: REAL-WORLD OEM APPLICATION SCENARIOS
// ============================================================================

void test_tier4_scenario_new_avanza_timing(void) {
    const OracleWheelDefinition* w = getOracleWheel(19);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL_STRING("Toyota Avanza 1.5 Crank only", w->friendlyName);
    TEST_ASSERT_EQUAL_UINT16(144, w->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)w->cycleDegrees);
    TEST_ASSERT_TRUE(w->hasCkp);
    TEST_ASSERT_TRUE(w->hasCmp1);
    TEST_ASSERT_FALSE(w->hasCmp2);

    uint32_t t_idle = (uint32_t)((720.0 * 1e6) / (6.0 * 144.0 * 850.0));
    TEST_ASSERT_UINT32_WITHIN(1, 980, t_idle);

    uint32_t t_cruise = (uint32_t)((720.0 * 1e6) / (6.0 * 144.0 * 3000.0));
    TEST_ASSERT_UINT32_WITHIN(1, 277, t_cruise);
}

void test_tier4_scenario_old_avanza_metadata(void) {
    const OracleWheelDefinition* w = getOracleWheel(18);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL_STRING("Toyota Avanza 1.3 Crank only", w->friendlyName);
    TEST_ASSERT_EQUAL_UINT16(144, w->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)w->cycleDegrees);
    TEST_ASSERT_TRUE(w->hasCkp);
    TEST_ASSERT_TRUE(w->hasCmp1);
}

void test_tier4_scenario_avanza_xenia_rush_metadata(void) {
    const OracleWheelDefinition* w = getOracleWheel(20);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL_STRING("Toyota Avanza/Xenia/Terios/Rush ", w->friendlyName);
    TEST_ASSERT_EQUAL_UINT16(144, w->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)w->cycleDegrees);
    TEST_ASSERT_TRUE(w->hasCkp);
    TEST_ASSERT_TRUE(w->hasCmp1);
}

void test_tier4_scenario_mitsubishi_4g63_cas(void) {
    const OracleWheelDefinition* w = getOracleWheel(46);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL_STRING("Mitsubishi 4g63 aka 4/2 crank and cam", w->friendlyName);
    TEST_ASSERT_EQUAL_UINT16(144, w->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)w->cycleDegrees);
    TEST_ASSERT_TRUE(w->hasCkp);
    TEST_ASSERT_TRUE(w->hasCmp1);

    uint32_t t_seg = (uint32_t)((720.0 * 1e6) / (6.0 * 144.0 * 1000.0));
    uint32_t t_crank_pulse = t_seg * 12;
    TEST_ASSERT_UINT32_WITHIN(10, 10000, t_crank_pulse);
}

void test_tier4_scenario_mitsubishi_6g72_cas(void) {
    const OracleWheelDefinition* w = getOracleWheel(25);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL_STRING("Mitsubishi 6g72 with cam", w->friendlyName);
    TEST_ASSERT_EQUAL_UINT16(144, w->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)w->cycleDegrees);
    TEST_ASSERT_TRUE(w->hasCkp);
    TEST_ASSERT_TRUE(w->hasCmp1);
}

void test_tier4_scenario_mitsubishi_3a92(void) {
    const OracleWheelDefinition* w = getOracleWheel(61);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL_STRING("Mitsubishi 3A92", w->friendlyName);
    TEST_ASSERT_EQUAL_UINT16(144, w->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)w->cycleDegrees);
    TEST_ASSERT_TRUE(w->hasCkp);
    TEST_ASSERT_TRUE(w->hasCmp1);
}

void test_tier4_scenario_honda_jazz_variants(void) {
    const OracleWheelDefinition* j1 = getOracleWheel(49);
    const OracleWheelDefinition* j2 = getOracleWheel(50);
    const OracleWheelDefinition* j3 = getOracleWheel(51);

    TEST_ASSERT_NOT_NULL(j1);
    TEST_ASSERT_NOT_NULL(j2);
    TEST_ASSERT_NOT_NULL(j3);

    TEST_ASSERT_EQUAL_UINT16(144, j1->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(144, j2->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(144, j3->totalEdges);

    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)j1->cycleDegrees);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)j2->cycleDegrees);
    TEST_ASSERT_EQUAL_UINT16(720, (uint16_t)j3->cycleDegrees);

    TEST_ASSERT_TRUE(j1->hasCkp && j1->hasCmp1);
    TEST_ASSERT_TRUE(j2->hasCkp && j2->hasCmp1);
    TEST_ASSERT_TRUE(j3->hasCkp && j3->hasCmp1);
}

void test_tier4_scenario_universal_60_2(void) {
    const OracleWheelDefinition* w = getOracleWheel(3);
    TEST_ASSERT_NOT_NULL(w);
    TEST_ASSERT_EQUAL_STRING("60-2 crank only", w->friendlyName);
    TEST_ASSERT_EQUAL_UINT16(120, w->totalEdges);
    TEST_ASSERT_EQUAL_UINT16(360, (uint16_t)w->cycleDegrees);
    TEST_ASSERT_TRUE(w->hasCkp);
    TEST_ASSERT_FALSE(w->hasCmp1);
    TEST_ASSERT_FALSE(w->hasCmp2);

    uint32_t t_seg = (uint32_t)((360.0 * 1e6) / (6.0 * 120.0 * 6000.0));
    uint32_t t_gap = t_seg * 4;
    TEST_ASSERT_UINT32_WITHIN(2, 83, t_seg);
    TEST_ASSERT_UINT32_WITHIN(4, 333, t_gap);
}

static void runRmtTimingTests(void) {
    UNITY_BEGIN();

    // Tier 3
    RUN_TEST(test_tier3_dual_cam_sync_bmw_n20);
    RUN_TEST(test_tier3_dual_cam_sync_gm_ls1);
    RUN_TEST(test_tier3_cycle_conversion_360_to_720);
    RUN_TEST(test_tier3_rle_duration_conservation);
    RUN_TEST(test_tier3_channel_bitmask_demuxing);

    // Tier 4
    RUN_TEST(test_tier4_scenario_new_avanza_timing);
    RUN_TEST(test_tier4_scenario_old_avanza_metadata);
    RUN_TEST(test_tier4_scenario_avanza_xenia_rush_metadata);
    RUN_TEST(test_tier4_scenario_mitsubishi_4g63_cas);
    RUN_TEST(test_tier4_scenario_mitsubishi_6g72_cas);
    RUN_TEST(test_tier4_scenario_mitsubishi_3a92);
    RUN_TEST(test_tier4_scenario_honda_jazz_variants);
    RUN_TEST(test_tier4_scenario_universal_60_2);

    UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
void setup() {
    delay(2000);
    runRmtTimingTests();
}
void loop() {}
#else
int main(int argc, char** argv) {
    runRmtTimingTests();
    return 0;
}
#endif
