#include <stdio.h>
#include <string.h>
#include "../src/3ds_coop_walk.c"

struct Sprite gSprites[MAX_SPRITES + 1];
static struct ObjectEventGraphicsInfo sOriginal,sShape;
static struct SpriteFrameImage sOriginalFrames[3];
static const union AnimCmd sIdle[]={ANIMCMD_FRAME(0,16),ANIMCMD_JUMP(0)};
static const union AnimCmd *sNativeAnims[ANIM_STD_COUNT];
static const struct OamData sOam={.shape=0,.size=2};
static unsigned sOpen,sErrors,sPaletteLoads,sPaletteFrees,sFallbacks;
static uint8_t sBound;
static unsigned sSize=8+WALK_COUNT*(8+WALK_ART_BYTES);
static uint8_t sFile[8+WALK_COUNT*(8+WALK_ART_BYTES)];
static int sFailures;
#define CHECK(c,m) do {if(!(c)){printf("FAIL: %s\n",m);++sFailures;}} while(0)

const struct ObjectEventGraphicsInfo *GetObjectEventGraphicsInfo(u8 id)
{return id==OBJ_EVENT_GFX_BRENDAN_MACH_BIKE?&sShape:&sOriginal;}
void CtrLog_Write(CtrLogCategory c,const char *f,...) {(void)c;(void)f;++sErrors;}
FILE *CtrFs_OpenAsset(const char *path)
{
    FILE *f=tmpfile(); ++sOpen;
    CHECK(!strcmp(path,"coop/walks.bin"),"loads engine walking asset");
    if(f){fwrite(sFile,1,sSize,f);rewind(f);}return f;
}
void FreeSpritePaletteByTag(u16 tag) {(void)tag;++sPaletteFrees;}
u8 LoadSpritePaletteExt(const struct SpritePalette *palette)
{CHECK(palette->data!=NULL,"preview uses generated palette");++sPaletteLoads;return 6;}
void SetSpriteExtPalette(struct Sprite *sprite,u8 slot) {(void)sprite;sBound=slot;}
void SetSubspriteTables(struct Sprite *sprite,const struct SubspriteTable *tables) {(void)sprite;(void)tables;}
u8 CreateSprite(const struct SpriteTemplate *template,s16 x,s16 y,u8 priority)
{(void)x;(void)y;(void)priority;gSprites[0].images=template->images;gSprites[0].anims=template->anims;return 0;}
u8 CreateObjectGraphicsSprite(u16 id,void (*cb)(struct Sprite *),s16 x,s16 y,u8 priority)
{(void)id;(void)cb;(void)x;(void)y;(void)priority;++sFallbacks;return 1;}
static void Put16(unsigned o,uint16_t v) {sFile[o]=v;sFile[o+1]=v>>8;}
static void Put32(unsigned o,uint32_t v) {for(unsigned i=0;i<4;++i)sFile[o+i]=v>>(i*8);}
static void ResetFixture(void)
{
    CtrCoopWalk_Reset();
    sOpen=sErrors=sPaletteLoads=sPaletteFrees=sFallbacks=0;sBound=255;
    sSize=sizeof(sFile);memset(sFile,0,sizeof(sFile));memcpy(sFile,"CWK1",4);Put16(4,1);Put16(6,WALK_COUNT);
    for(unsigned i=0;i<WALK_COUNT;++i)
    {
        unsigned offset=8+WALK_COUNT*8+i*WALK_ART_BYTES;
        Put16(8+i*8,sWalkIds[i]);Put32(12+i*8,offset);
        memset(sFile+offset,i+1,WALK_ART_BYTES);
    }
    sOriginal=(struct ObjectEventGraphicsInfo){.size=128,.width=16,.height=16,.inanimate=TRUE,.images=sOriginalFrames,.anims=sNativeAnims};
    sShape=(struct ObjectEventGraphicsInfo){.size=512,.width=32,.height=32,.oam=&sOam,.anims=sNativeAnims};
    for(unsigned i=0;i<ANIM_STD_COUNT;++i)sNativeAnims[i]=sIdle;
}

