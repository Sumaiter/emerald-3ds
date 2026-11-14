"""Compare real old/new endpoint modules on asset-derived battle payloads.

No emulator/screenshot or game save is needed. The 2D fixture assembles native
terrain tilemaps, palettes, Pokemon and font pixels; the voxel fixture adds a
deterministic detailed perspective scene beneath the same original text.
"""
import argparse
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile
import zlib

from test_coop_spectator import API, MOCKS, ROOT
from test_coop_bike_hooks import UPSTREAM


def png_indices(path):
    data = path.read_bytes()
    at, compressed, palette = 8, b'', []
    while at < len(data):
        size = int.from_bytes(data[at:at+4], 'big')
        kind, body = data[at+4:at+8], data[at+8:at+8+size]
        if kind == b'IHDR':
            w, h, bits, color, _, _, interlace = struct.unpack('>IIBBBBB', body)
            assert color == 3 and bits in (1, 2, 4, 8) and interlace == 0
        elif kind == b'PLTE':
            palette = list(zip(body[::3], body[1::3], body[2::3]))
        elif kind == b'IDAT':
            compressed += body
        at += size+12
    raw, stride, previous, pixels = zlib.decompress(compressed), (w*bits+7)//8, None, []
    for y in range(h):
        filt, row = raw[y*(stride+1)], bytearray(raw[y*(stride+1)+1:(y+1)*(stride+1)])
        for x in range(stride):
            a = row[x-1] if x else 0
            b = previous[x] if previous else 0
            c = previous[x-1] if previous and x else 0
            p = a+b-c
            pa, pb, pc = abs(p-a), abs(p-b), abs(p-c)
            predictor = (0, a, b, (a+b)//2, a if pa <= pb and pa <= pc else b if pb <= pc else c)[filt]
            row[x] = (row[x]+predictor) & 255
        pixels.extend((row[x*bits//8] >> (8-bits-(x*bits%8))) & ((1 << bits)-1) for x in range(w))
        previous = row
    return w, h, pixels, palette


def palette(path):
    result = []
    for (c,) in struct.iter_unpack('<H', path.read_bytes()):
        r, g, b = (c & 31)*255//31, ((c >> 5) & 31)*255//31, ((c >> 10) & 31)*255//31
        result.append((r & 224) | ((g >> 3) & 28) | (b >> 6))
    return result


def texel(tiles, tile, x, y):
    p = tiles[tile*32+y*4+x//2]
    return (p >> (4*(x & 1))) & 15


def fixtures(folder):
    gfx = UPSTREAM / 'graphics'
    terrain = gfx / 'battle_environment/tall_grass'
    tiles = (terrain / 'tiles.4bpp').read_bytes()
    tilemap = list(struct.unpack('<2048H', (terrain / 'map.bin').read_bytes()))
    colors = palette(terrain / 'palette.gbapal')
    fw, _, font, fp = png_indices(gfx / 'fonts/latin_normal.png')
    ink = min(range(len(fp)), key=lambda n: sum(fp[n]))
    base = bytearray([255]*(240*160))
    for y in range(112):
        for x in range(240):
            entry = tilemap[y//8*32+x//8]
            px, py = x % 8, y % 8
            if entry & 1024:
                px = 7-px
            if entry & 2048:
                py = 7-py
            # LoadBattleBackgrounds installs these three banks at BG_PLTT_ID(2).
            bank = (entry >> 12)-2
            assert 0 <= bank < 3
            base[y*240+x] = colors[bank*16+texel(tiles, entry & 1023, px, py)]

    def text(screen, message, x, y):
        for c in message:
            code = ord(c)-ord('A')+0xbb if c.isupper() else ord(c)-ord('a')+0xd5 if c.islower() else 0
            for yy in range(16):
                for xx in range(8):
                    if x+xx < 240 and y+yy < 160 and font[((code//16)*16+yy)*fw+(code % 16)*16+xx] == ink:
                        screen[(y+yy)*240+x+xx] = 0
            x += 8

    def actor(screen, species, side, x, y):
        data = (gfx / ('pokemon/'+species+'/'+side+'.4bpp')).read_bytes()
        pal = palette(gfx / ('pokemon/'+species+'/normal.gbapal'))
        for yy in range(64):
            for xx in range(64):
                index = texel(data, yy//8*8+xx//8, xx % 8, yy % 8)
                if index and 0 <= x+xx < 240 and 0 <= y+yy < 112:
                    screen[(y+yy)*240+x+xx] = pal[index]

    battle, voxel = bytearray(), bytearray()
    for f in range(6):
        screen = bytearray(base)
        actor(screen, 'treecko', 'back', 28+f % 3, 52)
        actor(screen, 'zigzagoon', 'front', 146-f % 3, 12)
        text(screen, 'Zigzachs setzt', 8, 118)
        text(screen, 'Tackle ein'[:5+f], 8, 134)
        full = bytearray(screen[y*160//240*240+x*240//400] for y in range(240) for x in range(400))
        battle.extend(full)
        # Detailed voxel-like perspective, intentionally more expensive than
        # tile art. HUD/text rows remain the exact same native font pixels.
        for y in range(167):
            for x in range(400):
                ground = y > 50
                v = ((x*23//(y+12) + y//7) % 17) if ground else y//9
                full[y*400+x] = ((2+(v % 4)) << 5) | ((3+v % 5) << 2) | (v % 4)
                if 120+f*2 <= x < 178+f*2 and 70 <= y < 132:
                    full[y*400+x] = screen[(y*160//240)*240+x*240//400]
        voxel.extend(full)
    paths = []
    for name, pixels in (('native-assets', battle), ('voxel-detail', voxel)):
        path = folder / (name+'.rgb332')
        path.write_bytes(pixels)
        paths.append((name, path))
    return paths


def run(cc, source, include, fixtures_, folder, label):
    flags = ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror', '-I'+str(include), '-I'+str(ROOT/'3ds_port/include')]
    objects = []
    for endpoint in ('A', 'B'):
        names = {'CtrCoopSpectator_'+n: endpoint+n for n in API}
        names.update({n: endpoint+replacement for n, replacement in MOCKS.items()})
        target = folder / (label+endpoint+'.o')
        subprocess.run(cc+flags+['-D'+k+'='+v for k, v in names.items()]+['-c', str(source), '-o', str(target)], check=True)
        objects.append(str(target))
    exe = folder / (label+('.exe' if os.name == 'nt' else '.out'))
    subprocess.run(cc+flags+[str(ROOT/'3ds_port/tests/coop_spectator_benchmark.c')]+objects+['-o', str(exe)], check=True)
    rows = []
    for name, path in fixtures_:
        result = subprocess.run([str(exe), str(path), name], capture_output=True, text=True)
        assert result.returncode == 0, result.stdout+result.stderr
        for line in result.stdout.splitlines():
            print(label+','+line, flush=True)
            rows.append(line.split(','))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    parser.add_argument('--baseline', type=Path, help='optional preserved old .c/.h directory for before/after comparison')
    parser.add_argument('--source', type=Path, default=ROOT/'3ds_port/src/3ds_coop_spectator.c')
    parser.add_argument('--fixtures', type=Path)
    args = parser.parse_args()
    cc = shlex.split(args.cc)
    with tempfile.TemporaryDirectory(prefix='spectator-bench-', dir=os.environ.get('TMPDIR')) as temp:
        folder = Path(temp)
        fixture_dir = args.fixtures or folder
        fixture_dir.mkdir(parents=True, exist_ok=True)
        samples = fixtures(fixture_dir)
        print('variant,payload,rtt_ms,busy,first_ms,changed_fps,mean_frame_age_ms,max_frame_age_ms,bytes_per_second,peak_queue,low_priority_refusals')
        old = run(cc, args.baseline/'3ds_coop_spectator.c', args.baseline, samples, folder, 'before') if args.baseline else []
        source = args.source/'3ds_coop_spectator.c' if args.source.is_dir() else args.source
        new = run(cc, source, source.parent.parent/'include', samples, folder, 'after')
        old = {tuple(r[:3]): r for r in old if r[0] != 'CODEC'}
        for row in new:
            if row[0] == 'CODEC':
                continue
            if old:
                previous = old[tuple(row[:3])]
                assert int(row[3]) < int(previous[3])/2, (previous, row)
                assert float(row[4]) > float(previous[4])*2, (previous, row)
            assert int(row[3]) < 1500 and float(row[4]) >= 1.5, row
            assert int(row[8]) <= 128, row
        assert any(int(r[9]) for r in new if r[0] != 'CODEC'), 'queue backpressure was never exercised'
        print('PASS: bounded queues, first image <1.5s and changed-frame rate >=1.5fps across all fixtures')
        if old:
            print('PASS: first frame >2x faster and changed-frame rate >2x across all delay/congestion fixtures')


if __name__ == '__main__':
    main()
