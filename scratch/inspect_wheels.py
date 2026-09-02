import json

with open(".agents/survey_spec_miner/parsed_wheels.json", "r", encoding="utf-8") as f:
    wheels = json.load(f)

print(json.dumps(wheels[0], indent=2))
print(json.dumps(wheels[18], indent=2)) # OLD_AVANZA
