import re
import sys

def extract_c_array(clean_text, var_name):
    pattern = re.compile(r'\b' + re.escape(var_name) + r'\s*\[[^\]]*\]\s*(?:PROGMEM)?\s*=')
    m = pattern.search(clean_text)
    if not m:
        return None
    start_pos = m.end()
    brace_open = clean_text.find('{', start_pos)
    if brace_open == -1:
        return None
    depth = 0
    brace_close = -1
    for i in range(brace_open, len(clean_text)):
        if clean_text[i] == '{':
            depth += 1
        elif clean_text[i] == '}':
            depth -= 1
            if depth == 0:
                brace_close = i
                break
    if brace_close == -1:
        return None
    body = clean_text[brace_open+1:brace_close]
    numbers = [int(x.strip()) for x in body.split(',') if x.strip()]
    return numbers

def main():
    print("=== Reviewer 1: Independent Deep Audit ===")

    # 1. Parse ardustim.ino Wheels[] table
    with open('external/ardustim-tftv2-touchscreen/ardustim/ardustim.ino', 'r', encoding='utf-8') as f:
        ino_text = f.read()

    start_idx = ino_text.find('wheels Wheels[MAX_WHEELS]')
    if start_idx == -1:
        print('FAILED: Could not find wheels Wheels[MAX_WHEELS]')
        sys.exit(1)
    end_idx = ino_text.find('};', start_idx)
    raw_wheels_entries = ino_text[start_idx:end_idx]

    entry_pattern = re.compile(r'\{\s*(\w+)\s*,\s*(\w+)\s*,\s*([\d\.]+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}')
    ardustim_wheels = []
    for line in raw_wheels_entries.strip().splitlines():
        line = line.strip()
        if not line or line.startswith('/*') or line.startswith('//') or 'wheels Wheels' in line:
            continue
        em = entry_pattern.search(line)
        if em:
            name_var, array_var, scaler, edges, degrees = em.groups()
            ardustim_wheels.append({
                'name_var': name_var,
                'array_var': array_var,
                'scaler': float(scaler),
                'edges': int(edges),
                'degrees': int(degrees)
            })

    print(f"1. Parsed {len(ardustim_wheels)} wheels from ardustim.ino")

    # 2. Parse friendly names from wheel_defs.h
    with open('external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h', 'r', encoding='utf-8') as f:
        raw_defs_text = f.read()

    # Pre-clean comments from wheel_defs.h
    clean_defs_text = re.sub(r'/\*.*?\*/', '', raw_defs_text, flags=re.DOTALL)
    clean_defs_text = re.sub(r'//.*', '', clean_defs_text)

    friendly_names = {}
    for m in re.finditer(r'const\s+char\s+(\w+)\s*\[\]\s*PROGMEM\s*=\s*\"([^\"]*)\";', clean_defs_text):
        var_name, name_val = m.groups()
        friendly_names[var_name] = name_val

    # Match each ardustim wheel & extract its array
    for i, w in enumerate(ardustim_wheels):
        name_str = friendly_names.get(w['name_var'], None)
        w['friendly_name'] = name_str
        arr = extract_c_array(clean_defs_text, w['array_var'])
        w['array'] = arr

    # 3. Parse lib/engine/src/wheel_database.cpp
    with open('lib/engine/src/wheel_database.cpp', 'r', encoding='utf-8') as f:
        raw_cpp_text = f.read()

    clean_cpp_text = re.sub(r'/\*.*?\*/', '', raw_cpp_text, flags=re.DOTALL)
    clean_cpp_text = re.sub(r'//.*', '', clean_cpp_text)

    db_start = clean_cpp_text.find('s_wheelDatabase')
    db_start = clean_cpp_text.find('{', db_start)
    db_end = clean_cpp_text.find('};', db_start)
    raw_cpp_db = clean_cpp_text[db_start:db_end]

    cpp_entries = []
    entry_re = re.compile(r'\{\s*(\d+)\s*,\s*\"([^\"]*)\"\s*,\s*\"([^\"]*)\"\s*,\s*BrandCategory::(\w+)\s*,\s*WheelCycleDegrees::(\w+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(true|false)\s*,\s*(true|false)\s*\}')
    for line in raw_cpp_db.strip().splitlines():
        line = line.strip()
        if not line:
            continue
        em = entry_re.search(line)
        if em:
            wid, fname, sname, cat, degs, edges, arr_var, cmp1, cmp2 = em.groups()
            cpp_entries.append({
                'id': int(wid),
                'friendlyName': fname,
                'shortName': sname,
                'category': cat,
                'cycleDegrees': 360 if degs == 'CRANK_360' else 720,
                'totalEdges': int(edges),
                'array_var': arr_var,
                'hasCmp1': cmp1 == 'true',
                'hasCmp2': cmp2 == 'true'
            })

    # 4. Full comparison
    all_passed = True
    total_bytes = 0
    categories_count = {}
    mismatched_wheels = []

    for i in range(70):
        aw = ardustim_wheels[i]
        cw = cpp_entries[i]
        c_arr = extract_c_array(clean_cpp_text, cw['array_var'])
        a_arr = aw['array']
        
        has_issue = False
        if cw['id'] != i or cw['friendlyName'] != aw['friendly_name'] or cw['cycleDegrees'] != aw['degrees'] or cw['totalEdges'] != aw['edges']:
            has_issue = True
            
        if c_arr is None or a_arr is None or len(c_arr) != len(a_arr) or c_arr != a_arr:
            has_issue = True
            mismatches = []
            if c_arr and a_arr and len(c_arr) == len(a_arr):
                for idx, (cv, av) in enumerate(zip(c_arr, a_arr)):
                    if cv != av:
                        mismatches.append((idx, cv, av))
            mismatched_wheels.append((i, cw['friendlyName'], cw['array_var'], aw['array_var'], len(c_arr) if c_arr else 0, len(a_arr) if a_arr else 0, mismatches))
            all_passed = False

        if c_arr:
            total_bytes += len(c_arr)
            actual_has_cmp1 = any((x & 0x02) != 0 for x in c_arr)
            actual_has_cmp2 = any((x & 0x04) != 0 for x in c_arr)
            if cw['hasCmp1'] != actual_has_cmp1:
                print(f"[FAIL] Flag hasCmp1 mismatch at {i} ({cw['friendlyName']}): Table={cw['hasCmp1']} vs Data={actual_has_cmp1}")
                all_passed = False
            if cw['hasCmp2'] != actual_has_cmp2:
                print(f"[FAIL] Flag hasCmp2 mismatch at {i} ({cw['friendlyName']}): Table={cw['hasCmp2']} vs Data={actual_has_cmp2}")
                all_passed = False

        cat = cw['category']
        categories_count[cat] = categories_count.get(cat, 0) + 1

    print(f"\n--- Category Distribution ---")
    for cat, cnt in sorted(categories_count.items()):
        print(f"  {cat}: {cnt} wheels")

    print(f"\n--- Total Flash Memory Footprint ---")
    print(f"  Total raw array bytes: {total_bytes} bytes ({total_bytes / 1024:.2f} KB)")

    if not mismatched_wheels and all_passed:
        print("\n>>> [PASS] ALL 70 WHEELS (0..69) VERIFIED 100% BIT-FOR-BIT AGAINST ARDUSTIM SOURCE <<<")
    else:
        print(f"\n>>> [FAIL] Found {len(mismatched_wheels)} mismatched wheels <<<")
        for wid, fname, cvar, avar, clen, alen, mm in mismatched_wheels:
            print(f" Wheel #{wid}: '{fname}' (CPP: {cvar}, ARD: {avar})")
            print(f"   Length: CPP={clen}, ARD={alen}")
            for idx, cv, av in mm[:10]:
                print(f"     Byte {idx}: CPP={cv} vs ARD={av}")
        sys.exit(1)

if __name__ == '__main__':
    main()
