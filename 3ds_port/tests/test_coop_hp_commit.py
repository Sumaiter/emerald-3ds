"""Stable co-op intro barrier uses the same native HP rebuild as menu return.

The disturbed-intro cases model the reported missing box and its menu recovery;
they do not claim to isolate the original disturbance's exact instruction.
"""
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from test_coop_bike_hooks import function, UPSTREAM, REPO
from test_coop_hp_lifecycle import PRE

EXTRA = r'''
#define HEALTHBOX_SAFARI_ALL_TEXT 1
static typeof(gPlayerParty) gEnemyParty;
static unsigned gBattlersCount=4,rebuilds,logs;
static bool sCoopIntroHealthboxesPending,cooperative=true;
static void CreateHealthboxSprite(u8);
static u8 CreateSafariPlayerHealthboxSprites(void){abort();}
static void DummyBattleInterfaceFunc(u8 id,bool b){(void)id;(void)b;}
static void CtrLog_Write(unsigned category,const char*format,...){(void)category;(void)format;++logs;}
#define CTR_LOG_GAME 1
'''
TEST = r'''
int main(void){
 (void)gBattleControllerData;(void)gBattlerSpriteIds;(void)gAbsentBattlerFlags;
 (void)gBitTable;(void)gBattleSpritesDataPtr;(void)gMPlayInfo_BGM;
 (void)gBattlerControllerFuncs;(void)gBattleBufferA;(void)gBattleControllerExecFlags;
 for(unsigned role=0;role<2;++role)for(unsigned fault=0;fault<4;++fault){
  unsigned delay=0,b=0;activeRole=role;cooperative=true;
  memset(gSprites,0,sizeof(gSprites));memset(hbs,0,sizeof(hbs));
  for(b=0;b<6;++b)gPlayerParty[b].hp=20;
  for(b=0;b<6;++b)gEnemyParty[b].hp=20;
  for(b=0;b<4;++b)CreateHealthboxSprite(b);
  CtrCoop_BeginIntroHealthboxes();
  unsigned missing=2;
  if(fault==0)gSprites[gHealthboxSpriteIds[missing]].invisible=true;
  if(fault==1)gSprites[gHealthboxSpriteIds[missing]].x2=115;
  if(fault==2)sBoxes[missing].used=false;
  if(fault==3){gSprites[gHealthboxSpriteIds[missing]].x=240;gSprites[gHealthboxSpriteIds[missing]].y=160;}
  /* Both possible first CHOOSEACTION deliveries use the actual entry hook. */
  gActiveBattler=0;
  if(role)LinkPartnerHandleChooseAction();else PlayerHandleChooseAction();
  CHECK(!sCoopIntroHealthboxesPending);
  for(b=0;b<4;++b){
   u8 id=gHealthboxSpriteIds[b];CHECK(gSprites[id].inUse&&!gSprites[id].invisible&&SlotAlive(b));
   CHECK(gSprites[id].x2==0&&gSprites[id].y2==0);
   CHECK(gSprites[id].x==(b==0?159:b==2?171:b==1?44:32));
   CHECK(gSprites[id].y==(b==0?76:b==2?101:b==1?11:36));
   CHECK(!gSprites[gSprites[id].data[5]].invisible&&!gSprites[gSprites[id].oam.affineParam].invisible);
   sHudReady[b]=true;rawOam[b*4+2]=gSprites[id].oam.tileNum;
  }
  for(b=0;b<4;++b)sHudOamBox[b]=HudBoxOfTile(rawOam[b*4+2]);
  for(b=0;b<4;++b)CHECK(HudLeftHalf(b)==(int)b);
  unsigned before=logs;gActiveBattler=2;
  if(role)PlayerHandleChooseAction();else LinkPartnerHandleChooseAction();
  CHECK(logs==before); /* one reconstruction per endpoint, no next-turn forcing */
  /* Legitimate later faint hiding remains hidden on the next action. */
  gSprites[gHealthboxSpriteIds[2]].invisible=true;PlayerHandleChooseAction();
  CHECK(gSprites[gHealthboxSpriteIds[2]].invisible&&logs==before);
 }
 /* Normal offline/link-native menus are not reconstructed by this path. */
 unsigned role=0,delay=0,b=0;cooperative=false;CtrCoop_BeginIntroHealthboxes();
 unsigned before=logs;CtrCoop_CommitIntroHealthboxes();CHECK(logs==before);
 (void)rebuilds;
 puts("native menu HP reconstruction before first action: both endpoint entries, hidden/offscreen/unregistered/parked box, once-only and later faint preserved passed");return 0;
}
'''

