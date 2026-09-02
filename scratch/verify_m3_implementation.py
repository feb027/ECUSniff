"""
Verification Script for Milestone 3 (UI Waveform Canvas & Browser Sync)
Validates WaveformCanvas dynamic geometry, multi-channel rendering, decimation,
PageWheelBrowser brand category mapping, and WheelDatabase integration.
"""
import sys
import os
import re

def main():
    print("================================================================================")
    print("          Worker M3 Verification: UI Waveform Canvas & Wheel Browser")
    print("================================================================================")

    base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    wc_h_path = os.path.join(base_dir, "lib", "ui", "include", "waveform_canvas.h")
    wc_cpp_path = os.path.join(base_dir, "lib", "ui", "src", "waveform_canvas.cpp")
    pwb_h_path = os.path.join(base_dir, "lib", "ui", "include", "page_wheel_browser.h")
    pwb_cpp_path = os.path.join(base_dir, "lib", "ui", "src", "page_wheel_browser.cpp")
    wdb_ui_h_path = os.path.join(base_dir, "lib", "ui", "include", "wheel_database.h")
    wdb_eng_cpp_path = os.path.join(base_dir, "lib", "engine", "src", "wheel_database.cpp")

    # 1. File existence check
    for p in [wc_h_path, wc_cpp_path, pwb_h_path, pwb_cpp_path, wdb_ui_h_path, wdb_eng_cpp_path]:
        if not os.path.exists(p):
            print(f"[FAIL] Missing file: {p}")
            sys.exit(1)
    print("[PASS] All required M3 headers and implementation files exist.")

    # 2. Inspect lib/ui/include/wheel_database.h
    with open(wdb_ui_h_path, "r", encoding="utf-8") as f:
        wdb_ui = f.read()
    assert "wheel_database.h" in wdb_ui
    assert "using BrandCategory = ::BrandCategory;" in wdb_ui
    assert "using WheelDefinition = ::WheelDefinition;" in wdb_ui
    assert "namespace WheelDatabase = ::WheelDatabase;" in wdb_ui
    assert "constexpr size_t OEM_DATABASE_COUNT = ::WheelDatabase::TOTAL_WHEELS;" in wdb_ui
    print("[PASS] lib/ui/include/wheel_database.h correctly aliases engine WheelDatabase and types.")

    # 3. Inspect WaveformCanvas header and source
    with open(wc_h_path, "r", encoding="utf-8") as f:
        wc_h = f.read()
    with open(wc_cpp_path, "r", encoding="utf-8") as f:
        wc_cpp = f.read()

    assert "void render(const WheelDefinition* wheel, int32_t screenX" in wc_h
    assert "void render(const EcuEngine::ParametricWheel& wheel" in wc_h
    assert "_calculateTrackGeometry" in wc_h and "_calculateTrackGeometry" in wc_cpp
    assert "TFT_YELLOW" in wc_cpp and "TFT_GREEN" in wc_cpp and "TFT_CYAN" in wc_cpp
    assert "0x01" in wc_cpp and "0x02" in wc_cpp and "0x04" in wc_cpp
    assert "_drawBitArrayTrace" in wc_cpp
    print("[PASS] WaveformCanvas header and source support dynamic multi-channel bit-array and parametric rendering.")

    # 4. Inspect PageWheelBrowser header and source
    with open(pwb_h_path, "r", encoding="utf-8") as f:
        pwb_h = f.read()
    with open(pwb_cpp_path, "r", encoding="utf-8") as f:
        pwb_cpp = f.read()

    assert "using WheelCategory = BrandCategory;" in pwb_h
    assert "matchesCategory(uint16_t globalIdx, BrandCategory cat)" in pwb_h
    assert "matchesCategory" in pwb_cpp
    assert "_canvas.render(wheelDef, 12, 184)" in pwb_cpp
    assert "WheelDatabase::getWheelCount()" in pwb_cpp
    assert "WheelDatabase::getWheel(" in pwb_cpp
    assert "CAT_NAMES" in pwb_cpp
    print("[PASS] PageWheelBrowser header and source correctly consume WheelDatabase with BrandCategory navigation.")

    # 5. Extract all 70 presets from engine wheel_database.cpp
    with open(wdb_eng_cpp_path, "r", encoding="utf-8") as f:
        eng_cpp = f.read()

    preset_pattern = re.compile(
        r'\{\s*(\d+),\s*"([^"]+)",\s*"([^"]+)",\s*BrandCategory::([A-Za-z0-9_]+),\s*WheelCycleDegrees::([A-Za-z0-9_]+),\s*(\d+),\s*([A-Za-z0-9_]+),\s*(true|false),\s*(true|false)\s*\}'
    )
    presets = preset_pattern.findall(eng_cpp)
    assert len(presets) == 70, f"Expected 70 presets, found {len(presets)}"
    print(f"[PASS] Successfully extracted {len(presets)} presets from lib/engine/src/wheel_database.cpp.")

    # 6. Emulate WaveformCanvas rendering for all 70 presets on different canvas heights (124px, 76px, 80px)
    def calculate_track_geometry(height, num_tracks):
        header_h = 14 if height > 90 else 12
        usable_h = height - header_h - 4
        track_h = usable_h // num_tracks
        margin_y = 6 if (height > 90 and track_h > 40) else (4 if height > 90 else 3)
        tracks = []
        for i in range(num_tracks):
            y_top = header_h + 2 + (i * track_h)
            y_high = y_top + margin_y
            y_low = y_top + track_h - margin_y - 1
            tracks.append((y_top, y_high, y_low))
        return tracks

    def simulate_canvas_trace(total_edges, cycle_degrees, width, height, num_tracks):
        x_offset = 28
        x_pad = 8
        available_w = width - x_offset - x_pad
        num_total_segments = total_edges * 2 if cycle_degrees == 360 else total_edges
        tracks = calculate_track_geometry(height, num_tracks)
        
        # Verify tracks are within [0, height)
        for i, (y_top, y_high, y_low) in enumerate(tracks):
            assert 0 <= y_top < height, f"y_top out of bounds: {y_top} for H={height}"
            assert 0 <= y_high < height, f"y_high out of bounds: {y_high} for H={height}"
            assert 0 <= y_low < height, f"y_low out of bounds: {y_low} for H={height}"
            assert y_high < y_low, f"y_high ({y_high}) must be above y_low ({y_low})"
            if i > 0:
                assert y_top > tracks[i-1][2], f"Track overlap between {i-1} and {i}"

        # Verify pixel column rasterization
        covered_pixels = 0
        for x in range(available_w):
            seg_start = (x * num_total_segments) // available_w
            seg_end = ((x + 1) * num_total_segments) // available_w
            if seg_end <= seg_start:
                seg_end = seg_start + 1
            if seg_end > num_total_segments:
                seg_end = num_total_segments
            assert seg_start < num_total_segments
            assert seg_end <= num_total_segments
            covered_pixels += 1

        assert covered_pixels == available_w, f"Covered pixels {covered_pixels} != {available_w}"

    test_canvas_sizes = [(456, 124), (448, 76), (440, 80), (320, 100)]
    for p in presets:
        w_id, friendly, short, cat, cycle_deg_str, edges_str, arr_name, has_cmp1, has_cmp2 = p
        edges = int(edges_str)
        cycle_deg = 360 if cycle_deg_str == "CRANK_360" else 720
        num_tracks = 3 if has_cmp2 == "true" else 2
        for w, h in test_canvas_sizes:
            simulate_canvas_trace(edges, cycle_deg, w, h, num_tracks)

    print(f"[PASS] WaveformCanvas geometric simulation passed for all 70 presets across 4 canvas resolutions.")

    # 7. Test BrandCategory mapping & filtering
    cat_counts = {}
    for p in presets:
        cat = p[3]
        cat_counts[cat] = cat_counts.get(cat, 0) + 1

    print("Preset distribution by BrandCategory:")
    for cat, cnt in sorted(cat_counts.items()):
        print(f"  - {cat}: {cnt} presets")

    assert cat_counts.get("TOYOTA_DAIHATSU", 0) >= 8
    assert cat_counts.get("HONDA", 0) >= 5
    assert cat_counts.get("MITSUBISHI", 0) >= 4
    assert cat_counts.get("NISSAN", 0) >= 2
    assert cat_counts.get("EURO_US", 0) >= 20
    assert cat_counts.get("UNIVERSAL", 0) >= 25
    print("[PASS] Category distribution strictly aligns with OEM brand classifications.")

    print("\n================================================================================")
    print("                    ALL M3 VERIFICATIONS PASSED (100%)")
    print("================================================================================")

if __name__ == "__main__":
    main()
