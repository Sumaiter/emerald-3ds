#!/usr/bin/env python3
"""Prepare local rider references and normalize newly drawn Imagegen sheets.

No game art is copied into the tracked asset directory. Preparation is
non-destructive: an existing manifest must be removed explicitly to rebuild it.
"""
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path
from PIL import Image
from pack_coop_bike import choices, NATIVE, FRAME_NAMES

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "3ds_port/assets/coop_bike"
LOCAL = ROOT / "build/bike-art"
PROMPT = """Use case: stylized-concept. Draw one production sprite sheet for the exact
Pokemon Emerald overworld character {name} riding a bicycle. Reference 1 is this
character's identity: retain hairstyle, headgear, age, body shape, outfit and
colors. Reference 2 is the native bicycle sheet's camera, pixel scale, cell
alignment and pose order, not the rider identity. Replace that rider completely
with {name}. Do not put a different trainer's headgear on this character.
Transparent background, crisp hand-drawn Gen 3 pixel art, dark outline, maximum
15 visible colors across the entire sheet. No text, labels, border or grid lines.
Exactly 9 columns and 3 rows, all cells equal and aligned, with a full 32x32 pixel
design per cell (overall 288x96 pixel design, may be rendered larger).
All riders use the SAME design and proportional size across all 27 frames.
Frame order from left to right, top to bottom:
{frames}.
South means facing the viewer, north means back to the viewer, west is left
profile. East is mirrored by the engine, do not include extra east frames.
Cycling frames have visibly alternating bent legs and pedals; wheelie frames
have the front wheel visibly raised with the body leaning back. Front-hop
frames have the front wheel down/back wheel raised. Keep both wheels within
their cells; no cut-off pixels, no inter-cell touching, no shadows or scenery.
Respect every pose's registration in reference 2 so wheels land consistently.
"""


def prepare(upstream: Path):
    path = ASSETS / "manifest.json"
    if path.exists(): raise ValueError("Manifest already exists; refusing to overwrite art progress")
    ASSETS.mkdir(parents=True, exist_ok=True)
    (LOCAL / "references").mkdir(parents=True, exist_ok=True)
    ptrs = (upstream / "src/data/object_events/object_event_graphics_info_pointers.h").read_text()
    infos = (upstream / "src/data/object_events/object_event_graphics_info.h").read_text()
    tables = (upstream / "src/data/object_events/object_event_pic_tables.h").read_text()
    graphics = (upstream / "src/data/object_events/object_event_graphics.h").read_text()
    menu = (upstream / "src/multiplayer_menu.c").read_text(encoding="utf-8")
    names = dict(re.findall(r'\{(OBJ_EVENT_GFX_\w+),\s*_\("([^"]+)"\)\}', menu))
    entries, prompts = [], {}
    for symbol, identity in choices(upstream).items():
        info_name = re.search(r"\["+symbol+r"\]\s*=\s*&(\w+)", ptrs)[1]
        info = re.search(r"\b"+info_name+r"\s*=\s*\{(.*?)\};", infos, re.S)[1]
        table = re.search(r"\.images\s*=\s*(\w+)", info)[1]
        body = re.search(r"\b"+table+r"\[\]\s*=\s*\{(.*?)\};", tables, re.S)[1]
        pic = re.search(r"overworld_frame\((\w+)", body)[1]
        source = re.search(r'\b'+pic+r'\[\]\s*=\s*INCBIN_U32\("([^"]+)"', graphics)[1]
        w, h = [int(re.search(r"\."+key+r"\s*=\s*(\d+)", info)[1]) for key in ("width", "height")]
        name = symbol.removeprefix("OBJ_EVENT_GFX_").lower()
        image = Image.open(upstream / Path(source).with_suffix(".png")).convert("RGBA")
        image.crop((0,0,3*w,h)).resize((3*w*8,h*8),Image.Resampling.NEAREST).save(LOCAL / "references" / (name+".png"))
        e = dict(symbol=symbol, graphics_id=identity, name=names[symbol], source="generated", image=name+".png")
        if symbol in NATIVE:
            e.update(source="native", image=None, native_graphics_ids=NATIVE[symbol])
        else:
            prompts[symbol] = PROMPT.format(name=names[symbol], frames="; ".join(f"{i}: {n}" for i,n in enumerate(FRAME_NAMES)))
        entries.append(e)
    # The original sheet is a local style/registration reference only.
    original = Image.open(upstream / "graphics/object_events/pics/people/brendan/acro_bike.png").convert("RGBA")
    reference = Image.new("RGBA",(288,96))
    for i in range(27):
        reference.paste(original.crop((i*32,0,(i+1)*32,32)),((i%9)*32,(i//9)*32))
    reference.resize((1728,576), Image.Resampling.NEAREST).save(LOCAL / "references/native_layout.png")
    path.write_text(json.dumps(dict(version=1, frame_width=32, frame_height=32, columns=9,
                                   frame_names=FRAME_NAMES, entries=entries), indent=2)+"\n",encoding="utf-8")
    (ASSETS / "prompts.json").write_text(json.dumps(prompts,indent=2)+"\n",encoding="utf-8")
    print(f"Prepared {len(entries)} riders ({len(prompts)} new sheets, {len(NATIVE)} native)")


def normalize(source: Path, target: Path):
    """Resize an already aligned generated sheet; never synthesize poses."""
    image = Image.open(source).convert("RGBA")
    # Sheet boundaries are fixed. Individual cropping would destroy registration.
    image = image.resize((288,96),Image.Resampling.NEAREST)
    alpha = image.getchannel("A").point(lambda p: 255 if p >= 128 else 0)
    image.putalpha(alpha)
    visible = [p[:3] for p in image.getdata() if p[3]]
    if not visible: raise ValueError("Empty drawing")
    samples = Image.new("RGB",(len(visible),1)); samples.putdata(visible)
    palette = samples.quantize(colors=15,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE).getpalette()[:45]
    colors = [tuple(palette[i:i+3]) for i in range(0,45,3)]
    lookup = {p: 1+min(range(15),key=lambda i:sum((p[c]-colors[i][c])**2 for c in range(3))) for p in set(visible)}
    out = Image.new("P",(288,96)); out.putpalette([0,0,0]+palette+[0]*(768-48))
    out.putdata([lookup[p[:3]] if p[3] else 0 for p in image.getdata()])
    target.parent.mkdir(parents=True, exist_ok=True)
    out.save(target, transparency=0, optimize=False)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    s=p.add_subparsers(dest="command", required=True)
    a=s.add_parser("prepare"); a.add_argument("--upstream",type=Path,default=ROOT/"build/upstream")
    a=s.add_parser("normalize"); a.add_argument("--source",type=Path,required=True); a.add_argument("--target",type=Path,required=True)
    a=p.parse_args()
    if a.command == "prepare": prepare(a.upstream)
    else: normalize(a.source,a.target)


if __name__ == "__main__": main()
