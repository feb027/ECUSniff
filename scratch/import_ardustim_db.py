import re
import os
import json

ARDUSTIM_INO = "external/ardustim-tftv2-touchscreen/ardustim/ardustim.ino"
WHEEL_DEFS_H = "external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h"

with open(ARDUSTIM_INO, "r", encoding="utf-8", errors="ignore") as f:
    ino_content = f.read()

with open(WHEEL_DEFS_H, "r", encoding="utf-8", errors="ignore") as f:
    defs_content = f.read()

friendly_names = {}
# Match both: const char var[] PROGMEM = "..." and const char var[] = "..."
name_matches = re.findall(r'const\s+char\s+(\w+)\s*\[\]\s*(?:PROGMEM\s*)?=\s*"([^"]+)";', defs_content)
for var_name, text in name_matches:
    friendly_names[var_name] = text.strip()

# Some friendly names in ardustim.ino use direct strings or pattern aliases
# E.g. nissan_livina_juke -> "Nissan Livina Juke crank and cam"
friendly_names["nissan_livina_juke"] = "Nissan Livina/Juke 1.5 HR15DE"

wheel_entries = []
pattern = re.compile(r'\{\s*(\w+)\s*,\s*(\w+)\s*,\s*([0-9.]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*\}')
for line in ino_content.split("\n"):
    m = pattern.search(line)
    if m:
        wheel_entries.append({
            "friendly_var": m.group(1),
            "array_var": m.group(2),
            "scaler": float(m.group(3)),
            "max_edges": int(m.group(4)),
            "degrees": int(m.group(5))
        })

print(f"Found {len(wheel_entries)} wheel entries in Wheels[]")

# Extract arrays from wheel_defs.h
arrays = {}
defs_no_comments = re.sub(r'/\*.*?\*/', '', defs_content, flags=re.DOTALL)
array_matches = re.finditer(r'const\s+unsigned\s+char\s+(\w+)\s*\[\]\s*(?:PROGMEM\s*)?=\s*(?:[^\n{]*\n\s*)*\{([^}]+)\};', defs_no_comments, re.DOTALL)
for m in array_matches:
    var_name = m.group(1)
    raw_vals = m.group(2)
    clean_vals = re.sub(r'//.*', '', raw_vals)
    clean_vals = clean_vals.replace('\n', ' ').replace('\r', ' ')
    tokens = [t.strip().rstrip('l').rstrip('L') for t in clean_vals.split(',') if t.strip()]
    numbers = []
    for t in tokens:
        if t.isdigit():
            numbers.append(int(t))
    arrays[var_name] = numbers

print(f"Extracted {len(arrays)} arrays from wheel_defs.h")

def clean_friendly_name(name, friendly):
    f = friendly.strip()
    if f.endswith(";"): f = f[:-1]
    # Clean redundant spaces
    f = re.sub(r'\s+', ' ', f)
    if "avanza 1.3" in f.lower(): return "Toyota Avanza 1.3 K3-VE"
    if "avanza 1.5" in f.lower(): return "Toyota Avanza 1.5 3SZ-VE"
    if "avanza/xenia" in f.lower() or "avanza_xenia" in name.lower(): return "Toyota Avanza/Xenia/Rush"
    if "livina" in f.lower() or "juke" in f.lower(): return "Nissan Livina/Juke HR15DE"
    if "jazz fit 04-08v2" in f.lower(): return "Honda Jazz/Fit L15A V2"
    if "jazz fit 04-08v3" in f.lower(): return "Honda Jazz/Fit L15A V3"
    if "jazz fit 04-08" in f.lower(): return "Honda Jazz/Fit GD3 L15A"
    return f

def categorize(name, friendly):
    low = (name + " " + friendly).lower()
    if "toyota" in low or "avanza" in low or "xenia" in low or "terios" in low or "rush" in low or "4age" in low or "4agze" in low or "2jz" in low or "1nz" in low or "daihatsu" in low:
        return "Toyota/Daihatsu"
    if "honda" in low or "jazz" in low or "fit" in low or "d17" in low or "rc51" in low or "suzuki" in low or "drz" in low:
        return "Honda/Suzuki"
    if "mitsubishi" in low or "4g63" in low or "6g72" in low or "3a92" in low or "nissan" in low or "livina" in low or "juke" in low or "cas" in low or "mazda" in low or "miata" in low or "323" in low or "fe3" in low or "subaru" in low or "boxer" in low:
        return "Mitsubishi/Nissan/Mazda"
    if "ford" in low or "edis" in low or "st170" in low or "gm" in low or "ls1" in low or "58x" in low or "optispark" in low or "7x" in low or "4200" in low or "oss" in low or "chrysler" in low or "ngc" in low or "jeep" in low or "viper" in low or "four_twenty_a" in low or "bmw" in low or "fiat" in low or "weber" in low or "volvo" in low or "audi" in low or "lotus" in low or "dsm" in low:
        return "Euro/Amerika"
    if "yamaha" in low or "buell" in low or "oddfire" in low or "r1" in low:
        return "Universal"
    return "Universal"

converted_db = []

