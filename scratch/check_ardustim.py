import re

with open("external/ardustim-tftv2-touchscreen/ardustim/wheel_defs.h", "r", encoding="utf-8") as f:
    text = f.read()

# Find all arrays
array_matches = list(re.finditer(r'const\s+uint8_t\s+([a-zA-Z0-9_]+)\[(\d+)?\]\s*PROGMEM\s*=\s*\{([^}]+)\};', text))
print(f"Total arrays in wheel_defs.h: {len(array_matches)}")

# Let's inspect enum Wheels from wheel_defs.h or ardustim.ino
with open("external/ardustim-tftv2-touchscreen/ardustim/ardustim.ino", "r", encoding="utf-8") as f:
    ino_text = f.read()

m = re.search(r'const\s+wheel_t\s+Wheels\[MAX_WHEELS\]\s*PROGMEM\s*=\s*\{([^;]+)\};', ino_text, re.DOTALL)
if m:
    wheels_block = m.group(1)
    # Parse items
    # e.g. {dizzy_four_cylinder, 4, 360, 0.03333, "4 cylinder dizzy"},
    items = re.findall(r'\{\s*([a-zA-Z0-9_]+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([0-9.]+)\s*,\s*"([^"]+)"\s*\}', wheels_block)
    print(f"Total Wheels in Wheels[]: {len(items)}")
    for i, item in enumerate(items):
        print(f"{i:2d}: arr={item[0]:<45} edges={item[1]:<5} deg={item[2]:<4} scaler={item[3]:<7} name='{item[4]}'")
