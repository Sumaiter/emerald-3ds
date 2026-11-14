"""Execute production longjmp/re-entry/reset/menu and journal teardown bodies."""
import argparse
import os
from pathlib import Path
import re
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
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "3ds_coop_inventory.h"
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#define CTR_SOFT_RESET_KEYS 15
#define CTR_LOG_ERROR 0
#define CTR_LOG_GAME 0
#define CTR_LOG_FS 0
#define CTR_LOG_AUDIO 0
#define CTR_STUB(a,b) ((void)0)
#define SAVE_NORMAL 0
#define SAVE_STATUS_EMPTY 0
#define SAVE_STATUS_CORRUPT 2
static char __ctr_game_data[16],__ctr_game_bss[16];
static char *__ctr_game_data_end=__ctr_game_data+16,*__ctr_game_bss_end=__ctr_game_bss+16;
static char *sGameAtStart;
static jmp_buf sAgbMainEntry,done;
static bool sStarted,sReturnToMenu,sReturnMenuBootSeen,sFadeHeld;
static u32 sResets,sFadeHeldFrames,sFadeLastVideoFrame;
static unsigned entries,afterStack,bottomReset,coopReset,freed,audioLocked,emuReset,bootLoads,heapInits,oldEffects;
static unsigned setupResult;
static bool guestLoss;
static struct { void (*callback2)(void); } gMain;
static struct { unsigned optionsSound; } save2,*gSaveBlock2Ptr;
static char *gSaveBlock1Ptr;
static unsigned gSaveFileStatus;
static char gHeap[32];
#define HEAP_SIZE sizeof(gHeap)
static struct CtrAssetStats { unsigned entries,indexBytes; } assets;
static struct { unsigned frames; } video={77};
typedef struct { void *vram;const uint16_t *palette,*oam,*regs; } CtrVideoMemory;
static uint8_t FLASH_BASE[128],VRAM_[8],PLTT[8],OAM[8],REG_BASE[8];
static void CtrLog_Write(int c,const char *fmt,...) {(void)c;(void)fmt;}
static void CtrPlatform_Fatal(const char *s) {(void)s;longjmp(done,3);}
static void CtrAssets_Fatal(const char *s) {CtrPlatform_Fatal(s);}
static void CtrPlatform_SleepUs(unsigned us) {(void)us;}
static bool CtrPlatform_BeginFrame(void) {return true;}
static void CtrPlatform_Shutdown(void) {}
static u16 Platform_GetKeyInput(void) {return 0;}
static void CtrPlatform_SetBattleScene(bool v) {assert(!v);}
static void CtrPlatform_ReportMemory(const char *s) {(void)s;}
static void CtrPlatform_AfterStackReset(void) {}
static void CtrCoop_AfterStackReset(void) {assert(entries==1 && !audioLocked);++afterStack;}
static void CtrBottom_SoftReset(void) {assert(afterStack==1);++bottomReset;}
static void CtrCoop_SoftReset(void) {assert(afterStack==1);++coopReset;}
static void FollowerMon_ReleaseAll(void) {++freed;}
static void CtrAudio_LockSound(void) {audioLocked=1;}
static void CtrAudio_UnlockSound(void) {assert(audioLocked);audioLocked=0;}
static const void *Unused(void) {return NULL;}
static void CtrEmu_Reset(void) {++emuReset;}
static void StartCgbChannels(void) {}
static bool CtrGameData_Init(void) {return true;}
static bool CtrScripts_Init(void) {return true;}
static bool CtrMaps_Init(void) {return true;}
static bool CtrSongs_Init(void) {return true;}
static bool CtrAssets_StartWarmup(void) {return true;}
static void Port_AssetPreload(void) {}
static void Port_PreloadLatinFonts(void) {}
static void CtrBottom_Init(void) {}
static void Port_SaveInit(void) {}
static const struct CtrAssetStats *CtrAssets_GetStats(void) {return &assets;}
static unsigned CtrMaps_Bytes(void) {return 0;}
static unsigned CtrScripts_Bytes(void) {return 0;}
static unsigned CtrSongs_Bytes(void) {return 0;}
static unsigned CtrGameData_Bytes(void) {return 0;}
static void CtrVideo_Bind(CtrVideoMemory v) {(void)v;}
#define CtrVideo_GetStats() (&video)
static void CB2_InitMainMenu(void) {}
static void Intro(void) {}
static void SetMainCallback2(void (*cb)(void)) {gMain.callback2=cb;}
static unsigned SetUpCopyrightScreen(void) {
 if(!setupResult) SetMainCallback2(Intro);
 return setupResult;
}
static unsigned GetSaveBlocksPointersBaseOffset(void) {return 0;}
static void SetSaveBlocksPointers(unsigned n) {(void)n;gSaveBlock1Ptr=gHeap;gSaveBlock2Ptr=&save2;}
static void ResetMenuAndMonGlobals(void) {}
static void Save_ResetSaveCounters(void) {}
static void LoadGameSave(unsigned s) {(void)s;assert(afterStack==1);++bootLoads;}
static void Sav2_ClearSetDefault(void) {}
static void SetPokemonCryStereo(unsigned n) {(void)n;}
static void InitHeap(void *p,unsigned n) {(void)p;(void)n;++heapInits;}
static void AgbMain(void);
'''

HARNESS = r'''
static void BlockedRpcEffect(void) {
 assert(!afterStack && entries==1);
 if(guestLoss) CtrGame_ReturnToMenu(); else CtrGame_SoftReset();
 ++oldEffects;
 assert(!"old item/money effect resumed");
}
static void AgbMain(void) {
 ++entries;
 if(entries==1) {
  memset(__ctr_game_data,37,16);memset(__ctr_game_bss,99,16);
  sFadeHeld=true;
  BlockedRpcEffect();
  assert(!"abandoned AgbMain returned");
 }
 assert(entries==2 && afterStack==1 && bottomReset==1 && coopReset==1 && freed==1);
 assert(!oldEffects && !audioLocked && !sFadeHeld && !sFadeHeldFrames && sResets==1);
 assert(sFadeLastVideoFrame==77 && emuReset==2);
 for(unsigned i=0;i<16;++i) {assert(__ctr_game_data[i]==1);assert(__ctr_game_bss[i]==2);}
 if(guestLoss) {
  /* No callback/save pointers yet: never jump straight into an old save. */
  gMain.callback2=NULL;gSaveBlock1Ptr=NULL;gSaveBlock2Ptr=NULL;
  FinishReturnToMenu();assert(sReturnToMenu && !sReturnMenuBootSeen);
  gMain.callback2=Intro;gSaveBlock1Ptr=gHeap;gSaveBlock2Ptr=&save2;
  FinishReturnToMenu();assert(gMain.callback2==Intro && sReturnToMenu && !bootLoads);
  gSaveBlock1Ptr=NULL;gSaveBlock2Ptr=NULL;
  gMain.callback2=CB2_InitCopyrightScreenAfterBootup;
  setupResult=1;CB2_InitCopyrightScreenAfterBootup();
  FinishReturnToMenu();assert(sReturnToMenu && !bootLoads);
  setupResult=0;CB2_InitCopyrightScreenAfterBootup();
  assert(bootLoads==1 && heapInits==1 && gMain.callback2==Intro);
  FinishReturnToMenu();assert(gMain.callback2==CB2_InitMainMenu && !sReturnToMenu);
  FinishReturnToMenu();assert(bootLoads==1);
 } else {assert(!sReturnToMenu && !bootLoads);}
 longjmp(done,2);
}
static void TestGameReset(bool guest) {
 guestLoss=guest;entries=afterStack=bottomReset=coopReset=freed=oldEffects=emuReset=bootLoads=heapInits=0;
 sStarted=false;sResets=0;sReturnToMenu=sReturnMenuBootSeen=false;
 memset(__ctr_game_data,1,16);memset(__ctr_game_bss,2,16);
 if(setjmp(done)==0) {CtrGame_Init();assert(!"game init returned");}
 assert(entries==2 && !oldEffects);
 free(sGameAtStart);sGameAtStart=NULL;
}
static CtrCoopInventory sInventory,sWallet;
static bool sConnected,sGuest,sSaveApplied,sReinstall,sKnownPeer,sResuming,sWantResume;
static uint32_t sClientNonce;
static CtrCoopInventory *bound;
void CtrCoopInventory_Bind(CtrCoopInventory *p) {bound=p;}
'''

TAIL = r'''
static void TestJournals(void) {
 for(unsigned host=0;host<2;++host) {
  memset(&sInventory,0,sizeof(sInventory));
  sInventory.enabled=true;sInventory.host=host;sInventory.pending=true;
  sInventory.reconciling=true;sInventory.useCount=1;sInventory.hostUseCount[1]=3;
  sInventory.cachedAcked=false;sInventory.quantity[0][2]=17;
  sConnected=sGuest=sSaveApplied=sReinstall=sKnownPeer=sResuming=sWantResume=true;
  sClientNonce=123;bound=&sInventory;
  assert(!CtrCoopInventory_Reset(&sInventory)); /* Ordinary Reset must refuse. */
  CtrCoopInventoryBridge_AfterStackReset();
  assert(!sInventory.enabled && !sInventory.pending && !sInventory.useCount && !bound);
  assert(!sClientNonce && !sConnected && !sKnownPeer && !sResuming);
  memset(&sWallet,0,sizeof(sWallet));
  sWallet.enabled=true;sWallet.host=host;sWallet.pending=true;sWallet.useCount=1;
  sWallet.hostUseCount[1]=3;sWallet.cachedAcked=false;sWallet.quantity[0][1]=37;
  sClientNonce=456;sConnected=sGuest=sSaveApplied=sReinstall=true;
  assert(!CtrCoopInventory_Reset(&sWallet));
  CtrCoopMoney_AfterStackReset();
  assert(!sWallet.enabled && !sWallet.pending && !sWallet.useCount && !sWallet.hostUseCount[1]);
  assert(!sClientNonce && !sConnected && !sGuest);
 }
}
int main(void) {
 (void)Unused;
 TestGameReset(true);TestGameReset(false);TestJournals();
 sStarted=false;sGameAtStart=NULL;
 if(setjmp(done)==0) {CtrGame_ReturnToMenu();assert(!"unsafe guest continuation");}
 assert(entries==2 && !oldEffects);
 puts("real reset bodies: RPC stack unwind, RAM/audio ordering, local-save boot/menu, host/guest journals passed");
 return 0;
}
'''

TRANSPORT_HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define CTR_LOG_GAME 0
#define LOCAL_SESSION_NONE 0
struct Node {
 bool present,heard;unsigned outCount,outHead;uint32_t heardMs;
 struct { uint32_t sentMs,firstMs; } out[1];
};
static struct { struct Node nodes[NODE_SLOTS];bool hosting;uint16_t myId,nodeMask;int lock; } sL;
static unsigned left,ended,ejected,nodeLeft;
static void LightLock_Lock(int *p) {(void)p;}
static void LightLock_Unlock(int *p) {(void)p;}
static void CtrLog_Write(int c,const char *s,...) {(void)c;(void)s;}
static void LeaveNetwork(bool v) {(void)v;++left;}
static void EndSession(unsigned state) {assert(state==LOCAL_SESSION_NONE);++ended;}
static void udsEjectClient(uint16_t id) {assert(id==1);++ejected;}
static void NodeLeft(uint16_t id) {assert(id==1);++nodeLeft;sL.nodes[id].present=false;}
'''

