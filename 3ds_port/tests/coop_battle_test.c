#include <stdio.h>
#include <string.h>
#include "../src/3ds_coop_battle.c"

u32 gBattleTypeFlags;
static uint8_t sHostGraphics = 12, sGuestGraphics = 13;
static bool sGuest, sAllowed = true;
static unsigned sPaletteLoads, sErrors;
static uint16_t sPaletteSlot;
static uint8_t sPaletteFirst;
static uint8_t sFile[8 + 3 * 8 + 2 * 2080];
static unsigned sFileSize;
static int sFailures;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); ++sFailures; } } while (0)

bool CtrCoop_UseCustomGraphicsId(void) { return sAllowed; }
uint8_t CtrCoop_LocalGraphicsId(void) { return sGuest ? sGuestGraphics : sHostGraphics; }
void CtrLog_Write(CtrLogCategory c, const char *f, ...) { (void)c; (void)f; ++sErrors; }
void LoadPalette(const void *p, u16 offset, u16 size)
{
    CHECK(size == 32, "one 16-color palette");
    sPaletteSlot = offset;
    sPaletteFirst = *(const uint8_t *)p;
    ++sPaletteLoads;
}
FILE *CtrFs_OpenAsset(const char *path)
{
    FILE *file = tmpfile();
    CHECK(strcmp(path, "coop/backpics.bin") == 0, "loads the engine's generated avatar art");
    if (file != NULL) { fwrite(sFile, 1, sFileSize, file); rewind(file); }
    return file;
}
static void Put32(unsigned offset, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) sFile[offset + i] = value >> (8 * i);
}
static void Reset(void)
{
    free(sArt);
    sArt = NULL;
    sTried = false;
    sGuest = false;
    sAllowed = true;
    gBattleTypeFlags = 0;
    sPaletteLoads = sErrors = 0;
    memset(sFile, 0, sizeof(sFile));
    memcpy(sFile, "CBK1\1\0\3\0", 8);
    sFile[8] = 11; sFile[10] = 2;  // Red, native animation
    sFile[16] = 12; sFile[18] = 255; Put32(20, 32);
    sFile[24] = 13; sFile[26] = 255; Put32(28, 32 + 2080);
    memset(sFile + 32, 0x11, 2080);
    memset(sFile + 32 + 2080, 0x22, 2080);
    sFileSize = sizeof(sFile);
}

int main(void)
{
    struct SpriteTemplate template = {0};
    Reset();
    CHECK(CtrCoopBattle_Active(), "local host avatar enabled");
    CtrCoopBattle_Apply(0, &template);
    CHECK(sPaletteLoads == 1 && sPaletteSlot == OBJ_PLTT_ID(0) && sPaletteFirst == 0x11,
          "host uses host profile palette");
    for (unsigned i = 0; i < 5; ++i)
    {
        CHECK(template.images[i].size == 2048 && *(const uint8_t *)template.images[i].data == 0x11,
              "all five animation indices have valid host image data");
        CHECK(template.images[i].offset == 0, "RAM frame has no asset relocation offset");
    }
    sGuest = true;
    CtrCoopBattle_Apply(2, &template);
    CHECK(sPaletteSlot == OBJ_PLTT_ID(2) && sPaletteFirst == 0x22
       && *(const uint8_t *)template.images[4].data == 0x22, "guest uses its own selected avatar");
    CtrCoopBattle_LoadPalette(9);
    CHECK(sPaletteSlot == OBJ_PLTT_ID(9) && sPaletteFirst == 0x22,
          "throw animation retains the guest palette in its newly allocated slot");
    CtrCoopBattle_LoadPalette(255);
    CHECK(sPaletteLoads == 3, "failed palette allocation cannot overwrite palette memory");
    sHostGraphics = 11; sGuest = false;
    CHECK(CtrCoopBattle_BackPicId(0) == 2, "native Red retains its original back-picture ID");
    template.images = NULL;
    CtrCoopBattle_Apply(0, &template);
    CHECK(template.images == NULL && sPaletteLoads == 3, "native animation and palette remain untouched");
    for (unsigned flags = 0; flags < 3; ++flags)
    {
        gBattleTypeFlags = (u32[]){BATTLE_TYPE_LINK, BATTLE_TYPE_RECORDED, BATTLE_TYPE_WALLY_TUTORIAL}[flags];
        CHECK(!CtrCoopBattle_Active() && CtrCoopBattle_BackPicId(0) == 0,
              "link, recording and tutorial identities remain unchanged");
    }
    gBattleTypeFlags = 0;
    sAllowed = false;
    CHECK(!CtrCoopBattle_Active(), "unauthenticated guest cannot use host graphics");
    Reset();
    Put32(20, sizeof(sFile) - 1);
    CHECK(!CtrCoopBattle_Active() && sErrors == 1, "truncated frame rejected safely");
    CHECK(!CtrCoopBattle_Active() && sErrors == 1, "invalid pack is not repeatedly loaded");
    Reset();
    sFile[24] = sFile[16];
    CHECK(!CtrCoopBattle_Active(), "duplicate graphics IDs rejected");
    Reset();
    sFile[10] = 8;
    CHECK(!CtrCoopBattle_Active(), "invalid native table index rejected");
    free(sArt);
    if (sFailures) return 1;
    puts("co-op battle avatar tests passed");
    return 0;
}
