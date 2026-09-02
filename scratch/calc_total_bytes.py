import json

with open(".agents/survey_spec_miner/parsed_wheels.json", "r", encoding="utf-8") as f:
    wheels = json.load(f)

total_bytes = sum(w["spec_edges"] for w in wheels)
print(f"Total array bytes: {total_bytes} bytes (~{total_bytes/1024:.2f} KB)")
