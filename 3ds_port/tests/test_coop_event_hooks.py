"""Execute the staged script and battle-transition gates with side-effect probes."""
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
typedef uint8_t u8;
typedef int16_t s16;
typedef bool bool8;
#define TRUE true
#define FALSE false
#define CONTEXT_SHUTDOWN 2
#define CONTEXT_WAITING 1
#define CONTEXT_RUNNING 0
#define NUM_TASKS 16
#define TRSEE_NONE 0
#define tFuncId data[0]
#define tState data[0]
#define tTransition data[1]
struct ScriptContext { int unused; };
struct Task { bool isActive; void (*func)(u8); s16 data[16]; } gTasks[NUM_TASKS];
struct { void (*callback2)(void); } gMain;
struct { u8 taskId; } gApproachingTrainers[2];
static u8 gNoOfApproachingTrainers, gApproachingTrainerId;
static bool gTrainerApproachedPlayer;
static u8 sGlobalScriptContextStatus;
static struct ScriptContext sGlobalScriptContext;
static int gScriptCmdTable[1], gScriptCmdTableEnd[1];
static int permission, polls, begins, ends, commands, transitions, cleanup, destroyed;
static bool locked, more, battleAllowed, cancelCallback, poison;
static void CB2_Overworld(void) {}
static void CB2_InitBattle(void) {}
static void SavedCallback(void) {}
static void Task_RunTrainerSeeFuncList(u8 id) { (void)id; }
static void LockPlayerFieldControls(void) { locked=true; }
static void UnlockPlayerFieldControls(void) { locked=false; }
static void InitScriptContext(struct ScriptContext *c,void *a,void *b) { (void)c;(void)a;(void)b; }
static void SetupBytecodeScript(struct ScriptContext *c,const u8 *p) { (void)c;(void)p; }
static void StopScript(struct ScriptContext *c) { (void)c; }
static bool RunScriptCommand(struct ScriptContext *c) { (void)c;++commands;return more; }
static void CtrCoop_ScriptBegin(const void *p) { (void)p;++begins; }
static int CtrCoop_ScriptPoll(void) { ++polls;return permission; }
static void CtrCoop_ScriptEnd(void) { ++ends; }
static bool CtrCoop_BattleStart(void) { if(cancelCallback)gMain.callback2=SavedCallback;return battleAllowed; }
static bool FldEffPoison_IsActive(void) { return poison; }
static void BattleTransition_StartOnField(s16 t) { (void)t;++transitions; }
static void ClearMirageTowerPulseBlendEffect(void) {}
static bool IsBattleTransitionDone(void) { return true; }
static void CleanupOverworldWindowsAndTilemaps(void) { ++cleanup; }
static void SetMainCallback2(void (*p)(void)) { gMain.callback2=p; }
static void RestartWildEncounterImmunitySteps(void) {}
static void ClearPoisonStepCounter(void) {}
static void DestroyTask(u8 id) { gTasks[id].isActive=false;++destroyed; }
#define CHECK(c) do { if(!(c)) { printf("engine event gate failed line %d\n",__LINE__);exit(1); } } while(0)
'''

HARNESS = r'''
int main(void)
{
    ScriptContext_Init();
    ends=0;
    ScriptContext_SetupScript((const u8 *)1);
    CHECK(begins==1 && locked);
    permission=0;more=true;
    CHECK(ScriptContext_RunScript() && !commands && !ends && locked);
    permission=1;
    CHECK(ScriptContext_RunScript() && commands==1 && !ends);
    ScriptContext_Stop();
    int oldPolls=polls;
    CHECK(!ScriptContext_RunScript() && !ends && polls==oldPolls);
    /* Battle waitstate/return and a replacement continuation keep the lease. */
    ScriptContext_SetupScript((const u8 *)2);
    CHECK(begins==1);
    ScriptContext_Enable();more=false;
    CHECK(!ScriptContext_RunScript() && commands==2 && ends==1 && !locked);
    ScriptContext_SetupScript((const u8 *)3);
    CHECK(begins==2);
    gNoOfApproachingTrainers=2;gTrainerApproachedPlayer=true;
    gApproachingTrainers[0].taskId=3;gApproachingTrainers[1].taskId=4;
    gTasks[3].isActive=gTasks[4].isActive=true;
    gTasks[3].func=gTasks[4].func=Task_RunTrainerSeeFuncList;
    gTasks[4].tFuncId=1; /* An unrelated active approach is never destroyed. */
    permission=-1;
    CHECK(!ScriptContext_RunScript() && commands==2 && ends==2 && !locked);
    CHECK(!gTasks[3].isActive && gTasks[4].isActive && !gTrainerApproachedPlayer && !gNoOfApproachingTrainers);
    CHECK(!ScriptContext_RunScript() && commands==2); /* No continuation after dismiss. */
    ScriptContext_Init();CHECK(ends==3);

    gTasks[0].isActive=true;gMain.callback2=CB2_Overworld;
    battleAllowed=false;
    Task_BattleStart(0);
    CHECK(!transitions && !cleanup && gTasks[0].tState==0 && gTasks[0].isActive);
    battleAllowed=true;
    Task_BattleStart(0);
    CHECK(transitions==1 && !cleanup && gTasks[0].tState==1);
    Task_BattleStart(0);
    CHECK(cleanup==1 && !gTasks[0].isActive && gMain.callback2==CB2_InitBattle);
    gTasks[0].tState=0;gTasks[0].isActive=true;
    battleAllowed=false;cancelCallback=true;gMain.callback2=CB2_Overworld;
    Task_BattleStart(0);
    CHECK(!gTasks[0].isActive && transitions==1 && cleanup==1 && gMain.callback2==SavedCallback);
    puts("actual engine event/script and pre-transition arbitration tests passed");
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=UPSTREAM / 'src')
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    args = parser.parse_args()
    script = (args.source / 'script.c').read_text()
    approach = (args.source / 'trainer_see.c').read_text()
    battle = (args.source / 'battle_setup.c').read_text()
    bodies = function(approach, 'CancelPendingTrainerApproach')
    for name in ('ScriptContext_Init', 'ScriptContext_SetupScript', 'ScriptContext_RunScript',
                 'ScriptContext_Stop', 'ScriptContext_Enable'):
        bodies += function(script, name)
    bodies += function(battle, 'Task_BattleStart')
    with tempfile.TemporaryDirectory(prefix='coop-event-hooks-') as directory:
        directory = Path(directory)
        source = directory / 'hooks.c'
        exe = directory / ('hooks.exe' if os.name == 'nt' else 'hooks')
        source.write_text(HEADER + bodies + HARNESS)
        subprocess.run(shlex.split(args.cc) + ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror',
                       '-DPLATFORM_3DS', str(source), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
