#!/usr/bin/env python3
"""
Adversarial Empirical Stress Test Suite for Milestone 2 & Milestone 3
Validates:
1. RMT Symbol Compilation across all 70 presets at speeds from 10 RPM to 12,000 RPM.
2. Pulse Slicing strictly <= 30,000 us for all channels.
3. Zero Cumulative Drift (sum of pulse durations == total cycle period) across all 70 wheels.
4. Multi-Channel Phase Alignment for BMW N20 and GM LS1 (CKP + CMP1 + CMP2).
5. Waveform Canvas Geometry across multiple canvas resolutions (456x124, 448x76, 400x100, etc.) ensuring 0 out-of-bounds pixel writes and correct track allocation.
"""

import sys
import os
import json
import re
import math

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
PARSED_JSON_PATH = os.path.join(ROOT_DIR, ".agents", "survey_spec_miner", "parsed_wheels.json")
WHEEL_DEFS_PATH = os.path.join(ROOT_DIR, "external", "ardustim-tftv2-touchscreen", "ardustim", "wheel_defs.h")

def load_data():
    if not os.path.exists(PARSED_JSON_PATH):
        raise FileNotFoundError(f"Missing {PARSED_JSON_PATH}")
    with open(PARSED_JSON_PATH, "r", encoding="utf-8") as f:
        wheels = json.load(f)

    if not os.path.exists(WHEEL_DEFS_PATH):
        raise FileNotFoundError(f"Missing {WHEEL_DEFS_PATH}")
    with open(WHEEL_DEFS_PATH, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    clean = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    clean = re.sub(r'//.*', '', clean)
    arrays = {}
    pattern = r'const\s+unsigned\s+char\s+(\w+)\s*\[\]\s*(?:PROGMEM)?\s*=\s*\{(.*?)\};'
    for m in re.finditer(pattern, clean, re.DOTALL):
        name = m.group(1)
        body = m.group(2)
        arrays[name] = [int(n) for n in re.findall(r'\b\d+\b', body)]
    return wheels, arrays

MAX_RMT_DURATION_CHUNK = 30000
MAX_CYCLE_PULSES = 512

def compile_bit_array_to_rmt(bit_array, total_edges, cycle_degrees, rpm, channel_bit_mask, max_items=MAX_CYCLE_PULSES):
    """Exact C++ implementation replica of RmtGenerator::compileBitArrayToRmt"""
    if not bit_array or total_edges == 0 or rpm == 0 or max_items < 2:
        return []

    if cycle_degrees not in (360, 720):
        cycle_degrees = 720 if cycle_degrees == 0 else cycle_degrees

    cycle_total_us = (cycle_degrees * 1000000) // (6 * rpm)
    if cycle_total_us == 0:
        cycle_total_us = 1

    max_phases = (max_items - 1) * 2
    phases = []
    curr_lvl = 1 if (bit_array[0] & channel_bit_mask) else 0
    run_start_seg = 0

    for s in range(1, total_edges):
        lvl = 1 if (bit_array[s] & channel_bit_mask) else 0
        if lvl != curr_lvl:
            t_start_us = (run_start_seg * cycle_total_us) // total_edges
            t_end_us = (s * cycle_total_us) // total_edges
            run_dur_us = t_end_us - t_start_us

            rem = run_dur_us
            while rem > MAX_RMT_DURATION_CHUNK and len(phases) < max_phases:
                phases.append((MAX_RMT_DURATION_CHUNK, curr_lvl))
                rem -= MAX_RMT_DURATION_CHUNK
            if rem > 0 and len(phases) < max_phases:
                phases.append((rem, curr_lvl))

            curr_lvl = lvl
            run_start_seg = s

    # Final run
    t_start_us = (run_start_seg * cycle_total_us) // total_edges
    t_end_us = cycle_total_us
    run_dur_us = t_end_us - t_start_us

    rem = run_dur_us
    while rem > MAX_RMT_DURATION_CHUNK and len(phases) < max_phases:
        phases.append((MAX_RMT_DURATION_CHUNK, curr_lvl))
        rem -= MAX_RMT_DURATION_CHUNK
    if rem > 0 and len(phases) < max_phases:
        phases.append((rem, curr_lvl))

    items = []
    for i in range(0, len(phases), 2):
        if len(items) >= max_items - 1:
            break
        d0, l0 = phases[i]
        if i + 1 < len(phases):
            d1, l1 = phases[i + 1]
        else:
            d1, l1 = 0, 0
        items.append({"d0": d0, "l0": l0, "d1": d1, "l1": l1})

    # Append EOT item
    items.append({"d0": 0, "l0": 0, "d1": 0, "l1": 0})
    return items

class VirtualTftCanvas:
    """Simulates TFT_eSprite 2D pixel buffer and tracks all drawing calls for bounds checks."""
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.oob_writes = []
        self.pixels = [[0 for _ in range(width)] for _ in range(height)]

    def _check_pixel(self, x, y):
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y][x] = 1
        else:
            self.oob_writes.append((x, y))

    def draw_pixel(self, x, y, color):
        self._check_pixel(x, y)

    def draw_fast_hline(self, x, y, w, color):
        for px in range(x, x + w):
            self._check_pixel(px, y)

    def draw_fast_vline(self, x, y, h, color):
        for py in range(y, y + h):
            self._check_pixel(x, py)

    def draw_rect(self, x, y, w, h, color):
        self.draw_fast_hline(x, y, w, color)
        self.draw_fast_hline(x, y + h - 1, w, color)
        self.draw_fast_vline(x, y, h, color)
        self.draw_fast_vline(x + w - 1, y, h, color)

    def draw_string(self, s, x, y):
        # 6x8 default font bounds estimation
        char_w = 6
        char_h = 8
        for ci, ch in enumerate(s):
            cx = x + ci * char_w
            for dy in range(char_h):
                for dx in range(char_w):
                    self._check_pixel(cx + dx, y + dy)


