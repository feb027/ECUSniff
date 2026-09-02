import json

with open(".agents/survey_spec_miner/parsed_wheels.json", "r", encoding="utf-8") as f:
    wheels = json.load(f)

brands = {}
for w in wheels:
    b = w['brand']
    brands[b] = brands.get(b, 0) + 1

print("Brands in parsed_wheels.json:")
for b, count in sorted(brands.items()):
    print(f"  {b}: {count}")
