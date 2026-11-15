"""Run native sparse-party/menu, NPC art and cooperative completion barriers."""
import argparse
import os
import re
from pathlib import Path
import shlex
import subprocess
import tempfile
from test_coop_bike_hooks import function, UPSTREAM

PORT = Path(__file__).resolve().parents[1]
COMMON = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constants/battle.h"
#undef assert
#define assert(c) do { if (!(c)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); exit(1); } } while (0)
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int16_t s16;typedef int32_t s32;typedef bool bool8;
#define TRUE true
#define FALSE false
#define PARTY_SIZE 6
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define MULTI_PARTY_SIZE (PARTY_SIZE / 2)
#define MAX_BATTLERS_COUNT 4
#define MAX_LINK_PLAYERS 4
#define SPECIES_NONE 0
static bool cooperative;
static bool CtrCoop_IsDoubleBattle(void){return cooperative;}
'''

PARTY = r'''
#define MON_DATA_SPECIES 1
#define MON_DATA_IS_EGG 2
#define MON_DATA_PERSONALITY 3
#define MON_DATA_HP 4
#define SPECIES_EGG 412
#define SPECIES_DEOXYS 410
#define CTR_PARTY_VIEW_LIST 1
#define AREA_X 4
#define AREA_Y 28
#define AREA_W 392
struct Pokemon {u16 species,hp;bool egg;};
static struct Pokemon gPlayerParty[6],gEnemyParty[6];
static u8 gPlayerPartyCount,gEnemyPartyCount,gBattlePartyCurrentOrder[3],gBattlerPartyIndexes[4];
static u32 gBattleTypeFlags;
static struct {bool inBattle;} gMain;
typedef struct {u16 species,iconSpecies;bool isEgg,deoxys,fainted;} MonView;
typedef struct {MonView party[6];bool multiParty;struct {u8 state;bool message,yesNo;} pv;} ViewState;
static bool CtrCoop_UsesDoubleParty(void){return cooperative;}
static u32 GetMonData(const struct Pokemon *p,unsigned field,...){return field==MON_DATA_SPECIES?p->species:field==MON_DATA_HP?p->hp:field==MON_DATA_IS_EGG?p->egg:0;}
static unsigned GetIconSpecies(unsigned s,unsigned p){(void)p;return s;}
static unsigned GetBattlerAtPosition(unsigned b){return b;}
static bool IsDoubleBattle(void){return !!(gBattleTypeFlags&BATTLE_TYPE_DOUBLE);}
static void *Alloc(unsigned n){return malloc(n);}
static void Free(void *p){free(p);}
'''
PARTY_TEST = r'''
int main(void){
    const unsigned localSlots[3]={0,2,3},partnerSlots[3]={1,4,5};
    for(unsigned role=0;role<2;++role)for(unsigned host=1;host<=3;++host)for(unsigned guest=1;guest<=3;++guest){
        cooperative=true;gMain.inBattle=true;gBattleTypeFlags=BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI|BATTLE_TYPE_LINK|BATTLE_TYPE_TRAINER;
        memset(gPlayerParty,0,sizeof(gPlayerParty));
        for(unsigned i=0;i<host;++i)gPlayerParty[i]=(struct Pokemon){10+i,20,false};
        for(unsigned i=0;i<guest;++i)gPlayerParty[3+i]=(struct Pokemon){30+i,20,false};
        memcpy(gEnemyParty,gPlayerParty,sizeof(gPlayerParty));
        struct Pokemon original[6];memcpy(original,gPlayerParty,sizeof(original));
        assert(CalculatePlayerPartyCount()==6 && CalculateEnemyPartyCount()==6);
        BufferBattlePartyOrder(gBattlePartyCurrentOrder,role);UpdatePartyToBattleOrder();
        ViewState view={0};SnapshotParty(&view);
        for(unsigned i=0;i<3;++i){
            assert(view.party[localSlots[i]].species==original[role*3+i].species);
            assert(view.party[partnerSlots[i]].species==original[(1-role)*3+i].species);
        }
        assert(view.party[0].species && view.party[1].species);
        UpdatePartyToFieldOrder();assert(!memcmp(original,gPlayerParty,sizeof(original)));
    }
    cooperative=false;gPlayerParty[1].species=0;assert(CalculatePlayerPartyCount()==1);
    ViewState v={.multiParty=true,.pv={.state=CTR_PARTY_VIEW_LIST}};
    int rect[6][4];
    for(int s=0;s<6;++s){assert(PartyGroupRect(&v,s,&rect[s][0],&rect[s][1],&rect[s][2],&rect[s][3]));assert(rect[s][1]+rect[s][3]<=AREA_Y+129);}
    for(unsigned i=0;i<3;++i){assert(rect[localSlots[i]][0]==AREA_X && rect[partnerSlots[i]][0]>AREA_X);}
    for(unsigned a=0;a<6;++a)for(unsigned b=a+1;b<6;++b){
        int *p=rect[a],*q=rect[b];assert(p[0]+p[2]<=q[0]||q[0]+q[2]<=p[0]||p[1]+p[3]<=q[1]||q[1]+q[3]<=p[1]);
    }
    v.pv.message=true;assert(!PartyGroupRect(&v,0,&rect[0][0],&rect[0][1],&rect[0][2],&rect[0][3]));
    puts("native 1..3 + 1..3 party counts/order, both local halves, snapshot and two-column touch geometry passed");return 0;
}
'''

BARRIER = r'''
typedef int8_t s8;typedef bool bool32;
struct DisableStruct;struct Sprite;
#define MAX_MON_MOVES 4
#include "battle_controllers.h"
#define LINK_BUFF_ACTIVE_BATTLER 1
#define LINK_BUFF_ATTACKER 2
#define LINK_BUFF_TARGET 3
#define LINK_BUFF_SIZE_LO 4
#define LINK_BUFF_SIZE_HI 5
#define LINK_BUFF_ABSENT_BATTLER_FLAGS 6
#define LINK_BUFF_EFFECT_BATTLER 7
#define LINK_BUFF_DATA 8
#define tCurrentBlock_Start data[0]
#define tCurrentBlock_End data[1]
#define tCurrentBlock_WrapFrom data[2]
static struct {s16 data[3];} gTasks[1];
static u8 gLinkBattleRecvBuffer[4096],gBattleBufferA[4][512],gBattleBufferB[4][512];
static u32 gBattleControllerExecFlags,gBattleTypeFlags;
static const u32 gBitTable[4]={1,2,4,8};
static u8 gBattlerAttacker,gBattlerTarget,gAbsentBattlerFlags,gEffectBattler;
static bool sDirectBattleLink=true;
static u8 sSkippedDones[4][4],sHeldMasterDones[4],localId;
static u8 GetMultiplayerId(void){return localId;}
static u8 GetLinkPlayerCount(void){return 2;}
static void MarkBattlerReceivedLinkData(u8 b){gBattleControllerExecFlags|=gBitTable[b]|(gBitTable[b]<<4);}
'''
BARRIER_TEST = r'''
static void queue(unsigned kind,unsigned battler,unsigned value){
    memset(gTasks,0,sizeof(gTasks));memset(gLinkBattleRecvBuffer,0,sizeof(gLinkBattleRecvBuffer));
    gLinkBattleRecvBuffer[0]=kind;gLinkBattleRecvBuffer[1]=battler;gLinkBattleRecvBuffer[4]=1;
    gLinkBattleRecvBuffer[8]=value;gTasks[0].tCurrentBlock_End=9;
}
int main(void){
    for(unsigned b=0;b<4;++b){
        cooperative=true;localId=0;gBattleTypeFlags=BATTLE_TYPE_LINK|BATTLE_TYPE_IS_MASTER;
        gBattleControllerExecFlags=0;memset(sSkippedDones,0,sizeof(sSkippedDones));
        queue(B_COMM_TO_CONTROLLER,b,CONTROLLER_DRAWTRAINERPIC);assert(ProcessReceivedLinkBuffer(0));
        assert(IS_BATTLE_CONTROLLER_ACTIVE_FOR_PLAYER(b,1) && !sSkippedDones[b][1]);
        MARK_BATTLE_CONTROLLER_IDLE_FOR_PLAYER(b,0);assert(IS_BATTLE_CONTROLLER_ACTIVE_FOR_PLAYER(b,1));
        queue(B_COMM_CONTROLLER_IS_DONE,b,1);assert(ProcessReceivedLinkBuffer(0));assert(!gBattleControllerExecFlags);
        /* Competitive mode retains its existing async optimization. */
        cooperative=false;queue(B_COMM_TO_CONTROLLER,b,CONTROLLER_DRAWTRAINERPIC);assert(ProcessReceivedLinkBuffer(0));
        assert(!IS_BATTLE_CONTROLLER_ACTIVE_FOR_PLAYER(b,1) && sSkippedDones[b][1]==1);
        gBattleControllerExecFlags=0;
        /* On the client, early master done cannot prevent the real animation callback. */
        cooperative=true;localId=1;gBattleTypeFlags=BATTLE_TYPE_LINK;
        queue(B_COMM_TO_CONTROLLER,b,CONTROLLER_INTROTRAINERBALLTHROW);assert(ProcessReceivedLinkBuffer(0));
        queue(B_COMM_CONTROLLER_IS_DONE,b,0);assert(ProcessReceivedLinkBuffer(0));
        assert(IS_BATTLE_CONTROLLER_ACTIVE_ON_LOCAL(b) && sHeldMasterDones[b]==1);
        gBattleControllerExecFlags=0;
    }
    puts("real controller receive queue waits for cooperative guest animations; competitive optimization and client completion preserved");return 0;
}
'''

ART = r'''
#include "constants/trainers.h"
#include "constants/global.h"
#include "constants/pokemon.h"
#define DISPLAY_WIDTH 240
#define BATTLE_SIDE_SHIFT(s) 0
#define sSpeedX data[0]
static u32 gBattleTypeFlags;
static u8 gActiveBattler,gBattlerSpriteIds[4];
static u16 gTrainerBattleOpponent_A,gTrainerBattleOpponent_B;
static struct {u16 trainerPic;} gTrainers[900];
static struct {unsigned id,version,gender;} gLinkPlayers[4];
static struct {s16 x,x2,y;bool invisible;struct {unsigned paletteNum,affineParam;} oam;s16 data[8];void (*callback)(void *);} gSprites[64];
static struct {unsigned size;} gTrainerFrontPicCoords[512];
static struct {unsigned tag;} gTrainerFrontPicPaletteTable[512];
static unsigned gMultiuseSpriteTemplate,lastPic,created;
static void (*gBattlerControllerFuncs[4])(void);
static unsigned GetSecretBaseTrainerPicIndex(void){return 301;}
static unsigned GetFrontierBrainTrainerPicIndex(void){return 302;}
static unsigned GetTrainerHillTrainerFrontSpriteId(unsigned id){return 303+(id==gTrainerBattleOpponent_B);}
static unsigned GetFrontierTrainerFrontSpriteId(unsigned id){return 305+(id==gTrainerBattleOpponent_B);}
static unsigned GetEreaderTrainerFrontSpriteId(void){return 307;}
static unsigned GetBattlerPosition(unsigned b){return b;}
static unsigned GetBattlerMultiplayerId(unsigned b){return b/2;}
static unsigned GetMultiplayerId(void){return 0;}
static unsigned GetUnionRoomTrainerPic(void){return 308;}
static unsigned PlayerGenderToFrontTrainerPicId(unsigned g){return 100+g;}
static const unsigned gFacilityClassToPicIndex[100]={0};
static void DecompressTrainerFrontPic(unsigned pic,unsigned b){(void)b;lastPic=pic;}
static void SetMultiuseSpriteTemplateToTrainerBack(unsigned pic,unsigned b){(void)b;assert(pic==lastPic);}
static unsigned GetBattlerSpriteSubpriority(unsigned b){return b;}
static unsigned CreateSprite(const void *p,int x,int y,unsigned pri){(void)p;(void)x;(void)y;(void)pri;++created;return gActiveBattler;}
static unsigned IndexOfSpritePaletteTag(unsigned tag){return tag%16;}
static void SpriteCB_TrainerSlideIn(void *s){(void)s;}
static void CompleteOnBattlerSpriteCallbackDummy(void){}
static void CompleteOnBankSpriteCallbackDummy2(void){}
static void LinkOpponentBufferExecCompleted(void){}
'''
ART_TEST = r'''
int main(void){
    cooperative=true;gTrainerBattleOpponent_A=856;gTrainerBattleOpponent_B=857;
    gTrainers[856].trainerPic=42;gTrainers[857].trainerPic=43;
    for(unsigned two=0;two<2;++two)for(unsigned side=0;side<2;++side){
        gActiveBattler=side?3:1;gBattleTypeFlags=BATTLE_TYPE_LINK|BATTLE_TYPE_MULTI|BATTLE_TYPE_TRAINER|(two?BATTLE_TYPE_TWO_OPPONENTS:0);
        unsigned expected=two&&side?43:42;
        assert(CtrCoop_EngineOpponentTrainerPic()==expected);
        LinkOpponentHandleDrawTrainerPic();assert(lastPic==expected && created);
        LinkOpponentHandleTrainerSlide();assert(lastPic==expected);
    }
    gBattleTypeFlags=BATTLE_TYPE_SECRET_BASE;assert(GetOpponentTrainerPic()==301);
    gBattleTypeFlags=BATTLE_TYPE_TRAINER_HILL|BATTLE_TYPE_TWO_OPPONENTS;assert(GetOpponentTrainerPic()==304);
    gBattleTypeFlags=BATTLE_TYPE_FRONTIER|BATTLE_TYPE_TWO_OPPONENTS;assert(GetOpponentTrainerPic()==306);
    gBattleTypeFlags=BATTLE_TYPE_EREADER_TRAINER;assert(GetOpponentTrainerPic()==307);
    puts("native guest AI-opponent front/slide uses encounter identity, including two trainers and original special sources");return 0;
}
'''

PARTNER = r'''
#define OBJ_PLTT_ID(i) ((i)*16+256)
#define PLTT_SIZE_4BPP 32
static struct {unsigned size;} gTrainerBackPicCoords[8];
static struct {const void *data;} gTrainerBackPicPaletteTable[8];
static unsigned mapped,applied,paletteBattler,paletteSlot,throwTasks;
static struct {s16 data[16];void (*func)(u8);} gTasks[4];
static u8 gBattlerStatusSummaryTaskId[4];
static struct {bool partyStatusSummaryShown;} health[4];
static struct {bool introAnimActive;} animation;
static struct {typeof(health[0]) *healthBoxesData;typeof(animation) *animationData;} state={health,&animation},*gBattleSpritesDataPtr=&state;
static u16 CtrCoopBattle_BattlerBackPicId(u8 b,u16 original){(void)original;mapped=b;return 4+b;}
static void CtrCoopBattle_Apply(u8 b,void *t){(void)t;applied=b;}
static void CtrCoopBattle_LoadBattlerPalette(u8 b,u8 slot){paletteBattler=b;paletteSlot=slot;}
static void DecompressTrainerBackPic(unsigned pic,unsigned b){assert(pic==4+b);lastPic=pic;}
static void LoadCompressedPalette(const void *data,unsigned slot,unsigned size){(void)data;(void)slot;assert(size==32);}
static void SetSpritePrimaryCoordsFromSecondaryCoords(void *s){(void)s;}
static void StartAnimLinearTranslation(void *s){(void)s;}
static void SpriteCB_FreePlayerSpriteLoadMonSprite(void *s){(void)s;}
static void StoreSpriteCallbackInData6(void *s,void (*cb)(void *)){(void)s;(void)cb;}
static void StartSpriteAnim(void *s,unsigned anim){(void)s;assert(anim==1);}
static unsigned AllocSpritePalette(unsigned tag){assert(tag==0xD6F9);return 8;}
static void Task_StartSendOutAnim(u8 task){(void)task;}
static unsigned CreateTask(void (*cb)(u8),unsigned pri){assert(cb==Task_StartSendOutAnim && pri==5);++throwTasks;return 0;}
static void Task_HidePartyStatusSummary(u8 task){(void)task;}
static void SpriteCB_Null2(void){}
#define SPRITE_NONE 64
#define MODE_PARENT 1
#define STATUS_INDICATOR_ACTIVE 0x1234
#define sValidator data[7]
#define sTileStart data[6]
static struct {unsigned parentChild;} rfu,*gRfuLinkStatus=&rfu;
static struct {unsigned tag;} sWirelessStatusIndicatorSpriteSheet;
static unsigned sWirelessStatusIndicatorSpriteTemplate;
static u8 gWirelessStatusIndicatorSpriteId;
static unsigned GetSpriteTileStartByTag(unsigned tag){(void)tag;return 0;}
'''
PARTNER_TEST = r'''
int main(void){
    (void)gFacilityClassToPicIndex;(void)gTrainerFrontPicPaletteTable;(void)gTrainerFrontPicCoords;(void)gTrainers;(void)gTrainerBattleOpponent_A;
    cooperative=true;gBattleTypeFlags=BATTLE_TYPE_LINK|BATTLE_TYPE_MULTI|BATTLE_TYPE_DOUBLE;
    for(unsigned b=0;b<=2;b+=2){
        gActiveBattler=b;gLinkPlayers[b/2].version=VERSION_EMERALD;
        mapped=applied=99;LinkPartnerHandleDrawTrainerPic();
        assert(mapped==b && applied==b && lastPic==4+b && gSprites[gBattlerSpriteIds[b]].oam.paletteNum==b);
        paletteBattler=paletteSlot=99;LinkPartnerHandleIntroTrainerBallThrow();
        assert(paletteBattler==b && paletteSlot==8 && gSprites[gBattlerSpriteIds[b]].oam.paletteNum==8);
    }
    assert(throwTasks==2);
    unsigned before=created;CreateWirelessStatusIndicatorSprite(0,0);assert(created==before && gWirelessStatusIndicatorSpriteId==SPRITE_NONE);
    cooperative=false;CreateWirelessStatusIndicatorSprite(0,0);assert(created==before+1 && gWirelessStatusIndicatorSpriteId!=SPRITE_NONE);
    puts("real link-partner draw/throw use battler-specific art/palette; RFU icon suppressed only in cooperative battles");return 0;
}
'''

TEXT = r'''
#include "constants/battle_string_ids.h"
#include "constants/trainers.h"
static u32 gBattleTypeFlags;
static u16 gTrainerBattleOpponent_A;
static u8 gActiveBattler;
static struct {u8 battler;} gBattleScripting;
static struct {u8 hpScale;} battle,*gBattleStruct=&battle;
static unsigned GetBattlerSide(unsigned b){return b&1;}
'''
TEXT_TEST = r'''
int main(void){
    cooperative=true;gActiveBattler=1;gBattleScripting.battler=1;
    gBattleTypeFlags=BATTLE_TYPE_LINK|BATTLE_TYPE_MULTI|BATTLE_TYPE_DOUBLE|BATTLE_TYPE_TRAINER;
    assert(SelectText(STRINGID_INTROMSG)==sText_Trainer1WantsToBattle);
    assert(SelectText(STRINGID_INTROSENDOUT)==sText_Trainer1SentOutTwoPkmn);
    assert(SelectText(STRINGID_SWITCHINMON)==sText_Trainer1SentOutPkmn2);
    gBattleTypeFlags|=BATTLE_TYPE_TWO_OPPONENTS;
    assert(SelectText(STRINGID_INTROMSG)==sText_TwoTrainersWantToBattle);
    assert(SelectText(STRINGID_INTROSENDOUT)==sText_TwoTrainersSentPkmn);
    gBattleScripting.battler=3;assert(SelectText(STRINGID_SWITCHINMON)==sText_Trainer2SentOutPkmn);
    cooperative=false;gBattleTypeFlags&=~BATTLE_TYPE_TWO_OPPONENTS;
    assert(SelectText(STRINGID_INTROMSG)==sText_TwoLinkTrainersWantToBattle);
    assert(SelectText(STRINGID_INTROSENDOUT)==sText_TwoLinkTrainersSentOutPkmn);
    puts("real native message cases identify NPCs instead of nonexistent opposing link players");return 0;
}
'''

def run(cc, include, code, name, expect_failure=False):
    with tempfile.TemporaryDirectory(prefix='coop-view-') as tmp:
        source=Path(tmp)/'test.c';exe=Path(tmp)/'test.exe'
        source.write_text(COMMON+code,encoding='utf-8')
        subprocess.run(shlex.split(cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function',
            '-DPLATFORM_3DS','-iquote',str(include),str(source),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],capture_output=expect_failure,timeout=30)
        if expect_failure:
            assert result.returncode and b'FAIL' in result.stderr, (name,result.returncode,result.stderr)
        else:
            result.check_returncode()
    print('PASS',name,flush=True)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=UPSTREAM/'src')
    p.add_argument('--cc',default=os.environ.get('HOSTCC','gcc'))
    p.add_argument('--check-mutations',action='store_true')
    a=p.parse_args();inc=a.source.parent/'include';mutations=[]
    read=lambda name:(a.source/name).read_text(encoding='utf-8')
    party=read('party_menu.c');pokemon=read('pokemon.c');controllers=read('battle_controllers.c')
    body=''.join(function(pokemon,n) for n in ['CalculatePlayerPartyCount','CalculateEnemyPartyCount'])
    body+=function(party,'IsMultiBattle')+function(party,'GetPartyIdFromBattleSlot')+function(party,'GetPartyIdFromBattlePartyId')
    body+=function(party,'BufferBattlePartyOrder')+function(party,'UpdatePartyToBattleOrder')+function(party,'UpdatePartyToFieldOrder')
    body+=function((PORT/'src/3ds_bottom_ui.c').read_text(encoding='utf-8'),'SnapshotParty')
    body+=function((PORT/'src/3ds_bottom_game.inc').read_text(encoding='utf-8'),'PartyGroupRect')
    run(a.cc,inc,PARTY+body+PARTY_TEST,'sparse native party and UI')
    mutations.append(('missing sparse-party extent',PARTY+body+PARTY_TEST,'return gPlayerPartyCount = PARTY_SIZE;','return gPlayerPartyCount = 1;'))
    mutations.append(('wrong party grouping',PARTY+body+PARTY_TEST,'{0, 1, 0, 0, 1, 1}','{0, 0, 0, 0, 1, 1}'))
    body=function(controllers,'ControllerCommandNeedsAnswer')+function(controllers,'ProcessReceivedLinkBuffer')
    run(a.cc,inc,BARRIER+body+BARRIER_TEST,'cooperative animation barrier')
    mutations.append(('missing cooperative completion barrier',BARRIER+body+BARRIER_TEST,'&& !CtrCoop_IsDoubleBattle()',''))
    opponent=read('battle_controller_opponent.c');link=read('battle_controller_link_opponent.c')
    body=function(opponent,'GetOpponentTrainerPic')+function(opponent,'CtrCoop_EngineOpponentTrainerPic')
    body+=function(link,'LinkOpponentHandleDrawTrainerPic')+function(link,'LinkOpponentHandleTrainerSlide')
    run(a.cc,inc,ART+body+ART_TEST,'guest NPC identity')
    mutations.append(('wrong guest opponent identity',ART+body+ART_TEST,'if (CtrCoop_IsDoubleBattle())','if (false)'))
    link=read('battle_controller_link_partner.c')
    body=function(link,'LinkPartnerHandleDrawTrainerPic')+function(link,'LinkPartnerHandleIntroTrainerBallThrow')
    body+=function(read('link_rfu_3.c'),'CreateWirelessStatusIndicatorSprite')
    run(a.cc,inc,ART+PARTNER+body+PARTNER_TEST,'native partner identity and RFU suppression')
    mutations.append(('missing partner art hook',ART+PARTNER+body+PARTNER_TEST,'    CtrCoopBattle_Apply(gActiveBattler, &gMultiuseSpriteTemplate);',''))
    mutations.append(('misleading RFU indicator',ART+PARTNER+body+PARTNER_TEST,'if (CtrCoop_IsDoubleBattle())','if (false)'))
    text=function(read('battle_message.c'),'BufferStringBattle')
    cases=''.join(re.search(r'^    case '+name+r':.*?(?=^    (?:case STRINGID_|default:))',text,re.M|re.S).group()
                  for name in ['STRINGID_INTROMSG','STRINGID_INTROSENDOUT','STRINGID_SWITCHINMON'])
    names=sorted(set(re.findall(r'\bsText_\w+',cases)))
    strings=''.join(f'static const u8 {name}[]={{ {i+1} }};\n' for i,name in enumerate(names))
    body='static const u8 *SelectText(u16 stringID){const u8 *stringPtr=NULL;bool8 coopAiBattle=CtrCoop_IsDoubleBattle();switch(stringID){\n'+cases+'default:break;}return stringPtr;}\n'
    run(a.cc,inc,TEXT+strings+body+TEXT_TEST,'native NPC encounter text')
    mutations.append(('wrong NPC encounter text',TEXT+strings+body+TEXT_TEST,'!coopAiBattle && ','(coopAiBattle || !coopAiBattle) && '))
    if a.check_mutations:
        for name,code,old,new in mutations:
            assert old in code,name
            run(a.cc,inc,code.replace(old,new),name,expect_failure=True)

if __name__=='__main__':main()
