"""Run real remote movement and native grass effects after coalesced snapshots."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
from test_coop_bike_hooks import function, UPSTREAM

ROOT = Path(__file__).resolve().parents[2]
HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "3ds_coop_protocol.h"
#include "constants/field_effects.h"
#include "constants/event_objects.h"
#include "constants/local_link.h"
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int16_t s16;typedef bool bool8;
#define TRUE true
#define FALSE false
#define OBJECT_EVENTS_COUNT 16
#define MAX_SPRITES 64
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define sObjEventId data[0]
struct Coords16 {s16 x,y;};
struct ObjectEvent {
 bool active,isPlayer,frozen,heldMovementActive,heldMovementFinished,singleMovementActive,
      triggerGroundEffectsOnMove,triggerGroundEffectsOnStop,hasShadow,hasReflection,
      inShortGrass,inShallowFlowingWater,inSandPile,inHotSprings,trackedByCamera,disableCoveringGroundEffects;
 u8 localId,mapGroup,mapNum,spriteId,graphicsId,facingDirection,currentMetatileBehavior,previousMetatileBehavior,previousElevation;
 struct Coords16 currentCoords,previousCoords;
} gObjectEvents[16];
struct SpriteTemplate;
struct Sprite {bool inUse,coordOffsetEnabled;const struct SpriteTemplate *template;struct {u8 priority;} oam;s16 x,y,data[8];s16 centerToCornerVecX,centerToCornerVecY;u8 animCmdIndex;} gSprites[64];
struct ObjectEventGraphicsInfo {u8 width,height;};
static const struct ObjectEventGraphicsInfo info={16,32};
#define OBJECT_GRAPHICS_INFO(o) (&info)
struct {struct {u8 mapGroup,mapNum;} location;} save,*gSaveBlock1Ptr=&save;
static struct {bool remoteValid,haveApplied;u8 objectEventId;CtrCoopSnapshot remote,applied;} sCoop;
static struct {CtrCoopSnapshot pose[64];u8 head,count;} sRemoteSteps;
static u32 gFieldEffectArguments[8];
static unsigned effects,animated,relocations,spawns,heldStarts;
static u8 terrain;
static const void *gFieldEffectObjectTemplatePointers[64];
static const int effectTemplates[64]={0};
static void CtrCoop_EngineRemoteStepEffects(uint8_t objectEventId);
static void DoGroundEffects_OnBeginStep(struct ObjectEvent*,struct Sprite*);
static void DoGroundEffects_OnSpawn(struct ObjectEvent*,struct Sprite*);
static void UpdateObjectEventCurrentMovement(struct ObjectEvent*,struct Sprite*,bool8(*)(struct ObjectEvent*,struct Sprite*));
static void RemoveRemoteObject(void){sCoop.objectEventId=OBJECT_EVENTS_COUNT;}
static bool SpawnRemoteObject(void){++spawns;return false;}
static bool CtrCoopBike_SetRemote(u8 graphics,u8 mode){(void)graphics;(void)mode;return false;}
static void ObjectEventSetGraphicsId(struct ObjectEvent *o,u8 g){o->graphicsId=g;}
static void ObjectEventTurn(struct ObjectEvent *o,u8 d){o->facingDirection=d;}
static bool ObjectEventIsHeldMovementActive(struct ObjectEvent *o){return o->heldMovementActive;}
static void ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *o){if(o->heldMovementFinished)o->heldMovementActive=false;}
static void ObjectEventSetHeldMovement(struct ObjectEvent *o,u8 action){(void)action;o->heldMovementActive=true;++heldStarts;}
static void ObjectEventClearHeldMovement(struct ObjectEvent *o){o->heldMovementActive=false;}
static void SetSpritePosToMapCoords(s16 x,s16 y,s16 *px,s16 *py){*px=x*16;*py=y*16;++relocations;}
static void SetSpritePosToOffsetMapCoords(s16 *x,s16 *y,s16 dx,s16 dy){*x=*x*16+dx;*y=*y*16+dy;}
static void CameraObjectReset(void){}
static u8 MapGridGetMetatileBehaviorAt(s16 x,s16 y){(void)x;(void)y;return terrain;}
static bool MetatileBehavior_IsTallGrass(u8 tile){return tile==1;}
static bool MetatileBehavior_IsLongGrass(u8 tile){return tile==2;}
static bool ObjectEventIsFarawayIslandMew(struct ObjectEvent *o){(void)o;return false;}
static bool ShouldMewShakeGrass(struct ObjectEvent *o){(void)o;return true;}
static void UpdateObjectEventElevationAndPriority(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static void SetObjectEventSpriteOamTableForLongGrass(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static u8 CreateSpriteAtEnd(const void *t,s16 x,s16 y,u8 sub){(void)sub;unsigned id=16+effects;if(id>=MAX_SPRITES)return MAX_SPRITES;++effects;gSprites[id]=(struct Sprite){.inUse=true,.template=t,.x=x,.y=y};return id;}
static void SeekSpriteAnim(struct Sprite *s,u8 cmd){s->animCmdIndex=cmd;}
static u8 ElevationToPriority(u8 elevation){(void)elevation;return 2;}
static u32 FieldEffectStart(u8 id);
static void TryEnableObjectEventAnim(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static void ObjectEventExecHeldMovementAction(struct ObjectEvent *o,struct Sprite *s){(void)s;if(!o->heldMovementFinished){--o->currentCoords.y;o->triggerGroundEffectsOnMove=true;o->heldMovementFinished=true;}}
static void UpdateObjectEventSpriteAnimPause(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static void UpdateObjectEventVisibility(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static void ObjectEventUpdateSubpriority(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
static void DoGroundEffects_OnFinishStep(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;}
'''
HARNESS = r'''
static u32 FieldEffectStart(u8 id){
 unsigned old=effects;
 if(id==FLDEFF_TALL_GRASS)FldEff_TallGrass();else if(id==FLDEFF_LONG_GRASS)FldEff_LongGrass();
 for(unsigned i=old;i<effects;++i)if(!gSprites[16+i].animCmdIndex)++animated;
 return 0;
}
static bool8 Idle(struct ObjectEvent *o,struct Sprite *s){(void)o;(void)s;return false;}
static void Frame(void){struct ObjectEvent *o=&gObjectEvents[1];UpdateObjectEventCurrentMovement(o,&gSprites[o->spriteId],Idle);}
static void Init(u8 grass){
 memset(&sCoop,0,sizeof(sCoop));memset(gObjectEvents,0,sizeof(gObjectEvents));memset(gSprites,0,sizeof(gSprites));
 terrain=grass;save.location.mapGroup=0;save.location.mapNum=10;sCoop.objectEventId=1;sCoop.remoteValid=sCoop.haveApplied=true;
 gObjectEvents[1]=(struct ObjectEvent){.active=true,.localId=LOCALID_COOP_PLAYER,.mapGroup=0,.mapNum=10,.spriteId=1,.graphicsId=1,.currentCoords={20,21},.previousCoords={20,21}};
 gSprites[1].inUse=true;
 for(unsigned i=0;i<64;++i)gFieldEffectObjectTemplatePointers[i]=&effectTemplates[i];
 sCoop.remote=(CtrCoopSnapshot){.mapNum=10,.mapLayoutId=1,.x=20,.y=21,.graphicsId=1,.facing=2,.movementActionId=CTR_COOP_ACTION_FACE_UP};
 sCoop.applied=sCoop.remote;effects=animated=relocations=spawns=heldStarts=0;
}
int main(void){
 for(u8 grass=1;grass<=2;++grass){
  Init(grass);CtrCoopSnapshot local=sCoop.remote;
  // Native spawn is deliberately still: this is correct for an idle arrival.
  gObjectEvents[1].triggerGroundEffectsOnMove=true;Frame();assert(effects==1&&!animated);
  effects=animated=0;
  // Walking packet was coalesced; last received state is already idle at next tile.
  sCoop.remote.y=20;UpdateRemoteObject(&local);Frame();
  assert(relocations==1&&effects==1&&animated==1);
  assert(gSprites[16].data[1]==20&&gSprites[16].data[2]==20);
  assert(gSprites[16].coordOffsetEnabled&&IsTileEffect(gSprites[16].template));
  assert((u8)(gSprites[16].data[3]>>8)==LOCALID_COOP_PLAYER);
  assert((u8)gSprites[16].data[3]==10&&gSprites[16].data[4]==0);
  // Data consumed once: identical heartbeat and facing changes do not replay.
  for(unsigned i=0;i<30;++i){UpdateRemoteObject(&local);Frame();}
  sCoop.remote.facing=4;sCoop.remote.movementActionId=CTR_COOP_ACTION_FACE_RIGHT;UpdateRemoteObject(&local);Frame();
  assert(effects==1&&animated==1&&relocations==1);
  // Another correction makes one more rustle; no stationary respawn of the grass.
  sCoop.remote.x=21;UpdateRemoteObject(&local);Frame();assert(effects==2&&animated==2);
  // A correction that cancels a jumping action must not carry its old mask.
  gObjectEvents[1].disableCoveringGroundEffects=true;sCoop.remote.x=22;
  UpdateRemoteObject(&local);Frame();assert(effects==3&&animated==3);
  // Ordinary native held step keeps its existing begin-step lifecycle.
  Init(grass);local=sCoop.remote;sCoop.remote.y=20;sCoop.remote.movementActionId=CTR_COOP_ACTION_WALK_NORMAL_DOWN+1;
  UpdateRemoteObject(&local);assert(heldStarts==1&&!effects);Frame();assert(effects==1&&animated==1&&!relocations);
  UpdateRemoteObject(&local);Frame();assert(effects==1&&animated==1);
 }
 Init(0);CtrCoopSnapshot local=sCoop.remote;sCoop.remote.y=20;UpdateRemoteObject(&local);Frame();assert(!effects);
 Init(1);gObjectEvents[1].triggerGroundEffectsOnMove=true;
 gObjectEvents[1].localId=2;CtrCoop_EngineRemoteStepEffects(1);assert(!effects);
 gObjectEvents[1].localId=LOCALID_COOP_PLAYER;gObjectEvents[1].mapNum=11;CtrCoop_EngineRemoteStepEffects(1);assert(!effects);
 gObjectEvents[1].mapNum=10;gSprites[1].inUse=false;CtrCoop_EngineRemoteStepEffects(1);assert(!effects);
 gSprites[1].inUse=true;gObjectEvents[1].spriteId=MAX_SPRITES;CtrCoop_EngineRemoteStepEffects(1);assert(!effects);
 CtrCoop_EngineRemoteStepEffects(OBJECT_EVENTS_COUNT);gSaveBlock1Ptr=NULL;CtrCoop_EngineRemoteStepEffects(1);assert(!effects);
 puts("real remote snapshot/native tall+long grass: corrected and normal steps animate once; idle/turn/map/sprite guards passed");return 0;
}
'''

def source(backend, engine, field, flags):
    native_names = ['SetObjectEventCoords', 'ResetObjectEventFldEffData', 'MoveObjectEventToMapCoords',
                    'ObjectEventUpdateMetatileBehaviors', 'GetGroundEffectFlags_TallGrassOnSpawn',
                    'GetGroundEffectFlags_TallGrassOnBeginStep', 'GetGroundEffectFlags_LongGrassOnSpawn',
                    'GetGroundEffectFlags_LongGrassOnBeginStep', 'GetAllGroundEffectFlags_OnSpawn',
                    'GetAllGroundEffectFlags_OnBeginStep', 'GroundEffect_SpawnOnTallGrass',
                    'GroundEffect_StepOnTallGrass', 'GroundEffect_SpawnOnLongGrass', 'GroundEffect_StepOnLongGrass']
    bodies = [function(engine, name) for name in native_names]
    ground_helpers = sorted(set(re.findall(r'GetGroundEffectFlags_\w+', ''.join(bodies)))-set(native_names))
    stubs = ''.join(f'static void {name}(struct ObjectEvent *o,u32 *flags){{(void)o;(void)flags;}}\n' for name in ground_helpers)
    start = engine.index('static void (*const sGroundEffectFuncs[]')
    table = engine[start:engine.index('};',start)+2]+'\n'
    others = set(re.findall(r'\bGroundEffect_\w+', table))-set(native_names)
    stubs += ''.join(f'static void {name}(struct ObjectEvent *o,struct Sprite *s){{(void)o;(void)s;}}\n' for name in sorted(others))
    bodies += [table]+[function(engine,name) for name in ['DoFlaggedGroundEffects','filters_out_some_ground_effects',
                'DoGroundEffects_OnSpawn','DoGroundEffects_OnBeginStep','CtrCoop_EngineRemoteStepEffects','UpdateObjectEventCurrentMovement']]
    macros = '\n#define sElevation data[0]\n#define sX data[1]\n#define sY data[2]\n#define sMapNum data[3]\n#define sMapGroup data[4]\n#define sCurrentMap data[5]\n'
    bodies += [macros, function(field,'FldEff_TallGrass'),function(field,'FldEff_LongGrass')]
    bodies += [function((ROOT/'3ds_port/src/voxel/voxel_entities.c').read_text(encoding='utf-8'),'IsTileEffect')]
    bodies += [function(backend,name) for name in ['SnapshotsEqual','RemoteObjectValid','UpdateRemoteObject']]
    return HEADER+flags+'\n'+stubs+'\n'+''.join(bodies)+HARNESS

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc',default=os.environ.get('HOSTCC','gcc'))
    parser.add_argument('--source',type=Path,default=UPSTREAM/'src')
    parser.add_argument('--check-baseline',action='store_true')
    args=parser.parse_args()
    engine=(args.source/'event_object_movement.c').read_text(encoding='utf-8')
    field=(args.source/'field_effect_helpers.c').read_text(encoding='utf-8')
    backend=(ROOT/'3ds_port/src/3ds_coop.c').read_text(encoding='utf-8')
    flags='\n'.join(line for line in (args.source.parent/'include/event_object_movement.h').read_text().splitlines() if line.startswith('#define GROUND_EFFECT_FLAG_'))
    variants={'current':backend}
    if args.check_baseline:
        variants['baseline']=(ROOT/'build/coop-nextwave/grass-baseline/3ds_coop.c').read_text(encoding='utf-8')
    out=ROOT/'build/coop-nextwave/grass-test';out.mkdir(parents=True,exist_ok=True)
    for name,body in variants.items():
        path=out/(name+'.c');exe=path.with_suffix('.exe' if os.name=='nt' else '.out')
        path.write_text(source(body,engine,field,flags),encoding='utf-8')
        subprocess.run(shlex.split(args.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-parameter',
                       '-DPLATFORM_3DS','-iquote'+str(ROOT/'3ds_port/include'),'-iquote'+str(args.source.parent/'include'),
                       str(path),str(ROOT/'3ds_port/src/3ds_coop_protocol.c'),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],capture_output=True,text=True)
        if name=='current':
            assert result.returncode==0,result.stdout+result.stderr
            print(result.stdout.strip())
        else:
            assert result.returncode!=0,'baseline did not reproduce missing remote grass animation'
            print('expected baseline failure: coalesced correction uses still spawn instead of animated grass step')

if __name__=='__main__':main()
