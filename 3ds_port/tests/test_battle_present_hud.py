"""Original HD HUD composition and isolated remote slots using shipped HUD art."""
import argparse, os, re, shlex, subprocess, tempfile
from pathlib import Path
from test_coop_bike_hooks import function, UPSTREAM, REPO
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--cc',default='gcc');a=p.parse_args();cc=shlex.split(a.cc)
 hud=(ROOT/'3ds_port/src/3ds_battle_hud.c').read_text(encoding='utf-8')
 with tempfile.TemporaryDirectory(prefix='present-hud-',dir=os.environ.get('TMPDIR')) as d:
  d=Path(d)
  initial=hud[hud.index('#define ART_MAGIC'):hud.index('static void LoadArt')]
  funcs=['LoadArt','CtrBattleHud_Ready','Over','BlitPart','Blit','BlitOverNothing','FillWidth','BlitFill','BlitLabel','BlitLabelCentred','Trace','TilePixel','MakeLabels','FontGlyphs','GlyphWidth','GlyphRow','NextGlyph','TextWidth','DrawGlyph','DrawText','DrawTextRight','Digits','Px','ComposeValues']
  remote=hud[hud.index('static CtrPresentHud sPresentValues'):hud.index('void CtrBattleHud_Clear')]
  remote=remote[:remote.rindex('#endif')]
  constants='\n'.join(re.findall(r'^#define (?:FILL_|TRACE_).*$',hud,re.M))+'\n'
  (d/'present_hud.inc').write_text(initial+constants+''.join(function(hud,n) for n in funcs)+remote+function(hud,'CtrBattleHud_Uploaded'))
  exe=d/'hud.exe';flags=['-O2','-std=gnu99','-Wall','-Wextra','-Werror','-DCTR_BATTLE_PRESENTATION','-I'+str(ROOT/'3ds_port/include'),'-I'+str(d),'-iquote',str(UPSTREAM/'include')]
  subprocess.run(cc+flags+[str(ROOT/'3ds_port/tests/battle_present_hud_test.c'),str(ROOT/'3ds_port/src/3ds_battle_present.c'),'-o',str(exe)],check=True);subprocess.run([str(exe)],cwd=REPO,check=True)
if __name__=='__main__':main()