int main(void)
{
    const struct ObjectEventGraphicsInfo *first;
    ResetFixture();
    CHECK(CtrCoopWalk_GraphicsInfo(OBJ_EVENT_GFX_QUINTY_PLUMP,&sOriginal)==&sOriginal,"existing 84 retain native walking tables");
    CHECK(CtrCoopWalk_Palette(OBJ_EVENT_GFX_QUINTY_PLUMP)==NULL && sOpen==0,"unrelated identities do not load pack");
    first=CtrCoopWalk_GraphicsInfo(sWalkIds[0],&sOriginal);
    for(unsigned i=0;i<WALK_COUNT;++i)
    {
        const struct ObjectEventGraphicsInfo *info=CtrCoopWalk_GraphicsInfo(sWalkIds[i],&sOriginal);
        CHECK(info!=&sOriginal && info->width==32 && info->height==32 && info->size==512,"all15 get bounded 32x32 foot geometry");
        CHECK(info->oam==&sOam && !info->inanimate && info->tracks==TRACKS_FOOT,"new walking OAM and footprint data");
        CHECK(*(const uint8_t *)CtrCoopWalk_Palette(sWalkIds[i])==i+1,"each identity owns its new foot palette");
        for(unsigned f=0;f<9;++f)
            CHECK(info->images[f].size==512 && *(const uint8_t *)info->images[f].data==i+1,"every frame uses correct character payload");
        CHECK(info->anims[ANIM_RUN_SOUTH][0].frame.imageValue==3 && info->anims[ANIM_RUN_EAST][0].frame.hFlip,"running and mirrored east stay within nine drawings");
        CHECK(info->anims[ANIM_RUN_SOUTH][0].frame.duration==5 && info->anims[ANIM_RUN_SOUTH][1].frame.duration==3,"native running cadence");
    }
    CHECK(first==CtrCoopWalk_GraphicsInfo(sWalkIds[0],&sOriginal) && sOpen==1,"immutable cached tables survive other identity lookups");
    CHECK(sOriginal.images==sOriginalFrames && sOriginal.inanimate,"native NPC graphics never mutated");
    CHECK(CtrCoopWalk_CreatePreview(sWalkIds[0],NULL,1,2,0)==0 && sBound==6 && sPaletteLoads==1,"selection preview uses new poses and ext palette");
    CHECK(CtrCoopWalk_CreatePreview(OBJ_EVENT_GFX_QUINTY_PLUMP,NULL,1,2,0)==1 && sFallbacks==1,"original previews retain existing path");
    CtrCoopWalk_FreePreviewPalettes();CHECK(sPaletteFrees==WALK_COUNT,"page switch releases all preview palettes");
    ResetFixture();sFile[0]='X';
    CHECK(CtrCoopWalk_GraphicsInfo(sWalkIds[0],&sOriginal)==&sOriginal && sErrors==1,"bad magic falls back safely");
    CHECK(CtrCoopWalk_Palette(sWalkIds[0])==NULL && sOpen==1,"invalid asset logged/attempted once");
    ResetFixture();Put16(8,239);
    CHECK(CtrCoopWalk_Palette(sWalkIds[0])==NULL,"follower/script IDs rejected");
    ResetFixture();Put16(16,sWalkIds[0]);
    CHECK(CtrCoopWalk_Palette(sWalkIds[0])==NULL,"duplicate identities rejected");
    ResetFixture();Put32(12,9);
    CHECK(CtrCoopWalk_Palette(sWalkIds[0])==NULL,"unaligned/aliased payload rejected");
    ResetFixture();--sSize;
    CHECK(CtrCoopWalk_Palette(sWalkIds[0])==NULL,"truncated asset rejected");
    CtrCoopWalk_Reset();
    if(sFailures)return 1;
    puts("walking avatar runtime tests passed");return 0;
}
