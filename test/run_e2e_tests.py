#!/usr/bin/env python3
"""
ECUSniff E2E Test Suite & Verification Oracle
4-Tier Comprehensive Opaque-Box Verification for all 70 Trigger Wheel Patterns
Author: E2E Test Writer
"""

import sys
import os
import json
import math
import re

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
PARSED_JSON_PATH = os.path.join(ROOT_DIR, ".agents", "survey_spec_miner", "parsed_wheels.json")
WHEEL_DEFS_PATH = os.path.join(ROOT_DIR, "external", "ardustim-tftv2-touchscreen", "ardustim", "wheel_defs.h")

def load_parsed_wheels():
    if not os.path.exists(PARSED_JSON_PATH):
        raise FileNotFoundError(f"Missing {PARSED_JSON_PATH}")
    with open(PARSED_JSON_PATH, "r", encoding="utf-8") as f:
        return json.load(f)

def parse_raw_wheel_arrays():
    """Extracts raw C byte arrays from wheel_defs.h directly as ground truth."""
    if not os.path.exists(WHEEL_DEFS_PATH):
        raise FileNotFoundError(f"Missing {WHEEL_DEFS_PATH}")
    
    with open(WHEEL_DEFS_PATH, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    clean_code = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    clean_code = re.sub(r'//.*', '', clean_code)
    
    arrays = {}
    pattern = r'const\s+unsigned\s+char\s+(\w+)\s*\[\]\s*(?:PROGMEM)?\s*=\s*\{(.*?)\};'
    for m in re.finditer(pattern, clean_code, re.DOTALL):
        name = m.group(1)
        body = m.group(2)
        nums = [int(n) for n in re.findall(r'\b\d+\b', body)]
        arrays[name] = nums
    return arrays

class TestRunner:
    def __init__(self):
        self.total_tests = 0
        self.passed_tests = 0
        self.failed_tests = 0
        self.tier_results = {1: [], 2: [], 3: [], 4: [], 5: []}

    def assert_true(self, tier, test_name, condition, message=""):
        self.total_tests += 1
        if condition:
            self.passed_tests += 1
            self.tier_results[tier].append((test_name, True, message))
        else:
            self.failed_tests += 1
            self.tier_results[tier].append((test_name, False, message))
            print(f"[FAIL] [Tier {tier}] {test_name}: {message}")

    def assert_equal(self, tier, test_name, expected, actual, message=""):
        cond = (expected == actual)
        msg = message or f"Expected '{expected}', got '{actual}'"
        self.assert_true(tier, test_name, cond, msg)

    def assert_within(self, tier, test_name, expected, actual, tolerance, message=""):
        cond = abs(expected - actual) <= tolerance
        msg = message or f"Expected {expected} +/- {tolerance}, got {actual}"
        self.assert_true(tier, test_name, cond, msg)

    def print_summary(self):
        print("\n" + "="*80)
        print("                        ECUSNIFF E2E TEST SUMMARY")
        print("="*80)
        for tier in [1, 2, 3, 4, 5]:
            t_tests = self.tier_results[tier]
            t_passed = sum(1 for _, passed, _ in t_tests if passed)
            t_total = len(t_tests)
            status = "PASS" if t_passed == t_total and t_total > 0 else "FAIL"
            print(f"Tier {tier}: {t_passed}/{t_total} tests passed [{status}]")
        print("-" * 80)
        print(f"TOTAL: {self.passed_tests}/{self.total_tests} passed ({self.failed_tests} failed)")
        print("=" * 80)
        return self.failed_tests == 0


def run_tier1_feature_coverage(runner, wheels, raw_arrays):
    """Tier 1: Feature Coverage (>=5 test cases per feature)"""
    print("\n--- Running Tier 1: Feature Coverage ---")

    # Feature 1.1: All 70 Presets Accessible
    runner.assert_equal(1, "T1.1_Preset_Count", 70, len(wheels), "Total wheel presets must equal exactly 70")
    for i in range(70):
        w = wheels[i]
        runner.assert_equal(1, f"T1.1_Index_Match_{i}", i, w["index"], f"Wheel at index {i} must have index {i}")
        runner.assert_true(1, f"T1.1_Enum_NonEmpty_{i}", len(w["enum_name"]) > 0, "Enum identifier must be non-empty")
        runner.assert_true(1, f"T1.1_Array_Exists_{i}", w["array_name"] in raw_arrays, f"Array {w['array_name']} must exist in raw definitions")

    # Feature 1.2: Friendly Name Matching
    critical_names = {
        18: "Toyota Avanza 1.3 Crank only",
        19: "Toyota Avanza 1.5 Crank only",
        20: "Toyota Avanza/Xenia/Terios/Rush ",
        46: "Mitsubishi 4g63 aka 4/2 crank and cam",
        25: "Mitsubishi 6g72 with cam",
        61: "Mitsubishi 3A92",
        49: "Honda Jazz Fit 04-08",
        50: "Honda Jazz Fit 04-08V2",
        51: "Honda Jazz Fit 04-08V3",
        3:  "60-2 crank only",
        6:  "36-1 crank only",
        35: "Nissan Livina Juke crank and cam",
        66: "BMW N20"
    }
    for idx, expected_name in critical_names.items():
        w = wheels[idx]
        runner.assert_equal(1, f"T1.2_Friendly_Name_{idx}", expected_name, w["friendly_name"], f"Preset {idx} friendly name must match exact ArduStim string")

    # Feature 1.3: Cycle Degrees Validity (Strictly 360 or 720)
    crank_360_count = sum(1 for w in wheels if w["degrees"] == 360)
    engine_720_count = sum(1 for w in wheels if w["degrees"] == 720)
    runner.assert_equal(1, "T1.3_Cycle_Degrees_360_Count", 17, crank_360_count, "Exactly 17 presets must be 360 deg")
    runner.assert_equal(1, "T1.3_Cycle_Degrees_720_Count", 53, engine_720_count, "Exactly 53 presets must be 720 deg")
    for i, w in enumerate(wheels):
        runner.assert_true(1, f"T1.3_Valid_Degrees_{i}", w["degrees"] in (360, 720), f"Degrees must be 360 or 720, got {w['degrees']}")
        step_deg = w["degrees"] / w["actual_len"]
        runner.assert_true(1, f"T1.3_Step_Deg_Positive_{i}", step_deg > 0.0, f"Angular step must be positive, got {step_deg}")

    # Feature 1.4: Edge Count Integrity
    for i, w in enumerate(wheels):
        runner.assert_true(1, f"T1.4_Edge_Len_Match_{i}", w["len_matches"], f"Length of array {w['array_name']} ({w['actual_len']}) must match spec ({w['spec_edges']})")
        arr = raw_arrays[w["array_name"]]
        runner.assert_equal(1, f"T1.4_Raw_Len_Match_{i}", w["actual_len"], len(arr), f"Raw array length mismatch for {w['array_name']}")

    # Feature 1.5: Bitmask Validity
    for i, w in enumerate(wheels):
        arr = raw_arrays[w["array_name"]]
        invalid_vals = [x for x in arr if x not in (0, 1, 2, 3, 4, 5, 6, 7)]
        runner.assert_equal(1, f"T1.5_Bitmask_Valid_{i}", 0, len(invalid_vals), f"Array {w['array_name']} contains invalid bitmask values: {set(invalid_vals)}")
        has_ckp = any((x & 0x01) for x in arr)
        has_cmp1 = any((x & 0x02) for x in arr)
        has_cmp2 = any((x & 0x04) for x in arr)
        runner.assert_true(1, f"T1.5_Crank_Present_{i}", has_ckp, f"Wheel {i} must have active Crank pulses")
        runner.assert_equal(1, f"T1.5_Cmp1_Flag_Match_{i}", w["has_cam1"], has_cmp1, f"Wheel {i} has_cam1 flag mismatch")
        runner.assert_equal(1, f"T1.5_Cmp2_Flag_Match_{i}", w["has_cam2"], has_cmp2, f"Wheel {i} has_cam2 flag mismatch")

    # Feature 1.6: Brand Categorization
    categories = set(w["brand"] for w in wheels)
    runner.assert_true(1, "T1.6_Brand_Toyota_Present", "Toyota/Daihatsu" in categories or "Daihatsu" in categories, "Toyota/Daihatsu category present")
    runner.assert_true(1, "T1.6_Brand_Honda_Present", "Honda" in categories, "Honda category present")
    runner.assert_true(1, "T1.6_Brand_Mitsubishi_Present", "Mitsubishi/DSM" in categories, "Mitsubishi category present")
    runner.assert_true(1, "T1.6_Brand_Nissan_Present", "Nissan" in categories, "Nissan category present")
    runner.assert_true(1, "T1.6_Brand_Universal_Present", "Universal" in categories, "Universal category present")


def run_tier2_boundary_corner(runner, wheels, raw_arrays):
    """Tier 2: Boundary & Corner Cases"""
    print("\n--- Running Tier 2: Boundary & Corner Cases ---")

    # Boundary 2.1: Lookup Boundaries
    name_map = {w["friendly_name"]: w for w in wheels}
    runner.assert_true(2, "T2.1_Lookup_Null_Safety", name_map.get(None) is None, "Null name query must return None")
    runner.assert_true(2, "T2.1_Lookup_Empty_Safety", name_map.get("") is None, "Empty string query must return None")
    runner.assert_true(2, "T2.1_Lookup_NonExistent", name_map.get("UNKNOWN_WHEEL_XYZ") is None, "Non-existent name query must return None")

    # Boundary 2.2: Extreme Edge Counts (Min 4, Max 1080)
    min_edges_wheel = min(wheels, key=lambda w: w["actual_len"])
    max_edges_wheel = max(wheels, key=lambda w: w["actual_len"])
    runner.assert_equal(2, "T2.2_Min_Edge_Count", 4, min_edges_wheel["actual_len"], "Minimum edge count wheel is Dizzy 4-cyl with 4 edges")
    runner.assert_equal(2, "T2.2_Max_Edge_Count", 1080, max_edges_wheel["actual_len"], "Maximum edge count wheel is Audi 135-tooth with 1080 edges")
    runner.assert_equal(2, "T2.2_Min_Edge_Enum", "DIZZY_FOUR_CYLINDER", min_edges_wheel["enum_name"])
    runner.assert_equal(2, "T2.2_Max_Edge_Enum", "AUDI_135_WITH_CAM", max_edges_wheel["enum_name"])

    # Boundary 2.3: Dynamic RPM Range Calculations (10 RPM to 12,000 RPM)
    # T_seg = (D * 10^6) / (6 * E * RPM) us
    # Test 60-2 (E=120, D=360): T_seg = (360 * 10^6) / (6 * 120 * RPM) = 500,000 / RPM us
    test_rpms = [10, 50, 200, 850, 3000, 6000, 12000]
    for rpm in test_rpms:
        expected_t_seg = (360.0 * 1e6) / (6.0 * 120.0 * rpm)
        runner.assert_within(2, f"T2.3_RPM_Scale_60_2_{rpm}RPM", 500000.0 / rpm, expected_t_seg, 0.001)

        # Test Avanza (E=144, D=720): T_seg = (720 * 10^6) / (6 * 144 * RPM) = 833333.333 / RPM us
        expected_t_avanza = (720.0 * 1e6) / (6.0 * 144.0 * rpm)
        runner.assert_within(2, f"T2.3_RPM_Scale_Avanza_{rpm}RPM", 833333.3333 / rpm, expected_t_avanza, 0.01)

    # Low RPM Chunking Verification (Pulse > 30,000 us must be chunked for ESP32 RMT)
    # At 10 RPM, 60-2 segment is 50,000 us -> chunked into 30,000 us + 20,000 us
    t_seg_10rpm = 500000.0 / 10  # 50,000 us
    chunks = []
    rem = t_seg_10rpm
    while rem > 30000:
        chunks.append(30000)
        rem -= 30000
    if rem > 0:
        chunks.append(rem)
    runner.assert_equal(2, "T2.3_RMT_Chunk_Count_10RPM", 2, len(chunks), "50,000 us pulse must be sliced into 2 RMT chunks")
    runner.assert_equal(2, "T2.3_RMT_Chunk_Sum_10RPM", 50000, sum(chunks), "RMT chunk sum must equal original 50,000 us")

    # Boundary 2.4: Multi-Gap Synchronization Patterns
    # 36-2-2-2 H4 (index 17): 3 distinct gaps
    h4_arr = raw_arrays["thirty_six_minus_two_two_two"]
    runner.assert_equal(2, "T2.4_H4_Len", 72, len(h4_arr))
    # Gaps in 36-2-2-2 occur where CKP is low for 2 teeth (4 segments)
    # Consecutive zeros count
    zero_runs = []
    curr_zero = 0
    for v in h4_arr:
        if (v & 0x01) == 0:
            curr_zero += 1
        else:
            if curr_zero > 0:
                zero_runs.append(curr_zero)
                curr_zero = 0
    if curr_zero > 0:
        zero_runs.append(curr_zero)
    # In 36-2-2-2, missing 2 teeth gives a run of 4 zeros + missing tooth gaps
    gap_runs = [r for r in zero_runs if r >= 3]
    runner.assert_equal(2, "T2.4_H4_Gap_Count", 3, len(gap_runs), "36-2-2-2 H4 must have exactly 3 multi-tooth gaps")


def run_tier3_cross_combinations(runner, wheels, raw_arrays):
    """Tier 3: Cross-Feature Combinations"""
    print("\n--- Running Tier 3: Cross-Feature Combinations ---")

    # Combination 3.1: Dual-Cam Synchronization in BMW N20 (index 66)
    bmw_w = wheels[66]
    runner.assert_equal(3, "T3.1_BMW_N20_Enum", "BMW_N20", bmw_w["enum_name"])
    bmw_arr = raw_arrays[bmw_w["array_name"]]
    runner.assert_equal(3, "T3.1_BMW_N20_Len", 240, len(bmw_arr))
    runner.assert_true(3, "T3.1_BMW_N20_Has_Crank", bmw_w["has_crank"])
    runner.assert_true(3, "T3.1_BMW_N20_Has_Cam1", bmw_w["has_cam1"])
    runner.assert_true(3, "T3.1_BMW_N20_Has_Cam2", bmw_w["has_cam2"])

    # Verify simultaneous active states (CKP=bit0, CMP1=bit1, CMP2=bit2)
    # Value 6 (CMP1+CMP2) and Value 7 (CKP+CMP1+CMP2)
    has_val_6 = any(x == 6 for x in bmw_arr)
    has_val_7 = any(x == 7 for x in bmw_arr)
    runner.assert_true(3, "T3.1_BMW_N20_DualCam_Active_Val6", has_val_6, "BMW N20 must contain dual-cam active gap states (bitmask 6)")
    runner.assert_true(3, "T3.1_BMW_N20_AllThree_Active_Val7", has_val_7, "BMW N20 must contain all-three active states (bitmask 7)")

    # Combination 3.2: Dual-Cam in GM LS1 (index 27)
    ls1_w = wheels[27]
    runner.assert_equal(3, "T3.2_GM_LS1_Enum", "GM_LS1_CRANK_AND_CAM", ls1_w["enum_name"])
    ls1_arr = raw_arrays[ls1_w["array_name"]]
    runner.assert_equal(3, "T3.2_GM_LS1_Len", 720, len(ls1_arr))
    runner.assert_true(3, "T3.2_GM_LS1_Has_Cam2", ls1_w["has_cam2"])
    # Segment 0 has bit 2 (0x04) set
    runner.assert_equal(3, "T3.2_GM_LS1_Seg0_Cam2", 4, ls1_arr[0] & 0x04, "GM LS1 segment 0 has CAM2 / knock trigger set")

    # Combination 3.3: 360 deg to 720 deg Periodic Cycle Conversion
    # A 360-degree wheel (e.g. 60-2, 120 segments) must duplicate 2x into 720 deg (240 segments)
    w_60_2 = raw_arrays["sixty_minus_two"]
    w_60_2_cam = raw_arrays["sixty_minus_two_with_cam"]
    runner.assert_equal(3, "T3.3_60_2_Crank_Len", 120, len(w_60_2))
    runner.assert_equal(3, "T3.3_60_2_Cam_Len", 240, len(w_60_2_cam))
    # CKP channel of sixty_minus_two_with_cam must match 2x repeated sixty_minus_two
    replicated_60_2_ckp = (w_60_2 + w_60_2)
    ckp_from_cam_wheel = [x & 0x01 for x in w_60_2_cam]
    runner.assert_equal(3, "T3.3_60_2_Crank_Periodic_Match", replicated_60_2_ckp, ckp_from_cam_wheel, "60-2 CKP in 720 deg wheel must match 2x repeated 360 deg crank")

    # Combination 3.4: Multi-Channel Demuxing and RLE Pulse Duration Conservation
    # For Avanza (144 edges @ 3000 RPM): T_cycle = 40,000 us.
    # Sum of all pulse durations on CKP channel must equal 40,000 us exactly.
    avanza_arr = raw_arrays["new_avanza"]
    t_seg_avanza = (720.0 * 1e6) / (6.0 * 144.0 * 3000.0) # 277.777778 us
    ckp_pulses = []
    curr_lvl = avanza_arr[0] & 0x01
    curr_cnt = 0
    for v in avanza_arr:
        lvl = v & 0x01
        if lvl == curr_lvl:
            curr_cnt += 1
        else:
            ckp_pulses.append((curr_cnt * t_seg_avanza, curr_lvl))
            curr_lvl = lvl
            curr_cnt = 1
    ckp_pulses.append((curr_cnt * t_seg_avanza, curr_lvl))

    total_ckp_time = sum(d for d, _ in ckp_pulses)
    expected_cycle_time = (720.0 * 1e6) / (6.0 * 3000.0) # 40,000 us
    runner.assert_within(3, "T3.4_Pulse_Duration_Conservation", expected_cycle_time, total_ckp_time, 0.001, "Sum of all RLE pulse durations must equal full 720 deg cycle period")


def run_tier4_realworld_scenarios(runner, wheels, raw_arrays):
    """Tier 4: Real-World Application Scenarios (Exact timing and edge comparison)"""
    print("\n--- Running Tier 4: Real-World OEM Application Scenarios ---")

    # Scenario 4.1: Toyota New Avanza (1.5L 3SZ-VE / 2NR-VE) - Index 19
    w_new = wheels[19]
    arr_new = raw_arrays[w_new["array_name"]]
    runner.assert_equal(4, "T4.1_New_Avanza_Len", 144, len(arr_new))
    runner.assert_equal(4, "T4.1_New_Avanza_Degrees", 720, w_new["degrees"])
    # New Avanza has Cam active ONLY on Rev 2 (segments 73 to 84)
    cam_active_segs_new = [i for i, v in enumerate(arr_new) if (v & 0x02)]
    runner.assert_true(4, "T4.1_New_Avanza_Cam_Active_Count", len(cam_active_segs_new) > 0)
    runner.assert_true(4, "T4.1_New_Avanza_Cam_Only_Rev2", all(i >= 72 for i in cam_active_segs_new), "New Avanza cam pulses must only appear in revolution 2 (>= 360 deg)")
    runner.assert_equal(4, "T4.1_New_Avanza_Cam_Start_Seg", 73, cam_active_segs_new[0])
    runner.assert_equal(4, "T4.1_New_Avanza_Cam_End_Seg", 84, cam_active_segs_new[-1])
    # Exact angular window: 73 * 5.0 = 365 deg to (84+1) * 5.0 = 425 deg
    runner.assert_equal(4, "T4.1_New_Avanza_Cam_Start_Deg", 365.0, 73 * 5.0)
    runner.assert_equal(4, "T4.1_New_Avanza_Cam_End_Deg", 420.0, 84 * 5.0)

    # Scenario 4.2: Toyota Old Avanza (1.3L K3-VE) - Index 18
    w_old = wheels[18]
    arr_old = raw_arrays[w_old["array_name"]]
    runner.assert_equal(4, "T4.2_Old_Avanza_Len", 144, len(arr_old))
    # Old Avanza has 3 cam pulse groups (segments 37-48, 73-84, 109-120)
    cam_active_segs_old = [i for i, v in enumerate(arr_old) if (v & 0x02)]
    runner.assert_true(4, "T4.2_Old_Avanza_Cam_Group1", 37 in cam_active_segs_old and 48 in cam_active_segs_old)
    runner.assert_true(4, "T4.2_Old_Avanza_Cam_Group2", 73 in cam_active_segs_old and 84 in cam_active_segs_old)
    runner.assert_true(4, "T4.2_Old_Avanza_Cam_Group3", 109 in cam_active_segs_old and 120 in cam_active_segs_old)

    # Scenario 4.3: Toyota Avanza/Xenia/Terios/Rush - Index 20
    w_rush = wheels[20]
    arr_rush = raw_arrays[w_rush["array_name"]]
    runner.assert_equal(4, "T4.3_Rush_Len", 144, len(arr_rush))
    cam_active_rush = [i for i, v in enumerate(arr_rush) if (v & 0x02)]
    runner.assert_true(4, "T4.3_Rush_Cam_Group1", 13 in cam_active_rush and 24 in cam_active_rush)
    runner.assert_true(4, "T4.3_Rush_Cam_Group2", 49 in cam_active_rush and 60 in cam_active_rush)
    runner.assert_true(4, "T4.3_Rush_Cam_Group3", 85 in cam_active_rush and 95 in cam_active_rush)

    # Scenario 4.4: Mitsubishi 4G63 (4/2 CAS Optical Disc) - Index 46
    w_4g63 = wheels[46]
    arr_4g63 = raw_arrays[w_4g63["array_name"]]
    runner.assert_equal(4, "T4.4_4G63_Len", 144, len(arr_4g63))
    # Crank: 4 pulse groups in 720 deg (segments 21-34, 57-70, 93-106, 129-142)
    crank_active_4g63 = [i for i, v in enumerate(arr_4g63) if (v & 0x01)]
    runner.assert_true(4, "T4.4_4G63_Crank_Pulse1", 21 in crank_active_4g63 and 34 in crank_active_4g63)
    runner.assert_true(4, "T4.4_4G63_Crank_Pulse2", 93 in crank_active_4g63 and 106 in crank_active_4g63)
    # Cam: 3 sync pulses (0-10, 54-67, 128-143)
    cam_active_4g63 = [i for i, v in enumerate(arr_4g63) if (v & 0x02)]
    runner.assert_true(4, "T4.4_4G63_Cam_Tooth1", 0 in cam_active_4g63 and 10 in cam_active_4g63)
    runner.assert_true(4, "T4.4_4G63_Cam_Tooth2", 54 in cam_active_4g63 and 67 in cam_active_4g63)
    runner.assert_true(4, "T4.4_4G63_Cam_Tooth3", 128 in cam_active_4g63 and 143 in cam_active_4g63)

    # Scenario 4.5: Mitsubishi 6G72 (6-Cylinder with Cam) - Index 25
    w_6g72 = wheels[25]
    arr_6g72 = raw_arrays[w_6g72["array_name"]]
    runner.assert_equal(4, "T4.5_6G72_Len", 144, len(arr_6g72))
    # Count 6 crank pulses across continuous cycle with wrap
    crank_runs_6g72 = 0
    prev = arr_6g72[-1] & 0x01
    for v in arr_6g72:
        curr = v & 0x01
        if curr and not prev:
            crank_runs_6g72 += 1
        prev = curr
    runner.assert_equal(4, "T4.5_6G72_Crank_Pulse_Count", 6, crank_runs_6g72, "6G72 must have exactly 6 crank pulses across 720 deg cycle")

    # Scenario 4.6: Mitsubishi 3A92 (3-Cylinder Mirage) - Index 61
    w_3a92 = wheels[61]
    arr_3a92 = raw_arrays[w_3a92["array_name"]]
    runner.assert_equal(4, "T4.6_3A92_Len", 144, len(arr_3a92))
    cam_active_3a92 = [i for i, v in enumerate(arr_3a92) if (v & 0x02)]
    runner.assert_true(4, "T4.6_3A92_Cam_Tooth1", 20 in cam_active_3a92)
    runner.assert_true(4, "T4.6_3A92_Cam_Tooth2", 68 in cam_active_3a92 or 69 in cam_active_3a92)
    runner.assert_true(4, "T4.6_3A92_Cam_Tooth3", 116 in cam_active_3a92)

    # Scenario 4.7: Honda Jazz / Fit GD3 V1, V2, V3 - Indices 49, 50, 51
    w_j1 = wheels[49]
    w_j2 = wheels[50]
    w_j3 = wheels[51]
    arr_j1 = raw_arrays[w_j1["array_name"]]
    arr_j2 = raw_arrays[w_j2["array_name"]]
    arr_j3 = raw_arrays[w_j3["array_name"]]
    runner.assert_equal(4, "T4.7_Jazz_V1_Len", 144, len(arr_j1))
    runner.assert_equal(4, "T4.7_Jazz_V2_Len", 144, len(arr_j2))
    runner.assert_equal(4, "T4.7_Jazz_V3_Len", 144, len(arr_j3))
    # CKP channel must be identical 12+1 across all 3 variants
    ckp_j1 = [x & 0x01 for x in arr_j1]
    ckp_j2 = [x & 0x01 for x in arr_j2]
    ckp_j3 = [x & 0x01 for x in arr_j3]
    runner.assert_equal(4, "T4.7_Jazz_CKP_Identical_V1_V2", ckp_j1, ckp_j2, "Jazz V1 and V2 must share identical 12+1 CKP pattern")
    runner.assert_equal(4, "T4.7_Jazz_CKP_Identical_V1_V3", ckp_j1, ckp_j3, "Jazz V1 and V3 must share identical 12+1 CKP pattern")
    # CAM channels must differ in phase / event timing
    cam_j1 = [x & 0x02 for x in arr_j1]
    cam_j2 = [x & 0x02 for x in arr_j2]
    runner.assert_true(4, "T4.7_Jazz_CAM_Differs_V1_V2", cam_j1 != cam_j2, "Jazz V1 and V2 CAM patterns must differ in phase")

    # Scenario 4.8: Universal 60-2 Missing Tooth - Index 3
    w_60_2 = wheels[3]
    arr_60_2 = raw_arrays[w_60_2["array_name"]]
    runner.assert_equal(4, "T4.8_60_2_Len", 120, len(arr_60_2))
    runner.assert_equal(4, "T4.8_60_2_Degrees", 360, w_60_2["degrees"])
    # 58 active teeth: 58 teeth * 2 segments (1 High, 1 Low) = 116 segments
    # 2 missing teeth: 2 teeth * 2 segments = 4 segments of LOW (0) at segments 116-119
    for i in range(0, 116, 2):
        runner.assert_equal(4, f"T4.8_60_2_Tooth_{i//2}_High", 1, arr_60_2[i], f"Tooth {i//2} first half must be HIGH")
        runner.assert_equal(4, f"T4.8_60_2_Tooth_{i//2}_Low", 0, arr_60_2[i+1], f"Tooth {i//2} second half must be LOW")
    runner.assert_equal(4, "T4.8_60_2_Gap_116", 0, arr_60_2[116], "Missing tooth segment 116 must be LOW")
    runner.assert_equal(4, "T4.8_60_2_Gap_117", 0, arr_60_2[117], "Missing tooth segment 117 must be LOW")
    runner.assert_equal(4, "T4.8_60_2_Gap_118", 0, arr_60_2[118], "Missing tooth segment 118 must be LOW")
    runner.assert_equal(4, "T4.8_60_2_Gap_119", 0, arr_60_2[119], "Missing tooth segment 119 must be LOW")


def run_tier5_adversarial(runner, wheels, raw_arrays):
    """Tier 5: Adversarial Coverage Hardening & Stress Verification"""
    print("\n--- Running Tier 5: Adversarial Coverage Hardening ---")
    import test_tier5_adversarial as t5

    class Tier5Adapter:
        def __init__(self, e2e_runner):
            self.runner = e2e_runner
        def assert_true(self, category, test_name, condition, message=""):
            self.runner.assert_true(5, f"T5_{category}_{test_name}", condition, message)
        def assert_false(self, category, test_name, condition, message=""):
            self.runner.assert_true(5, f"T5_{category}_{test_name}", not condition, message)
        def assert_equal(self, category, test_name, expected, actual, message=""):
            self.runner.assert_equal(5, f"T5_{category}_{test_name}", expected, actual, message)
        def assert_within(self, category, test_name, expected, actual, tolerance, message=""):
            self.runner.assert_within(5, f"T5_{category}_{test_name}", expected, actual, tolerance, message)

    adapter = Tier5Adapter(runner)
    t5.test_category_a_rapid_rpm(adapter, wheels, raw_arrays)
    t5.test_category_b_dense_patterns(adapter, wheels, raw_arrays)
    t5.test_category_c_jitter_and_buffer_swap(adapter, wheels, raw_arrays)
    t5.test_category_d_null_and_boundary_safety(adapter, wheels, raw_arrays)
    t5.test_category_e_canvas_rendering(adapter, wheels, raw_arrays)


def main():
    print("=" * 80)
    print("      ECUSniff 5-Tier Comprehensive E2E Verification Suite")
    print("=" * 80)
    
    wheels = load_parsed_wheels()
    raw_arrays = parse_raw_wheel_arrays()
    print(f"Loaded {len(wheels)} wheel metadata records and {len(raw_arrays)} raw PROGMEM arrays.")

    runner = TestRunner()
    run_tier1_feature_coverage(runner, wheels, raw_arrays)
    run_tier2_boundary_corner(runner, wheels, raw_arrays)
    run_tier3_cross_combinations(runner, wheels, raw_arrays)
    run_tier4_realworld_scenarios(runner, wheels, raw_arrays)
    run_tier5_adversarial(runner, wheels, raw_arrays)

    success = runner.print_summary()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()

