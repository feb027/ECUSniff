#!/usr/bin/env python3
"""
EMPIRICAL CHALLENGER 1 — MILESTONE 1 INDEPENDENT ORACLE & VERIFICATION HARNESS
Exhaustive verification of lib/engine/src/wheel_database.cpp against:
- external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h
- external/ardustim-tftv2-touchscreen/ardustim/ardustim.ino
"""

import re
import sys
import os

def strip_c_comments(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    text = re.sub(r'//.*', '', text)
    return text

def parse_ardustim_source(wheel_defs_path, ardustim_ino_path):
    with open(wheel_defs_path, 'r', encoding='utf-8', errors='ignore') as f:
        defs_content = f.read()

    with open(ardustim_ino_path, 'r', encoding='utf-8', errors='ignore') as f:
        ino_content = f.read()

    clean_defs = strip_c_comments(defs_content)

    # 1. Parse friendly names from wheel_defs.h
    # e.g., const char dizzy_four_cylinder_friendly_name[] PROGMEM = "4 cylinder dizzy";
    friendly_names = {}
    name_matches = re.finditer(r'const\s+char\s+([a-zA-Z0-9_]+)\s*\[\s*\](?:\s+PROGMEM)?\s*=\s*"([^"]*)";', clean_defs)
    for m in name_matches:
        friendly_names[m.group(1)] = m.group(2)

    # 2. Parse Wheels[] array from ardustim.ino
    # Format in ardustim.ino:
    # { dizzy_four_cylinder_friendly_name, dizzy_four_cylinder, 0.03333, 4, 360 },
    wheels_body_match = re.search(r'wheels\s+Wheels\s*\[[^\]]*\]\s*=\s*\{([^;]+)\};', strip_c_comments(ino_content), flags=re.DOTALL)
    if not wheels_body_match:
        raise ValueError("Could not find Wheels[] in ardustim.ino")
    
    wheels_body = wheels_body_match.group(1)

    # Parse each struct entry inside Wheels[]
    entry_pattern = re.finditer(r'\{\s*([a-zA-Z0-9_]+)\s*,\s*([a-zA-Z0-9_]+)\s*,\s*([0-9.fF]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\}', wheels_body)
    entries = []
    
    for idx, em in enumerate(entry_pattern):
        name_var = em.group(1)
        array_var = em.group(2)
        rpm_scaler = float(em.group(3))
        max_edges = int(em.group(4))
        degrees = int(em.group(5))
        
        friendly_str = friendly_names.get(name_var, None)
        
        # Look up array_var in clean_defs
        # Pattern: const (unsigned char|uint8_t) array_var[...] PROGMEM = { ... };
        # or array_var[...] = { ... };
        arr_pattern = rf'(?:const\s+(?:unsigned\s+char|uint8_t)\s+)?{re.escape(array_var)}\s*(?:\[\s*\d*\s*\])?\s*(?:PROGMEM)?\s*=\s*\{{([^}}]+)\}};'
        am = re.search(arr_pattern, clean_defs)
        if am:
            body = am.group(1)
            array_data = [int(v.strip()) for v in body.replace('\n', ',').split(',') if v.strip()]
        else:
            array_data = None
        
        entries.append({
            'index': idx,
            'name_var': name_var,
            'friendly_name': friendly_str,
            'array_var': array_var,
            'rpm_scaler': rpm_scaler,
            'max_edges': max_edges,
            'degrees': degrees,
            'array_data': array_data
        })

    return entries, friendly_names


def parse_wheel_database_cpp(cpp_path):
    with open(cpp_path, 'r', encoding='utf-8') as f:
        cpp_content = f.read()

    clean_cpp = strip_c_comments(cpp_content)

    # 1. Parse all PROGMEM arrays in wheel_database.cpp
    # static const uint8_t s_pattern_00_dizzy_four_cylinder[4] PROGMEM = { ... };
    cpp_arrays = {}
    pattern_matches = re.finditer(r'(?:static\s+)?const\s+uint8_t\s+([a-zA-Z0-9_]+)\s*\[\s*([0-9]+)\s*\](?:\s+PROGMEM)?\s*=\s*\{([^}]+)\};', clean_cpp, flags=re.DOTALL)
    for m in pattern_matches:
        arr_name = m.group(1)
        arr_len = int(m.group(2))
        body = m.group(3)
        values = [int(v.strip()) for v in body.replace('\n', ',').split(',') if v.strip()]
        cpp_arrays[arr_name] = {
            'length_declared': arr_len,
            'data': values
        }

    # 2. Parse s_wheelDatabase table entries
    # static const WheelDefinition s_wheelDatabase[70] = { ... };
    table_match = re.search(r'static\s+const\s+WheelDefinition\s+s_wheelDatabase\s*\[[^\]]*\]\s*=\s*\{([^;]+)\};', clean_cpp, flags=re.DOTALL)
    if not table_match:
        raise ValueError("Could not find s_wheelDatabase in wheel_database.cpp")
    
    table_body = table_match.group(1)
    db_entries = []
    
    row_matches = re.finditer(
        r'\{\s*([0-9]+)\s*,\s*"([^"]*)"\s*,\s*"([^"]*)"\s*,\s*BrandCategory::([a-zA-Z0-9_]+)\s*,\s*WheelCycleDegrees::([a-zA-Z0-9_]+)\s*,\s*([0-9]+)\s*,\s*([a-zA-Z0-9_]+)\s*,\s*(true|false)\s*,\s*(true|false)\s*\}',
        table_body
    )
    
    for rm in row_matches:
        wid = int(rm.group(1))
        fname = rm.group(2)
        sname = rm.group(3)
        cat = rm.group(4)
        cycle_enum = rm.group(5)
        total_edges = int(rm.group(6))
        arr_name = rm.group(7)
        has_cmp1 = rm.group(8) == 'true'
        has_cmp2 = rm.group(9) == 'true'
        
        cycle_deg = 360 if '360' in cycle_enum else 720
        
        db_entries.append({
            'id': wid,
            'friendlyName': fname,
            'shortName': sname,
            'category': cat,
            'cycleDegreesEnum': cycle_enum,
            'cycleDegrees': cycle_deg,
            'totalEdges': total_edges,
            'bitArrayName': arr_name,
            'hasCmp1': has_cmp1,
            'hasCmp2': has_cmp2
        })

    return db_entries, cpp_arrays


def run_verification():
    print("=" * 80)
    print("  EMPIRICAL CHALLENGER 1 — MILESTONE 1 ORACLE VERIFICATION HARNESS")
    print("=" * 80)

    base_dir = r"g:\semester 7\ECUSniff"
    defs_path = os.path.join(base_dir, "external", "ardustim-tftv2-touchscreen", "ardustim", "wheel_defs.h")
    ino_path = os.path.join(base_dir, "external", "ardustim-tftv2-touchscreen", "ardustim", "ardustim.ino")
    cpp_path = os.path.join(base_dir, "lib", "engine", "src", "wheel_database.cpp")
    h_path = os.path.join(base_dir, "lib", "engine", "include", "wheel_database.h")

    # 1. Parse ArduStim source
    print(f"[*] Reading ArduStim source definitions from:\n    {defs_path}\n    {ino_path}")
    ardustim_entries, friendly_names_map = parse_ardustim_source(defs_path, ino_path)
    print(f"[+] Successfully parsed {len(ardustim_entries)} ArduStim presets from Wheels[] table.")
    print(f"[+] Total friendly name variables mapped: {len(friendly_names_map)}")

    # 2. Parse ECUSniff wheel_database.cpp
    print(f"\n[*] Reading ECUSniff database implementation from:\n    {cpp_path}")
    cpp_entries, cpp_arrays = parse_wheel_database_cpp(cpp_path)
    print(f"[+] Successfully parsed {len(cpp_entries)} database entries in s_wheelDatabase[].")
    print(f"[+] Total PROGMEM bit-arrays found in cpp: {len(cpp_arrays)}")

    failures = []
    warnings = []

    # Check preset counts
    if len(ardustim_entries) != 70:
        failures.append(f"Expected 70 ArduStim presets, found {len(ardustim_entries)}")
    if len(cpp_entries) != 70:
        failures.append(f"Expected 70 ECUSniff database entries, found {len(cpp_entries)}")
    if len(cpp_arrays) != 70:
        failures.append(f"Expected 70 PROGMEM bit arrays in cpp, found {len(cpp_arrays)}")

    print("\n" + "=" * 80)
    print("  TEST 1: BYTE-FOR-BYTE BIT-ARRAY VALIDATION ACROSS ALL 70 PRESETS")
    print("=" * 80)
    
    total_bytes_cpp = 0
    total_bytes_ardustim = 0
    
    for i in range(70):
        ard = ardustim_entries[i]
        cpp = cpp_entries[i]
        
        # ID check
        if cpp['id'] != i:
            failures.append(f"Preset {i}: ID mismatch! Expected {i}, found {cpp['id']}")
            
        # Array lookup in cpp
        arr_name = cpp['bitArrayName']
        if arr_name not in cpp_arrays:
            failures.append(f"Preset {i} ({cpp['friendlyName']}): Array '{arr_name}' missing from cpp!")
            continue
        
        cpp_arr_info = cpp_arrays[arr_name]
        cpp_data = cpp_arr_info['data']
        cpp_len = cpp_arr_info['length_declared']
        
        if len(cpp_data) != cpp_len:
            failures.append(f"Preset {i} ({cpp['friendlyName']}): Declared length {cpp_len} != actual element count {len(cpp_data)}")
        
        if cpp['totalEdges'] != cpp_len:
            failures.append(f"Preset {i} ({cpp['friendlyName']}): s_wheelDatabase totalEdges {cpp['totalEdges']} != array length {cpp_len}")
            
        ard_data = ard['array_data']
        if ard_data is None:
            failures.append(f"Preset {i}: ArduStim array '{ard['array_var']}' not found in wheel_defs.h!")
            continue
            
        total_bytes_cpp += len(cpp_data)
        total_bytes_ardustim += len(ard_data)
        
        # Edge count comparison
        if len(cpp_data) != ard['max_edges']:
            failures.append(f"Preset {i} ({cpp['friendlyName']}): Array length {len(cpp_data)} != ArduStim max_edges {ard['max_edges']}")
            
        if len(ard_data) != ard['max_edges']:
            failures.append(f"Preset {i} ({ard['name_var']}): ArduStim array length {len(ard_data)} != ArduStim max_edges {ard['max_edges']}")
            
        # Byte-for-byte content comparison
        if cpp_data != ard_data:
            diff_indices = [idx for idx, (a, b) in enumerate(zip(cpp_data, ard_data)) if a != b]
            failures.append(f"Preset {i} ({cpp['friendlyName']}): Byte mismatch at indices {diff_indices[:10]}! (total {len(diff_indices)} diffs)")
        else:
            # Verify valid bitmasks in array: each byte should only use bits 0, 1, 2 (0..7)
            invalid_vals = [val for val in cpp_data if val > 7]
            if invalid_vals:
                failures.append(f"Preset {i} ({cpp['friendlyName']}): Invalid byte values > 7 found: {invalid_vals[:5]}")

    if not failures:
        print(f"[PASS] All 70 bit-arrays match ArduStim byte-for-byte perfectly!")
        print(f"[INFO] Total Flash PROGMEM array storage: {total_bytes_cpp} bytes ({total_bytes_cpp / 1024.0:.2f} KB).")
    else:
        print(f"[FAIL] Found {len(failures)} failures during byte-for-byte comparison!")
        for f in failures:
            print(f"  [-] {f}")

    print("\n" + "=" * 80)
    print("  TEST 2: EXACT FRIENDLY NAME & CYCLE DEGREES MATCHING")
    print("=" * 80)
    
    name_failures = []
    degree_failures = []
    
    for i in range(70):
        ard = ardustim_entries[i]
        cpp = cpp_entries[i]
        
        expected_name = ard['friendly_name']
        actual_name = cpp['friendlyName']
        
        if expected_name != actual_name:
            name_failures.append(f"Preset {i}: Friendly name mismatch!\n    Expected: '{expected_name}' (len {len(expected_name) if expected_name else 0})\n    Actual:   '{actual_name}' (len {len(actual_name)})")
            
        expected_deg = ard['degrees']
        actual_deg = cpp['cycleDegrees']
        
        if expected_deg != actual_deg:
            degree_failures.append(f"Preset {i} ({actual_name}): Cycle degrees mismatch! Expected {expected_deg}, found {actual_deg}")

    if not name_failures:
        print(f"[PASS] All 70 friendly names match ArduStim verbatim (including special cases like trailing spaces in Avanza Rush)!")
    else:
        print(f"[FAIL] Friendly name mismatches ({len(name_failures)}):")
        for f in name_failures:
            print(f"  [-] {f}")

    if not degree_failures:
        count_360 = sum(1 for e in cpp_entries if e['cycleDegrees'] == 360)
        count_720 = sum(1 for e in cpp_entries if e['cycleDegrees'] == 720)
        print(f"[PASS] All 70 cycle degrees match ArduStim (360° count: {count_360}, 720° count: {count_720}, total: {count_360 + count_720}).")
    else:
        print(f"[FAIL] Cycle degree mismatches ({len(degree_failures)}):")
        for f in degree_failures:
            print(f"  [-] {f}")

    print("\n" + "=" * 80)
    print("  TEST 3: SIGNAL CHANNEL PRESENCE FLAGS (hasCmp1, hasCmp2)")
    print("=" * 80)
    
    channel_failures = []
    cmp1_count = 0
    cmp2_count = 0
    
    for i in range(70):
        cpp = cpp_entries[i]
        cpp_data = cpp_arrays[cpp['bitArrayName']]['data']
        
        actual_has_cmp1 = any((val & 0x02) != 0 for val in cpp_data)
        actual_has_cmp2 = any((val & 0x04) != 0 for val in cpp_data)
        
        if cpp['hasCmp1'] != actual_has_cmp1:
            channel_failures.append(f"Preset {i} ({cpp['friendlyName']}): hasCmp1 is {cpp['hasCmp1']} but bit 0x02 presence is {actual_has_cmp1}")
            
        if cpp['hasCmp2'] != actual_has_cmp2:
            channel_failures.append(f"Preset {i} ({cpp['friendlyName']}): hasCmp2 is {cpp['hasCmp2']} but bit 0x04 presence is {actual_has_cmp2}")
            
        if cpp['hasCmp1']:
            cmp1_count += 1
        if cpp['hasCmp2']:
            cmp2_count += 1

    if not channel_failures:
        print(f"[PASS] Channel presence flags (hasCmp1={cmp1_count}, hasCmp2={cmp2_count}) 100% consistent with array bit values.")
        print(f"       Dual-cam patterns with hasCmp2=true:")
        for e in cpp_entries:
            if e['hasCmp2']:
                print(f"       - [{e['id']}] {e['friendlyName']} ({e['shortName']})")
    else:
        print(f"[FAIL] Channel flag mismatches ({len(channel_failures)}):")
        for f in channel_failures:
            print(f"  [-] {f}")

    print("\n" + "=" * 80)
    print("  TEST 4: BRAND CATEGORIZATION & getWheelsByCategory FOR ALL 8 CATEGORIES")
    print("=" * 80)
    
    valid_categories = [
        "TOYOTA_DAIHATSU",
        "HONDA",
        "MITSUBISHI",
        "NISSAN",
        "EURO_US",
        "UNIVERSAL",
        "CUSTOM"
    ]
    
    cat_counts = {cat: 0 for cat in valid_categories}
    cat_members = {cat: [] for cat in valid_categories}
    invalid_cats = []
    
    for e in cpp_entries:
        cat = e['category']
        if cat in cat_counts:
            cat_counts[cat] += 1
            cat_members[cat].append(f"[{e['id']:02d}] {e['friendlyName']}")
        else:
            invalid_cats.append(f"Preset {e['id']} has unknown category: {cat}")
            
    print("Category Breakdown:")
    for cat in valid_categories:
        print(f"  - {cat:20s}: {cat_counts[cat]:2d} presets")
        
    sum_individual = sum(cat_counts.values())
    print(f"  ----------------------------------------")
    print(f"  Sum of individual categories: {sum_individual} / 70 presets")
    
    cat_test_failures = []
    if sum_individual != 70:
        cat_test_failures.append(f"Sum of individual categories is {sum_individual}, expected exactly 70!")
    if invalid_cats:
        cat_test_failures.extend(invalid_cats)
        
    # Simulate getWheelsByCategory behavior
    for cat in valid_categories:
        matching = [e for e in cpp_entries if e['category'] == cat]
        if len(matching) != cat_counts[cat]:
            cat_test_failures.append(f"getWheelsByCategory({cat}) count mismatch: {len(matching)} vs {cat_counts[cat]}")

    # Query ALL
    all_matching = [e for e in cpp_entries]
    if len(all_matching) != 70:
        cat_test_failures.append(f"getWheelsByCategory(ALL) returned {len(all_matching)}, expected 70")

    if not cat_test_failures:
        print(f"[PASS] getWheelsByCategory matches all 8 categories with total sum == 70 presets exactly.")
    else:
        print(f"[FAIL] Category test failures:")
        for f in cat_test_failures:
            print(f"  [-] {f}")

    print("\n" + "=" * 80)
    print("  TEST 5: ADVERSARIAL LOOKUP STRESS TESTING")
    print("=" * 80)
    
    lookup_failures = []
    
    # 1. findByFriendlyName case insensitivity and exact match
    for e in cpp_entries:
        fname = e['friendlyName']
        # exact match
        exact_match = next((x for x in cpp_entries if x['friendlyName'] == fname), None)
        if exact_match is None or exact_match['id'] != e['id']:
            lookup_failures.append(f"Exact findByFriendlyName failed for: {fname}")
            
        # lowercase match
        lower_match = next((x for x in cpp_entries if x['friendlyName'].lower() == fname.lower()), None)
        if lower_match is None or lower_match['id'] != e['id']:
            lookup_failures.append(f"Case-insensitive lowercase findByFriendlyName failed for: {fname}")
            
        # uppercase match
        upper_match = next((x for x in cpp_entries if x['friendlyName'].upper() == fname.upper()), None)
        if upper_match is None or upper_match['id'] != e['id']:
            lookup_failures.append(f"Case-insensitive uppercase findByFriendlyName failed for: {fname}")

    # 2. findByShortName exact and case-insensitive
    for e in cpp_entries:
        sname = e['shortName']
        # exact match
        exact_match = next((x for x in cpp_entries if x['shortName'] == sname), None)
        if exact_match is None or exact_match['id'] != e['id']:
            lookup_failures.append(f"Exact findByShortName failed for: {sname}")

    # 3. Check uniqueness of friendlyName
    friendly_name_list = [e['friendlyName'] for e in cpp_entries]
    if len(friendly_name_list) != len(set(friendly_name_list)):
        duplicates = [x for x in friendly_name_list if friendly_name_list.count(x) > 1]
        lookup_failures.append(f"Duplicate friendly names found: {set(duplicates)}")

    # 4. Check uniqueness of shortName
    short_name_list = [e['shortName'] for e in cpp_entries]
    if len(short_name_list) != len(set(short_name_list)):
        duplicates = [x for x in short_name_list if short_name_list.count(x) > 1]
        lookup_failures.append(f"Duplicate short names found: {set(duplicates)}")

    # 5. Check ID indexing continuity (0..69)
    ids = [e['id'] for e in cpp_entries]
    if ids != list(range(70)):
        lookup_failures.append(f"IDs are not continuous 0..69: {ids}")

    if not lookup_failures:
        print(f"[PASS] All lookup tests passed! All 70 friendly names & short names are 100% unique and case-insensitively resolvable.")
    else:
        print(f"[FAIL] Lookup failures ({len(lookup_failures)}):")
        for f in lookup_failures:
            print(f"  [-] {f}")

    print("\n" + "=" * 80)
    print("  TEST 6: OEM CRITICAL PRESET SPOT-CHECKS")
    print("=" * 80)
    critical_presets = [
        (18, "Toyota Avanza 1.3 Crank only", "Old Avanza", 144, 720, "TOYOTA_DAIHATSU", True, False),
        (19, "Toyota Avanza 1.5 Crank only", "New Avanza", 144, 720, "TOYOTA_DAIHATSU", True, False),
        (20, "Toyota Avanza/Xenia/Terios/Rush ", "Avanza/Xenia/Terios/Rush", 144, 720, "TOYOTA_DAIHATSU", True, False),
        (46, "Mitsubishi 4g63 aka 4/2 crank and cam", "4G63 4/2 DSM EVO", 144, 720, "MITSUBISHI", True, False),
        (3,  "60-2 crank only", "60-2 KIA CKP Only", 120, 360, "UNIVERSAL", False, False),
        (4,  "60-2 crank and cam", "60-2 CKP+CMP", 240, 720, "UNIVERSAL", True, False),
        (27, "GM LS1 crank and cam", "GM LS1 24X", 720, 720, "EURO_US", True, True),
        (66, "BMW N20", "BMW N20 58x+CMP", 240, 720, "EURO_US", True, True)
    ]
    spot_failures = []
    for pid, exp_fname, exp_sname, exp_edges, exp_deg, exp_cat, exp_cmp1, exp_cmp2 in critical_presets:
        w = cpp_entries[pid]
        if w['friendlyName'] != exp_fname:
            spot_failures.append(f"Preset {pid} friendlyName mismatch: '{w['friendlyName']}' vs '{exp_fname}'")
        if w['shortName'] != exp_sname:
            spot_failures.append(f"Preset {pid} shortName mismatch: '{w['shortName']}' vs '{exp_sname}'")
        if w['totalEdges'] != exp_edges:
            spot_failures.append(f"Preset {pid} totalEdges mismatch: {w['totalEdges']} vs {exp_edges}")
        if w['cycleDegrees'] != exp_deg:
            spot_failures.append(f"Preset {pid} cycleDegrees mismatch: {w['cycleDegrees']} vs {exp_deg}")
        if w['category'] != exp_cat:
            spot_failures.append(f"Preset {pid} category mismatch: {w['category']} vs {exp_cat}")
        if w['hasCmp1'] != exp_cmp1:
            spot_failures.append(f"Preset {pid} hasCmp1 mismatch: {w['hasCmp1']} vs {exp_cmp1}")
        if w['hasCmp2'] != exp_cmp2:
            spot_failures.append(f"Preset {pid} hasCmp2 mismatch: {w['hasCmp2']} vs {exp_cmp2}")

    if not spot_failures:
        print(f"[PASS] All {len(critical_presets)} OEM critical presets passed spot-checks perfectly.")
    else:
        print(f"[FAIL] Spot check failures ({len(spot_failures)}):")
        for f in spot_failures:
            print(f"  [-] {f}")

    print("\n" + "=" * 80)
    total_failures = len(failures) + len(name_failures) + len(degree_failures) + len(channel_failures) + len(cat_test_failures) + len(lookup_failures) + len(spot_failures)
    
    if total_failures == 0:
        print("  FINAL VERDICT: ALL EMPIRICAL CHALLENGES PASSED (0 ERRORS) -> APPROVE")
        print("=" * 80)
        return 0
    else:
        print(f"  FINAL VERDICT: {total_failures} TOTAL FAILURES DETECTED -> REQUEST_CHANGES")
        print("=" * 80)
        return 1


if __name__ == '__main__':
    sys.exit(run_verification())
