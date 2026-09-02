import json

with open(".agents/survey_spec_miner/parsed_wheels.json", "r", encoding="utf-8") as f:
    wheels = json.load(f)

for w in wheels:
    print(f"[{w['index']:2d}] ID: {w['index']:2d} | Enum: {w['enum_name']:<40} | Friendly: \"{w['friendly_name']}\" | TFT: \"{w['tft_name']}\" | Edges: {w['spec_edges']:4d} | Deg: {w['degrees']:3d} | Scaler: {w['rpm_scaler']}")
