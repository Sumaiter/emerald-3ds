"""Native turn/run decisions: following has no turn pause or local run-toggle drift."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from test_coop_bike_hooks import function, UPSTREAM

HEADER=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
typedef uint8_t u8;typedef uint16_t u16;
#define DIR_NONE 0
#define NOT_MOVING 0
#define TURN_DIRECTION 1
#define MOVING 2
#define PLAYER_AVATAR_FLAG_SURFING 1
#define PLAYER_AVATAR_FLAG_UNDERWATER 2
#define PLAYER_AVATAR_FLAG_DASH 4
#define COLLISION_LEDGE_JUMP 1
#define COLLISION_OBJECT_EVENT 2
#define COLLISION_STOP_SURFING 3
#define COLLISION_PUSHED_BOULDER 4
#define COLLISION_ROTATING_GATE 5
#define B_BUTTON 2
#define FLAG_SYS_B_DASH 0
static struct {unsigned runningState,flags,objectEventId;} gPlayerAvatar;
static struct {unsigned currentMetatileBehavior;} gObjectEvents[1];
static bool following,running,always;static unsigned facing,walks,runs,collisions;
static bool CtrCoop_IsFollowing(void){return following;}
static bool CtrCoop_FollowRunning(void){return running;}
static bool CtrPlayer_RunAlways(void){return always;}
static unsigned GetPlayerMovementDirection(void){return facing;}
static unsigned CheckForPlayerAvatarCollision(unsigned d){(void)d;return collisions;}
static bool IsPlayerCollidingWithFarawayIslandMew(unsigned d){(void)d;return false;}
static void PlayerJumpLedge(unsigned d){(void)d;}
static void PlayerNotOnBikeCollideWithFarawayIslandMew(unsigned d){(void)d;}
static void PlayerNotOnBikeCollide(unsigned d){(void)d;}
static void PlayerWalkFast(unsigned d){(void)d;++runs;}
static void PlayerRun(unsigned d){(void)d;++runs;}
static void PlayerWalkNormal(unsigned d){(void)d;++walks;}
static bool FlagGet(unsigned f){(void)f;return true;}
static unsigned IsRunningDisallowed(unsigned b){(void)b;return 0;}
'''
TEST=r'''
int main(void){
    for(unsigned localToggle=0;localToggle<2;++localToggle)
    for(unsigned leaderRun=0;leaderRun<2;++leaderRun)
    for(unsigned d=1;d<=4;++d){
        following=true;always=localToggle;running=leaderRun;facing=d%4+1;
        gPlayerAvatar.runningState=NOT_MOVING;gPlayerAvatar.flags=0;walks=runs=0;
        assert(CheckMovementInputNotOnBike(d)==MOVING);
        PlayerNotOnBikeMoving(d,0);assert(runs==leaderRun && walks==!leaderRun);
        collisions=COLLISION_OBJECT_EVENT;PlayerNotOnBikeMoving(d,0);
        assert(runs==leaderRun && walks==!leaderRun);collisions=0;
    }
    following=false;facing=1;gPlayerAvatar.runningState=NOT_MOVING;
    assert(CheckMovementInputNotOnBike(2)==TURN_DIRECTION);
    assert(CheckMovementInputNotOnBike(0)==NOT_MOVING);
    puts("native following: turns without stationary pause, leader run speed independent of local toggle; collision and solo behavior preserved");return 0;
}
'''
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,default=UPSTREAM/'src');p.add_argument('--cc',default=os.environ.get('HOSTCC','gcc'));a=p.parse_args()
    s=(a.source/'field_player_avatar.c').read_text(encoding='utf-8')
    code=HEADER+function(s,'CheckMovementInputNotOnBike')+function(s,'PlayerNotOnBikeMoving')+TEST
    with tempfile.TemporaryDirectory(prefix='coop-follow-input-') as tmp:
        src=Path(tmp)/'test.c';exe=Path(tmp)/'test.exe';src.write_text(code,encoding='utf-8')
        subprocess.run(shlex.split(a.cc)+['-std=gnu99','-DPLATFORM_3DS','-O2','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
if __name__=='__main__':main()
