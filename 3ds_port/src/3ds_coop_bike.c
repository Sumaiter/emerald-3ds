/* Two independent bike views over the selected overworld identities.
 * CBI1 contains new drawings only; Brendan/May retain native bike art. */
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "sprite.h"
#include "palette.h"
#include "field_weather.h"
#include "event_object_movement.h"
#include "main.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "3ds_coop.h"
#include "3ds_coop_bike.h"
#include "3ds_coop_walk.h"
#include "3ds_platform.h"

extern void CB2_Overworld(void);

#define HEADER_BYTES 8
#define RECORD_BYTES 12
#define FRAME_BYTES 512
#define FRAMES 27
#define ART_BYTES (32 + FRAMES * FRAME_BYTES)
#define CUSTOM 255
#define BIKE_PAL_TAG 0x7E10

static uint8_t sLocalMode, sRemoteMode, sRemoteGraphics;
static uint8_t sMovementSequence, sLastAction = 255;
static bool sRemoteValid, sTried;
static uint8_t *sArt;
static uint32_t sArtSize;
static struct ObjectEventGraphicsInfo sInfo[2];
static struct SpriteFrameImage sFrames[2][FRAMES];
static uint8_t sPaletteGraphics[2] = {255, 255};
static uint8_t sPaletteSource[2] = {255, 255};
static uint16_t sReflectionPaletteTag[2] = {65535,65535};

static uint16_t Read16(const uint8_t *p) { return p[0] | ((uint16_t)p[1] << 8); }
static uint32_t Read32(const uint8_t *p) { return Read16(p) | ((uint32_t)Read16(p + 2) << 16); }

static bool LoadArt(void)
{
    FILE *file;
    long size;
    unsigned count;
    bool seen[239] = {false};
    if (sTried) return sArt != NULL;
    sTried = true;
    file = CtrFs_OpenAsset("coop/bikes.bin");
    if (file == NULL) goto bad;
    if (fseek(file, 0, SEEK_END) || (size = ftell(file)) < HEADER_BYTES
     || size > HEADER_BYTES + 239 * (RECORD_BYTES + ART_BYTES)
     || fseek(file, 0, SEEK_SET)) { fclose(file); goto bad; }
    sArt = malloc(size);
    if (sArt == NULL) { fclose(file); goto bad; }
    sArtSize = size;
    if (fread(sArt, 1, size, file) != (size_t)size) { fclose(file); goto bad; }
    fclose(file);
    if (memcmp(sArt, "CBI1", 4) || Read16(sArt + 4) != 1) goto bad;
    count = Read16(sArt + 6);
    if (!count || count > 239 || HEADER_BYTES + count * RECORD_BYTES > sArtSize) goto bad;
    for (unsigned i = 0; i < count; ++i)
    {
        const uint8_t *r = sArt + HEADER_BYTES + i * RECORD_BYTES;
        unsigned id = r[0];
        uint32_t offset = Read32(r + 4);
        if (id >= 239 || seen[id] || r[3] || Read32(r + 8)) goto bad;
        seen[id] = true;
        if (r[1] == CUSTOM && r[2] == CUSTOM)
        {
            if ((offset & 3) || offset < HEADER_BYTES + count * RECORD_BYTES
             || offset > sArtSize || ART_BYTES > sArtSize - offset) goto bad;
        }
        else if (offset || !((id == OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL
                 && r[1] == OBJ_EVENT_GFX_RIVAL_BRENDAN_MACH_BIKE && r[2] == OBJ_EVENT_GFX_RIVAL_BRENDAN_ACRO_BIKE)
              || (id == OBJ_EVENT_GFX_RIVAL_MAY_NORMAL
                 && r[1] == OBJ_EVENT_GFX_RIVAL_MAY_MACH_BIKE && r[2] == OBJ_EVENT_GFX_RIVAL_MAY_ACRO_BIKE))) goto bad;
    }
    return true;
bad:
    free(sArt); sArt = NULL; sArtSize = 0;
    CtrLog_Write(CTR_LOG_ERROR, "coop: missing or invalid bicycle art");
    return false;
}

static const uint8_t *Record(uint8_t id)
{
    if (!LoadArt()) return NULL;
    for (unsigned i = 0; i < Read16(sArt + 6); ++i)
    {
        const uint8_t *r = sArt + HEADER_BYTES + i * RECORD_BYTES;
        if (r[0] == id) return r;
    }
    return NULL;
}

uint8_t CtrCoopBike_LocalAvatarGraphicsId(uint8_t state, uint8_t original)
{
    uint8_t oldMode = sLocalMode;
    sLocalMode = CTR_COOP_BIKE_NONE;
    if (!CtrCoop_UseCustomGraphicsId()) return original;
    if (state == PLAYER_AVATAR_STATE_MACH_BIKE) sLocalMode = CTR_COOP_BIKE_MACH;
    if (state == PLAYER_AVATAR_STATE_ACRO_BIKE) sLocalMode = CTR_COOP_BIKE_ACRO;
    if (oldMode != sLocalMode) sLastAction = 255;
    return state <= PLAYER_AVATAR_STATE_ACRO_BIKE ? CtrCoop_LocalGraphicsId() : original;
}

