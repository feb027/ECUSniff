#!/usr/bin/env python3
"""
ECUSniff Tier 5 Adversarial Coverage Hardening & Stress Verification Suite
Author: Empirical Challenger
Platform: ESP32-S3 / Python 3 Oracle

Tests:
- Category A: Rapid & Extreme RPM Transitions (10 -> 12,000 -> 600 -> 10 RPM, 0 RPM, 100k RPM, integer scaling)
- Category B: Dense Optical Patterns & Memory Block Capacity (360 CAS, Optispark LT1, Audi 135-tooth 1080 edges)
- Category C: Pulse Train Jitter & Double-Buffer Swap Atomicity (zero cumulative drift, state machine simulation)
- Category D: Zero-Length, NULL Pointer & Boundary Safety across all Public APIs
- Category E: Waveform Canvas Rendering & Multi-Channel Track Geometry (1, 2, 3 tracks on small/large/extreme canvases)
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
    if not os.path.exists(WHEEL_DEFS_PATH):
        raise FileNotFoundError(f"Missing {WHEEL_DEFS_PATH}")
    with open(WHEEL_DEFS_PATH, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
    clean_code = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    clean_code = re.sub(r'//.*', '', clean_code)
    arrays = {}
    pattern = r'const\s+unsigned\s+char\s+(\w+)\s*\[\]\s*(?:PROGMEM)?\s*=\s*\{(.*?)\};'
    for m in re.finditer(pattern, clean_code, re.DOTALL):
        arrays[m.group(1)] = [int(n) for n in re.findall(r'\b\d+\b', m.group(2))]
    return arrays

class Tier5TestRunner:
    def __init__(self):
        self.total_tests = 0
        self.passed_tests = 0
        self.failed_tests = 0
        self.categories = {
            "A_Rapid_RPM": [],
            "B_Dense_Patterns": [],
            "C_Jitter_Buffer_Swap": [],
            "D_Null_Boundary_Safety": [],
            "E_Canvas_Rendering": []
        }

    def assert_true(self, category, test_name, condition, message=""):
        self.total_tests += 1
        if condition:
            self.passed_tests += 1
            self.categories[category].append((test_name, True, message))
        else:
            self.failed_tests += 1
            self.categories[category].append((test_name, False, message))
            print(f"[FAIL] [{category}] {test_name}: {message}")

    def assert_equal(self, category, test_name, expected, actual, message=""):
        cond = (expected == actual)
        msg = message or f"Expected '{expected}', got '{actual}'"
        self.assert_true(category, test_name, cond, msg)

    def assert_within(self, category, test_name, expected, actual, tolerance, message=""):
        cond = abs(expected - actual) <= tolerance
        msg = message or f"Expected {expected} +/- {tolerance}, got {actual}"
        self.assert_true(category, test_name, cond, msg)

    def assert_false(self, category, test_name, condition, message=""):
        self.assert_true(category, test_name, not condition, message or "Expected condition to be False")

    def print_summary(self):
        print("\n" + "=" * 80)
        print("         TIER 5 ADVERSARIAL COVERAGE HARDENING TEST SUMMARY")
        print("=" * 80)
        for cat, tests in self.categories.items():
            t_passed = sum(1 for _, p, _ in tests if p)
            t_total = len(tests)
            status = "PASS" if t_passed == t_total and t_total > 0 else "FAIL"
            print(f"Category {cat:25s}: {t_passed:4d}/{t_total:4d} passed [{status}]")
        print("-" * 80)
        print(f"TOTAL TIER 5 TESTS: {self.passed_tests}/{self.total_tests} passed ({self.failed_tests} failed)")
        print("=" * 80)
        return self.failed_tests == 0


def compile_bit_array_to_rmt_sim(bit_array, total_edges, cycle_degrees, rpm, channel_mask, max_items=512, max_chunk=30000):
    """Exact emulation of C++ RmtGenerator::compileBitArrayToRmt"""
    if bit_array is None or total_edges == 0 or rpm == 0 or max_items < 2:
        return 0, []

    if cycle_degrees != 360 and cycle_degrees != 720:
        cycle_degrees = 720 if cycle_degrees == 0 else cycle_degrees

    cycle_total_us = (cycle_degrees * 1000000) // (6 * rpm)
    if cycle_total_us == 0:
        cycle_total_us = 1

    max_phases = (max_items - 1) * 2
    compile_phases = []

    curr_lvl = 1 if (bit_array[0] & channel_mask) else 0
    run_start = 0

    for s in range(1, total_edges):
        lvl = 1 if (bit_array[s] & channel_mask) else 0
        if lvl != curr_lvl:
            t_start_us = (run_start * cycle_total_us) // total_edges
            t_end_us = (s * cycle_total_us) // total_edges
            run_dur_us = t_end_us - t_start_us

            rem = run_dur_us
            while rem > max_chunk and len(compile_phases) < max_phases:
                compile_phases.append((max_chunk, curr_lvl))
                rem -= max_chunk
            if rem > 0 and len(compile_phases) < max_phases:
                compile_phases.append((rem, curr_lvl))

            curr_lvl = lvl
            run_start = s

    # Final run
    t_start_us = (run_start * cycle_total_us) // total_edges
    t_end_us = cycle_total_us
    run_dur_us = t_end_us - t_start_us

    rem = run_dur_us
    while rem > max_chunk and len(compile_phases) < max_phases:
        compile_phases.append((max_chunk, curr_lvl))
        rem -= max_chunk
    if rem > 0 and len(compile_phases) < max_phases:
        compile_phases.append((rem, curr_lvl))

    # Pack into items
    items = []
    for i in range(0, len(compile_phases), 2):
        if len(items) >= (max_items - 1):
            break
        d0, l0 = compile_phases[i]
        d1, l1 = compile_phases[i+1] if (i + 1 < len(compile_phases)) else (0, 0)
        items.append({"duration0": d0, "level0": l0, "duration1": d1, "level1": l1})

    # Append EOT {0,0,0,0}
    items.append({"duration0": 0, "level0": 0, "duration1": 0, "level1": 0})
    return len(items), items, compile_phases


# ============================================================================
# CATEGORY A: RAPID & EXTREME RPM TRANSITIONS
# ============================================================================
def test_category_a_rapid_rpm(runner, wheels, raw_arrays):
    print("\n--- Running Tier 5 Category A: Rapid & Extreme RPM Transitions ---")

    # A.1: Rapid RPM Stress Sequences (10 -> 12000 -> 600 -> 10 -> 850 -> 3000 -> 12000 -> 200 -> 0 -> 850)
    rpm_transition_seq = [10, 12000, 600, 10, 850, 3000, 12000, 200, 0, 850]
    sample_wheels = [
        ("SIXTY_MINUS_TWO", 3),
        ("NEW_AVANZA", 19),
        ("OLD_AVANZA", 18),
        ("MITSUBISHI_4G63", 46),
        ("BMW_N20", 66),
        ("AUDI_135_WITH_CAM", 47)
    ]

    for wheel_label, wheel_idx in sample_wheels:
        w = wheels[wheel_idx]
        raw_arr = raw_arrays[w["array_name"]]
        deg = w["degrees"]
        if deg == 360:
            arr = raw_arr + raw_arr
            edges = len(arr)
            deg = 720
        else:
            arr = raw_arr
            edges = len(arr)

        for step_i, rpm in enumerate(rpm_transition_seq):
            if rpm == 0:
                count, items = compile_bit_array_to_rmt_sim(arr, edges, deg, rpm, 0x01)[:2]
                runner.assert_equal("A_Rapid_RPM", f"A.1_{wheel_label}_RPM_0_Rejection", 0, count, "RPM=0 must return 0 items safely")
                continue

            count, items, phases = compile_bit_array_to_rmt_sim(arr, edges, deg, rpm, 0x01)
            runner.assert_true("A_Rapid_RPM", f"A.1_{wheel_label}_SeqStep_{step_i}_{rpm}RPM_NonZero", count > 0)
            runner.assert_true("A_Rapid_RPM", f"A.1_{wheel_label}_SeqStep_{step_i}_{rpm}RPM_EOT",
                               items[-1]["duration0"] == 0 and items[-1]["duration1"] == 0, "Last item must be EOT marker")
            
            # Mathematical duration conservation
            expected_cycle_us = (deg * 1000000) // (6 * rpm)
            total_dur = sum(p[0] for p in phases)
            runner.assert_equal("A_Rapid_RPM", f"A.1_{wheel_label}_SeqStep_{step_i}_{rpm}RPM_CycleSum",
                                expected_cycle_us, total_dur, "Pulse sum must match theoretical cycle period")

    # A.2: Step-by-Step Rapid RPM Sweeps across All 70 Wheels (10 to 12,000 RPM)
    sweep_rpms = [10, 50, 100, 250, 500, 750, 850, 1000, 2500, 5000, 7500, 10000, 12000]
    for w in wheels:
        arr = raw_arrays[w["array_name"]]
        edges = len(arr)
        deg = w["degrees"]
        if deg == 360:
            arr = arr + arr
            edges = edges * 2
            deg = 720
        for rpm in sweep_rpms:
            exp_cycle = (deg * 1000000) // (6 * rpm)
            count, items, phases = compile_bit_array_to_rmt_sim(arr, edges, deg, rpm, 0x01)
            total_dur = sum(p[0] for p in phases)
            runner.assert_equal("A_Rapid_RPM", f"A.2_Sweep_{w['enum_name']}_{rpm}RPM", exp_cycle, total_dur)

    # A.3: Extreme RPM Boundary Checks
    # Minimum Spec RPM (10 RPM): 12,000,000 us cycle period safely fits in 512 item buffer
    count, items, phases = compile_bit_array_to_rmt_sim(raw_arrays["new_avanza"], 144, 720, 10, 0x01)
    runner.assert_true("A_Rapid_RPM", "A.3_RPM_10_Slicing_Valid", count > 0)
    runner.assert_true("A_Rapid_RPM", "A.3_RPM_10_Fits_Buffer", count <= 512)
    runner.assert_equal("A_Rapid_RPM", "A.3_RPM_10_Cycle_Sum", 12000000, sum(p[0] for p in phases))

    # Ultra high speed simulation (RPM = 100,000)
    count, items, phases = compile_bit_array_to_rmt_sim(raw_arrays["new_avanza"], 144, 720, 100000, 0x01)
    runner.assert_true("A_Rapid_RPM", "A.3_RPM_100k_Valid", count > 0)
    runner.assert_equal("A_Rapid_RPM", "A.3_RPM_100k_Cycle_Sum", 1200, sum(p[0] for p in phases))


# ============================================================================
# CATEGORY B: DENSE OPTICAL PATTERNS & MEMORY BLOCK CAPACITY
# ============================================================================
def test_category_b_dense_patterns(runner, wheels, raw_arrays):
    print("\n--- Running Tier 5 Category B: Dense Optical Patterns & Memory Block Capacity ---")

    # B.1: Audi 135-Tooth with Cam (1080 edges / 540 transitions) - Max Edges in Catalog
    w_audi = wheels[47] # AUDI_135_WITH_CAM
    arr_audi = raw_arrays[w_audi["array_name"]]
    runner.assert_equal("B_Dense_Patterns", "B.1_Audi135_Edge_Count", 1080, len(arr_audi))

    for rpm in [600, 3000, 6000, 12000]:
        count_ckp, items_ckp, phases_ckp = compile_bit_array_to_rmt_sim(arr_audi, 1080, 720, rpm, 0x01)
        runner.assert_true("B_Dense_Patterns", f"B.1_Audi135_CKP_Capacity_{rpm}RPM", count_ckp <= 512,
                           f"RMT items ({count_ckp}) must fit in MAX_CYCLE_PULSES (512)")
        runner.assert_true("B_Dense_Patterns", f"B.1_Audi135_CKP_Phases_{rpm}RPM", len(phases_ckp) <= 1024,
                           f"Phase count ({len(phases_ckp)}) must fit in compile phase buffer (1024)")
        runner.assert_equal("B_Dense_Patterns", f"B.1_Audi135_CKP_EOT_{rpm}RPM", 0, items_ckp[-1]["duration0"])

    # B.2: Optispark LT1 (720 edges / 720 transitions on CKP) - Index 15
    w_opti = wheels[15] # OPTISPARK_LT1
    arr_opti = raw_arrays[w_opti["array_name"]]
    runner.assert_equal("B_Dense_Patterns", "B.2_Optispark_Len", 720, len(arr_opti))
    count_opti, items_opti, phases_opti = compile_bit_array_to_rmt_sim(arr_opti, 720, 720, 6000, 0x01)
    runner.assert_equal("B_Dense_Patterns", "B.2_Optispark_Phase_Count", 720, len(phases_opti))
    runner.assert_equal("B_Dense_Patterns", "B.2_Optispark_Item_Count", 361, count_opti) # 360 pairs + 1 EOT
    runner.assert_true("B_Dense_Patterns", "B.2_Optispark_Fits_Max_Pulses", count_opti <= 512)

    # B.3: Nissan 360 CAS (720 edges / 720 transitions on CKP) - Index 38
    w_nissan = wheels[38] # THREE_SIXTY_NISSAN_CAS
    arr_nissan = raw_arrays[w_nissan["array_name"]]
    runner.assert_equal("B_Dense_Patterns", "B.2_Nissan360_Len", 720, len(arr_nissan))
    count_nissan, items_nissan, phases_nissan = compile_bit_array_to_rmt_sim(arr_nissan, 720, 720, 6000, 0x01)
    runner.assert_equal("B_Dense_Patterns", "B.2_Nissan360_Phase_Count", 720, len(phases_nissan))
    runner.assert_equal("B_Dense_Patterns", "B.2_Nissan360_Item_Count", 361, count_nissan)

    # B.4: Buffer Capacity Limit Clamping & Overflow Resilience
    small_max = 50
    count_clamped, items_clamped, _ = compile_bit_array_to_rmt_sim(arr_audi, 1080, 720, 3000, 0x01, max_items=small_max)
    runner.assert_equal("B_Dense_Patterns", "B.4_Buffer_Clamping_Item_Count", small_max, count_clamped)
    runner.assert_equal("B_Dense_Patterns", "B.4_Buffer_Clamping_EOT", 0, items_clamped[-1]["duration0"])
    runner.assert_equal("B_Dense_Patterns", "B.4_Buffer_Clamping_EOT_Dur1", 0, items_clamped[-1]["duration1"])


# ============================================================================
# CATEGORY C: PULSE TRAIN JITTER & DOUBLE-BUFFER SWAP ATOMICITY
# ============================================================================
def test_category_c_jitter_and_buffer_swap(runner, wheels, raw_arrays):
    print("\n--- Running Tier 5 Category C: Pulse Train Jitter & Double-Buffer Swap Atomicity ---")

    # C.1: Zero Cumulative Jitter Verification Across All 70 Presets
    for w in wheels:
        arr = raw_arrays[w["array_name"]]
        edges = len(arr)
        deg = w["degrees"]
        if deg == 360:
            arr = arr + arr
            edges = edges * 2
            deg = 720

        for rpm in [10, 600, 850, 3000, 6000, 12000]:
            cycle_total_us = (deg * 1000000) // (6 * rpm)
            _, _, phases = compile_bit_array_to_rmt_sim(arr, edges, deg, rpm, 0x01)
            actual_sum = sum(p[0] for p in phases)
            runner.assert_equal("C_Jitter_Buffer_Swap", f"C.1_Zero_Jitter_{w['enum_name']}_{rpm}RPM", cycle_total_us, actual_sum)

    # C.2: High-Speed Sub-Millisecond Pulse Train Jitter @ 12,000 RPM
    w_60_2 = raw_arrays["sixty_minus_two"] + raw_arrays["sixty_minus_two"]
    _, items_60_2, phases_60_2 = compile_bit_array_to_rmt_sim(w_60_2, 240, 720, 12000, 0x01)
    runner.assert_equal("C_Jitter_Buffer_Swap", "C.2_HighSpeed_60_2_Cycle_Sum", 10000, sum(p[0] for p in phases_60_2))

    # C.3: Double-Buffering State Machine Simulation
    class RmtDoubleBufferSim:
        def __init__(self):
            self.active_buf_idx = 0
            self.active_rpm = 850
            self.pending_rpm = 850
            self.needs_update = False
            self.active_wheel = None
            self.bufA = []
            self.bufB = []

        def set_wheel_pattern(self, wheel):
            if wheel is None: return False
            self.active_wheel = wheel
            self.needs_update = True
            return True

        def set_rpm(self, rpm):
            if rpm != self.pending_rpm:
                self.pending_rpm = rpm
                self.needs_update = True

        def prepare_next_cycle(self):
            if not self.needs_update or self.pending_rpm == 0 or self.active_wheel is None:
                return
            target_is_B = (self.active_buf_idx == 0)
            arr = raw_arrays[self.active_wheel["array_name"]]
            edges = len(arr)
            deg = self.active_wheel["degrees"]
            if deg == 360:
                arr = arr + arr
                edges = edges * 2
                deg = 720
            _, items, _ = compile_bit_array_to_rmt_sim(arr, edges, deg, self.pending_rpm, 0x01)
            if target_is_B:
                self.bufB = items
            else:
                self.bufA = items

        def swap_buffer(self):
            if not self.needs_update: return
            self.active_buf_idx = 1 if self.active_buf_idx == 0 else 0
            self.active_rpm = self.pending_rpm
            self.needs_update = False

    sim = RmtDoubleBufferSim()
    sim.set_wheel_pattern(wheels[19]) # New Avanza
    sim.prepare_next_cycle()
    sim.swap_buffer()
    runner.assert_equal("C_Jitter_Buffer_Swap", "C.3_Initial_Buffer_Idx", 1, sim.active_buf_idx)
    runner.assert_equal("C_Jitter_Buffer_Swap", "C.3_Initial_Active_RPM", 850, sim.active_rpm)
    runner.assert_true("C_Jitter_Buffer_Swap", "C.3_Initial_BufB_Populated", len(sim.bufB) > 0)

    # Dynamic transition to 3000 RPM
    sim.set_rpm(3000)
    runner.assert_true("C_Jitter_Buffer_Swap", "C.3_NeedsUpdate_True", sim.needs_update)
    sim.prepare_next_cycle() # Writes to inactive BufA
    runner.assert_equal("C_Jitter_Buffer_Swap", "C.3_Active_Idx_Unchanged_Before_Swap", 1, sim.active_buf_idx)
    runner.assert_equal("C_Jitter_Buffer_Swap", "C.3_Active_RPM_Unchanged_Before_Swap", 850, sim.active_rpm)
    
    sim.swap_buffer() # Atomic switch to BufA
    runner.assert_equal("C_Jitter_Buffer_Swap", "C.3_Active_Idx_Swapped", 0, sim.active_buf_idx)
    runner.assert_equal("C_Jitter_Buffer_Swap", "C.3_Active_RPM_Updated", 3000, sim.active_rpm)
    runner.assert_false("C_Jitter_Buffer_Swap", "C.3_NeedsUpdate_Cleared", sim.needs_update)


# ============================================================================
# CATEGORY D: ZERO-LENGTH, NULL POINTER & BOUNDARY SAFETY ACROSS PUBLIC APIS
# ============================================================================
def test_category_d_null_and_boundary_safety(runner, wheels, raw_arrays):
    print("\n--- Running Tier 5 Category D: Zero-Length, NULL Pointer & Boundary Safety ---")

    # D.1: RmtGenerator API Null and Zero-Length Robustness
    runner.assert_equal("D_Null_Boundary_Safety", "D.1_Null_BitArray", 0, compile_bit_array_to_rmt_sim(None, 144, 720, 850, 0x01)[0])
    runner.assert_equal("D_Null_Boundary_Safety", "D.1_Zero_TotalEdges", 0, compile_bit_array_to_rmt_sim(raw_arrays["new_avanza"], 0, 720, 850, 0x01)[0])
    runner.assert_equal("D_Null_Boundary_Safety", "D.1_Zero_RPM", 0, compile_bit_array_to_rmt_sim(raw_arrays["new_avanza"], 144, 720, 0, 0x01)[0])
    runner.assert_equal("D_Null_Boundary_Safety", "D.1_MaxItems_Zero", 0, compile_bit_array_to_rmt_sim(raw_arrays["new_avanza"], 144, 720, 850, 0x01, max_items=0)[0])
    runner.assert_equal("D_Null_Boundary_Safety", "D.1_MaxItems_One", 0, compile_bit_array_to_rmt_sim(raw_arrays["new_avanza"], 144, 720, 850, 0x01, max_items=1)[0])

    # D.2: WheelDatabase Out-Of-Bounds Lookups
    def get_wheel(idx):
        if 0 <= idx < len(wheels):
            return wheels[idx]
        return None

    def find_by_friendly_name(name):
        if name is None or len(name) == 0:
            return None
        for w in wheels:
            if w["friendly_name"] == name:
                return w
        for w in wheels:
            if w["friendly_name"].lower() == name.lower():
                return w
        return None

    runner.assert_equal("D_Null_Boundary_Safety", "D.2_GetWheel_OOB_70", None, get_wheel(70))
    runner.assert_equal("D_Null_Boundary_Safety", "D.2_GetWheel_OOB_255", None, get_wheel(255))
    runner.assert_equal("D_Null_Boundary_Safety", "D.2_GetWheel_OOB_Negative", None, get_wheel(-1))
    runner.assert_equal("D_Null_Boundary_Safety", "D.2_Find_Null", None, find_by_friendly_name(None))
    runner.assert_equal("D_Null_Boundary_Safety", "D.2_Find_Empty", None, find_by_friendly_name(""))
    runner.assert_equal("D_Null_Boundary_Safety", "D.2_Find_NonExistent", None, find_by_friendly_name("NON_EXISTENT_WHEEL"))
    runner.assert_equal("D_Null_Boundary_Safety", "D.2_Find_Partial", None, find_by_friendly_name("Toyota Avanza 1.5"))

    # D.3: Category Filtering Buffer Bounds
    def get_wheels_by_category(brand_str, max_out):
        matches = [w for w in wheels if brand_str == "ALL" or w["brand"] == brand_str]
        return len(matches), matches[:max_out]

    total_all, out_all = get_wheels_by_category("ALL", 10)
    runner.assert_equal("D_Null_Boundary_Safety", "D.3_Cat_All_Count", 70, total_all)
    runner.assert_equal("D_Null_Boundary_Safety", "D.3_Cat_All_Clamped", 10, len(out_all))

    total_toyota, out_toyota = get_wheels_by_category("Toyota/Daihatsu", 5)
    runner.assert_true("D_Null_Boundary_Safety", "D.3_Cat_Toyota_Present", total_toyota >= 3)
    runner.assert_equal("D_Null_Boundary_Safety", "D.3_Cat_Toyota_Clamped", min(total_toyota, 5), len(out_toyota))


# ============================================================================
# CATEGORY E: WAVEFORM CANVAS RENDERING & MULTI-CHANNEL TRACK GEOMETRY
# ============================================================================
def test_category_e_canvas_rendering(runner, wheels, raw_arrays):
    print("\n--- Running Tier 5 Category E: Waveform Canvas Rendering & Track Geometry ---")

    # E.1: Track Geometry Calculation Math Verification
    def calculate_track_geometry(canvas_w, canvas_h, num_tracks):
        if num_tracks == 0 or canvas_h <= 0 or canvas_w <= 0:
            return []
        header_h = 14 if canvas_h > 90 else 12
        usable_h = canvas_h - header_h - 4
        track_h = usable_h // num_tracks
        margin_y = (6 if track_h > 40 else 4) if canvas_h > 90 else 3

        tracks = []
        for i in range(num_tracks):
            y_top = header_h + 2 + (i * track_h)
            y_high = y_top + margin_y
            y_low = y_top + track_h - margin_y - 1
            tracks.append({"yTop": y_top, "yHigh": y_high, "yLow": y_low, "trackH": track_h})
        return tracks

    # Small Canvas: 448 x 76 px (Dashboard & Capture)
    tracks_small_2 = calculate_track_geometry(448, 76, 2)
    runner.assert_equal("E_Canvas_Rendering", "E.1_Small_2Track_Count", 2, len(tracks_small_2))
    runner.assert_true("E_Canvas_Rendering", "E.1_Small_2Track_Bounds_0", 0 <= tracks_small_2[0]["yHigh"] < tracks_small_2[0]["yLow"] < 76)
    runner.assert_true("E_Canvas_Rendering", "E.1_Small_2Track_Bounds_1", 0 <= tracks_small_2[1]["yHigh"] < tracks_small_2[1]["yLow"] < 76)
    runner.assert_true("E_Canvas_Rendering", "E.1_Small_2Track_No_Overlap", tracks_small_2[0]["yLow"] < tracks_small_2[1]["yTop"])

    tracks_small_3 = calculate_track_geometry(448, 76, 3)
    runner.assert_equal("E_Canvas_Rendering", "E.1_Small_3Track_Count", 3, len(tracks_small_3))
    runner.assert_true("E_Canvas_Rendering", "E.1_Small_3Track_Bounds_Last", tracks_small_3[2]["yLow"] < 76)

    # Large Canvas: 456 x 124 px (Wheel Browser)
    tracks_large_2 = calculate_track_geometry(456, 124, 2)
    runner.assert_equal("E_Canvas_Rendering", "E.1_Large_2Track_Count", 2, len(tracks_large_2))
    runner.assert_true("E_Canvas_Rendering", "E.1_Large_2Track_Bounds_Last", tracks_large_2[1]["yLow"] < 124)

    tracks_large_3 = calculate_track_geometry(456, 124, 3)
    runner.assert_equal("E_Canvas_Rendering", "E.1_Large_3Track_Count", 3, len(tracks_large_3))
    runner.assert_true("E_Canvas_Rendering", "E.1_Large_3Track_Bounds_Last", tracks_large_3[2]["yLow"] < 124)

    # E.2: Extreme Canvas Dimensions Safety
    tracks_zero = calculate_track_geometry(0, 0, 2)
    runner.assert_equal("E_Canvas_Rendering", "E.2_Zero_Canvas", 0, len(tracks_zero))

    tracks_negative = calculate_track_geometry(-100, -50, 2)
    runner.assert_equal("E_Canvas_Rendering", "E.2_Negative_Canvas", 0, len(tracks_negative))

    tracks_huge = calculate_track_geometry(480, 480, 3)
    runner.assert_equal("E_Canvas_Rendering", "E.2_Huge_Canvas_Count", 3, len(tracks_huge))
    runner.assert_true("E_Canvas_Rendering", "E.2_Huge_Canvas_Bounds", tracks_huge[2]["yLow"] < 480)

    # E.3: Trace Pixel Mapping & Horizontal Normalization (0-720 deg)
    def simulate_trace_rasterization(bit_array, total_edges, cycle_degrees, channel_mask, available_w=420):
        if bit_array is None or total_edges == 0 or available_w <= 0:
            return None
        num_total_segs = total_edges * 2 if cycle_degrees == 360 else total_edges
        pixel_levels = []
        for x in range(available_w):
            seg_start = (x * num_total_segs) // available_w
            seg_end = ((x + 1) * num_total_segs) // available_w
            if seg_end <= seg_start: seg_end = seg_start + 1
            if seg_end > num_total_segs: seg_end = num_total_segs

            has_high = False
            has_low = False
            for s in range(seg_start, seg_end):
                arr_idx = (s % total_edges) if cycle_degrees == 360 else s
                lvl = 1 if (bit_array[arr_idx] & channel_mask) else 0
                if lvl: has_high = True
                else: has_low = True

            if has_high and has_low:
                pixel_levels.append("TRANSITION")
            elif has_high:
                pixel_levels.append("HIGH")
            else:
                pixel_levels.append("LOW")
        return pixel_levels

    # 1-Channel Crank (60-2, 360 deg) -> replicated seamlessly
    pixels_60_2 = simulate_trace_rasterization(raw_arrays["sixty_minus_two"], 120, 360, 0x01, 420)
    runner.assert_equal("E_Canvas_Rendering", "E.3_60_2_Pixel_Count", 420, len(pixels_60_2))
    runner.assert_true("E_Canvas_Rendering", "E.3_60_2_Has_High_Low", "HIGH" in pixels_60_2 and "LOW" in pixels_60_2)
    # Gap appears twice in 720 deg (once in first half, once in second half)
    half1_gaps = [i for i in range(0, 210) if pixels_60_2[i] == "LOW" and (i < 5 or pixels_60_2[i-1] == "LOW")]
    half2_gaps = [i for i in range(210, 420) if pixels_60_2[i] == "LOW" and (i < 215 or pixels_60_2[i-1] == "LOW")]
    runner.assert_true("E_Canvas_Rendering", "E.3_60_2_Gap_Replicated_Half1", len(half1_gaps) > 0)
    runner.assert_true("E_Canvas_Rendering", "E.3_60_2_Gap_Replicated_Half2", len(half2_gaps) > 0)

    # 2-Channel (New Avanza, 720 deg) -> Cam active ONLY in 2nd half
    pixels_avanza_cam = simulate_trace_rasterization(raw_arrays["new_avanza"], 144, 720, 0x02, 420)
    first_half_cam = pixels_avanza_cam[:210]
    second_half_cam = pixels_avanza_cam[210:]
    runner.assert_true("E_Canvas_Rendering", "E.3_Avanza_Cam_Silent_Half1", all(p == "LOW" for p in first_half_cam))
    runner.assert_true("E_Canvas_Rendering", "E.3_Avanza_Cam_Active_Half2", any(p in ("HIGH", "TRANSITION") for p in second_half_cam))

    # 3-Channel (BMW N20, 720 deg) -> CKP + CMP1 + CMP2 all active
    pixels_bmw_ckp = simulate_trace_rasterization(raw_arrays["bmw_n20"], 240, 720, 0x01, 420)
    pixels_bmw_cmp1 = simulate_trace_rasterization(raw_arrays["bmw_n20"], 240, 720, 0x02, 420)
    pixels_bmw_cmp2 = simulate_trace_rasterization(raw_arrays["bmw_n20"], 240, 720, 0x04, 420)
    runner.assert_true("E_Canvas_Rendering", "E.3_BMW_N20_CKP_Active", any(p in ("HIGH", "TRANSITION") for p in pixels_bmw_ckp))
    runner.assert_true("E_Canvas_Rendering", "E.3_BMW_N20_CMP1_Active", any(p in ("HIGH", "TRANSITION") for p in pixels_bmw_cmp1))
    runner.assert_true("E_Canvas_Rendering", "E.3_BMW_N20_CMP2_Active", any(p in ("HIGH", "TRANSITION") for p in pixels_bmw_cmp2))


def main():
    print("=" * 80)
    print("   ECUSniff Tier 5 Adversarial Coverage Hardening & Stress Suite")
    print("=" * 80)
    
    wheels = load_parsed_wheels()
    raw_arrays = parse_raw_wheel_arrays()
    print(f"Loaded {len(wheels)} wheel metadata records and {len(raw_arrays)} raw arrays.")

    runner = Tier5TestRunner()
    test_category_a_rapid_rpm(runner, wheels, raw_arrays)
    test_category_b_dense_patterns(runner, wheels, raw_arrays)
    test_category_c_jitter_and_buffer_swap(runner, wheels, raw_arrays)
    test_category_d_null_and_boundary_safety(runner, wheels, raw_arrays)
    test_category_e_canvas_rendering(runner, wheels, raw_arrays)

    success = runner.print_summary()
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
