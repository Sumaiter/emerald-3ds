#!/usr/bin/env python3
"""Execute actual staged engine function bodies with palette/sprite probes."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
UPSTREAM = ROOT if (ROOT / "src/event_object_movement.c").is_file() else ROOT / "build/upstream"


def function(text, name):
    match = re.search(r"^(?:static )?[^\n;{}]*\b"+name+r"\([^;{}]*\)\n\{",text,re.M)
    if not match: raise ValueError("Missing engine function: "+name)
    start = text.index("{",match.start())
    depth = 1
    end = start+1
    while depth:
        depth += (text[end]=="{")-(text[end]=="}")
        end += 1
    return text[match.start():end]+"\n"


HEADER = r'''
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "sprite.h"
#include "palette.h"
#include "field_camera.h"
#include "event_object_movement.h"
#include "metatile_behavior.h"
#include "field_weather.h"
#include "3ds_coop_bike.h"
#define OBJ_EVENT_PAL_TAG_NONE 0x11FF
#define sReflectionVerticalOffset data[2]
#define REFLECTION_GRAPHICS_INFO(o) CtrCoopBike_ObjectGraphicsInfo((o),GetObjectEventGraphicsInfo((o)->graphicsId))
struct Sprite gSprites[MAX_SPRITES + 1];
static struct ObjectEventGraphicsInfo info;
static struct SpriteFrameImage frames[27];
static const union AnimCmd anim[] = {ANIMCMD_END};
static const union AnimCmd *const anims[] = {anim};
static const struct OamData oam = {.shape=1,.size=2};
static bool own;
static unsigned writes, binds, camera, reflectionBinds;
static u16 darkTag;
static u8 bridge;
static void _PatchObjectPalette(u16 tag, u8 slot);
static void LoadObjectHighBridgeReflectionPalette(struct ObjectEvent *,u8);
static void LoadObjectRegularReflectionPalette(struct ObjectEvent *,u8);
'''

HARNESS = r'''
const struct ObjectEventGraphicsInfo *GetObjectEventGraphicsInfo(u8 id) { (void)id; return &info; }
const struct ObjectEventGraphicsInfo *CtrCoopBike_GraphicsInfo(uint8_t id,uint8_t local,uint8_t type,const struct ObjectEventGraphicsInfo *original)
{ (void)id;(void)local;(void)type; return original; }
const struct ObjectEventGraphicsInfo *CtrCoopBike_ObjectGraphicsInfo(const struct ObjectEvent *o,const struct ObjectEventGraphicsInfo *original)
{ (void)o; return original; }
bool CtrCoopBike_UsesOwnPalette(uint8_t id,uint8_t local,uint8_t type) { (void)id;(void)local;(void)type;return own; }
void CtrCoopBike_BindSprite(const struct ObjectEvent *o,struct Sprite *s) { (void)s; if(o->graphicsId==7) ++binds; }
bool CtrCoopBike_BindReflection(const struct ObjectEvent *o,const struct Sprite *s,struct Sprite *r,uint16_t tag)
{ (void)o;(void)s;(void)r;++reflectionBinds;darkTag=tag;return true; }
void PatchObjectPalette(u16 tag,u8 slot) { (void)tag;(void)slot;++writes; }
static void _PatchObjectPalette(u16 tag,u8 slot) { PatchObjectPalette(tag,slot); }
void LoadPlayerObjectReflectionPalette(u16 tag,u8 slot) { PatchObjectPalette(tag,slot); }
void LoadSpecialObjectReflectionPalette(u16 tag,u8 slot) { PatchObjectPalette(tag,slot); }
void UpdateSpritePaletteWithWeather(u8 slot) { (void)slot; }
u16 GetObjectPaletteTag(u8 slot) { (void)slot;return 0; }
u8 MetatileBehavior_GetBridgeType(u8 behavior) { (void)behavior;return bridge; }
void SetSpritePosToMapCoords(s16 x,s16 y,s16 *px,s16 *py) { *px=x*16;*py=y*16; }
void CameraObjectReset(void) { ++camera; }
int main(void)
{
    struct ObjectEvent object={0};
    struct Sprite reflection={0};
    const u8 slots[]={PALSLOT_PLAYER,PALSLOT_NPC_SPECIAL,PALSLOT_NPC_1};
    info.oam=&oam;info.images=frames;info.anims=anims;info.width=info.height=32;
    info.reflectionPaletteTag=0x501;
    for(unsigned p=0;p<2;++p) for(unsigned i=0;i<3;++i)
    {
        own=p; writes=binds=camera=0;
        memset(gSprites,0,sizeof(gSprites));
        object.graphicsId=5;object.spriteId=0;object.currentCoords.x=3;object.currentCoords.y=4;object.trackedByCamera=true;
        info.paletteSlot=slots[i];
        ObjectEventSetGraphicsId(&object,7);
        if(object.graphicsId!=7 || gSprites[0].images!=frames || gSprites[0].anims!=anims
         || gSprites[0].oam.shape!=1 || gSprites[0].oam.size!=2 || binds!=1 || camera!=1
         || gSprites[0].x!=56 || gSprites[0].y!=64 || writes!=(!own && i<2?1:0))
        { printf("graphics transition failed own=%u slot=%u writes=%u binds=%u\n",p,slots[i],writes,binds);return 1; }
    }
    info.paletteSlot=PALSLOT_NPC_SPECIAL;
    for(unsigned p=0;p<2;++p) for(unsigned high=0;high<2;++high)
    {
        own=p;bridge=high?BRIDGE_TYPE_POND_HIGH:0;writes=reflectionBinds=0;darkTag=0;
        LoadObjectReflectionPalette(&object,&reflection);
        if(writes!=(own?0:1) || reflectionBinds!=(own?1:0)
         || reflection.sReflectionVerticalOffset!=(high?44:0)
         || (own && darkTag!=(high?0x501:65535)) || GetReflectionVerticalOffset(&object)!=30)
        { printf("reflection failed own=%u high=%u writes=%u binds=%u tag=%u\n",p,high,writes,reflectionBinds,darkTag);return 1; }
    }
    puts("actual engine bicycle graphics and reflection hook tests passed");
    return 0;
}
'''


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source",type=Path,default=UPSTREAM/"src")
    p.add_argument("--cc",default=os.environ.get("HOSTCC","gcc"))
    a=p.parse_args()
    movement=(a.source/"event_object_movement.c").read_text()
    reflections=(a.source/"field_effect_helpers.c").read_text()
    bodies=function(movement,"ObjectEventSetGraphicsId")
    for name in ("GetReflectionVerticalOffset","LoadObjectReflectionPalette","LoadObjectRegularReflectionPalette","LoadObjectHighBridgeReflectionPalette"):
        bodies+=function(reflections,name)
    with tempfile.TemporaryDirectory(prefix="coop-bike-hooks-") as directory:
        directory=Path(directory)
        source=directory/"hooks.c";exe=directory/("hooks.exe" if os.name=="nt" else "hooks")
        source.write_text(HEADER+bodies+HARNESS)
        command=shlex.split(a.cc)+["-std=gnu99","-O2","-Wall","-Wextra","-Werror",
                 "-DPORTABLE","-DPLATFORM_3DS","-DPORT_BRIDGE"]
        for path in (ROOT/"3ds_port/include",UPSTREAM/"include",UPSTREAM/"include/constants",UPSTREAM,UPSTREAM/"3ds_port/compat"):
            command.extend(["-iquote",str(path)])
        subprocess.run(command+[str(source),"-o",str(exe)],check=True)
        subprocess.run([str(exe)],check=True)


if __name__=="__main__": main()