TRANSPORT_TEST = r'''
int main(void) {
 sL.myId=2;
 sL.nodes[1].present=sL.nodes[1].heard=true;
 sL.nodes[1].outCount=1;sL.nodes[1].heardMs=1;
 sL.nodes[1].out[0].sentMs=sL.nodes[1].out[0].firstMs=1;
 DropSilent(SILENT_MS);assert(!left && !ended);
 DropSilent(SILENT_MS+1);assert(left==1 && ended==1 && !ejected);
 /* The same worker timeout keeps the host alive and evicts only its guest. */
 sL.hosting=true;DropSilent(SILENT_MS+1);
 assert(left==1 && ended==1 && ejected==1 && nodeLeft==1);
 puts("real transport timeout: silent host ends guest session at 6000ms; host remains alive");
 return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    parser.add_argument('--check-mutations', action='store_true')
    args = parser.parse_args()
    game = (ROOT / '3ds_port/src/3ds_game_full.c').read_text(encoding='utf-8')
    inventory = (ROOT / '3ds_port/src/3ds_coop_inventory.c').read_text(encoding='utf-8')
    bridge = (ROOT / '3ds_port/src/3ds_coop_inventory_bridge.c').read_text(encoding='utf-8')
    money = (ROOT / '3ds_port/src/3ds_coop_money.c').read_text(encoding='utf-8')
    boot = (UPSTREAM / 'src/intro.c').read_text(encoding='utf-8')
    local = (ROOT / '3ds_port/src/3ds_local.c').read_text(encoding='utf-8')
    bodies = ''.join(re.search(r'^static size_t '+n+r'\(void\).*$', game, re.M)[0]+'\n'
                     for n in ('GameDataBytes', 'GameBssBytes'))
    bodies += ''.join(function(game, n) for n in (
        'KeepGameAtStart', 'WaitForResetKeysUp',
        'CtrGame_SoftReset', 'CtrGame_ReturnToMenu', 'ResetGameAfterStackUnwind',
        'FinishReturnToMenu', 'CtrGame_Init'))
    # The real VBlank always checks before emulation/engine callbacks, including
    # when PortRfu_Frame's LocalLink_Enabled early return would skip its pump.
    vblank = function(game, 'CtrGame_VBlank')
    assert vblank.index('CtrCoop_CheckHostConnection();') < vblank.index('CtrEmu_BeginVBlank();')
    source = HEADER + function(boot, 'CB2_InitCopyrightScreenAfterBootup') + bodies + HARNESS
    source += function(inventory, 'CtrCoopInventory_Reset')
    source += function(bridge, 'CtrCoopInventoryBridge_AfterStackReset')
    source += function(money, 'CtrCoopMoney_AfterStackReset') + TAIL
    variants = {'current': source}
    transport = '\n'.join(re.search(r'^#define '+n+r'\b.*$', local, re.M)[0]
                          for n in ('NODE_SLOTS', 'SILENT_MS'))+'\n'
    transport += TRANSPORT_HEADER + function(local, 'DropSilent') + TRANSPORT_TEST
    variants['transport-timeout'] = transport
    if args.check_mutations:
        for name, old, new in (
            ('effect-resumes', function(game, 'CtrGame_ReturnToMenu'),
             function(game, 'CtrGame_ReturnToMenu').replace('longjmp(sAgbMainEntry, 1);', 'return;')),
            ('no-stack-journal-cleanup', 'CtrCoop_AfterStackReset();', '(void)CtrCoop_AfterStackReset;'),
            ('host-journal-cross-world', 'void CtrCoopMoney_AfterStackReset(void)\n{',
             'void CtrCoopMoney_AfterStackReset(void)\n{\n if(sWallet.host) return;'),
            ('skip-local-save-boot', 'else if (sReturnMenuBootSeen && gMain.callback2 != NULL',
             'else if (gMain.callback2 != NULL'),
        ):
            assert old in source, name
            variants[name] = source.replace(old, new, 1)
    with tempfile.TemporaryDirectory(prefix='coop-guest-reset-', dir=os.environ.get('TMPDIR')) as temp:
        for name, body in variants.items():
            path = Path(temp) / (name + '.c')
            exe = path.with_suffix('.exe' if os.name == 'nt' else '.out')
            path.write_text(body, encoding='utf-8')
            subprocess.run(shlex.split(args.cc) + ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror',
                           '-I' + str(ROOT / '3ds_port/include'), str(path), '-o', str(exe)], check=True)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            if name in ('current', 'transport-timeout'):
                assert result.returncode == 0, result.stdout + result.stderr
                print(result.stdout.strip())
            else:
                assert result.returncode != 0, 'undetected regression: ' + name
                print('expected regression rejection:', name)


if __name__ == '__main__':
    main()