def main():
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=UPSTREAM/'src');p.add_argument('--cc',default='gcc');p.add_argument('--check-mutations',action='store_true');a=p.parse_args()
 src={n:(a.source/(n+'.c')).read_text(encoding='utf-8') for n in ['battle_interface','sprite','pokeball','battle_controller_player','battle_controller_link_partner','reshow_battle_screen','battle_gfx_sfx_util']}
 hud=(REPO/'3ds_port/src/3ds_battle_hud.c').read_text()
 pre=PRE.replace('static bool CtrCoop_IsDoubleBattle(void){return true;}','static bool cooperative;\nstatic bool CtrCoop_IsDoubleBattle(void){return cooperative;}')
 extra=EXTRA.replace(',cooperative=true','')
 funcs=''.join(function(src['sprite'],n) for n in ['CreateSprite','CreateSpriteAtEnd'])
 funcs+=''.join(function(hud,n) for n in ['RegisterHealthbox','CtrBattleHud_BoxCreated','SlotAlive'])
 funcs+=''.join(function(src['battle_interface'],n) for n in ['CreateBattlerHealthboxSprites','SpriteCB_HealthBar','SpriteCB_HealthBoxOther','SetHealthboxSpriteVisible','SetHealthboxSpriteInvisible','DestoryHealthboxSprite','UpdateSpritePos','InitBattlerHealthboxCoords'])
 funcs+=''.join(function(src['pokeball'],n) for n in ['StartHealthboxSlideIn','SpriteCB_HealthboxSlideInDelayed','SpriteCB_HealthboxSlideIn'])
 funcs+=''.join(function(src['reshow_battle_screen'],n) for n in ['CreateHealthboxSprite','CtrCoop_BeginIntroHealthboxes','CtrCoop_CommitIntroHealthboxes'])
 video=(REPO/'3ds_port/src/3ds_video.c').read_text()
 funcs+=function(video,'HudBoxOfTile')+function(video,'HudLeftHalf')
 # Execute the real choose-action prefix before its normal menu/task setup;
 # the unmodified remainder is independently built in the full ARM TU.
 for n,file in [('PlayerHandleChooseAction','battle_controller_player'),('LinkPartnerHandleChooseAction','battle_controller_link_partner')]:
  body=function(src[file],n);hook='    CtrCoop_CommitIntroHealthboxes();'
  assert body.index(hook)<body.index('gBattlerControllerFuncs' if n.startswith('Player') else 'LinkPartnerBufferExecCompleted')
  funcs+='static void '+n+'(void){\n'+body.split('#ifdef PLATFORM_3DS\n',1)[1].split('#endif',1)[0]+'}\n'
 assert 'CtrCoop_BeginIntroHealthboxes();' in function(src['battle_gfx_sfx_util'],'BattleInitAllSprites')
 variants=[('fixed',funcs,True)]
 if a.check_mutations:variants.append(('missing-initial-commit',funcs.replace('    CtrCoop_CommitIntroHealthboxes();','    (void)0;'),False))
 with tempfile.TemporaryDirectory(prefix='coop-hp-commit-',dir=os.environ.get('TMPDIR')) as d:
  for name,body,wanted in variants:
   c=Path(d)/(name+'.c');exe=c.with_suffix('.exe');c.write_text(pre+extra+body+TEST,encoding='utf-8')
   subprocess.run(shlex.split(a.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-DPLATFORM_3DS','-I'+str(REPO/'3ds_port/include'),'-iquote'+str(a.source.parent/'include'),str(c),'-o',str(exe)],check=True,timeout=120)
   r=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
   if (r.returncode==0)!=wanted:raise RuntimeError(name+' unexpected result\n'+r.stdout+r.stderr)
   print(r.stdout.strip() if wanted else 'detected mutation: missing initial HP commit',flush=True)
if __name__=='__main__':main()
