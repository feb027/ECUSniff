import re

def parse_ardustim(file_path):
    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
    clean = re.sub(r"/\*.*?\*/", "", content, flags=re.DOTALL)
    clean = re.sub(r"//.*", "", clean)
    arrays = {}
    pattern = r"const\s+unsigned\s+char\s+(\w+)\s*\[\]\s*(?:PROGMEM)?\s*=\s*\{([^}]+)\};"
    for m in re.finditer(pattern, clean):
        arr_name = m.group(1)
        body = m.group(2)
        vals = [int(x) for x in re.findall(r"\b\d+\b", body)]
        arrays[arr_name] = vals
    return arrays

def parse_cpp_arrays(file_path):
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()
    arrays = {}
    pattern = r"static\s+const\s+uint8_t\s+(\w+)\[(\d+)\]\s*PROGMEM\s*=\s*\{([^}]+)\};"
    for m in re.finditer(pattern, content):
        arr_name = m.group(1)
        body = m.group(3)
        vals = [int(x) for x in re.findall(r"\b\d+\b", body)]
        arrays[arr_name] = vals
    return arrays

# Load parsed mapping
import json
with open(r".agents/survey_spec_miner/parsed_wheels.json", "r", encoding="utf-8") as f:
    parsed_json = json.load(f)

ard_arrs = parse_ardustim(r"external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h")
cpp_arrs = parse_cpp_arrays(r"lib/engine/src/wheel_database.cpp")

# Map index to cpp array
with open(r"lib/engine/src/wheel_database.cpp", "r", encoding="utf-8") as f:
    cpp_content = f.read()
entry_pattern = r'\{\s*(\d+),\s*"([^"]*)",\s*"([^"]*)",\s*BrandCategory::\w+,\s*WheelCycleDegrees::\w+,\s*\d+,\s*(\w+),'
cpp_map = {}
for m in re.finditer(entry_pattern, cpp_content):
    idx = int(m.group(1))
    arr_name = m.group(4)
    cpp_map[idx] = arr_name

mismatches = []
for p in parsed_json:
    idx = p["index"]
    ard_name = p["array_name"]
    cpp_arr_name = cpp_map.get(idx)
    
    if not cpp_arr_name:
        mismatches.append(f"Index {idx} has no cpp array mapped")
        continue
    
    ard_data = ard_arrs.get(ard_name)
    cpp_data = cpp_arrs.get(cpp_arr_name)
    
    if not ard_data:
        mismatches.append(f"ArduStim array {ard_name} not found")
        continue
    if not cpp_data:
        mismatches.append(f"CPP array {cpp_arr_name} not found")
        continue
        
    if len(ard_data) != len(cpp_data):
        mismatches.append(f"Index {idx} length mismatch: ArduStim={len(ard_data)}, CPP={len(cpp_data)}")
        continue
        
    diff_count = sum(1 for a, b in zip(ard_data, cpp_data) if a != b)
    if diff_count > 0:
        mismatches.append(f"Index {idx} data mismatch: {diff_count} differing bytes")

if mismatches:
    print(f"FAILED: {len(mismatches)} mismatches")
    for m in mismatches:
        print(" - ", m)
else:
    print(f"SUCCESS: 100% byte-for-byte exact match across all {len(parsed_json)} patterns ({sum(len(v) for v in cpp_arrs.values())} total bytes)!")
