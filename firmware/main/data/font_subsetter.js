/**
 * font_subsetter.js — LVGL v9 Binary Font Subsetter
 *
 * Reads material_icons_24.bin (full font from LittleFS),
 * extracts only the glyphs used in buttons.json,
 * produces material_icons_subset_24.bin ready for LVGL.
 *
 * Binary format matches lv_binfont_loader.c exactly.
 * Section layout: [head][cmap][loca][glyf]
 * Each section:   uint32 length | char[4] label | data...
 */

class LvglFontSubsetter {

    constructor(arrayBuffer) {
        this.src = arrayBuffer;
        this.dv  = new DataView(arrayBuffer);
        this.u8  = new Uint8Array(arrayBuffer);

        this.header         = null;
        this.cmaps          = [];
        this.locaOffsets    = [];
        this.loca_count     = 0;
        this.glyphSecStart  = 0;
        this.glyphSecLength = 0;
    }

    // ── Read helpers (little-endian) ──────────────────────────────────────────
    _u8(o)  { return this.dv.getUint8(o); }
    _u16(o) { return this.dv.getUint16(o, true); }
    _u32(o) { return this.dv.getUint32(o, true); }
    _i16(o) { return this.dv.getInt16(o, true); }

    _section(offset, expected) {
        const length = this._u32(offset);
        const label  = String.fromCharCode(
            this._u8(offset+4), this._u8(offset+5),
            this._u8(offset+6), this._u8(offset+7)
        );
        if (expected && label !== expected)
            throw new Error(`Expected section '${expected}', got '${label}' at 0x${offset.toString(16)}`);
        return { length, label, data: offset + 8 };
    }

    // ── Parse "head" (font_header_bin_t) ─────────────────────────────────────
    // Offsets relative to section data start (= file offset + 8)
    _parseHead() {
        const { length, data: d } = this._section(0, 'head');
        this.header = {
            secLen:                length,
            version:               this._u32(d),
            tables_count:          this._u16(d+4),
            font_size:             this._u16(d+6),
            ascent:                this._u16(d+8),
            descent:               this._i16(d+10),
            typo_ascent:           this._u16(d+12),
            typo_descent:          this._i16(d+14),
            typo_line_gap:         this._u16(d+16),
            min_y:                 this._i16(d+18),
            max_y:                 this._i16(d+20),
            default_advance_width: this._u16(d+22),
            kerning_scale:         this._u16(d+24),
            index_to_loc_format:   this._u8(d+26),  // 0=uint16 loca, 1=uint32 loca
            glyph_id_format:       this._u8(d+27),
            advance_width_format:  this._u8(d+28),
            bits_per_pixel:        this._u8(d+29),
            xy_bits:               this._u8(d+30),
            wh_bits:               this._u8(d+31),
            advance_width_bits:    this._u8(d+32),
            compression_id:        this._u8(d+33),
            subpixels_mode:        this._u8(d+34),
            // d+35: padding
            underline_position:    this._i16(d+36),
            underline_thickness:   this._u16(d+38),
        };
        return length;
    }

