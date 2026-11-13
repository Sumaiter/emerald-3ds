#include <stdio.h>
#include <string.h>
#include "../src/3ds_coop_bike.c"

/* The walk loader has its own malformed-pack/runtime suite. These probes
 * exercise the player-only handoff and palette-source transition here. */
static bool sWalkProbe;
static struct ObjectEventGraphicsInfo sWalkProbeInfo;
static const uint16_t sWalkProbeColors[16] = {0x7777};
const struct ObjectEventGraphicsInfo *CtrCoopWalk_GraphicsInfo(uint8_t id,
    const struct ObjectEventGraphicsInfo *original)
{ (void)id; return sWalkProbe ? &sWalkProbeInfo : original; }
const uint16_t *CtrCoopWalk_Palette(uint8_t id)
{ (void)id; return sWalkProbe ? sWalkProbeColors : NULL; }
void CtrCoopWalk_Reset(void) { sWalkProbe=false; }

struct Main gMain;
static bool sAllowed = true;
static uint8_t sIdentity = 4;
static unsigned sLoads, sErrors, sWeather, sFrees;
static uint8_t sSlots[4] = {255,255,255,255}, sBound;
static uint8_t sExpectedPalette[4] = {0x11,0x22,0x66,0x66}, sLoadedByte[4];
static const uint16_t sNativeColors[4][16] = {{0x3333},{0x4444},{0x5555},{0x6666}};
static struct Sprite *sReflection;
static uint8_t sSourcePalette = 255, sReflectionPalette = 255;
static uint8_t sFile[8 + 4*12 + 2*ART_BYTES];
static unsigned sFileSize;
static struct ObjectEventGraphicsInfo sBase, sMach, sAcro;
static struct ObjectEventGraphicsInfo sNativeBase[2];
static struct SpriteFrameImage sBaseImages[3];
static int sFailures;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); ++sFailures; } } while (0)

bool CtrCoop_UseCustomGraphicsId(void) { return sAllowed; }
uint8_t CtrCoop_LocalGraphicsId(void) { return sIdentity; }
void CB2_Overworld(void) { }
void CtrLog_Write(CtrLogCategory c, const char *f, ...) { (void)c; (void)f; ++sErrors; }
const struct ObjectEventGraphicsInfo *GetObjectEventGraphicsInfo(u8 id)
{
    if (id == OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL) return &sNativeBase[0];
    if (id == OBJ_EVENT_GFX_RIVAL_MAY_NORMAL) return &sNativeBase[1];
    if (id == OBJ_EVENT_GFX_BRENDAN_MACH_BIKE || id == OBJ_EVENT_GFX_RIVAL_BRENDAN_MACH_BIKE || id == OBJ_EVENT_GFX_RIVAL_MAY_MACH_BIKE) return &sMach;
    if (id == OBJ_EVENT_GFX_BRENDAN_ACRO_BIKE || id == OBJ_EVENT_GFX_RIVAL_BRENDAN_ACRO_BIKE || id == OBJ_EVENT_GFX_RIVAL_MAY_ACRO_BIKE) return &sAcro;
    return &sBase;
}
const uint16_t *CtrCoopBike_ObjectPalette(uint16_t tag)
{
    if (tag >= 0x500 && tag <= 0x503) return sNativeColors[tag-0x500];
    return NULL;
}
u8 IndexOfSpritePaletteExtTag(u16 tag) { return sSlots[tag-BIKE_PAL_TAG]; }
u8 LoadSpritePaletteExt(const struct SpritePalette *palette)
{
    unsigned role = palette->tag - BIKE_PAL_TAG;
    CHECK(role < 4, "independent player and bridge reflection palette tags");
    sLoadedByte[role]=*(const u8 *)palette->data;
    CHECK(sLoadedByte[role] == sExpectedPalette[role], "palette belongs to selected host or guest rider");
    ++sLoads;
    return sSlots[role] = role + 2;
}
void FreeSpritePaletteByTag(u16 tag) { sSlots[tag-BIKE_PAL_TAG] = 255; ++sFrees; }
void SetSpriteExtPalette(struct Sprite *sprite, u8 slot) { sBound = slot; if (sprite == sReflection) sReflectionPalette = slot; }
u8 GetSpriteExtPalette(const struct Sprite *sprite) { return sprite == sReflection ? sReflectionPalette : sSourcePalette; }
void UpdateSpritePaletteExtWithWeather(u8 slot) { (void)slot; ++sWeather; }
FILE *CtrFs_OpenAsset(const char *path)
{
    FILE *f = tmpfile();
    CHECK(strcmp(path,"coop/bikes.bin")==0,"loads engine bicycle art");
    if (f) { fwrite(sFile,1,sFileSize,f); rewind(f); }
    return f;
}
static void Put32(unsigned offset, uint32_t value)
{
    for(unsigned i=0;i<4;++i) sFile[offset+i]=value>>(8*i);
}
static void Reset(void)
{
    CtrCoopBike_Reset();
    sAllowed=true; sIdentity=4; sLoads=sErrors=sWeather=sFrees=0;
    memset(sFile,0,sizeof(sFile));
    memcpy(sFile,"CBI1\1\0\4\0",8);
    sFile[8]=4; sFile[9]=sFile[10]=255; Put32(12,56);
    sFile[20]=5; sFile[21]=sFile[22]=255; Put32(24,56+ART_BYTES);
    sFile[32]=100; sFile[33]=101; sFile[34]=102;
    sFile[44]=105; sFile[45]=106; sFile[46]=107;
    memset(sFile+56,0x11,ART_BYTES);
    memset(sFile+56+ART_BYTES,0x22,ART_BYTES);
    sFileSize=sizeof(sFile);
    sBase.size=128; sBase.width=sBase.height=16; sBase.images=sBaseImages;
    sBase.paletteTag=0x500;
    sNativeBase[0]=sNativeBase[1]=sBase;
    sNativeBase[0].paletteTag=0x501; sNativeBase[1].paletteTag=0x502;
    sNativeBase[0].paletteSlot=sNativeBase[1].paletteSlot=10;
    sExpectedPalette[0]=0x11; sExpectedPalette[1]=0x22;
    sBaseImages[0].data=(const void *)"base"; sBaseImages[0].size=128;
    sMach.size=sAcro.size=512; sMach.width=sAcro.width=sMach.height=sAcro.height=32;
    sMach.tracks=sAcro.tracks=2;
    sMach.anims=(const void *)"mach"; sAcro.anims=(const void *)"acro";
    gMain.callback2=CB2_Overworld;
}

