import re
import json

wheel_defs_path = r'external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h'
ardustim_ino_path = r'external/ardustim-tftv2-touchscreen/ardustim/ardustim.ino'
manager_cpp_path = r'external/ardustim-tftv2-touchscreen/ardustim/src/core/WheelPatternManager.cpp'

with open(wheel_defs_path, 'r', encoding='utf-8') as f:
    wheel_defs = f.read()

with open(ardustim_ino_path, 'r', encoding='utf-8') as f:
    ardustim_ino = f.read()

with open(manager_cpp_path, 'r', encoding='utf-8') as f:
    manager_cpp = f.read()

# 1. Extract enum definitions
enum_match = re.search(r'typedef enum\s*\{(.*?)\}\s*WheelType;', wheel_defs, re.DOTALL)
enum_items = []
if enum_match:
    enum_body = enum_match.group(1)
    enum_lines = [line.strip() for line in enum_body.split('\n') if line.strip() and not line.strip().startswith('/*') and not line.strip().startswith('//')]
    for line in enum_lines:
        cleaned = re.sub(r'/\*.*?\*/', '', line).strip()
        cleaned = re.sub(r'//.*', '', cleaned).strip()
        parts = [p.strip() for p in cleaned.split(',') if p.strip()]
        for p in parts:
            if p and p != 'MAX_WHEELS':
                enum_items.append(p)

# 2. Extract friendly names constants from wheel_defs.h
friendly_names_defs = {}
for m in re.finditer(r'const\s+char\s+(\w+)\s*\[\]\s*PROGMEM\s*=\s*\"([^\"]*)\";', wheel_defs):
    var_name, val = m.group(1), m.group(2)
    friendly_names_defs[var_name] = val

# 3. Extract Wheels[] array from ardustim.ino
wheels_table_match = re.search(r'wheels\s+Wheels\[MAX_WHEELS\]\s*=\s*\{(.*?)\};', ardustim_ino, re.DOTALL)
wheels_entries = []
if wheels_table_match:
    raw_entries = wheels_table_match.group(1)
    pattern = r'\{\s*(\w+)\s*,\s*(\w+)\s*,\s*([0-9\.]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\}'
    for m in re.finditer(pattern, raw_entries):
        name_var, array_var, scaler, max_edges, degrees = m.group(1), m.group(2), float(m.group(3)), int(m.group(4)), int(m.group(5))
        friendly_str = friendly_names_defs.get(name_var, name_var)
        wheels_entries.append({
            'name_var': name_var,
            'friendly_name': friendly_str,
            'array_var': array_var,
            'rpm_scaler': scaler,
            'wheel_max_edges': max_edges,
            'wheel_degrees': degrees
        })

# 4. Extract arrays robustly
# Clean C multi-line comments and single-line comments properly first, but preserve array definitions
def extract_c_arrays(code):
    # Strip block comments /* ... */
    # Note: avoid nested comment confusion
    clean_code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)
    # Strip line comments // ...
    clean_code = re.sub(r'//.*', '', clean_code)
    
    arrays = {}
    pattern = r'const\s+unsigned\s+char\s+(\w+)\s*\[\]\s*(?:PROGMEM)?\s*=\s*\{(.*?)\};'
    for m in re.finditer(pattern, clean_code, re.DOTALL):
        name = m.group(1)
        body = m.group(2)
        nums = [int(n) for n in re.findall(r'\b\d+\b', body)]
        arrays[name] = nums
    return arrays

arrays_data = extract_c_arrays(wheel_defs)

# 5. Extract TFT friendly names from WheelPatternManager.cpp
tft_names_match = re.search(r'static\s+const\s+char\*\s+const\s+friendlyNames\[\]\s*PROGMEM\s*=\s*\{(.*?)\};', manager_cpp, re.DOTALL)
tft_names = []
if tft_names_match:
    for m in re.finditer(r'\"([^\"]*)\"', tft_names_match.group(1)):
        tft_names.append(m.group(1))

# Extract categorized list from WheelPatternManager.cpp
def extract_category_list(var_name):
    m = re.search(r'const\s+uint8_t\s+WheelPatternManager::' + var_name + r'\[\]\s*PROGMEM\s*=\s*\{(.*?)\};', manager_cpp, re.DOTALL)
    if m:
        body = m.group(1)
        items = [i.strip() for i in body.split(',') if i.strip()]
        return items
    return []

crank_only_enums = extract_category_list('crankOnlyPatterns')
crank_and_cam_enums = extract_category_list('crankAndCamPatterns')
dizzy_enums = extract_category_list('dizzyPatterns')

print(f"Total Enums: {len(enum_items)}")
print(f"Total Wheels Entries: {len(wheels_entries)}")
print(f"Total TFT Names: {len(tft_names)}")
print(f"Total Parsed Arrays: {len(arrays_data)}")

