"""Native healthbox creation, both controller intros, slide and HD/OAM identity."""
import argparse,os,re,shlex,subprocess,tempfile
from pathlib import Path
from test_coop_bike_hooks import function,UPSTREAM,REPO
PRE=r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int16_t s16;typedef int32_t s32;typedef bool bool8;typedef bool bool32;
#define TRUE true
#define FALSE false
#include "constants/battle.h"
#include "3ds_battle_hud.h"
#define MAX_SPRITES 64
#define POKEMON_NAME_LENGTH 10
#define EOS 255
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 160
#define ST_OAM_SQUARE 0
#define SUBSPRITES_IGNORE_PRIORITY 2
#define BATTLE_SIDE_SHIFT(side) ((side)?-8:2)
#define HEALTHBOX_GFX_1 1
#define HEALTHBOX_ALL 0
#define TILE_SIZE_4BPP 32
#define TRACKS_ALL 0xffff
#define CpuCopy32(a,b,c) ((void)(a),(void)(b),(void)(c))
#define OBJ_VRAM0 ((uintptr_t)vram)
static u8 vram[32768];
struct SpriteTemplate {unsigned tile;};
struct Sprite {struct {unsigned shape,tileNum,affineParam,priority;} oam;const struct SpriteTemplate *template;unsigned subspriteMode;bool inUse,invisible;s16 x,y,x2,y2,data[8];void (*callback)(struct Sprite*);};
static struct Sprite gSprites[MAX_SPRITES];
static const struct SpriteTemplate sHealthboxPlayerSpriteTemplates[2]={{0},{64}},sHealthboxOpponentSpriteTemplates[2]={{128},{192}},sHealthbarSpriteTemplates[4]={{256},{264},{273},{281}};
static const unsigned sHealthBar_SubspriteTables[2]={0,0};
static u8 gHealthboxSpriteIds[4],gBattlerPositions[4]={0,1,2,3},gBattlerPartyIndexes[4]={0,0,3,3},gBattleControllerData[4],gBattlerSpriteIds[4];
static unsigned gActiveBattler,gAbsentBattlerFlags,gBattleTypeFlags=BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI|BATTLE_TYPE_LINK;
static const unsigned gBitTable[4]={1,2,4,8};
static struct {unsigned hp;} gPlayerParty[6];
static struct {bool ballAnimActive,triedShinyMonAnim,finishedShinyMonAnim,healthboxSlideInStarted,waitForCry,bgmRestored;u8 introEndDelay;} hbs[4];
static struct {bool introAnimActive;} animation;
static struct {typeof(hbs[0])*healthBoxesData;typeof(animation)*animationData;} bs={hbs,&animation},*gBattleSpritesDataPtr=&bs;
static unsigned gMPlayInfo_BGM;
static void (*gBattlerControllerFuncs[4])(void);
static u8 gBattleBufferA[4][512];
static unsigned activeRole;
static unsigned GetMultiplayerId(void){return activeRole;}
static void PrepareBufferDataTransferLink(unsigned a,unsigned n,void*p){(void)a;(void)n;(void)p;}
static void PlayerBufferRunCommand(void){}
static void LinkPartnerBufferRunCommand(void){}
static unsigned gBattleControllerExecFlags;
#define B_COMM_CONTROLLER_IS_DONE 2
#define CONTROLLER_INTROTRAINERBALLTHROW 47
#define CONTROLLER_TERMINATOR_NOP 56
#define MON_DATA_HP 1
static unsigned GetMonData(const void*p,unsigned f){(void)f;return ((const typeof(gPlayerParty[0])*)p)->hp;}
static bool CtrCoop_IsDoubleBattle(void){return true;}
static bool IsDoubleBattle(void){return !!(gBattleTypeFlags&BATTLE_TYPE_DOUBLE);}
static unsigned GetBattlerSide(unsigned b){return b&1;}
static unsigned GetBattlerPosition(unsigned b){return gBattlerPositions[b];}
static void SpriteCallbackDummy(struct Sprite*s){(void)s;}
static u8 CreateSpriteAt(u8 i,const struct SpriteTemplate*t,s16 x,s16 y,u8 p){(void)p;gSprites[i]=(struct Sprite){.template=t,.inUse=true,.x=x,.y=y,.callback=SpriteCallbackDummy};gSprites[i].oam.tileNum=t->tile;return i;}
static void SetSubspriteTables(struct Sprite*s,const void*t){(void)s;(void)t;}
static const u8 *GetHealthboxElementGfxPtr(unsigned i){(void)i;return vram;}
static void DestroySprite(struct Sprite*s){s->inUse=false;}
static void UpdateHealthboxAttribute(unsigned id,const void*p,unsigned kind){(void)id;(void)p;(void)kind;}
static void TryShinyAnimation(unsigned b,const void*p){(void)p;hbs[b].triedShinyMonAnim=true;hbs[b].finishedShinyMonAnim=true;}
static bool IsCryPlayingOrClearCrySongs(void){return false;}
static void m4aMPlayContinue(const void*p){(void)p;}
static void m4aMPlayVolumeControl(const void*p,unsigned a,unsigned b){(void)p;(void)a;(void)b;}
static void Intro_WaitForShinyAnimAndHealthbox(void){}
static void Intro_WaitForHealthbox(void){}
typedef struct {u8 kind,sprite,battler,bar;const struct SpriteTemplate *boxTemplate,*barTemplate;u8 name[11],gender;} Slot;
static Slot sSlots[4];static CtrHudBox sBoxes[4];static u32 sPixels[4][CTR_HUD_W*CTR_HUD_H];
void CtrBattleHud_Clear(void){for(unsigned i=0;i<4;++i)sBoxes[i].used=false;}
void CtrBattleHud_Prepare(void){}
static void MarkAll(Slot*s){(void)s;}
static bool SlotAlive(int);
static const CtrHudBox *sHud=sBoxes;static bool sHudReady[4];static int8_t sHudOamBox[128];
static u16 rawOam[128*4];static struct {u16*oam;} sMemory={rawOam};
#define hMain_HealthBarSpriteId data[5]
#define hMain_Battler data[6]
#define hOther_HealthBoxSpriteId data[5]
#define hBar_HealthBoxSpriteId data[5]
#define hBar_Data6 data[6]
static void SpriteCB_HealthBoxOther(struct Sprite*);
static void SpriteCB_HealthBar(struct Sprite*);
static void SpriteCB_HealthboxSlideIn(struct Sprite*);
static void SpriteCB_HealthboxSlideInDelayed(struct Sprite*);
static void UpdateSpritePos(u8,s16,s16);
static void StartHealthboxSlideIn(u8);
#define sSpeedX data[0]
#define sSpeedY data[1]
#define sDelayTimer data[1]
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s role=%u delay=%u bank=%u\n",__LINE__,#c,role,delay,b);exit(1);}}while(0)
'''
TEST=r'''
int main(void){
 for(unsigned role=0;role<2;++role)for(unsigned delay=0;delay<8;++delay){
  unsigned b=0;(void)SpriteCB_HealthBar;memset(gSprites,0,sizeof(gSprites));memset(hbs,0,sizeof(hbs));
  for(b=0;b<4;++b)gHealthboxSpriteIds[b]=CreateBattlerHealthboxSprites(b);
  for(b=0;b<6;++b)gPlayerParty[b].hp=20;
  for(b=0;b<4;++b){InitBattlerHealthboxCoords(b);if(b&1)SetHealthboxSpriteVisible(gHealthboxSpriteIds[b]);gBattleControllerData[b]=CreateSpriteAt(10+b,&sHealthboxPlayerSpriteTemplates[0],0,0,0);gBattlerSpriteIds[b]=CreateSpriteAt(20+b,&sHealthboxPlayerSpriteTemplates[0],0,0,0);}
  unsigned local=role*2,remote=local^2;
  for(unsigned frame=0;frame<100;++frame){
   if(frame>=delay && frame<delay+2){gActiveBattler=remote;Intro_ShowHealthbox();}
   if(frame==delay+3){gActiveBattler=local;Intro_TryShinyAnimShowHealthbox();}
   for(b=0;b<64;++b)if(gSprites[b].inUse)gSprites[b].callback(&gSprites[b]);
  }
  // A lost HD slot / hidden box must not survive the intro's commit barrier.
  // This recovery probe models the reported symptom, not its unknown origin.
  b=local;sBoxes[b].used=false;gSprites[gHealthboxSpriteIds[b]].invisible=true;
  activeRole=role;gActiveBattler=local;gBattleBufferA[local][0]=CONTROLLER_INTROTRAINERBALLTHROW;PlayerBufferExecCompleted();
  gActiveBattler=remote;gBattleBufferA[remote][0]=CONTROLLER_INTROTRAINERBALLTHROW;LinkPartnerBufferExecCompleted();
  memset(rawOam,0,sizeof(rawOam));for(b=0;b<128;++b)rawOam[b*4]=160;
  unsigned n=0;
  for(b=0;b<4;++b){
   struct Sprite*s=&gSprites[gHealthboxSpriteIds[b]];
   CHECK(s->inUse&&!s->invisible&&s->x2==0);CHECK(SlotAlive(b));
   CHECK(!gSprites[s->data[5]].invisible&&!gSprites[s->oam.affineParam].invisible);
   sHudReady[b]=true;rawOam[n*4]=s->y+s->y2;rawOam[n*4+1]=s->x+s->x2;rawOam[n*4+2]=s->oam.tileNum;++n;
  }
  for(b=0;b<128;++b)sHudOamBox[b]=HudBoxOfTile(rawOam[b*4+2]&1023);
  for(b=0;b<4;++b)CHECK(HudLeftHalf(b)==(int)b);
 }
 puts("native double healthbox creation + local/partner intro + native slide + SlotAlive/OAM identity, both roles and 8 offsets passed");return 0;
}
'''
def main():
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=UPSTREAM/'src');p.add_argument('--cc',default='gcc');p.add_argument('--check-mutations',action='store_true');a=p.parse_args()
 src={n:(a.source/(n+'.c')).read_text(encoding='utf-8') for n in ['battle_interface','sprite','pokeball','battle_controller_player','battle_controller_link_partner']}
 hud=(REPO/'3ds_port/src/3ds_battle_hud.c').read_text();vid=(REPO/'3ds_port/src/3ds_video.c').read_text()
 funcs=''.join(function(src['sprite'],n) for n in ['CreateSprite','CreateSpriteAtEnd'])
 funcs+=function(hud,'RegisterHealthbox')+function(hud,'CtrBattleHud_BoxCreated')+function(hud,'SlotAlive')+function(hud,'CtrBattleHud_RefreshBox')
 funcs+=''.join(function(src['battle_interface'],n) for n in ['CreateBattlerHealthboxSprites','SpriteCB_HealthBar','SpriteCB_HealthBoxOther','SetHealthboxSpriteVisible','UpdateSpritePos','InitBattlerHealthboxCoords'])
 funcs+=''.join(function(src['pokeball'],n) for n in ['StartHealthboxSlideIn','SpriteCB_HealthboxSlideInDelayed','SpriteCB_HealthboxSlideIn'])
 funcs+=function(src['battle_controller_player'],'PlayerHasTwoIntroMons')+function(src['battle_controller_player'],'Intro_TryShinyAnimShowHealthbox')+function(src['battle_controller_link_partner'],'Intro_ShowHealthbox')
 funcs+=function(src['battle_interface'],'CtrCoop_FinalizeIntroHealthbox')+function(src['battle_controller_player'],'PlayerBufferExecCompleted')+function(src['battle_controller_link_partner'],'LinkPartnerBufferExecCompleted')
 funcs+=function(vid,'HudBoxOfTile')+function(vid,'HudLeftHalf')
 with tempfile.TemporaryDirectory(prefix='coop-hp-',dir=os.environ.get('TMPDIR')) as d:
  for name,body,wanted in [('fixed',funcs,True)]+([('no-finalize',funcs.replace('        CtrCoop_FinalizeIntroHealthbox();','        (void)0;'),False)] if a.check_mutations else []):
   c=Path(d)/(name+'.c');c.write_text(PRE+body+TEST);exe=c.with_suffix('.exe')
   subprocess.run(shlex.split(a.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-DPLATFORM_3DS','-I'+str(REPO/'3ds_port/include'),'-iquote'+str(a.source.parent/'include'),str(c),'-o',str(exe)],check=True,timeout=120)
   r=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
   if (r.returncode==0)!=wanted:raise RuntimeError(name+' unexpected result\n'+r.stdout+r.stderr)
   print(r.stdout.strip() if wanted else 'detected mutation: missing intro HP commit',flush=True)
if __name__=='__main__':main()
