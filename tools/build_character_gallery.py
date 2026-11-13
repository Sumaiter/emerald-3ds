#!/usr/bin/env python3
"""Build a local, self-contained character review HTML from actual sprite assets.

The generated file includes private upstream graphics and MUST stay in build/.
No drawing or pose synthesis: image operations only extract/arrange source frames.
Animation commands, timing and flips are read from the engine's own tables.
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import io
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "build/python-packages"))
from PIL import Image


def read(path):
    return Path(path).read_text(encoding="utf-8")


def body(source, name):
    match = re.search(r"\b" + re.escape(name) + r"\s*(?:\[[^\]]*\])?\s*=\s*\{(.*?)\};", source, re.S)
    if not match:
        raise ValueError(f"Missing source table: {name}")
    return match[1]


def animation(source, name):
    """Keep actual FRAME durations/flips and distinguish looping from finite clips."""
    frames, loop, loop_start = [], False, 0
    for command in re.finditer(r"ANIMCMD_(FRAME|JUMP|END|LOOP)(?:\(([^)]*)\))?", body(source, name)):
        op, arguments = command[1], command[2] or ""
        if op == "FRAME":
            values = arguments.split(",")
            frames.append(dict(index=int(values[0].strip(), 0), duration=int(values[1].strip(), 0),
                               hflip=bool(re.search(r"\.hFlip\s*=\s*TRUE", arguments)),
                               vflip=bool(re.search(r"\.vFlip\s*=\s*TRUE", arguments))))
        elif op == "JUMP":
            loop, loop_start = True, int(arguments.strip(), 0)
            break
        elif op == "END":
            break
        else:
            raise ValueError(f"Unsupported animation loop in {name}")
    if not frames or not 0 <= loop_start < len(frames):
        raise ValueError(f"Invalid animation: {name}")
    return dict(frames=frames, loop=loop, loop_start=loop_start)


def clips(source, table):
    result = []
    for key, name in re.findall(r"\[(ANIM_\w+)\]\s*=\s*(\w+)", body(source, table)):
        direction = next((d.lower() for d in ("SOUTH", "NORTH", "WEST", "EAST") if key.endswith(d)), "south")
        action = re.sub(r"_(SOUTH|NORTH|WEST|EAST)$", "", key.removeprefix("ANIM_"))
        result.append(dict(id=key, action=action, direction=direction, **animation(source, name)))
    if not result:
        raise ValueError(f"Empty animation table: {table}")
    return result


def image_data(image):
    output = io.BytesIO()
    image.save(output, format="PNG", optimize=True)
    return "data:image/png;base64," + base64.b64encode(output.getvalue()).decode("ascii")


def make_variant(frames, animations, source, origin, names=None):
    if not frames:
        raise ValueError("Empty frame list")
    for clip in animations:
        for command in clip["frames"]:
            if not 0 <= command["index"] < len(frames) or command["duration"] < 1:
                raise ValueError(f"Animation outside sprite sheet: {clip['id']}")
    width, height = max(f.width for f in frames), max(f.height for f in frames)
    columns = min(9, len(frames))
    atlas = Image.new("RGBA", (width * columns, height * ((len(frames) + columns - 1) // columns)))
    for index, frame in enumerate(frames):
        atlas.alpha_composite(frame, ((index % columns) * width + (width - frame.width) // 2,
                                     (index // columns) * height + height - frame.height))
    return dict(image=image_data(atlas), width=width, height=height, columns=columns, count=len(frames),
                animations=animations, source=source, origin=origin,
                names=names or [str(i) for i in range(len(frames))],
                sha256=hashlib.sha256(atlas.tobytes()).hexdigest())


class Engine:
    def __init__(self, root):
        self.root = Path(root)
        folder = self.root / "src/data/object_events"
        self.pointers = read(folder / "object_event_graphics_info_pointers.h")
        self.infos = read(folder / "object_event_graphics_info.h")
        self.pictures = read(folder / "object_event_pic_tables.h")
        self.graphics = read(folder / "object_event_graphics.h")
        self.anims = read(folder / "object_event_anims.h")
        self.movement = read(self.root / "src/event_object_movement.c")
        self.constants = dict((key, int(value)) for key, value in re.findall(
            r"^#define\s+(OBJ_EVENT_GFX_\w+)\s+(\d+)\s*$", read(self.root / "include/constants/event_objects.h"), re.M))
        self.palette_symbols = dict((tag, pointer) for pointer, tag in re.findall(
            r"\{\s*(gObjectEventPal_\w+)\s*,\s*(OBJ_EVENT_PAL_TAG_\w+)\s*\}",
            body(self.movement, "sObjectEventSpritePalettes")))
        self.cache = {}

    def rgba(self, path, palette_tag=None):
        path = self.root / path
        key = (str(path), palette_tag)
        if key in self.cache:
            return self.cache[key].copy()
        with Image.open(path) as original:
            indexed = original.copy()
        if indexed.mode == "P":
            if palette_tag:
                pointer = self.palette_symbols.get(palette_tag)
                match = re.search(r"\b" + re.escape(pointer or "__missing__") + r'\[\]\s*=\s*INCBIN_U16\("([^"]+)"', self.graphics)
                if not match:
                    raise ValueError(f"Missing native palette: {palette_tag}")
                pal_path = self.root / Path(match[1]).with_suffix(".pal")
                palette = [int(v) for v in read(pal_path).splitlines()[3:] for v in v.split()]
                indexed.putpalette(palette + [0] * (768 - len(palette)))
            # The engine always treats 4bpp index 0 as transparent.
            indexed.info["transparency"] = 0
        result = indexed.convert("RGBA")
        self.cache[key] = result
        return result.copy()

    def native(self, symbol):
        match = re.search(r"\[" + re.escape(symbol) + r"\]\s*=\s*&(\w+)", self.pointers)
        if not match:
            raise ValueError(f"Missing graphics info: {symbol}")
        info = body(self.infos, match[1])
        field = lambda name: re.search(r"\." + name + r"\s*=\s*(\w+)", info)[1]
        frames = []
        for picture, width, height, index in re.findall(
                r"overworld_frame\((\w+),\s*(\d+),\s*(\d+),\s*(\d+)\)", body(self.pictures, field("images"))):
            width, height, index = int(width) * 8, int(height) * 8, int(index)
            match = re.search(r"\b" + picture + r'\[\]\s*=\s*INCBIN_U32\("([^"]+)"', self.graphics)
            if not match:
                raise ValueError(f"Missing picture: {picture}")
            image = self.rgba(Path(match[1]).with_suffix(".png"), field("paletteTag"))
            columns = image.width // width
            x, y = (index % columns) * width, (index // columns) * height
            if x + width > image.width or y + height > image.height:
                raise ValueError(f"Native frame outside sheet: {symbol}/{index}")
            frames.append(image.crop((x, y, x + width, y + height)))
        return make_variant(frames, clips(self.anims, field("anims")), "native", field("images"))


def generated_variant(path, manifest, animations, frame_limit=None):
    with Image.open(path) as im:
        image = im.convert("RGBA")
    width, height = manifest["frame_width"], manifest["frame_height"]
    columns, names = manifest["columns"], manifest["frame_names"]
    expected = (columns * width, ((len(names) + columns - 1) // columns) * height)
    if image.size != expected:
        raise ValueError(f"Incorrect gallery sheet size: {path.name} {image.size}, expected {expected}")
    frames = [image.crop(((i % columns) * width, (i // columns) * height,
                          (i % columns + 1) * width, (i // columns + 1) * height)) for i in range(len(names))]
    if frame_limit is not None:
        frames, names = frames[:frame_limit], names[:frame_limit]
    return make_variant(frames, animations, "generated", path.name, names)


def manifest(folder):
    path = ROOT / "3ds_port/assets" / folder / "manifest.json"
    data = json.loads(read(path)) if path.exists() else {"entries": []}
    return path.parent, data, {entry["symbol"]: entry for entry in data["entries"]}


def battle_variant(engine, entry, folder):
    if entry["source"] == "native":
        image = engine.rgba(entry["native_path"])
        frames = [image.crop((x, y, x + 64, y + 64)) for y in range(0, image.height, 64)
                  for x in range(0, image.width, 64)]
        native = ["Brendan", "May", "Red", "Leaf", "RubySapphireBrendan", "RubySapphireMay", "Wally", "Steven"][entry["native_back_pic_id"]]
        source = read(engine.root / "src/data/trainer_graphics/back_pic_anims.h")
        throw = "sAnimCmd_May_Steven_1" if native in ("May", "Steven") else "sAnimCmd_" + native + "_1"
        idle_frame = 0 if native in ("Red", "Leaf") else 3
        animations = [dict(id="battle_idle", action="BATTLE_IDLE", direction="south", loop=False, loop_start=0,
                           frames=[dict(index=idle_frame, duration=60, hflip=False, vflip=False)]),
                      dict(id="battle_throw", action="BATTLE_THROW", direction="south", **animation(source, throw))]
        return make_variant(frames, animations, "native", entry["native_path"])
    path = folder / entry["image"]
    if not path.exists():
        return None
    with Image.open(path) as image:
        image = image.convert("RGBA")
    if image.size != (64, 64):
        raise ValueError(f"Invalid back picture: {path.name}")
    return make_variant([image], [dict(id="battle_idle", action="BATTLE_IDLE", direction="south", loop=False,
                        loop_start=0, frames=[dict(index=0, duration=60, hflip=False, vflip=False)])], "generated", path.name)


def build(upstream, allow_pending=False, expect_count=None):
    engine = Engine(upstream)
    bike_folder, bike, bikes = manifest("coop_bike")
    battle_folder, _, backs = manifest("coop_battle_back")
    walk_folder, walk, walks = manifest("coop_walk")
    menu = body(read(engine.root / "src/multiplayer_menu.c"), "sCharacters")
    roster = dict(re.findall(r'\{(OBJ_EVENT_GFX_\w+),\s*_\("([^"]+)"\)\}', menu))
    zero = re.findall(r"\|\s*([^|\n]+)\s*\|\s*`(OBJ_EVENT_GFX_\w+)`", read(ROOT / "ZEROWALK.md"))
    zero_symbols = {symbol for _, symbol in zero} | set(walks)
    for name, symbol in zero:
        roster.setdefault(symbol, name.strip())
    for symbol, entry in walks.items():
        roster.setdefault(symbol, entry["name"])
    if expect_count is not None and len(roster) != expect_count:
        raise ValueError(f"Expected {expect_count} characters, got {len(roster)}")
    standard = clips(engine.anims, "sAnimTable_Standard")
    acro = clips(engine.anims, "sAnimTable_AcroBike")
    walking = list(standard)
    walk_source = ROOT / "3ds_port/src/3ds_coop_walk.c"
    if walks:
        for direction in ("South", "North", "West", "East"):
            walking.append(dict(id="ANIM_RUN_" + direction.upper(), action="RUN", direction=direction.lower(),
                                **animation(read(walk_source), "sWalkRun" + direction)))
    characters, pending = [], []
    for symbol, name in roster.items():
        original = engine.native(symbol)
        entry = walks.get(symbol)
        foot = original if symbol not in zero_symbols else None
        if entry and (walk_folder / entry["image"]).exists():
            foot = generated_variant(walk_folder / entry["image"], walk, walking)
        variants = dict(original=original, walk=foot, mach=None, acro=None, battle=None)
        entry = bikes.get(symbol)
        if entry:
            if entry["source"] == "native":
                id_to_symbol = {value: key for key, value in engine.constants.items()}
                variants["mach"], variants["acro"] = [engine.native(id_to_symbol[i]) for i in entry["native_graphics_ids"]]
            elif (bike_folder / entry["image"]).exists():
                variants["acro"] = generated_variant(bike_folder / entry["image"], bike, acro)
                # The first nine images and Standard commands are the Mach runtime path.
                variants["mach"] = generated_variant(bike_folder / entry["image"], bike, standard, frame_limit=9)
        if symbol in backs:
            variants["battle"] = battle_variant(engine, backs[symbol], battle_folder)
        missing = [key for key, value in variants.items() if value is None]
        if missing:
            pending.append(dict(symbol=symbol, missing=missing))
        characters.append(dict(symbol=symbol, id=engine.constants[symbol], name=name,
                               new=symbol in zero_symbols, variants=variants, missing=missing))
    if pending and not allow_pending:
        raise ValueError(f"Gallery incomplete: {len(pending)} characters still missing assets; use --allow-pending only for preparation")
    characters.sort(key=lambda entry: (entry["id"], entry["name"]))
    return dict(version=1, generated_at=datetime.now(timezone.utc).isoformat(timespec="seconds"),
                complete=not pending, count=len(characters), pending=pending, characters=characters)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, default=ROOT / "build/upstream")
    parser.add_argument("--output", type=Path, default=ROOT / "build/character-gallery/index.html")
    parser.add_argument("--allow-pending", action="store_true")
    parser.add_argument("--expect-count", type=int, default=99)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / "build").resolve()):
        parser.error("Generated gallery contains original game graphics; keep output under ignored build/")
    data = build(args.upstream, args.allow_pending, args.expect_count)
    payload = json.dumps(data, ensure_ascii=False, separators=(",", ":")).replace("<", "\\u003c")
    template = read(ROOT / "tools/character_gallery.html")
    if template.count("__CHARACTER_DATA__") != 1:
        raise ValueError("Invalid gallery template")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(template.replace("__CHARACTER_DATA__", payload), encoding="utf-8")
    (output.parent / "coverage.json").write_text(json.dumps({key: value for key, value in data.items()
        if key != "characters"}, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Gallery: {data['count']} characters, {len(data['pending'])} pending, {output.stat().st_size} bytes -> {output}")


if __name__ == "__main__":
    main()
