"""Run actual HP conversion, field interaction, autosave and spectator-exit code."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from test_coop_bike_hooks import function, UPSTREAM

ROOT = Path(__file__).resolve().parents[2]
HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "3ds_input.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef bool bool8;
#define TRUE true
#define FALSE false
#define W 320
#define HIT_NONE 0
#define FOCUS_NONE 0
#define INSIDE_NONE 0
#define A_BUTTON 1
#define B_BUTTON 2
#define DPAD_RIGHT 16
#define LOCALID_NONE 0
#define GAME_STAT_STEPS 0
#define SE_WIN_OPEN 0
#define SAVE_NORMAL 0
#define SAVE_OVERWRITE_DIFFERENT_FILE 4
#define SAVE_STATUS_OK 1
#define MAIL_NONE 255
#define MON_DATA_STATUS 0
#define MON_DATA_HP 1
#define MON_DATA_MAX_HP 2
#define MON_DATA_MAIL 3
#define CTR_SOFT_RESET_KEYS (1|2|4|8)
struct BoxPokemon { u16 species;u8 pp[4],rest[74]; };
struct Pokemon { struct BoxPokemon box;u32 status;u16 hp,maxHP;u8 mail; };
static struct BoxPokemon guestBox,ordinaryBox;
static unsigned statCalculations,restoreCalls;
static u16 restoredHp;
static void SetMonData(struct Pokemon *p,int field,const void *ptr) {
    u32 value=*(const u32 *)ptr;
    switch(field){case MON_DATA_STATUS:p->status=value;break;case MON_DATA_HP:p->hp=value;break;
        case MON_DATA_MAX_HP:p->maxHP=value;break;case MON_DATA_MAIL:p->mail=value;break;default:assert(0);}
}
static void CalculateMonStats(struct Pokemon *p) { ++statCalculations;p->maxHP=100;p->hp=100; }
static void CtrCoop_RestoreBoxVitals(const struct BoxPokemon *source,struct Pokemon *dest) {
    ++restoreCalls;assert(dest->maxHP==100 && dest->hp==100);
    if(source==&guestBox){dest->hp=restoredHp;dest->status=16;dest->mail=4;}
}
struct FieldInput {
    bool pressedAButton,pressedBButton,tookStep,checkStandardWildEncounter,heldDirection,heldDirection2;
    bool pressedStartButton,pressedSelectButton;u8 dpadDirection;
};
struct MapPosition { s16 x,y;u8 elevation; };
static struct { void (*callback2)(void);bool inBattle; } gMain;
static struct { bool active; } gPaletteFade;
static bool gDifferentSaveFile,gSoftResetDisabled;
static u16 gSpecialVar_LastTalked;
static u8 gSelectedObjectEvent;
static bool watching,controls,script,grantWatch,normalInteraction;
static unsigned steps,locks,hidden,updates,interactions,watchRequests,trainerChecks,mapViews,saveCalls;
static u8 saveResult=SAVE_STATUS_OK,lastSaveType;
static void CB2_Overworld(void) {}
static void OtherCallback(void) {}
static bool CtrCoop_IsWatching(void) { return watching; }
static bool CtrCoop_IsFollowing(void) { return false; }
static bool CtrCoop_TryFollowAt(s16 x,s16 y) { (void)x;(void)y;return false; }
static void CtrCoop_FollowKeys(u16 *pressed,u16 *held) { (void)pressed;(void)held; }
static bool ArePlayerFieldControlsLocked(void) { return controls; }
static bool ScriptContext_IsEnabled(void) { return script; }
static void LockPlayerFieldControls(void) { ++locks;controls=true; }
static void HideMapNamePopUpWindow(void) { ++hidden; }
static void UpdatePlayerAvatarTransitionState(void) { ++updates; }
static void FieldClearPlayerInput(struct FieldInput *input) { memset(input,0,sizeof(*input)); }
static void FieldGetPlayerInput(struct FieldInput *input,u16 down,u16 held) {
    input->pressedAButton=(down&A_BUTTON)!=0;input->pressedBButton=(down&B_BUTTON)!=0;
    input->dpadDirection=(held&DPAD_RIGHT)?4:0;
}
static u8 GetPlayerFacingDirection(void) { return 1; }
static void GetPlayerPosition(struct MapPosition *p) { p->x=10;p->y=12; }
static void GetInFrontOfPlayerPosition(struct MapPosition *p) { p->x=11;p->y=12; }
static u16 MapGridGetMetatileBehaviorAt(int x,int y) { (void)x;(void)y;return 0; }
static bool CheckForTrainersWantingBattle(void) { ++trainerChecks;return false; }
static bool TryRunOnFrameMapScript(void) { return false; }
static bool TrySetupDiveEmergeScript(void) { return false; }
static void IncrementGameStat(int stat) { (void)stat; }
static void IncrementBirthIslandRockStepCount(void) {}
static bool TryStartStepBasedScript(struct MapPosition *p,u16 a,u8 b) { (void)p;(void)a;(void)b;return false; }
static bool CheckStandardWildEncounter(u16 a) { (void)a;return false; }
static bool TryArrowWarp(struct MapPosition *p,u16 a,u8 b) { (void)p;(void)a;(void)b;return false; }
static bool TryDoorWarp(struct MapPosition *p,u16 a,u8 b) { (void)p;(void)a;(void)b;return false; }
static bool CtrCoop_TryWatchAt(s16 x,s16 y) { ++watchRequests;assert(x==11 && y==12);watching=grantWatch;return watching; }
static bool TryStartInteractionScript(struct MapPosition *p,u16 a,u8 b) {
    (void)p;(void)a;(void)b;++interactions;return normalInteraction;
}
static bool TrySetupDiveDownScript(void) { return false; }
static bool UseRegisteredKeyItemOnField(void) { return false; }
static void PlaySE(int song) { (void)song; }
static void ShowStartMenu(void) {}
static void PlayerStep(u8 direction,u16 down,u16 held) { (void)direction;(void)down;(void)held;++steps; }
static void SaveMapView(void) { ++mapViews; }
static u8 TrySavingData(u8 mode) { ++saveCalls;lastSaveType=mode;return saveResult; }
static CtrInput input;
static const CtrInput *CtrInput_Get(void) { return &input; }
static void CtrCoopSpectator_Stop(void) { watching=false; }
static const u8 *CtrCoop_NoticeName(void) { return NULL; }
static u16 sSpectatorExitMask;
static bool sSwallow,sMapFly;
static unsigned sFocus,sInside;
static struct { bool active,bag; } sTouch;
static struct { bool watching; } sShown;
'''
HARNESS = r'''
int main(void) {
    struct Pokemon converted;
    guestBox.species=211;guestBox.pp[0]=0;guestBox.pp[1]=7;guestBox.pp[2]=1;guestBox.pp[3]=19;
    memset(&converted,0xcc,sizeof(converted));restoredHp=37;
    BoxMonToMon(&guestBox,&converted);
    assert(statCalculations==1 && restoreCalls==1 && converted.hp==37 && converted.maxHP==100);
    assert(converted.status==16 && converted.mail==4 && !memcmp(&converted.box,&guestBox,sizeof(guestBox)));
    restoredHp=0;BoxMonToMon(&guestBox,&converted);assert(converted.hp==0 && converted.status==16);
    ordinaryBox=guestBox;BoxMonToMon(&ordinaryBox,&converted);
    assert(converted.hp==100 && converted.status==0 && converted.mail==MAIL_NONE);

    gMain.callback2=CB2_Overworld;grantWatch=true;
    DoCB1_Overworld(A_BUTTON,DPAD_RIGHT);
    assert(watching && watchRequests==1 && !interactions && !locks && !hidden && !steps && !controls);
    unsigned oldUpdates=updates,oldTrainers=trainerChecks;
    DoCB1_Overworld(A_BUTTON,DPAD_RIGHT);
    assert(updates==oldUpdates && trainerChecks==oldTrainers && !steps);

    /* Real UI watch branch: the exit press is consumed, a held direction
     * remains available immediately, and later B presses are not swallowed. */
    input.down=B_BUTTON;input.held=B_BUTTON|DPAD_RIGHT;
    ProcessKeys(0);assert(!watching && !sSwallow);
    assert(CtrBottom_FilterKeys(B_BUTTON|DPAD_RIGHT)==DPAD_RIGHT);
    DoCB1_Overworld(0,CtrBottom_FilterKeys(DPAD_RIGHT));assert(steps==1);
    assert(CtrBottom_FilterKeys(0)==0 && CtrBottom_FilterKeys(B_BUTTON)==B_BUTTON);
    watching=true;sShown.watching=true;input.touchDown=true;input.touchX=100;input.touchY=180;
    sTouch.active=sTouch.bag=true;ProcessTouch(0);
    assert(!watching && !sSwallow && !sTouch.active && !sTouch.bag);
    assert(CtrBottom_FilterKeys(DPAD_RIGHT)==DPAD_RIGHT);
    watching=true;input.touchX=35;ProcessTouch(0);assert(watching);
    input.touchX=100;input.touchY=210;ProcessTouch(0);assert(watching);
    input.touchY=156;ProcessTouch(0);assert(!watching);

    grantWatch=false;normalInteraction=true;input.down=input.held=0;
    DoCB1_Overworld(A_BUTTON,0);assert(interactions==1 && controls && locks==1 && hidden==1);
    controls=false;normalInteraction=false;DoCB1_Overworld(0,DPAD_RIGHT);assert(steps==2);

    for(unsigned guard=0;guard<6;guard++){
        gMain.callback2=guard==0?OtherCallback:CB2_Overworld;
        gMain.inBattle=guard==1;gPaletteFade.active=guard==2;controls=guard==3;script=guard==4;watching=guard==5;
        assert(!CtrCoop_EngineAutoSave() && !saveCalls && !mapViews);
    }
    gMain.callback2=CB2_Overworld;gMain.inBattle=gPaletteFade.active=controls=script=watching=false;
    gDifferentSaveFile=true;saveResult=0;
    assert(!CtrCoop_EngineAutoSave() && saveCalls==1 && mapViews==1 && gDifferentSaveFile);
    assert(lastSaveType==SAVE_OVERWRITE_DIFFERENT_FILE);
    saveResult=SAVE_STATUS_OK;assert(CtrCoop_EngineAutoSave() && !gDifferentSaveFile && saveCalls==2 && mapViews==2);
    assert(lastSaveType==SAVE_OVERWRITE_DIFFERENT_FILE);
    assert(CtrCoop_EngineAutoSave() && lastSaveType==SAVE_NORMAL);
    puts("actual BoxMonToMon/A-interaction/DoCB1/autosave and UI B/touch-exit integration passed");
    return 0;
}
'''


def ui_watch_branch(text, name):
    """Keep the exact native declarations/watch branch; stub unrelated UI paths."""
    body = function(text, name)
    end = body.index('\n    if (CtrCoop_NoticeName() != NULL)')
    suffix = '\n    (void)mode;\n'
    if name == 'ProcessKeys':
        suffix += '    (void)x;\n'
    else:
        suffix += '    return HIT_NONE;\n'
    return body[:end] + suffix + '}\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=UPSTREAM / 'src')
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    parser.add_argument('--check-mutations', action='store_true',
                        help='also prove that early healing, missing field gating and sticky exit input are rejected')
    args = parser.parse_args()
    pokemon = (args.source / 'pokemon.c').read_text(encoding='utf-8')
    overworld = (args.source / 'overworld.c').read_text(encoding='utf-8')
    field = (args.source / 'field_control_avatar.c').read_text(encoding='utf-8')
    ui = (ROOT / '3ds_port/src/3ds_bottom_ui.c').read_text(encoding='utf-8')
    bodies = function(pokemon, 'BoxMonToMon') + function(field, 'ProcessPlayerFieldInput')
    bodies += function(overworld, 'DoCB1_Overworld') + function(overworld, 'CtrCoop_EngineAutoSave')
    bodies += ui_watch_branch(ui, 'ProcessKeys') + ui_watch_branch(ui, 'ProcessTouch')
    bodies += function(ui, 'CtrBottom_FilterKeys')
    # Input's public declaration is external; preserve its linkage in the mock.
    source = (HEADER + bodies + HARNESS).replace(
        'static const CtrInput *CtrInput_Get(void)', 'const CtrInput *CtrInput_Get(void)')
    with tempfile.TemporaryDirectory(prefix='coop-live-hooks-', dir=os.environ.get('TMPDIR')) as folder:
        folder = Path(folder)
        variants = {'current': source}
        if args.check_mutations:
            replacements = {
                'healed-guest': ('CtrCoop_RestoreBoxVitals(src, dest);', '(void)CtrCoop_RestoreBoxVitals;'),
                'sticky-B': ('sSpectatorExitMask = B_BUTTON;', 'sSwallow = TRUE;'),
                'sticky-touch': ('sSpectatorExitMask = 0;', 'sSwallow = TRUE;'),
                'save-while-watching': ('|| CtrCoop_IsWatching())', ')'),
                'step-after-watch-request': (
                    'handled = ProcessPlayerFieldInput(&inputStruct);\n#ifdef PLATFORM_3DS\n'
                    '        if (CtrCoop_IsWatching())\n            return;\n#endif',
                    'handled = ProcessPlayerFieldInput(&inputStruct);'),
            }
            for name, (old, new) in replacements.items():
                assert old in source, name
                variants[name] = source.replace(old, new, 1)
        for name, text in variants.items():
            file = folder / (name + '.c')
            file.write_text(text)
            exe = folder / (name + ('.exe' if os.name == 'nt' else ''))
            subprocess.run(shlex.split(args.cc) + ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror',
                           '-DPLATFORM_3DS', '-I' + str(ROOT / '3ds_port/include'), str(file), '-o', str(exe)], check=True)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            if name == 'current':
                assert result.returncode == 0, result.stdout + result.stderr
                print(result.stdout.strip())
            else:
                assert result.returncode != 0, 'Regression was not detected: ' + name
                print('expected regression rejection:', name)


if __name__ == '__main__':
    main()
