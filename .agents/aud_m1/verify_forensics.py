#!/usr/bin/env python3
"""
Forensic Integrity Audit Script for Milestone 1 (Wheel Database)
Compares lib/engine/src/wheel_database.cpp and lib/engine/include/wheel_database.h
against external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h and ardustim.ino.
"""

import os
import re
import sys

ENGINE_SRC = r"g:\semester 7\ECUSniff\lib\engine\src\wheel_database.cpp"
ENGINE_HDR = r"g:\semester 7\ECUSniff\lib\engine\include\wheel_database.h"
ARDUSTIM_HDR = r"g:\semester 7\ECUSniff\external\ardustim-tftv2-touchscreen\ardustim\wheel_defs.h"
ARDUSTIM_INO = r"g:\semester 7\ECUSniff\external\ardustim-tftv2-touchscreen\ardustim\ardustim.ino"

def clean_c_code(text):
    # Remove block comments
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    # Remove line comments
    text = re.sub(r'//.*', '', text)
    return text

def parse_ardustim_header():
    with open(ARDUSTIM_HDR, "r", encoding="utf-8", errors="replace") as f:
        raw_content = f.read()

    # Friendly names
    friendly_names = {}
    name_re = re.compile(r'const\s+char\s+(\w+)\s*\[\s*\]\s*PROGMEM\s*=\s*"([^"]*)";')
    for match in name_re.finditer(raw_content):
        var_name, text = match.groups()
        friendly_names[var_name] = text

    clean_content = clean_c_code(raw_content)

    arrays = {}
    # Match array blocks on cleaned content
    array_re = re.compile(r'(?:static\s+)?const\s+(?:unsigned\s+char|uint8_t)\s+(\w+)\s*\[\s*\d*\s*\]\s*PROGMEM\s*=\s*\{([^}]+)\};', re.MULTILINE | re.DOTALL)
    for match in array_re.finditer(clean_content):
        arr_name, body = match.groups()
        tokens = [t.strip() for t in body.replace('\n', ' ').split(',') if t.strip()]
        elems = []
        for t in tokens:
            try:
                elems.append(int(t, 0))
            except ValueError:
                pass
        arrays[arr_name] = (len(elems), elems)

    return friendly_names, arrays

def parse_ardustim_wheels():
    with open(ARDUSTIM_INO, "r", encoding="utf-8", errors="replace") as f:
        raw_content = f.read()

    clean_content = clean_c_code(raw_content)

    wheels_block_match = re.search(r'wheels\s+Wheels\s*\[[^\]]*\]\s*=\s*\{(.*?)\n\};', clean_content, re.DOTALL)
    wheels_entries = []
    if wheels_block_match:
        block = wheels_block_match.group(1)
        entry_re = re.compile(r'\{\s*(\w+)\s*,\s*(\w+)\s*,\s*([\d\.]+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}')
        for match in entry_re.finditer(block):
            name_var, arr_var, scaler, edges, degrees = match.groups()
            wheels_entries.append({
                'name_var': name_var,
                'arr_var': arr_var,
                'scaler': float(scaler),
                'edges': int(edges),
                'degrees': int(degrees)
            })
    return wheels_entries

def parse_ecusniff_database():
    with open(ENGINE_SRC, "r", encoding="utf-8", errors="replace") as f:
        raw_content = f.read()

    clean_content = clean_c_code(raw_content)

    arrays = {}
    array_re = re.compile(r'static\s+const\s+uint8_t\s+(\w+)\s*\[(\d+)\]\s*PROGMEM\s*=\s*\{([^}]+)\};', re.MULTILINE | re.DOTALL)
    for match in array_re.finditer(clean_content):
        arr_name, length_str, body = match.groups()
        length = int(length_str)
        tokens = [t.strip() for t in body.replace('\n', ' ').split(',') if t.strip()]
        elems = []
        for t in tokens:
            try:
                elems.append(int(t, 0))
            except ValueError:
                pass
        arrays[arr_name] = (length, elems)

    table_match = re.search(r'static\s+const\s+WheelDefinition\s+s_wheelDatabase\[[^\]]+\]\s*=\s*\{(.*?)\n\};', clean_content, re.DOTALL)
    table_entries = []
    if table_match:
        block = table_match.group(1)
        entry_re = re.compile(r'\{\s*(\d+)\s*,\s*"([^"]*)"\s*,\s*"([^"]*)"\s*,\s*BrandCategory::(\w+)\s*,\s*WheelCycleDegrees::(\w+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(true|false)\s*,\s*(true|false)\s*\}')
        for match in entry_re.finditer(block):
            wid, friendly, short, cat, deg, edges, bit_arr, cmp1, cmp2 = match.groups()
            table_entries.append({
                'id': int(wid),
                'friendlyName': friendly,
                'shortName': short,
                'category': cat,
                'cycleDegrees': 360 if '360' in deg else 720,
                'totalEdges': int(edges),
                'bitArray': bit_arr,
                'hasCmp1': cmp1 == 'true',
                'hasCmp2': cmp2 == 'true'
            })

    return arrays, table_entries

