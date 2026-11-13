/* New foot poses for the fifteen previously static selectable characters.
 * Original object graphics, NPCs and IDs remain unchanged. */
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "sprite.h"
#include "palette.h"
#include "event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "3ds_coop_walk.h"
#include "3ds_platform.h"

#define WALK_COUNT 15
#define WALK_FRAMES 9
#define WALK_FRAME_BYTES 512
#define WALK_ART_BYTES (32 + WALK_FRAMES * WALK_FRAME_BYTES)
#define WALK_HEADER_BYTES 8
#define WALK_RECORD_BYTES 8
#define WALK_PREVIEW_TAG 0x7D00

static const uint8_t sWalkIds[WALK_COUNT] = {
    OBJ_EVENT_GFX_COOK, OBJ_EVENT_GFX_OLD_MAN, OBJ_EVENT_GFX_SIDNEY,
    OBJ_EVENT_GFX_PHOEBE, OBJ_EVENT_GFX_GLACIA, OBJ_EVENT_GFX_DRAKE,
    OBJ_EVENT_GFX_ROXANNE, OBJ_EVENT_GFX_BRAWLY, OBJ_EVENT_GFX_WATTSON,
    OBJ_EVENT_GFX_FLANNERY, OBJ_EVENT_GFX_WINONA, OBJ_EVENT_GFX_LIZA,
    OBJ_EVENT_GFX_TATE, OBJ_EVENT_GFX_GAMEBOY_KID, OBJ_EVENT_GFX_MYSTERY_GIFT_MAN
};

static uint8_t *sWalkArt;
static bool sWalkTried;
static const uint8_t *sWalkRecords[WALK_COUNT];
static struct ObjectEventGraphicsInfo sWalkInfo[WALK_COUNT];
static struct SpriteFrameImage sWalkFrames[WALK_COUNT][WALK_FRAMES];
static struct SpriteTemplate sPreviewTemplates[WALK_COUNT];
static const union AnimCmd *sWalkAnims[ANIM_STD_COUNT + 4];

/* Running reuses the new step drawings, with the native running cadence.
 * Every run direction stays within the nine-frame pack. */
static const union AnimCmd sWalkRunSouth[] = {
    ANIMCMD_FRAME(3,5), ANIMCMD_FRAME(0,3), ANIMCMD_FRAME(4,5), ANIMCMD_FRAME(0,3), ANIMCMD_JUMP(0)
};
static const union AnimCmd sWalkRunNorth[] = {
    ANIMCMD_FRAME(5,5), ANIMCMD_FRAME(1,3), ANIMCMD_FRAME(6,5), ANIMCMD_FRAME(1,3), ANIMCMD_JUMP(0)
};
static const union AnimCmd sWalkRunWest[] = {
    ANIMCMD_FRAME(7,5), ANIMCMD_FRAME(2,3), ANIMCMD_FRAME(8,5), ANIMCMD_FRAME(2,3), ANIMCMD_JUMP(0)
};
static const union AnimCmd sWalkRunEast[] = {
    ANIMCMD_FRAME(7,5,.hFlip=TRUE), ANIMCMD_FRAME(2,3,.hFlip=TRUE),
    ANIMCMD_FRAME(8,5,.hFlip=TRUE), ANIMCMD_FRAME(2,3,.hFlip=TRUE), ANIMCMD_JUMP(0)
};

static uint16_t WalkRead16(const uint8_t *p) { return p[0] | ((uint16_t)p[1] << 8); }
static uint32_t WalkRead32(const uint8_t *p) { return WalkRead16(p) | ((uint32_t)WalkRead16(p+2) << 16); }
static int WalkIndex(uint8_t id)
{
    for (unsigned i=0; i<WALK_COUNT; ++i) if (sWalkIds[i]==id) return i;
    return -1;
}

static bool LoadWalkArt(void)
{
    FILE *file;
    long size;
    const unsigned base=WALK_HEADER_BYTES + WALK_COUNT*WALK_RECORD_BYTES;
    bool seen[WALK_COUNT]={false};
    if (sWalkTried) return sWalkArt!=NULL;
    sWalkTried=true;
    file=CtrFs_OpenAsset("coop/walks.bin");
    if (file==NULL) goto bad;
    if (fseek(file,0,SEEK_END) || (size=ftell(file)) != (long)(base + WALK_COUNT*WALK_ART_BYTES)
     || fseek(file,0,SEEK_SET)) { fclose(file); goto bad; }
    sWalkArt=malloc(size);
    if (sWalkArt==NULL) { fclose(file); goto bad; }
    if (fread(sWalkArt,1,size,file)!=(size_t)size) { fclose(file); goto bad; }
    fclose(file);
    if (memcmp(sWalkArt,"CWK1",4) || WalkRead16(sWalkArt+4)!=1 || WalkRead16(sWalkArt+6)!=WALK_COUNT) goto bad;
    for (unsigned i=0; i<WALK_COUNT; ++i)
    {
        const uint8_t *r=sWalkArt+WALK_HEADER_BYTES+i*WALK_RECORD_BYTES;
        uint16_t id=WalkRead16(r);
        int index=id<239 ? WalkIndex(id) : -1;
        /* Canonical contiguous payloads prevent aliasing and partial records. */
        if (index<0 || seen[index] || r[2] || r[3] || WalkRead32(r+4)!=base+i*WALK_ART_BYTES) goto bad;
        seen[index]=true; sWalkRecords[index]=r;
    }
    return true;
bad:
    free(sWalkArt); sWalkArt=NULL;
    memset(sWalkRecords,0,sizeof(sWalkRecords));
    CtrLog_Write(CTR_LOG_ERROR,"coop: missing or invalid walking art");
    return false;
}

