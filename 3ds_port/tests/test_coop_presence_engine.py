"""Execute native peer cleanup, save repair, map departure and bounded animation ticks."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from test_coop_bike_hooks import function, UPSTREAM

HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;typedef bool bool8;
#define TRUE true
#define FALSE false
#define OBJECT_EVENTS_COUNT 16
#define MAX_SPRITES 64
#define LOCALID_COOP_PLAYER 252
struct ObjectEvent {bool active,isPlayer,heldMovementActive,heldMovementFinished,frozen;u8 localId,spriteId;} gObjectEvents[16];
struct Sprite {bool inUse;int data[8];} gSprites[64];
struct Warp {int mapGroup,mapNum;};
struct {struct ObjectEvent objectEvents[16];struct Warp location;} save,*gSaveBlock1Ptr=&save;
static struct Warp gLastUsedWarp,sWarpDestination,sFixedDiveWarp,sFixedHoleWarp,sDummyWarpData;
static unsigned destroys,ticks,remaining,starts,finishes,exits;
static bool catchUp;
static bool FollowerMon_IsFollower(const struct ObjectEvent *o){return o->localId==251;}
static void RemoveObjectEvent(struct ObjectEvent *o){++destroys;gSprites[o->spriteId].inUse=false;o->active=false;}
static bool CtrCoop_CatchUpMovement(u8 id){assert(id<16);return catchUp;}
static void CtrCoop_MapLeaving(void){assert(gSaveBlock1Ptr->location.mapNum==10);++exits;}
static bool ObjectEventIsHeldMovementActive(struct ObjectEvent *o){return o->heldMovementActive;}
static void ObjectEventExecHeldMovementAction(struct ObjectEvent *o,struct Sprite *s){(void)s;assert(!o->heldMovementFinished);++ticks;if(!--remaining)o->heldMovementFinished=true;}
static void DoGroundEffects_OnSpawn(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static void TryEnableObjectEventAnim(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static void DoGroundEffects_OnBeginStep(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;++starts;}
static void DoGroundEffects_OnFinishStep(struct ObjectEvent *o,struct Sprite *s){(void)s;if(o->heldMovementFinished)++finishes;}
static void UpdateObjectEventSpriteAnimPause(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static void UpdateObjectEventVisibility(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static void ObjectEventUpdateSubpriority(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static bool8 Idle(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;return false;}
'''

TEST = r'''
int main(void){
    gObjectEvents[0]=(struct ObjectEvent){.active=true,.isPlayer=true,.localId=255};
    gObjectEvents[1]=(struct ObjectEvent){.active=true,.localId=LOCALID_COOP_PLAYER,.spriteId=7};
    gObjectEvents[2]=gObjectEvents[1];gObjectEvents[2].spriteId=8;
    gObjectEvents[3]=(struct ObjectEvent){.active=true,.localId=4};
    gObjectEvents[4]=(struct ObjectEvent){.active=true,.localId=251};
    SaveObjectEvents();assert(gObjectEvents[1].active && gObjectEvents[2].active);
    assert(!save.objectEvents[1].active && !save.objectEvents[2].active && !save.objectEvents[4].active);
    assert(save.objectEvents[0].isPlayer && save.objectEvents[3].localId==4);
    save.objectEvents[1]=gObjectEvents[1];save.objectEvents[2]=gObjectEvents[2];
    LoadObjectEvents();assert(!gObjectEvents[1].active && !gObjectEvents[2].active);
    assert(gObjectEvents[0].isPlayer && gObjectEvents[3].localId==4);
    gObjectEvents[1]=(struct ObjectEvent){.active=true,.localId=LOCALID_COOP_PLAYER,.spriteId=7};
    gObjectEvents[2]=gObjectEvents[1];gObjectEvents[2].spriteId=8;
    gSprites[7].inUse=gSprites[8].inUse=true;gSprites[7].data[0]=1;gSprites[8].data[0]=9;
    CtrCoop_EngineRemoveRemoteObject(1);assert(destroys==1 && !gSprites[7].inUse && !gObjectEvents[1].active);
    CtrCoop_EngineRemoveRemoteObject(2);assert(destroys==1 && gSprites[8].inUse && !gObjectEvents[2].active);
    CtrCoop_EngineRemoveRemoteObject(0);CtrCoop_EngineRemoveRemoteObject(3);CtrCoop_EngineRemoveRemoteObject(255);
    assert(gObjectEvents[0].active && gObjectEvents[3].active && destroys==1);
    save.location.mapNum=10;sWarpDestination.mapNum=11;ApplyCurrentWarp();assert(exits==1 && save.location.mapNum==11);
    for(unsigned fast=0;fast<2;++fast){
        struct ObjectEvent *o=&gObjectEvents[1];o->heldMovementActive=true;o->heldMovementFinished=false;
        catchUp=fast;remaining=9;ticks=starts=finishes=0;
        while(remaining){unsigned before=ticks;UpdateObjectEventCurrentMovement(o,&gSprites[7],Idle);assert(ticks-before<=(fast?2:1));}
        assert(ticks==9 && starts==(fast?5:9) && finishes==1);
    }
    puts("native presence: save/load repair, exact-slot cleanup, reused sprite safety, exit before warp, bounded complete animation actions passed");
}
'''

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,default=UPSTREAM/'src')
    p.add_argument('--cc',default=os.environ.get('HOSTCC','gcc'))
    a=p.parse_args()
    movement=(a.source/'event_object_movement.c').read_text(encoding='utf-8')
    save=(a.source/'load_save.c').read_text(encoding='utf-8')
    world=(a.source/'overworld.c').read_text(encoding='utf-8')
    code=HEADER+function(save,'SaveObjectEvents')+function(save,'LoadObjectEvents')
    code+=function(movement,'CtrCoop_EngineRemoveRemoteObject')+function(world,'ApplyCurrentWarp')
    code+=function(movement,'UpdateObjectEventCurrentMovement')+TEST
    with tempfile.TemporaryDirectory(prefix='coop-presence-') as tmp:
        src=Path(tmp)/'test.c';exe=Path(tmp)/'test.exe';src.write_text(code,encoding='utf-8')
        subprocess.run(shlex.split(a.cc)+['-std=gnu99','-DPLATFORM_3DS','-O2','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)

if __name__=='__main__':main()
