"""Run the actual native double-battle return and link-setup bodies."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from test_coop_bike_hooks import function, UPSTREAM

HEADER = r'''
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants/battle.h"
#define FALSE false
#define SE_LOW_HEALTH 1
static struct { bool active; } gPaletteFade;
static struct { bool inBattle; void (*callback1)(void),(*savedCallback)(void); } gMain;
static void (*gPreBattleCallback1)(void),(*gBattlerControllerFuncs[4])(void);
static unsigned gBattleTypeFlags,gBattleOutcome,gWirelessCommType,gActiveBattler;
static bool gReceivedRemoteLinkPlayers,cooperative,linkReady=true;
static unsigned restores,ends,closes,standbys,opens,tasks,buffers,wireless;
static void SetLinkBattleEndCallbacks(void) {}
static bool CtrCoop_IsDoubleBattle(void) { return cooperative; }
static void CtrCoop_BeginBattleEvolutions(void) { ++restores; }
static bool IsLinkTaskFinished(void) { return linkReady; }
static void SetCloseLinkCallback(void) { ++closes; }
static void SetLinkStandbyCallback(void) { ++standbys; }
static void m4aSongNumStop(unsigned song) { (void)song; }
static void SetMainCallback2(void (*cb)(void)) { (void)cb; }
static void SetWirelessCommType1(void) { ++wireless; }
static void OpenLink(void) { ++opens; }
static void Task_WaitForLinkPlayerConnection(unsigned char task) { (void)task; }
static void CreateTask(void (*cb)(unsigned char),unsigned priority) { (void)cb;(void)priority;++tasks; }
static void CreateTasksForSendRecvLinkBuffers(void) { ++buffers; }
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;} } while(0)
'''

TEST = r'''
int main(void) {
    unsigned outcomes[]={B_OUTCOME_WON,B_OUTCOME_LOST,B_OUTCOME_DREW,B_OUTCOME_RAN};
    gBattleTypeFlags=BATTLE_TYPE_LINK;cooperative=true;
    for(unsigned i=0;i<sizeof(outcomes)/sizeof(outcomes[0]);++i) {
        restores=ends=closes=standbys=0;gBattleOutcome=outcomes[i];
        SetBattleEndCallbacks();
        CHECK(restores==1 && ends==0); /* native callback finalizes after flags/party restore */
        CHECK(!closes && !standbys && !gBattlerControllerFuncs[0]);
    }
    restores=ends=0;gPaletteFade.active=true;SetBattleEndCallbacks();CHECK(!restores && !ends);
    gPaletteFade.active=false;linkReady=false;SetBattleEndCallbacks();CHECK(!restores && !ends);linkReady=true;
    cooperative=false;gWirelessCommType=0;SetBattleEndCallbacks();CHECK(closes==1 && !restores);
    gWirelessCommType=1;SetBattleEndCallbacks();CHECK(standbys==1 && !restores);
    cooperative=true;HandleLinkBattleSetup();CHECK(buffers==1 && !opens && !tasks && !wireless);
    cooperative=false;HandleLinkBattleSetup();CHECK(buffers==2 && opens==1 && tasks==1 && wireless==1);
    gBattleTypeFlags=0;HandleLinkBattleSetup();CHECK(buffers==2 && opens==1);
    puts("actual double return restores before native loss callbacks; native link setup remains independent");
    return 0;
}
'''

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--source',type=Path,default=UPSTREAM/'src')
    p.add_argument('--cc',default='gcc')
    a=p.parse_args()
    bodies=function((a.source/'battle_controller_player.c').read_text(encoding='utf-8'),'SetBattleEndCallbacks')
    bodies+=function((a.source/'battle_controllers.c').read_text(encoding='utf-8'),'HandleLinkBattleSetup')
    with tempfile.TemporaryDirectory(prefix='coop-double-',dir=os.environ.get('TMPDIR')) as d:
        d=Path(d);source=d/'test.c';exe=d/'test.exe'
        source.write_text(HEADER+bodies+TEST,encoding='utf-8')
        subprocess.run(shlex.split(a.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror',
            '-iquote',str(a.source.parent/'include'),str(source),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)

if __name__=='__main__':main()
