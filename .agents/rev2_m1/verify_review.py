import re
import os
import sys

def parse_ardustim(file_path):
    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
    
    # Remove comments
    content_no_comments = re.sub(r"/\*.*?\*/", "", content, flags=re.DOTALL)
    content_no_comments = re.sub(r"//.*", "", content_no_comments)
    
    # Extract arrays
    arrays = {}
    pattern = r"const\s+unsigned\s+char\s+(\w+)\s*\[\]\s*(?:PROGMEM)?\s*=\s*\{([^}]+)\};"
    for m in re.finditer(pattern, content_no_comments):
        arr_name = m.group(1)
        body = m.group(2)
        vals = [int(x) for x in re.findall(r"\b\d+\b", body)]
        arrays[arr_name] = vals
        
    return arrays

def parse_wheel_db(file_path):
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()
        
    # Extract static arrays: static const uint8_t s_pattern_...[N] PROGMEM = { ... };
    arrays = {}
    pattern = r"static\s+const\s+uint8_t\s+(\w+)\[(\d+)\]\s*PROGMEM\s*=\s*\{([^}]+)\};"
    for m in re.finditer(pattern, content):
        arr_name = m.group(1)
        declared_size = int(m.group(2))
        body = m.group(3)
        vals = [int(x) for x in re.findall(r"\b\d+\b", body)]
        arrays[arr_name] = (declared_size, vals)
        
    # Extract s_wheelDatabase entries
    entries = []
    db_match = re.search(r"static\s+const\s+WheelDefinition\s+s_wheelDatabase\[\d+\]\s*=\s*\{([^;]+)\};", content, re.DOTALL)
    if not db_match:
        print("ERROR: Could not find s_wheelDatabase!")
        return arrays, entries
        
    db_body = db_match.group(1)
    entry_pattern = r'\{\s*(\d+),\s*"([^"]*)",\s*"([^"]*)",\s*BrandCategory::(\w+),\s*WheelCycleDegrees::(\w+),\s*(\d+),\s*(\w+),\s*(true|false),\s*(true|false)\s*\}'
    for m in re.finditer(entry_pattern, db_body):
        entries.append({
            "id": int(m.group(1)),
            "friendlyName": m.group(2),
            "shortName": m.group(3),
            "category": m.group(4),
            "cycleDegrees": m.group(5),
            "totalEdges": int(m.group(6)),
            "bitArrayName": m.group(7),
            "hasCmp1": m.group(8) == "true",
            "hasCmp2": m.group(9) == "true"
        })
    return arrays, entries

ard_arrs = parse_ardustim("external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h")
db_arrs, db_entries = parse_wheel_db("lib/engine/src/wheel_database.cpp")

print(f"ArduStim arrays found: {len(ard_arrs)}")
print(f"WheelDatabase arrays found: {len(db_arrs)}")
print(f"WheelDatabase entries found: {len(db_entries)}")

errors = []
# Check 1: Exactly 70 presets and 70 arrays
if len(db_entries) != 70:
    errors.append(f"Expected 70 entries in s_wheelDatabase, found {len(db_entries)}")
if len(db_arrs) != 70:
    errors.append(f"Expected 70 arrays in wheel_database.cpp, found {len(db_arrs)}")

# Check 2: Sequential ID matching index
for i, entry in enumerate(db_entries):
    if entry["id"] != i:
        errors.append(f"Entry {i} has id {entry['id']}")

# Check 3: Check declared size vs actual element count vs totalEdges
for i, entry in enumerate(db_entries):
    arr_name = entry["bitArrayName"]
    if arr_name not in db_arrs:
        errors.append(f"Entry {i} references unknown array {arr_name}")
        continue
    decl_size, vals = db_arrs[arr_name]
    if len(vals) != decl_size:
        errors.append(f"Array {arr_name} declared size {decl_size} != actual elements {len(vals)}")
    if entry["totalEdges"] != decl_size:
        errors.append(f"Entry {i} totalEdges {entry['totalEdges']} != declared size {decl_size}")

# Check 4: Check hasCmp1 and hasCmp2 flags against array contents
for i, entry in enumerate(db_entries):
    arr_name = entry["bitArrayName"]
    if arr_name in db_arrs:
        _, vals = db_arrs[arr_name]
        actual_has_cmp1 = any((x & 0x02) != 0 for x in vals)
        actual_has_cmp2 = any((x & 0x04) != 0 for x in vals)
        
        if entry["hasCmp1"] != actual_has_cmp1:
            errors.append(f"Entry {i} ({entry['friendlyName']}): hasCmp1 flag {entry['hasCmp1']} != actual array {actual_has_cmp1}")
        if entry["hasCmp2"] != actual_has_cmp2:
            errors.append(f"Entry {i} ({entry['friendlyName']}): hasCmp2 flag {entry['hasCmp2']} != actual array {actual_has_cmp2}")

# Check 5: BMW N20 and GM LS1 dual-cam flags
n20 = db_entries[66] if len(db_entries) > 66 else None
ls1 = db_entries[27] if len(db_entries) > 27 else None

if not n20 or not n20["hasCmp1"] or not n20["hasCmp2"]:
    errors.append(f"BMW N20 (index 66) flags incorrect: hasCmp1={n20['hasCmp1'] if n20 else 'N/A'}, hasCmp2={n20['hasCmp2'] if n20 else 'N/A'}")
