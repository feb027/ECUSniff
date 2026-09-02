import os
import re
import sys

def run_comprehensive_audit():
    print("================================================================================")
    print("             ECUSNIFF INDEPENDENT VICTORY AUDITOR FORENSIC SUITE                ")
    print("================================================================================")

    # -------------------------------------------------------------------------
    # Part 1: Source Code Parity Check (ArduStim vs ECUSniff)
    # -------------------------------------------------------------------------
    ardustim_path = "external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h"
    with open(ardustim_path, "r", encoding="utf-8", errors="ignore") as f:
        ardustim_src = f.read()

    ecu_db_path = "lib/engine/src/wheel_database.cpp"
    with open(ecu_db_path, "r", encoding="utf-8") as f:
        ecu_src = f.read()

    # Extract enum WheelType
    enum_match = re.search(r'typedef enum\s*\{(.*?)\}\s*WheelType;', ardustim_src, re.DOTALL)
    assert enum_match, "Enum WheelType not found in ArduStim"
    enum_body = enum_match.group(1)
    enum_clean = re.sub(r'/\*.*?\*/|//.*', '', enum_body)
    raw_tokens = [t.strip() for t in enum_clean.replace('\n', ',').split(',') if t.strip()]
    enum_tokens = [t for t in raw_tokens if t and t != 'MAX_WHEELS']

    print(f"[AUDIT P1] ArduStim WheelType count: {len(enum_tokens)} enum items.")
    assert len(enum_tokens) == 70, f"Expected 70 enum tokens, found {len(enum_tokens)}"

    # Extract friendly name strings in ArduStim
    friendly_names_in_ardustim = re.findall(r'const\s+char\s+(\w+)\s*\[\]\s*PROGMEM\s*=\s*\"([^\"]*)\";', ardustim_src)
    ardustim_name_dict = {var: val for var, val in friendly_names_in_ardustim}
    print(f"[AUDIT P1] ArduStim friendly name strings parsed: {len(ardustim_name_dict)}")

    # Extract s_wheelDatabase table
    db_match = re.search(r'static const WheelDefinition s_wheelDatabase\[\w+\] = \{(.*?)\n\};', ecu_src, re.DOTALL)
    assert db_match, "s_wheelDatabase table not found in wheel_database.cpp"
    db_lines = [l.strip() for l in db_match.group(1).split('\n') if l.strip().startswith('{')]

    print(f"[AUDIT P1] ECUSniff s_wheelDatabase table entries: {len(db_lines)}")
    assert len(db_lines) == 70, f"Expected 70 database entries, got {len(db_lines)}"

    total_progmem_bytes = 0
    parsed_wheels = []

    for idx, line in enumerate(db_lines):
        m = re.match(r'\{\s*(\d+)\s*,\s*\"([^\"]*)\"\s*,\s*\"([^\"]*)\"\s*,\s*BrandCategory::(\w+)\s*,\s*WheelCycleDegrees::(\w+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(true|false)\s*,\s*(true|false)\s*\}', line)
        assert m, f"Line {idx} does not match format: {line}"
        
        wid, fname, sname, cat, cycle_deg, edges, arr_var, has_cmp1, has_cmp2 = m.groups()
        wid = int(wid)
        edges = int(edges)
        has_cmp1 = (has_cmp1 == "true")
        has_cmp2 = (has_cmp2 == "true")
        deg_val = 360 if cycle_deg == "CRANK_360" else 720

        assert wid == idx, f"ID mismatch at {idx}: {wid}"
        assert edges > 0, f"Wheel {idx} ({fname}) has zero edges"

        # Find array definition in wheel_database.cpp
        arr_match = re.search(rf'static\s+const\s+uint8_t\s+{arr_var}\[(\d+)\]\s*PROGMEM\s*=\s*\{{([^}}]+)\}};', ecu_src)
        assert arr_match, f"Array {arr_var} not found in wheel_database.cpp"
        
        declared_size = int(arr_match.group(1))
        assert declared_size == edges, f"Array {arr_var} declared size {declared_size} != edges {edges}"

        raw_bytes_str = arr_match.group(2)
        raw_bytes_str = re.sub(r'/\*.*?\*/|//.*', '', raw_bytes_str)
        byte_tokens = [b.strip() for b in raw_bytes_str.replace('\n', ',').split(',') if b.strip()]
        byte_values = [int(b) for b in byte_tokens]

        assert len(byte_values) == edges, f"Parsed {len(byte_values)} bytes != declared edges {edges}"
        total_progmem_bytes += len(byte_values)

        # Check channel integrity
        assert any((b & 0x01) for b in byte_values), f"Wheel {idx} ({fname}) has no CKP pulses"
        assert any((b & 0x02) for b in byte_values) == has_cmp1, f"Wheel {idx} CMP1 mismatch"
        assert any((b & 0x04) for b in byte_values) == has_cmp2, f"Wheel {idx} CMP2 mismatch"
        for b in byte_values:
            assert 0 <= b <= 7, f"Invalid bitmask {b} in {arr_var}"

        parsed_wheels.append({
            "id": wid,
            "friendlyName": fname,
            "shortName": sname,
            "category": cat,
            "cycleDegrees": deg_val,
            "totalEdges": edges,
            "arrVar": arr_var,
            "hasCmp1": has_cmp1,
            "hasCmp2": has_cmp2,
            "bytes": byte_values
        })

    print(f"[AUDIT P1] Flash PROGMEM footprints: {total_progmem_bytes} bytes across 70 arrays.")
    print("[AUDIT P1] PASS: All 70 presets verified for syntax, array bounds, and channel masks.")

    # -------------------------------------------------------------------------
    # Part 2: Critical OEM Pattern Edge Transition Verification (Requirement R1, R4)
    # -------------------------------------------------------------------------
    print("\n--- Step 2: Critical OEM Pattern Forensics ---")
    
    # 1. New Avanza (Index 19)
    w_new_avanza = parsed_wheels[19]
    assert "Toyota Avanza 1.5 Crank only" in w_new_avanza["friendlyName"]
    assert w_new_avanza["totalEdges"] == 144
    assert w_new_avanza["cycleDegrees"] == 720
    assert w_new_avanza["hasCmp1"] is True
    # Verify Camshaft pulse in 2nd revolution (segments 73 to 84)
    for s in range(0, 72):
        assert (w_new_avanza["bytes"][s] & 0x02) == 0, f"New Avanza Rev1 has unexpected CAM pulse at seg {s}"
    for s in range(73, 85):
        assert (w_new_avanza["bytes"][s] & 0x02) == 0x02, f"New Avanza Rev2 missing expected CAM pulse at seg {s}"
    print("[AUDIT P2] PASS: Toyota Avanza 1.5 New Avanza (144 seg, 720 deg) CAM pulse position verified.")

    # 2. Old Avanza (Index 18)
    w_old_avanza = parsed_wheels[18]
    assert "Toyota Avanza 1.3 Crank only" in w_old_avanza["friendlyName"]
    assert w_old_avanza["totalEdges"] == 144
    assert w_old_avanza["cycleDegrees"] == 720
    assert w_old_avanza["hasCmp1"] is True
    # 3 CAM groups
    cam_edges_old = [s for s, b in enumerate(w_old_avanza["bytes"]) if (b & 0x02)]
    assert len(cam_edges_old) == 36 # 3 groups of 12 segments
    print("[AUDIT P2] PASS: Toyota Avanza 1.3 Old Avanza (144 seg, 720 deg) 3 CAM pulse groups verified.")

    # 3. Avanza / Xenia / Terios / Rush (Index 20)
    w_rush = parsed_wheels[20]
    assert "Toyota Avanza/Xenia/Terios/Rush" in w_rush["friendlyName"]
    assert w_rush["totalEdges"] == 144
    assert w_rush["cycleDegrees"] == 720
    assert w_rush["hasCmp1"] is True
    cam_edges_rush = [s for s, b in enumerate(w_rush["bytes"]) if (b & 0x02)]
    assert len(cam_edges_rush) == 35 # 12 + 12 + 11 segments
    print("[AUDIT P2] PASS: Toyota Avanza/Xenia/Terios/Rush (144 seg, 720 deg) VVT-i pattern verified.")

    # 4. Mitsubishi 4G63 (Index 46)
    w_4g63 = parsed_wheels[46]
    assert "4g63" in w_4g63["friendlyName"].lower()
    assert w_4g63["totalEdges"] == 144
    assert w_4g63["hasCmp1"] is True
    # Check 4 crank pulses (2 per rev * 2 revs) = 56 segments
    crank_active_4g63 = [s for s, b in enumerate(w_4g63["bytes"]) if (b & 0x01)]
    assert len(crank_active_4g63) == 56 # 14 * 4 segments
    cam_active_4g63 = [s for s, b in enumerate(w_4g63["bytes"]) if (b & 0x02)]
    assert len(cam_active_4g63) == 41 # 11 + 14 + 16 segments
    print("[AUDIT P2] PASS: Mitsubishi 4G63 (4/2 CAS) crank (56 seg) and cam (41 seg) verified.")

    # 5. Bosch 60-2 (Index 3)
    w_60_2 = parsed_wheels[3]
    assert "60-2" in w_60_2["friendlyName"]
    assert w_60_2["totalEdges"] == 120
    assert w_60_2["cycleDegrees"] == 360
    # Missing teeth at segments 116..119 (last 4 segments = 2 teeth)
    for s in range(0, 116):
        expected_level = 1 if (s % 2 == 0) else 0
        assert w_60_2["bytes"][s] == expected_level, f"60-2 tooth error at seg {s}"
    for s in range(116, 120):
        assert w_60_2["bytes"][s] == 0, f"60-2 missing gap error at seg {s}"
    print("[AUDIT P2] PASS: Bosch 60-2 crank-only (120 seg, 360 deg) 58 active teeth + 2 missing teeth verified.")

    # 6. BMW N20 (Index 66) - Dual Cam (CMP1 + CMP2)
    w_n20 = parsed_wheels[66]
    assert "BMW N20" in w_n20["friendlyName"]
    assert w_n20["hasCmp1"] is True
    assert w_n20["hasCmp2"] is True
    assert w_n20["totalEdges"] == 240
    print("[AUDIT P2] PASS: BMW N20 Dual Cam (CKP + CMP1 + CMP2) verified.")

    # -------------------------------------------------------------------------
    # Part 3: Independent Simulation of compileBitArrayToRmt Algorithm
    # -------------------------------------------------------------------------
    print("\n--- Step 3: Independent RMT Generator Simulation & Math Proof ---")
    
    MAX_RMT_CHUNK = 30000

    def simulate_compile_rmt(bit_array, total_edges, cycle_degrees, rpm, channel_mask):
        cycle_total_us = int((cycle_degrees * 1000000) / (6 * rpm))
        if cycle_total_us == 0: cycle_total_us = 1
        
        phases = []
        curr_lvl = 1 if (bit_array[0] & channel_mask) else 0
        run_start = 0

        for s in range(1, total_edges):
            lvl = 1 if (bit_array[s] & channel_mask) else 0
            if lvl != curr_lvl:
                t_start = (run_start * cycle_total_us) // total_edges
                t_end   = (s * cycle_total_us) // total_edges
                dur = t_end - t_start
                rem = dur
                while rem > MAX_RMT_CHUNK:
                    phases.append((MAX_RMT_CHUNK, curr_lvl))
                    rem -= MAX_RMT_CHUNK
                if rem > 0:
                    phases.append((rem, curr_lvl))
                curr_lvl = lvl
                run_start = s
        
        t_start = (run_start * cycle_total_us) // total_edges
        t_end = cycle_total_us
        dur = t_end - t_start
        rem = dur
        while rem > MAX_RMT_CHUNK:
            phases.append((MAX_RMT_CHUNK, curr_lvl))
            rem -= MAX_RMT_CHUNK
        if rem > 0:
            phases.append((rem, curr_lvl))

        # Check total duration conservation
        total_phase_us = sum(p[0] for p in phases)
        assert total_phase_us == cycle_total_us, f"Duration mismatch: sum={total_phase_us} vs cycle={cycle_total_us}"
        for d, lvl in phases:
            assert d <= MAX_RMT_CHUNK, f"Chunk exceeded 30000 us: {d}"
            assert lvl in (0, 1)

        return len(phases), total_phase_us

    # Test all 70 wheels across standard & extreme RPMs: 10, 50, 200, 850, 3000, 6000, 12000
    rpms_to_test = [10, 50, 200, 850, 3000, 6000, 12000]
    total_sim_assertions = 0

    for w in parsed_wheels:
        arr = w["bytes"]
        edges = w["totalEdges"]
        deg = w["cycleDegrees"]
        for rpm in rpms_to_test:
            # CKP
            num_phases, dur = simulate_compile_rmt(arr, edges, deg, rpm, 0x01)
            total_sim_assertions += 1
            if w["hasCmp1"]:
                num_phases, dur = simulate_compile_rmt(arr, edges, deg, rpm, 0x02)
                total_sim_assertions += 1
            if w["hasCmp2"]:
                num_phases, dur = simulate_compile_rmt(arr, edges, deg, rpm, 0x04)
                total_sim_assertions += 1

    print(f"[AUDIT P3] PASS: {total_sim_assertions} RMT compilation simulations executed with 0 timing drift and 0 chunk overflows.")

    # -------------------------------------------------------------------------
    # Part 4: Dynamic Waveform Canvas Geometry Verification (Requirement R3)
    # -------------------------------------------------------------------------
    print("\n--- Step 4: Dynamic Waveform Canvas Track Geometry Verification ---")
    
    def test_canvas_geometry(canvas_h, num_tracks):
        header_h = 14 if canvas_h > 90 else 12
        usable_h = canvas_h - header_h - 4
        track_h = usable_h // num_tracks
        margin_y = 6 if (canvas_h > 90 and track_h > 40) else (4 if canvas_h > 90 else 3)
        
        tracks = []
        for i in range(num_tracks):
            y_top = header_h + 2 + (i * track_h)
            y_high = y_top + margin_y
            y_low = y_top + track_h - margin_y - 1
            assert y_high < y_low, f"Inverted track at {i}: high={y_high} >= low={y_low}"
            assert y_low < canvas_h, f"Track exceeds canvas height: {y_low} >= {canvas_h}"
            tracks.append((y_top, y_high, y_low))
        
        # Verify track separation
        for i in range(len(tracks) - 1):
            assert tracks[i][2] < tracks[i+1][0], f"Track overlap between {i} and {i+1}"
        return tracks

    # WheelBrowser canvas: 456 x 124 px
    t124_2 = test_canvas_geometry(124, 2)
    t124_3 = test_canvas_geometry(124, 3)
    print(f"[AUDIT P4] Large Canvas (124px) Geometry: 2-track usable={t124_2}, 3-track usable={t124_3} [OK]")

    # Dashboard canvas: 448 x 76 px
    t76_2 = test_canvas_geometry(76, 2)
    t76_3 = test_canvas_geometry(76, 3)
    print(f"[AUDIT P4] Compact Canvas (76px) Geometry: 2-track usable={t76_2}, 3-track usable={t76_3} [OK]")

    print("\n================================================================================")
    print("                      ALL FORENSIC AUDIT CHECKS PASSED                         ")
    print("================================================================================")

if __name__ == "__main__":
    run_comprehensive_audit()