results = []
for idx in range(len(wheels_entries)):
    enum_name = enum_items[idx] if idx < len(enum_items) else "N/A"
    w = wheels_entries[idx]
    arr_name = w['array_var']
    arr = arrays_data.get(arr_name, [])
    tft_name = tft_names[idx] if idx < len(tft_names) else "N/A"
    distinct_vals = sorted(list(set(arr))) if arr else []
    
    # Check signal channels
    has_crank = any(v & 1 for v in distinct_vals)
    has_cam1 = any(v & 2 for v in distinct_vals)
    has_cam2 = any(v & 4 for v in distinct_vals)
    
    # Determine type category
    ptype = "Special / Arbitrary"
    if enum_name in crank_only_enums:
        ptype = "Crank Only"
    elif enum_name in crank_and_cam_enums:
        ptype = "Crank and Cam"
    elif enum_name in dizzy_enums:
        ptype = "Distributor"
    
    # Brand categorization inference
    brand = "Universal"
    fn = w['friendly_name'].lower()
    en = enum_name.lower()
    tft = tft_name.lower()
    
    if "avanza" in fn or "terios" in fn or "4age" in fn or "4agze" in fn or "2jz" in tft or "toyota" in fn:
        brand = "Toyota/Daihatsu"
    elif "honda" in fn or "d17" in fn or "jazz" in fn or "rc51" in fn:
        brand = "Honda"
    elif "mitsubishi" in fn or "4g63" in fn or "6g72" in fn or "3a92" in fn or "420a" in fn or "eclipse" in tft:
        brand = "Mitsubishi/DSM"
    elif "nissan" in fn or "livina" in fn or "juke" in fn or ("cas" in fn and "360" in fn):
        brand = "Nissan"
    elif "mazda" in fn or "miata" in fn or "fe3" in fn or "323" in fn:
        brand = "Mazda"
    elif "subaru" in fn:
        brand = "Subaru"
    elif "gm" in fn or "optispark" in fn or "ls1" in fn or "58x" in fn or "4200" in fn or "7x" in fn or "oss" in fn:
        brand = "GM"
    elif "ford" in fn or "st170" in fn:
        brand = "Ford"
    elif "chrysler" in fn or "jeep" in fn or "viper" in fn:
        brand = "Chrysler/Jeep/Dodge"
    elif "audi" in fn:
        brand = "Audi/VAG"
    elif "bmw" in fn:
        brand = "BMW"
    elif "fiat" in fn:
        brand = "Fiat"
    elif "weber" in fn or "iaw" in fn:
        brand = "Weber-Marelli"
    elif "volvo" in fn:
        brand = "Volvo"
    elif "suzuki" in fn or "drz" in fn or "swift" in tft:
        brand = "Suzuki"
    elif "yamaha" in fn or "r1" in fn or "r6" in fn:
        brand = "Yamaha/Motorcycle"
    elif "buell" in fn:
        brand = "Buell/Harley"
    elif "lotus" in fn:
        brand = "Lotus"
    elif "daihatsu" in fn or "taruna" in tft:
        brand = "Daihatsu"
    elif "dizzy" in fn or "sixty_minus_two" in en or "thirty_six" in en or "twenty_four" in en or "twelve" in en or "eight" in en or "six" in en or "four" in en or "oddfire" in en:
        brand = "Universal"
    
    res = {
        'index': idx,
        'enum_name': enum_name,
        'friendly_name': w['friendly_name'],
        'tft_name': tft_name,
        'brand': brand,
        'pattern_category': ptype,
        'array_name': arr_name,
        'actual_len': len(arr),
        'spec_edges': w['wheel_max_edges'],
        'degrees': w['wheel_degrees'],
        'rpm_scaler': w['rpm_scaler'],
        'has_crank': has_crank,
        'has_cam1': has_cam1,
        'has_cam2': has_cam2,
        'distinct_vals': distinct_vals,
        'len_matches': len(arr) == w['wheel_max_edges'],
        'raw_array': arr
    }
    results.append(res)

with open(r'.agents/survey_spec_miner/parsed_wheels.json', 'w') as f:
    json.dump([{k: v for k, v in r.items() if k != 'raw_array'} for r in results], f, indent=2)

print("Saved .agents/survey_spec_miner/parsed_wheels.json successfully.")

# Check for length mismatches
mismatches = [r for r in results if not r['len_matches']]
if mismatches:
    print(f"WARNING: {len(mismatches)} length mismatches detected!")
    for m in mismatches:
        print(f"  [{m['index']}] {m['enum_name']}: actual={m['actual_len']} vs spec={m['spec_edges']}")
else:
    print("ALL 70 patterns array lengths match their spec_edges perfectly!")
