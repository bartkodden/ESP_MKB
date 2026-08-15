#!/usr/bin/env python3
"""
gen_icons.py -- Material Icons font pipeline for ESP_MKB
Generates:
  - material_icons_used_24.c        (compiled into firmware)
  - data/icons/material_icons_used_24.bin  (for web upload / JS subsetter)
  - icon_codepoints.h               (name->codepoint lookup table)
  - data/mi_icons.js                (web editor icon browser)

Usage:
  python gen_icons.py               # subset mode (default)
  python gen_icons.py --full        # all 2235 icons
  python gen_icons.py --size 20     # different size
  python gen_icons.py --no-font     # skip font, only regenerate header + JS
"""
import sys
import io

if sys.stdout.encoding != 'utf-8':
    try:
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    except AttributeError:
        pass
if sys.stderr.encoding != 'utf-8':
    try:
        sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')
    except AttributeError:
        pass

import subprocess
import argparse
import json
import re
import shutil
from pathlib import Path

# ── Paths ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR = Path(__file__).parent.resolve()
MAIN_DIR   = SCRIPT_DIR.parent                        # firmware/main/
ICONS_DIR  = MAIN_DIR / "data" / "icons"
CP_FILE    = SCRIPT_DIR / "MaterialIcons-Regular.codepoints"
TTF_FILE   = SCRIPT_DIR / "MaterialIcons-Regular.ttf"
HEADER_OUT = MAIN_DIR / "icon_codepoints.h"
JS_OUT     = MAIN_DIR / "data" / "mi_icons.js"

# Files to scan for mi: icon references
SCAN_FILES = [
    MAIN_DIR / "buttons.json",
    MAIN_DIR / "menu.json",
    MAIN_DIR / "data" / "buttons.json",
    MAIN_DIR / "data" / "menu.json",
]


# ── Prerequisites ─────────────────────────────────────────────────────────────
def find_lv_font_conv():
    cmd = shutil.which("lv_font_conv")
    if cmd:
        print(f"  OK  lv_font_conv: {cmd}")
        return [cmd]
    npx = shutil.which("npx")
    if npx:
        r = subprocess.run([npx, "lv_font_conv", "--version"],
                           capture_output=True, text=True)
        if r.returncode == 0:
            print("  OK  lv_font_conv via npx")
            return [npx, "lv_font_conv"]
    return None


def check_prerequisites():
    errors = []
    if not TTF_FILE.exists():
        errors.append(f"  MISSING: {TTF_FILE}")
        errors.append("  -> Download: https://github.com/google/material-design-icons"
                      "/raw/master/font/MaterialIcons-Regular.ttf")
    if not CP_FILE.exists():
        errors.append(f"  MISSING: {CP_FILE}")
        errors.append("  -> Download: https://github.com/google/material-design-icons"
                      "/raw/master/font/MaterialIcons-Regular.codepoints")

    lv_cmd = find_lv_font_conv()
    if lv_cmd is None:
        errors.append("  MISSING: lv_font_conv")
        errors.append("  -> Run: npm install -g lv_font_conv")

    if errors:
        print("\nERROR: Prerequisites missing:\n")
        for e in errors:
            print(e)
        sys.exit(1)

    return lv_cmd


# ── Codepoint parsing ─────────────────────────────────────────────────────────
def parse_codepoints():
    """Parse MaterialIcons-Regular.codepoints -> sorted [(name, int)]."""
    icons = []
    with open(CP_FILE, "r", encoding="utf-8") as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) != 2:
                print(f"  WARN  Line {line_num} skipped: {line!r}")
                continue
            name, cp_hex = parts
            try:
                icons.append((name, int(cp_hex, 16)))
            except ValueError:
                print(f"  WARN  Line {line_num} bad codepoint: {line!r}")

    icons.sort(key=lambda x: x[0])
    print(f"  OK  Parsed {len(icons)} icons from codepoints file")
    return icons


# ── Subset detection ──────────────────────────────────────────────────────────
def find_used_icons(all_icons):
    """Scan JSON files for mi: references -> filtered [(name, int)]."""
    cp_map     = {name: cp for name, cp in all_icons}
    used_names = set()

    for json_file in SCAN_FILES:
        if not json_file.exists():
            continue
        try:
            text    = json_file.read_text(encoding="utf-8")
            matches = set(re.findall(r'"mi:([a-z0-9_]+)"', text))
            used_names.update(matches)
            print(f"  OK  {json_file.name}: {len(matches)} mi: icons found")
            if matches:
                print(f"      {sorted(matches)}")
        except Exception as exc:
            print(f"  WARN  Could not scan {json_file.name}: {exc}")

    if not used_names:
        print("  WARN  No mi: icons found -- falling back to full set")
        return all_icons

    subset  = []
    missing = []
    for name in sorted(used_names):
        if name in cp_map:
            subset.append((name, cp_map[name]))
        else:
            missing.append(name)

    if missing:
        print(f"\n  WARN  Unknown icon names (check for typos in JSON):")
        for m in missing:
            print(f"        mi:{m}")

    print(f"\n  OK  Subset: {len(subset)} unique icons across all sets")
    return subset