uint8_t CtrCoopBike_LocalMode(void) { return sLocalMode; }
uint8_t CtrCoopBike_LocalMovementSequence(void) { return sMovementSequence; }
uint8_t CtrCoopBike_LocalStateFlag(uint8_t original)
{
    if (!CtrCoop_UseCustomGraphicsId() || !sLocalMode) return original;
    return sLocalMode == CTR_COOP_BIKE_MACH ? PLAYER_AVATAR_FLAG_MACH_BIKE : PLAYER_AVATAR_FLAG_ACRO_BIKE;
}

bool CtrCoopBike_SetRemote(uint8_t graphicsId, uint8_t mode)
{
    if (graphicsId >= NUM_OBJ_EVENT_GFX || mode > CTR_COOP_BIKE_ACRO) return false;
    if (sRemoteValid && sRemoteGraphics == graphicsId && sRemoteMode == mode) return false;
    sRemoteGraphics = graphicsId; sRemoteMode = mode; sRemoteValid = true;
    return true;
}

void CtrCoopBike_ClearRemote(void) { sRemoteValid = false; sRemoteMode = 0; }

uint16_t CtrCoopBike_AllocationSize(uint8_t localId, uint8_t movementType, uint16_t original)
{
    if ((localId == LOCALID_PLAYER || localId == LOCALID_COOP_PLAYER || movementType == MOVEMENT_TYPE_PLAYER)
     && original < FRAME_BYTES) return FRAME_BYTES;
    return original;
}

static int Role(uint8_t id, uint8_t localId, uint8_t movementType)
{
    if ((localId == LOCALID_PLAYER || movementType == MOVEMENT_TYPE_PLAYER)
     && CtrCoop_UseCustomGraphicsId() && id == CtrCoop_LocalGraphicsId()) return 0;
    if (localId == LOCALID_COOP_PLAYER && sRemoteValid && id == sRemoteGraphics) return 1;
    return -1;
}

bool CtrCoopBike_UsesOwnPalette(uint8_t id, uint8_t localId, uint8_t movementType)
{
    return Role(id, localId, movementType) >= 0;
}

void CtrCoopBike_MovementStarted(const struct ObjectEvent *object, uint8_t action)
{
    if (!sLocalMode || Role(object->graphicsId, object->localId, object->movementType) != 0) return;
    /* These are persistent face poses. The engine reapplies them when idle;
     * actual hopping actions must increment even when repeated in place. */
    if (action == sLastAction && (action <= MOVEMENT_ACTION_FACE_RIGHT
     || (action >= MOVEMENT_ACTION_ACRO_WHEELIE_FACE_DOWN && action <= MOVEMENT_ACTION_ACRO_WHEELIE_FACE_RIGHT))) return;
    ++sMovementSequence;
    sLastAction = action;
}

const struct ObjectEventGraphicsInfo *CtrCoopBike_GraphicsInfo(uint8_t id, uint8_t localId,
    uint8_t movementType, const struct ObjectEventGraphicsInfo *original)
{
    int role = Role(id, localId, movementType);
    const struct ObjectEventGraphicsInfo *bike;
    const uint8_t *record;
    unsigned mode;
    if (role < 0 || original == NULL) return original;
    mode = role ? sRemoteMode : sLocalMode;
    if (!mode) return CtrCoopWalk_GraphicsInfo(id, original);
    /* Native identities work even when the optional custom pack is damaged. */
    if (id == OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL)
        return GetObjectEventGraphicsInfo(mode == CTR_COOP_BIKE_MACH ? OBJ_EVENT_GFX_RIVAL_BRENDAN_MACH_BIKE : OBJ_EVENT_GFX_RIVAL_BRENDAN_ACRO_BIKE);
    if (id == OBJ_EVENT_GFX_RIVAL_MAY_NORMAL)
        return GetObjectEventGraphicsInfo(mode == CTR_COOP_BIKE_MACH ? OBJ_EVENT_GFX_RIVAL_MAY_MACH_BIKE : OBJ_EVENT_GFX_RIVAL_MAY_ACRO_BIKE);
    bike = GetObjectEventGraphicsInfo(mode == CTR_COOP_BIKE_MACH ? OBJ_EVENT_GFX_BRENDAN_MACH_BIKE : OBJ_EVENT_GFX_BRENDAN_ACRO_BIKE);
    record = Record(id);
    sInfo[role] = record != NULL ? *bike : *original;
    sInfo[role].anims = bike->anims;
    sInfo[role].tracks = bike->tracks;
    /* A corrupt/missing pack keeps the selected person with bounded frames;
     * release packaging refuses missing drawings. Never substitute Brendan. */
    for (unsigned i = 0; i < FRAMES; ++i)
    {
        sFrames[role][i] = original->images[0];
        if (record != NULL)
        {
            sFrames[role][i].data = sArt + Read32(record + 4) + 32 + i * FRAME_BYTES;
            sFrames[role][i].size = FRAME_BYTES;
#ifdef PORT_BRIDGE
            sFrames[role][i].offset = 0;
#endif
        }
    }
    sInfo[role].images = sFrames[role];
    return &sInfo[role];
}

