#!/usr/bin/env python3
"""Prepare local identity references and normalize newly drawn walking sheets.

Original game pixels stay under ignored build/. This script never draws poses.
"""
from __future__ import annotations

import argparse
import json
import re
from collections import deque
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / "build/zerowalk-art"
ASSETS = ROOT / "3ds_port/assets/coop_walk"
SYMBOLS = ["COOK", "OLD_MAN", "SIDNEY", "PHOEBE", "GLACIA", "DRAKE",
           "ROXANNE", "BRAWLY", "WATTSON", "FLANNERY", "WINONA", "LIZA",
           "TATE", "GAMEBOY_KID", "MYSTERY_GIFT_MAN"]
FRAME_NAMES = ["south_idle", "north_idle", "west_idle", "south_step_a",
               "south_step_b", "north_step_a", "north_step_b", "west_step_a", "west_step_b"]


def identities(upstream: Path) -> dict[str, int]:
    constants = dict(re.findall(r"^#define\s+(OBJ_EVENT_GFX_\w+)\s+(\d+)\s*$",
        (upstream / "include/constants/event_objects.h").read_text(), re.M))
    return {"OBJ_EVENT_GFX_" + s: int(constants["OBJ_EVENT_GFX_" + s]) for s in SYMBOLS}


def prepare(upstream: Path):
    ASSETS.mkdir(parents=True, exist_ok=True)
    refs = LOCAL / "references"
    refs.mkdir(parents=True, exist_ok=True)
    ptrs = (upstream / "src/data/object_events/object_event_graphics_info_pointers.h").read_text()
    infos = (upstream / "src/data/object_events/object_event_graphics_info.h").read_text()
    tables = (upstream / "src/data/object_events/object_event_pic_tables.h").read_text()
    graphics = (upstream / "src/data/object_events/object_event_graphics.h").read_text()
    entries = []
    for symbol, identity in identities(upstream).items():
        name = symbol.removeprefix("OBJ_EVENT_GFX_").lower()
        info_name = re.search(r"\["+symbol+r"\]\s*=\s*&(\w+)", ptrs)[1]
        info = re.search(r"\b"+info_name+r"\s*=\s*\{(.*?)\};", infos, re.S)[1]
        table = re.search(r"\.images\s*=\s*(\w+)", info)[1]
        body = re.search(r"\b"+table+r"\[\]\s*=\s*\{(.*?)\};", tables, re.S)[1]
        pic = re.search(r"overworld_frame\((\w+)", body)[1]
        source = re.search(r'\b'+pic+r'\[\]\s*=\s*INCBIN_U32\("([^"]+)"', graphics)[1]
        w, h = [int(re.search(r"\."+key+r"\s*=\s*(\d+)", info)[1]) for key in ("width", "height")]
        image = Image.open(upstream / Path(source).with_suffix(".png")).convert("RGBA")
        image.crop((0, 0, min(image.width, 3*w), h)).resize((3*w*8, h*8), Image.Resampling.NEAREST).save(refs / (name+".png"))
        entries.append(dict(symbol=symbol, graphics_id=identity, name=name.upper().replace("_", " "),
                            source="generated", image=name+".png", identity_reference=source.replace(".4bpp", ".png")))
    path = ASSETS / "manifest.json"
    if not path.exists():
        path.write_text(json.dumps(dict(version=1, frame_width=32, frame_height=32,
            columns=3, frame_names=FRAME_NAMES, entries=entries), indent=2)+"\n", encoding="utf-8")
    print(f"Prepared {len(entries)} local references")


def register_walk(image):
    """Format nine complete drawn figures into a true 3x3 sprite grid.

    Imagegen may vary the outer margins and row spacing. Splitting its canvas
    into mathematical thirds can put the next character's hair in a prior
    frame. Component bounds identify the nine drawings; cropping, uniform
    nearest scaling and transparent padding register them without drawing or
    synthesizing a single pose. The crop rectangles preserve the input pixels.
    """
    alpha=image.getchannel("A").point(lambda p:255 if p>=128 else 0)
    mask=bytearray(alpha.tobytes()); width,height=image.size
    components=[]
    for start in range(len(mask)):
        if not mask[start]:continue
        mask[start]=0; todo=deque([start]); count=0
        left=right=start%width; top=bottom=start//width
        while todo:
            pos=todo.popleft(); x,y=pos%width,pos//width;count+=1
            left=min(left,x);right=max(right,x);top=min(top,y);bottom=max(bottom,y)
            for neighbor in (pos-1 if x else -1,pos+1 if x+1<width else -1,
                             pos-width if y else -1,pos+width if y+1<height else -1):
                if neighbor>=0 and mask[neighbor]:mask[neighbor]=0;todo.append(neighbor)
        components.append((count,(left,top,right+1,bottom+1)))
    largest=sorted(components,reverse=True)[:9]
    if len(largest)!=9 or largest[-1][0]<largest[0][0]*0.2:
        raise ValueError("Expected nine separated complete walking drawings")
    boxes=[box for _,box in sorted(largest,key=lambda c:(c[1][1]+c[1][3])/2)]
    boxes=[box for row in range(3) for box in sorted(boxes[row*3:row*3+3],key=lambda b:(b[0]+b[2])/2)]
    # One scale for all nine poses preserves relative head/body size.
    scale=min(28/max(b[2]-b[0] for b in boxes),28/max(b[3]-b[1] for b in boxes))
    out=Image.new("RGBA",(96,96))
    for frame,box in enumerate(boxes):
        drawing=image.crop(box)
        drawing=drawing.resize((max(1,round(drawing.width*scale)),max(1,round(drawing.height*scale))),Image.Resampling.NEAREST)
        x=(frame%3)*32+(32-drawing.width)//2
        y=(frame//3)*32+31-drawing.height
        out.paste(drawing,(x,y))
    return out


def normalize(source: Path, target: Path, size=(96,96)):
    image = Image.open(source).convert("RGBA")
    image = register_walk(image) if size==(96,96) else image.resize(size, Image.Resampling.NEAREST)
    image.putalpha(image.getchannel("A").point(lambda p: 255 if p >= 128 else 0))
    visible = [p[:3] for p in image.getdata() if p[3]]
    if not visible:
        raise ValueError("Empty drawing")
    samples = Image.new("RGB", (len(visible),1)); samples.putdata(visible)
    palette = samples.quantize(colors=15, method=Image.Quantize.MEDIANCUT,
        dither=Image.Dither.NONE).getpalette()[:45]
    colors = [tuple(palette[i:i+3]) for i in range(0,45,3)]
    lookup = {p: 1+min(range(15), key=lambda i: sum((p[c]-colors[i][c])**2 for c in range(3))) for p in set(visible)}
    out = Image.new("P", size); out.putpalette([0,0,0]+palette+[0]*(768-48))
    out.putdata([lookup[p[:3]] if p[3] else 0 for p in image.getdata()])
    target.parent.mkdir(parents=True, exist_ok=True)
    out.save(target, transparency=0, optimize=False)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest="command", required=True)
    a = sub.add_parser("prepare"); a.add_argument("--upstream", type=Path, default=ROOT/"build/upstream")
    a = sub.add_parser("normalize"); a.add_argument("--source", type=Path, required=True); a.add_argument("--target", type=Path, required=True)
    a = p.parse_args()
    if a.command == "prepare": prepare(a.upstream)
    else: normalize(a.source, a.target)


if __name__ == "__main__": main()