# ── Range string ──────────────────────────────────────────────────────────────
def build_range_string(icons):
    """Compress codepoints into ranges: 0xe037,0xe045-0xe050,..."""
    codepoints = sorted(set(cp for _, cp in icons))
    ranges     = []
    start = end = codepoints[0]

    for cp in codepoints[1:]:
        if cp == end + 1:
            end = cp
        else:
            ranges.append(f"0x{start:x}" if start == end
                          else f"0x{start:x}-0x{end:x}")
            start = end = cp
    ranges.append(f"0x{start:x}" if start == end
                  else f"0x{start:x}-0x{end:x}")

    return ",".join(ranges)


# ── Font generation ───────────────────────────────────────────────────────────
def generate_font(icons, size, bpp, lv_cmd, subset_mode):
    ICONS_DIR.mkdir(parents=True, exist_ok=True)
    suffix     = "_used" if subset_mode else ""
    font_name  = f"material_icons{suffix}_{size}"
    output_c   = MAIN_DIR   / f"{font_name}.c"
    output_bin = ICONS_DIR  / f"{font_name}.bin"

    range_str = build_range_string(icons)
    base_cmd  = lv_cmd + [
        "--font",  str(TTF_FILE),
        "--range", range_str,
        "--size",  str(size),
        "--bpp",   str(bpp),
    ]

    # ── C array (compiled into firmware) ─────────────────────────────────────
    cmd_c = base_cmd + [
        "--format",       "lvgl",
        "--no-compress",
        "--lv-font-name", font_name,
        "--lv-include",   "lvgl.h",
        "-o",             str(output_c),
    ]
    print(f"\n  Generating C array: {output_c.name}")
    r = subprocess.run(cmd_c, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"\nERROR: C array generation failed:\n{r.stderr}", file=sys.stderr)
        sys.exit(1)
    print(f"  OK  {output_c.name}  ({output_c.stat().st_size / 1024:.1f} KB)")

    # ── Binary (for web editor / JS subsetter) ────────────────────────────────
    cmd_bin = base_cmd + [
        "--format",      "bin",
        "--no-compress",
        "-o",            str(output_bin),
    ]
    print(f"  Generating binary: {output_bin.name}")
    r = subprocess.run(cmd_bin, capture_output=True, text=True)
    if r.returncode == 0:
        print(f"  OK  {output_bin.name}  ({output_bin.stat().st_size / 1024:.1f} KB)")
    else:
        print(f"  WARN  Binary generation failed (non-fatal):")
        print(f"        {r.stderr[:200]}")

    return output_c, output_bin


