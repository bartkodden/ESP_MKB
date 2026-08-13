#!/usr/bin/env python3
import sys
import io

# Force UTF-8 output — ESP-IDF Python env on Windows defaults to cp1252
if sys.stdout.encoding != 'utf-8':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
if sys.stderr.encoding != 'utf-8':
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')
"""
gen_icons.py — Material Icons font pipeline for ESP_MKB
"""

import subprocess
import sys
import os
import argparse
import json
import re
from pathlib import Path

SCRIPT_DIR   = Path(__file__).parent.resolve()
MAIN_DIR     = SCRIPT_DIR.parent
ICONS_DIR    = MAIN_DIR / "data" / "icons"
CP_FILE      = SCRIPT_DIR / "MaterialIcons-Regular.codepoints"
TTF_FILE     = SCRIPT_DIR / "MaterialIcons-Regular.ttf"
HEADER_OUT   = MAIN_DIR / "icon_codepoints.h"


def check_prerequisites():
    import shutil
    errors = []
    if not TTF_FILE.exists():
        errors.append(f"  MISSING: {TTF_FILE}")
        errors.append("  → Download from: https://github.com/google/material-design-icons/raw/master/font/MaterialIcons-Regular.ttf")
    if not CP_FILE.exists():
        errors.append(f"  MISSING: {CP_FILE}")
        errors.append("  → Download from: https://github.com/google/material-design-icons/raw/master/font/MaterialIcons-Regular.codepoints")

    cmd = shutil.which("lv_font_conv")
    if cmd:
        print(f"  ✓  Found lv_font_conv: {cmd}")
        lv_cmd = [cmd]
    else:
        npx = shutil.which("npx")
        if npx:
            result = subprocess.run([npx, "lv_font_conv", "--version"], capture_output=True, text=True)
            if result.returncode == 0:
                print(f"  ℹ  Using npx lv_font_conv")
                lv_cmd = [npx, "lv_font_conv"]
            else:
                lv_cmd = None
        else:
            lv_cmd = None

    if lv_cmd is None:
        errors.append("  MISSING: lv_font_conv")
        errors.append("  → Run: npm install -g lv_font_conv")

    if errors:
        print("\n❌ Prerequisites missing:\n")
        for e in errors: print(e)
        print()
        sys.exit(1)

    return lv_cmd


def parse_codepoints():
    """Parse full codepoints file → sorted list of (name, int)."""
    icons = []
    with open(CP_FILE, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) != 2:
                continue
            name, cp_hex = parts
            try:
                icons.append((name, int(cp_hex, 16)))
            except ValueError:
                pass
    icons.sort(key=lambda x: x[0])
    print(f"  ✓  Parsed {len(icons)} icons from codepoints file")
    return icons


def find_used_icons(icons):
    """
    Scan buttons.json and menu.json for mi: prefixed icon names.
    Returns a filtered list of (name, codepoint) for only used icons.
    """
    cp_map = {name: cp for name, cp in icons}
    used_names = set()

    # Files to scan — add more if you add more JSON config files later
    scan_files = [
        MAIN_DIR / "buttons.json",
        MAIN_DIR / "menu.json",
        MAIN_DIR / "data" / "buttons.json",  # also check LittleFS copy if present
        MAIN_DIR / "data" / "menu.json",
    ]

    for json_file in scan_files:
        if not json_file.exists():
            continue
        try:
            text = json_file.read_text(encoding="utf-8")
            # Find all "mi:icon_name" occurrences
            matches = re.findall(r'"mi:([a-z0-9_]+)"', text)
            found = set(matches)
            used_names.update(found)
            print(f"  ✓  {json_file.name}: found {len(found)} mi: icons → {sorted(found)}")
        except Exception as e:
            print(f"  ⚠  Could not scan {json_file.name}: {e}")

    if not used_names:
        print("  ⚠  No mi: icons found in any JSON file — falling back to full set")
        return icons

    # Validate and filter
    subset = []
    missing = []
    for name in sorted(used_names):
        if name in cp_map:
            subset.append((name, cp_map[name]))
        else:
            missing.append(name)

    if missing:
        print(f"\n  ⚠  Unknown icon names (typo in buttons.json?):")
        for m in missing:
            print(f"     mi:{m}")

    print(f"\n  ✓  Subset: {len(subset)} unique icons used across all button sets")
    return subset


