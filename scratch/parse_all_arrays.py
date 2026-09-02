import re
import json

with open(".agents/survey_spec_miner/parsed_wheels.json", "r", encoding="utf-8") as f:
    wheels = json.load(f)

with open("external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h", "r", encoding="utf-8") as f:
    defs_text = f.read()

# Clean comments from defs_text while preserving structure
# Replace /* ... */ and // ...
clean_text = re.sub(r'/\*.*?\*/', '', defs_text, flags=re.DOTALL)
clean_text = re.sub(r'//.*', '', clean_text)

extracted_arrays = {}

for w in wheels:
    arr_name = w["array_name"]
    # Look for const unsigned char arr_name[] PROGMEM = { ... };
    # or const uint8_t arr_name[] PROGMEM = { ... };
    pattern = rf'(?:const\s+(?:unsigned\s+char|uint8_t)\s+{arr_name}\s*(?:\[\s*\d*\s*\])?\s*PROGMEM\s*=\s*\{{)([^\}}]+)\}};'
    m = re.search(pattern, clean_text)
    if not m:
        # try without PROGMEM or variations
        pattern = rf'{arr_name}\s*(?:\[\s*\d*\s*\])?\s*PROGMEM\s*=\s*\{{([^\}}]+)\}};'
        m = re.search(pattern, clean_text)
    if not m:
        print(f"[ERROR] Could not find array {arr_name}")
        continue
    
    # Parse numbers
    nums_str = m.group(1)
    nums = [int(x.strip()) for x in nums_str.split(',') if x.strip()]
    extracted_arrays[arr_name] = nums
    if len(nums) != w["spec_edges"]:
        print(f"[MISMATCH] {arr_name}: parsed {len(nums)} vs expected {w['spec_edges']}")
    else:
        # Check has_cam1, has_cam2
        has_cam1 = any((n & 2) != 0 for n in nums)
        has_cam2 = any((n & 4) != 0 for n in nums)
        if has_cam1 != w["has_cam1"] or has_cam2 != w["has_cam2"]:
            print(f"[CAM FLAG DIFF] {arr_name}: json=(cam1={w['has_cam1']}, cam2={w['has_cam2']}), parsed=(cam1={has_cam1}, cam2={has_cam2})")

print(f"Extracted {len(extracted_arrays)} / {len(wheels)} arrays successfully!")
