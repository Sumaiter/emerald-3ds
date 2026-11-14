"""Execute the real object/flag engine hooks and the real Route103 completion."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from test_coop_bike_hooks import function, UPSTREAM

HEADER = r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef bool bool8;
#define TRUE true
#define FALSE false
#define OBJECT_EVENTS_COUNT 16
#define OBJECT_EVENT_TEMPLATES_COUNT 64
#define LOCALID_COOP_PLAYER 126
#define MAP_OFFSET 7
#define NUM_MOVEMENT_TYPES 81
struct Coords { s16 x,y; };
struct ObjectEvent { bool active,isPlayer;u8 localId,mapNum,mapGroup,movementType;struct Coords currentCoords; };
struct ObjectEventTemplate { u8 localId,movementType;u16 flagId;s16 x,y; };
struct { struct {u8 mapGroup,mapNum;} location;struct ObjectEventTemplate objectEventTemplates[64];u8 flags[512];u16 vars[256]; } save,*gSaveBlock1Ptr=&save;
struct ObjectEvent gObjectEvents[16];
static u16 specialResult;
static unsigned records,removals,moves,turns,flagWrites;
static u8 lastFields,lastId,lastMovement;
static s16 lastX,lastY;
static bool own,checkRemovalHook;
#define CHECK(c) do { if(!(c)) {printf("story engine failure line %d\n",__LINE__);exit(1);} } while(0)
static u8 *GetFlagPointer(u16 id) { return id && id<4096 ? &save.flags[id/8] : NULL; }
static bool FlagGet(u16 id) { return id && id<4096 && (save.flags[id/8] & (1<<(id&7))); }
static u16 *GetVarPointer(u16 id) { if(id==0x800D)return &specialResult;return id>=0x4000 && id<0x4100 ? &save.vars[id-0x4000] : NULL; }
static void CtrCoop_StoryFlagChanged(u16 id) { (void)id;++flagWrites; }
static void CtrCoop_StoryVarChanged(u16 id) { (void)id; }
static void CtrCoop_StoryObject(u8 group,u8 num,u8 id,u8 fields,s16 x,s16 y,u8 movement)
{ CHECK(group==0 && num==18);++records;lastId=id;lastFields=fields;lastX=x;lastY=y;lastMovement=movement; }
static const struct ObjectEventTemplate *GetObjectEventTemplateByLocalIdAndMap(u8 id,u8 num,u8 group)
{ if(group!=save.location.mapGroup || num!=save.location.mapNum)return NULL;for(unsigned i=0;i<64;++i)if(save.objectEventTemplates[i].localId==id)return &save.objectEventTemplates[i];return NULL; }
static struct ObjectEventTemplate *GetBaseTemplateForObjectEvent(const struct ObjectEvent *o)
{ return (struct ObjectEventTemplate *)GetObjectEventTemplateByLocalIdAndMap(o->localId,o->mapNum,o->mapGroup); }
static bool TryGetObjectEventIdByLocalIdAndMap(u8 id,u8 num,u8 group,u8 *index)
{ for(unsigned i=0;i<16;++i)if(gObjectEvents[i].active && gObjectEvents[i].localId==id && gObjectEvents[i].mapNum==num && gObjectEvents[i].mapGroup==group){*index=i;return false;}return true; }
static void RemoveObjectEvent(struct ObjectEvent *o) { if(checkRemovalHook)CHECK(records && lastFields==8);o->active=false;++removals; }
static u16 GetObjectEventFlagIdByObjectEventId(u8 index)
{ const struct ObjectEventTemplate *t=GetBaseTemplateForObjectEvent(&gObjectEvents[index]);return t?t->flagId:0; }
static void GetObjectEventMovingCameraOffset(s16 *x,s16 *y) { *x=*y=0; }
static u8 TrySpawnObjectEventTemplate(const struct ObjectEventTemplate *t,u8 num,u8 group,s16 cx,s16 cy)
{ (void)cx;(void)cy;for(unsigned i=1;i<16;++i)if(!gObjectEvents[i].active){gObjectEvents[i]=(struct ObjectEvent){.active=true,.localId=t->localId,.mapNum=num,.mapGroup=group,.movementType=t->movementType,.currentCoords={t->x+7,t->y+7}};return i;}return 16; }
static void MoveObjectEventToMapCoords(struct ObjectEvent *o,s16 x,s16 y) { o->currentCoords.x=x;o->currentCoords.y=y;++moves; }
static void SetTrainerMovementType(struct ObjectEvent *o,u8 movement) {o->movementType=movement;++turns;}
static bool CtrCoop_StoryOwnsNpc(u8 group,u8 num,u8 id) { (void)group;(void)num;(void)id;return own; }
bool CtrCoop_EngineStoryNpcAvailable(u8 group,u8 num,u8 id);
static bool CtrCoop_StoryNpcVisible(u8 group,u8 num,u8 id) { return CtrCoop_EngineStoryNpcAvailable(group,num,id); }
static void Init(void)
{ memset(&save,0,sizeof(save));memset(gObjectEvents,0,sizeof(gObjectEvents));save.location.mapNum=18;
  save.objectEventTemplates[0]=(struct ObjectEventTemplate){.localId=2,.flagId=0x2D3,.x=10,.y=3,.movementType=4};
  gObjectEvents[1]=(struct ObjectEvent){.active=true,.localId=2,.mapNum=18,.currentCoords={17,10}};
  records=removals=moves=turns=flagWrites=0;own=checkRemovalHook=false; }
'''

HARNESS = r'''
int main(void)
{
    Init();specialResult=0x1234;
    FlagSet(0x379);FlagSet(0x3D3);
    checkRemovalHook=true;
    /*REAL_RIVAL_END*/
    CHECK(!gObjectEvents[1].active && FlagGet(0x2D3) && FlagGet(0x82));
    CHECK(!FlagGet(0x379) && !FlagGet(0x3D3) && save.vars[0x84]==4 && save.vars[0xC7]==1);
    CHECK(specialResult==0x1234); /* Completion does not transfer VAR_RESULT. */
    /* Recreate the original host bug: receiving the save flag leaves a live
       NPC until the generic world reconciliation runs. */
    checkRemovalHook=false;gObjectEvents[1].active=true;
    CHECK(!CtrCoop_EngineStoryNpcAvailable(0,18,2));
    unsigned writes=flagWrites;own=true;CtrCoop_EngineStoryRefresh();
    CHECK(gObjectEvents[1].active); /* An owned exit animation is retained. */
    own=false;CtrCoop_EngineStoryRefresh();
    CHECK(!gObjectEvents[1].active && flagWrites==writes); /* No flag echo. */

    Init();CtrCoop_EngineStoryObject(0,18,2,3,44,25,1);
    CHECK(save.objectEventTemplates[0].x==44 && save.objectEventTemplates[0].y==25);
    CHECK(gObjectEvents[1].currentCoords.x==51 && gObjectEvents[1].currentCoords.y==32 && gObjectEvents[1].movementType==1);
    gObjectEvents[1].currentCoords.x=99;unsigned oldMoves=moves;
    CtrCoop_EngineStoryObject(0,18,2,3,44,25,1);
    CHECK(moves==oldMoves && gObjectEvents[1].currentCoords.x==99); /* Autonomous movement continues. */
    CtrCoop_EngineStoryObject(0,18,2,3|16,55,66,2);
    CHECK(save.objectEventTemplates[0].x==55 && gObjectEvents[1].currentCoords.x==99);
    CtrCoop_EngineStoryObject(0,18,2,3|32,55,66,2);
    CHECK(gObjectEvents[1].currentCoords.x==62 && gObjectEvents[1].currentCoords.y==73);
    CtrCoop_EngineStoryObject(0,17,2,3,1,1,1);
    CHECK(save.objectEventTemplates[0].x==55); /* Never corrupt another map's saved templates. */

    Init();gObjectEvents[1].currentCoords=(struct Coords){50,60};
    OverrideTemplateCoordsForObjectEvent(&gObjectEvents[1]);
    CHECK(lastFields==1 && lastX==43 && lastY==53);
    TryOverrideTemplateCoordsForObjectEvent(&gObjectEvents[1],3);
    CHECK(lastFields==2 && lastMovement==3);
    CtrCoop_EngineStoryTemplateChanged(2,3);
    CHECK(lastFields==3 && lastId==2 && lastX==43 && lastY==53 && lastMovement==3);
    gObjectEvents[1].active=false;records=0;
    CHECK(TrySpawnObjectEvent(2,18,0)<16 && records==1 && lastFields==7);
    Init();save.objectEventTemplates[0].flagId=0;checkRemovalHook=true;
    RemoveObjectEventByLocalIdAndMap(2,18,0);
    CHECK(records==1 && lastFields==8 && !gObjectEvents[1].active && !flagWrites);
    CHECK(VarSet(0x800D,1) && specialResult==1); /* Native gender/result local scratch works. */
    puts("real Route103 completion and engine story object tests passed");
    return 0;
}
'''


def constant(path, name):
    match = re.search(r'^#define\s+' + re.escape(name) + r'\s+(0x[0-9A-Fa-f]+|[0-9]+)\b', path.read_text(), re.M)
    if not match:
        raise AssertionError(name)
    return int(match[1], 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=UPSTREAM / 'src')
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    args = parser.parse_args()
    root = args.source.parent
    route = (root / 'data/maps/Route103/scripts.inc').read_text()
    entry = route.split('Route103_EventScript_Rival::', 1)[1].split('Route103_EventScript_RivalMay::', 1)[0]
    assert 'checkplayergender' in entry and 'VAR_RESULT, MALE' in entry and 'VAR_RESULT, FEMALE' in entry
    assert 'FLAG_DEFEATED' not in entry and 'FLAG_HIDE' not in entry
    variants = re.findall(r'trainerbattle_no_intro\s+(TRAINER_(?:MAY|BRENDAN)_ROUTE_103_\w+)', route)
    values = [constant(root / 'include/constants/opponents.h', name) for name in variants]
    assert sorted(values) == [520, 523, 526, 529, 532, 535]
    rival_end = route.split('Route103_EventScript_RivalEnd::', 1)[1].split('\n\tend', 1)[0]
    commands = []
    for line in rival_end.splitlines():
        words = line.strip().replace(',', ' ').split()
        if not words:
            continue
        if words[0] == 'removeobject':
            local_id = constant(root / 'include/constants/map_event_ids.h', words[1])
            commands.append(f'RemoveObjectEventByLocalIdAndMap({local_id},18,0);')
        elif words[0] == 'setvar':
            var = constant(root / 'include/constants/vars.h', words[1])
            commands.append(f'VarSet({var},{int(words[2], 0)});')
        elif words[0] in ('setflag', 'clearflag'):
            flag = constant(root / 'include/constants/flags.h', words[1])
            commands.append(f'{"FlagSet" if words[0]=="setflag" else "FlagClear"}({flag});')
    assert len(commands) == 6
    objects = (args.source / 'event_object_movement.c').read_text()
    event_data = (args.source / 'event_data.c').read_text()
    bodies = ''.join(function(event_data, name) for name in ('VarSet', 'FlagSet', 'FlagClear'))
    bodies += ''.join(function(objects, name) for name in ('TrySpawnObjectEvent',
        'RemoveObjectEventByLocalIdAndMap', 'OverrideTemplateCoordsForObjectEvent',
        'TryOverrideTemplateCoordsForObjectEvent', 'CtrCoop_EngineStoryNpcAvailable',
        'CtrCoop_EngineStoryRefresh', 'CtrCoop_EngineStoryTemplateChanged', 'CtrCoop_EngineStoryObject'))
    with tempfile.TemporaryDirectory(prefix='coop-story-hooks-') as directory:
        source = Path(directory) / 'story.c'
        exe = Path(directory) / ('story.exe' if os.name == 'nt' else 'story')
        source.write_text(HEADER + bodies + HARNESS.replace('/*REAL_RIVAL_END*/', '\n'.join(commands)))
        subprocess.run(shlex.split(args.cc) + ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror',
                       '-DPLATFORM_3DS', str(source), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print('all six starter/gender trainer variants verified against real script constants')


if __name__ == '__main__':
    main()
