"""Execute native run/escape/controller/field-return bodies, including script tail."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
from test_coop_bike_hooks import function, UPSTREAM

HEADER = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "constants/battle.h"
#include "constants/abilities.h"
#include "constants/items.h"
#include "constants/hold_effects.h"
#include "constants/pokemon.h"
#include "constants/battle_string_ids.h"
#include "constants/battle_script_commands.h"
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;
typedef u8 bool8; typedef u32 bool32;
#define TRUE true
#define FALSE false
#define PLATFORM_3DS
#define PYRAMID_LOCATION_NONE 0
#define BG_PLTT 0
#define BG_PLTT_SIZE 0
#define B_COMM_TO_CONTROLLER 0
#define IS_BATTLER_OF_TYPE(b,t) ((void)(b),(void)(t),false)
#define ABILITYEFFECT_CHECK_FIELD_EXCEPT_BATTLER 0
static struct { u16 item,speed,hp; u8 ability; u32 status2; } gBattleMons[4];
static struct { u8 holdEffect; } gEnigmaBerries[4];
static struct { u8 fleeType; } gProtectStructs[4];
static struct { u8 runTries; } battleStruct,*gBattleStruct=&battleStruct;
static struct { struct { u8 disableRecordBattle; } frontier; } save,*gSaveBlock2Ptr=&save;
static struct { u8 battler; } gBattleScripting;
static u32 gBattleTypeFlags,gBattleOutcome,gStatuses3[4],gBattleControllerExecFlags;
static u32 gBitTable[4]={1,2,4,8};
static u8 gActiveBattler,gBattlerAttacker,gBattlersCount=4,gAbsentBattlerFlags;
static u8 gCurrentTurnActionNumber,gCurrentActionFuncId,gBattlerByTurnOrder[4]={0,1,2,3};
static u8 gPotentialItemEffectBattler,gLastUsedAbility,gLastUsedItem;
static u8 gChosenActionByBattler[4],gBattleCommunication[8],gBattleBufferA[4][16],gBattleBufferB[4][16];
static void (*gBattlerControllerFuncs[4])(void),(*callback)(void),(*gFieldCallback)(void);
static const u8 BattleScript_PrintFailedToRunString[1]={0};
static const u8 BattleScript_PrintCantRunFromTrainer[1]={0};
#define STATE_BEFORE_ACTION_CHOSEN 0
static const u8 *gBattlescriptCurrInstr;
static void BattleScriptExecute(const u8*p){gBattlescriptCurrInstr=p;}
static bool cooperative=true; static unsigned rolls,randomValue,emits,restores,ends,clears;
static u8 transmitted;
static bool CtrCoop_IsDoubleBattle(void){return cooperative;}
static unsigned GetBattlerSide(unsigned b){return b&1;}
static unsigned GetItemHoldEffect(unsigned item){return item==ITEM_SMOKE_BALL?HOLD_EFFECT_CAN_ALWAYS_RUN:0;}
static unsigned CurrentBattlePyramidLocation(void){return PYRAMID_LOCATION_NONE;}
static unsigned GetPyramidRunMultiplier(void){return 128;}
static unsigned Random(void){++rolls;return randomValue;}
static unsigned AbilityBattleEffects(unsigned a,unsigned b,unsigned c,unsigned d,unsigned e){(void)a;(void)b;(void)c;(void)d;(void)e;return 0;}
static void ClearFuryCutterDestinyBondGrudge(unsigned b){(void)b;++clears;}
static unsigned GetBattlerAtPosition(unsigned b){return b;}
static void BtlController_EmitEndLinkBattle(unsigned comm,unsigned outcome){(void)comm;++emits;transmitted=outcome;}
static void MarkBattlerForControllerExec(unsigned b){gBattleControllerExecFlags|=1u<<b;}
static void RecordedBattle_RecordAllBattlerData(const u8 *p){(void)p;}
static void FadeOutMapMusic(unsigned n){(void)n;}
static void BeginFastPaletteFade(unsigned n){(void)n;}
static void SetBattleEndCallbacks(void){++restores;}
static void LinkPartnerBufferExecCompleted(void){gBattleControllerExecFlags=0;}
static void CB2_WhiteOut(void){}
static void CB2_ReturnToField(void){}
static void FieldCB_ReturnToFieldNoScriptCheckMusic(void){}
static void SetMainCallback2(void (*cb)(void)){callback=cb;}
static void CpuFill16(unsigned a,void *b,unsigned c){(void)a;(void)b;(void)c;}
static void ResetOamRange(unsigned a,unsigned b){(void)a;(void)b;}
static bool InBattlePike(void){return false;}
static void CtrCoop_BattleEnd(void){++ends;}
static void CtrCoop_BattleEndAfterPartyRestore(void){++ends;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
'''

TEST = r'''
static void Reset(unsigned battler){
    memset(gBattleMons,0,sizeof(gBattleMons));memset(gProtectStructs,0,sizeof(gProtectStructs));
    memset(gStatuses3,0,sizeof(gStatuses3));memset(gChosenActionByBattler,0,sizeof(gChosenActionByBattler));
    gBattleTypeFlags=BATTLE_TYPE_LINK|BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI;
    cooperative=true;gBattleOutcome=0;gCurrentTurnActionNumber=0;gBattlerByTurnOrder[0]=battler;
    gBattleStruct->runTries=0;gCurrentActionFuncId=0;rolls=emits=restores=ends=clears=0;
    gBattleControllerExecFlags=0;gAbsentBattlerFlags=0;callback=NULL;
    for(unsigned i=0;i<4;i++){gBattleMons[i].hp=12;gBattleMons[i].speed=50;}
    gBattleMons[battler].speed=150;gChosenActionByBattler[battler]=B_ACTION_RUN;
}
static void EscapeTail(unsigned kind){
    unsigned bank=gBattlerByTurnOrder[0],outcome=gBattleOutcome;const u8 *script=escapeScripts[kind];
    gBattlescriptCurrInstr=script;
    for(unsigned step=0;step<20 && gCurrentActionFuncId!=B_ACTION_TRY_FINISH;step++){
        CHECK(!gBattleControllerExecFlags);
        switch(*gBattlescriptCurrInstr){
        case 1:++gBattlescriptCurrInstr;break; // original print, wait, animation
        case 2:gBattlescriptCurrInstr=(gBattleTypeFlags&BATTLE_TYPE_LINK)?escapeEnd:gBattlescriptCurrInstr+1;break;
        case 3:Cmd_endlinkbattle();CHECK(emits==1 && transmitted==outcome);
            gActiveBattler=bank?2:0;gBattleBufferA[gActiveBattler][1]=transmitted;
            LinkPartnerHandleEndLinkBattle();CHECK(gBattleOutcome==outcome && !gBattleControllerExecFlags);
            gBattlerControllerFuncs[gActiveBattler]();CHECK(restores==1);break;
        case 4:Cmd_end2();break;
        default:CHECK(false);
        }
    }
    CHECK(gCurrentActionFuncId==B_ACTION_TRY_FINISH && emits==1);
    CHECK(!IsPlayerDefeated(gBattleOutcome));CB2_EndWildBattle();
    CHECK(callback==CB2_ReturnToField && ends==1 && gFieldCallback==FieldCB_ReturnToFieldNoScriptCheckMusic);
}
int main(void){
    for(unsigned bank=0;bank<=2;bank+=2){
        for(unsigned kind=0;kind<3;kind++){
            Reset(bank);
            if(kind==1)gBattleMons[bank].item=ITEM_SMOKE_BALL;
            if(kind==2)gBattleMons[bank].ability=ABILITY_RUN_AWAY;
            HandleAction_Run();CHECK(gBattleOutcome==B_OUTCOME_RAN && gCurrentTurnActionNumber==4);
            CHECK(gBattleMons[bank].hp==12 && gBattleMons[bank].item==(kind==1?ITEM_SMOKE_BALL:0));
            CHECK(gProtectStructs[bank].fleeType==(kind==1?FLEE_ITEM:kind==2?FLEE_ABILITY:0));EscapeTail(kind);
        }
        Reset(bank);gBattleMons[bank].speed=1;gBattleMons[1].speed=100;gBattleMons[3].speed=200;randomValue=255;
        HandleAction_Run();CHECK(!gBattleOutcome && rolls==1 && clears==1 && gBattleStruct->runTries==1);
        CHECK(gCurrentActionFuncId==B_ACTION_EXEC_SCRIPT && gBattlescriptCurrInstr==BattleScript_PrintFailedToRunString);
        gCurrentActionFuncId=0;randomValue=0;HandleAction_Run();CHECK(gBattleOutcome==B_OUTCOME_RAN);EscapeTail(0);
        Reset(bank);gBattleMons[bank].speed=75;gBattleMons[bank^1].hp=0;gBattleMons[bank^1].speed=0;
        gAbsentBattlerFlags=1u<<(bank^1);gBattleMons[(bank^1)^2].speed=200;randomValue=255;
        HandleAction_Run();CHECK(!gBattleOutcome && rolls==1); // cannot use absent opposite's zero speed
        Reset(bank);gBattleMons[bank].speed=1;gBattleMons[1].hp=gBattleMons[3].hp=0;HandleAction_Run();CHECK(gBattleOutcome==B_OUTCOME_RAN);
        Reset(bank);gActiveBattler=bank;gBattleMons[bank].status2=STATUS2_WRAPPED;
        CHECK(IsRunningFromBattleImpossible()==BATTLE_RUN_FORBIDDEN);
        gBattleMons[bank].item=ITEM_SMOKE_BALL;CHECK(IsRunningFromBattleImpossible()==BATTLE_RUN_SUCCESS);
        Reset(bank);gActiveBattler=bank;gBattleMons[1].ability=ABILITY_SHADOW_TAG;CHECK(IsRunningFromBattleImpossible()==BATTLE_RUN_FAILURE);
        gBattleMons[bank].ability=ABILITY_RUN_AWAY;CHECK(IsRunningFromBattleImpossible()==BATTLE_RUN_SUCCESS);
        for(unsigned item=0;item<2;++item){
            Reset(bank);gActiveBattler=bank;gBattleTypeFlags|=BATTLE_TYPE_TRAINER;
            gBattleMons[bank].item=item?ITEM_SMOKE_BALL:0;gBattleMons[bank].ability=ABILITY_RUN_AWAY;
            CHECK(IsRunningFromBattleImpossible()==BATTLE_RUN_FORBIDDEN);
            gBattlescriptCurrInstr=NULL;gBattleBufferB[bank][1]=B_ACTION_RUN;gBattleCommunication[bank]=7;
            CheckTrainerSelection();CHECK(gBattlescriptCurrInstr==BattleScript_PrintCantRunFromTrainer&&gBattleCommunication[bank]==STATE_BEFORE_ACTION_CHOSEN);
            HandleAction_Run();CHECK(!gBattleOutcome&&gCurrentActionFuncId==B_ACTION_EXEC_SCRIPT&&gCurrentTurnActionNumber==0);
            CHECK(!rolls&&!emits&&gBattleMons[bank].hp==12);
        }
        Reset(bank);cooperative=false;HandleAction_Run();CHECK(gBattleOutcome==(B_OUTCOME_LOST|B_OUTCOME_LINK_BATTLE_RAN));
        Reset(bank);gBattleTypeFlags&=~(BATTLE_TYPE_LINK|BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI);cooperative=false;
        HandleAction_Run();CHECK(gBattleOutcome==B_OUTCOME_RAN && !emits);
    }
    Reset(1);HandleAction_Run();CHECK(gBattleOutcome==B_OUTCOME_MON_FLED);EscapeTail(3);
    puts("native co-op wild escape and trainer refusal: both banks, trainer selection/queued RUN, smoke/ability, PvP preserved, wild END and field return passed");return 0;
}
'''


def scripts(source,cc):
    # Port assembly is preprocessed with PORT_BRIDGE (not PLATFORM_3DS).
    text=subprocess.check_output(shlex.split(cc)+['-E','-P','-x','assembler-with-cpp','-DPORT_BRIDGE','-I'+str(source.parent/'include'),str(source.parent/'data/battle_scripts_1.s')],text=True,timeout=60)
    names=['GotAwaySafely','SmokeBallEscape','RanAwayUsingMonAbility','WildMonFled']
    result=[]
    for label in names+['LinkWildEscapeEnd']:
        part=text.split('BattleScript_'+label+':',1)[1].lstrip(':')
        part=re.split(r'(?m)^[A-Za-z_]\w*::?',part,maxsplit=1)[0]
        ops=[]
        for line in part.splitlines():
            line=line.strip()
            if not line or line.startswith(('#','//')):continue
            op=line.split()[0]
            ops.append({'printstring':1,'waitmessage':1,'playanimation':1,'jumpifbattletype':2,'endlinkbattle':3,'end2':4}[op])
        result.append('static const u8 '+('escapeEnd' if label=='LinkWildEscapeEnd' else 'script'+label)+'[]={'+','.join(map(str,ops))+'};\n')
    result.append('static const u8 *escapeScripts[]={'+','.join('script'+n for n in names)+'};\n')
    return ''.join(result)


def main():
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=UPSTREAM/'src');p.add_argument('--cc',default='gcc');p.add_argument('--check-mutations',action='store_true');a=p.parse_args()
    bodies=''
    for file,names in [('battle_util.c',['TryRunFromBattle','HandleAction_Run']),('battle_main.c',['IsRunningFromBattleImpossible']),('battle_script_commands.c',['Cmd_endlinkbattle','Cmd_end2']),('battle_controller_link_partner.c',['LinkPartnerHandleEndLinkBattle']),('battle_setup.c',['IsPlayerDefeated','CB2_EndWildBattle'])]:
        text=(a.source/file).read_text(encoding='utf-8')
        bodies+=''.join(function(text,n) for n in names)
    selection=(a.source/'battle_main.c').read_text(encoding='utf-8')
    branch=selection.split('else if (gBattleTypeFlags & BATTLE_TYPE_TRAINER\n                         && (',1)[1]
    branch='if (gBattleTypeFlags & BATTLE_TYPE_TRAINER\n                         && ('+branch.split('                else if (IsRunningFromBattleImpossible()',1)[0]
    bodies+='static void CheckTrainerSelection(void) {\n'+branch+'}\n'
    definitions='\n'.join(re.findall(r'^#define B_ACTION_.*$',(a.source.parent/'include/battle.h').read_text(),re.M))+'\n'
    code=HEADER+definitions+bodies+scripts(a.source,a.cc)+TEST
    with tempfile.TemporaryDirectory(prefix='coop-escape-',dir=os.environ.get('TMPDIR')) as d:
        d=Path(d)
        variants=[('fixed',code,True)]
        if a.check_mutations:
            old=code.replace('&& !(CtrCoop_IsDoubleBattle() && !(gBattleTypeFlags & BATTLE_TYPE_TRAINER))','&& TRUE')
            variants.extend([('old-link-forfeit',old,False),('missing-peer-end',code.replace('escapeEnd[]={3,4}','escapeEnd[]={4}'),False)])
            variants.extend([('trainer-link-bypass',code.replace('|| CtrCoop_IsDoubleBattle()','|| FALSE'),False),('queued-trainer-forfeit',code.replace('if (CtrCoop_IsDoubleBattle() && (gBattleTypeFlags & BATTLE_TYPE_TRAINER))','if (FALSE)',1),False)])
        for name,content,expected in variants:
            c=d/(name+'.c');exe=d/(name+'.exe');c.write_text(content,encoding='utf-8')
            subprocess.run(shlex.split(a.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-iquote',str(a.source.parent/'include'),str(c),'-o',str(exe)],check=True,timeout=60)
            run=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            if (run.returncode==0)!=expected:raise RuntimeError(name+' unexpected result\n'+run.stdout+run.stderr)
            print(run.stdout.strip() if expected else 'detected mutation: '+name,flush=True)

if __name__=='__main__':main()
