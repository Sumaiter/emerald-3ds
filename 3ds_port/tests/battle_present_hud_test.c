#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "3ds_battle_hud.h"
#include "3ds_battle_present.h"
#include "constants/characters.h"
typedef uint16_t u16;typedef uint8_t u8;
#define POKEMON_NAME_LENGTH 10
#define GAME_LANGUAGE 2
#define LANGUAGE_JAPANESE 1
#define HUD_GLYPHS gFontNormalLatinGlyphs
#define CTR_LOG_ERROR 0
static u16 gFontNormalLatinGlyphs[256*32];
static u8 gFontNormalLatinGlyphWidths[256];
static const uint8_t gText_SafariBalls[]={0xbb,0xff},gText_SafariBallLeft[]={0xbc,0xff};
static struct { struct { bool hpNumbersNoBars; } battlerData[4]; } battleSprites,*gBattleSpritesDataPtr=&battleSprites;
static const void *Port_ResolveFontPointer(const void *p) { return p; }
static const void *Port_ResolveAssetPointer(const void *p) { return p; }
static void WarmWait(void) {}
static void CtrLog_Write(int n,const char *s) { (void)n;(void)s; }
static unsigned GetExtCtrlCodeLength(unsigned c) { (void)c;return 1; }
static uint8_t *StringCopy(uint8_t *dst,const uint8_t *src) { while((*dst=*src)!=0xff) { ++dst;++src; }return dst; }
const uint8_t *CtrBattleHud_ElementGfx(unsigned char element) { static uint8_t tile[32];memset(tile,element==1?0x77:0x22,32);return tile; }
static FILE *ArtOpen(const char *path,const char *mode) {
    assert(!strcmp(path,"romfs:/battle/hud.bin"));return fopen("build/upstream/3ds_port/romfs/battle/hud.bin",mode);
}
#define fopen ArtOpen
#include "present_hud.inc"
#undef fopen
int main(void)
{
    for(unsigned g=0;g<256;++g) {
        gFontNormalLatinGlyphWidths[g]=4+g%4;
        for(unsigned i=0;i<32;++i)gFontNormalLatinGlyphs[g*32+i]=(g+i)%2?0x5555:0x1111;
    }
    memset(sSlots,0x5a,sizeof(sSlots));memset(sBoxes,0,sizeof(sBoxes));memset(sPixels,0xa5,sizeof(sPixels));
    Slot slots[4];memcpy(slots,sSlots,sizeof(slots));
    static CtrBattlePresentFrame p;
    p.magic=CTR_PRESENT_MAGIC;p.frame=1;p.flags=CTR_PRESENT_HD;
    p.hud[0]=(CtrPresentHud){.used=1,.kind=CTR_HUD_SINGLE_PLAYER,.gender=2,.level=17,.hp=50,.maxHp=50,.hpPixels=48,.boxTile=512,.boxTiles=128};
    p.hud[0].name[0]=0xbb;p.hud[0].name[1]=0xff;
    assert(CtrBattlePresent_Validate(&p));CtrBattlePresent_Bind(&p);CtrBattleHud_PresentReset();
    const CtrHudBox *box=PresentBoxes(&p);assert(box && box[0].pixels && box[0].used);
    static uint32_t full[CTR_HUD_W*CTR_HUD_H];
    memcpy(full,box[0].pixels,sizeof(full));unsigned version=box[0].version;
    CtrBattleHud_Uploaded(0);p.hud[0].fade=.5f;PresentBoxes(&p);assert(box[0].version==version && box[0].fade==.5f);
    clock_t start=clock();
    for(unsigned i=0;i<100;++i) {
        p.hud[0].hp=50-i%50;p.hud[0].hpPixels=48-i%40;PresentBoxes(&p);CtrBattleHud_Uploaded(0);
    }
    printf("HD HUD actual composition: %.3f ms/HP update on host\n",1000.0*(clock()-start)/CLOCKS_PER_SEC/100);
    assert(memcmp(full,box[0].pixels,sizeof(full)) && !memcmp(full,box[0].pixels,16*CTR_HUD_W*4));
    /* Partial redraw must be byte-identical to composing all rows. */
    Slot expected={0};expected.kind=p.hud[0].kind;expected.gender=2;expected.level=17;expected.hp=p.hud[0].hp;expected.maxHp=50;expected.hpPixels=p.hud[0].hpPixels;
    memcpy(expected.name,p.hud[0].name,sizeof(expected.name));
    ComposeValues(&expected,full,false,(const u16*)(p.vram+0x14000),0,CTR_HUD_H);
    assert(!memcmp(full,box[0].pixels,sizeof(full)));
    p.hud[0].status=3;PresentBoxes(&p);assert(memcmp(full,box[0].pixels,sizeof(full)));
    assert(!memcmp(slots,sSlots,sizeof(slots)) && sPixels[0][0]==0xa5a5a5a5 && !sBoxes[0].used);
    CtrBattleHud_PresentReset();assert(!sPresentBoxes[0].used);CtrBattlePresent_Bind(NULL);
    for(unsigned i=0;i<ART_COUNT;++i)free((void*)sArt[i].pixels);
    puts("HD HUD private composition: original art/text/HP/status, fade-only no upload, partial rows equal full image, local slots unchanged");
    return 0;
}