    // ── Parse "cmap" section ──────────────────────────────────────────────────
    // cmap_table_bin_t = 16 bytes:
    //   u32 data_offset  (from cmap section start)
    //   u32 range_start
    //   u16 range_length
    //   u16 glyph_id_start
    //   u16 data_entries_count
    //   u8  format_type
    //   u8  padding
    //
    // format_type values (lv_font_fmt_txt_cmap_type_t):
    //   0 = FORMAT0_TINY  — sequential, no extra data
    //   1 = FORMAT0_FULL  — sequential + uint8 glyph_id_ofs_list
    //   2 = SPARSE_TINY   — uint16 unicode_list, sequential IDs
    //   3 = SPARSE_FULL   — uint16 unicode_list + uint16 glyph_id_ofs_list
    _parseCmaps(start) {
        const { length, data } = this._section(start, 'cmap');
        const count   = this._u32(data);
        const ENTRY   = 16;
        const tblBase = data + 4;  // start of cmap_table_bin_t array

        this.cmaps = [];
        for (let i = 0; i < count; i++) {
            const t       = tblBase + i * ENTRY;
            const fmt     = this._u8(t+14);
            const nEnt    = this._u16(t+12);
            const dataAbs = start + this._u32(t);  // absolute file offset

            const entry = {
                format_type:        fmt,
                range_start:        this._u32(t+4),
                range_length:       this._u16(t+8),
                glyph_id_start:     this._u16(t+10),
                data_entries_count: nEnt,
                unicode_list:       null,
                glyph_id_ofs_u8:    null,
                glyph_id_ofs_u16:   null,
            };

            if (fmt === 1) {
                // FORMAT0_FULL: uint8 glyph id offsets
                entry.glyph_id_ofs_u8 = new Uint8Array(this.src, dataAbs, nEnt);

            } else if (fmt === 2) {
                // SPARSE_TINY: sorted uint16 (cp - range_start) list
                entry.unicode_list = new Uint16Array(
                    this.src.slice(dataAbs, dataAbs + nEnt * 2)
                );

            } else if (fmt === 3) {
                // SPARSE_FULL: unicode_list + glyph_id_ofs_list
                entry.unicode_list = new Uint16Array(
                    this.src.slice(dataAbs, dataAbs + nEnt * 2)
                );
                const ofsBase = dataAbs + nEnt * 2;
                entry.glyph_id_ofs_u16 = new Uint16Array(
                    this.src.slice(ofsBase, ofsBase + nEnt * 2)
                );
            }
            // fmt === 0 (FORMAT0_TINY): no extra data

            this.cmaps.push(entry);
        }
        return length;
    }

    // ── Parse "loca" section ──────────────────────────────────────────────────
    _parseLoca(start) {
        const { length, data } = this._section(start, 'loca');
        this.loca_count = this._u32(data);
        const base = data + 4;
        this.locaOffsets = [];

        if (this.header.index_to_loc_format === 0) {
            // uint16 offsets
            for (let i = 0; i < this.loca_count; i++)
                this.locaOffsets.push(this._u16(base + i * 2));
        } else {
            // uint32 offsets
            for (let i = 0; i < this.loca_count; i++)
                this.locaOffsets.push(this._u32(base + i * 4));
        }
        return length;
    }

    // ── Store "glyf" section metadata (raw bytes copied later) ───────────────
    _parseGlyfSec(start) {
        const { length } = this._section(start, 'glyf');
        this.glyphSecStart  = start;
        this.glyphSecLength = length;  // total section size incl. 8-byte header
        return length;
    }

    // ── Full parse entry point ────────────────────────────────────────────────
    parse() {
        const headLen = this._parseHead();
        const cmapLen = this._parseCmaps(headLen);
        const locaLen = this._parseLoca(headLen + cmapLen);
        this._parseGlyfSec(headLen + cmapLen + locaLen);

        console.log(
            `[Subsetter] Parsed font: ${this.header.font_size}px, ` +
            `${this.loca_count} glyphs, ${this.cmaps.length} cmap(s)`
        );
    }

    // ── Codepoint → source glyph ID ──────────────────────────────────────────
    // Mirrors the lookup logic in lv_font_get_glyph_dsc_fmt_txt.c
    _cpToGlyphId(cp) {
        for (const c of this.cmaps) {
            if (cp < c.range_start || cp >= c.range_start + c.range_length) continue;
            const rel = cp - c.range_start;

            switch (c.format_type) {
                case 0:  // FORMAT0_TINY — sequential
                    return c.glyph_id_start + rel;

                case 1:  // FORMAT0_FULL — uint8 offset table
                    if (rel < c.data_entries_count)
                        return c.glyph_id_start + c.glyph_id_ofs_u8[rel];
                    break;

                case 2:  // SPARSE_TINY — binary search, sequential ID
                case 3: {// SPARSE_FULL — binary search, explicit ID
                    const list = c.unicode_list;
                    let lo = 0, hi = list.length - 1;
                    while (lo <= hi) {
                        const mid = (lo + hi) >>> 1;
                        if      (list[mid] === rel) {
                            return c.format_type === 2
                                ? c.glyph_id_start + mid
                                : c.glyph_id_start + c.glyph_id_ofs_u16[mid];
                        }
                        else if (list[mid] < rel) lo = mid + 1;
                        else                      hi = mid - 1;
                    }
                    break;
                }
            }
        }
        return 0;  // not found → null glyph
    }