def main():
    print("================================================================================")
    print("                    FORENSIC INTEGRITY AUDIT - MILESTONE 1                      ")
    print("================================================================================")

    ardu_names, ardu_arrays = parse_ardustim_header()
    ardu_wheels = parse_ardustim_wheels()
    ecu_arrays, ecu_table = parse_ecusniff_database()

    print(f"ArduStim Header Arrays Extracted: {len(ardu_arrays)}")
    print(f"ArduStim Wheels[] Entries Extracted: {len(ardu_wheels)}")
    print(f"ECUSniff PROGMEM Arrays Extracted: {len(ecu_arrays)}")
    print(f"ECUSniff Master Table Entries: {len(ecu_table)}")

    checks_passed = 0
    checks_total = 0

    # CHECK 1: Table Size
    checks_total += 1
    if len(ecu_table) == 70:
        print("[PASS] Check 1: ECUSniff master table contains exactly 70 definitions (0..69).")
        checks_passed += 1
    else:
        print(f"[FAIL] Check 1: ECUSniff master table contains {len(ecu_table)} definitions (expected 70).")

    # CHECK 2: Array Count
    checks_total += 1
    if len(ecu_arrays) == 70:
        print("[PASS] Check 2: ECUSniff contains exactly 70 PROGMEM bit-arrays.")
        checks_passed += 1
    else:
        print(f"[FAIL] Check 2: ECUSniff contains {len(ecu_arrays)} PROGMEM bit-arrays (expected 70).")

    # CHECK 3: Byte-by-byte bit-array verification
    checks_total += 1
    array_mismatches = []
    dummy_arrays = []

    for i in range(len(ardu_wheels)):
        wheel_meta = ardu_wheels[i]
        ecu_entry = ecu_table[i] if i < len(ecu_table) else None
        if not ecu_entry:
            array_mismatches.append(f"Index {i}: Missing in ECUSniff table")
            continue

        ardu_arr_var = wheel_meta['arr_var']
        ecu_arr_var = ecu_entry['bitArray']

        # Look up ardu array case-insensitively
        ardu_match_name = next((k for k in ardu_arrays if k.lower() == ardu_arr_var.lower()), None)
        if not ardu_match_name:
            array_mismatches.append(f"Index {i}: ArduStim array '{ardu_arr_var}' not found in header")
            continue

        if ecu_arr_var not in ecu_arrays:
            array_mismatches.append(f"Index {i}: ECUSniff array '{ecu_arr_var}' not found in cpp")
            continue

        ardu_len, ardu_bytes = ardu_arrays[ardu_match_name]
        ecu_len, ecu_bytes = ecu_arrays[ecu_arr_var]

        # Check lengths
        if ecu_len != ecu_entry['totalEdges']:
            array_mismatches.append(f"Index {i} ({ecu_entry['friendlyName']}): Array length {ecu_len} != totalEdges {ecu_entry['totalEdges']}")

        if len(ardu_bytes) != len(ecu_bytes):
            array_mismatches.append(f"Index {i} ({ecu_entry['friendlyName']}): Token count mismatch (ardu={len(ardu_bytes)} vs ecu={len(ecu_bytes)})")
        elif ardu_bytes != ecu_bytes:
            diff_idx = next(idx for idx, (a, b) in enumerate(zip(ardu_bytes, ecu_bytes)) if a != b)
            array_mismatches.append(f"Index {i} ({ecu_entry['friendlyName']}): Byte mismatch at pos {diff_idx} (ardu={ardu_bytes[diff_idx]}, ecu={ecu_bytes[diff_idx]})")

        # Check if array is dummy (all zeros, all ones, etc)
        if len(ecu_bytes) > 0 and len(set(ecu_bytes)) == 1:
            dummy_arrays.append(f"Index {i} ({ecu_entry['friendlyName']}): Constant array of {ecu_bytes[0]}")

    if not array_mismatches and not dummy_arrays:
        total_b = sum(len(b) for _, b in ecu_arrays.values())
        print(f"[PASS] Check 3: All 70 bit-arrays match ArduStim byte-for-byte ({total_b} total bytes verified).")
        checks_passed += 1
    else:
        print(f"[FAIL] Check 3: Found {len(array_mismatches)} array mismatches and {len(dummy_arrays)} dummy arrays:")
        for m in array_mismatches[:10]:
            print(f"   - {m}")
        for d in dummy_arrays:
            print(f"   - {d}")

    # CHECK 4: Friendly Name exact match
    checks_total += 1
    name_mismatches = []
    for i in range(len(ardu_wheels)):
        wheel_meta = ardu_wheels[i]
        ecu_entry = ecu_table[i] if i < len(ecu_table) else None
        if not ecu_entry:
            continue
        ardu_name_var = wheel_meta['name_var']
        ardu_str = ardu_names.get(ardu_name_var, None)
        if ardu_str is None:
            match_k = next((k for k in ardu_names if k.lower() == ardu_name_var.lower()), None)
            if match_k:
                ardu_str = ardu_names[match_k]
            else:
                name_mismatches.append(f"Index {i}: ArduStim string var {ardu_name_var} not found")
                continue

        if ecu_entry['friendlyName'] != ardu_str:
            name_mismatches.append(f"Index {i}: Friendly name mismatch ('{ecu_entry['friendlyName']}' vs ArduStim '{ardu_str}')")

    if not name_mismatches:
        print("[PASS] Check 4: All 70 friendly names match ArduStim source definitions 100%.")
        checks_passed += 1
    else:
        print(f"[FAIL] Check 4: Found {len(name_mismatches)} friendly name mismatches:")
        for nm in name_mismatches:
            print(f"   - {nm}")

    # CHECK 5: Cycle degrees and edge counts
    checks_total += 1
    param_mismatches = []
    for i in range(len(ardu_wheels)):
        wheel_meta = ardu_wheels[i]
        ecu_entry = ecu_table[i] if i < len(ecu_table) else None
        if not ecu_entry:
            continue

        if wheel_meta['degrees'] != ecu_entry['cycleDegrees']:
            param_mismatches.append(f"Index {i}: Cycle degree mismatch ({ecu_entry['cycleDegrees']} vs {wheel_meta['degrees']})")
        if wheel_meta['edges'] != ecu_entry['totalEdges']:
            param_mismatches.append(f"Index {i}: Total edges mismatch ({ecu_entry['totalEdges']} vs {wheel_meta['edges']})")

    if not param_mismatches:
        print("[PASS] Check 5: All 70 cycle degrees and total edge counts match ArduStim table.")
        checks_passed += 1
    else:
        print(f"[FAIL] Check 5: Found {len(param_mismatches)} parameter mismatches:")
        for pm in param_mismatches:
            print(f"   - {pm}")

    # CHECK 6: CMP1 and CMP2 flag correctness
    checks_total += 1
    flag_mismatches = []
    for i, entry in enumerate(ecu_table):
        arr_name = entry['bitArray']
        if arr_name in ecu_arrays:
            _, bytes_list = ecu_arrays[arr_name]
            actual_has_cmp1 = any((b & 0x02) != 0 for b in bytes_list)
            actual_has_cmp2 = any((b & 0x04) != 0 for b in bytes_list)
            if entry['hasCmp1'] != actual_has_cmp1:
                flag_mismatches.append(f"Index {i} ({entry['friendlyName']}): hasCmp1 is {entry['hasCmp1']} but data has CMP1={actual_has_cmp1}")
            if entry['hasCmp2'] != actual_has_cmp2:
                flag_mismatches.append(f"Index {i} ({entry['friendlyName']}): hasCmp2 is {entry['hasCmp2']} but data has CMP2={actual_has_cmp2}")

    if not flag_mismatches:
        print("[PASS] Check 6: hasCmp1 and hasCmp2 flags accurately reflect bitmasks across all 70 arrays.")
        checks_passed += 1
    else:
        print(f"[FAIL] Check 6: Found {len(flag_mismatches)} flag mismatches:")
        for fm in flag_mismatches:
            print(f"   - {fm}")

    # CHECK 7: Category sanity and distribution
    checks_total += 1
    cat_counts = {}
    for entry in ecu_table:
        cat_counts[entry['category']] = cat_counts.get(entry['category'], 0) + 1
    print(f"Brand Category Distribution: {cat_counts}")
    required_cats = ['TOYOTA_DAIHATSU', 'HONDA', 'MITSUBISHI', 'NISSAN', 'EURO_US', 'UNIVERSAL']
    if all(cat_counts.get(cat, 0) > 0 for cat in required_cats):
        print("[PASS] Check 7: Brand categories are properly populated with OEM distributions.")
        checks_passed += 1
    else:
        print("[FAIL] Check 7: One or more major OEM brand categories are empty.")

    # CHECK 8: Code hygiene & lookup function implementation audit
    checks_total += 1
    with open(ENGINE_SRC, "r", encoding="utf-8") as f:
        src_text = f.read()

    suspicious_patterns = [
        r'if\s*\(\s*strcmp\s*\(\s*name\s*,\s*"Toyota Avanza"\s*\)\s*==\s*0\s*\)\s*return\s*&s_wheelDatabase\[18\];',
        r'return\s+0x[0-9a-fA-F]+;\s*//\s*dummy',
        r'TODO',
        r'FIXME',
        r'NotImplemented',
    ]
    found_suspicious = []
    for pat in suspicious_patterns:
        if re.search(pat, src_text, re.IGNORECASE):
            found_suspicious.append(pat)

    if not found_suspicious:
        print("[PASS] Check 8: No hardcoded query shortcuts, stubs, TODOs, or bypasses detected in source.")
        checks_passed += 1
    else:
        print(f"[FAIL] Check 8: Suspicious code patterns detected: {found_suspicious}")

    print("--------------------------------------------------------------------------------")
    print(f"FORENSIC AUDIT SUMMARY: {checks_passed}/{checks_total} CHECKS PASSED")
    print("================================================================================")
    return 0 if checks_passed == checks_total else 1

if __name__ == "__main__":
    sys.exit(main())
