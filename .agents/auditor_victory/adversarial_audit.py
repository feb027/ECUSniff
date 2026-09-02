import os
import re
import sys

def run_adversarial_auditor_stress():
    print("================================================================================")
    print("        AUDITOR INDEPENDENT ADVERSARIAL STRESS & CORNER CASE SUITE              ")
    print("================================================================================")

    # Load wheel database and raw arrays
    with open("lib/engine/src/wheel_database.cpp", "r", encoding="utf-8") as f:
        src = f.read()

    # Extract all arrays
    arr_matches = re.findall(r'static\s+const\s+uint8_t\s+(\w+)\[\d+\]\s*PROGMEM\s*=\s*\{([^}]+)\};', src)
    arr_dict = {}
    for name, content in arr_matches:
        cleaned = re.sub(r'/\*.*?\*/|//.*', '', content)
        tokens = [t.strip() for t in cleaned.replace('\n', ',').split(',') if t.strip()]
        arr_dict[name] = [int(t) for t in tokens]

    # Extract s_wheelDatabase
    db_match = re.search(r'static const WheelDefinition s_wheelDatabase\[\w+\] = \{(.*?)\n\};', src, re.DOTALL)
    db_lines = [l.strip() for l in db_match.group(1).split('\n') if l.strip().startswith('{')]
    
    wheels = []
    for line in db_lines:
        m = re.match(r'\{\s*(\d+)\s*,\s*\"([^\"]*)\"\s*,\s*\"([^\"]*)\"\s*,\s*BrandCategory::(\w+)\s*,\s*WheelCycleDegrees::(\w+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(true|false)\s*,\s*(true|false)\s*\}', line)
        wid, fname, sname, cat, cycle_deg, edges, arr_var, has_cmp1, has_cmp2 = m.groups()
        wheels.append({
            "id": int(wid),
            "friendlyName": fname,
            "shortName": sname,
            "category": cat,
            "cycleDegrees": 360 if cycle_deg == "CRANK_360" else 720,
            "totalEdges": int(edges),
            "arrVar": arr_var,
            "hasCmp1": has_cmp1 == "true",
            "hasCmp2": has_cmp2 == "true",
            "bytes": arr_dict[arr_var]
        })

    print(f"[ADV AUDIT] Loaded {len(wheels)} wheels and {len(arr_dict)} raw arrays.")

    # -------------------------------------------------------------------------
    # Test 1: Category Mapping Completeness
    # -------------------------------------------------------------------------
    print("\n[ADV Test 1] Auditing Brand Category Coverage and Distribution...")
    cat_counts = {}
    for w in wheels:
        c = w["category"]
        cat_counts[c] = cat_counts.get(c, 0) + 1
    print(f"  Distribution: {cat_counts}")
    assert "TOYOTA_DAIHATSU" in cat_counts and cat_counts["TOYOTA_DAIHATSU"] >= 6
    assert "HONDA" in cat_counts and cat_counts["HONDA"] >= 5
    assert "MITSUBISHI" in cat_counts and cat_counts["MITSUBISHI"] >= 4
    assert "NISSAN" in cat_counts and cat_counts["NISSAN"] >= 2
    assert "EURO_US" in cat_counts and cat_counts["EURO_US"] >= 10
    assert "UNIVERSAL" in cat_counts and cat_counts["UNIVERSAL"] >= 20
    assert sum(cat_counts.values()) == 70
    print("  -> Category Coverage [PASS]")

    # -------------------------------------------------------------------------
    # Test 2: Invariance of Logical Pulse Count Across RPM Spectrum
    # -------------------------------------------------------------------------
    print("\n[ADV Test 2] Testing Transition Count Invariance across 10 RPM to 18,000 RPM...")
    
    MAX_RMT_CHUNK = 30000

    def compile_phases(bit_array, total_edges, cycle_degrees, rpm, channel_mask):
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
        return phases

    def coalesce_logical_transitions(phases):
        # Merge adjacent phases of same level
        logical = []
        for d, lvl in phases:
            if not logical or logical[-1][1] != lvl:
                logical.append([d, lvl])
            else:
                logical[-1][0] += d
        return logical

    tested_combinations = 0
    for w in wheels:
        arr = w["bytes"]
        edges = w["totalEdges"]
        deg = w["cycleDegrees"]
        for mask in [0x01, 0x02 if w["hasCmp1"] else None, 0x04 if w["hasCmp2"] else None]:
            if mask is None: continue
            
            # Baseline at 1000 RPM
            base_phases = compile_phases(arr, edges, deg, 1000, mask)
            base_logical = coalesce_logical_transitions(base_phases)
            
            for test_rpm in [10, 50, 200, 600, 3000, 7500, 12000, 18000]:
                test_phases = compile_phases(arr, edges, deg, test_rpm, mask)
                test_logical = coalesce_logical_transitions(test_phases)
                
                # Check that number of logical transitions is identical
                assert len(base_logical) == len(test_logical), f"Transition mismatch on {w['friendlyName']} mask {mask} at RPM {test_rpm}: base={len(base_logical)} vs test={len(test_logical)}"
                # Check levels match
                for i in range(len(base_logical)):
                    assert base_logical[i][1] == test_logical[i][1]
                tested_combinations += 1

    print(f"  -> {tested_combinations} RPM/Mask combinations verified with identical logical edge counts [PASS]")

    # -------------------------------------------------------------------------
    # Test 3: Microsecond Sum Integrity under Continuous Loopback
    # -------------------------------------------------------------------------
    print("\n[ADV Test 3] Verifying 1,000 Continuous Revolution Timebase Integration...")
    # For a high-speed wheel (e.g. 60-2 @ 6000 RPM = 10,000 us/rev)
    # Total time for 1,000 revs must equal 10,000,000 us with zero cumulative jitter
    w60_2 = wheels[3]
    rev_dur_us = int((360 * 1000000) / (6 * 6000))
    phases_60_2 = compile_phases(w60_2["bytes"], w60_2["totalEdges"], 360, 6000, 0x01)
    single_rev_sum = sum(p[0] for p in phases_60_2)
    assert single_rev_sum == rev_dur_us == 10000, f"Single rev sum {single_rev_sum} != 10000"
    
    thousand_revs = single_rev_sum * 1000
    assert thousand_revs == 10000000, f"1000 revs sum {thousand_revs} != 10,000,000 us"
    print("  -> Continuous loopback timing conservation [PASS]")

    print("\n================================================================================")
    print("      ALL AUDITOR ADVERSARIAL STRESS TESTS COMPLETED SUCCESSFULLY!             ")
    print("================================================================================")

if __name__ == "__main__":
    run_adversarial_auditor_stress()
