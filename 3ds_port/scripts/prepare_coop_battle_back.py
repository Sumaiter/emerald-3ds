#!/usr/bin/env python3
"""Prepare references, normalize Imagegen results and audit coop battle back art.

Uses only local upstream assets as references; never copies native game art into
the tracked asset directory. New drawings are produced with the built-in
Imagegen tool, not with this script.
"""
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'build/python-packages'))
from PIL import Image, ImageDraw

ASSETS = ROOT / '3ds_port/assets/coop_battle_back'
LOCAL = ROOT / 'build/battle-art'
NATIVE = {
    'RIVAL_BRENDAN_NORMAL': 'brendan', 'RIVAL_MAY_NORMAL': 'may',
    'RED': 'red', 'LEAF': 'leaf', 'STEVEN': 'steven', 'WALLY': 'wally',
    'LINK_RS_BRENDAN': 'brendan_rs', 'LINK_RS_MAY': 'may_rs',
}
PROMPT = (
    'Use case: stylized-concept. Asset: Pokemon Emerald Gen 3 battle trainer '
    'BACK sprite for {name}. Reference image 1 shows this exact character in '
    'several overworld directions: preserve the actual hairstyle/headgear, '
    'outfit, age, body type and clothing colors; do not redesign them. '
    'Reference image 2 shows only the required rear-view battle camera, '
    'composition and pixel technique. Create ONE new, detailed upper-body '
    'trainer drawing in three-quarter REAR view, looking toward the upper '
    'right opponent. The back of head and shoulders must dominate; only a '
    'small facial side profile may show. Calm battle-ready pose with a '
    'Pokeball held near the right shoulder. Waist cropped at the bottom. '
    'Hand-drawn crisp pixel art designed for a 64x64 pixel grid, dark '
    'outlines, hard square pixels, restrained Gen 3 shading, maximum 15 '
    'visible colors. Transparent background, no ground, no shadow, no '
    'labels, no text, no front view, no sprite sheet, no multiple frames. '
    'Do not enlarge the overworld sprite: draw proper larger battle anatomy.'
)


def read(path):
    return path.read_text(encoding='utf-8')


def roster(upstream):
    menu = read(upstream / 'src/multiplayer_menu.c')
    block = menu.split('sCharacters[]', 1)[1].split('};', 1)[0]
    choices = re.findall(r'\{(OBJ_EVENT_GFX_\w+),\s*_\("([^"]+)"\)\}', block)
    constants = dict(re.findall(r'^#define\s+(OBJ_EVENT_GFX_\w+)\s+(\d+)\s*$',
                                read(upstream / 'include/constants/event_objects.h'), re.M))
    pointers = read(upstream / 'src/data/object_events/object_event_graphics_info_pointers.h')
    info = read(upstream / 'src/data/object_events/object_event_graphics_info.h')
    tables = read(upstream / 'src/data/object_events/object_event_pic_tables.h')
    graphics = read(upstream / 'src/data/object_events/object_event_graphics.h')
    result = []
    for symbol, name in choices:
        key = symbol.removeprefix('OBJ_EVENT_GFX_')
        info_name = re.search(r'\[' + symbol + r'\]\s*=\s*&(\w+)', pointers)[1]
        body = re.search(r'\b' + info_name + r'\s*=\s*\{(.*?)\};', info, re.S)[1]
        table = re.search(r'\.images\s*=\s*(\w+)', body)[1]
        table_body = re.search(r'\b' + table + r'\[\]\s*=\s*\{(.*?)\};', tables, re.S)[1]
        pic = re.search(r'overworld_frame\((\w+)', table_body)[1]
        source = re.search(r'\b' + pic + r'\[\]\s*=\s*INCBIN_U32\("([^"]+)"', graphics)[1]
        item = dict(graphics_id=int(constants[symbol]), symbol=symbol, name=name,
                    image=key.lower() + '.png', source='generated',
                    reference_path=str(Path(source).with_suffix('.png')).replace('\\', '/'),
                    reference_width=int(re.search(r'\.width\s*=\s*(\d+)', body)[1]),
                    reference_height=int(re.search(r'\.height\s*=\s*(\d+)', body)[1]))
        if key in NATIVE:
            native_id = ['brendan', 'may', 'red', 'leaf', 'brendan_rs', 'may_rs', 'wally', 'steven'].index(NATIVE[key])
            item.update(image=None, source='native',
                        native_path='graphics/trainers/back_pics/' + NATIVE[key] + '.png',
                        native_back_pic_id=native_id, frame=0)
        result.append(item)
    if len(result) != 84 or len({e['graphics_id'] for e in result}) != 84:
        raise ValueError('Expected exactly 84 unique playable coop characters')
    return result