if not ls1 or not ls1["hasCmp1"] or not ls1["hasCmp2"]:
    errors.append(f"GM LS1 (index 27) flags incorrect: hasCmp1={ls1['hasCmp1'] if ls1 else 'N/A'}, hasCmp2={ls1['hasCmp2'] if ls1 else 'N/A'}")

# Check 6: Check for any other dual-cam pattern in database
dual_cam_entries = [e for e in db_entries if e["hasCmp2"]]
print(f"Patterns with hasCmp2 == True: {[e['friendlyName'] for e in dual_cam_entries]}")

# Check 7: Trailing space in Avanza/Xenia/Terios/Rush
rush_entry = db_entries[20] if len(db_entries) > 20 else None
if rush_entry:
    if rush_entry["friendlyName"] != "Toyota Avanza/Xenia/Terios/Rush ":
        errors.append(f"Entry 20 friendlyName mismatch: expected 'Toyota Avanza/Xenia/Terios/Rush ', got '{rush_entry['friendlyName']}'")
    if not rush_entry["friendlyName"].endswith(" "):
        errors.append(f"Entry 20 friendlyName missing trailing space: '{rush_entry['friendlyName']}'")
    if rush_entry["shortName"] != "Avanza/Xenia/Terios/Rush":
        errors.append(f"Entry 20 shortName mismatch: expected 'Avanza/Xenia/Terios/Rush', got '{rush_entry['shortName']}'")

# Check 8: String lookup simulation (Case insensitivity, exact match, null, empty)
def simulate_find_friendly(query, entries):
    if not query or query == "":
        return None
    # exact
    for e in entries:
        if e["friendlyName"] == query:
            return e
    # case insensitive
    q_lower = query.lower()
    for e in entries:
        if e["friendlyName"].lower() == q_lower:
            return e
    return None

def simulate_find_short(query, entries):
    if not query or query == "":
        return None
    for e in entries:
        if e["shortName"] == query:
            return e
    q_lower = query.lower()
    for e in entries:
        if e["shortName"].lower() == q_lower:
            return e
    return None

# Test all friendly names case-insensitive
for i, entry in enumerate(db_entries):
    # exact
    res1 = simulate_find_friendly(entry["friendlyName"], db_entries)
    if res1 is None or res1["id"] != i:
        errors.append(f"Exact match failed for friendlyName: '{entry['friendlyName']}'")
    # uppercase
    res2 = simulate_find_friendly(entry["friendlyName"].upper(), db_entries)
    if res2 is None or res2["id"] != i:
        errors.append(f"Upper match failed for friendlyName: '{entry['friendlyName']}'")
    # lowercase
    res3 = simulate_find_friendly(entry["friendlyName"].lower(), db_entries)
    if res3 is None or res3["id"] != i:
        errors.append(f"Lower match failed for friendlyName: '{entry['friendlyName']}'")

# Test null/empty/nonexistent
if simulate_find_friendly(None, db_entries) is not None:
    errors.append("Null friendlyName lookup returned non-None")
if simulate_find_friendly("", db_entries) is not None:
    errors.append("Empty friendlyName lookup returned non-None")
if simulate_find_friendly("NonExistentPatternName12345", db_entries) is not None:
    errors.append("Nonexistent friendlyName lookup returned non-None")

# Test getWheelsByCategory simulation
def simulate_get_by_category(cat_str, entries, max_out=None, supply_out=True):
    count = 0
    out_arr = []
    for e in entries:
        if cat_str == "ALL" or e["category"] == cat_str:
            if supply_out and max_out is not None and len(out_arr) < max_out:
                out_arr.append(e)
            count += 1
    return count, out_arr

categories = ["ALL", "TOYOTA_DAIHATSU", "HONDA", "MITSUBISHI", "NISSAN", "EURO_US", "UNIVERSAL", "CUSTOM"]
cat_counts = {}
for cat in categories:
    total_found, out_arr = simulate_get_by_category(cat, db_entries, max_out=70, supply_out=True)
    cat_counts[cat] = total_found
    # Test buffer limit
    _, limited_arr = simulate_get_by_category(cat, db_entries, max_out=3, supply_out=True)
    if total_found >= 3 and len(limited_arr) != 3:
        errors.append(f"Category {cat}: maxOut=3 failed, wrote {len(limited_arr)}")
    # Test nullptr outWheels
    null_count, null_arr = simulate_get_by_category(cat, db_entries, max_out=70, supply_out=False)
    if null_count != total_found:
        errors.append(f"Category {cat}: count with nullptr outWheels mismatch")

print("Category distribution:", cat_counts)
if cat_counts["ALL"] != 70:
    errors.append(f"Category ALL count {cat_counts['ALL']} != 70")
sum_individual = sum(cat_counts[c] for c in categories if c != "ALL")
if sum_individual != 70:
    errors.append(f"Sum of individual categories {sum_individual} != 70")

if errors:
    print(f"FAIL: {len(errors)} errors found:")
    for err in errors:
        print(" - ", err)
    sys.exit(1)
else:
    print("ALL VERIFICATIONS PASSED CLEANLY!")