for w in wheel_entries:
    arr_name = w["array_var"]
    raw_friendly = friendly_names.get(w["friendly_var"], arr_name.replace("_", " ").title())
    friendly_name = clean_friendly_name(arr_name, raw_friendly)
    deg = w["degrees"]
    data = arrays.get(arr_name, [])
    if not data:
        print(f"[WARN] Array {arr_name} not found in extracted arrays!")
        continue

    total_steps = len(data)
    deg_per_step = float(deg) / float(total_steps)

    ckp_rising_indices = []
    cmp_events = []
    
    last_ckp = 0
    last_cmp = 0
    
    for i in range(total_steps):
        val = data[i]
        ckp = 1 if (val & 1) else 0
        cmp1 = 1 if (val & 2) else 0
        angle = round(i * deg_per_step, 1)

        if ckp == 1 and last_ckp == 0:
            ckp_rising_indices.append(i)
        
        if cmp1 != last_cmp:
            cmp_angle = angle if deg == 720 else (angle * 2.0)
            cmp_events.append({"angle": round(cmp_angle, 1), "high": bool(cmp1 == 1)})
        
        last_ckp = ckp
        last_cmp = cmp1

    num_teeth = len(ckp_rising_indices)
    missing_teeth = 0
    
    if len(ckp_rising_indices) >= 2:
        diffs = []
        for k in range(len(ckp_rising_indices)):
            next_idx = ckp_rising_indices[(k + 1) % len(ckp_rising_indices)]
            if next_idx <= ckp_rising_indices[k]:
                step_diff = (total_steps - ckp_rising_indices[k]) + next_idx
            else:
                step_diff = next_idx - ckp_rising_indices[k]
            diffs.append(step_diff)
        
        min_diff = min(diffs)
        max_diff = max(diffs)
        if min_diff > 0 and (max_diff / min_diff) >= 1.6:
            missing_teeth = int(round(max_diff / min_diff) - 1)
            total_teeth_calc = num_teeth + missing_teeth
            if deg == 720:
                total_teeth_calc = int(round(total_teeth_calc / 2))
        else:
            total_teeth_calc = num_teeth if deg == 360 else int(round(num_teeth / 2))
    else:
        total_teeth_calc = num_teeth

    cat = categorize(arr_name, friendly_name)
    
    filtered_cmp = []
    for ev in cmp_events:
        if not filtered_cmp or filtered_cmp[-1]["high"] != ev["high"]:
            filtered_cmp.append(ev)

    converted_db.append({
        "id": arr_name.lower(),
        "name": friendly_name,
        "category": cat,
        "desc": f"{friendly_name} ({total_teeth_calc}-{missing_teeth}, {deg}°)",
        "ckp": {
            "totalTeeth": max(1, total_teeth_calc),
            "missingTeeth": missing_teeth,
            "missingPosition": 0,
            "dutyCycle": 0.5,
            "inverted": False
        },
        "cmp": filtered_cmp
    })

print(f"Successfully converted {len(converted_db)} wheel patterns.")

# 1. Output data/js/wheel_db.js
js_items = []
for item in converted_db:
    js_items.append("    " + json.dumps(item) + ",")
if js_items:
    js_items[-1] = js_items[-1].rstrip(",")

js_content = (
    "// Automotive Car Wheel & Camshaft Pattern Database (OEM Library)\n"
    f"// Auto-imported & synchronized with Ardustim Touchscreen database ({len(converted_db)} Patterns)\n\n"
    "const WHEEL_DATABASE = [\n" +
    "\n".join(js_items) +
    "\n];\n\n"
    "window.WHEEL_DATABASE = WHEEL_DATABASE;\n"
)

with open("data/js/wheel_db.js", "w", encoding="utf-8") as f:
    f.write(js_content)

print(f"Updated data/js/wheel_db.js successfully ({len(converted_db)} patterns)!")

# 2. Output lib/ui/include/wheel_database.h
cpp_lines = []
for item in converted_db:
    name = item["name"][:47]
    teeth = item["ckp"]["totalTeeth"]
    mTeeth = item["ckp"]["missingTeeth"]
    mPos = item["ckp"]["missingPosition"]
    duty = item["ckp"]["dutyCycle"]
    inv = "true" if item["ckp"]["inverted"] else "false"
    
    cmp_list = item["cmp"]
    cam_count = min(4, len(cmp_list))
    
    angles = [0.0, 0.0, 0.0, 0.0]
    highs = ["false", "false", "false", "false"]
    for i in range(cam_count):
        angles[i] = float(cmp_list[i]["angle"])
        highs[i] = "true" if cmp_list[i]["high"] else "false"
        
    angles_str = "{" + ", ".join(f"{a:.1f}f" for a in angles) + "}"
    highs_str = "{" + ", ".join(highs) + "}"
    
    cpp_lines.append(f'    {{ "{name}", {teeth}, {mTeeth}, {mPos}, {duty:.2f}f, {inv}, {cam_count}, {angles_str}, {highs_str} }},')

cpp_content = f"""#ifndef ECU_ENGINE_WHEEL_DATABASE_H
#define ECU_ENGINE_WHEEL_DATABASE_H

#include <stdint.h>
#include <stddef.h>
#include "page_dashboard.h"

namespace EcuUi {{

constexpr size_t OEM_DATABASE_COUNT = {len(converted_db)};

const WheelPresetItem OEM_DATABASE_PRESETS[OEM_DATABASE_COUNT] = {{
""" + "\n".join(cpp_lines) + f"""
}};

}} // namespace EcuUi

#endif // ECU_ENGINE_WHEEL_DATABASE_H
"""

with open("lib/ui/include/wheel_database.h", "w", encoding="utf-8") as f:
    f.write(cpp_content)

print(f"Updated lib/ui/include/wheel_database.h successfully ({len(converted_db)} presets, {len(cpp_content.splitlines())} lines)!")