def prepare(upstream):
    ASSETS.mkdir(parents=True, exist_ok=True)
    (LOCAL / 'references').mkdir(parents=True, exist_ok=True)
    entries = roster(upstream)
    for entry in entries:
        source = Image.open(upstream / entry['reference_path']).convert('RGBA')
        # Preserve the three directional source frames at nearest-neighbor scale.
        w, h = entry['reference_width'], entry['reference_height']
        source = source.crop((0, 0, min(source.width, 3*w), h))
        source.resize((source.width * 8, source.height * 8), Image.Resampling.NEAREST).save(
            LOCAL / 'references' / (entry['symbol'].removeprefix('OBJ_EVENT_GFX_').lower() + '.png'))
    style = Image.open(upstream / 'graphics/trainers/back_pics/brendan.png')
    style = style.crop((0, 0, 64, 64)).convert('RGBA')
    style.resize((512, 512), Image.Resampling.NEAREST).save(LOCAL / 'references/style.png')
    # Manifest deliberately states pending status until every generated drawing is present.
    manifest = dict(version=1, width=64, height=64, visible_colors=15,
                    native_root='build/upstream', status='in_progress', entries=entries)
    manifest_path = ASSETS / 'manifest.json'
    if manifest_path.exists():
        previous = json.loads(read(manifest_path))
        if [(e['symbol'], e['graphics_id']) for e in previous['entries']] != [
                (e['symbol'], e['graphics_id']) for e in entries]:
            raise ValueError('Existing manifest differs from the playable roster; reconcile it explicitly')
    else:
        manifest_path.write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    prompts = {e['symbol']: PROMPT.format(name=e['name']) for e in entries if e['source']=='generated'}
    # Final prompts can include targeted revisions. Preparing references again
    # must not replace their recorded provenance with the initial prompt template.
    prompts_path = ASSETS / 'prompts.json'
    if not prompts_path.exists():
        prompts_path.write_text(json.dumps(prompts, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(entries))


def normalize(source, target):
    """Production resize/palette reduction; never synthesizes or redraws art."""
    src = Image.open(source).convert('RGBA')
    alpha = src.getchannel('A').point(lambda p: 255 if p >= 128 else 0)
    box = alpha.getbbox()
    if not box:
        raise ValueError('Generated image contains no opaque character')
    src.putalpha(alpha)
    src = src.crop(box)
    ratio = min(60/src.width, 62/src.height)
    src = src.resize((max(1, round(src.width*ratio)), max(1, round(src.height*ratio))), Image.Resampling.NEAREST)
    canvas = Image.new('RGBA', (64, 64))
    canvas.paste(src, ((64-src.width)//2, 64-src.height))
    # Fit the palette to visible pixels only. Reserve a red and a white so the
    # small battle ball remains readable even on a blue/green/brown character.
    opaque = [p[:3] for p in canvas.getdata() if p[3]]
    samples = Image.new('RGB', (len(opaque), 1))
    samples.putdata(opaque)
    fitted = samples.quantize(colors=13, method=Image.Quantize.MEDIANCUT,
                             dither=Image.Dither.NONE)
    palette = fitted.getpalette()[:39] + [224,48,56,248,248,248]
    colors = [tuple(palette[i:i+3]) for i in range(0,45,3)]
    color_indices = {p: 1 + min(range(15), key=lambda i: sum((p[c]-colors[i][c])**2 for c in range(3)))
                     for p in set(opaque)}
    out = Image.new('P', (64, 64))
    out.putpalette([0,0,0]+palette+[0]*(768-48))
    out.putdata([color_indices[p[:3]] if p[3] else 0 for p in canvas.getdata()])
    out.save(target, transparency=0, optimize=False)


def check(upstream):
    manifest_path = ASSETS / 'manifest.json'
    manifest = json.loads(read(manifest_path))
    expected = roster(upstream)
    assert [(e['symbol'], e['graphics_id']) for e in expected] == [
        (e['symbol'], e['graphics_id']) for e in manifest['entries']]
    missing = []
    images = []
    for e in manifest['entries']:
        if e['source'] == 'native':
            im = Image.open(upstream / e['native_path']).crop((0, 0, 64, 64)).convert('RGBA')
        elif not (ASSETS / e['image']).is_file():
            missing.append(e['symbol'])
            continue
        else:
            im = Image.open(ASSETS / e['image']).convert('RGBA')
            assert im.size == (64,64), e['symbol']
            colors = {p[:3] for p in im.getdata() if p[3]}
            assert len(colors) <= 15, (e['symbol'], len(colors))
            assert set(im.getchannel('A').getdata()) == {0,255}, e['symbol']
        images.append((e, im))
    for page in range((len(images)+15)//16):
        sheet = Image.new('RGB', (4*240, 4*240), '#d7dce5')
        comparison = Image.new('RGB', (4*240, 4*310), '#d7dce5')
        draw = ImageDraw.Draw(sheet)
        compare_draw = ImageDraw.Draw(comparison)
        for i, (e,im) in enumerate(images[page*16:(page+1)*16]):
            x,y = (i%4)*240, (i//4)*240
            sheet.paste(im.resize((192,192),Image.Resampling.NEAREST), (x+24,y+8),
                        im.getchannel('A').resize((192,192),Image.Resampling.NEAREST))
            draw.text((x+8,y+205), str(e['graphics_id'])+' '+e['name'], fill='black')
            draw.text((x+8,y+222), e['source'], fill='black')
            cy = (i//4)*310
            comparison.paste(im.resize((192,192),Image.Resampling.NEAREST), (x+24,cy+8),
                             im.getchannel('A').resize((192,192),Image.Resampling.NEAREST))
            compare_draw.text((x+8,cy+205), str(e['graphics_id'])+' '+e['name'], fill='black')
            ref = Image.open(upstream / e['reference_path']).convert('RGBA')
            rw,rh = e['reference_width'],e['reference_height']
            ref = ref.crop((0,0,min(ref.width,rw*3),rh))
            ref = ref.resize((ref.width*2,ref.height*2),Image.Resampling.NEAREST)
            comparison.paste(ref,(x+(240-ref.width)//2,cy+230),ref)
        sheet.save(LOCAL / ('contact-%02d.png'%page))
        comparison.save(LOCAL / ('compare-%02d.png'%page))
    manifest['status'] = 'complete' if not missing else 'in_progress'
    manifest_path.write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    report = dict(total=len(expected), native=8, generated_present=len(images)-8,
                  missing=missing, status=manifest['status'])
    (LOCAL/'validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=['prepare','normalize','check'])
    parser.add_argument('--upstream',type=Path,default=ROOT/'build/upstream')
    parser.add_argument('--source',type=Path)
    parser.add_argument('--target',type=Path)
    args=parser.parse_args()
    if args.action == 'prepare': prepare(args.upstream)
    elif args.action == 'normalize': normalize(args.source,args.target)
    else: check(args.upstream)
