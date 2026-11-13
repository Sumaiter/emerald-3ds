/* Local co-op avatar backs. The engine asset contains only newly drawn art
 * and numeric references to the game's eight native back pictures. */
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "battle.h"
#include "sprite.h"
#include "palette.h"
#include "3ds_coop.h"
#include "3ds_coop_battle.h"
#include "3ds_platform.h"

#define BACK_BYTES 2048
#define PALETTE_BYTES 32
#define RECORD_BYTES 8
#define HEADER_BYTES 8
#define CUSTOM_BACK 255

static uint8_t *sArt;
static uint32_t sArtSize;
static bool sTried;
static struct SpriteFrameImage sFrames[5];

static uint16_t Read16(const uint8_t *p)
{
    return p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t Read32(const uint8_t *p)
{
    return Read16(p) | ((uint32_t)Read16(p + 2) << 16);
}

static bool LoadArt(void)
{
    FILE *file;
    long size;
    unsigned count;
    bool seen[256] = {false};

    if (sTried)
        return sArt != NULL;
    sTried = true;
    file = CtrFs_OpenAsset("coop/backpics.bin");
    if (file == NULL)
        goto bad;
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < HEADER_BYTES
     || size > HEADER_BYTES + 256 * (RECORD_BYTES + PALETTE_BYTES + BACK_BYTES)
     || fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        goto bad;
    }
    sArt = malloc(size);
    if (sArt == NULL)
    {
        fclose(file);
        goto bad;
    }
    sArtSize = size;
    if (fread(sArt, 1, size, file) != (size_t)size)
    {
        fclose(file);
        goto bad;
    }
    fclose(file);
    if (memcmp(sArt, "CBK1", 4) != 0 || Read16(sArt + 4) != 1)
        goto bad;
    count = Read16(sArt + 6);
    if (count == 0 || count > 256 || HEADER_BYTES + count * RECORD_BYTES > sArtSize)
        goto bad;
    for (unsigned i = 0; i < count; ++i)
    {
        const uint8_t *record = sArt + HEADER_BYTES + i * RECORD_BYTES;
        unsigned id = Read16(record), native = record[2];
        uint32_t offset = Read32(record + 4);
        if (id >= 256 || seen[id] || record[3] != 0)
            goto bad;
        seen[id] = true;
        if (native == CUSTOM_BACK)
        {
            if (offset < HEADER_BYTES + count * RECORD_BYTES || (offset & 3) != 0
             || offset > sArtSize || PALETTE_BYTES + BACK_BYTES > sArtSize - offset)
                goto bad;
        }
        else if (native > 7 || offset != 0)
            goto bad;
    }
    return true;

bad:
    free(sArt);
    sArt = NULL;
    sArtSize = 0;
    CtrLog_Write(CTR_LOG_ERROR, "coop: missing or invalid battle avatar art");
    return false;
}

static const uint8_t *LocalRecord(void)
{
    /* Link opponents, recordings and Wally's scripted tutorial retain their
     * own identities. Host and guest otherwise use their own profile slot. */
    if (!CtrCoop_UseCustomGraphicsId()
     || (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED | BATTLE_TYPE_WALLY_TUTORIAL))
     || !LoadArt())
        return NULL;
    for (unsigned i = 0; i < Read16(sArt + 6); ++i)
    {
        const uint8_t *record = sArt + HEADER_BYTES + i * RECORD_BYTES;
        if (Read16(record) == CtrCoop_LocalGraphicsId())
            return record;
    }
    return NULL;
}

uint16_t CtrCoopBattle_BackPicId(uint16_t original)
{
    const uint8_t *record = LocalRecord();
    return record != NULL && record[2] != CUSTOM_BACK ? record[2] : original;
}

bool CtrCoopBattle_Active(void)
{
    return LocalRecord() != NULL;
}

void CtrCoopBattle_LoadPalette(uint8_t paletteNum)
{
    const uint8_t *record = LocalRecord();
    if (record != NULL && record[2] == CUSTOM_BACK && paletteNum < 16)
        LoadPalette(sArt + Read32(record + 4), OBJ_PLTT_ID(paletteNum), PALETTE_BYTES);
}

void CtrCoopBattle_Apply(uint8_t battler, struct SpriteTemplate *template)
{
    const uint8_t *record = LocalRecord();
    const uint8_t *pixels;

    if (record == NULL || record[2] != CUSTOM_BACK || battler >= MAX_BATTLERS_COUNT || template == NULL)
        return;
    pixels = sArt + Read32(record + 4);
    CtrCoopBattle_LoadPalette(battler);
    for (unsigned i = 0; i < ARRAY_COUNT(sFrames); ++i)
    {
        sFrames[i].data = pixels + PALETTE_BYTES;
        sFrames[i].size = BACK_BYTES;
#ifdef PORT_BRIDGE
        sFrames[i].offset = 0;
#endif
    }
    /* The game's throw/slide animation can request frames 0..4. Repeating
     * the static drawing keeps every frame valid without reading past it. */
    template->images = sFrames;
}