def build_range_string(icons):
    """Build compact range string for lv_font_conv."""
    codepoints = sorted(set(cp for _, cp in icons))
    ranges = []
    start = codepoints[0]
    end   = codepoints[0]
    for cp in codepoints[1:]:
        if cp == end + 1:
            end = cp
        else:
            ranges.append(f"0x{start:x}" if start == end else f"0x{start:x}-0x{end:x}")
            start = end = cp
    ranges.append(f"0x{start:x}" if start == end else f"0x{start:x}-0x{end:x}")
    return ",".join(ranges)


def generate_font(icons, size, bpp, lv_cmd, subset_mode):
    ICONS_DIR.mkdir(parents=True, exist_ok=True)
    suffix = "_used" if subset_mode else ""

    # Generate as .c array (LVGL 8/9 compatible) AND as .bin (web upload)
    output_c   = MAIN_DIR / f"material_icons{suffix}_{size}.c"
    output_bin = ICONS_DIR / f"material_icons{suffix}_{size}.bin"

    range_str = build_range_string(icons)
    base_cmd = lv_cmd + [
        "--font",   str(TTF_FILE),
        "--range",  range_str,
        "--size",   str(size),
        "--bpp",    str(bpp),
    ]

    # C array — goes into firmware, version-safe
    cmd_c = base_cmd + ["--format", "lvgl", "--no-compress",
                    "--lv-font-name", f"material_icons_used_{size}",
                    "--lv-include",   "lvgl.h",
                    "-o", str(output_c)]
    print(f"\n  Generating C array ({output_c.name})...")
    result = subprocess.run(cmd_c, capture_output=True, text=True)
    if result.returncode != 0:
        # Print full error so CMake shows it in build output
        print(f"\n❌ C array generation failed:\n{result.stderr}", file=sys.stderr)
        sys.exit(1)
    print(f"  ✓  {output_c.name} ({output_c.stat().st_size/1024:.1f} KB)")

    # Binary — keep generating for future web upload use
    cmd_bin = base_cmd + ["--format", "bin", "--no-compress", "-o", str(output_bin)]
    print(f"  Generating binary ({output_bin.name})...")
    result = subprocess.run(cmd_bin, capture_output=True, text=True)
    if result.returncode == 0:
        print(f"  ✓  {output_bin.name} ({output_bin.stat().st_size/1024:.1f} KB)")
    else:
        print(f"  ⚠  Binary generation failed (non-fatal): {result.stderr[:100]}")

    return output_c, output_bin


