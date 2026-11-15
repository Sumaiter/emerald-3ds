"""Execute real native party/loss, trainer preparation and map-warp bodies."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from test_coop_bike_hooks import function, UPSTREAM

ROOT = Path(__file__).resolve().parents[2]
COMMON = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int32_t s32;typedef int16_t s16;typedef bool bool8;
#define TRUE true
#define FALSE false
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define PARTY_SIZE 6
#define MULTI_PARTY_SIZE 3
#define MON_DATA_HP 0
#define MON_DATA_SPECIES 1
#define MON_DATA_SPECIES_OR_EGG 1
#define MON_DATA_IS_EGG 2
#define MON_DATA_LEVEL 3
#define MON_DATA_HP_IV 4
#define MON_DATA_HELD_ITEM 5
#include "constants/battle.h"
#include "constants/battle_script_commands.h"
#include "constants/species.h"
struct Pokemon { unsigned species,hp,egg,level,iv; };
static struct Pokemon gPlayerParty[6],gEnemyParty[6];
static unsigned gBattleTypeFlags;
static bool cooperative=true;
static bool CtrCoop_IsDoubleBattle(void){return cooperative;}
static unsigned GetMonData(const struct Pokemon *p,unsigned field){
    switch(field){case 0:return p->hp;case 1:return p->species;case 2:return p->egg;case 3:return p->level;case 4:return p->iv;case 5:return 0;}assert(0);return 0;
}
'''

LOSS = r'''
#define TRAINER_STEVEN_PARTNER 804
static u8 gActiveBattler,gBattlersCount=4,gBattleOutcome;
static unsigned gPartnerTrainerId,gBattleControllerExecFlags,gHitMarker;
static unsigned gStatuses3[4];static u8 gBattlerPartyIndexes[4]={0,0,3,1};
static unsigned gBitTable[]={1,2,4,8,16,32};
static struct {unsigned status2;} gBattleMons[4];
static struct {bool faintedHasReplacement;} gSpecialStatuses[4];
static struct {unsigned arenaLostPlayerMons,arenaLostOpponentMons;u8 monToSwitchIntoId[4];} battleStruct,*gBattleStruct=&battleStruct;
static unsigned GetBattlerSide(unsigned i){return i&1;}
static unsigned GetBattlerAtPosition(unsigned i){return i;}
static unsigned GetBattlerForBattleScript(unsigned i){return i;}
static unsigned GetBattlerMultiplayerId(unsigned i){return i==2?1:0;}
static unsigned GetLinkTrainerFlankId(unsigned i){return i;}
static const u8 commands[16]={0,0},jumpTarget[16]={0};
static const u8 *gBattlescriptCurrInstr=commands;
#define T1_READ_PTR(a) ((void)(a),jumpTarget)
#define T2_READ_PTR(a) ((void)(a),jumpTarget)
'''
LOSS_TEST = r'''
int main(void){
    gBattleTypeFlags=BATTLE_TYPE_TRAINER|BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI|BATTLE_TYPE_LINK;
    for(unsigned i=0;i<6;++i){gPlayerParty[i]=(struct Pokemon){10+i,20,0,5,0};gEnemyParty[i]=(struct Pokemon){30+i,20,0,5,0};}
    for(unsigned role=0;role<2;++role){
        for(unsigned i=0;i<6;++i)gPlayerParty[i].hp=(i/3==role)?0:20;
        unsigned battler=role?2:0;
        assert(HasNoMonsToSwitch(battler,6,6));
        assert(!HasNoMonsToSwitch(battler^2,6,6));
        gBattleOutcome=0;gBattlescriptCurrInstr=commands;Cmd_checkteamslost();
        assert(!gBattleOutcome); /* partner continues, no reserve/ally borrowing */
        u8 command[16]={0,battler};gBattlescriptCurrInstr=command;Cmd_jumpifcantswitch();
        assert(gBattlescriptCurrInstr==jumpTarget);
    }
    for(unsigned i=0;i<6;++i)gPlayerParty[i].hp=0;
    gBattleOutcome=0;gBattlescriptCurrInstr=commands;Cmd_checkteamslost();assert(gBattleOutcome==B_OUTCOME_LOST);
    for(unsigned i=0;i<6;++i)gPlayerParty[i].hp=20;
    for(unsigned i=0;i<6;++i)gEnemyParty[i].hp=0;
    gBattleOutcome=0;gBattlescriptCurrInstr=commands;Cmd_checkteamslost();assert(gBattleOutcome==B_OUTCOME_WON);
    /* Single NPC's fifth Pokemon is usable by either enemy flank, and the
     * other active Pokemon can never be selected as its own replacement. */
    gEnemyParty[0].hp=gEnemyParty[1].hp=20;gEnemyParty[4].hp=20;
    assert(!HasNoMonsToSwitch(1,6,6));assert(!HasNoMonsToSwitch(3,6,6));
    for(unsigned b=1;b<4;b+=2){u8 c[16]={0,b};gBattlescriptCurrInstr=c;Cmd_jumpifcantswitch();assert(gBattlescriptCurrInstr==c+6);}
    gEnemyParty[4].hp=0;assert(HasNoMonsToSwitch(1,6,6));assert(HasNoMonsToSwitch(3,6,6));
    /* Two authored NPCs retain their separate three-Pokemon teams. */
    gBattleTypeFlags|=BATTLE_TYPE_TWO_OPPONENTS;gEnemyParty[4].hp=20;gEnemyParty[0].hp=gEnemyParty[1].hp=0;
    assert(HasNoMonsToSwitch(1,6,6));assert(!HasNoMonsToSwitch(3,6,6));
    puts("native triplet ownership, exhausted-owner continuation, total defeat and NPC full-six pool passed");return 0;
}
'''

TRAINER = r'''
#include "constants/opponents.h"
#define OT_ID_RANDOM_NO_SHINY 0
#define TRAINER_MON_SPECIES(s) (s)
static struct {u8 partySize;} gTrainers[TRAINERS_COUNT];
static unsigned gTrainerBattleOpponent_A,gTrainerBattleOpponent_B,creates,originalCreates;
static bool wild=true;
static bool CtrCoop_EngineCreateWildPartner(void){return wild;}
static void CreateMon(struct Pokemon *p,unsigned species,unsigned level,unsigned iv,bool fixed,unsigned personality,unsigned ot,unsigned id){
    (void)fixed;(void)personality;(void)ot;(void)id;++creates;*p=(struct Pokemon){species,30,0,level,iv};
}
static void CreateNPCTrainerParty(struct Pokemon *party,unsigned trainer,bool first){
    ++originalCreates;if(first)memset(gEnemyParty,0,sizeof(gEnemyParty));
    for(unsigned i=0;i<gTrainers[trainer].partySize && i<((gBattleTypeFlags&BATTLE_TYPE_TWO_OPPONENTS)?3:6);++i)
        party[i]=(struct Pokemon){400+i,20,0,12+i,17};
}
'''
TRAINER_TEST = r'''
int main(void){
    (void)gPlayerParty;(void)CtrCoop_IsDoubleBattle;
    unsigned ids[]={TRAINER_MAY_ROUTE_103_TREECKO,TRAINER_MAY_ROUTE_103_TORCHIC,TRAINER_MAY_ROUTE_103_MUDKIP,TRAINER_RICK,TRAINER_ROXANNE_1};
    for(unsigned t=0;t<ARRAY_COUNT(ids);++t){
        gTrainerBattleOpponent_A=ids[t];gTrainers[ids[t]].partySize=1;gBattleTypeFlags=BATTLE_TYPE_TRAINER;creates=originalCreates=0;
        assert(CtrCoop_EnginePrepareFollowBattle() && creates==1 && originalCreates==1);
        assert(gEnemyParty[0].species==400 && gEnemyParty[0].hp==20 && gEnemyParty[1].species==sCoopTrainerAdditions[ids[t]][0]);
        assert(gEnemyParty[1].level==12 && gEnemyParty[1].iv==17);
    }
    gTrainerBattleOpponent_A=TRAINER_ROXANNE_1;gTrainers[gTrainerBattleOpponent_A].partySize=6;creates=0;
    assert(CtrCoop_EnginePrepareFollowBattle() && !creates);
    gTrainers[gTrainerBattleOpponent_A].partySize=2;gBattleTypeFlags=BATTLE_TYPE_TRAINER|BATTLE_TYPE_DOUBLE;creates=0;
    assert(CtrCoop_EnginePrepareFollowBattle() && !creates && !gEnemyParty[2].species);
    gTrainerBattleOpponent_B=TRAINER_RICK;gTrainers[gTrainerBattleOpponent_B].partySize=2;
    gBattleTypeFlags|=BATTLE_TYPE_TWO_OPPONENTS|BATTLE_TYPE_INGAME_PARTNER;
    assert(CtrCoop_EnginePrepareFollowBattle() && !creates && gEnemyParty[3].species==400 && gEnemyParty[4].species==401);
    gBattleTypeFlags=BATTLE_TYPE_SAFARI;assert(!CtrCoop_EnginePrepareFollowBattle());
    gBattleTypeFlags=BATTLE_TYPE_TRAINER;gTrainerBattleOpponent_A=TRAINERS_COUNT;assert(!CtrCoop_EnginePrepareFollowBattle());
    gBattleTypeFlags=0;assert(CtrCoop_EnginePrepareFollowBattle());wild=false;assert(!CtrCoop_EnginePrepareFollowBattle());
    puts("native original roster preparation, diverse additions, authored doubles and six-slot cap passed");return 0;
}
'''

WILD = r'''
#define HEADER_NONE 65535
#define PLAYER_AVATAR_FLAG_SURFING 1
#define WILD_SPECIES(s) (s)
#define USE_RANDOM_IVS 32
#define OT_ID_PLAYER_ID 0
static struct {unsigned species,minLevel,maxLevel;} landMons[12],waterMons[5];
static struct WildPokemonInfo {typeof(landMons[0]) *wildPokemon;} land={landMons},water={waterMons};
static struct {struct WildPokemonInfo *waterMonsInfo;} gWildMonHeaders[1];
static struct {unsigned flags;} gPlayerAvatar;
static unsigned header,index,created;
static u16 GetCurrentMapWildMonHeaderId(void){return header;}
static struct WildPokemonInfo *GetLandMonsInfo(u16 h){assert(!h);return &land;}
static u8 ChooseWildMonIndex_WaterRock(void){return index%5;}
static u8 ChooseWildMonIndex_Land(void){return index%12;}
static unsigned ChooseWildMonLevel(const typeof(landMons[0]) *p){return p->minLevel;}
static void CreateMon(struct Pokemon *p,unsigned species,unsigned level,unsigned iv,bool fixed,unsigned personality,unsigned ot,unsigned id){
    (void)fixed;(void)personality;(void)ot;(void)id;assert(iv==32);++created;*p=(struct Pokemon){species,12,0,level,0};
}
'''
WILD_TEST = r'''
int main(void){
    (void)gPlayerParty;(void)gBattleTypeFlags;(void)CtrCoop_IsDoubleBattle;
    for(unsigned i=0;i<12;++i)landMons[i]=(typeof(landMons[0])){261+i%3,2+i%3,4};
    for(unsigned i=0;i<5;++i)waterMons[i]=(typeof(waterMons[0])){72+i%2,5+i,9};
    gWildMonHeaders[0].waterMonsInfo=&water;
    for(unsigned surf=0;surf<2;++surf)for(index=0;index<12;++index){
        memset(gEnemyParty,0,sizeof(gEnemyParty));gEnemyParty[0]=(struct Pokemon){258,7,0,3,15};
        struct Pokemon original=gEnemyParty[0];created=0;gPlayerAvatar.flags=surf;
        assert(CtrCoop_EngineCreateWildPartner() && created==1);
        assert(!memcmp(&original,&gEnemyParty[0],sizeof(original)));
        assert(gEnemyParty[1].species==(surf?waterMons[index%5].species:landMons[index].species));
        assert(gEnemyParty[1].hp && !gEnemyParty[2].species);
        assert(CtrCoop_EngineCreateWildPartner() && created==1);
    }
    memset(&gEnemyParty[1],0,sizeof(gEnemyParty[1]));header=HEADER_NONE;
    assert(!CtrCoop_EngineCreateWildPartner());
    puts("real wild helper: second independently created map-table land/water opponent, first opponent unchanged, no trainer flags");return 0;
}
'''

WARP = r'''
#define MAP_OFFSET 7
#define WARP_ID_NONE -1
static struct {void(*callback2)(void);bool inBattle;} gMain;
static struct {bool active;} gPaletteFade;
static bool controls,script;
static unsigned warps,callbacks;
static struct {struct{s16 x,y;}pos;} save,*gSaveBlock1Ptr=&save;
static void CB2_Overworld(void){}static void CB2_LoadMap(void){}
static bool ArePlayerFieldControlsLocked(void){return controls;}
static bool ScriptContext_IsEnabled(void){return script;}
struct MapLayout{unsigned width,height;};
struct MapHeader{struct MapLayout *mapLayout;u16 mapLayoutId;};
static struct MapLayout layout={200,200};
static struct MapHeader map={&layout,10};
static const struct MapHeader *group[]={&map};
static const struct MapHeader *const *gMapGroups[]={group};
static void SetWarpDestination(u8 g,u8 n,int id,int x,int y){assert(!g&&!n&&id==-1);(void)x;(void)y;}
static void WarpIntoMap(void){++warps;}
static void SetMainCallback2(void(*cb)(void)){gMain.callback2=cb;++callbacks;}
'''
WARP_TEST = r'''
int main(void){
    gMain.callback2=CB2_Overworld;
    assert(!CtrCoop_EngineFollowWarp(255,0,10,17,17));
    assert(!CtrCoop_EngineFollowWarp(0,255,10,17,17));
    assert(!CtrCoop_EngineFollowWarp(0,0,11,17,17));
    assert(!CtrCoop_EngineFollowWarp(0,0,10,6,17));
    assert(!CtrCoop_EngineFollowWarp(0,0,10,207,17));
    controls=true;assert(!CtrCoop_EngineFollowWarp(0,0,10,17,17));controls=false;
    script=true;assert(!CtrCoop_EngineFollowWarp(0,0,10,17,17));script=false;
    gPaletteFade.active=true;assert(!CtrCoop_EngineFollowWarp(0,0,10,17,17));gPaletteFade.active=false;
    assert(!warps&&!callbacks);assert(CtrCoop_EngineFollowWarp(0,0,10,17,17));
    assert(warps==1&&callbacks==1&&gMain.callback2==CB2_LoadMap);
    assert(save.pos.x==10 && save.pos.y==10);
    gMain.callback2=CB2_Overworld;assert(CtrCoop_EngineFollowWarp(0,0,10,157,177));
    assert(save.pos.x==150 && save.pos.y==170); /* no signed-byte truncation */
    (void)gSaveBlock1Ptr;
    puts("native follow-map transition bounds and busy-state gates passed");return 0;
}
'''

ENTRY = r'''
#include "constants/battle_frontier.h"
#include "constants/map_groups.h"
#include "constants/vars.h"
#define MAP_GROUP(m) ((m)>>8)
#define MAP_NUM(m) ((m)&255)
#define ITEM_NONE 0
static struct {struct {u8 mapGroup,mapNum;}location;} save,*gSaveBlock1Ptr=&save;
static u16 gFrontierBannedSpecies[]={0xFFFF};
static bool travelling=true;
static unsigned facility=FACILITY_MULTI_OR_EREADER;
static unsigned VarGet(unsigned var){assert(var==VAR_FRONTIER_FACILITY);return facility;}
static unsigned GetBattleEntryLevelCap(void){return 100;}
static bool CtrCoop_RestrictStoryParty(void){return travelling;}
'''
ENTRY_TEST = r'''
int main(void){
    (void)gEnemyParty;(void)gBattleTypeFlags;(void)CtrCoop_IsDoubleBattle;
    for(unsigned i=0;i<6;++i)gPlayerParty[i]=(struct Pokemon){10+i,20,0,5,0};
    for(unsigned i=0;i<6;++i)assert(GetBattleEntryEligibility(&gPlayerParty[i])==(i<3));
    travelling=false;for(unsigned i=0;i<6;++i)assert(GetBattleEntryEligibility(&gPlayerParty[i]));
    travelling=true;facility=FACILITY_UNION_ROOM;
    for(unsigned i=0;i<6;++i)assert(GetBattleEntryEligibility(&gPlayerParty[i]));
    puts("native story partner selection rejects reserve slots without changing ordinary party selection");return 0;
}
'''

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=UPSTREAM/'src')
    p.add_argument('--cc',default=os.environ.get('HOSTCC','gcc'))
    a=p.parse_args()
    read=lambda name:(a.source/name).read_text(encoding='utf-8')
    loss = function(read('battle_util.c'),'HasNoMonsToSwitch')
    loss += function(read('battle_script_commands.c'),'Cmd_checkteamslost')
    loss += function(read('battle_script_commands.c'),'Cmd_jumpifcantswitch')
    variants={
        'party-loss':COMMON+LOSS+loss+LOSS_TEST,
        'trainer-preparation':COMMON+TRAINER+'#include "3ds_coop_trainer_teams.h"\n'+function(read('battle_main.c'),'CtrCoop_EnginePrepareFollowBattle')+TRAINER_TEST,
        'wild-pair':COMMON+WILD+function(read('wild_encounter.c'),'CtrCoop_EngineCreateWildPartner')+WILD_TEST,
        'map-transfer':COMMON.split('struct Pokemon')[0]+WARP+function(read('overworld.c'),'CtrCoop_EngineFollowWarp')+WARP_TEST,
        'story-selection':COMMON+ENTRY+function(read('party_menu.c'),'GetBattleEntryEligibility')+ENTRY_TEST,
    }
    with tempfile.TemporaryDirectory(prefix='coop-follow-',dir=os.environ.get('TMPDIR')) as d:
        d=Path(d)
        for name,source in variants.items():
            path=d/(name+'.c');exe=d/(name+'.exe');path.write_text(source,encoding='utf-8')
            subprocess.run(shlex.split(a.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-DPLATFORM_3DS',
                '-I'+str(ROOT/'3ds_port/include'),'-iquote',str(a.source.parent/'include'),str(path),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
    subprocess.run([os.sys.executable,str(ROOT/'3ds_port/scripts/gen_coop_trainer_teams.py'),'--upstream',str(a.source.parent),'--check'],check=True)

if __name__=='__main__':main()
