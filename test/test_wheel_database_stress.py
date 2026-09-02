#!/usr/bin/env python3
"""
ECUSniff Milestone 1 (Wheel Database) - Empirical Challenger Stress & Boundary Test Suite
Author: Challenger 2 (Empirical Challenger)
"""

import sys
import os
import re

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
WHEEL_CPP_PATH = os.path.join(ROOT_DIR, "lib", "engine", "src", "wheel_database.cpp")
WHEEL_H_PATH = os.path.join(ROOT_DIR, "lib", "engine", "include", "wheel_database.h")
ARDUSTIM_DEFS_PATH = os.path.join(ROOT_DIR, "external", "ardustim-tftv2-touchscreen", "ardustim", "wheel_defs.h")

class TestSuite:
    def __init__(self):
        self.total = 0
        self.passed = 0
        self.failed = 0
        self.findings = []
        self.failures = []

    def assert_true(self, condition, test_name, error_msg=""):
        self.total += 1
        if condition:
            self.passed += 1
        else:
            self.failed += 1
            msg = f"[FAIL] {test_name}: {error_msg}"
            self.failures.append(msg)
            print(msg)

    def assert_equal(self, expected, actual, test_name, error_msg=""):
        cond = (expected == actual)
        msg = error_msg or f"Expected '{expected}', got '{actual}'"
        self.assert_true(cond, test_name, msg)

    def assert_is_none(self, val, test_name, error_msg=""):
        self.assert_true(val is None, test_name, error_msg or f"Expected None, got {val}")

    def assert_is_not_none(self, val, test_name, error_msg=""):
        self.assert_true(val is not None, test_name, error_msg or "Expected non-None value")

    def record_finding(self, finding_title, description):
        self.findings.append((finding_title, description))

    def print_summary(self):
        print("\n" + "=" * 80)
        print("         CHALLENGER 2: EMPIRICAL STRESS & BOUNDARY TEST SUMMARY")
        print("=" * 80)
        print(f"Total assertions: {self.total}")
        print(f"Passed:           {self.passed}")
        print(f"Failed:           {self.failed}")
        if self.findings:
            print(f"\nDocumented Findings ({len(self.findings)}):")
            for title, desc in self.findings:
                print(f"  [FINDING] {title}: {desc}")
        print("=" * 80)
        return self.failed == 0


def parse_ardustim_header(path):
    if not os.path.exists(path):
        raise FileNotFoundError(f"Missing ArduStim header: {path}")
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    clean_code = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    clean_code = re.sub(r'//.*', '', clean_code)

    arrays = {}
    pattern = r'const\s+unsigned\s+char\s+(\w+)\s*\[\]\s*(?:PROGMEM)?\s*=\s*\{(.*?)\};'
    for m in re.finditer(pattern, clean_code, re.DOTALL):
        name = m.group(1)
        body = m.group(2)
        nums = [int(n) for n in re.findall(r'\b\d+\b', body)]
        arrays[name] = nums

    return arrays


def parse_cpp_database(path):
    if not os.path.exists(path):
        raise FileNotFoundError(f"Missing wheel_database.cpp: {path}")
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    arrays = {}
    pattern = r'static\s+const\s+uint8_t\s+(\w+)\s*\[\s*(\d+)\s*\]\s*PROGMEM\s*=\s*\{(.*?)\};'
    for m in re.finditer(pattern, content, re.DOTALL):
        arr_name = m.group(1)
        arr_len = int(m.group(2))
        body = m.group(3)
        nums = [int(n) for n in re.findall(r'\b\d+\b', body)]
        arrays[arr_name] = {
            "declared_len": arr_len,
            "data": nums
        }

    entry_pattern = r'\{\s*(\d+)\s*,\s*"([^"]*)"\s*,\s*"([^"]*)"\s*,\s*BrandCategory::(\w+)\s*,\s*WheelCycleDegrees::(\w+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(true|false)\s*,\s*(true|false)\s*\}'
    entries = []
    for m in re.finditer(entry_pattern, content):
        entries.append({
            "id": int(m.group(1)),
            "friendlyName": m.group(2),
            "shortName": m.group(3),
            "category": m.group(4),
            "cycleDegrees": 360 if m.group(5) == "CRANK_360" else 720,
            "totalEdges": int(m.group(6)),
            "arrayName": m.group(7),
            "hasCmp1": m.group(8) == "true",
            "hasCmp2": m.group(9) == "true"
        })

    return arrays, entries


