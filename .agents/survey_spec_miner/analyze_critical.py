import json

with open('.agents/survey_spec_miner/parsed_wheels.json') as f:
    wheels = json.load(f)

with open('.agents/survey_spec_miner/analyze_patterns.py') as f:
    pass

import analyze_patterns

arrays = analyze_patterns.arrays_data

critical_enums = [
    "OLD_AVANZA",
    "NEW_AVANZA",
    "AVANZA_XENIA_TERIOS_RUSH",
    "MITSUBISH_4g63_4_2",
    "SIX_G_SEVENTY_TWO_WITH_CAM",
    "MITSUBISHI_3A92",
    "THIRTY_SIX_MINUS_TWO_TWO_TWO",
    "THIRTY_SIX_MINUS_TWO_TWO_TWO_H6",
    "THIRTY_SIX_MINUS_TWO_TWO_TWO_WITH_CAM",
    "MAZDA_THIRTY_SIX_MINUS_TWO_TWO_TWO_WITH_SIX_TOOTH_CAM",
    "HONDA_JAZZ_FIT_04_08",
    "HONDA_JAZZ_FIT_04_08V2",
    "HONDA_JAZZ_FIT_04_08V3",
    "HONDA_D17_NO_CAM",
    "HONDA_RC51_WITH_CAM",
    "NISSAN_LIVINA_JUKE",
    "THREE_SIXTY_NISSAN_CAS",
    "SIXTY_MINUS_TWO",
    "SIXTY_MINUS_TWO_WITH_CAM",
    "SIXTY_MINUS_TWO_WITH_HALFMOON_CAM",
    "THIRTY_SIX_MINUS_ONE",
    "THIRTY_SIX_MINUS_ONE_WITH_CAM_FE3",
    "THIRTY_SIX_MINUS_ONE_WITH_SECOND_TRIGGER",
    "THIRTY_SIX_MINUS_TWO_WITH_ONE_CAM",
    "TOYOTA_4AGE_CAS",
    "TOYOTA_4AGZE",
    "BMW_N20",
    "GM_LS1_CRANK_AND_CAM",
    "GM_58x_LS_CRANK_4X_CAM",
    "GM_7X",
    "OPTISPARK_LT1",
    "FOUR_TWENTY_A",
    "FORD_ST170",
    "SUBARU_SIX_SEVEN",
    "JEEP2000",
    "VIPER_96_02",
    "DAIHATSU_3CYL",
    "MIATA_9905",
    "MAZDA_323_AU",
    "GM_40_OSS"
]

print(f"Total critical patterns to analyze: {len(critical_enums)}")

for enum_name in critical_enums:
    w = next(item for item in wheels if item['enum_name'] == enum_name)
    arr = arrays[w['array_name']]
    deg_per_seg = w['degrees'] / len(arr)
    
    # Calculate pulse transitions
    ckp_edges = []
    cmp1_edges = []
    cmp2_edges = []
    
    prev_ckp = 0
    prev_cmp1 = 0
    prev_cmp2 = 0
    
    for i, val in enumerate(arr):
        c = val & 1
        m1 = (val & 2) >> 1
        m2 = (val & 4) >> 2
        
        angle = i * deg_per_seg
        if c != prev_ckp:
            ckp_edges.append((angle, "RISING" if c else "FALLING"))
            prev_ckp = c
        if m1 != prev_cmp1:
            cmp1_edges.append((angle, "RISING" if m1 else "FALLING"))
            prev_cmp1 = m1
        if m2 != prev_cmp2:
            cmp2_edges.append((angle, "RISING" if m2 else "FALLING"))
            prev_cmp2 = m2
            
    print(f"\n=======================================================")
    print(f"[{w['index']:02d}] {w['enum_name']} | \"{w['friendly_name']}\"")
    print(f"  Category: {w['pattern_category']} | Brand: {w['brand']}")
    print(f"  Cycle: {w['degrees']} deg | Segments: {len(arr)} | Deg/Seg: {deg_per_seg:.2f} deg | Scaler: {w['rpm_scaler']}")
    print(f"  Active signals: CKP={w['has_crank']} ({len(ckp_edges)} edges), CMP1={w['has_cam1']} ({len(cmp1_edges)} edges), CMP2={w['has_cam2']} ({len(cmp2_edges)} edges)")
    print(f"  Distinct values: {w['distinct_vals']}")
    print(f"  First 20 segments: {arr[:20]}")
    print(f"  Last 20 segments: {arr[-20:]}")
