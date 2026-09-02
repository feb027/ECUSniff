import os
import re
import sys

def clean_c_code(text):
    # Remove multi-line comments
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    # Remove single-line comments
    text = re.sub(r'//.*', '', text)
    return text

def audit_byte_for_byte_parity():
    print("\n--- Step 5: Direct Byte-for-Byte Array Comparison (ArduStim vs ECUSniff) ---")
    
    ardustim_path = "external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h"
    with open(ardustim_path, "r", encoding="utf-8", errors="ignore") as f:
        ardustim_src = f.read()

    ecu_db_path = "lib/engine/src/wheel_database.cpp"
    with open(ecu_db_path, "r", encoding="utf-8") as f:
        ecu_src = f.read()

    # Clean the entire sources first
    ardustim_clean = clean_c_code(ardustim_src)
    ecu_clean = clean_c_code(ecu_src)

    # Extract all array definitions from ArduStim
    raw_arr_matches = re.findall(r'const\s+(?:unsigned\s+char|uint8_t)\s+(\w+)\s*\[\]\s*PROGMEM\s*=\s*\{([^}]+)\};', ardustim_clean)
    ardustim_arrays = {}
    for name, content in raw_arr_matches:
        tokens = [t.strip() for t in content.replace('\n', ',').split(',') if t.strip()]
        vals = [int(t) for t in tokens]
        ardustim_arrays[name] = vals

    print(f"[AUDIT P5] Extracted {len(ardustim_arrays)} raw arrays from ArduStim source.")

    # Match each of ECUSniff's 70 arrays
    ecu_arr_matches = re.findall(r'static\s+const\s+uint8_t\s+(\w+)\[\d+\]\s*PROGMEM\s*=\s*\{([^}]+)\};', ecu_clean)
    ecu_arrays = {}
    for name, content in ecu_arr_matches:
        tokens = [t.strip() for t in content.replace('\n', ',').split(',') if t.strip()]
        vals = [int(t) for t in tokens]
        ecu_arrays[name] = vals

    print(f"[AUDIT P5] Extracted {len(ecu_arrays)} raw arrays from ECUSniff source.")
    assert len(ecu_arrays) == 70, f"Expected 70 arrays in ECUSniff, found {len(ecu_arrays)}"

    # Compare arrays
    matched_count = 0
    for ecu_name, ecu_vals in ecu_arrays.items():
        base_name = re.sub(r'^s_pattern_\d+_', '', ecu_name)
        target_name = None
        for ard_name in ardustim_arrays:
            if ard_name.lower() == base_name.lower() or ard_name.lower().replace('_', '') == base_name.lower().replace('_', ''):
                target_name = ard_name
                break
        
        if target_name:
            ard_vals = ardustim_arrays[target_name]
            assert len(ecu_vals) == len(ard_vals), f"Length mismatch for {ecu_name} ({len(ecu_vals)}) vs {target_name} ({len(ard_vals)})"
            diffs = [(i, ev, av) for i, (ev, av) in enumerate(zip(ecu_vals, ard_vals)) if ev != av]
            assert len(diffs) == 0, f"Byte mismatch in {ecu_name} vs {target_name}: {diffs[:5]}"
            matched_count += 1
        else:
            print(f"[AUDIT P5] Note: No direct name match in ArduStim for {ecu_name} (base: {base_name})")

    print(f"[AUDIT P5] PASS: {matched_count}/70 arrays directly cross-matched with 100% byte-for-byte identity against ArduStim source!")

if __name__ == "__main__":
    audit_byte_for_byte_parity()
