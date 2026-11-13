#!/usr/bin/env python3
"""Validate every selected rider and pack new 27-frame 32x32 bicycle sheets.

CBI1: <4sHH>, then <BBBBII> records (identity, native Mach ID/native Acro
ID or 255/255, zero, data offset, zero). Each custom payload is one 16-color
RGB555 palette plus 27 tiled 4bpp frames. Native records carry no game pixels.
Sheets are 288x96: 9 columns x 3 rows in the native Acro frame order. The
first 9 frames also drive Mach riding; Acro has all 27 distinct safe indices.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
from pathlib import Path
from PIL import Image

NATIVE = {
    "OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL": (101, 102),
    "OBJ_EVENT_GFX_RIVAL_MAY_NORMAL": (106, 107),
}
FRAME_NAMES = [
    "south_idle", "north_idle", "west_idle",
    "south_pedal_a", "south_pedal_b", "north_pedal_a", "north_pedal_b", "west_pedal_a", "west_pedal_b",
    "south_raise", "south_wheelie", "south_lower", "south_front_hop",
    "north_raise", "north_wheelie", "north_lower", "north_front_hop",
    "west_raise", "west_wheelie", "west_lower", "west_front_hop",
    "south_wheelie_pedal_a", "south_wheelie_pedal_b", "north_wheelie_pedal_a", "north_wheelie_pedal_b",
    "west_wheelie_pedal_a", "west_wheelie_pedal_b",
]


def choices(upstream: Path) -> dict[str, int]:
    menu = (upstream / "src/multiplayer_menu.c").read_text(encoding="utf-8")
    block = menu.split("sCharacters[]", 1)[1].split("};", 1)[0]
    symbols = re.findall(r"\{\s*(OBJ_EVENT_GFX_\w+)\s*,", block)
    constants = dict(re.findall(r"^#define\s+(OBJ_EVENT_GFX_\w+)\s+(\d+)\s*$",
                               (upstream / "include/constants/event_objects.h").read_text(), re.M))
    result = {s: int(constants[s]) for s in symbols}
    if not result or len(result) != len(symbols) or len(set(result.values())) != len(result):
        raise ValueError("Invalid playable character list")
    return result


def encode_sheet(path: Path) -> bytes:
    with Image.open(path) as im:
        rgba = im.convert("RGBA")
    if rgba.size != (288, 96):
        raise ValueError(f"{path}: expected a 288x96 sheet (27 frames)")
    palette, colors, indices = [0], {}, []
    for r, g, b, a in rgba.getdata():
        if a not in (0, 255): raise ValueError(f"{path}: alpha must be binary")
        if not a:
            indices.append(0)
            continue
        color = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)
        if color not in colors:
            if len(colors) == 15: raise ValueError(f"{path}: more than 15 visible RGB555 colors")
            colors[color] = len(palette)
            palette.append(color)
        indices.append(colors[color])
    pixels, frames = bytearray(), []
    for frame in range(27):
        fx, fy = (frame % 9) * 32, (frame // 9) * 32
        raw = [indices[(fy+y)*288+fx+x] for y in range(32) for x in range(32)]
        if not any(raw) or 0 not in raw: raise ValueError(f"{path}: frame {frame} must contain art and transparency")
        frames.append(bytes(raw))
        for ty in range(4):
            for tx in range(4):
                for y in range(8):
                    for x in range(0, 8, 2):
                        i = (ty*8+y)*32+tx*8+x
                        pixels.append(raw[i] | (raw[i+1] << 4))
    # Static repeats are not cycling or wheelie animations.
    for a, b in [(3,4),(5,6),(7,8),(0,10),(1,14),(2,18),(21,22),(23,24),(25,26)]:
        if frames[a] == frames[b]: raise ValueError(f"{path}: frames {a}/{b} do not animate")
    return struct.pack("<16H", *(palette + [0]*(16-len(palette)))) + pixels


def pack(manifest_path: Path, expected: dict[str, int]) -> bytes:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    entries = manifest["entries"]
    actual = {e["symbol"]: e["graphics_id"] for e in entries}
    if len(actual) != len(entries) or actual != expected or len(set(actual.values())) != len(entries):
        raise ValueError("Manifest must cover every playable identity exactly once")
    if not 0 < len(entries) <= 239 or any(not 0 <= e["graphics_id"] < 239 for e in entries):
        raise ValueError("Invalid object graphics ID")
    records, data = bytearray(), bytearray()
    base = 8 + len(entries)*12
    for entry in sorted(entries, key=lambda e: e["graphics_id"]):
        native = NATIVE.get(entry["symbol"])
        if entry["source"] == "native":
            if native != tuple(entry["native_graphics_ids"]): raise ValueError("Incorrect native bike mapping")
            mach, acro = native
            offset = 0
        elif entry["source"] == "generated":
            if native: raise ValueError("Native rider must retain its native animations")
            path = (manifest_path.parent / entry["image"]).resolve()
            if not path.is_relative_to(manifest_path.parent.resolve()): raise ValueError("Image outside asset directory")
            offset, mach, acro = base + len(data), 255, 255
            data.extend(encode_sheet(path))
        else:
            raise ValueError("Unknown art source")
        records.extend(struct.pack("<BBBBII", entry["graphics_id"], mach, acro, 0, offset, 0))
    return struct.pack("<4sHH", b"CBI1", 1, len(entries)) + records + data


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--manifest", type=Path, default=Path("assets/coop_bike/manifest.json"))
    p.add_argument("--upstream", type=Path, default=Path(".."))
    p.add_argument("--output", type=Path, default=Path("romfs/coop/bikes.bin"))
    a = p.parse_args()
    blob = pack(a.manifest, choices(a.upstream))
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_bytes(blob)
    print(f"Bicycle art: {len(blob)} bytes -> {a.output}")


if __name__ == "__main__": main()
