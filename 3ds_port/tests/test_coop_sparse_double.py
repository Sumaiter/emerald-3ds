"""Run native sparse-double intro tasks, including missing shiny/healthbox slots."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from test_coop_bike_hooks import function, UPSTREAM

HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef bool bool8;typedef bool bool32;
#define TRUE true
#define FALSE false
#define SPECIES_NONE 0
#define B_POSITION_PLAYER_RIGHT 2
#define B_POSITION_PLAYER_LEFT 0
#define BATTLE_PARTNER(b) ((b)^2)
#define BIT_FLANK 2
#define BATTLE_TYPE_DOUBLE 1
#define BATTLE_TYPE_MULTI 2
#define BATTLE_TYPE_LINK 4
#define ANIM_TAG_GOLD_STARS 0
#define HEALTHBOX_ALL 0
#define TRACKS_ALL 0
#define tStartTimer data[1]
#define tBattlerId data[0]
static unsigned gBattleTypeFlags,gAbsentBattlerFlags,gActiveBattler;
static const unsigned gBitTable[]={1,2,4,8};
static struct {unsigned species,hp;} gBattleMons[4],gPlayerParty[6];
static unsigned gBattlerPartyIndexes[4],gHealthboxSpriteIds[4]={0,1,2,3},gBattlerSpriteIds[4]={4,5,6,7},gBattleControllerData[4]={8,9,10,11};
static struct {unsigned data[2];} gTasks[1];
static u8 gBattleBufferA[4][2];
static void (*gBattlerControllerFuncs[4])(void);
static struct Sprite {void (*callback)(struct Sprite *);} gSprites[12];
static struct { bool triedShinyMonAnim,finishedShinyMonAnim,ballAnimActive,healthboxSlideInStarted,waitForCry,bgmRestored;unsigned introEndDelay;} health[4];
static struct {bool introAnimActive;} animation;
static struct {typeof(health) *unused;typeof(health[0]) *healthBoxesData;typeof(animation) *animationData;} state={0,health,&animation},*gBattleSpritesDataPtr=&state;
static unsigned sends,shinies,slides,completed;
static int gMPlayInfo_BGM;
static unsigned GetBattlerAtPosition(unsigned pos){return pos;}
static bool IsDoubleBattle(void){return gBattleTypeFlags&BATTLE_TYPE_DOUBLE;}
static void SpriteCallbackDummy(struct Sprite *s){(void)s;}
static void neverDone(struct Sprite *s){(void)s;}
static void Intro_DelayAndEnd(void){++completed;}
static void StartSendOutAnim(unsigned battler,bool unused){(void)unused;assert(!(gAbsentBattlerFlags&gBitTable[battler]));++sends;}
static void BattleLoadPlayerMonSpriteGfx(const void *p,unsigned b){(void)p;assert(!(gAbsentBattlerFlags&gBitTable[b]));}
static void DestroyTask(unsigned task){assert(task==0);}
static void DestroySprite(struct Sprite *s){assert(s!=&gSprites[10]);}
static void TryShinyAnimation(unsigned b,const void *p){(void)p;assert(!(gAbsentBattlerFlags&gBitTable[b]));++shinies;health[b].triedShinyMonAnim=health[b].finishedShinyMonAnim=true;}
static void UpdateHealthboxAttribute(unsigned b,const void *p,unsigned mode){(void)p;(void)mode;assert(!(gAbsentBattlerFlags&gBitTable[b]));}
static void StartHealthboxSlideIn(unsigned b){assert(!(gAbsentBattlerFlags&gBitTable[b]));++slides;}
static void SetHealthboxSpriteVisible(unsigned b){assert(!(gAbsentBattlerFlags&gBitTable[b]));}
static bool IsCryPlayingOrClearCrySongs(void){return false;}
static void m4aMPlayContinue(void *p){(void)p;}
static void m4aMPlayVolumeControl(void *p,unsigned t,unsigned v){(void)p;(void)t;(void)v;}
static void FreeSpriteTilesByTag(unsigned t){(void)t;}
static void FreeSpritePaletteByTag(unsigned t){(void)t;}
static void HandleLowHpMusicChange(const void *p,unsigned b){(void)p;assert(!(gAbsentBattlerFlags&gBitTable[b]));}
'''
TEST = r'''
int main(void){
    for(unsigned mode=0;mode<3;++mode){
        memset(health,0,sizeof(health));gAbsentBattlerFlags=0;gActiveBattler=0;
        gBattleTypeFlags=BATTLE_TYPE_DOUBLE;gBattleMons[0].species=258;gBattleMons[0].hp=10;
        gBattleMons[2]=gBattleMons[0];gBattlerPartyIndexes[0]=0;gBattlerPartyIndexes[2]=mode?1:0;
        if(mode==1)gBattleMons[2].hp=0;
        InitSparseDoubleBattlers();
        assert(!!(gAbsentBattlerFlags&4)==(mode!=2));
        if(mode==2){assert(PlayerHasTwoIntroMons());continue;}
        assert(!PlayerHasTwoIntroMons() && !gBattleMons[2].hp && !gBattleMons[2].species);
        for(unsigned i=0;i<12;++i)gSprites[i].callback=SpriteCallbackDummy;
        /* Poison all absent-slot sprite callbacks: waiting for one must fail. */
        gSprites[2].callback=gSprites[6].callback=gSprites[10].callback=neverDone;
        sends=shinies=slides=completed=0;gTasks[0].tStartTimer=31;gTasks[0].tBattlerId=0;
        Task_StartSendOutAnim(0);assert(sends==1);
        Intro_TryShinyAnimShowHealthbox();assert(shinies==1 && slides==1);
        assert(gBattlerControllerFuncs[0]==Intro_WaitForShinyAnimAndHealthbox);
        Intro_WaitForShinyAnimAndHealthbox();assert(gBattlerControllerFuncs[0]==Intro_DelayAndEnd);
        gBattlerControllerFuncs[0]();assert(completed==1);
    }
    /* Native multi remains one independent mon/controller per participant. */
    gBattleTypeFlags=BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI|BATTLE_TYPE_LINK;
    gAbsentBattlerFlags=0;InitSparseDoubleBattlers();assert(!gAbsentBattlerFlags && !PlayerHasTwoIntroMons());
    puts("one-Pokemon double: no duplicate, absent ball/shiny/healthbox never started or awaited; native multi unchanged");return 0;
}
'''
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=UPSTREAM/'src')
    p.add_argument('--cc',default=os.environ.get('HOSTCC','gcc'))
    a=p.parse_args();main=(a.source/'battle_main.c').read_text(encoding='utf-8');player=(a.source/'battle_controller_player.c').read_text(encoding='utf-8')
    bodies=function(main,'InitSparseDoubleBattlers')
    for n in ['PlayerHasTwoIntroMons','Intro_WaitForShinyAnimAndHealthbox','Intro_TryShinyAnimShowHealthbox','Task_StartSendOutAnim']:
        bodies+=function(player,n)
    with tempfile.TemporaryDirectory(prefix='coop-sparse-') as tmp:
        src=Path(tmp)/'test.c';exe=Path(tmp)/'test.exe';src.write_text(HEADER+bodies+TEST,encoding='utf-8')
        subprocess.run(shlex.split(a.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
if __name__=='__main__':main()