class SimulatedDatabase:
    """Exact emulation of C++ WheelDatabase query semantics"""
    def __init__(self, entries, arrays):
        self.entries = entries
        self.arrays = arrays
        self.TOTAL_WHEELS = len(entries)

    def getWheelCount(self):
        return self.TOTAL_WHEELS

    def getWheel(self, index):
        if 0 <= index < self.TOTAL_WHEELS:
            return self.entries[index]
        return None

    def getWheelById(self, wheel_id):
        if 0 <= wheel_id < self.TOTAL_WHEELS:
            return self.entries[wheel_id]
        return None

    def _stringEqualsIgnoreCase(self, a, b):
        if a is None or b is None:
            return False
        return a.lower() == b.lower()

    def findByFriendlyName(self, name):
        if name is None or len(name) == 0:
            return None
        # First pass: exact match
        for w in self.entries:
            if w["friendlyName"] == name:
                return w
        # Second pass: case-insensitive match
        for w in self.entries:
            if self._stringEqualsIgnoreCase(w["friendlyName"], name):
                return w
        return None

    def findByShortName(self, name):
        if name is None or len(name) == 0:
            return None
        # First pass: exact match
        for w in self.entries:
            if w["shortName"] == name:
                return w
        # Second pass: case-insensitive match
        for w in self.entries:
            if self._stringEqualsIgnoreCase(w["shortName"], name):
                return w
        return None

    def getWheelsByCategory(self, cat, maxOut=None, provide_out=True):
        count = 0
        outWheels = []
        for w in self.entries:
            if cat == "ALL" or w["category"] == cat:
                if provide_out and (maxOut is None or count < maxOut):
                    outWheels.append(w)
                count += 1
        return count, outWheels

    def getCategoryName(self, cat):
        names = {
            "ALL": "ALL",
            "TOYOTA_DAIHATSU": "Toyota / Daihatsu",
            "HONDA": "Honda",
            "MITSUBISHI": "Mitsubishi",
            "NISSAN": "Nissan",
            "EURO_US": "Euro / US",
            "UNIVERSAL": "Universal",
            "CUSTOM": "Custom"
        }
        return names.get(cat, "Unknown")


