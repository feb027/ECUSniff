import json
import analyze_patterns

with open('.agents/survey_spec_miner/parsed_wheels.json') as f:
    wheels = json.load(f)

arrays = analyze_patterns.arrays_data

def get_pattern_type_desc(w, arr):
    fn = w['friendly_name'].lower()
    en = w['enum_name'].lower()
    edges = w['spec_edges']
    deg = w['degrees']
    vals = w['distinct_vals']
    has_cam = w['has_cam1'] or w['has_cam2']
    
    if "dizzy" in en or "distributor" in fn:
        return "Distributor / Even Pulse"
    if "optispark" in en or "cas" in en:
        return "Optical / Slotted CAS"
    if "36-2-2-2" in fn or "thirty_six_minus_two_two_two" in en:
        return "Multi-Gap (36-2-2-2)"
    if "avanza" in en:
        return "Arbitrary Bit-Array (Multi-Tooth CAM)"
    if "4g63" in en or "4/2" in fn:
        return "Unequal Tooth (4/2 CAS)"
    if "6g72" in en:
        return "Unequal Width Multi-Tooth"
    if "3a92" in en:
        return "Multi-Gap + Cam Pulses"
    if "ngc" in en:
        return "Variable Group Multi-Tooth (NGC)"
    if "ls1" in en:
        return "Unequal Crank + Half-Moon Cam"
    if "58x" in en or "60-2" in fn or "sixty_minus_two" in en:
        return "Missing Tooth (60-2 / 58X)" + (" + Cam" if has_cam else "")
    if "36-1" in fn or "thirty_six_minus_one" in en:
        return "Missing Tooth (36-1)" + (" + Cam" if has_cam else "")
    if "36-2" in fn or "thirty_six_minus_two" in en:
        return "Missing Tooth (36-2)" + (" + Cam" if has_cam else "")
    if "24-1" in fn or "twenty_four_minus_one" in en:
        return "Missing Tooth (24-1)"
    if "24-2" in fn or "twenty_four_minus_two" in en:
        return "Missing Tooth (24-2) + Outer Trigger"
    if "12-1" in fn or "twelve_minus_one" in en:
        return "Missing Tooth (12-1) + Cam"
    if "12-3" in fn:
        return "Missing Tooth (12-3)"
    if "40-1" in fn:
        return "Missing Tooth (40-1)"
    if "8-1" in fn:
        return "Missing Tooth (8-1)"
    if "4-1" in fn:
        return "Missing Tooth (4-1) + Cam"
    if "6-1" in fn:
        return "Missing Tooth (6-1) + Cam"
    if "oddfire" in en:
        return "Odd-Fire Unequal Angle"
    if "oss" in en:
        return "Even Tooth Transmission OSS"
    if "135" in en:
        return "High-Tooth Flywheel (135T) + Cam"
    if "d17" in en:
        return "Multi-Tooth (12+1) Crank"
    if "jazz" in en:
        return "Arbitrary Bit-Array (12+1 + Multi-Cam)"
    if "n20" in en:
        return "Missing Tooth (60-2) + Dual CAM (Intake+Exhaust)"
    if "viper" in en:
        return "Unequal Pairs Crank + Half-Moon Cam"
    if "subaru" in en:
        return "Subaru 6/7 Unequal Cam/Crank"
    if "7x" in en:
        return "GM 7X (6 Even + 1 Extra)"
    if "4200" in en:
        return "GM 4200 Variable Duration Crank"
    if "420a" in en or "four_twenty_a" in en:
        return "DSM 420A Multi-Tooth Dual-Pulse"
    if "st170" in en:
        return "Ford ST170 Variable Crank/Cam"
    if "taruna" in fn or "daihatsu_3cyl" in en:
        return "Daihatsu 3+1 Distributor"
    if "miata" in en:
        return "Miata 99-05 (2-Tooth Crank + 1/2 Cam)"
    if "323" in en:
        return "Mazda 323 AU Multi-Tooth"
    if "drz400" in en:
        return "Suzuki DRZ400 (6 coil, 2 crank pulses)"
    if "jeep" in en:
        return "Jeep 2000 4.0L Variable Tooth"
    if "rc51" in en:
        return "Honda RC51 90° V-Twin Oddfire + Cam"
    if "livina" in en:
        return "Nissan Livina/Juke Variable Width Pulses"
    if "iaw" in en:
        return "Weber-Marelli 8+2"
    if "fiat" in en:
        return "Fiat 1.8 16V Variable Crank/Cam"
    if "volvo" in en:
        return "Volvo D12 (17-1-17-1-17-1) Diesel"
    if "4age" in en:
        return "Toyota 4A-GE 4-Crank + 1-Cam CAS"
    if "4agze" in en:
        return "Toyota 4A-GZE 24-Crank + 1-Cam CAS"
    if "twelve_with_cam" in en:
        return "12 Even Crank + 1 Cam"
    if "twenty_four_with_cam" in en:
        return "24 Even Crank + 1 Cam"
    if "eight_tooth" in en or "four_tooth" in en or "six_tooth" in en:
        return "Even Tooth Crank + Half-Moon/1-Tooth Cam"
    
    return "Arbitrary Bit-Array"

output_md = []
output_md.append("| # | Enum Identifier | Friendly Name (`wheel_defs.h`) | TFT Name (`WheelPatternManager.cpp`) | Category / Brand | Pattern Architecture | Segments ($E$) | Cycle ($D$) | Deg/Seg | RPM Scaler | Signal Channels | Distinct Bitmasks |")
output_md.append("|---|---|---|---|---|---|---|---|---|---|---|---|")

for w in wheels:
    arr = arrays[w['array_name']]
    deg_per_seg = w['degrees'] / len(arr)
    arch = get_pattern_type_desc(w, arr)
    channels = []
    if w['has_crank']: channels.append("CKP")
    if w['has_cam1']: channels.append("CMP1")
    if w['has_cam2']: channels.append("CMP2")
    chan_str = "+".join(channels) if channels else "None"
    
    vals_str = ", ".join(str(v) for v in w['distinct_vals'])
    
    row = f"| {w['index']} | `{w['enum_name']}` | `{w['friendly_name']}` | `{w['tft_name']}` | {w['brand']} | {arch} | {w['spec_edges']} | {w['degrees']}° | {deg_per_seg:.2f}° | {w['rpm_scaler']} | {chan_str} | `[{vals_str}]` |"
    output_md.append(row)

with open('.agents/survey_spec_miner/all_70_table.md', 'w', encoding='utf-8') as f:
    f.write("\n".join(output_md))

print("Generated .agents/survey_spec_miner/all_70_table.md successfully.")
