"""Run real reset re-entry, VBlank, native callbacks/menu init and task scheduler.

Complements the real platform EndFrame regression: graphics/audio hardware
operations are probes here, while callback and scheduling bodies are original.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from test_coop_bike_hooks import function, ROOT, UPSTREAM
import test_coop_guest_reset as reset


EXTRA = r'''
typedef uint8_t bool8;typedef int16_t s16;typedef void (*MainCallback)(void);
#define TRUE 1
#define FALSE 0
#include "task.h"
struct Task gTasks[NUM_TASKS];
static bool8 sMainCallback2JustChanged;
static bool hostMissing,rpcReset,confirm;
static unsigned taskTicks,animations,oamBuilds,fades,oamLoads,plttLoads,callback1Ticks,continued;
static void (*vblankCallback)(void);
static u32 InitMainMenu(bool8);
static void CB2_MainMenu(void);
static void VBlankCB_MainMenu(void);
static void FinishReturnToMenu(void);
static void CtrCoop_CheckHostConnection(void);
static void SetVBlankCallback(void (*cb)(void)) {vblankCallback=cb;}
static void ContinueLocalSave(void) {++continued;}
static void Task_MainMenuCheckSaveFile(u8 taskId) {
 ++taskTicks;
 if(confirm) {SetMainCallback2(ContinueLocalSave);DestroyTask(taskId);}
}
static void AnimateSprites(void) {++animations;}
static void BuildOamBuffer(void) {++oamBuilds;}
static void UpdatePaletteFade(void) {++fades;}
static void LoadOam(void) {++oamLoads;}
static void ProcessSpriteCopyRequests(void) {}
static void TransferPlttBuffer(void) {++plttLoads;}
#define NOOP(...) ((void)0)
#define SetGpuReg NOOP
#define DmaFill16 NOOP
#define DmaFill32 NOOP
#define ResetPaletteFade NOOP
#define LoadPalette NOOP
#define ScanlineEffect_Stop NOOP
#define ResetSpriteData NOOP
#define FreeAllSpritePalettes NOOP
#define BeginNormalPaletteFade NOOP
#define ResetBgsAndClearDma3BusyFlags NOOP
#define InitBgsFromTemplates NOOP
#define ChangeBgX NOOP
#define ChangeBgY NOOP
#define InitWindows NOOP
#define DeactivateAllTextPrinters NOOP
#define LoadMainMenuWindowFrameTiles NOOP
#define EnableInterrupts NOOP
#define ShowBg NOOP
#define HideBg NOOP
#define PORT_PROF_BEGIN(name) ((void)0)
#define PORT_PROF_END(name,section) ((void)0)
static unsigned sFrames;
static unsigned REG_IME=1,REG_IE=1;
#define INTR_FLAG_VBLANK 1
#define INTR_INDEX_VBLANK 0
static void DispatchVBlank(void) {if(vblankCallback)vblankCallback();}
static void (*gIntrTable[1])(void)={DispatchVBlank};
static void CtrEmu_BeginVBlank(void) {}
static void CtrEmu_EndVBlank(void) {}
static void PortRfu_Frame(void) {}
static void UpdateStage(void) {}
static void CaptureLineScroll(void) {}
static void CaptureLineRegisters(void) {}
static void CheckAudioRate(void) {}
static void SampleAudioStats(void) {}
void TaskDummy(u8 id) {(void)id;}
static void InsertTask(u8);
static u8 FindFirstActiveTask(void);
'''

HARNESS = r'''
static void Callback1(void) {++callback1Ticks;}
static void CtrCoop_CheckHostConnection(void) {
 if(hostMissing) CtrGame_ReturnToMenu();
}
static void RpcBeforeEffect(void) {
 CtrCoop_CheckHostConnection();++oldEffects;
}
static void AgbMain(void) {
 ++entries;
 if(entries==1) {
  hostMissing=true;
  if(rpcReset) RpcBeforeEffect();else CtrGame_VBlank();
  ++oldEffects;assert(!"old callback stack resumed");
 }
 assert(entries==2 && afterStack==1 && bottomReset==1 && coopReset==1);
 assert(!oldEffects && !audioLocked && sResets==1);
 hostMissing=false;gMain.callback1=Callback1;
 gMain.callback2=NULL;gSaveBlock1Ptr=NULL;gSaveBlock2Ptr=NULL;
 CtrGame_VBlank();assert(sReturnToMenu && !sReturnMenuBootSeen);
 SetMainCallback2(CB2_InitCopyrightScreenAfterBootup);
 setupResult=1;CallCallbacks();CtrGame_VBlank();
 assert(sReturnMenuBootSeen && sReturnToMenu && !bootLoads);
 setupResult=0;CallCallbacks();assert(bootLoads==1 && gMain.callback2==Intro);
 CtrGame_VBlank();assert(gMain.callback2==CB2_InitMainMenu && !sReturnToMenu);
 /* The next native main-loop dispatch must initialize the menu, not leave
  * a callback pointer which the test mistakes for a responsive menu. */
 CallCallbacks();assert(gMain.callback2==CB2_MainMenu && vblankCallback==VBlankCB_MainMenu);
 assert(gTasks[0].isActive && gTasks[0].func==Task_MainMenuCheckSaveFile);
 for(unsigned i=0;i<4;++i) {CallCallbacks();CtrGame_VBlank();}
 assert(taskTicks==4 && animations==4 && oamBuilds==4 && fades==4);
 assert(oamLoads==4 && plttLoads==4 && callback1Ticks>=7);
 confirm=true;CallCallbacks();
 assert(gMain.callback2==ContinueLocalSave && !gTasks[0].isActive);
 assert(animations==4 && oamBuilds==4); /* CB2_MainMenu detects callback change. */
 CallCallbacks();assert(continued==1);
 longjmp(done,2);
}
static void Test(bool rpc) {
 rpcReset=rpc;confirm=false;entries=afterStack=bottomReset=coopReset=freed=oldEffects=emuReset=bootLoads=heapInits=0;
 taskTicks=animations=oamBuilds=fades=oamLoads=plttLoads=callback1Ticks=continued=0;
 sStarted=false;sResets=0;sReturnToMenu=sReturnMenuBootSeen=false;vblankCallback=NULL;
 gMain.callback1=gMain.callback2=NULL;memset(gTasks,0,sizeof(gTasks));
 memset(__ctr_game_data,1,16);memset(__ctr_game_bss,2,16);
 if(setjmp(done)==0) {CtrGame_Init();assert(!"game init returned");}
 assert(entries==2 && !oldEffects && continued==1);
 free(sGameAtStart);sGameAtStart=NULL;
}
int main(void) {
 Test(false);Test(true);
 puts("real reset/menu callbacks: VBlank and out-of-frame RPC unwind; native boot/menu/tasks and subsequent continue passed");
 return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    parser.add_argument('--check-mutations', action='store_true')
    args = parser.parse_args()
    game = (ROOT / '3ds_port/src/3ds_game_full.c').read_text(encoding='utf-8')
    main_src = (UPSTREAM / 'src/main.c').read_text(encoding='utf-8')
    menu = (UPSTREAM / 'src/main_menu.c').read_text(encoding='utf-8')
    tasks = (UPSTREAM / 'src/task.c').read_text(encoding='utf-8')
    boot = (UPSTREAM / 'src/intro.c').read_text(encoding='utf-8')
    header = reset.HEADER.replace('static struct { void (*callback2)(void); } gMain;',
                                 'static struct { void (*callback1)(void);void (*callback2)(void);unsigned state; } gMain;')
    header = header.replace('static void CB2_InitMainMenu(void) {}', 'void CB2_InitMainMenu(void);')
    header = header.replace('static void SetMainCallback2(void (*cb)(void)) {gMain.callback2=cb;}',
                            'void SetMainCallback2(void (*cb)(void));')
    header = header.replace('static bool guestLoss;', '')
    source = header + EXTRA
    source += ''.join(function(main_src, n) for n in ('CallCallbacks', 'SetMainCallback2'))
    source += ''.join(function(tasks, n) for n in ('ResetTasks', 'CreateTask', 'InsertTask', 'DestroyTask', 'RunTasks', 'FindFirstActiveTask'))
    source += ''.join(function(menu, n) for n in ('CB2_MainMenu', 'VBlankCB_MainMenu', 'CB2_InitMainMenu', 'InitMainMenu'))
    source += function(boot, 'CB2_InitCopyrightScreenAfterBootup')
    import re
    source += ''.join(re.search(r'^static size_t '+n+r'\(void\).*$', game, re.M)[0]+'\n'
                      for n in ('GameDataBytes', 'GameBssBytes'))
    source += ''.join(function(game, n) for n in ('KeepGameAtStart', 'WaitForResetKeysUp', 'CtrGame_SoftReset',
                                                'CtrGame_ReturnToMenu', 'ResetGameAfterStackUnwind',
                                                'FinishReturnToMenu', 'CtrGame_VBlank', 'CtrGame_Init'))
    source += HARNESS
    variants = {'current': source}
    if args.check_mutations:
        variants['no-menu-dispatch'] = source.replace('InitMainMenu(FALSE);', '(void)InitMainMenu;')
        variants['no-menu-task-progress'] = source.replace('    RunTasks();', '    (void)RunTasks;')
        variants['no-post-boot-menu'] = source.replace('SetMainCallback2(CB2_InitMainMenu);', 'SetMainCallback2(Intro);')
    with tempfile.TemporaryDirectory(prefix='reset-menu-', dir=os.environ.get('TMPDIR')) as temp:
        for name, body in variants.items():
            path = Path(temp)/(name+'.c');exe = path.with_suffix('.exe' if os.name=='nt' else '.out')
            path.write_text(body, encoding='utf-8')
            subprocess.run(shlex.split(args.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror',
                           '-Wno-unused-function','-I'+str(ROOT/'3ds_port/include'),
                           '-iquote'+str(UPSTREAM/'include'),str(path),'-o',str(exe)],check=True)
            result = subprocess.run([str(exe)],capture_output=True,text=True)
            if name=='current':
                assert result.returncode==0,result.stdout+result.stderr
                print(result.stdout.strip())
            else:
                assert result.returncode!=0,'undetected regression: '+name
                print('expected regression rejection:',name)


if __name__=='__main__':
    main()