const uint16_t *CtrCoopWalk_Palette(uint8_t id)
{
    int index=WalkIndex(id);
    if (index<0 || !LoadWalkArt()) return NULL;
    return (const uint16_t *)(sWalkArt+WalkRead32(sWalkRecords[index]+4));
}

const struct ObjectEventGraphicsInfo *CtrCoopWalk_GraphicsInfo(uint8_t id,
    const struct ObjectEventGraphicsInfo *original)
{
    int index=WalkIndex(id);
    const struct ObjectEventGraphicsInfo *shape;
    const uint8_t *pixels;
    if (index<0 || original==NULL || !LoadWalkArt()) return original;
    if (sWalkInfo[index].images==sWalkFrames[index]) return &sWalkInfo[index];
    /* Mach's standard 0..19 animation indices match the nine foot frames.
     * Its 32x32 geometry has the same bottom anchor as the bicycle frames. */
    shape=GetObjectEventGraphicsInfo(OBJ_EVENT_GFX_BRENDAN_MACH_BIKE);
    for (unsigned i=0; i<ANIM_STD_COUNT; ++i) sWalkAnims[i]=shape->anims[i];
    sWalkAnims[ANIM_RUN_SOUTH]=sWalkRunSouth;
    sWalkAnims[ANIM_RUN_NORTH]=sWalkRunNorth;
    sWalkAnims[ANIM_RUN_WEST]=sWalkRunWest;
    sWalkAnims[ANIM_RUN_EAST]=sWalkRunEast;
    sWalkInfo[index]=*original;
    sWalkInfo[index].size=WALK_FRAME_BYTES;
    sWalkInfo[index].width=sWalkInfo[index].height=32;
    sWalkInfo[index].oam=shape->oam;
    sWalkInfo[index].subspriteTables=shape->subspriteTables;
    sWalkInfo[index].inanimate=FALSE;
    sWalkInfo[index].tracks=TRACKS_FOOT;
    sWalkInfo[index].anims=sWalkAnims;
    pixels=sWalkArt+WalkRead32(sWalkRecords[index]+4)+32;
    for (unsigned i=0; i<WALK_FRAMES; ++i)
    {
        sWalkFrames[index][i].data=pixels+i*WALK_FRAME_BYTES;
        sWalkFrames[index][i].size=WALK_FRAME_BYTES;
#ifdef PORT_BRIDGE
        sWalkFrames[index][i].offset=0;
#endif
    }
    sWalkInfo[index].images=sWalkFrames[index];
    return &sWalkInfo[index];
}

uint8_t CtrCoopWalk_CreatePreview(uint8_t id, void (*callback)(struct Sprite *),
    int16_t x,int16_t y,uint8_t subpriority)
{
    int index=WalkIndex(id);
    const struct ObjectEventGraphicsInfo *original=GetObjectEventGraphicsInfo(id);
    const struct ObjectEventGraphicsInfo *info=CtrCoopWalk_GraphicsInfo(id,original);
    struct SpriteTemplate *template;
    uint8_t spriteId,slot;
    if (index<0 || info==original) return CreateObjectGraphicsSprite(id,callback,x,y,subpriority);
    template=&sPreviewTemplates[index];
    *template=(struct SpriteTemplate){.tileTag=info->tileTag,.paletteTag=TAG_NONE,
        .oam=info->oam,.anims=info->anims,.images=info->images,
        .affineAnims=info->affineAnims,.callback=callback};
    slot=LoadSpritePaletteExt(&(const struct SpritePalette){CtrCoopWalk_Palette(id),WALK_PREVIEW_TAG+index});
    if (slot==255) return MAX_SPRITES;
    spriteId=CreateSprite(template,x,y,subpriority);
    if (spriteId!=MAX_SPRITES)
    {
        SetSpriteExtPalette(&gSprites[spriteId],slot);
        if (info->subspriteTables!=NULL)
        {
            SetSubspriteTables(&gSprites[spriteId],info->subspriteTables);
            gSprites[spriteId].subspriteMode=SUBSPRITES_IGNORE_PRIORITY;
        }
    }
    return spriteId;
}

void CtrCoopWalk_FreePreviewPalettes(void)
{
    for (unsigned i=0; i<WALK_COUNT; ++i) FreeSpritePaletteByTag(WALK_PREVIEW_TAG+i);
}

void CtrCoopWalk_Reset(void)
{
    CtrCoopWalk_FreePreviewPalettes();
    free(sWalkArt); sWalkArt=NULL; sWalkTried=false;
    memset(sWalkRecords,0,sizeof(sWalkRecords));
    memset(sWalkInfo,0,sizeof(sWalkInfo));
}