def calculate_track_geometry(height, num_tracks):
    """Calculates track geometry exactly as WaveformCanvas::_calculateTrackGeometry"""
    if num_tracks == 0:
        return []
    header_h = 14 if height > 90 else 12
    usable_h = height - header_h - 4
    track_h = usable_h // num_tracks
    margin_y = 6 if (height > 90 and track_h > 40) else (4 if height > 90 else 3)
    tracks = []
    for i in range(num_tracks):
        y_top = header_h + 2 + (i * track_h)
        y_high = y_top + margin_y
        y_low = y_top + track_h - margin_y - 1
        tracks.append({"y_top": y_top, "y_high": y_high, "y_low": y_low, "track_h": track_h})
    return tracks

def render_canvas_simulation(canvas, bit_array, total_edges, cycle_degrees, has_cmp1, has_cmp2):
    """Simulates WaveformCanvas::render and records all pixel accesses."""
    width = canvas.width
    height = canvas.height
    num_tracks = 3 if has_cmp2 else 2
    tracks = calculate_track_geometry(height, num_tracks)

    # 1. Grid
    canvas.draw_rect(0, 0, width, height, 0x39E7)
    x_offset = 28
    x_pad = 8
    available_w = width - x_offset - x_pad

    for i in range(1, 4):
        x = x_offset + (i * available_w) // 4
        for y in range(2, height - 2, 4):
            canvas.draw_pixel(x, y, 0x52AA)

    for i in range(1, num_tracks):
        canvas.draw_fast_hline(0, tracks[i]["y_top"] - 1, width, 0x2965)

    canvas.draw_string("0", x_offset - 2, 2)
    canvas.draw_string("360", x_offset + (available_w // 2) - 8, 2)
    canvas.draw_string("720", x_offset + available_w - 18, 2)

    if num_tracks >= 1:
        canvas.draw_string("CKP", 4, tracks[0]["y_high"])
    if num_tracks >= 2:
        canvas.draw_string("CM1" if has_cmp2 else "CMP", 4, tracks[1]["y_high"])
    if num_tracks >= 3:
        canvas.draw_string("CM2", 4, tracks[2]["y_high"])

    # 2. Traces
    def draw_trace(bit_mask, y_high, y_low):
        if not bit_array or total_edges == 0 or available_w <= 0:
            canvas.draw_fast_hline(x_offset, y_low, available_w, 0xFFFF)
            return

        num_total_segments = (total_edges * 2) if cycle_degrees == 360 else total_edges
        if num_total_segments == 0:
            canvas.draw_fast_hline(x_offset, y_low, available_w, 0xFFFF)
            return

        prev_end_level = 1 if (bit_array[0] & bit_mask) else 0

        for x in range(available_w):
            seg_start = (x * num_total_segments) // available_w
            seg_end = ((x + 1) * num_total_segments) // available_w
            if seg_end <= seg_start:
                seg_end = seg_start + 1
            if seg_end > num_total_segments:
                seg_end = num_total_segments

            has_high = False
            has_low = False
            first_level = 0
            last_level = 0

            for s in range(seg_start, seg_end):
                arr_idx = (s % total_edges) if (cycle_degrees == 360) else s
                lvl = 1 if (bit_array[arr_idx] & bit_mask) else 0
                if lvl:
                    has_high = True
                else:
                    has_low = True
                if s == seg_start:
                    first_level = lvl
                last_level = lvl

            px = x_offset + x

            # Transition from previous column
            if x > 0 and first_level != prev_end_level:
                y_a = y_high if prev_end_level else y_low
                y_b = y_high if first_level else y_low
                y_min = min(y_a, y_b)
                y_h = abs(y_b - y_a) + 1
                canvas.draw_fast_vline(px, y_min, y_h, 0xFFFF)

            # Within this column
            if has_high and has_low:
                y_min = min(y_high, y_low)
                y_h = abs(y_low - y_high) + 1
                canvas.draw_fast_vline(px, y_min, y_h, 0xFFFF)
            elif has_high:
                canvas.draw_pixel(px, y_high, 0xFFFF)
            else:
                canvas.draw_pixel(px, y_low, 0xFFFF)

            prev_end_level = last_level

    # Trace 0: CKP
    draw_trace(0x01, tracks[0]["y_high"], tracks[0]["y_low"])
    # Trace 1: CMP1
    draw_trace(0x02, tracks[1]["y_high"], tracks[1]["y_low"])
    # Trace 2: CMP2
    if has_cmp2:
        draw_trace(0x04, tracks[2]["y_high"], tracks[2]["y_low"])


def main():
    print("=" * 80)
    print("   ADVERSARIAL EMPIRICAL STRESS TEST SUITE: MILESTONE 2 & MILESTONE 3")
    print("=" * 80)

    wheels, arrays = load_data()
    print(f"Loaded {len(wheels)} wheel records and {len(arrays)} raw PROGMEM arrays.\n")

    total_checks = 0
    passed_checks = 0
    failed_checks = 0

    def record_check(name, condition, error_msg=""):
        nonlocal total_checks, passed_checks, failed_checks
        total_checks += 1
        if condition:
            passed_checks += 1
        else:
            failed_checks += 1
            print(f"[FAIL] {name}: {error_msg}")

    # =========================================================================
    # CHALLENGE 1: RMT Symbol Compilation across 70 Presets & Full RPM Gamut
    # =========================================================================
    print("--- [Challenge 1] Testing RMT compilation for 70 presets across 15 RPM points (10 to 12,000 RPM) ---")
    rpm_gamut = [10, 15, 25, 50, 100, 250, 500, 850, 1000, 2500, 4000, 6000, 8000, 10000, 12000]
    
    compilation_count = 0
    for rpm in rpm_gamut:
        for w in wheels:
            arr = arrays[w["array_name"]]
            total_edges = w["actual_len"]
            cycle_deg = w["degrees"]

            # Seamless 2x repetition for 360 deg wheels
            if cycle_deg == 360:
                rep_arr = arr + arr
                rep_edges = total_edges * 2
                rep_deg = 720
            else:
                rep_arr = arr
                rep_edges = total_edges
                rep_deg = cycle_deg

            for mask, ch_name in [(0x01, "CKP"), (0x02, "CMP1"), (0x04, "CMP2")]:
                if mask == 0x02 and not w["has_cam1"]:
                    continue
                if mask == 0x04 and not w["has_cam2"]:
                    continue

                items = compile_bit_array_to_rmt(rep_arr, rep_edges, rep_deg, rpm, mask)
                compilation_count += 1

                # 1. Output item validity
                record_check(f"C1_ItemsNonEmpty_{w['enum_name']}_{rpm}RPM_{ch_name}", len(items) >= 2, f"RMT items count {len(items)} < 2")
                record_check(f"C1_MaxItemsLimit_{w['enum_name']}_{rpm}RPM_{ch_name}", len(items) <= MAX_CYCLE_PULSES, f"Exceeded MAX_CYCLE_PULSES: {len(items)}")

                # 2. EOT Terminator check
                eot = items[-1]
                record_check(f"C1_EOT_{w['enum_name']}_{rpm}RPM_{ch_name}", eot == {"d0": 0, "l0": 0, "d1": 0, "l1": 0}, f"Invalid EOT: {eot}")

                # 3. Phase duration checks
                for item_idx, it in enumerate(items[:-1]):
                    record_check(f"C1_D0_Chunk_{w['enum_name']}_{rpm}_{ch_name}_{item_idx}", 0 < it["d0"] <= MAX_RMT_DURATION_CHUNK, f"d0 {it['d0']} out of range")
                    if it["d1"] > 0:
                        record_check(f"C1_D1_Chunk_{w['enum_name']}_{rpm}_{ch_name}_{item_idx}", it["d1"] <= MAX_RMT_DURATION_CHUNK, f"d1 {it['d1']} out of range")
                    record_check(f"C1_L0_Valid_{w['enum_name']}_{rpm}_{ch_name}_{item_idx}", it["l0"] in (0, 1), f"l0 {it['l0']} invalid")
                    record_check(f"C1_L1_Valid_{w['enum_name']}_{rpm}_{ch_name}_{item_idx}", it["l1"] in (0, 1), f"l1 {it['l1']} invalid")

    print(f"  -> Total channel compilations executed: {compilation_count}")
    print(f"  -> Challenge 1 Status: {'PASSED' if failed_checks == 0 else 'FAILED'}")

    # =========================================================================
    # CHALLENGE 2: Pulse Slicing Behavior & Conservation
    # =========================================================================
    print("\n--- [Challenge 2] Testing Low-RPM Pulse Slicing Mechanics (10 to 100 RPM) ---")
    slicing_tests = [10, 20, 30, 50, 100]
    total_sliced_runs = 0
    for rpm in slicing_tests:
        for w in wheels:
            arr = arrays[w["array_name"]]
            total_edges = w["actual_len"]
            cycle_deg = w["degrees"]
            cycle_total_us = (cycle_deg * 1000000) // (6 * rpm)

            for mask in [0x01, 0x02, 0x04]:
                if mask == 0x02 and not w["has_cam1"]:
                    continue
                if mask == 0x04 and not w["has_cam2"]:
                    continue

                # Find runs in original array
                runs = []
                curr_lvl = 1 if (arr[0] & mask) else 0
                run_start = 0
                for s in range(1, total_edges):
                    lvl = 1 if (arr[s] & mask) else 0
                    if lvl != curr_lvl:
                        t0 = (run_start * cycle_total_us) // total_edges
                        t1 = (s * cycle_total_us) // total_edges
                        runs.append((t1 - t0, curr_lvl))
                        curr_lvl = lvl
                        run_start = s
                t0 = (run_start * cycle_total_us) // total_edges
                t1 = cycle_total_us
                runs.append((t1 - t0, curr_lvl))

                # Compile RMT items
                items = compile_bit_array_to_rmt(arr, total_edges, cycle_deg, rpm, mask)
                # Flatten phases
                phases = []
                for it in items[:-1]:
                    phases.append((it["d0"], it["l0"]))
                    if it["d1"] > 0:
                        phases.append((it["d1"], it["l1"]))

                # Verify each run mapping into phases
                phase_idx = 0
                for run_dur, run_lvl in runs:
                    if run_dur > MAX_RMT_DURATION_CHUNK:
                        total_sliced_runs += 1
                        expected_chunks = math.ceil(run_dur / MAX_RMT_DURATION_CHUNK)
                        reconstructed_dur = 0
                        for c in range(expected_chunks):
                            chunk_d, chunk_l = phases[phase_idx]
                            record_check(f"C2_SliceLevel_{w['enum_name']}_{rpm}", chunk_l == run_lvl, f"Level mismatch during slice")
                            record_check(f"C2_SliceMax_{w['enum_name']}_{rpm}", chunk_d <= MAX_RMT_DURATION_CHUNK, f"Sliced chunk exceeds 30,000 us")
                            reconstructed_dur += chunk_d
                            phase_idx += 1
                        record_check(f"C2_ReconstructedDur_{w['enum_name']}_{rpm}", reconstructed_dur == run_dur, f"Reconstructed run duration mismatch: {reconstructed_dur} vs {run_dur}")
                    else:
                        chunk_d, chunk_l = phases[phase_idx]
                        record_check(f"C2_UncutLevel_{w['enum_name']}_{rpm}", chunk_l == run_lvl, f"Level mismatch on uncut pulse")
                        record_check(f"C2_UncutDur_{w['enum_name']}_{rpm}", chunk_d == run_dur, f"Duration mismatch on uncut pulse: {chunk_d} vs {run_dur}")
                        phase_idx += 1

                record_check(f"C2_PhasesExhausted_{w['enum_name']}_{rpm}", phase_idx == len(phases), f"Unconsumed phases remaining: {phase_idx} vs {len(phases)}")

    print(f"  -> Total sliced multi-chunk pulses verified: {total_sliced_runs}")
    print(f"  -> Challenge 2 Status: {'PASSED' if failed_checks == 0 else 'FAILED'}")

    # =========================================================================
    # CHALLENGE 3: Zero Cumulative Drift Verification
    # =========================================================================
    print("\n--- [Challenge 3] Testing Zero Cumulative Drift across all 70 Wheels & 7 RPMs ---")
    drift_rpms = [10, 50, 200, 850, 3000, 6000, 12000]
    for rpm in drift_rpms:
        for w in wheels:
            arr = arrays[w["array_name"]]
            total_edges = w["actual_len"]
            cycle_deg = w["degrees"]
            expected_cycle_us = (cycle_deg * 1000000) // (6 * rpm)

            for mask in [0x01, 0x02, 0x04]:
                if mask == 0x02 and not w["has_cam1"]:
                    continue
                if mask == 0x04 and not w["has_cam2"]:
                    continue

                items = compile_bit_array_to_rmt(arr, total_edges, cycle_deg, rpm, mask)
                total_dur = sum(it["d0"] + it["d1"] for it in items[:-1])
                drift = abs(total_dur - expected_cycle_us)
                record_check(f"C3_ZeroDrift_{w['enum_name']}_{rpm}RPM_{mask}", drift == 0, f"Drift detected: {drift} us (Got {total_dur}, Expected {expected_cycle_us})")

    print(f"  -> Challenge 3 Status: {'PASSED' if failed_checks == 0 else 'FAILED'}")

    # =========================================================================
    # CHALLENGE 4: Multi-Channel Phase Alignment (BMW N20 & GM LS1)
    # =========================================================================
    print("\n--- [Challenge 4] Testing Multi-Channel Phase Alignment for BMW N20 and GM LS1 ---")
    # 4.1 BMW N20
    bmw_w = wheels[66]
    bmw_arr = arrays[bmw_w["array_name"]]
    bmw_rpm = 3000
    bmw_cycle_us = (720 * 1000000) // (6 * bmw_rpm) # 40,000 us
    bmw_ckp_items = compile_bit_array_to_rmt(bmw_arr, 240, 720, bmw_rpm, 0x01)
    bmw_cmp1_items = compile_bit_array_to_rmt(bmw_arr, 240, 720, bmw_rpm, 0x02)
    bmw_cmp2_items = compile_bit_array_to_rmt(bmw_arr, 240, 720, bmw_rpm, 0x04)

    # Reconstruct microsecond state timeline for all 3 channels
    def build_timeline(items, total_us):
        tl = []
        for it in items[:-1]:
            if it["d0"] > 0:
                tl.extend([it["l0"]] * it["d0"])
            if it["d1"] > 0:
                tl.extend([it["l1"]] * it["d1"])
        assert len(tl) == total_us, f"Timeline len {len(tl)} != {total_us}"
        return tl

    bmw_ckp_tl = build_timeline(bmw_ckp_items, bmw_cycle_us)
    bmw_cmp1_tl = build_timeline(bmw_cmp1_items, bmw_cycle_us)
    bmw_cmp2_tl = build_timeline(bmw_cmp2_items, bmw_cycle_us)

    # Sample timeline at each segment midpoint
    seg_step_us = bmw_cycle_us / 240.0 # 166.6667 us
    for s in range(240):
        t_sample = int((s + 0.5) * seg_step_us)
        ckp_val = bmw_ckp_tl[t_sample]
        cmp1_val = bmw_cmp1_tl[t_sample]
        cmp2_val = bmw_cmp2_tl[t_sample]
        expected_byte = bmw_arr[s]
        expected_ckp = 1 if (expected_byte & 0x01) else 0
        expected_cmp1 = 1 if (expected_byte & 0x02) else 0
        expected_cmp2 = 1 if (expected_byte & 0x04) else 0
        record_check(f"C4_BMW_N20_Sync_Seg_{s}", (ckp_val == expected_ckp) and (cmp1_val == expected_cmp1) and (cmp2_val == expected_cmp2),
                     f"Phase mismatch at seg {s}: got ({ckp_val},{cmp1_val},{cmp2_val}) expected ({expected_ckp},{expected_cmp1},{expected_cmp2})")

    # 4.2 GM LS1
    ls1_w = wheels[27]
    ls1_arr = arrays[ls1_w["array_name"]]
    ls1_rpm = 2000
    ls1_cycle_us = (720 * 1000000) // (6 * ls1_rpm) # 60,000 us
    ls1_ckp_items = compile_bit_array_to_rmt(ls1_arr, 720, 720, ls1_rpm, 0x01)
    ls1_cmp1_items = compile_bit_array_to_rmt(ls1_arr, 720, 720, ls1_rpm, 0x02)
    ls1_cmp2_items = compile_bit_array_to_rmt(ls1_arr, 720, 720, ls1_rpm, 0x04)

    ls1_ckp_tl = build_timeline(ls1_ckp_items, ls1_cycle_us)
    ls1_cmp1_tl = build_timeline(ls1_cmp1_items, ls1_cycle_us)
    ls1_cmp2_tl = build_timeline(ls1_cmp2_items, ls1_cycle_us)

    seg_step_us_ls1 = ls1_cycle_us / 720.0
    for s in range(720):
        t_sample = int((s + 0.5) * seg_step_us_ls1)
        ckp_val = ls1_ckp_tl[t_sample]
        cmp1_val = ls1_cmp1_tl[t_sample]
        cmp2_val = ls1_cmp2_tl[t_sample]
        expected_byte = ls1_arr[s]
        expected_ckp = 1 if (expected_byte & 0x01) else 0
        expected_cmp1 = 1 if (expected_byte & 0x02) else 0
        expected_cmp2 = 1 if (expected_byte & 0x04) else 0
        record_check(f"C4_GM_LS1_Sync_Seg_{s}", (ckp_val == expected_ckp) and (cmp1_val == expected_cmp1) and (cmp2_val == expected_cmp2),
                     f"LS1 Phase mismatch at seg {s}: got ({ckp_val},{cmp1_val},{cmp2_val}) expected ({expected_ckp},{expected_cmp1},{expected_cmp2})")

    print(f"  -> Challenge 4 Status: {'PASSED' if failed_checks == 0 else 'FAILED'}")

    # =========================================================================
    # CHALLENGE 5: Waveform Canvas Geometry & Pixel Bounds Verification
    # =========================================================================
    print("\n--- [Challenge 5] Testing Waveform Canvas geometry and 0 out-of-bounds pixel writes across resolutions ---")
    test_resolutions = [
        (456, 124),  # Wheel Browser Canvas
        (448, 76),   # Dashboard Canvas
        (400, 100),  # PRD Target Alternate
        (440, 80),   # Compact Canvas
        (320, 60),   # Extreme Low-Height Stress
        (480, 240)   # High-Res Full Height
    ]

    total_canvas_renders = 0
    for width, height in test_resolutions:
        # Check track geometry math
        for num_tracks in [2, 3]:
            tracks = calculate_track_geometry(height, num_tracks)
            record_check(f"C5_TrackCount_{width}x{height}_{num_tracks}", len(tracks) == num_tracks, "Track count mismatch")
            for ti, trk in enumerate(tracks):
                record_check(f"C5_YTop_InBounds_{width}x{height}_{num_tracks}_{ti}", 0 <= trk["y_top"] < height, f"y_top {trk['y_top']} >= {height}")
                record_check(f"C5_YHigh_InBounds_{width}x{height}_{num_tracks}_{ti}", 0 <= trk["y_high"] < height, f"y_high {trk['y_high']} >= {height}")
                record_check(f"C5_YLow_InBounds_{width}x{height}_{num_tracks}_{ti}", 0 <= trk["y_low"] < height, f"y_low {trk['y_low']} >= {height}")
                record_check(f"C5_YHighAboveYLow_{width}x{height}_{num_tracks}_{ti}", trk["y_high"] < trk["y_low"], f"y_high {trk['y_high']} not above y_low {trk['y_low']}")
                if ti > 0:
                    prev_low = tracks[ti-1]["y_low"]
                    curr_top = trk["y_top"]
                    record_check(f"C5_TrackNoOverlap_{width}x{height}_{num_tracks}_{ti}", curr_top > prev_low, f"Track overlap: {curr_top} <= {prev_low}")

        # Render all 70 presets on VirtualTftCanvas and verify 0 OOB writes
        for w in wheels:
            arr = arrays[w["array_name"]]
            canvas = VirtualTftCanvas(width, height)
            render_canvas_simulation(canvas, arr, w["actual_len"], w["degrees"], w["has_cam1"], w["has_cam2"])
            total_canvas_renders += 1
            record_check(f"C5_ZeroOOB_{width}x{height}_{w['enum_name']}", len(canvas.oob_writes) == 0, f"OOB writes detected: {len(canvas.oob_writes)} instances (e.g. {canvas.oob_writes[:5]})")

    print(f"  -> Total canvas rendering simulations: {total_canvas_renders}")
    print(f"  -> Challenge 5 Status: {'PASSED' if failed_checks == 0 else 'FAILED'}")

    # =========================================================================
    # SUMMARY
    # =========================================================================
    print("\n" + "=" * 80)
    print("                 ADVERSARIAL STRESS TEST FINAL SUMMARY")
    print("=" * 80)
    print(f"Total Verification Checks : {total_checks}")
    print(f"Passed Checks             : {passed_checks}")
    print(f"Failed Checks             : {failed_checks}")
    print("=" * 80)

    if failed_checks == 0:
        print(">>> ALL ADVERSARIAL STRESS CHALLENGES PASSED WITH ZERO ERRORS (100% PASS) <<<")
        return 0
    else:
        print(f">>> CRITICAL: {failed_checks} ADVERSARIAL CHECKS FAILED <<<")
        return 1

if __name__ == "__main__":
    sys.exit(main())
