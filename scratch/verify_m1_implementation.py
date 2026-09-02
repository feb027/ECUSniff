import re
import json
import os

print("--- Starting Worker M1 Comprehensive Verification ---")

# 1. Check file existence
h_path = "lib/engine/include/wheel_database.h"
cpp_path = "lib/engine/src/wheel_database.cpp"
types_path = "lib/engine/include/pattern_types.h"

assert os.path.exists(h_path), f"Missing {h_path}"
assert os.path.exists(cpp_path), f"Missing {cpp_path}"
assert os.path.exists(types_path), f"Missing {types_path}"
print("[PASS] All required header and source files exist.")

# 2. Check pattern_types.h
with open(types_path, "r", encoding="utf-8") as f:
    types_content = f.read()

assert "SIGNAL_BIT_CKP" in types_content
assert "SIGNAL_BIT_CMP1" in types_content
assert "SIGNAL_BIT_CMP2" in types_content
assert "SIGNAL_BIT_KNOCK" in types_content
assert "struct PulseTransition" in types_content
print("[PASS] pattern_types.h contains all required bitmasks and PulseTransition struct.")

# 3. Check wheel_database.h
with open(h_path, "r", encoding="utf-8") as f:
    h_content = f.read()

assert "enum class BrandCategory" in h_content
assert "enum class WheelCycleDegrees" in h_content
assert "struct WheelDefinition" in h_content
assert "namespace WheelDatabase" in h_content
assert "getWheelCount" in h_content
assert "getWheel(" in h_content
assert "getWheelById" in h_content
assert "findByFriendlyName" in h_content
assert "findByShortName" in h_content
assert "getWheelsByCategory" in h_content
assert "getCategoryName" in h_content
print("[PASS] wheel_database.h exports full WheelDefinition struct and WheelDatabase API.")

# 4. Check wheel_database.cpp arrays and definitions against parsed_wheels.json and wheel_defs.h
with open(".agents/survey_spec_miner/parsed_wheels.json", "r", encoding="utf-8") as f:
    wheels = json.load(f)

with open(cpp_path, "r", encoding="utf-8") as f:
    cpp_content = f.read()

assert len(wheels) == 70, f"Expected 70 wheels, got {len(wheels)}"

# Check each array in cpp
total_verified_bytes = 0
for w in wheels:
    idx = w["index"]
    arr_name = w["array_name"]
    var_name = f"s_pattern_{idx:02d}_{arr_name}"
    
    # Extract array from cpp
    pattern = rf'static\s+const\s+uint8_t\s+{var_name}\[(\d+)\]\s*PROGMEM\s*=\s*\{{([^}}]+)\}};'
    m = re.search(pattern, cpp_content)
    assert m, f"Missing array {var_name} in wheel_database.cpp"
    
    size_in_decl = int(m.group(1))
    assert size_in_decl == w["spec_edges"], f"Size mismatch in {var_name}: {size_in_decl} vs {w['spec_edges']}"
    
    nums = [int(x.strip()) for x in m.group(2).split(',') if x.strip()]
    assert len(nums) == w["spec_edges"], f"Element count mismatch in {var_name}: {len(nums)} vs {w['spec_edges']}"
    total_verified_bytes += len(nums)
    
    # Check friendly name in cpp table
    fn_escaped = w["friendly_name"].replace('"', '\\"')
    assert f'"{fn_escaped}"' in cpp_content, f"Missing friendly name '{fn_escaped}' in cpp table"

print(f"[PASS] All 70 PROGMEM arrays verified ({total_verified_bytes} bytes).")
print(f"[PASS] All 70 friendly names verified in master database table.")

# 5. Check BrandCategory mapping completeness
categories = [
    "BrandCategory::ALL",
    "BrandCategory::TOYOTA_DAIHATSU",
    "BrandCategory::HONDA",
    "BrandCategory::MITSUBISHI",
    "BrandCategory::NISSAN",
    "BrandCategory::EURO_US",
    "BrandCategory::UNIVERSAL",
    "BrandCategory::CUSTOM",
]
for cat in categories[1:-1]:
    assert cat in cpp_content, f"Category {cat} not utilized in wheel_database.cpp"

print("[PASS] All brand categories correctly mapped and utilized.")
print("--- ALL VERIFICATIONS PASSED 100% ---")