def run_tests():
    t = TestSuite()

    print(">>> Loading source files and raw definitions...")
    ardustim_arrays = parse_ardustim_header(ARDUSTIM_DEFS_PATH)
    cpp_arrays, entries = parse_cpp_database(WHEEL_CPP_PATH)
    db = SimulatedDatabase(entries, cpp_arrays)

    print(f"Loaded {len(ardustim_arrays)} ArduStim arrays, {len(cpp_arrays)} C++ arrays, {len(entries)} DB entries.")
    t.assert_equal(70, len(entries), "Total Entries Check", "Database must have exactly 70 entries")
    t.assert_equal(70, len(cpp_arrays), "C++ Arrays Check", "Database must define exactly 70 PROGMEM arrays")

    # =========================================================================
    # SECTION 1: BOUNDARY & EDGE CASE INPUT TESTING
    # =========================================================================
    print("\n--- Running Section 1: Boundary & Input Stress Tests ---")

    # 1.1 Numerical Index Boundaries
    boundary_indices = [-1000, -1, 70, 71, 100, 255, 65535, 1000000]
    for idx in boundary_indices:
        res = db.getWheel(idx)
        t.assert_is_none(res, f"getWheel({idx}) Boundary Check", f"Out of bounds index {idx} must return nullptr/None")
        res_id = db.getWheelById(idx)
        t.assert_is_none(res_id, f"getWheelById({idx}) Boundary Check", f"Out of bounds ID {idx} must return nullptr/None")

    # Valid indices 0..69
    for i in range(70):
        w = db.getWheel(i)
        t.assert_is_not_none(w, f"getWheel({i}) Valid Check")
        t.assert_equal(i, w["id"], f"getWheel({i}) ID Check")
        w_id = db.getWheelById(i)
        t.assert_is_not_none(w_id, f"getWheelById({i}) Valid Check")
        t.assert_equal(i, w_id["id"], f"getWheelById({i}) ID Check")

    # 1.2 Null and Empty String Queries
    t.assert_is_none(db.findByFriendlyName(None), "findByFriendlyName(NULL)")
    t.assert_is_none(db.findByFriendlyName(""), "findByFriendlyName('')")
    t.assert_is_none(db.findByShortName(None), "findByShortName(NULL)")
    t.assert_is_none(db.findByShortName(""), "findByShortName('')")

    # Whitespace and Non-existent queries
    t.assert_is_none(db.findByFriendlyName("   "), "findByFriendlyName('   ')")
    t.assert_is_none(db.findByFriendlyName("NON_EXISTENT_WHEEL_404"), "findByFriendlyName('NON_EXISTENT')")
    t.assert_is_none(db.findByShortName("NON_EXISTENT_SHORT_NAME"), "findByShortName('NON_EXISTENT')")

    # Partial prefix mismatch tests (must NOT false positive)
    t.assert_is_none(db.findByFriendlyName("Toyota Avanza"), "findByFriendlyName('Toyota Avanza') partial")
    t.assert_is_none(db.findByFriendlyName("60-2"), "findByFriendlyName('60-2') partial")
    t.assert_is_none(db.findByShortName("Avanza"), "findByShortName('Avanza') partial")

    # 1.3 Case Sensitivity & Invariance Across ALL 70 Patterns
    for i, w in enumerate(entries):
        fname = w["friendlyName"]
        sname = w["shortName"]

        # Exact match
        t.assert_equal(i, db.findByFriendlyName(fname)["id"], f"Exact friendlyName match [{i}]")
        
        # Check shortName collisions
        match_short = db.findByShortName(sname)
        if i == 55 and sname == "12-1 CKP+CMP":
            t.record_finding("ShortName Collision", "Preset 55 ('12/1 (12 crank with cam)') shares duplicate shortName '12-1 CKP+CMP' with Preset 11 ('12-1 crank with cam'), causing findByShortName to resolve to ID 11 instead of 55.")
            t.assert_equal(11, match_short["id"], f"Documented shortName collision for preset 55 resolves to 11")
        else:
            t.assert_equal(i, match_short["id"], f"Exact shortName match [{i}]")

        # Lowercase match
        t.assert_equal(i, db.findByFriendlyName(fname.lower())["id"], f"Lowercase friendlyName match [{i}] ({fname})")
        if i != 55:
            t.assert_equal(i, db.findByShortName(sname.lower())["id"], f"Lowercase shortName match [{i}] ({sname})")

        # Uppercase match
        t.assert_equal(i, db.findByFriendlyName(fname.upper())["id"], f"Uppercase friendlyName match [{i}] ({fname})")
        if i != 55:
            t.assert_equal(i, db.findByShortName(sname.upper())["id"], f"Uppercase shortName match [{i}] ({sname})")

        # Mixed / Inverted case match (alternating upper/lower)
        alt_fname = "".join([c.upper() if idx % 2 == 0 else c.lower() for idx, c in enumerate(fname)])
        alt_sname = "".join([c.upper() if idx % 2 == 0 else c.lower() for idx, c in enumerate(sname)])
        t.assert_equal(i, db.findByFriendlyName(alt_fname)["id"], f"Mixed-case friendlyName match [{i}] ({alt_fname})")
        if i != 55:
            t.assert_equal(i, db.findByShortName(alt_sname)["id"], f"Mixed-case shortName match [{i}] ({alt_sname})")

    # 1.4 Category Filtering Boundary Tests
    valid_categories = ["ALL", "TOYOTA_DAIHATSU", "HONDA", "MITSUBISHI", "NISSAN", "EURO_US", "UNIVERSAL", "CUSTOM"]
    expected_category_counts = {
        "ALL": 70,
        "TOYOTA_DAIHATSU": 8,
        "HONDA": 5,
        "MITSUBISHI": 4,
        "NISSAN": 2,
        "EURO_US": 23,
        "UNIVERSAL": 28,
        "CUSTOM": 0
    }
    
    total_grouped = 0
    for cat in valid_categories:
        count, wheels = db.getWheelsByCategory(cat)
        expected_cnt = expected_category_counts[cat]
        t.assert_equal(expected_cnt, count, f"Category count for {cat}")
        t.assert_equal(expected_cnt, len(wheels), f"Returned wheels length for {cat}")
        if cat != "ALL":
            total_grouped += count
            for w in wheels:
                t.assert_equal(cat, w["category"], f"Category member verification for {cat}")
    t.assert_equal(70, total_grouped, "Sum of individual categories must equal 70")

    # Category buffer limits: maxOut = 0, maxOut < count, maxOut > count, provide_out = False (nullptr simulation)
    cnt_toyota, out_toyota_limited = db.getWheelsByCategory("TOYOTA_DAIHATSU", maxOut=3)
    t.assert_equal(8, cnt_toyota, "Toyota total count should be 8 regardless of maxOut")
    t.assert_equal(3, len(out_toyota_limited), "Toyota limited output should only write maxOut=3 items")

    cnt_all_null, out_null = db.getWheelsByCategory("ALL", provide_out=False)
    t.assert_equal(70, cnt_all_null, "Query with nullptr outWheels returns full count")
    t.assert_equal(0, len(out_null), "Query with nullptr writes 0 items")

    # Invalid Category Enum
    cnt_invalid, out_invalid = db.getWheelsByCategory("INVALID_CATEGORY_999")
    t.assert_equal(0, cnt_invalid, "Invalid category enum query returns 0")
    t.assert_equal(0, len(out_invalid), "Invalid category enum query writes 0 items")

    # Category Names check
    t.assert_equal("Toyota / Daihatsu", db.getCategoryName("TOYOTA_DAIHATSU"), "Toyota category name")
    t.assert_equal("Honda", db.getCategoryName("HONDA"), "Honda category name")
    t.assert_equal("Mitsubishi", db.getCategoryName("MITSUBISHI"), "Mitsubishi category name")
    t.assert_equal("Nissan", db.getCategoryName("NISSAN"), "Nissan category name")
    t.assert_equal("Euro / US", db.getCategoryName("EURO_US"), "Euro/US category name")
    t.assert_equal("Universal", db.getCategoryName("UNIVERSAL"), "Universal category name")
    t.assert_equal("Custom", db.getCategoryName("CUSTOM"), "Custom category name")
    t.assert_equal("Unknown", db.getCategoryName("INVALID"), "Unknown category fallback")


    # =========================================================================
    # SECTION 2: BITMASK INTEGRITY ACROSS ALL 70 PATTERNS
    # =========================================================================
    print("\n--- Running Section 2: Bitmask Integrity Across All 70 Patterns ---")

    total_progmem_bytes = 0
    cmp1_active_wheels = []
    cmp2_active_wheels = []

    for i, w in enumerate(entries):
        arr_info = cpp_arrays.get(w["arrayName"])
        t.assert_is_not_none(arr_info, f"Array {w['arrayName']} found in C++")
        arr = arr_info["data"]
        total_progmem_bytes += len(arr)

        # 2.1 Array length matches struct totalEdges
        t.assert_equal(w["totalEdges"], len(arr), f"Array length match for wheel {i} ({w['friendlyName']})")
        t.assert_equal(arr_info["declared_len"], len(arr), f"Declared array size match for wheel {i}")

        # 2.2 Bit values in range [0..7] (no high bits > 0x07)
        for byte_idx, val in enumerate(arr):
            t.assert_true(val in (0, 1, 2, 3, 4, 5, 6, 7), 
                          f"Wheel {i} byte {byte_idx} validity", 
                          f"Found invalid byte value {val} at offset {byte_idx} in {w['arrayName']}")
            t.assert_equal(0, val & ~0x07, f"Wheel {i} byte {byte_idx} high bits clean")

        # 2.3 Bit 0 is Crankshaft (CKP) -> Every wheel must have active CKP
        has_ckp = any((val & 0x01) != 0 for val in arr)
        t.assert_true(has_ckp, f"Wheel {i} ({w['friendlyName']}) has active CKP")

        # 2.4 Bit 1 is Camshaft 1 (CMP1)
        has_cmp1 = any((val & 0x02) != 0 for val in arr)
        t.assert_equal(w["hasCmp1"], has_cmp1, f"Wheel {i} hasCmp1 flag matches data ({w['friendlyName']})")
        if has_cmp1:
            cmp1_active_wheels.append(i)

        # 2.5 Bit 2 is Camshaft 2 (CMP2)
        has_cmp2 = any((val & 0x04) != 0 for val in arr)
        t.assert_equal(w["hasCmp2"], has_cmp2, f"Wheel {i} hasCmp2 flag matches data ({w['friendlyName']})")
        if has_cmp2:
            cmp2_active_wheels.append(i)

    print(f"Total Flash PROGMEM data verified: {total_progmem_bytes} bytes across 70 arrays.")
    t.assert_equal(15429, total_progmem_bytes, "Total PROGMEM footprint check")
    t.assert_equal(50, len(cmp1_active_wheels), "Total wheels with CMP1 active must equal exactly 50")
    t.assert_equal(2, len(cmp2_active_wheels), "Total wheels with CMP2 active must equal exactly 2")
    t.assert_equal([27, 66], cmp2_active_wheels, "Only GM LS1 (27) and BMW N20 (66) have CMP2 active")


    # =========================================================================
    # SECTION 3: CRITICAL OEM PATTERN VERIFICATION
    # =========================================================================
    print("\n--- Running Section 3: Critical OEM Pattern Deep Verification ---")

    # 3.1 Old Avanza (1.3L K3-VE) - Index 18
    # 144 segments, 720 deg (5.0 deg/seg), CKP + CMP1
    w_old = entries[18]
    t.assert_equal("Toyota Avanza 1.3 Crank only", w_old["friendlyName"], "Old Avanza friendly name")
    t.assert_equal("Old Avanza", w_old["shortName"], "Old Avanza short name")
    t.assert_equal(144, w_old["totalEdges"], "Old Avanza total edges")
    t.assert_equal(720, w_old["cycleDegrees"], "Old Avanza cycle degrees")
    t.assert_true(w_old["hasCmp1"], "Old Avanza has CMP1")
    t.assert_true(not w_old["hasCmp2"], "Old Avanza no CMP2")
    arr_old = cpp_arrays[w_old["arrayName"]]["data"]
    # Byte-by-byte comparison against ArduStim source
    arr_old_ref = ardustim_arrays["old_avanza"]
    t.assert_equal(arr_old_ref, arr_old, "Old Avanza byte-for-byte fidelity with ArduStim")
    # Verify CMP1 3 pulse groups (intake / cylinder identifier teeth)
    cmp1_old_active = [idx for idx, v in enumerate(arr_old) if (v & 0x02)]
    t.assert_equal(36, len(cmp1_old_active), "Old Avanza total CAM active segments")
    t.assert_true(all(37 <= x <= 48 or 73 <= x <= 84 or 109 <= x <= 120 for x in cmp1_old_active), "Old Avanza CAM pulse positions")

    # 3.2 New Avanza (1.5L 3SZ-VE / 2NR-VE) - Index 19
    # 144 segments, 720 deg (5.0 deg/seg), CKP + CMP1 (only in Rev 2)
    w_new = entries[19]
    t.assert_equal("Toyota Avanza 1.5 Crank only", w_new["friendlyName"], "New Avanza friendly name")
    t.assert_equal("New Avanza", w_new["shortName"], "New Avanza short name")
    t.assert_equal(144, w_new["totalEdges"], "New Avanza total edges")
    t.assert_equal(720, w_new["cycleDegrees"], "New Avanza cycle degrees")
    arr_new = cpp_arrays[w_new["arrayName"]]["data"]
    arr_new_ref = ardustim_arrays["new_avanza"]
    t.assert_equal(arr_new_ref, arr_new, "New Avanza byte-for-byte fidelity with ArduStim")
    # Cam active ONLY in Rev 2 (segments 73 to 84 = 365 deg to 420 deg)
    cmp1_new_active = [idx for idx, v in enumerate(arr_new) if (v & 0x02)]
    t.assert_equal(12, len(cmp1_new_active), "New Avanza CAM active segments count")
    t.assert_true(all(73 <= x <= 84 for x in cmp1_new_active), "New Avanza CAM pulse strictly in Rev 2")
    t.assert_true(all(x >= 72 for x in cmp1_new_active), "New Avanza zero CAM pulses in Rev 1 (0-360 deg)")

    # 3.3 Toyota Rush / Terios / Xenia - Index 20
    # 144 segments, 720 deg, CKP + CMP1
    w_rush = entries[20]
    t.assert_equal("Toyota Avanza/Xenia/Terios/Rush ", w_rush["friendlyName"], "Rush/Terios friendly name (with trailing space preserved)")
    t.assert_equal("Avanza/Xenia/Terios/Rush", w_rush["shortName"], "Rush/Terios clean short name")
    t.assert_equal(144, w_rush["totalEdges"], "Rush/Terios total edges")
    t.assert_equal(720, w_rush["cycleDegrees"], "Rush/Terios cycle degrees")
    arr_rush = cpp_arrays[w_rush["arrayName"]]["data"]
    arr_rush_ref = ardustim_arrays["avanza_xenia_terios_rush"]
    t.assert_equal(arr_rush_ref, arr_rush, "Rush/Terios byte-for-byte fidelity with ArduStim")
    # Verify Rush Cam pulse groups
    cmp1_rush_active = [idx for idx, v in enumerate(arr_rush) if (v & 0x02)]
    t.assert_true(all(13 <= x <= 24 or 49 <= x <= 60 or 85 <= x <= 95 for x in cmp1_rush_active), "Rush/Terios CAM groups")

    # 3.4 Mitsubishi 4G63 (4/2 CAS Optical Disc) - Index 46
    # 144 segments, 720 deg, CKP + CMP1
    w_4g63 = entries[46]
    t.assert_equal("Mitsubishi 4g63 aka 4/2 crank and cam", w_4g63["friendlyName"], "4G63 friendly name")
    t.assert_equal("Mitsu 4G63 4/2", w_4g63["shortName"], "4G63 short name")
    t.assert_equal(144, w_4g63["totalEdges"], "4G63 total edges")
    t.assert_equal(720, w_4g63["cycleDegrees"], "4G63 cycle degrees")
    arr_4g63 = cpp_arrays[w_4g63["arrayName"]]["data"]
    arr_4g63_ref = ardustim_arrays["mitsubishi_4g63_4_2"]
    t.assert_equal(arr_4g63_ref, arr_4g63, "4G63 byte-for-byte fidelity with ArduStim")
    # Count CKP pulses (4 pulses over 720 deg)
    crank_edges_4g63 = 0
    prev_c = arr_4g63[-1] & 0x01
    for v in arr_4g63:
        curr_c = v & 0x01
        if curr_c and not prev_c:
            crank_edges_4g63 += 1
        prev_c = curr_c
    t.assert_equal(4, crank_edges_4g63, "4G63 has exactly 4 crank pulses over 720 deg cycle")

    # 3.5 Universal 60-2 (Bosch Motronic standard) - Index 3
    # 120 segments, 360 deg (3.0 deg/seg), 58 teeth (116 segments) + 2 missing teeth (4 segments)
    w_60_2 = entries[3]
    t.assert_equal("60-2 crank only", w_60_2["friendlyName"], "60-2 friendly name")
    t.assert_equal("60-2 KIA CKP Only", w_60_2["shortName"], "60-2 short name")
    t.assert_equal(120, w_60_2["totalEdges"], "60-2 total edges")
    t.assert_equal(360, w_60_2["cycleDegrees"], "60-2 cycle degrees")
    arr_60_2 = cpp_arrays[w_60_2["arrayName"]]["data"]
    arr_60_2_ref = ardustim_arrays["sixty_minus_two"]
    t.assert_equal(arr_60_2_ref, arr_60_2, "60-2 byte-for-byte fidelity with ArduStim")
    # Verify 58 teeth are strictly (1, 0) and missing gap is (0, 0, 0, 0)
    for tooth_idx in range(58):
        t.assert_equal(1, arr_60_2[tooth_idx * 2], f"60-2 tooth {tooth_idx} HIGH")
        t.assert_equal(0, arr_60_2[tooth_idx * 2 + 1], f"60-2 tooth {tooth_idx} LOW")
    t.assert_equal([0, 0, 0, 0], arr_60_2[116:120], "60-2 missing gap (2 teeth = 4 segments of LOW)")

    # 3.6 Universal 36-1 (Ford / Standard missing tooth) - Index 6
    # 72 segments, 360 deg (5.0 deg/seg), 35 teeth (70 segments) + 1 missing tooth (2 segments)
    w_36_1 = entries[6]
    t.assert_equal("36-1 crank only", w_36_1["friendlyName"], "36-1 friendly name")
    t.assert_equal("36-1 CKP Only", w_36_1["shortName"], "36-1 short name")
    t.assert_equal(72, w_36_1["totalEdges"], "36-1 total edges")
    t.assert_equal(360, w_36_1["cycleDegrees"], "36-1 cycle degrees")
    arr_36_1 = cpp_arrays[w_36_1["arrayName"]]["data"]
    arr_36_1_ref = ardustim_arrays["thirty_six_minus_one"]
    t.assert_equal(arr_36_1_ref, arr_36_1, "36-1 byte-for-byte fidelity with ArduStim")
    # Verify 35 teeth are strictly (1, 0) and missing gap is (0, 0)
    for tooth_idx in range(35):
        t.assert_equal(1, arr_36_1[tooth_idx * 2], f"36-1 tooth {tooth_idx} HIGH")
        t.assert_equal(0, arr_36_1[tooth_idx * 2 + 1], f"36-1 tooth {tooth_idx} LOW")
    t.assert_equal([0, 0], arr_36_1[70:72], "36-1 missing gap (1 tooth = 2 segments of LOW)")

    return t.print_summary()


if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
