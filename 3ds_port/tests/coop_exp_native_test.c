/* Extracted native EXP commands/controllers; deterministic Pokemon data IO. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32; typedef bool bool8; typedef bool bool32;
#define TRUE true
#define FALSE false
#define PARTY_SIZE 6
#define MULTI_PARTY_SIZE 3
#define MAX_MON_MOVES 4
#define MAX_LINK_PLAYERS 4
#define PLAYER_NAME_LENGTH 7
struct Sprite; struct DisableStruct;
#include "constants/battle.h"
#include "constants/pokemon.h"
#include "constants/items.h"
#include "constants/hold_effects.h"
#include "constants/species.h"
#include "constants/battle_string_ids.h"
#include "constants/battle_anim.h"
#include "constants/songs.h"
#include "constants/abilities.h"
#include "battle_controllers.h"
#include "exp_mon_fields.inc"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s role=%u bank=%u state=%u\n",__LINE__,#c,role,gActiveBattler,gBattleScripting.getexpState);exit(1);}}while(0)
#define T1_READ_16(p) ((p)[0]|((p)[1]<<8))
#define SAFE_DIV(a,b) ((b)?(a)/(b):0)
#define B_WIN_MSG 0
#define PALETTES_ALL 0
#define RGB_BLACK 0
#define WINDOW_CLEAR 0
#define YESNOBOX_X_Y 0
#define PARTY_EXP_SHARE() (share)
#define MOVE_IS_PERMANENT(b,s) (!(gBattleMons[b].status2&STATUS2_TRANSFORMED) && !(gDisableStructs[b].mimickedMoves&(1u<<(s))))
struct Pokemon {
    u16 species,hp,maxHP,attack,defense,speed,spAttack,spDefense,moves[4];
    u32 experience,otId;
    u8 level,friendship,ev,pp[4],ppBonuses,name[8];
    u16 item;u8 pad[50];
};
_Static_assert(sizeof(struct Pokemon)==100,"real wire Pokemon size");
struct BattlePokemon {u16 species,hp,maxHP,attack,defense,speed,spAttack,spDefense,moves[4],item;u8 level,pp[4],ppBonuses,ability;u32 status2;};
static struct Pokemon gPlayerParty[6],guestParty[6];
static struct BattlePokemon gBattleMons[4];
static struct {u8 mimickedMoves;} gDisableStructs[4];
static struct {u16 expValue;u8 wildVictorySong,givenExpMons,expGetterMonId,expGetterBattlerId,sentInPokes;} bs,*gBattleStruct=&bs;
static struct {u8 getexpState,learnMoveState,drawlvlupboxState;} gBattleScripting;
static struct {u16 stats[6];} stats;
static struct {void *unused;typeof(stats)*beforeLvlUp;} resources={.beforeLvlUp=&stats},*gBattleResources=&resources;
static struct {u8 playerTrainerId[4];struct {u8 holdEffect;} enigmaBerry;} save,*gSaveBlock1Ptr=&save,*gSaveBlock2Ptr=&save;
static struct {u8 name[8];} gLinkPlayers[2];
static struct {u16 expYield;u8 growthRate;} gSpeciesInfo[500];
static u32 gExperienceTables[1][102];
static u32 gBattleTypeFlags,gBattleControllerExecFlags,gLeveledUpInBattle;
static u8 gActiveBattler,gBattlerFainted,gAbsentBattlerFlags,gBattlerPartyIndexes[4],gSentPokesToOpponent[2],gBattleBufferA[4][512],gBattleBufferB[4][512];
static const u32 gBitTable[6]={1,2,4,8,16,32};
static s32 gBattleMoveDamage;static u16 gExpShareExp;
static u8 instr[16],gBattleTextBuff1[40],gBattleTextBuff2[40],gBattleTextBuff3[40];
static const u8 *gBattlescriptCurrInstr=instr;
static const u8 BattleScript_LevelUp[2]={0};
static bool share,cooperative=true,borrowed,sTeamExpShown;
static unsigned role,initiator,ownerReplies,done[2],messages[6],evAwards[6],wireLen,wireCount,rawCount,levelMessages;
static u8 wire[512];
static void (*gBattlerControllerFuncs[4])(void);
static struct {void(*func)(u8);s16 data[16];bool active;} gTasks[16];
static struct {struct {bool specialAnimActive;} healthBoxesData[4];} sprites,*gBattleSpritesDataPtr=&sprites;
void BattleControllerDummy(void){}
static bool CtrCoop_IsDoubleBattle(void){return cooperative;}
static unsigned CtrExtras_Get(const char *key){(void)key;return 0;}
static bool CtrCoop_BattleMonEarnsExp(u8 i){return !borrowed || (initiator==0?i<3:i>=3);}
static unsigned GetBattlerMultiplayerId(unsigned b){return b/2;}
static unsigned GetMultiplayerId(void){return role;}
static unsigned GetBattlerForBattleScript(unsigned b){return b;}
static unsigned GetBattlerSide(unsigned b){return b&1;}
static unsigned GetItemHoldEffect(unsigned item){return item==ITEM_EXP_SHARE?HOLD_EFFECT_EXP_SHARE:0;}
static u32 GetMonData(const struct Pokemon*p,unsigned f,...){
    if(f==MON_DATA_OT_ID)return p->otId;
    if(f==MON_DATA_OT_NAME){va_list ap;va_start(ap,f);u8 *dest=va_arg(ap,u8*);memcpy(dest,p->name,8);va_end(ap);return 0;}
    switch(f){case MON_DATA_SPECIES:return p->species;case MON_DATA_HP:return p->hp;case MON_DATA_MAX_HP:return p->maxHP;
    case MON_DATA_LEVEL:return p->level;case MON_DATA_EXP:return p->experience;case MON_DATA_HELD_ITEM:return p->item;
    case MON_DATA_ATK:return p->attack;case MON_DATA_DEF:return p->defense;case MON_DATA_SPEED:return p->speed;
    case MON_DATA_SPATK:return p->spAttack;case MON_DATA_SPDEF:return p->spDefense;case MON_DATA_IS_EGG:return p->pad[0];
    case MON_DATA_PP_BONUSES:return p->ppBonuses;default:
      if(f>=MON_DATA_MOVE1&&f<=MON_DATA_MOVE4)return p->moves[f-MON_DATA_MOVE1];
      if(f>=MON_DATA_PP1&&f<=MON_DATA_PP4)return p->pp[f-MON_DATA_PP1];return 0;}
}
static void SetMonData(struct Pokemon*p,unsigned f,const void*v){
    if(f==MON_DATA_EXP)memcpy(&p->experience,v,4);
}
static void CalculateMonStats(struct Pokemon*p){
    while(p->level<100&&p->experience>=gExperienceTables[0][p->level+1]){++p->level;++p->hp;++p->maxHP;++p->spDefense;}
}
static bool8 IsTradedMon(struct Pokemon*p){return p->otId!=123;}
static unsigned StringCompare(const u8*a,const u8*b){for(unsigned i=0;i<8;++i){if(a[i]!=b[i])return 1;if(a[i]==255)return 0;}return 0;}
static void BattleStopLowHpSound(void){}
static void PlayBGM(unsigned s){(void)s;}
static void MonGainEVs(struct Pokemon*p,unsigned species){(void)species;++p->ev;++evAwards[p-gPlayerParty];}
static void PrepareStringBattle(unsigned id,unsigned b){(void)b;if(id==STRINGID_PKMNGAINEDEXP)++messages[bs.expGetterMonId];}
#define PREPARE_MON_NICK_WITH_PREFIX_BUFFER(a,b,c) ((void)(a),(void)(b),(void)(c))
#define PREPARE_STRING_BUFFER(a,b) ((void)(a),(void)(b))
#define PREPARE_WORD_NUMBER_BUFFER(a,b,c) ((void)(a),(void)(b),(void)(c))
#define PREPARE_BYTE_NUMBER_BUFFER(a,b,c) ((void)(a),(void)(b),(void)(c))
static void HandleLowHpMusicChange(struct Pokemon*p,unsigned b){(void)p;(void)b;}
static void BattleScriptPushCursor(void){++levelMessages;}
static void AdjustFriendship(struct Pokemon*p,unsigned e){(void)e;++p->friendship;}
static void PrepareBufferDataTransfer(u8 id,const void*p,unsigned n){(void)id;CHECK(n<=sizeof(wire));memcpy(wire,p,n);wireLen=n;++wireCount;}
static u8 sBattleBuffersTransferData[512];
static void MarkBattlerForControllerExec(unsigned b){(void)b;gBattleControllerExecFlags=1;}
static void PlayerBufferExecCompleted(void){++done[role];gBattlerControllerFuncs[gActiveBattler]=NULL;}
void CtrCoop_LinkPartnerComplete(void){++done[role];gBattlerControllerFuncs[gActiveBattler]=NULL;}
static bool IsTextPrinterActive(unsigned id){(void)id;return false;}
static bool IsDoubleBattle(void){return !!(gBattleTypeFlags&BATTLE_TYPE_DOUBLE);}
void BtlController_EmitTwoReturnValues(u8 id,u8 ret,u16 xp){(void)id;++ownerReplies;gBattleBufferB[gActiveBattler][0]=CONTROLLER_TWORETURNVALUES;gBattleBufferB[gActiveBattler][1]=ret;gBattleBufferB[gActiveBattler][2]=xp;gBattleBufferB[gActiveBattler][3]=xp>>8;}
static void DestroyTask(unsigned t){gTasks[t].active=false;}
static unsigned CreateTask(void(*fn)(u8),unsigned pri){(void)pri;for(unsigned t=0;t<16;++t)if(!gTasks[t].active){memset(&gTasks[t],0,sizeof(gTasks[t]));gTasks[t].active=true;gTasks[t].func=fn;return t;}CHECK(false);return 0;}
static void LoadBattleBarGfx(unsigned k){(void)k;}
static void InitAndLaunchSpecialAnimation(unsigned a,unsigned b,unsigned c,unsigned d){(void)a;(void)b;(void)c;(void)d;}
static void UpdateHealthboxAttribute(unsigned a,struct Pokemon*p,unsigned c){(void)a;(void)p;(void)c;}
static u8 gHealthboxSpriteIds[4];
static u8 gBattlerInMenuId;
static struct {bool active;} gPaletteFade;
static struct {void (*callback2)(void);} gMain;
static unsigned summaries,learnAnswers,selectedSlot;
static u16 gMoveToLearn=77;
static void BattleMainCB2(void){}
static void ReshowBattleScreenAfterMenu(void){}
static void FreeAllWindowBuffers(void){}
static void ShowSelectMovePokemonSummaryScreen(struct Pokemon*p,u8 mon,u8 last,void(*cb)(void),u16 move){CHECK(p==gPlayerParty&&mon==3&&last==5&&cb==ReshowBattleScreenAfterMenu&&move==77);++summaries;}
static unsigned GetMoveSlotToReplace(void){return selectedSlot;}
void BtlController_EmitOneReturnValue(u8 id,u16 slot){(void)id;++learnAnswers;gBattleBufferB[gActiveBattler][0]=CONTROLLER_ONERETURNVALUE;gBattleBufferB[gActiveBattler][1]=slot;}
static void BeginNormalPaletteFade(unsigned a,unsigned b,unsigned c,unsigned d,unsigned e){(void)a;(void)b;(void)c;(void)d;(void)e;gPaletteFade.active=true;}
static void HandleBattleWindow(unsigned a,unsigned b){(void)a;(void)b;}
static void BattlePutTextOnWindow(const u8*p,unsigned w){(void)p;(void)w;}
static u8 gMultiUsePlayerCursor;
static const u8 gText_BattleYesNoChoice[]={0};
static void BattleCreateYesNoCursorAt(unsigned n){(void)n;}
static void PlayerHandleYesNoInput(void){}
static bool IsHMMove2(unsigned m){return m==99;}
#define PREPARE_MOVE_BUFFER(a,b) ((void)(a),(void)(b));
static void RemoveMonPPBonus(struct Pokemon*p,unsigned slot){p->ppBonuses&=~(3u<<(slot*2));}
static void SetMonMoveSlot(struct Pokemon*p,unsigned move,unsigned slot){p->moves[slot]=move;p->pp[slot]=20;}
static void RemoveBattleMonPPBonus(struct BattlePokemon*p,unsigned slot){p->ppBonuses&=~(3u<<(slot*2));}
static void SetBattleMonMoveSlot(struct BattlePokemon*p,unsigned move,unsigned slot){p->moves[slot]=move;p->pp[slot]=20;}
static const u8 *learnJump=instr+8;
#define T1_READ_PTR(p) ((void)(p),learnJump)
#define HEALTHBOX_ALL 0
#define tExpTask_monId data[0]
#define tExpTask_gainedExp data[1]
#define tExpTask_battler data[2]
static void Task_PrepareToGiveExpWithExpBar(u8 t){(void)t;CHECK(false);}
static void Task_LaunchLvlUpAnim(u8);
static void Task_UpdateLvlInHealthbox(u8);
static void DestroyExpTaskAndCompleteOnInactiveTextPrinter(u8);
static unsigned gBattleOutcome,evolutions,worldReturns;
static void (*gCB2_AfterEvolution)(void),(*gBattleMainFunc)(void);
static void TryEvolvePokemon(void);
static void WaitForEvoSceneToFinish(void);
static void ReturnFromBattleToOverworld(void){++worldReturns;}
static unsigned CtrCoop_LocalMultiplayerId(void){return role;}
static void ResetSpriteData(void){}
static bool monGfxAllocated;
static void FreeMonSpritesGfx(void){monGfxAllocated=false;}
static void CtrCoop_PrepareDoubleReturn(void){if(role)memmove(gPlayerParty,gPlayerParty+3,3*sizeof(*gPlayerParty));}
static u8 CtrCoop_EnginePrepareEvolutionParty(u8 bits){return bits;}
static unsigned GetEvolutionTargetSpecies(struct Pokemon*p,unsigned mode,unsigned bits){(void)mode;(void)bits;return p->species==1&&p->level>=6?2:SPECIES_NONE;}
static void EvolutionBusy(void){}
static void EvolutionScene(struct Pokemon*p,unsigned species,bool battle,unsigned index){CHECK(battle&&index<3&&!monGfxAllocated);monGfxAllocated=true;++evolutions;p->species=species;gMain.callback2=EvolutionBusy;}
#include "exp_native.inc"

static void runController(void){
    PlayerHandleExpUpdate();
    for(unsigned f=0;f<100;++f){
        bool active=false;for(unsigned t=0;t<16;++t)if(gTasks[t].active){active=true;gTasks[t].func(t);}
        if(gBattlerControllerFuncs[gActiveBattler]){active=true;gBattlerControllerFuncs[gActiveBattler]();}
        if(!active)return;
    }CHECK(false);
}
static void deliver(void){
    struct Pokemon master[6];u8 reply[512];
    memcpy(gBattleBufferA[gActiveBattler],wire,wireLen);
    if(wire[0]==CONTROLLER_EXPUPDATE){
        CHECK(wireLen==104);role=0;runController();memcpy(master,gPlayerParty,sizeof(master));
        memcpy(gPlayerParty,guestParty,sizeof(guestParty));role=1;runController();memcpy(guestParty,gPlayerParty,sizeof(guestParty));
        memcpy(reply,gBattleBufferB[gActiveBattler],sizeof(reply));memcpy(gPlayerParty,master,sizeof(master));memcpy(gBattleBufferB[gActiveBattler],reply,sizeof(reply));
    }else{CHECK(wire[0]==CONTROLLER_SETRAWMONDATA);++rawCount;
        CHECK(CtrCoop_ApplyPartyProgress());memcpy(master,gPlayerParty,sizeof(master));
        memcpy(gPlayerParty,guestParty,sizeof(guestParty));CHECK(CtrCoop_ApplyPartyProgress());memcpy(guestParty,gPlayerParty,sizeof(guestParty));memcpy(gPlayerParty,master,sizeof(master));
    }
    role=0;gBattleControllerExecFlags=0;
}
static void runExp(void){
    instr[1]=1;gBattlescriptCurrInstr=instr;gBattleScripting.getexpState=0;
    for(unsigned f=0;f<400;++f){
        Cmd_getexp();if(gBattleControllerExecFlags)deliver();
        if(gBattlescriptCurrInstr==BattleScript_LevelUp){
            // Simulate script completion after checking the authoritative
            // native state; learn/move UI has its own extracted-body probe.
            CHECK(gPlayerParty[bs.expGetterMonId].level>5);gBattlescriptCurrInstr=instr;
        }
        if(gBattlescriptCurrInstr==instr+2)return;
    }CHECK(false);
}
int main(void){
    save.playerTrainerId[0]=123;memset(gLinkPlayers,255,sizeof(gLinkPlayers));gLinkPlayers[0].name[0]=0xbb;gLinkPlayers[1].name[0]=0xcc;
    for(unsigned i=0;i<102;++i)gExperienceTables[0][i]=i*100;
    gSpeciesInfo[1].expYield=140;gBattleMons[1]=(struct BattlePokemon){.species=1,.level=7};
    gBattlerPartyIndexes[0]=0;gBattlerPartyIndexes[2]=3;
    for(unsigned b=0;b<2;++b)for(unsigned init=0;init<2;++init)for(unsigned xpShare=0;xpShare<2;++xpShare){
        borrowed=b;initiator=init;share=xpShare;cooperative=true;ownerReplies=0;rawCount=0;
        memset(messages,0,sizeof(messages));memset(evAwards,0,sizeof(evAwards));memset(done,0,sizeof(done));memset(&bs,0,sizeof(bs));
        gBattleTypeFlags=BATTLE_TYPE_LINK|BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI|BATTLE_TYPE_TRAINER;
        gSentPokesToOpponent[0]=9;gBattleMons[0].hp=20;gBattleMons[2].hp=20;
        for(unsigned i=0;i<6;++i)gPlayerParty[i]=(struct Pokemon){.species=1,.hp=20,.maxHP=20,.level=5,.experience=590,.otId=123};
        for(unsigned i=0;i<6;++i)memcpy(gPlayerParty[i].name,gLinkPlayers[i/3].name,8);
        // Slot 1 is fainted, slot 5 is an egg. Neither may earn experience.
        gPlayerParty[1].hp=0;gPlayerParty[5].pad[0]=1;
        memcpy(guestParty,gPlayerParty,sizeof(guestParty));runExp();
        CHECK(!memcmp(gPlayerParty,guestParty,sizeof(guestParty)));CHECK(rawCount==6);
        unsigned levels=0;
        for(unsigned i=0;i<6;++i){
            bool gets=CtrCoop_BattleMonEarnsExp(i)&&i!=1&&i!=5&&(i==0||i==3||share);
            CHECK((gPlayerParty[i].experience>590)==gets);CHECK(messages[i]==(unsigned)gets);CHECK(evAwards[i]==(unsigned)gets);
            if(gets){unsigned amount=(share||borrowed?140:70)*150/100;if(i!=0&&i!=3)amount=105;CHECK(gPlayerParty[i].experience==590+amount);}
            if(gets)levels+=gPlayerParty[i].level-5;
        }
        CHECK(ownerReplies==levels);CHECK(done[0]==done[1]);
    }
    cooperative=false;memset(messages,0,sizeof(messages));runExp();for(unsigned i=0;i<6;++i)CHECK(!messages[i]);
    cooperative=true;borrowed=false;gBattleStruct->expGetterMonId=3;gBattleMons[2].hp=20;
    CHECK(!CoopIsTradedExpMon(0)&&!CoopIsTradedExpMon(3));
    ++gPlayerParty[3].otId;CHECK(CoopIsTradedExpMon(3));--gPlayerParty[3].otId;
    gPlayerParty[3].name[0]=0xab;CHECK(CoopIsTradedExpMon(3));gPlayerParty[3].name[0]=0xcc;
    for(unsigned choice=0;choice<5;++choice){
        gBattleScripting.learnMoveState=0;gBattlescriptCurrInstr=instr;gBattleControllerExecFlags=0;
        CoopLearnMove();CHECK(gBattleScripting.learnMoveState==1);deliver();
        CoopLearnMove();CHECK(gBattleScripting.learnMoveState==2&&wire[0]==CONTROLLER_YESNOBOX&&wireLen==5);
        memcpy(gBattleBufferA[2],wire,wireLen);role=1;gActiveBattler=2;selectedSlot=choice;
        PlayerHandleYesNoBox();CHECK(gPaletteFade.active);gPaletteFade.active=false;
        CoopChooseLearnMove();CHECK(sCoopLearnMoveState==1);gMain.callback2=BattleMainCB2;
        CoopChooseLearnMove();CHECK(learnAnswers==choice+1);
        role=0;gBattleControllerExecFlags=0;CoopLearnMove();
        CHECK(gBattlescriptCurrInstr==(choice==4?instr+5:learnJump));
        if(choice<4)CHECK(gPlayerParty[3].moves[choice]==77&&gBattleMons[2].moves[choice]==77);
    }
    CHECK(summaries==5);
    for(unsigned r=0;r<2;++r)for(unsigned npc=0;npc<2;++npc)for(unsigned leader=0;leader<2;++leader){
        role=r;borrowed=npc;initiator=leader;gBattleOutcome=B_OUTCOME_WON;gLeveledUpInBattle=63;evolutions=worldReturns=0;monGfxAllocated=true;
        for(unsigned i=0;i<6;++i)gPlayerParty[i]=(struct Pokemon){.species=1,.level=6};
        CtrCoop_BeginBattleEvolutions();CHECK(gCB2_AfterEvolution==BattleMainCB2);
        for(unsigned tick=0;tick<12&&!worldReturns;++tick){gBattleMainFunc();if(gMain.callback2==EvolutionBusy){monGfxAllocated=false;gMain.callback2=BattleMainCB2;}}
        CHECK(worldReturns==1&&evolutions==(npc&&leader!=r?0u:3u));
        for(unsigned i=0;i<6;++i)CHECK(gPlayerParty[i].species==(i<3&&(!npc||leader==r)?2:1));
    }
    puts("native post-battle evolution loop: each owner only, borrowed team excluded, one native return passed");
    puts("native owner-only learn-move summary: all four replacement slots, cancel, authoritative moves/PP passed");
    puts("native cooperative EXP: two owners/mirrors, EV once, levels/residual EXP, per-mon messages, NPC/fainted/egg exclusion; PvP exclusion passed");return 0;
}
