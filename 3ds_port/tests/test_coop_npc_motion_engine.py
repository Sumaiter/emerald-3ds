"""Execute staged motion helpers and the native Oldale guide walk kernel."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
from test_coop_bike_hooks import function, UPSTREAM

ROOT = Path(__file__).resolve().parents[2]
HEADER = r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "3ds_coop_npc_motion.h"
#include "constants/event_object_movement.h"
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int16_t s16;typedef int8_t s8;typedef bool bool8;
#define TRUE true
#define FALSE false
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define OBJECT_EVENTS_COUNT 16
#define MAX_SPRITES 64
#define NUM_OBJ_EVENT_GFX 239
#define LOCALID_COOP_PLAYER 126
#define MAP_OFFSET 7
#define DIR_SOUTH 1
#define DIR_NORTH 2
#define DIR_WEST 3
#define DIR_EAST 4
#define sObjEventId data[0]
#define sTypeFuncId data[1]
#define sActionFuncId data[2]
#define sDirection data[3]
#define sSpeed data[4]
#define sTimer data[5]
enum { MOVE_SPEED_NORMAL,MOVE_SPEED_FAST_1,MOVE_SPEED_FAST_2,MOVE_SPEED_FASTER,MOVE_SPEED_FASTEST };
struct Coords16 {s16 x,y;};
struct ObjectEvent {
 bool active,isPlayer,frozen,heldMovementActive,heldMovementFinished,singleMovementActive,invisible,inanimate,
      facingDirectionLocked,spriteAnimPausedBackup,spriteAffineAnimPausedBackup,triggerGroundEffectsOnMove,triggerGroundEffectsOnStop;
 u8 localId,mapGroup,mapNum,spriteId,graphicsId,facingDirection,previousMovementDirection,movementDirection,movementActionId;
 struct Coords16 currentCoords,previousCoords;
} gObjectEvents[16];
struct Sprite { s16 x,y,data[8];bool inUse,animPaused,affineAnimPaused;u8 animNum,animCmdIndex; } gSprites[64];
struct {struct{u8 mapGroup,mapNum;}location;} save,*gSaveBlock1Ptr=&save;
struct {u32 width,height;} layout={30,30};struct {__typeof__(layout)*mapLayout;} gMapHeader={&layout};
static bool sourceContext,sourceFinished;static unsigned starts,sourceStarts,framesChecked,teleports;
static const u8 *movementPtr;
#define CHECK(c) do {if(!(c)){printf("native NPC failure line %d: %s\n",__LINE__,#c);exit(1);}}while(0)
static void CtrCoopBike_MovementStarted(struct ObjectEvent *o,u8 action){(void)o;(void)action;}
static void CtrCoopNpcMotion_StartedProbe(u8 group,u8 num,u8 id){CHECK(group==0&&num==10&&id==2);++starts;if(sourceContext)++sourceStarts;}
#define CtrCoopNpcMotion_Started CtrCoopNpcMotion_StartedProbe
static bool TryGetObjectEventIdByLocalIdAndMap(u8 id,u8 num,u8 group,u8 *index)
{*index=sourceContext?1:2;return id!=gObjectEvents[*index].localId||num!=10||group!=0;}
static void ObjectEventSetGraphicsId(struct ObjectEvent *o,u8 id){o->graphicsId=id;}
static void ObjectEventTurn(struct ObjectEvent *o,u8 d){o->facingDirection=d;}
static void SetMovementScriptFinished(u8 task,u8 index){(void)task;(void)index;sourceFinished=true;}
static void SetMovementScript(u8 index,const u8 *p){(void)index;movementPtr=p;}
static bool FreezeObjectEvent(struct ObjectEvent *o){o->frozen=true;return false;}
static void *sLockedAnimObjectEvents;
static u8 FindLockedObjectEventIndex(struct ObjectEvent *o){(void)o;return OBJECT_EVENTS_COUNT;}
static void SetStepAnimHandleAlternation(struct ObjectEvent *o,struct Sprite *s,u8 n){(void)o;s->animNum=n;}
static u8 GetMoveDirectionAnimNum(u8 n){return n;}
static u8 GetMoveDirectionFasterAnimNum(u8 n){return n;}
static u8 (*const sDirectionAnimFuncsBySpeed[5])(u8)={GetMoveDirectionAnimNum,GetMoveDirectionAnimNum,GetMoveDirectionAnimNum,GetMoveDirectionAnimNum,GetMoveDirectionAnimNum};
#define ACTION(name,base) static u8 name(u32 dir){return base+dir-1;}
ACTION(GetWalkNormalMovementAction,8) ACTION(GetWalkSlowMovementAction,4) ACTION(GetWalkFastMovementAction,0x15)
ACTION(GetJump2MovementAction,0xC) ACTION(GetJumpMovementAction,0x42) ACTION(GetRideWaterCurrentMovementAction,0x29)
ACTION(GetWalkFasterMovementAction,0x2D) ACTION(GetSlideMovementAction,0x31) ACTION(GetPlayerRunMovementAction,0x35)
ACTION(GetAcroWheelieHopDirectionMovementAction,0x74) ACTION(GetAcroWheelieJumpDirectionMovementAction,0x78)
ACTION(GetAcroPopWheelieMoveDirectionMovementAction,0x80) ACTION(GetAcroWheelieMoveDirectionMovementAction,0x84)
ACTION(GetAcroEndWheelieMoveDirectionMovementAction,0x88)
typedef bool8 (*ActionFunc)(struct ObjectEvent*,struct Sprite*);
static const ActionFunc *gMovementActionFuncs[0x9E];
'''

HARNESS = r'''
static void Init(void)
{
 memset(gObjectEvents,0,sizeof(gObjectEvents));memset(gSprites,0,sizeof(gSprites));save.location.mapNum=10;save.location.mapGroup=0;
 sourceFinished=false;sourceStarts=0;
 for(unsigned i=1;i<=2;++i){gObjectEvents[i]=(struct ObjectEvent){.active=true,.localId=2,.mapNum=10,.spriteId=i,.graphicsId=1,.facingDirection=1,.currentCoords={20,21}};
  gSprites[i]=(struct Sprite){.x=320,.y=336,.inUse=true,.data={i}};}
}
static void Render(u8 id)
{if(gObjectEvents[id].heldMovementActive)ObjectEventExecHeldMovementAction(&gObjectEvents[id],&gSprites[id]);}
static void RunGuide(const u8 *path)
{
 Init();movementPtr=path;CtrCoopNpcMotionPlayback state={0};CtrCoopNpcMotionPose latest={0};bool settled=false;
 for(unsigned frame=0;frame<700&&!settled;++frame)
 {
  sourceContext=true;if(!sourceFinished)ScriptMovement_TakeStep(0,0,1,movementPtr);Render(1);
  if(frame%4==0||sourceFinished){CHECK(CtrCoop_EngineNpcMotionCapture(0,10,2,&latest));latest.actionSerial=sourceStarts;}
  sourceContext=false;s16 x=gSprites[2].x,y=gSprites[2].y;
  int result=CtrCoop_EngineNpcMotionApply(&latest,&state,sourceFinished);CHECK(result>=0);Render(2);
  CHECK(abs(gSprites[2].x-x)<=1&&abs(gSprites[2].y-y)<=1);++framesChecked;
  settled=sourceFinished&&result==1;
 }
 CHECK(settled&&gObjectEvents[2].currentCoords.x==20&&gObjectEvents[2].currentCoords.y==14);
 CHECK(gSprites[2].x==320&&gSprites[2].y==224&&gObjectEvents[2].facingDirection==1&&!gObjectEvents[2].frozen);
 CHECK(!gObjectEvents[2].heldMovementActive);
}
int main(void)
{
 static const ActionFunc down[]={MovementAction_WalkNormalDown_Step0,MovementAction_WalkNormalDown_Step1,MovementAction_PauseSpriteAnim};
 static const ActionFunc up[]={MovementAction_WalkNormalUp_Step0,MovementAction_WalkNormalUp_Step1,MovementAction_PauseSpriteAnim};
 static const ActionFunc left[]={MovementAction_WalkNormalLeft_Step0,MovementAction_WalkNormalLeft_Step1,MovementAction_PauseSpriteAnim};
 static const ActionFunc right[]={MovementAction_WalkNormalRight_Step0,MovementAction_WalkNormalRight_Step1,MovementAction_PauseSpriteAnim};
 static const ActionFunc place[]={MovementAction_WalkInPlaceFasterDown_Step0,MovementAction_WalkInPlace_Step1,MovementAction_PauseSpriteAnim};
 gMovementActionFuncs[8]=down;gMovementActionFuncs[9]=up;gMovementActionFuncs[10]=left;gMovementActionFuncs[11]=right;gMovementActionFuncs[0x25]=place;
 RunGuide(guideSouth);RunGuide(guideNorth);RunGuide(guideEast);
 Init();sourceContext=false;CtrCoopNpcMotionPlayback state={0};
 CtrCoopNpcMotionPose p={.group=0,.num=10,.id=2,.x=20,.y=14,.facing=1,.graphics=1,.action=0xFF};
 /* Recovery after coalescing uses seven real walking steps, never relocation. */
 int result=0;for(unsigned frame=0;frame<200&&!result;++frame){s16 y=gSprites[2].y;result=CtrCoop_EngineNpcMotionApply(&p,&state,true);Render(2);CHECK(abs(gSprites[2].y-y)<=1);++framesChecked;}
 CHECK(result==1&&gSprites[2].y==224&&!teleports);
 Init();sourceContext=false;state=(CtrCoopNpcMotionPlayback){0};p.y=20;
 gObjectEvents[2].singleMovementActive=true;gObjectEvents[2].movementActionId=8;
 for(unsigned frame=0;frame<100;++frame){s16 y=gSprites[2].y;result=CtrCoop_EngineNpcMotionApply(&p,&state,true);Render(2);CHECK(abs(gSprites[2].y-y)<=1);if(result==1)break;}
 CHECK(result==1&&!gObjectEvents[2].singleMovementActive&&!gObjectEvents[2].heldMovementActive&&!gObjectEvents[2].frozen);
 Init();sourceContext=false;state=(CtrCoopNpcMotionPlayback){0};p.y=21;p.action=0x25;p.flags=1;p.actionSerial=55;
 unsigned before=starts;
 for(unsigned i=0;i<20;++i){CtrCoop_EngineNpcMotionApply(&p,&state,false);Render(2);}
 CHECK(starts==before+1); /* repeated snapshots do not restart an action */
 p.action=0xFE;CHECK(CtrCoop_EngineNpcMotionApply(&p,&state,true)==-1);
 p.action=0xFF;p.y=999;CHECK(CtrCoop_EngineNpcMotionApply(&p,&state,true)==-1);
 p.y=21;gObjectEvents[2].spriteId=3;gSprites[3].inUse=true;gSprites[3].sObjEventId=2;
 CHECK(CtrCoop_EngineNpcMotionApply(&p,&state,true)==-1);
 gObjectEvents[2].localId=254;CHECK(!CtrCoop_EngineNpcMotionCapture(0,10,254,&p));
 printf("real Oldale guide/helper/native pixel-step bodies: %u frame checks passed\n",framesChecked);return 0;
}
'''

def array(source, declaration):
    start = source.index(declaration)
    return source[start:source.index('};', start)+2]+'\n'

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--source',type=Path,default=UPSTREAM/'src')
    parser.add_argument('--cc',default=os.environ.get('HOSTCC','gcc'))
    args=parser.parse_args();root=args.source.parent
    source=(args.source/'event_object_movement.c').read_text(encoding='utf-8')
    movement=(args.source/'script_movement.c').read_text(encoding='utf-8')
    route=(root/'data/maps/OldaleTown/scripts.inc').read_text(encoding='utf-8')
    mapping={'walk_up':9,'walk_left':10,'walk_right':11,'walk_in_place_faster_down':0x25,'step_end':0xFE}
    paths=[]
    for direction in ('South','North','East'):
        label='OldaleTown_Movement_Employee'+direction
        body=route.split(label+':',1)[1].split('\n\n',1)[0]
        steps=[line.strip() for line in body.splitlines() if line.strip()]
        paths.append('static const u8 guide'+direction+'[]={'+','.join(str(mapping[s]) for s in steps)+'};\n')
    guide=route.split('OldaleTown_EventScript_MartEmployee::',1)[1].split('OldaleTown_EventScript_ExplainPotion::',1)[0]
    assert guide.count('giveitem ITEM_POTION')==1
    assert 'setobjectxyperm' not in guide
    assert 'setflag FLAG_TEMP_1' in guide and guide.count('applymovement LOCALID_OLDALE_MART_EMPLOYEE')==3
    bodies=[array(source,'static const struct Coords16 sDirectionToVectors[]')]
    funcs=['SetObjectEventDirection','MoveCoords','ShiftObjectEventCoords','ShiftStillObjectEventCoords',
           'UnfreezeObjectEvent','ObjectEventIsMovementOverridden','ObjectEventIsHeldMovementActive','ObjectEventSetHeldMovement',
           'ObjectEventClearHeldMovement','ObjectEventClearHeldMovementIfActive','ObjectEventCheckHeldMovementStatus','ObjectEventClearHeldMovementIfFinished',
           'ObjectEventExecHeldMovementAction','ObjectEventExecSingleMovementAction',
           'Step1','Step2','Step3','Step4','Step8','SetSpriteDataForNormalStep']
    bodies.extend(function(source,f) for f in funcs)
    begin=source.index('typedef void (*SpriteStepFunc)')
    finish=source.index('#undef sSpeed',begin)
    bodies.append(source[begin:finish])
    funcs=['InitNpcForMovement','InitMovementNormal','UpdateMovementNormal','InitMoveInPlace',
           'MovementAction_WalkInPlace_Step1','MovementAction_WalkInPlaceFasterDown_Step0','MovementAction_PauseSpriteAnim']
    for direction in ('Down','Up','Left','Right'):
        funcs.extend(['MovementAction_WalkNormal'+direction+'_Step0','MovementAction_WalkNormal'+direction+'_Step1'])
    bodies.extend(function(source,f) for f in funcs)
    bodies.extend(function(source,f) for f in ['CtrCoop_EngineNpcMotionCapture','NpcMotionTravels','NpcMotionTravelAction','CtrCoop_EngineNpcMotionApply'])
    bodies.append(function(movement,'ScriptMovement_TakeStep'))
    # Native routines use forward declarations and intentionally unused engine arguments.
    prototypes='\n'.join(b.split('{',1)[0].strip()+';' for b in bodies
                         if re.match(r'^(?:static )?(?:bool8|void|u8|int|bool) \w+\(',b))
    out=ROOT/'build/coop-fixes-20261114';out.mkdir(parents=True,exist_ok=True)
    cfile=out/'npc-native.c';exe=out/('npc-native.exe' if os.name=='nt' else 'npc-native')
    cfile.write_text(HEADER+'\n'+prototypes+'\n'+''.join(paths)+''.join(bodies)+HARNESS,encoding='utf-8')
    subprocess.run(shlex.split(args.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-DPLATFORM_3DS',
                    '-iquote',str(ROOT/'3ds_port/include'),'-iquote',str(root/'include'),str(cfile),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)

if __name__=='__main__':main()