    // ── Raw bytes for one source glyph ────────────────────────────────────────
    // loca offsets are relative to glyph section start (incl. 8-byte header)
    // Last glyph ends at glyphSecLength (the section length field value)
    _glyphBytes(srcId) {
        if (srcId >= this.loca_count) return new Uint8Array(0);
        const start      = this.glyphSecStart + this.locaOffsets[srcId];
        const nextLocaOfs = (srcId + 1 < this.loca_count)
            ? this.locaOffsets[srcId + 1]
            : this.glyphSecLength;
        return new Uint8Array(this.src, start, this.glyphSecStart + nextLocaOfs - start);
    }

    // ── Build subset binary ───────────────────────────────────────────────────
    buildSubset(codepoints) {
        // ── 1. Resolve codepoints to source glyph IDs ────────────────────────
        const sortedCps = [...new Set(codepoints)].sort((a, b) => a - b);

        // Always keep glyph 0 (null/fallback glyph — LVGL requires it at ID 0)
        const srcToNew = new Map([[0, 0]]);
        const cpToNew  = new Map();
        let   nextId   = 1;

        for (const cp of sortedCps) {
            const srcId = this._cpToGlyphId(cp);
            if (srcId === 0) {
                console.warn(`[Subsetter] U+${cp.toString(16).toUpperCase()} not in font — skipped`);
                continue;
            }
            if (!srcToNew.has(srcId)) srcToNew.set(srcId, nextId++);
            cpToNew.set(cp, srcToNew.get(srcId));
        }

        const nGlyphs  = srcToNew.size;
        const foundCps = [...cpToNew.keys()].sort((a, b) => a - b);

        console.log(`[Subsetter] ${nGlyphs} glyphs for ${foundCps.length} codepoints`);

        // ── 2. Collect raw glyph bytes + build new loca offsets ───────────────
        // byNewId[newId] = srcId
        const byNewId  = new Array(nGlyphs);
        for (const [srcId, newId] of srcToNew) byNewId[newId] = srcId;

        const glyphData    = byNewId.map(srcId => this._glyphBytes(srcId));
        const newLoca      = [];
        let   glyphOffset  = 8;  // first glyph at offset 8 (after section header)

        for (const bytes of glyphData) {
            newLoca.push(glyphOffset);
            glyphOffset += bytes.length;
        }
        const glyphSecSize = glyphOffset;  // = length field value for glyf section

        // ── 3. Build output cmap (single SPARSE_FULL entry) ──────────────────
        // unicode_list    : raw codepoint values as uint16 (range_start = 0)
        // glyph_id_ofs    : new glyph ID as uint16 (glyph_id_start = 0)
        // range_length    : max(cp) + 1, capped at 0xFFFF
        const maxCp       = foundCps.length ? foundCps[foundCps.length - 1] : 0;
        const rangeLength = Math.min(maxCp + 1, 0xFFFF);
        const unicodeArr  = new Uint16Array(foundCps);
        const glyphIdArr  = new Uint16Array(foundCps.map(cp => cpToNew.get(cp)));

        // cmap section layout:
        //   8  bytes : length(4) + "cmap"(4)
        //   4  bytes : subtables_count = 1
        //   16 bytes : cmap_table_bin_t
        //   N  bytes : unicode_list (u16 × n) + glyph_id_ofs (u16 × n)
        const cmapDataOfs = 28;  // data_offset field value (from cmap section start)
        const cmapSecSize = 28 + foundCps.length * 4;

        // loca section layout:
        //   8  bytes : length(4) + "loca"(4)
        //   4  bytes : loca_count
        //   4n bytes : uint32 offsets (we always write format 1 = uint32)
        const locaSecSize = 12 + nGlyphs * 4;

        const headSecSize = this.header.secLen;
        const totalSize   = headSecSize + cmapSecSize + locaSecSize + glyphSecSize;

        // ── 4. Write output buffer ────────────────────────────────────────────
        const outBuf = new ArrayBuffer(totalSize);
        const dv     = new DataView(outBuf);
        const outU8  = new Uint8Array(outBuf);
        let   p      = 0;

        const W8  = v  => { dv.setUint8(p++, v); };
        const W16 = v  => { dv.setUint16(p, v, true);  p += 2; };
        const W32 = v  => { dv.setUint32(p, v, true);  p += 4; };
        const WS  = s  => { for (const c of s) W8(c.charCodeAt(0)); };
        const WB  = a  => { outU8.set(a, p); p += a.length; };

        // head: copy original verbatim, then patch two fields
        WB(new Uint8Array(this.src, 0, headSecSize));
        dv.setUint16(12, 3, true);  // tables_count = 3 (head+cmap+loca+glyf, no kern)
        dv.setUint8(34,  1);        // index_to_loc_format = 1 (uint32 loca offsets)

        // cmap section
        W32(cmapSecSize);           WS('cmap');
        W32(1);                     // subtables_count
        // cmap_table_bin_t (16 bytes):
        W32(cmapDataOfs);           // data_offset from cmap section start
        W32(0);                     // range_start = 0
        W16(rangeLength);           // range_length
        W16(0);                     // glyph_id_start = 0
        W16(foundCps.length);       // data_entries_count
        W8(3);                      // format_type = SPARSE_FULL (3)
        W8(0);                      // padding
        // cmap data:
        WB(new Uint8Array(unicodeArr.buffer));
        WB(new Uint8Array(glyphIdArr.buffer));

        // loca section
        W32(locaSecSize);           WS('loca');
        W32(nGlyphs);
        for (const ofs of newLoca) W32(ofs);

        // glyf section
        W32(glyphSecSize);          WS('glyf');
        for (const bytes of glyphData) WB(bytes);

        console.log(`[Subsetter] Done: ${totalSize} bytes (${(totalSize/1024).toFixed(1)} KB)`);
        return outBuf;
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

/**
 * Extract codepoints from buttons.json text.
 * @param {string} buttonsJsonText
 * @param {Object} miMap  { icon_name: codepoint }
 *   Build from mi_icons.js:  Object.fromEntries(MI_ICONS.map(([n,c]) => [n,c]))
 */
function extractCodepoints(buttonsJsonText, miMap) {
    const found = [];
    for (const [, name] of buttonsJsonText.matchAll(/"mi:([a-z0-9_]+)"/g)) {
        const cp = miMap[name];
        if (cp !== undefined) found.push(cp);
        else console.warn(`[Subsetter] Unknown icon: mi:${name}`);
    }
    return [...new Set(found)];
}

/**
 * Full pipeline: full font + buttons.json → subset ArrayBuffer
 *
 * @param {ArrayBuffer} fullFontBuf  content of material_icons_24.bin
 * @param {string}      buttonsJson  content of buttons.json
 * @param {Object}      miMap        { name: codepoint } from mi_icons.js
 * @returns {ArrayBuffer}            subset .bin ready to upload
 */
function buildSubsetFont(fullFontBuf, buttonsJson, miMap) {
    const codepoints = extractCodepoints(buttonsJson, miMap);
    console.log(`[Subsetter] Building subset for ${codepoints.length} codepoints...`);
    const subsetter = new LvglFontSubsetter(fullFontBuf);
    subsetter.parse();
    return subsetter.buildSubset(codepoints);
}