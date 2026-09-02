import json
from collections import Counter

with open('.agents/survey_spec_miner/parsed_wheels.json') as f:
    data = json.load(f)

val_counter = Counter()
pattern_vals = {}
for w in data:
    for v in w['distinct_vals']:
        val_counter[v] += 1
    pattern_vals[w['index']] = w['distinct_vals']

print('Distinct values usage across patterns:')
for v, count in sorted(val_counter.items()):
    binary_rep = f"0b{v:03b}"
    ckp = "HIGH" if (v & 1) else "LOW"
    cmp1 = "HIGH" if (v & 2) else "LOW"
    cmp2 = "HIGH" if (v & 4) else "LOW"
    print(f'  Value {v} ({binary_rep}): CKP={ckp}, CMP1={cmp1}, CMP2={cmp2} | used in {count}/70 patterns')

# Patterns with Cam2 (bit 2)
cam2_patterns = [w for w in data if w['has_cam2']]
print(f'\nPatterns with CAM2 (bit 2 / 0x04 set, total={len(cam2_patterns)}):')
for w in cam2_patterns:
    print(f"  [{w['index']:02d}] {w['enum_name']}: {w['friendly_name']} | Vals: {w['distinct_vals']}")

# Summary by degree length
deg_counter = Counter(w['degrees'] for w in data)
print(f'\nCycle lengths: {dict(deg_counter)}')

# Summary by pattern category
cat_counter = Counter(w['pattern_category'] for w in data)
print(f'\nCategories in ArduStim Manager: {dict(cat_counter)}')

# Summary by Brand
brand_counter = Counter(w['brand'] for w in data)
print(f'\nBrands: {dict(brand_counter)}')