# ── Header generation ─────────────────────────────────────────────────────────
def generate_header(all_icons, size):
    """
    Generate icon_codepoints.h with the FULL icon set for name->codepoint
    lookup. Always uses the full set so future icons can be referenced without
    regenerating the header.
    """
    lines = [
        "// icon_codepoints.h",
        "// AUTO-GENERATED by scripts/gen_icons.py -- DO NOT EDIT",
        f"// Material Icons -- {len(all_icons)} icons at {size}px",
        "// Regenerate: python scripts/gen_icons.py",
        "",
        "#pragma once",
        "#include <stdint.h>",
        "#include <string.h>",
        "",
        f"#define MATERIAL_ICONS_FONT_SIZE {size}",
        f"#define MATERIAL_ICONS_COUNT     {len(all_icons)}",
        "",
        "typedef struct {",
        "    const char  *name;",
        "    uint32_t     codepoint;",
        "} mi_icon_t;",
        "",
        "// Sorted alphabetically for binary search",
        "static const mi_icon_t k_material_icons[] = {",
    ]

    for name, cp in all_icons:
        lines.append(f'    {{"{name}", 0x{cp:x}}},')

    lines += [
        "};",
        "",
        "// Binary search -- returns 0 if not found",
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
        "// Encode codepoint to UTF-8",
        "// All Material Icons are U+E000..U+F8FF (BMP) -- always 3 bytes",
        "static inline int icon_cp_to_utf8(uint32_t cp, char out[5]) {",
        "    if (cp < 0x10000) {",
        "        out[0] = (char)(0xE0 | (cp >> 12));",
        "        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));",
        "        out[2] = (char)(0x80 | (cp & 0x3F));",
        "        out[3] = '\\0';",
        "        return 3;",
        "    }",
        "    // Supplementary plane (Material Symbols extended range)",
        "    out[0] = (char)(0xF0 | (cp >> 18));",
        "    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));",
        "    out[2] = (char)(0x80 | ((cp >> 6)  & 0x3F));",
        "    out[3] = (char)(0x80 | (cp & 0x3F));",
        "    out[4] = '\\0';",
        "    return 4;",
        "}",
    ]

    with open(HEADER_OUT, "w", newline="\n", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    print(f"  OK  {HEADER_OUT.name}  ({HEADER_OUT.stat().st_size / 1024:.1f} KB)")


# ── JS icon list ──────────────────────────────────────────────────────────────
def generate_js_list(all_icons):
    """
    Generate mi_icons.js for the web editor icon browser.
    Always uses the full set so all icons are browsable even if
    only a subset is compiled into firmware.
    Format: const MI_ICONS=[[name,codepoint],...];
    """
    JS_OUT.parent.mkdir(parents=True, exist_ok=True)
    entries  = [[name, cp] for name, cp in all_icons]
    json_str = json.dumps(entries, separators=(',', ':'))
    with open(JS_OUT, "w", newline="\n", encoding="utf-8") as f:
        f.write(f"const MI_ICONS={json_str};\n")
    print(f"  OK  {JS_OUT.name}  ({JS_OUT.stat().st_size / 1024:.1f} KB)"
          f"  <- web editor icon browser")


# ── Main ──────────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description="Generate Material Icons font subset for ESP_MKB")
    parser.add_argument("--size",      type=int, default=24,
                        help="Font size in px (default: 24)")
    parser.add_argument("--bpp",       type=int, default=4,
                        help="Bits per pixel (default: 4)")
    parser.add_argument("--full",      action="store_true",
                        help="Include all icons instead of subset")
    parser.add_argument("--no-font",   action="store_true",
                        help="Skip font generation (header + JS only)")
    parser.add_argument("--no-header", action="store_true",
                        help="Skip icon_codepoints.h generation")
    parser.add_argument("--no-js",     action="store_true",
                        help="Skip mi_icons.js generation")
    args = parser.parse_args()

    subset_mode = not args.full
    mode_label  = "SUBSET (used icons only)" if subset_mode else "FULL (all icons)"

    print("\n+--------------------------------------------------+")
    print("|   ESP_MKB - Material Icons Font Pipeline         |")
    print(f"|   Mode: {mode_label:<43}|")
    print("+--------------------------------------------------+\n")

    # ── 1. Prerequisites ──────────────────────────────────────────────────────
    print("1. Checking prerequisites...")
    lv_cmd = check_prerequisites()
    print("   All prerequisites found\n")

    # ── 2. Parse codepoints ───────────────────────────────────────────────────
    print("2. Parsing codepoints...")
    all_icons = parse_codepoints()

    # ── 3. Determine icon set ─────────────────────────────────────────────────
    if subset_mode:
        print("\n3. Scanning JSON files for used icons...")
        icons = find_used_icons(all_icons)
    else:
        print(f"\n3. Using full icon set ({len(all_icons)} icons)")
        icons = all_icons

    # ── 4. Generate font ──────────────────────────────────────────────────────
    if not args.no_font:
        print(f"\n4. Generating LVGL font ({args.size}px, {args.bpp}bpp)...")
        if not subset_mode:
            print("   NOTE: Full set may take 30-90 seconds...")
        generate_font(icons, args.size, args.bpp, lv_cmd, subset_mode)
    else:
        print("\n4. Skipping font generation (--no-font)")

    # ── 5. Generate header ────────────────────────────────────────────────────
    if not args.no_header:
        print("\n5. Generating icon_codepoints.h (full set for lookup)...")
        generate_header(all_icons, args.size)
    else:
        print("\n5. Skipping header generation (--no-header)")

    # ── 6. Generate JS list ───────────────────────────────────────────────────
    if not args.no_js:
        print("\n6. Generating mi_icons.js (full set for web editor)...")
        generate_js_list(all_icons)
    else:
        print("\n6. Skipping JS generation (--no-js)")

    # ── Summary ───────────────────────────────────────────────────────────────
    print("\n+--------------------------------------------------+")
    print("|   Done!                                          |")
    print("+--------------------------------------------------+")

    font_name = f"material_icons{'_used' if subset_mode else ''}_{args.size}"
    out_c     = MAIN_DIR  / f"{font_name}.c"
    out_bin   = ICONS_DIR / f"{font_name}.bin"

    if out_c.exists():
        print(f"\n  {out_c}  ({out_c.stat().st_size / 1024:.1f} KB)")
    if out_bin.exists():
        print(f"  {out_bin}  ({out_bin.stat().st_size / 1024:.1f} KB)")
    if HEADER_OUT.exists():
        print(f"  {HEADER_OUT}  ({HEADER_OUT.stat().st_size / 1024:.1f} KB)")
    if JS_OUT.exists():
        print(f"  {JS_OUT}  ({JS_OUT.stat().st_size / 1024:.1f} KB)")

    print("\n  Next steps:")
    print("  1. Run 'idf.py flash' to write firmware + LittleFS")
    print("  2. Connect to ESP-MKB AP or check IP on your network")
    print("  3. Open http://192.168.4.1 in your browser\n")


if __name__ == "__main__":
    main()