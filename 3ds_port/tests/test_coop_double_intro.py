"""Exercise native intro emission and the two-endpoint controller task queues."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from test_coop_bike_hooks import function, UPSTREAM

HEADER = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;
typedef int8_t s8;typedef int16_t s16;typedef int32_t s32;typedef bool bool8;typedef bool bool32;
#define TRUE true
#define FALSE false
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define MAX_LINK_PLAYERS 4
#define PARTY_SIZE 6
#define MAX_MON_MOVES 4
struct DisableStruct;struct Sprite;
#include "constants/battle.h"
#include "battle_controllers.h"
#undef assert
#define assert(c) do {if(!(c)){fprintf(stderr,"FAIL line %d: %s (role %u frame %u flags %08x)\n",__LINE__,#c,ep->role,frame,ep->flags);exit(1);}}while(0)
enum {LINK_BUFF_BUFFER_ID,LINK_BUFF_ACTIVE_BATTLER,LINK_BUFF_ATTACKER,LINK_BUFF_TARGET,LINK_BUFF_SIZE_LO,LINK_BUFF_SIZE_HI,LINK_BUFF_ABSENT_BATTLER_FLAGS,LINK_BUFF_EFFECT_BATTLER,LINK_BUFF_DATA};
enum {SENDTASK_STATE_INITIALIZE,SENDTASK_STATE_INITIAL_DELAY,SENDTASK_STATE_COUNT_PLAYERS,SENDTASK_STATE_BEGIN_SEND_BLOCK,SENDTASK_STATE_FINISH_SEND_BLOCK,SENDTASK_STATE_UNUSED_STATE};
#define BATTLE_BUFFER_LINK_SIZE 4096
#define tState data[10]
#define tInitialDelayTimer data[11]
#define tCurrentBlock_Start data[0]
#define tCurrentBlock_End data[1]
#define tCurrentBlock_WrapFrom data[2]
#define tBlockSendDelayTimer data[3]
struct Packet {u8 bytes[600];u16 length;unsigned at;};
struct Endpoint {
    unsigned role;u32 flags,type;u8 active,attacker,target,absent,effect;
    struct {s16 data[16];} tasks[2];
    u8 send[4096],recv[4096],a[4][512],b[4][512],skip[4][4],held[4];
    struct Packet incoming[256];unsigned head,count;
    unsigned busy[4],draws[4],loads[4],throws[4],completed[4],summaries;
    void (*controllers[4])(void);
    bool direct;
};
static struct Endpoint endpoints[2],*ep;
static unsigned frame,wireDelay;
static bool cooperative=true;
static const u32 gBitTable[4]={1,2,4,8};
#define gBattleTypeFlags (ep->type)
#define gBattleControllerExecFlags (ep->flags)
#define gActiveBattler (ep->active)
#define gBattlerAttacker (ep->attacker)
#define gBattlerTarget (ep->target)
#define gAbsentBattlerFlags (ep->absent)
#define gEffectBattler (ep->effect)
#define gTasks (ep->tasks)
#define gLinkBattleSendBuffer (ep->send)
#define gLinkBattleRecvBuffer (ep->recv)
#define gBattleBufferA (ep->a)
#define gBattleBufferB (ep->b)
#define sSkippedDones (ep->skip)
#define sHeldMasterDones (ep->held)
#define sDirectBattleLink (ep->direct)
#define gBattlerControllerFuncs (ep->controllers)
#define sLinkSendTaskId 0
#define sLinkReceiveTaskId 1
static u8 GetMultiplayerId(void){return ep->role;}
static u8 GetLinkPlayerCount(void){return 2;}
static bool CtrCoop_IsDoubleBattle(void){return cooperative;}
static bool gWirelessCommType=true;
static unsigned GetLinkPlayerCount_2(void){return 2;}
static bool IsLinkMaster(void){return !ep->role;}
static void CheckShouldAdvanceLinkState(void){}
static unsigned BitmaskAllOtherLinkPlayers(void){return 0;}
static void SendBlock(unsigned mask,void *d,unsigned n){(void)mask;(void)d;(void)n;assert(false);}
static bool IsLinkTaskFinished(void){return true;}
static void PlatformLinkBattleSend(const void *data,u16 len,bool8 self){
    for(unsigned role=0;role<2;++role)if(role!=ep->role||self){
        struct Endpoint *to=&endpoints[role];assert(to->count<256);
        struct Packet *p=&to->incoming[(to->head+to->count++)%256];
        memcpy(p->bytes,data,len);p->length=len;p->at=frame+(role==ep->role?0:wireDelay);
    }
}
static unsigned PlatformLinkBattleRecv(void *out,unsigned max){
    if(!ep->count)return 0;struct Packet *p=&ep->incoming[ep->head];
    if(p->at>frame)return 0;assert(p->length<=max);memcpy(out,p->bytes,p->length);
    ep->head=(ep->head+1)%256;--ep->count;return p->length;
}
void PrepareBufferDataTransferLink(u8,u16,u8*);
static void Task_HandleSendLinkBuffersData(u8);
static void Task_HandleCopyReceivedLinkBuffersData(u8);
static void MarkBattlerReceivedLinkData(u8);
static u8 sBattleBuffersTransferData[256];
static void PlayerBufferRunCommand(void){}
static void OpponentBufferRunCommand(void){}
static void LinkPartnerBufferRunCommand(void){}
static void LinkOpponentBufferRunCommand(void){}
static void CtrCoop_FinalizeIntroHealthbox(void){}
'''

INTRO = r'''
#define SPECIES_NONE 0
#define NUM_BATTLE_STATS 8
#define DEFAULT_STAT_STAGE 6
#define FLAG_SET_SEEN 1
struct BattlePokemon {u16 species,hp;u8 types[2],ability,abilityNum,statStages[8];u32 status2,personality;};
static struct BattlePokemon gBattleMons[4];
static struct {u8 types[2];} gSpeciesInfo[500];
static struct {u16 hpOnSwitchout[2];u8 switchInAbilitiesCounter,switchInItemsCounter;bool overworldWeatherDone;} bs,*gBattleStruct=&bs;
static u8 gBattlersCount=4,gBattlerPartyIndexes[4];
static struct {u16 species;} gEnemyParty[6],gPlayerParty[6];
static struct {u16 lastOpponentSpecies;} gBattleResults;
static void (*gBattleMainFunc)(void);
static unsigned GetBattlerPosition(unsigned b){return b;}
static unsigned GetBattlerSide(unsigned b){return b&1;}
static unsigned GetAbilityBySpecies(unsigned s,unsigned a){(void)s;return a;}
static void HandleSetPokedexFlag(unsigned s,unsigned f,unsigned p){(void)s;(void)f;(void)p;}
static unsigned SpeciesToNationalPokedexNum(unsigned s){return s;}
#define MON_DATA_SPECIES 1
#define MON_DATA_SPECIES_OR_EGG 2
#define MON_DATA_HP 3
#define MON_DATA_STATUS 4
#define SPECIES_EGG 412
static unsigned GetMonData(const void *p,unsigned f,...){(void)p;(void)f;return 1;}
static void BattleArena_InitPoints(void){}
static void InitSparseDoubleBattlers(void){}
static unsigned GetBattlerAtPosition(unsigned b){return b;}
static void TryDoEventsBeforeFirstTurn(void){++ep->summaries;}
static void BattleIntroDrawPartySummaryScreens(void);
static void BattleIntroPrintTrainerWantsToBattle(void);
static void BattleIntroPrintWildMonAttacked(void);
static void BattleIntroPrintOpponentSendsOut(void);
static void BattleIntroOpponent1SendsOutMonAnimation(void);
static void BattleIntroOpponent2SendsOutMonAnimation(void);
static void BattleIntroRecordMonsToDex(void);
static void BattleIntroPrintPlayerSendsOut(void);
static void BattleIntroPlayer1SendsOutMonAnimation(void);
static void BattleIntroPlayer2SendsOutMonAnimation(void);
void BtlController_EmitPrintString(u8 bufferId,u16 stringId){
    u8 bytes[4]={CONTROLLER_PRINTSTRING,0,stringId&255,stringId>>8};
    PrepareBufferDataTransferLink(bufferId,4,bytes);
}
'''

TEST = r'''
static void init(unsigned kind){
    memset(endpoints,0,sizeof(endpoints));
    for(unsigned role=0;role<2;++role){
        ep=&endpoints[role];ep->role=role;ep->direct=true;
        ep->type=BATTLE_TYPE_LINK|BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI
            |(kind==2?0:BATTLE_TYPE_TRAINER)
            |(kind==1?BATTLE_TYPE_TWO_OPPONENTS:0)
            |(role?0:BATTLE_TYPE_IS_MASTER);
        ep->tasks[0].tState=SENDTASK_STATE_BEGIN_SEND_BLOCK;
    }
    ep=&endpoints[0];memset(gBattleMons,0,sizeof(gBattleMons));
    BattleIntroDrawTrainersOrMonsSprites();
}
static void tick(unsigned role){
    ep=&endpoints[role];Task_HandleSendLinkBuffersData(0);
    TryReceiveDirectBattleData();Task_HandleCopyReceivedLinkBuffersData(1);
    for(unsigned bank=0;bank<4;++bank){
        ep->active=bank;
        if(ep->busy[bank]){
            if(!--ep->busy[bank]){
                if(bank==(role?2:0))PlayerBufferExecCompleted();
                else if(!(bank&1))LinkPartnerBufferExecCompleted();
                else if(role)LinkOpponentBufferExecCompleted();
                else OpponentBufferExecCompleted();
                ++ep->completed[bank];
            }
        }else if(IS_BATTLE_CONTROLLER_ACTIVE_ON_LOCAL(bank)&&ep->a[bank][0]!=CONTROLLER_TERMINATOR_NOP){
            if(ep->a[bank][0]==CONTROLLER_DRAWTRAINERPIC){
                assert(!(bank&1)||(ep->type&BATTLE_TYPE_TRAINER));
                ++ep->draws[bank];
            }
            if(ep->a[bank][0]==CONTROLLER_LOADMONSPRITE)++ep->loads[bank];
            if(ep->a[bank][0]==CONTROLLER_INTROTRAINERBALLTHROW)++ep->throws[bank];
            ep->busy[bank]=8+bank*3+(role?11:0);
        }
    }
    if(!role&&!ep->flags)gBattleMainFunc();
}
int main(void){
    for(unsigned delay=0;delay<8;++delay)for(unsigned kind=0;kind<3;++kind){
        frame=0;wireDelay=delay;init(kind);
        for(frame=0;frame<1200;++frame){tick(0);tick(1);}
        for(unsigned role=0;role<2;++role){
            ep=&endpoints[role];
            for(unsigned b=0;b<4;++b){
                unsigned trainer=(kind!=2||!(b&1));
                if(ep->draws[b]!=trainer)fprintf(stderr,"unexpected trainer draw: kind=%u role=%u bank=%u count=%u expected=%u\n",kind,role,b,ep->draws[b],trainer);
                assert(ep->draws[b]==trainer);
                assert(ep->loads[b]==(kind==2&&(b&1)?1u:0u));
                assert(ep->throws[b]==trainer);assert(ep->completed[b]>0);
            }
            assert(!ep->flags);assert(!ep->count);
            assert(ep->tasks[0].tCurrentBlock_Start==ep->tasks[0].tCurrentBlock_End);
            assert(ep->tasks[1].tCurrentBlock_Start==ep->tasks[1].tCurrentBlock_End);
        }
        ep=&endpoints[0];assert(ep->summaries>0);
    }
    puts("native wild/trainer/two-trainer intros + real two-endpoint send/receive and all four controller completions: wild opponents load Pokemon only; player and NPC-trainer send-outs preserved; no stalled flags or queued commands");
    return 0;
}
'''

def code(source):
    controllers=(source/'battle_controllers.c').read_text(encoding='utf-8')
    util=(source/'battle_util.c').read_text(encoding='utf-8')
    main=(source/'battle_main.c').read_text(encoding='utf-8')
    names=['HandleOwnMessageNow','PrepareBufferDataTransferLink','Task_HandleSendLinkBuffersData',
           'TryReceiveDirectBattleData','ControllerCommandNeedsAnswer','ProcessReceivedLinkBuffer',
           'Task_HandleCopyReceivedLinkBuffersData','PrepareBufferDataTransfer','BtlController_EmitDrawTrainerPic','BtlController_EmitLoadMonSprite']
    names+=['BtlController_EmitIntroTrainerBallThrow','BtlController_EmitDrawPartyStatusSummary']
    # Rendering and text completion durations are simulated; native phase,
    # buffer, task, completion-bit and acknowledgement bodies are unchanged.
    header=HEADER+'#include "constants/battle_string_ids.h"\n'
    completions=''.join(function((source/file).read_text(encoding='utf-8'),name) for file,name in [
        ('battle_controller_player.c','PlayerBufferExecCompleted'),
        ('battle_controller_opponent.c','OpponentBufferExecCompleted'),
        ('battle_controller_link_partner.c','LinkPartnerBufferExecCompleted'),
        ('battle_controller_link_opponent.c','LinkOpponentBufferExecCompleted')])
    phases=['BattleIntroDrawTrainersOrMonsSprites','BattleIntroDrawPartySummaryScreens',
            'BattleIntroPrintTrainerWantsToBattle','BattleIntroPrintWildMonAttacked',
            'BattleIntroPrintOpponentSendsOut','BattleIntroOpponent1SendsOutMonAnimation',
            'BattleIntroOpponent2SendsOutMonAnimation','BattleIntroRecordMonsToDex',
            'BattleIntroPrintPlayerSendsOut','BattleIntroPlayer1SendsOutMonAnimation',
            'BattleIntroPlayer2SendsOutMonAnimation']
    return header+''.join(function(util,n) for n in ['MarkBattlerForControllerExec','MarkBattlerReceivedLinkData'])+''.join(function(controllers,n) for n in names)+completions+INTRO+function(util,'PrepareStringBattle')+''.join(function(main,n) for n in phases)+TEST

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=UPSTREAM/'src')
    p.add_argument('--cc',default=os.environ.get('HOSTCC','gcc'))
    p.add_argument('--check-mutations',action='store_true')
    a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='coop-intro-') as tmp:
        c=Path(tmp)/'probe.c';exe=Path(tmp)/'probe.exe';text=code(a.source)
        # Existing native loops compare s32 with sizeof. Keep their bodies
        # verbatim; suppress only that pre-existing host-width warning.
        flags=['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function',
               '-Wno-misleading-indentation','-Wno-sign-compare','-DPLATFORM_3DS',
               '-iquote',str(a.source.parent/'include')]
        def run(value,mutant=False):
            c.write_text(value,encoding='utf-8')
            subprocess.run(shlex.split(a.cc)+flags+[str(c),'-o',str(exe)],check=True,timeout=90)
            result=subprocess.run([str(exe)],capture_output=mutant,timeout=20)
            if mutant:
                assert result.returncode and b'FAIL' in result.stderr, result.stderr
                print('PASS: restored duplicate-draw bug detected by runtime assertions',flush=True)
            else:result.check_returncode()
        run(text)
        if a.check_mutations:
            old='if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI)\n         && (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)'
            assert old in text
            run(text.replace(old,'if ((gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)'),True)
            old='''|| ((gBattleTypeFlags & BATTLE_TYPE_TRAINER)
              && GetBattlerPosition(gActiveBattler) == B_POSITION_OPPONENT_RIGHT))'''
            assert old in text
            run(text.replace(old,'|| GetBattlerPosition(gActiveBattler) == B_POSITION_OPPONENT_RIGHT)'),True)
            print('PASS: restored wild-opponent trainer sprite detected on the native command path',flush=True)

if __name__=='__main__':main()
