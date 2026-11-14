"""Run real scene actor hooks from staged engine bodies before their effects."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from test_coop_bike_hooks import function, UPSTREAM

HEADER = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef bool bool8;
#define TRUE true
#define FALSE false
struct ScriptContext { u16 localId; u8 group, num, index; };
struct ObjectEvent { bool active; u8 mapGroup, mapNum, localId; };
struct ObjectEvent gObjectEvents[16];
u8 gSelectedObjectEvent;
struct { struct { u8 mapGroup, mapNum; } location; } save, *gSaveBlock1Ptr = &save;
static unsigned actors, effects;
static u8 expectedGroup, expectedNum, expectedId;
static bool missing;
#define CHECK(c) do { if (!(c)) { printf("scene hook failed line %d\n", __LINE__); exit(1); } } while (0)
static void CtrCoop_ScriptActor(u8 group, u8 num, u8 id)
{ CHECK(!effects && group==expectedGroup && num==expectedNum && id==expectedId); ++actors; }
static void Effect(void) { CHECK(actors==1); ++effects; }
static const u8 *Port_ResolveEventPointer(const u8 *p) { return p; }
static bool CheckHeap(void) { return true; }
static void Port_SetBirchDebugEx(u16 tag,u16 id,u16 index,u16 map,bool heap)
{ (void)tag;(void)id;(void)index;(void)map;(void)heap; }
static u16 VarGet(u16 id) { return id; }
static u16 ScriptReadHalfword(struct ScriptContext *ctx) { return ctx->localId; }
static u8 ScriptReadByte(struct ScriptContext *ctx) { return ctx->index++ ? ctx->num : ctx->group; }
static u8 TryGetObjectEventIdByLocalIdAndMap(u8 id,u8 num,u8 group,u8 *index)
{ CHECK(id==expectedId && num==expectedNum && group==expectedGroup); *index=1; return missing; }
static void ScriptMovement_MoveObjects(u8 id) { (void)id; }
static bool FuncIsActiveTask(void (*fn)(u8)) { (void)fn; return true; }
static void ScriptMovement_StartMoveObjects(u8 priority) { (void)priority; }
static u8 GetMoveObjectsTaskId(void) { return 0; }
static bool ScriptMovement_TryAddNewMovement(u8 task,u8 object,const u8 *script)
{ (void)task;(void)object;(void)script; Effect();return false; }
static void RemoveObjectEventByLocalIdAndMap(u8 id,u8 num,u8 group)
{ CHECK(id==expectedId && num==expectedNum && group==expectedGroup); Effect(); }
static u8 TrySpawnObjectEvent(u8 id,u8 num,u8 group)
{ CHECK(id==expectedId && num==expectedNum && group==expectedGroup); Effect(); return 1; }
static void TryMoveObjectEventToMapCoords(u8 id,u8 num,u8 group,u16 x,u16 y)
{ (void)x;(void)y; CHECK(id==expectedId && num==expectedNum && group==expectedGroup); Effect(); }
static u8 GetPlayerFacingDirection(void) { return 1; }
static void ObjectEventFaceOppositeDirection(struct ObjectEvent *o,u8 direction)
{ (void)o;(void)direction; Effect(); }
static void ObjectEventTurnByLocalIdAndMap(u8 id,u8 num,u8 group,u8 direction)
{ (void)direction; CHECK(id==expectedId && num==expectedNum && group==expectedGroup); Effect(); }
'''

HARNESS = r'''
int main(void)
{
    bool8 (*local[])(struct ScriptContext *) = {ScrCmd_removeobject, ScrCmd_addobject,
        ScrCmd_setobjectxy, ScrCmd_turnobject, ScrCmd_faceplayer};
    bool8 (*remote[])(struct ScriptContext *) = {ScrCmd_removeobjectat, ScrCmd_addobjectat};
    save.location.mapGroup=24; save.location.mapNum=4;
    expectedGroup=24;expectedNum=4;expectedId=7;
    gSelectedObjectEvent=1;
    gObjectEvents[1]=(struct ObjectEvent){.active=true,.mapGroup=24,.mapNum=4,.localId=7};
    for (unsigned i=0;i<sizeof(local)/sizeof(local[0]);++i)
    {
        struct ScriptContext ctx={.localId=7}; actors=effects=0;
        CHECK(!local[i](&ctx) && actors==1 && effects==1);
    }
    expectedGroup=3;expectedNum=5;
    for (unsigned i=0;i<sizeof(remote)/sizeof(remote[0]);++i)
    {
        struct ScriptContext ctx={.localId=7,.group=3,.num=5}; actors=effects=0;
        CHECK(!remote[i](&ctx) && actors==1 && effects==1);
    }
    actors=effects=0;
    CHECK(!ScriptMovement_StartObjectMovementScript(7,5,3,(const u8 *)1) && actors==1 && effects==1);
    missing=true; actors=effects=0;
    CHECK(ScriptMovement_StartObjectMovementScript(7,5,3,(const u8 *)1) && !actors && !effects);
    puts("actual engine scene actor hook tests passed");
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=UPSTREAM / 'src')
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    args = parser.parse_args()
    commands = (args.source / 'scrcmd.c').read_text()
    movement = (args.source / 'script_movement.c').read_text()
    bodies = function(movement, 'ScriptMovement_StartObjectMovementScript')
    for name in ('removeobject', 'removeobjectat', 'addobject', 'addobjectat',
                 'setobjectxy', 'turnobject', 'faceplayer'):
        bodies += function(commands, 'ScrCmd_' + name)
    with tempfile.TemporaryDirectory(prefix='coop-scene-hooks-') as directory:
        source = Path(directory) / 'hooks.c'
        exe = Path(directory) / ('hooks.exe' if os.name == 'nt' else 'hooks')
        source.write_text(HEADER + bodies + HARNESS)
        subprocess.run(shlex.split(args.cc) + ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror',
                       '-Wno-unused-parameter', '-DPLATFORM_3DS', '-DPORT_BRIDGE',
                       str(source), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
