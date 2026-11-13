#!/usr/bin/env python3
"""Pack the 15 newly drawn walking sheets without replacing any NPC data.

CWK1: <4sHH>, then sorted <HBBI> records (identity, zero, zero, absolute
payload offset). Payload: 16 RGB555 colors then nine 32x32 tiled 4bpp frames.
The sheet is 96x96, three columns; frame order matches the standard 0..8.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path
from PIL import Image
from prepare_coop_walk import FRAME_NAMES, identities
from pack_coop_bike import choices


def encode_sheet(path: Path) -> bytes:
    with Image.open(path) as image:
        rgba = image.convert("RGBA")
    if rgba.size != (96,96):
        raise ValueError(f"{path}: expected a 96x96 walking sheet")
    palette, colors, indices = [0], {}, []
    for r,g,b,a in rgba.getdata():
        if a not in (0,255):
            raise ValueError(f"{path}: alpha must be binary")
        if not a:
            indices.append(0)
            continue
        rgb = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)
        if rgb not in colors:
            if len(colors) == 15:
                raise ValueError(f"{path}: more than 15 visible RGB555 colors")
            colors[rgb] = len(palette); palette.append(rgb)
        indices.append(colors[rgb])
    pixels, frames = bytearray(), []
    for frame in range(9):
        fx,fy = (frame%3)*32, (frame//3)*32
        raw = [indices[(fy+y)*96+fx+x] for y in range(32) for x in range(32)]
        if not any(raw) or 0 not in raw:
            raise ValueError(f"{path}: frame {frame} must contain art and transparency")
        if any(raw[y*32+x] for y in range(32) for x in range(32) if x in (0,31) or y in (0,31)):
            raise ValueError(f"{path}: frame {frame} touches its cell boundary")
        frames.append(bytes(raw))
        for ty in range(4):
            for tx in range(4):
                for y in range(8):
                    for x in range(0,8,2):
                        i = (ty*8+y)*32+tx*8+x
                        pixels.append(raw[i] | (raw[i+1] << 4))
    for idle,a,b in ((0,3,4),(1,5,6),(2,7,8)):
        if len({frames[idle],frames[a],frames[b]}) != 3:
            raise ValueError(f"{path}: frames {idle}/{a}/{b} must be distinct walking poses")
    return struct.pack("<16H", *(palette+[0]*(16-len(palette))))+pixels


def pack(manifest_path: Path, expected: dict[str,int]) -> bytes:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if any(manifest.get(k) != v for k,v in dict(version=1, frame_width=32,
        frame_height=32, columns=3, frame_names=FRAME_NAMES).items()):
        raise ValueError("Invalid walking layout metadata")
    entries = manifest["entries"]
    actual = {e["symbol"]:e["graphics_id"] for e in entries}
    if not entries or len(entries)>15 or len(actual)!=len(entries) or actual!=expected or len(set(actual.values()))!=len(entries):
        raise ValueError("Manifest must cover every ZeroWalk identity exactly once")
    if any(not 0<=e["graphics_id"]<239 for e in entries):
        raise ValueError("Invalid object graphics ID")
    records,data = bytearray(),bytearray()
    base = 8+len(entries)*8
    for entry in sorted(entries,key=lambda e:e["graphics_id"]):
        if entry["source"]!="generated":
            raise ValueError("Walking pack contains new drawings only")
        path = (manifest_path.parent/entry["image"]).resolve()
        if not path.is_relative_to(manifest_path.parent.resolve()):
            raise ValueError("Image outside asset directory")
        records.extend(struct.pack("<HBBI",entry["graphics_id"],0,0,base+len(data)))
        data.extend(encode_sheet(path))
    return struct.pack("<4sHH",b"CWK1",1,len(entries))+records+data


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--manifest",type=Path,default=Path("assets/coop_walk/manifest.json"))
    p.add_argument("--upstream",type=Path,default=Path(".."))
    p.add_argument("--output",type=Path,default=Path("romfs/coop/walks.bin"))
    a=p.parse_args()
    expected=identities(a.upstream)
    selected=choices(a.upstream)
    if any(selected.get(s)!=i for s,i in expected.items()):
        raise ValueError("All 15 ZeroWalk identities must be selected before packing")
    blob=pack(a.manifest,expected)
    a.output.parent.mkdir(parents=True,exist_ok=True); a.output.write_bytes(blob)
    print(f"Walking art: {len(expected)} identities, {len(blob)} bytes -> {a.output}")


if __name__=="__main__": main()
