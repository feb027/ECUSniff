import re
import os
import json

ARDUSTIM_INO = "external/Ardu-Stim/ardustim/ardustim/ardustim.ino"
WHEEL_DEFS_H = "external/Ardu-Stim/ardustim/ardustim/wheel_defs.h"

with open(ARDUSTIM_INO, "r", encoding="utf-8", errors="ignore") as f:
    ino_content = f.read()

with open(WHEEL_DEFS_H, "r", encoding="utf-8", errors="ignore") as f:
    defs_content = f.read()

friendly_names = {}
name_matches = re.findall(r'const\s+char\s+(\w+)\s*\[\]\s*PROGMEM\s*=\s*"([^"]+)";', defs_content)
for var_name, text in name_matches:
    friendly_names[var_name] = text

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
# Clean multiline comments first
defs_no_comments = re.sub(r'/\*.*?\*/', '', defs_content, flags=re.DOTALL)
# Extract each const unsigned char var_name[] PROGMEM = { ... };
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

def categorize(name, friendly):
    low = (name + " " + friendly).lower()
    if "toyota" in low or "4age" in low or "4agze" in low or "2jz" in low or "1nz" in low:
        return "Toyota"
    if "honda" in low or "d17" in low:
        return "Honda"
    if "mitsubishi" in low or "4g63" in low or "6g72" in low or "3a92" in low:
        return "Mitsubishi"
    if "subaru" in low or "boxer" in low:
        return "Subaru"
    if "mazda" in low or "miata" in low or "323" in low or "fe3" in low:
        return "Mazda"
    if "nissan" in low or "cas" in low:
        return "Nissan"
    if "daihatsu" in low:
        return "Daihatsu"
    if "suzuki" in low or "drz" in low:
        return "Suzuki"
    if "ford" in low or "edis" in low or "st170" in low:
        return "Ford"
    if "gm" in low or "ls1" in low or "58x" in low or "optispark" in low or "7x" in low or "4200" in low or "oss" in low:
        return "General Motors"
    if "chrysler" in low or "ngc" in low or "jeep" in low or "viper" in low or "four_twenty_a" in low:
        return "Chrysler/Jeep"
    if "bmw" in low or "sixty_minus_two" in low:
        return "BMW / Bosch"
    if "fiat" in low or "weber" in low or "volvo" in low or "audi" in low or "lotus" in low:
        return "European OEM"
    if "yamaha" in low or "buell" in low or "rc51" in low or "oddfire" in low:
        return "Motorcycle"
    return "Universal"

converted_db = []

for w in wheel_entries:
    arr_name = w["array_var"]
    friendly_name = friendly_names.get(w["friendly_var"], arr_name.replace("_", " ").title())
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
            "totalTeeth": total_teeth_calc,
            "missingTeeth": missing_teeth,
            "missingPosition": 0,
            "dutyCycle": 0.5,
            "inverted": False
        },
        "cmp": filtered_cmp
    })

print(f"Successfully converted {len(converted_db)} wheel patterns.")

# Format compact JSON lines (1 line per item) to respect the <= 300 line architecture rule
js_items = []
for item in converted_db:
    js_items.append("    " + json.dumps(item) + ",")

# remove trailing comma on last item
if js_items:
    js_items[-1] = js_items[-1].rstrip(",")

js_content = (
    "// Automotive Car Wheel & Camshaft Pattern Database (OEM Library)\n"
    "// Auto-imported & synchronized with ArduStim database (64 Patterns)\n\n"
    "const WHEEL_DATABASE = [\n" +
    "\n".join(js_items) +
    "\n];\n\n"
    "window.WHEEL_DATABASE = WHEEL_DATABASE;\n"
)

with open("data/js/wheel_db.js", "w", encoding="utf-8") as f:
    f.write(js_content)

print(f"Updated data/js/wheel_db.js successfully ({len(js_content.splitlines())} lines)!")