const struct ObjectEventGraphicsInfo *CtrCoopBike_ObjectGraphicsInfo(const struct ObjectEvent *object,
    const struct ObjectEventGraphicsInfo *original)
{
    return CtrCoopBike_GraphicsInfo(object->graphicsId, object->localId, object->movementType, original);
}

void CtrCoopBike_BindSprite(const struct ObjectEvent *object, struct Sprite *sprite)
{
    int role = Role(object->graphicsId, object->localId, object->movementType);
    const uint8_t *record;
    const uint16_t *colors;
    const uint16_t *walkColors;
    uint8_t slot;
    unsigned mode, source;
    if (role < 0) return;
    mode = role ? sRemoteMode : sLocalMode;
    record = mode && object->graphicsId != OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL
                  && object->graphicsId != OBJ_EVENT_GFX_RIVAL_MAY_NORMAL ? Record(object->graphicsId) : NULL;
    walkColors = !mode ? CtrCoopWalk_Palette(object->graphicsId) : NULL;
    source = record != NULL ? 1 : walkColors != NULL ? 2 : 0;
    colors = record != NULL ? (const uint16_t *)(sArt + Read32(record + 4))
             : walkColors != NULL ? walkColors
             : CtrCoopBike_ObjectPalette(GetObjectEventGraphicsInfo(object->graphicsId)->paletteTag);
    if (colors == NULL) return;
    if (sPaletteGraphics[role] != object->graphicsId || sPaletteSource[role] != source)
    {
        FreeSpritePaletteByTag(BIKE_PAL_TAG + role);
        sPaletteGraphics[role] = object->graphicsId;
        sPaletteSource[role] = source;
    }
    slot = IndexOfSpritePaletteExtTag(BIKE_PAL_TAG + role);
    if (slot == 255)
    {
        slot = LoadSpritePaletteExt(&(const struct SpritePalette){colors, BIKE_PAL_TAG + role});
        if (slot == 255) return;
        if (gMain.callback2 == CB2_Overworld) UpdateSpritePaletteExtWithWeather(slot);
    }
    SetSpriteExtPalette(sprite, slot);
}

void CtrCoopBike_Reset(void)
{
    /* Only at soft reset, after the engine has stopped using sprite pointers. */
    CtrCoopWalk_Reset();
    free(sArt); sArt = NULL; sArtSize = 0; sTried = false; sLocalMode = 0;
    sMovementSequence = 0; sLastAction = 255;
    CtrCoopBike_ClearRemote();
    for (unsigned role = 0; role < 2; ++role)
    {
        FreeSpritePaletteByTag(BIKE_PAL_TAG + role);
        sPaletteGraphics[role] = 255;
        sPaletteSource[role] = 255;
        FreeSpritePaletteByTag(BIKE_PAL_TAG + 2 + role);
        sReflectionPaletteTag[role] = 65535;
    }
}

bool CtrCoopBike_BindReflection(const struct ObjectEvent *object,
    const struct Sprite *source, struct Sprite *reflection, uint16_t darkPaletteTag)
{
    uint8_t previous, current;
    int role = Role(object->graphicsId, object->localId, object->movementType);
    const uint16_t *colors;
    if (role < 0) return false;
    previous = GetSpriteExtPalette(reflection);
    current = GetSpriteExtPalette(source);
    colors = darkPaletteTag == 65535 || darkPaletteTag == 0x11FF ? NULL : CtrCoopBike_ObjectPalette(darkPaletteTag);
    if (colors != NULL)
    {
        uint16_t tag = BIKE_PAL_TAG + 2 + role;
        if (sReflectionPaletteTag[role] != darkPaletteTag)
        {
            FreeSpritePaletteByTag(tag);
            sReflectionPaletteTag[role] = darkPaletteTag;
        }
        current = IndexOfSpritePaletteExtTag(tag);
        if (current == 255)
        {
            current = LoadSpritePaletteExt(&(const struct SpritePalette){colors,tag});
            if (current != 255 && gMain.callback2 == CB2_Overworld)
                UpdateSpritePaletteExtWithWeather(current);
        }
    }
    SetSpriteExtPalette(reflection, current);
    return previous != current;
}
