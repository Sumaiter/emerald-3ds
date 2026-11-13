#!/usr/bin/env python3
"""Validate avatar coverage and encode existing drawings as GBA 4bpp tiles.

CBK1: magic, u16 version, u16 count; count records <HBBI> (graphics ID,
native back-pic ID or 255 for new art, zero, absolute data offset).
Custom data: 16 RGB555 colors, then 64x64 tiled 4bpp pixels. Index 0 is
transparent. Native records carry only an ID, never original game pixels.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
from pathlib import Path

from PIL import Image

NATIVE = {
    "OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL": 0, "OBJ_EVENT_GFX_RIVAL_MAY_NORMAL": 1,
    "OBJ_EVENT_GFX_RED": 2, "OBJ_EVENT_GFX_LEAF": 3,
    "OBJ_EVENT_GFX_LINK_RS_BRENDAN": 4, "OBJ_EVENT_GFX_LINK_RS_MAY": 5,
    "OBJ_EVENT_GFX_WALLY": 6, "OBJ_EVENT_GFX_STEVEN": 7,
}


def choices(upstream: Path) -> dict[str, int]:
    menu = (upstream / "src/multiplayer_menu.c").read_text(encoding="utf-8")
    block = menu.split("sCharacters[]", 1)[1].split("};", 1)[0]
    symbols = re.findall(r"\{\s*(OBJ_EVENT_GFX_\w+)\s*,", block)
    constants = dict(re.findall(r"^#define\s+(OBJ_EVENT_GFX_\w+)\s+(\d+)\s*$",
                               (upstream / "include/constants/event_objects.h").read_text(), re.M))
    result = {s: int(constants[s]) for s in symbols}
    if not result or len(result) != len(symbols) or len(set(result.values())) != len(result):
        raise ValueError("Invalid or duplicate playable character list")
    return result


def encode_image(path: Path) -> bytes:
    with Image.open(path) as image:
        rgba = image.convert("RGBA")
    if rgba.size != (64, 64):
        raise ValueError(f"{path}: battle back must be 64x64")
    palette = [0]
    colors = {}
    indices = []
    for r, g, b, a in rgba.getdata():
        if a not in (0, 255):
            raise ValueError(f"{path}: alpha must be 0 or 255")
        if not a:
            indices.append(0)
            continue
        color = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)
        if color not in colors:
            if len(colors) == 15:
                raise ValueError(f"{path}: more than 15 visible RGB555 colors")
            colors[color] = len(palette)
            palette.append(color)
        indices.append(colors[color])
    if not colors or 0 not in indices:
        raise ValueError(f"{path}: expected a visible drawing and transparent background")
    pixels = bytearray()
    for ty in range(8):
        for tx in range(8):
            for y in range(8):
                for x in range(0, 8, 2):
                    i = (ty * 8 + y) * 64 + tx * 8 + x
                    pixels.append(indices[i] | (indices[i + 1] << 4))
    return struct.pack("<16H", *(palette + [0] * (16 - len(palette)))) + pixels


def pack(manifest_path: Path, expected: dict[str, int]) -> bytes:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    entries = manifest["entries"]
    actual = {e["symbol"]: e["graphics_id"] for e in entries}
    if len(actual) != len(entries) or actual != expected or len(set(actual.values())) != len(entries):
        raise ValueError("Manifest must cover every playable character exactly once, with matching graphics IDs")
    if len(entries) > 256 or any(not 0 <= e["graphics_id"] < 256 for e in entries):
        raise ValueError("Graphics IDs must fit the co-op profile")
    records = bytearray()
    data = bytearray()
    base = 8 + len(entries) * 8
    for entry in sorted(entries, key=lambda e: e["graphics_id"]):
        if entry["source"] == "native":
            native = entry["native_back_pic_id"]
            if NATIVE.get(entry["symbol"]) != native:
                raise ValueError(f"Incorrect native back-picture mapping: {entry['symbol']}")
            offset = 0
        elif entry["source"] == "generated":
            if entry["symbol"] in NATIVE:
                raise ValueError("Native choices must retain their original animation")
            native = 255
            path = (manifest_path.parent / entry["image"]).resolve()
            if not path.is_relative_to(manifest_path.parent.resolve()):
                raise ValueError("Image must lie in the manifest directory")
            offset = base + len(data)
            data.extend(encode_image(path))
        else:
            raise ValueError("Unknown art source")
        records.extend(struct.pack("<HBBI", entry["graphics_id"], native, 0, offset))
    return struct.pack("<4sHH", b"CBK1", 1, len(entries)) + records + data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=Path("assets/coop_battle_back/manifest.json"))
    parser.add_argument("--upstream", type=Path, default=Path(".."))
    parser.add_argument("--output", type=Path, default=Path("romfs/coop/backpics.bin"))
    args = parser.parse_args()
    blob = pack(args.manifest, choices(args.upstream))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(blob)
    print(f"Co-op battle backs: {len(choices(args.upstream))} characters, {len(blob)} bytes -> {args.output}")


if __name__ == "__main__":
    main()
