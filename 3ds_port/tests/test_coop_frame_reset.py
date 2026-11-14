"""Execute actual frame/reset bodies across a longjmp from the VBlank hook."""
import argparse
from pathlib import Path
import subprocess
from test_coop_bike_hooks import function

ROOT = Path(__file__).resolve().parents[2]

HEADER = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "3ds_platform.h"
typedef uint32_t u32;
#define SYSCLOCK_ARM11 1000000
#undef CTR_STUB
#define CTR_STUB(a,b) ((void)0)
static CtrPlatformHooks sHooks;
static bool sWaiting,sPacedFrame,sSoundTick;
static uint64_t sWorkStart,sLastFrame,sLastTick,sExitStart,sFrameMs,sFrames,sShownTick,sPaceStart;
static unsigned sEarned,sHidden;
static float sPaceFrames,sPresentMs;
static CtrTiming sTiming;
static uint64_t ticks=100000;
static unsigned audioFrames,vblanks,presents,nestedAttempts,oldEffects,stackResets,coops,audioLock;
static uint64_t svcGetSystemTick(void) { ticks+=1000;return ticks; }
uint64_t CtrPlatform_Milliseconds(void) {return ticks/1000;}
static void UpdateHomeAllowed(void) {}
static void PaceUpdate(void) {}
static bool FrameShown(uint64_t now) {(void)now;return true;}
static struct { unsigned frames;float waitMs; } video={17,0};
#define CtrVideo_GetStats() (&video)
void CtrProf_EndFrame(float work,float wait,uint64_t count) {(void)work;(void)wait;(void)count;}
static char __ctr_game_data[16],__ctr_game_bss[16],initial[32];
static char *sGameAtStart=initial;
static bool sStarted=true,sReturnToMenu,sReturnMenuBootSeen,sFadeHeld;
static u32 sResets,sFadeHeldFrames,sFadeLastVideoFrame;
static jmp_buf sAgbMainEntry;
static size_t GameDataBytes(void) {return sizeof(__ctr_game_data);}
static size_t GameBssBytes(void) {return sizeof(__ctr_game_bss);}
static void CtrCoop_AfterStackReset(void) {++stackResets;assert(!sWaiting);}
static void CtrBottom_SoftReset(void) {assert(stackResets==1);}
static void CtrCoop_SoftReset(void) {++coops;}
static void FollowerMon_ReleaseAll(void) {}
static void CtrAudio_LockSound(void) {assert(!audioLock);++audioLock;}
static void CtrAudio_UnlockSound(void) {assert(audioLock==1);--audioLock;}
static void CtrEmu_Reset(void) {assert(audioLock==1);}
static void StartCgbChannels(void) {}
void CtrPlatform_ReportMemory(const char *stage) {(void)stage;}
void CtrPlatform_SetBattleScene(bool on) {assert(!on);}
void CtrPlatform_Fatal(const char *reason) {(void)reason;abort();}
static void WaitForResetKeysUp(void) {}
'''

TEST = r'''
static void Audio(void) {++audioFrames;}
static void Present(void) {++presents;}
static void HealthyVblank(void) {
 ++vblanks;++nestedAttempts;
 CtrPlatform_EndFrame(); /* A genuinely nested frame must remain blocked. */
}
static void LostHost(void) {++vblanks;CtrGame_ReturnToMenu();++oldEffects;}
static void SoftReset(void) {++vblanks;CtrGame_SoftReset();++oldEffects;}
static void Scenario(unsigned mode) {
 audioFrames=vblanks=presents=nestedAttempts=oldEffects=stackResets=coops=audioLock=0;
 sFrames=0;sWaiting=false;sResets=0;sReturnToMenu=sReturnMenuBootSeen=false;
 sHooks.audioFrame=Audio;sHooks.videoPresent=Present;
 sHooks.vblank=mode==2?SoftReset:LostHost;
 memset(initial,7,sizeof(initial));memset(__ctr_game_data,8,16);memset(__ctr_game_bss,9,16);
 sEarned=5;sHidden=1;sPaceStart=99;sPaceFrames=40;sShownTick=88;
 sLastTick=sExitStart=55;sPacedFrame=true;sFadeHeld=true;
 if(setjmp(sAgbMainEntry)==0) {
  if(mode==1) CtrGame_ReturnToMenu(); /* Synchronous RPC outside EndFrame. */
  else CtrPlatform_EndFrame();
  ++oldEffects;assert(!"abandoned effect/frame resumed");
 }
 ResetGameAfterStackUnwind();
 assert(stackResets==1 && coops==1 && !audioLock && !oldEffects && !sWaiting);
 assert(!sEarned && !sHidden && !sPaceStart && !sPaceFrames && !sShownTick);
 assert(!sLastTick && !sExitStart && !sPacedFrame && sSoundTick);
 assert(sWorkStart && sLastFrame && !sFrameMs);
 assert(sResets==1 && !sFadeHeld && sFadeLastVideoFrame==17);
 assert((mode==2)==!sReturnToMenu);
 for(unsigned i=0;i<16;++i)assert(__ctr_game_data[i]==7 && __ctr_game_bss[i]==7);
 sHooks.vblank=HealthyVblank;
 for(unsigned frame=0;frame<4;++frame)CtrPlatform_EndFrame();
 assert(presents==4 && nestedAttempts==4 && sFrames==4 && !sWaiting);
 assert(vblanks==4+(mode!=1) && audioFrames==4+(mode!=1));
}
int main(void) {
 for(unsigned mode=0;mode<3;++mode)Scenario(mode);
 puts("actual EndFrame longjmp/reset: VBlank host loss, RPC, soft reset, four subsequent frames and nested guard passed");
 return 0;
}
'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc',default='gcc')
    parser.add_argument('--check-mutations',action='store_true')
    args=parser.parse_args()
    platform=(ROOT/'3ds_port/src/3ds_platform.c').read_text(encoding='utf-8')
    game=(ROOT/'3ds_port/src/3ds_game_full.c').read_text(encoding='utf-8')
    source=HEADER+''.join(function(platform,n) for n in ('CtrPlatform_AfterStackReset','CtrPlatform_EndFrame'))
    source+=''.join(function(game,n) for n in ('CtrGame_ReturnToMenu','CtrGame_SoftReset','ResetGameAfterStackUnwind'))+TEST
    variants={'current':source}
    if args.check_mutations:
        reset=function(platform,'CtrPlatform_AfterStackReset')
        variants['stuck-frame-guard']=source.replace(reset,reset.replace('sWaiting = false;',''))
        variants['missing-post-jump-call']=source.replace('CtrPlatform_AfterStackReset();','(void)CtrPlatform_AfterStackReset;')
        variants['old-pacing-debt']=source.replace(reset,reset.replace('sPaceStart = sShownTick = 0;',''))
    out=ROOT/'build/coop-fixes-20261114/frame-reset'
    out.mkdir(parents=True,exist_ok=True)
    for name,body in variants.items():
        path=out/(name+'.c');exe=path.with_suffix('.exe')
        path.write_text(body,encoding='utf-8')
        subprocess.run([args.cc,'-std=gnu99','-O2','-Wall','-Wextra','-Werror',
                        '-I'+str(ROOT/'3ds_port/include'),str(path),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],capture_output=True,text=True)
        if name=='current':
            assert result.returncode==0,result.stdout+result.stderr
            print(result.stdout.strip())
        else:
            assert result.returncode!=0,'undetected regression: '+name
            print('expected regression rejection:',name)

if __name__=='__main__':main()
