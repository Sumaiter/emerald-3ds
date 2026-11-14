"""Compile real battle-return, WhiteOut, party-load and co-op deferral bodies."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from test_coop_bike_hooks import ROOT, UPSTREAM, function

HEADER = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constants/battle.h"
#include "constants/battle_frontier.h"
#include "constants/battle_pyramid.h"
#include "constants/trainers.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
#define TRUE true
#define FALSE false
#define PARTY_SIZE 6
#define MON_DATA_PERSONALITY 0
#define MON_DATA_OT_ID 1
#define MON_DATA_SPECIES_OR_EGG 65
#define MON_DATA_HELD_ITEM 12
#define BG_PLTT 0
#define BG_PLTT_SIZE 0
#define SE_LOW_HEALTH 0
#define SAVE_LINK 0
#define CpuFill16(a,b,c) ((void)0)
struct Pokemon { u32 personality, ot, species; u16 item, hp; };
static struct Pokemon gPlayerParty[PARTY_SIZE], gEnemyParty[PARTY_SIZE], frozen[PARTY_SIZE];
static struct { struct Pokemon playerParty[PARTY_SIZE]; u8 playerPartyCount; u32 money; }
    save1, *gSaveBlock1Ptr = &save1;
static struct { struct { u32 battlesCount; } frontier; } save2, *gSaveBlock2Ptr = &save2;
static struct { int specialTrainerBattleType; } gBattleScripting;
static struct { bool battleActive; } sCoop;
static struct {
    void (*callback1)(void), (*callback2)(void), (*savedCallback)(void);
    bool inBattle;
    unsigned state;
} gMain;
static void (*gPreBattleCallback1)(void), (*gFieldCallback)(void);
static u32 gBattleTypeFlags, gTrainerBattleOpponent_A, gBattleOutcome, gSpecialVar_Result;
static bool gReceivedRemoteLinkPlayers;
static u8 gPlayerPartyCount;
static unsigned pyramid, hill, pike, ends, checks, frozenCount, frozenMoney, trainerFlags;
static void CB2_WhiteOut(void);
static void CtrCoop_ResetBattleReturn(void);
static void CB2_Overworld(void) {}
static void CB2_ReturnToField(void) {}
static void CB2_ReturnToFieldContinueScriptPlayMapMusic(void) {}
static void FieldCB_ReturnToFieldNoScriptCheckMusic(void) {}
static void FieldCB_WarpExitFadeFromBlack(void) {}
static void CB1_Overworld(void) {}
static void SetMainCallback2(void (*cb)(void)) { gMain.callback2=cb;gMain.state=0; }
static void SetMainCallback1(void (*cb)(void)) { gMain.callback1=cb; }
static int IsPlayerDefeated(unsigned outcome) { return outcome==B_OUTCOME_LOST || outcome==B_OUTCOME_DREW; }
static int CurrentBattlePyramidLocation(void) { return pyramid; }
static int InTrainerHillChallenge(void) { return hill; }
static int InBattlePike(void) { return pike; }
static u32 GetMonData(struct Pokemon *p, int field) {
    switch(field) {
    case MON_DATA_PERSONALITY:return p->personality;
    case MON_DATA_OT_ID:return p->ot;
    case MON_DATA_SPECIES_OR_EGG:return p->species;
    case MON_DATA_HELD_ITEM:return p->item;
    default:abort();
    }
}
static void SetMonData(struct Pokemon *p,int field,const void *value) {
    (void)field;p->item=*(const u16 *)value;
}
static void CtrCoop_BattleEnd(void) {
    if(!sCoop.battleActive)return;
    CtrCoop_ResetBattleReturn();
    ++ends;frozenCount=gPlayerPartyCount;frozenMoney=save1.money;
    memcpy(frozen,gPlayerParty,sizeof(frozen));
    /* Reproduce the real sender's host-backup write: an early call destroys
     * a full team or the secret-base held-item backup, not just a counter. */
    memcpy(save1.playerParty,gPlayerParty,sizeof(gPlayerParty));
    save1.playerPartyCount=gPlayerPartyCount;sCoop.battleActive=false;
}
static void ResetOamRange(int a,int b) { (void)a;(void)b; }
static void RegisterTrainerInMatchCall(void) {}
static void SetBattledTrainersFlags(void) { ++trainerFlags; }
static void HandleRematchVarsOnBattleEnd(void) {}
static void RecordedBattle_SaveBattleOutcome(void) {}
static void UpdateGymLeaderRematch(void) {}
static void CopyEReaderTrainerFarewellMessage(void) {}
static void Overworld_ClearSavedMusic(void) {}
static void RandomlyGivePartyPokerus(struct Pokemon *p) { (void)p; }
static void PartySpreadPokerus(struct Pokemon *p) { (void)p; }
static void UpdateRoamerHPStatus(struct Pokemon *p) { (void)p; }
static void SetRoamerInactive(void) {}
static void m4aSongNumStop(int song) { (void)song; }
static const u8 EventScript_WhiteOut[1]={0};
static void RunScriptImmediately(const u8 *p) { (void)p; }
static u32 GetMoney(const u32 *p) { return *p; }
static void SetMoney(u32 *p,u32 value) { *p=value; }
static void HealPlayerParty(void) { for(unsigned i=0;i<gPlayerPartyCount;i++)gPlayerParty[i].hp=100; }
static void Overworld_ResetStateAfterWhiteOut(void) {}
static void SetWarpDestinationToLastHealLocation(void) {}
static void WarpIntoMap(void) {}
static void FieldClearVBlankHBlankCallbacks(void) {}
static void StopMapMusic(void) {}
static void ResetSafariZoneFlag_(void) {}
static void ResetInitialPlayerAvatarState(void) {}
static void ScriptContext_Init(void) {}
static void UnlockPlayerFieldControls(void) {}
static void DoMapLoadLoop(u8 *state) { (void)state; }
static void SetFieldVBlankCallback(void) {}
static void *AllocZeroed(unsigned size) { return calloc(1,size); }
static void Free(void *p) { free(p); }
static void SetContinueGameWarpStatusToDynamicWarp(void) {}
static void TrySavingData(int mode) { (void)mode; }
static void ClearContinueGameWarpStatus2(void) {}
#define CHECK(c) do { ++checks;if(!(c)){printf("battle-return check failed line %d: %s\n",__LINE__,#c);exit(1);} } while(0)
'''

HARNESS = r'''
static void Reset(unsigned outcome, unsigned flags) {
    memset(&gMain,0,sizeof(gMain));memset(&save1,0,sizeof(save1));
    CtrCoop_ResetBattleReturn();sCoop.battleActive=true;
    gBattleOutcome=outcome;gBattleTypeFlags=flags;gTrainerBattleOpponent_A=7;
    pyramid=hill=pike=ends=0;save1.money=1000;gPlayerPartyCount=save1.playerPartyCount=6;
    for(unsigned i=0;i<PARTY_SIZE;i++){
        gPlayerParty[i]=(struct Pokemon){100+i,200+i,300+i,10+i,100};
        save1.playerParty[i]=gPlayerParty[i];
    }
}
static void Return(void (*cb)(void)) {
    gMain.inBattle=true;gMain.savedCallback=cb;
    ReturnFromBattleToOverworld();
    CHECK(!gMain.inBattle && gMain.callback2==cb);
    cb();
}
static void FinishWhiteOut(void) {
    CHECK(gMain.callback2==CB2_WhiteOut);
    for(unsigned i=0;i<119;i++)CB2_WhiteOut();
    CHECK(ends==0 && save1.money==1000);
    CB2_WhiteOut();
    CHECK(ends==1 && frozenMoney==500 && frozen[0].hp==100);
    CHECK(gMain.callback2==CB2_Overworld);
}
static void ReducedParty(void) {
    /* A selected order different from the full team's first slots. */
    gPlayerParty[0]=save1.playerParty[4];gPlayerParty[1]=save1.playerParty[2];
    gPlayerParty[2]=save1.playerParty[0];
    memset(gPlayerParty+3,0,3*sizeof(*gPlayerParty));gPlayerPartyCount=3;
    gPlayerParty[0].hp=0;gPlayerParty[0].item=0;
}
static void RestoreAndFinish(void) {
    CHECK(ends==0 && sCoop.battleActive && save1.playerPartyCount==6);
    gMain.callback2=CB2_Overworld;
    SaveGameFrontier(); /* Temporary native load must not finalize. */
    CtrCoop_TryFinishBattleReturn();
    CHECK(ends==0 && sCoop.battleActive && gPlayerPartyCount==3 && save1.playerPartyCount==6);
    gMain.inBattle=true;LoadPlayerParty();CtrCoop_TryFinishBattleReturn();CHECK(ends==0);
    gMain.inBattle=false;gMain.callback2=CB2_ReturnToField;CtrCoop_TryFinishBattleReturn();CHECK(ends==0);
    gMain.callback2=CB2_Overworld;
    gPlayerParty[0].personality++;CtrCoop_TryFinishBattleReturn();CHECK(ends==0);
    gPlayerParty[0].personality--;gPlayerParty[0].ot++;CtrCoop_TryFinishBattleReturn();CHECK(ends==0);
    gPlayerParty[0].ot--;gPlayerParty[0].species++;CtrCoop_TryFinishBattleReturn();CHECK(ends==0);
    gPlayerParty[0].species--;gPlayerParty[0].hp=88;
    CtrCoop_TryFinishBattleReturn();
    CHECK(ends==1 && !sCoop.battleActive && frozenCount==6 && frozen[0].hp==88);
    CHECK(frozen[5].personality==105 && save1.playerParty[5].personality==105);
    CtrCoop_TryFinishBattleReturn();CHECK(ends==1);
}
int main(void) {
    const unsigned specialTypes[]={SPECIAL_BATTLE_TOWER,SPECIAL_BATTLE_DOME,SPECIAL_BATTLE_PALACE,
        SPECIAL_BATTLE_ARENA,SPECIAL_BATTLE_FACTORY,SPECIAL_BATTLE_PIKE_SINGLE,
        SPECIAL_BATTLE_PIKE_DOUBLE,SPECIAL_BATTLE_PYRAMID};
    const unsigned specialFlags[]={BATTLE_TYPE_BATTLE_TOWER,BATTLE_TYPE_DOME,BATTLE_TYPE_PALACE,
        BATTLE_TYPE_ARENA,BATTLE_TYPE_FACTORY,BATTLE_TYPE_BATTLE_TOWER,
        BATTLE_TYPE_BATTLE_TOWER|BATTLE_TYPE_DOUBLE,BATTLE_TYPE_PYRAMID};
    for(unsigned outcome=B_OUTCOME_WON;outcome<=B_OUTCOME_DREW;outcome++){
        /* Every ordinary defeat retains the real post-DoWhiteOut snapshot. */
        if(IsPlayerDefeated(outcome)){
            void (*callbacks[])(void)={CB2_EndWildBattle,CB2_EndScriptedWildBattle,CB2_EndTrainerBattle,CB2_EndRematchBattle};
            for(unsigned i=0;i<4;i++){
                Reset(outcome,i<2?0:BATTLE_TYPE_TRAINER);gPlayerParty[0].hp=0;
                Return(callbacks[i]);CHECK(ends==0 && sCoop.battleActive);FinishWhiteOut();
            }
        }
        Reset(outcome,BATTLE_TYPE_SECRET_BASE|BATTLE_TYPE_TRAINER);
        gBattleScripting.specialTrainerBattleType=SPECIAL_BATTLE_SECRET_BASE;
        gPlayerParty[0].item=0;
        Return(HandleSpecialTrainerBattleEnd);
        CHECK(ends==1 && frozen[0].item==10 && save1.playerParty[0].item==10);
        CHECK(gMain.callback2==CB2_ReturnToFieldContinueScriptPlayMapMusic);
        Reset(outcome,BATTLE_TYPE_EREADER_TRAINER|BATTLE_TYPE_TRAINER);
        gBattleScripting.specialTrainerBattleType=SPECIAL_BATTLE_EREADER;
        Return(HandleSpecialTrainerBattleEnd);CHECK(ends==1);
        /* Legacy secret-base trainer/rematch return paths also avoid WhiteOut. */
        Reset(outcome,BATTLE_TYPE_SECRET_BASE|BATTLE_TYPE_TRAINER);gTrainerBattleOpponent_A=TRAINER_SECRET_BASE;
        Return(CB2_EndTrainerBattle);CHECK(ends==1);
        Reset(outcome,BATTLE_TYPE_SECRET_BASE|BATTLE_TYPE_TRAINER);gTrainerBattleOpponent_A=TRAINER_SECRET_BASE;
        Return(CB2_EndRematchBattle);CHECK(ends==1);
        Reset(outcome,BATTLE_TYPE_TRAINER_HILL|BATTLE_TYPE_TRAINER);hill=1;
        Return(CB2_EndTrainerBattle);CHECK(ends==1);
        for(unsigned i=0;i<sizeof(specialTypes)/sizeof(*specialTypes);i++){
            Reset(outcome,BATTLE_TYPE_TRAINER|specialFlags[i]);ReducedParty();
            gBattleScripting.specialTrainerBattleType=specialTypes[i];
            Return(HandleSpecialTrainerBattleEnd);RestoreAndFinish();
        }
        void (*pyramidCallbacks[])(void)={CB2_EndWildBattle,CB2_EndScriptedWildBattle,CB2_EndTrainerBattle};
        for(unsigned i=0;i<3;i++){
            Reset(outcome,BATTLE_TYPE_PYRAMID);pyramid=1;ReducedParty();
            Return(pyramidCallbacks[i]);RestoreAndFinish();
        }
        Reset(outcome,BATTLE_TYPE_PIKE);pike=1;ReducedParty();
        Return(CB2_EndWildBattle);RestoreAndFinish();
        Reset(outcome,BATTLE_TYPE_TRAINER|BATTLE_TYPE_INGAME_PARTNER);ReducedParty();
        gBattleScripting.specialTrainerBattleType=SPECIAL_BATTLE_STEVEN;
        Return(HandleSpecialTrainerBattleEnd);
        CHECK(ends==0 && sCoop.battleActive && save1.playerPartyCount==6);
        LoadPlayerParty();gMain.callback2=CB2_Overworld;CtrCoop_TryFinishBattleReturn();
        if(IsPlayerDefeated(outcome)){
            CHECK(ends==0);SetMainCallback2(CB2_WhiteOut);FinishWhiteOut();
        }else CHECK(ends==1 && frozenCount==6);
    }
    /* Even a selected 3-of-3 or 6-of-6 party with identical identities must
     * not finalize on SaveGameFrontier's deliberately temporary full load. */
    for(unsigned count=3;count<=6;count+=3){
        Reset(B_OUTCOME_WON,BATTLE_TYPE_BATTLE_TOWER|BATTLE_TYPE_TRAINER);
        gPlayerPartyCount=save1.playerPartyCount=count;
        gBattleScripting.specialTrainerBattleType=SPECIAL_BATTLE_TOWER;
        Return(HandleSpecialTrainerBattleEnd);gMain.callback2=CB2_Overworld;
        CtrCoop_TryFinishBattleReturn();CHECK(ends==0);
        SaveGameFrontier();CtrCoop_TryFinishBattleReturn();
        CHECK(ends==0 && !sBattlePartyRestoreSeen && !sBattleIgnoreNextPartyRestore);
        LoadPlayerParty();CtrCoop_TryFinishBattleReturn();CHECK(ends==1 && frozenCount==count);
    }
    /* Regular wins/catches/escape and direct Safari callbacks keep the
     * existing immediate result path; cancelled/link exits don't finalize. */
    for(unsigned outcome=B_OUTCOME_WON;outcome<=B_OUTCOME_CAUGHT;outcome++){
        if(IsPlayerDefeated(outcome))continue;
        Reset(outcome,0);Return(CB2_ReturnToField);CHECK(ends==1);
    }
    Reset(B_OUTCOME_LOST,BATTLE_TYPE_FIRST_BATTLE);Return(CB2_EndFirstBattle);CHECK(ends==1);
    Reset(B_OUTCOME_WON,BATTLE_TYPE_LINK);gReceivedRemoteLinkPlayers=true;gMain.inBattle=true;
    ReturnFromBattleToOverworld();CHECK(ends==0 && gMain.inBattle);gReceivedRemoteLinkPlayers=false;
    Reset(B_OUTCOME_WON,BATTLE_TYPE_BATTLE_TOWER);ReducedParty();
    CtrCoop_BattleEndAfterPartyRestore();LoadPlayerParty();CtrCoop_ResetBattleReturn();
    gMain.callback2=CB2_Overworld;CtrCoop_TryFinishBattleReturn();CHECK(ends==0);
    sCoop.battleActive=false;CtrCoop_BattleEndAfterPartyRestore();CtrCoop_BattlePartyRestored();
    CtrCoop_TryFinishBattleReturn();CHECK(ends==0);
    printf("actual engine battle returns/party restores/WhiteOut passed: %u checks\n",checks);
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=UPSTREAM / 'src')
    parser.add_argument('--backend', type=Path, default=ROOT / '3ds_port/src/3ds_coop.c')
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    args = parser.parse_args()
    backend = args.backend.read_text()
    start = backend.index('/* A facility challenge may keep a reduced/rental party')
    end = backend.index('static bool UsesOrdinaryTrainerFlag', start)
    bodies = backend[start:end]
    for file, names in {
        'battle_main.c': ('ReturnFromBattleToOverworld',),
        'battle_setup.c': ('CB2_EndWildBattle', 'CB2_EndScriptedWildBattle',
                           'CB2_EndTrainerBattle', 'CB2_EndRematchBattle', 'CB2_EndFirstBattle'),
        'battle_tower.c': ('HandleSpecialTrainerBattleEnd',),
        'load_save.c': ('LoadPlayerParty',),
        'overworld.c': ('DoWhiteOut', 'CB2_WhiteOut'),
    }.items():
        text = (args.source / file).read_text()
        bodies += ''.join(function(text, name) for name in names)
    bodies += function((args.source / 'frontier_util.c').read_text(), 'SaveGameFrontier')
    # Match engine linkage; prototypes above intentionally precede extracted definitions.
    source_text = (HEADER + bodies + HARNESS).replace(
        'static void CtrCoop_ResetBattleReturn(void);', 'void CtrCoop_ResetBattleReturn(void);'
    ).replace('static void CB2_WhiteOut(void);', 'void CB2_WhiteOut(void);')
    with tempfile.TemporaryDirectory(prefix='coop-battle-return-') as directory:
        directory = Path(directory)
        source = directory / 'returns.c'
        exe = directory / ('returns.exe' if os.name == 'nt' else 'returns')
        source.write_text(source_text)
        subprocess.run(shlex.split(args.cc) + ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror',
                       '-DPLATFORM_3DS', '-iquote', str(UPSTREAM / 'include'),
                       str(source), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