int main(void)
{
    struct ObjectEvent local={0}, remote={0}, npc={0};
    struct Sprite sprite={0};
    struct Sprite reflection={0};
    const struct ObjectEventGraphicsInfo *a,*b;
    local.localId=LOCALID_PLAYER; local.movementType=MOVEMENT_TYPE_PLAYER; local.graphicsId=4;
    remote.localId=LOCALID_COOP_PLAYER; remote.graphicsId=5;
    npc.localId=7; npc.graphicsId=4;
    Reset();
    CHECK(CtrCoopBike_LocalAvatarGraphicsId(PLAYER_AVATAR_STATE_MACH_BIKE,1)==4,"mounting retains chosen host identity");
    CHECK(CtrCoopBike_LocalMode()==CTR_COOP_BIKE_MACH,"Mach mode separate from identity");
    CHECK(CtrCoopBike_SetRemote(5,CTR_COOP_BIKE_ACRO),"accepts guest's independent bike selection");
    CHECK(!CtrCoopBike_SetRemote(5,CTR_COOP_BIKE_ACRO),"identical remote context does not restart animation");
    CHECK(!CtrCoopBike_SetRemote(239,CTR_COOP_BIKE_MACH) && sRemoteGraphics==5 && sRemoteMode==CTR_COOP_BIKE_ACRO,
          "invalid remote context cannot mutate current identity or mode");
    a=CtrCoopBike_ObjectGraphicsInfo(&local,&sBase);
    b=CtrCoopBike_ObjectGraphicsInfo(&remote,&sBase);
    CHECK(a!=b && a->anims==sMach.anims && b->anims==sAcro.anims,"independent host Mach/guest Acro animation tables");
    CHECK(a->width==32 && a->height==32 && a->tracks==2,"bike geometry and tire tracks");
    for(unsigned i=0;i<27;++i)
    {
        CHECK(a->images[i].size==512 && *(const u8 *)a->images[i].data==0x11,"all host wheelie/pedal indices are bounded");
        CHECK(b->images[i].size==512 && *(const u8 *)b->images[i].data==0x22,"all guest wheelie/pedal indices are independent");
        CHECK(a->images[i].offset==0 && b->images[i].offset==0,"RAM images have no asset relocation offset");
    }
    CHECK(CtrCoopBike_ObjectGraphicsInfo(&npc,&sBase)==&sBase,"same-looking NPC remains on foot");
    CHECK(CtrCoopBike_UsesOwnPalette(4,LOCALID_PLAYER,MOVEMENT_TYPE_PLAYER)
       && CtrCoopBike_UsesOwnPalette(5,LOCALID_COOP_PLAYER,0)
       && !CtrCoopBike_UsesOwnPalette(4,7,0),"only actual players skip writes to legacy shared NPC palettes");
    CHECK(CtrCoopBike_AllocationSize(LOCALID_PLAYER,0,128)==512 && CtrCoopBike_AllocationSize(LOCALID_COOP_PLAYER,0,256)==512,"small riders reserve enough tiles before mounting");
    CHECK(CtrCoopBike_AllocationSize(7,0,128)==128,"NPC allocation unchanged");
    CtrCoopBike_BindSprite(&local,&sprite); CHECK(sBound==2,"host ext palette");
    CtrCoopBike_BindSprite(&remote,&sprite); CHECK(sBound==3 && sLoads==2 && sWeather==2,"guest ext palette and weather isolated");
    CtrCoopBike_BindSprite(&local,&sprite); CHECK(sLoads==2,"unchanged palette reused");
    sReflection=&reflection; sSourcePalette=2; sReflectionPalette=255;
    CHECK(CtrCoopBike_BindReflection(&local,&sprite,&reflection,65535) && sReflectionPalette==2,"reflection takes host bicycle colors");
    CHECK(!CtrCoopBike_BindReflection(&local,&sprite,&reflection,65535),"unchanged reflection palette does not reload");
    sSourcePalette=3;
    CHECK(CtrCoopBike_BindReflection(&remote,&sprite,&reflection,65535) && sReflectionPalette==3,"reflection takes guest's independent colors");
    sSourcePalette=255;
    CHECK(CtrCoopBike_BindReflection(&local,&sprite,&reflection,65535) && sReflectionPalette==255,"a cleared source palette clears stale reflection colors");
    sSourcePalette=7;
    CHECK(!CtrCoopBike_BindReflection(&npc,&sprite,&reflection,65535) && sReflectionPalette==255,"unrelated field effects keep their palettes");
    sSlots[0]=255; gMain.callback2=NULL;
    CtrCoopBike_BindSprite(&local,&sprite); CHECK(sLoads==3 && sWeather==2,"menu-return palette reload leaves fade to engine");
    CtrCoopBike_MovementStarted(&local,MOVEMENT_ACTION_FACE_DOWN);
    CtrCoopBike_MovementStarted(&local,MOVEMENT_ACTION_FACE_DOWN);
    CHECK(CtrCoopBike_LocalMovementSequence()==1,"unchanged idle face does not spam sequence");
    CtrCoopBike_LocalAvatarGraphicsId(PLAYER_AVATAR_STATE_ACRO_BIKE,63);
    CtrCoopBike_MovementStarted(&local,MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_DOWN);
    CtrCoopBike_MovementStarted(&local,MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_DOWN);
    CHECK(CtrCoopBike_LocalMovementSequence()==3,"repeated in-place hop starts are distinguishable");
    CtrCoopBike_MovementStarted(&remote,MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_DOWN);
    CHECK(CtrCoopBike_LocalMovementSequence()==3,"remote actions cannot increment local sequence");
    for(unsigned i=0;i<256;++i) CtrCoopBike_MovementStarted(&local,MOVEMENT_ACTION_ACRO_WHEELIE_HOP_FACE_DOWN);
    CHECK(CtrCoopBike_LocalMovementSequence()==3,"sequence wraps in one byte");
    CHECK(CtrCoopBike_LocalStateFlag(1)==PLAYER_AVATAR_FLAG_ACRO_BIKE,"field recreation preserves current bike mode");
    CHECK(CtrCoopBike_LocalAvatarGraphicsId(PLAYER_AVATAR_STATE_NORMAL,0)==4,"dismount returns to own selected identity");
    sExpectedPalette[0]=0x33;
    CtrCoopBike_BindSprite(&local,&sprite);
    CHECK(sBound==2 && sLoadedByte[0]==0x33 && CtrCoopBike_ObjectGraphicsInfo(&local,&sBase)==&sBase,
          "dismount replaces generated palette with original colors in an independent slot");
    CHECK(CtrCoopBike_ObjectGraphicsInfo(&remote,&sBase)!=&sBase,"host dismount does not affect guest bike");
    sFrees=0; CtrCoopBike_ClearRemote();
    CHECK(sFrees==0 && sArt!=NULL && CtrCoopBike_ObjectGraphicsInfo(&remote,&sBase)==&sBase,"disconnect only clears context; no sprite access or image frees");
    CHECK(!CtrCoopBike_SetRemote(239,1) && !CtrCoopBike_SetRemote(4,3),"invalid modes and follower identity rejected");
    sIdentity=100; local.graphicsId=100;
    CtrCoopBike_LocalAvatarGraphicsId(PLAYER_AVATAR_STATE_ACRO_BIKE,0);
    CHECK(CtrCoopBike_ObjectGraphicsInfo(&local,&sBase)==&sAcro,"native Brendan preserves original Acro animation");
    sExpectedPalette[0]=0x44; CtrCoopBike_BindSprite(&local,&sprite);
    CHECK(sBound==2 && sLoadedByte[0]==0x44,"native host uses its original palette asset in its own ext slot");
    remote.graphicsId=105; CtrCoopBike_SetRemote(105,CTR_COOP_BIKE_MACH);
    sExpectedPalette[1]=0x55; CtrCoopBike_BindSprite(&remote,&sprite);
    CHECK(sBound==3 && sLoadedByte[0]==0x44 && sLoadedByte[1]==0x55,
          "two native riders sharing legacy slot10 retain independent colors");
    sSourcePalette=2; sReflectionPalette=2;
    CHECK(CtrCoopBike_BindReflection(&local,&sprite,&reflection,0x503) && sReflectionPalette==4 && sLoadedByte[2]==0x66,
          "high bridge reflection retains native dark-blue palette in an isolated slot");
    CHECK(CtrCoopBike_BindReflection(&local,&sprite,&reflection,65535) && sReflectionPalette==2,
          "leaving high bridge restores regular source colors");
    sIdentity=105; local.graphicsId=105;
    CtrCoopBike_LocalAvatarGraphicsId(PLAYER_AVATAR_STATE_MACH_BIKE,0);
    CHECK(CtrCoopBike_ObjectGraphicsInfo(&local,&sBase)==&sMach,"native May preserves original Mach animation");
    sAllowed=false;
    CHECK(CtrCoopBike_LocalAvatarGraphicsId(PLAYER_AVATAR_STATE_MACH_BIKE,1)==1,"unauthenticated guest uses engine graphics");
    Reset(); local.graphicsId=4;
    Put32(12,sizeof(sFile)-1);
    CtrCoopBike_LocalAvatarGraphicsId(PLAYER_AVATAR_STATE_ACRO_BIKE,0);
    a=CtrCoopBike_ObjectGraphicsInfo(&local,&sBase);
    CHECK(sErrors==1 && a->width==16 && a->anims==sAcro.anims,"corrupt pack retains selected person with bounded Acro table");
    for(unsigned i=0;i<27;++i) CHECK(a->images[i].data==sBaseImages[0].data,"corrupt pack cannot read outside base art");
    Reset(); sFile[20]=4;
    CHECK(!LoadArt(),"duplicate identity rejected");
    Reset(); sFile[33]=103;
    CHECK(!LoadArt(),"wrong native mapping rejected");
    Reset(); local.graphicsId=4; remote.graphicsId=5;
    sWalkProbe=true;
    CtrCoopBike_LocalAvatarGraphicsId(PLAYER_AVATAR_STATE_NORMAL,0);
    CtrCoopBike_SetRemote(5,CTR_COOP_BIKE_NONE);
    CHECK(CtrCoopBike_ObjectGraphicsInfo(&local,&sBase)==&sWalkProbeInfo
       && CtrCoopBike_ObjectGraphicsInfo(&remote,&sBase)==&sWalkProbeInfo,
          "new foot drawings are resolved for both selected player roles");
    CHECK(CtrCoopBike_ObjectGraphicsInfo(&npc,&sBase)==&sBase,
          "same-ID NPC bypasses new foot drawings");
    sExpectedPalette[0]=sExpectedPalette[1]=0x77;
    CtrCoopBike_BindSprite(&local,&sprite); CtrCoopBike_BindSprite(&remote,&sprite);
    CHECK(sLoadedByte[0]==0x77 && sLoadedByte[1]==0x77,"generated foot colors use independent host/guest palettes");
    CtrCoopBike_LocalAvatarGraphicsId(PLAYER_AVATAR_STATE_ACRO_BIKE,0);
    sExpectedPalette[0]=0x11; CtrCoopBike_BindSprite(&local,&sprite);
    CHECK(sLoadedByte[0]==0x11 && CtrCoopBike_ObjectGraphicsInfo(&local,&sBase)!=&sWalkProbeInfo,
          "mounting switches from new foot art to 27-frame bike art");
    CtrCoopBike_LocalAvatarGraphicsId(PLAYER_AVATAR_STATE_NORMAL,0);
    sExpectedPalette[0]=0x77; CtrCoopBike_BindSprite(&local,&sprite);
    CHECK(sLoadedByte[0]==0x77 && CtrCoopBike_ObjectGraphicsInfo(&local,&sBase)==&sWalkProbeInfo,
          "dismount restores new foot drawings and their palette");
    CtrCoopBike_Reset();
    if(sFailures) return 1;
    puts("co-op bicycle tests passed");
    return 0;
}
