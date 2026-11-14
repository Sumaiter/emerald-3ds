#ifndef CTR_BATTLE_PRESENT_H
#define CTR_BATTLE_PRESENT_H

/* Resolved display state, never a battle-controller command. All members are
 * values; no engine, task, callback, asset or GPU pointer crosses the link.
 * Peers use the same build ABI and little-endian IEEE754 representation. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CTR_PRESENT_MAGIC 0x31525042u /* BPR1 */
#define CTR_PRESENT_VRAM 0x18000u
#define CTR_PRESENT_LINES 240u
#define CTR_PRESENT_LINE_REGS 35u
#define CTR_PRESENT_HUDS 4u
#define CTR_PRESENT_MAP_CELLS 10240u
#define CTR_PRESENT_MAPS 16u
enum { CTR_PRESENT_VOXEL=1, CTR_PRESENT_BLUR=2, CTR_PRESENT_MOVE_BG=4,
       CTR_PRESENT_INTRO=8, CTR_PRESENT_HD=16 };

typedef struct {
    uint16_t layout,id;
    int16_t x,y,lift,reserved;
} CtrPresentMap;
typedef struct {
    uint16_t layout,id;
    int16_t x,y,phaseX,phaseY;
    uint8_t drawable,reserved[3];
} CtrPresentBorder;

typedef struct {
    uint8_t used,kind,gender,level,name[12];
    int16_t hp,maxHp;
    uint8_t hpPixels,hpColour,expPixels,status,ball,safariBalls,numbers,reserved;
    uint16_t boxTile,boxTiles,barTile,barTiles;
    float fade;
    uint32_t fadeColour;
} CtrPresentHud;

typedef struct {
    float camera[14],intro[5],shadows[4][4];
    float light[5],sun[3];
    uint32_t sunKey;
    float weatherAmount,lightning,fog,fade,fadeRgb[3],bloom;
    uint32_t mapHash;
    uint16_t layout;
    uint8_t mapGroup,mapNum,weather,shadowCount,ready,outdoor;
    uint16_t gridWidth,gridHeight;
    uint8_t mapCount,borderCount,underground,connected;
    uint32_t closedEdges;
    CtrPresentMap maps[CTR_PRESENT_MAPS];
    CtrPresentBorder borders[CTR_PRESENT_MAPS];
    uint16_t grid[CTR_PRESENT_MAP_CELLS];
} CtrPresentWorld;

typedef struct {
    uint32_t magic,frame,flags;
    uint16_t lineCount,lineMask,extInUse,lineRegs;
    int16_t boxSlide;
    uint8_t boxHidden,boxDecided,boxBarsUp,blurStrength,borderFollowsEdge,measureGlow;
    CtrPresentHud hud[CTR_PRESENT_HUDS];
    CtrPresentWorld world;
    uint16_t regs[64],palette[512],unfaded[512],oam[512];
    uint32_t anchored[4];
    uint32_t oamMarks[8][4];
    int16_t anchor[128];
    uint16_t extPalette[256];
    uint8_t oamExt[128];
    uint16_t scroll[8][CTR_PRESENT_LINES];
    uint16_t scanlines[160][CTR_PRESENT_LINE_REGS];
    uint8_t vram[CTR_PRESENT_VRAM];
} CtrBattlePresentFrame;

#define CTR_PRESENT_BYTES ((unsigned)sizeof(CtrBattlePresentFrame))
#define CTR_PRESENT_ENCODED_MAX (CTR_PRESENT_BYTES + (CTR_PRESENT_BYTES+127u)/128u)

/* Validation is mandatory before any renderer can borrow a network state. */
bool CtrBattlePresent_Validate(const CtrBattlePresentFrame *frame);
bool CtrBattlePresent_WorldValid(const CtrPresentWorld *world);
size_t CtrBattlePresent_Encode(const uint8_t *state,const uint8_t *base,size_t count,
                             uint8_t *encoded,size_t capacity);
bool CtrBattlePresent_Decode(const uint8_t *encoded,size_t bytes,const uint8_t *base,
                           uint8_t *state,size_t count);
uint32_t CtrBattlePresent_Checksum(const uint8_t *data,size_t bytes);
/* Only continuous presentation quantities interpolate. Asset, text,
 * visibility and status changes remain atomic; palette fades interpolate
 * only with identical tile data. Input frames are immutable. */
void CtrBattlePresent_Interpolate(const CtrBattlePresentFrame *from,
                                const CtrBattlePresentFrame *to,
                                unsigned elapsed,unsigned duration,
                                CtrBattlePresentFrame *out);

/* Renderer ownership. Set only while drawing the private presentation state;
 * hooks use this to read captured values without changing the game's globals. */
const CtrBattlePresentFrame *CtrBattlePresent_Current(void);
void CtrBattlePresent_Bind(const CtrBattlePresentFrame *frame);
/* Read-only engine hook: resolved border placement, never engine pointers. */
unsigned CtrBattlePresent_CaptureBorders(CtrPresentBorder out[CTR_PRESENT_MAPS]);

#endif
