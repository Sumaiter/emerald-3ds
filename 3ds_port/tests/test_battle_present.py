"""Real presentation core, two transport endpoints and video ownership bodies."""
import argparse, json, os, re, shlex, subprocess, tempfile
from pathlib import Path
from test_coop_bike_hooks import function, UPSTREAM, REPO
from test_coop_spectator import API
from test_coop_spectator_benchmark import png_indices
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--cc',default='gcc');a=parser.parse_args()
    cc=shlex.split(a.cc)
    manifest=(ROOT/'3ds_port/src/3ds_battle_present_assets.inc').read_text()
    groups=json.loads((UPSTREAM/'data/maps/map_groups.json').read_text())
    layouts=json.loads((UPSTREAM/'data/layouts/layouts.json').read_text())['layouts']
    counts=','.join(str(len(groups[g])) for g in groups['group_order'])
    assert '{'+counts+'}' in manifest
    assert re.findall(r'\{(\d+),(\d+)\}',manifest)==[('0','0')]+[(str(l['width']),str(l['height'])) for l in layouts]
    flags=['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-DCTR_BATTLE_PRESENTATION','-I'+str(ROOT/'3ds_port/include'),'-I'+str(ROOT/'3ds_port/src')]
    with tempfile.TemporaryDirectory(prefix='present-',dir=os.environ.get('TMPDIR')) as tmp:
        tmp=Path(tmp);objs=[]
        for e in 'AB':
            names={'CtrCoopSpectator_'+k:e+k for k in API}
            names.update({'LocalLink_TrySendLowPriority':e+'Send','LocalLink_Milliseconds':e+'Now'})
            for x,y in [('RequestCapture','CaptureRequest'),('TakeCapture','CaptureTake'),('CancelCapture','CaptureCancel'),('Display','Display')]: names['CtrVideo_Present'+x]=e+y
            obj=tmp/(e+'.o');subprocess.run(cc+flags+['-D'+k+'='+v for k,v in names.items()]+['-c',str(ROOT/'3ds_port/src/3ds_coop_spectator.c'),'-o',str(obj)],check=True);objs.append(str(obj))
        video=(ROOT/'3ds_port/src/3ds_video.c').read_text(encoding='utf-8')
        dims=video[video.index('static const uint8_t sObjDims'):video.index('/* A visible OAM')]
        (tmp/'present_renderer.inc').write_text(dims+function(video,'OamPlace'))
        w,h,pixels,palette=png_indices(UPSTREAM/'graphics/battle_anims/backgrounds/psychic.png')
        tiles=bytearray()
        for ty in range(0,h,8):
            for tx in range(0,w,8):
                for y in range(8):
                    for x in range(0,8,2):tiles.append(pixels[(ty+y)*w+tx+x]|pixels[(ty+y)*w+tx+x+1]<<4)
        assert len(tiles)<=32768 and max(pixels)<16
        colours=[(r>>3)|((g>>3)<<5)|((b>>3)<<10) for r,g,b in palette[:16]]
        (tmp/'present_attack.inc').write_text('static const uint8_t attackTiles[]={' + ','.join(map(str,tiles))+'};\nstatic const uint16_t attackPalette[]={' + ','.join(map(str,colours))+'};\n')
        exe=tmp/'present.exe'
        subprocess.run(cc+flags+['-I'+str(tmp),str(ROOT/'3ds_port/tests/battle_present_test.c'),str(ROOT/'3ds_port/src/3ds_battle_present.c')]+objs+['-o',str(exe)],check=True)
        subprocess.run([str(exe)],cwd=REPO,check=True)
if __name__=='__main__':main()