def generate_header(icons, size):
    lines = [
        "// icon_codepoints.h",
        "// AUTO-GENERATED by scripts/gen_icons.py — DO NOT EDIT",
        f"// Material Icons — {len(icons)} icons at {size}px",
        "",
        "#pragma once",
        "#include <stdint.h>",
        "#include <string.h>",
        "",
        f"#define MATERIAL_ICONS_FONT_SIZE {size}",
        f"#define MATERIAL_ICONS_COUNT     {len(icons)}",
        "",
        "typedef struct { const char *name; uint32_t codepoint; } mi_icon_t;",
        "",
        "static const mi_icon_t k_material_icons[] = {",
    ]
    for name, cp in icons:
        lines.append(f'    {{"{name}", 0x{cp:x}}},')
    lines += [
        "};",
        "",
        "static inline uint32_t icon_lookup_codepoint(const char *name) {",
        "    int lo = 0, hi = MATERIAL_ICONS_COUNT - 1;",
        "    while (lo <= hi) {",
        "        int mid = (lo + hi) / 2;",
        "        int cmp = strcmp(name, k_material_icons[mid].name);",
        "        if (cmp == 0) return k_material_icons[mid].codepoint;",
        "        if (cmp  < 0) hi = mid - 1;",
        "        else          lo = mid + 1;",
        "    }",
        "    return 0;",
        "}",
        "",
        "static inline int icon_cp_to_utf8(uint32_t cp, char out[5]) {",
        "    if (cp < 0x10000) {",
        "        out[0] = (char)(0xE0 | (cp >> 12));",
        "        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));",
        "        out[2] = (char)(0x80 | (cp & 0x3F));",
        "        out[3] = '\\0';",
        "        return 3;",
        "    }",
        "    out[0] = (char)(0xF0 | (cp >> 18));",
        "    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));",
        "    out[2] = (char)(0x80 | ((cp >> 6)  & 0x3F));",
        "    out[3] = (char)(0x80 | (cp & 0x3F));",
        "    out[4] = '\\0';",
        "    return 4;",
        "}",
    ]
    with open(HEADER_OUT, "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    size_kb = HEADER_OUT.stat().st_size / 1024
    print(f"  ✓  Generated: {HEADER_OUT.name} ({size_kb:.1f} KB)")


def generate_js_list(icons):
    js_out = MAIN_DIR / "data" / "mi_icons.js"
    js_out.parent.mkdir(parents=True, exist_ok=True)
    entries = [[name, cp] for name, cp in icons]
    json_str = json.dumps(entries, separators=(',', ':'))
    with open(js_out, "w", newline="\n") as f:
        f.write(f"const MI_ICONS={json_str};\n")
    size_kb = js_out.stat().st_size / 1024
    print(f"  ✓  Generated: {js_out.name} ({size_kb:.1f} KB)")


def main():
    parser = argparse.ArgumentParser(description="Generate Material Icons font for ESP_MKB")
    parser.add_argument("--size",      type=int, default=24)
    parser.add_argument("--bpp",       type=int, default=4)
    parser.add_argument("--subset",    action="store_true", default=True,
                        help="Only include icons used in buttons.json/menu.json (default: on)")
    parser.add_argument("--full",      action="store_true",
                        help="Include all 2235 icons (overrides --subset)")
    parser.add_argument("--no-font",   action="store_true")
    parser.add_argument("--no-header", action="store_true")
    args = parser.parse_args()

    subset_mode = not args.full   # subset is default unless --full is passed

    print("\n╔══════════════════════════════════════════════╗")
    print("║   ESP_MKB — Material Icons Font Pipeline     ║")
    mode_label = "SUBSET (used icons only)" if subset_mode else "FULL (all 2235 icons)"
    print(f"║   Mode: {mode_label:<37}║")
    print("╚══════════════════════════════════════════════╝\n")

    print("① Checking prerequisites...")
    lv_cmd = check_prerequisites()
    print("  ✓  All prerequisites found\n")

    print("② Parsing codepoints...")
    all_icons = parse_codepoints()

    if subset_mode:
        print("\n③ Scanning JSON files for used icons...")
        icons = find_used_icons(all_icons)
    else:
        icons = all_icons

    if not args.no_font:
        print(f"\n④ Generating LVGL binary font ({args.size}px, {args.bpp}bpp)...")
        if not subset_mode:
            print("  ⏳ Full set — this may take 30–90 seconds...")
        generate_font(icons, args.size, args.bpp, lv_cmd, subset_mode)
    else:
        print("\n④ Skipping font generation (--no-font)")

    if not args.no_header:
        print("\n⑤ Generating icon_codepoints.h...")
        # Header always uses full set for lookup (subset would break future icon additions)
        generate_header(all_icons, args.size)
    else:
        print("\n⑤ Skipping header generation (--no-header)")

    print("\n⑥ Generating mi_icons.js for web editor...")
    generate_js_list(all_icons)   # web editor always shows full set

    print("\n✅ Done!\n")


if __name__ == "__main__":
    main()