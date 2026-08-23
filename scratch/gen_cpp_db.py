import re
import os
import json

with open("scratch/import_ardustim_db.py", "r", encoding="utf-8") as f:
    pass

# We already have converted_db in scratch/import_ardustim_db.py
# Let's generate lib/engine/include/wheel_database.h

with open("data/js/wheel_db.js", "r", encoding="utf-8") as f:
    js_text = f.read()

# Extract json array
m = re.search(r'const\s+WHEEL_DATABASE\s*=\s*(\[.*?\]);', js_text, re.DOTALL)
if not m:
    print("[ERROR] Failed to parse WHEEL_DATABASE from wheel_db.js")
    exit(1)

db = json.loads(m.group(1))
print(f"Loaded {len(db)} presets from wheel_db.js")

header_lines = [
    "#ifndef ECU_ENGINE_WHEEL_DATABASE_H",
    "#define ECU_ENGINE_WHEEL_DATABASE_H",
    "",
    "#include <stdint.h>",
    "#include <stddef.h>",
    "#include \"page_dashboard.h\"",
    "",
    "namespace EcuUi {",
    "",
    f"constexpr size_t OEM_DATABASE_COUNT = {len(db)};",
    "",
    "const WheelPresetItem OEM_DATABASE_PRESETS[OEM_DATABASE_COUNT] = {"
]

for item in db:
    name = item["name"][:23]
    ckp = item["ckp"]
    cmp_list = item["cmp"][:4]
    
    tot = ckp["totalTeeth"]
    mis = ckp["missingTeeth"]
    pos = ckp.get("missingPosition", 0)
    duty = ckp.get("dutyCycle", 0.5)
    inv = "true" if ckp.get("inverted", False) else "false"
    
    cam_cnt = len(cmp_list)
    angles = [0.0, 0.0, 0.0, 0.0]
    highs = ["false", "false", "false", "false"]
    for i, c in enumerate(cmp_list):
        angles[i] = float(c["angle"])
        highs[i] = "true" if c["high"] else "false"
        
    angles_str = "{" + ", ".join(f"{a:.1f}f" for a in angles) + "}"
    highs_str = "{" + ", ".join(highs) + "}"
    
    line = f'    {{ "{name}", {tot}, {mis}, {pos}, {duty:.2f}f, {inv}, {cam_cnt}, {angles_str}, {highs_str} }},'
    header_lines.append(line)

header_lines.append("};")
header_lines.append("")
header_lines.append("} // namespace EcuUi")
header_lines.append("")
header_lines.append("#endif // ECU_ENGINE_WHEEL_DATABASE_H")
header_lines.append("")

output_header = "\n".join(header_lines)
with open("lib/ui/include/wheel_database.h", "w", encoding="utf-8") as f:
    f.write(output_header)

print(f"Generated lib/ui/include/wheel_database.h ({len(output_header.splitlines())} lines) successfully!")
