#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "3ds_video.h"
#include "3ds_coop_spectator.h"
#include "3ds_battle_present.h"
#define DECLARE(P) \
void P##SetState(uint16_t,bool,bool,bool,bool);bool P##Request(void); \
void P##Reset(void);void P##Stop(void);void P##Tick(void);bool P##HasFrame(void); \
bool P##IsWatching(void);bool P##HasViewer(void); \
bool P##HandlePacket(uint16_t,const uint8_t*,uint16_t);bool P##InputGate(uint16_t*,uint16_t*);
DECLARE(A)
DECLARE(B)

/* Only platform/GPU boundaries are probes. The playback, capture, binding,
 * interpolation, OAM placement and both endpoints below are real bodies. */
#define LINE_MAX 240
#define CTR_EXT_PALETTES 16
#define CTR_LOG_ERROR 1
static unsigned now,fence,park,releases,resetHud,writeNotice,admitted,localFrames,movingFrames;
static CtrVideoMemory sMemory;
static const uint16_t (*sLineRegs)[CTR_LINE_REGS];
static uint16_t sLineScroll[8][LINE_MAX],sExtPltt[256],gPlttBufferUnfaded[512],sPalette[512],sBoxEntries[104];
static uint32_t sOamAnchored[4],sVoxelWeatherOam[4],sVoxelCloudOam[4],sScreenOam[4],sMachineOam[4],sFlyOam[4],sFogOam[4],sLatticeOam[4],sParticleOam[4],sHash[256];
static int16_t sOamAnchor[128];
static uint8_t sOamExt[128];
static unsigned sLineMask,sLineCount,sExtInUse,sCentredRequested,sTileBanksLive,sUsed,sCacheGeneration,sBoxBattleFrames;
static bool sStageRequested,sStageFadedRequested,sStageZoomedRequested,sBattleRequested,sTransitionRequested,sHoldTop,sFieldBanner,sInBattle,sOamExtMarked;
static bool sSceneCached,sSceneEmpty,sSceneFailed,sBandsFailed,sBoxDecided,sBoxHidden,sBoxBarsUp,sBoxText,sHudCrisp,sBattle;
static int sBoxSlide;
static const void *sHud,*sSceneOn;
static struct { bool on,held; } sEdgeStore;
static struct { uint32_t frames; } sStats;
static CtrBattlePresentFrame source,own,first,last;
static unsigned order;
static void C3D_FrameSync(void) { ++fence;order=1; }
static void CtrVoxel_EndBattle(void) { assert(order==1);order=2; }
static void CtrVoxel_ParkVram(void) { assert(order==2);order=3;++park; }
static void Release(void) { assert(order==3);++releases; }
#define StereoSurfaceRelease Release
#define BandsRelease Release
#define LayersRelease Release
#define SceneRelease Release
#define SlotsRelease Release
#define SceneBlendRelease Release
#define ObjWindowRelease Release
#define HudRelease Release
static void BlendForget(void) {}
static void CtrBattleHud_PresentReset(void) { ++resetHud; }
static void CtrBattleHud_Prepare(void) {}
static void CtrBattleHud_PresentCapture(CtrPresentHud *p) { memcpy(p,source.hud,sizeof(source.hud)); }
static void CtrVoxel_PresentCapture(CtrPresentWorld *p) { *p=source.world; }
static bool CtrSettings_Voxel(void) { return (source.flags&CTR_PRESENT_VOXEL)!=0; }
static bool CtrSettings_VoxelBattle(void) { return CtrSettings_Voxel(); }
static bool CtrSettings_VoxelBlur(void) { return (source.flags&CTR_PRESENT_BLUR)!=0; }
static int CtrSettings_VoxelBlurStrength(void) { return source.blurStrength; }
static int CtrSettings_GetInt(const char *name,int fallback) { (void)fallback;return !strcmp(name,"measure_glow")?source.measureGlow:source.borderFollowsEdge; }
static bool CtrBattleBg_MoveBgShown(void) { return (source.flags&CTR_PRESENT_MOVE_BG)!=0; }
static bool CtrBattleIntro_Sliding(void) { return (source.flags&CTR_PRESENT_INTRO)!=0; }
static uint32_t LocalLink_Milliseconds(void) { return now; }
static void CtrLog_Write(int n,const char *s) { (void)n;(void)s; }
void CtrVideo_NotifyWrite(const void *p,unsigned n) { assert(p==sMemory.vram && n==CTR_PRESENT_VRAM);++writeNotice; }
bool VoxelWorld_PresentMatches(const CtrPresentWorld *p) { return CtrBattlePresent_WorldValid(p); }
void CtrCoopSpectator_Stop(void) { AStop(); }
#include "3ds_battle_present_video.inc"
#include "present_renderer.inc"
#include "present_attack.inc"

struct Packet { unsigned from,length,due;uint8_t bytes[160]; };
static struct Packet queue[512],stale;
static unsigned queued,sends[2],wireBytes,keys,deltas,lastKey,lastDelta,maxBurst,delay;
static bool blocked,dispatching,dropChunk,corruptChunk,duplicate;
static bool Send(unsigned from,const void *p,unsigned length)
{
    assert(!dispatching && length<=160 && queued<512);
    if(blocked) return false;
    assert(++sends[from]<=CTR_SPECTATOR_TICK_PACKETS);
    if(sends[from]>maxBurst) maxBurst=sends[from];
    wireBytes+=length;
    struct Packet *q=&queue[queued++];q->from=from;q->length=length;q->due=now+delay;memcpy(q->bytes,p,length);
    if(q->bytes[0]==71) {
        unsigned base=q->bytes[10]|q->bytes[11]<<8|q->bytes[12]<<16|q->bytes[13]<<24;
        unsigned size=q->bytes[14]|q->bytes[15]<<8|q->bytes[16]<<16|q->bytes[17]<<24;
        if(base) { ++deltas;lastDelta=size; } else { ++keys;lastKey=size; }
    }
    if(q->bytes[0]==73) stale=*q;
    return true;
}
bool ASend(uint16_t peer,const void *p,uint32_t n) { assert(peer==2);return Send(0,p,n); }
bool BSend(uint16_t peer,const void *p,uint32_t n) { assert(peer==1);return Send(1,p,n); }
uint32_t ANow(void) { return now; }
uint32_t BNow(void) { return now; }
bool BCaptureRequest(void) { return CtrVideo_PresentRequestCapture(); }
const uint8_t *BCaptureTake(void) { return CtrVideo_PresentTakeCapture(); }
void BCaptureCancel(void) { CtrVideo_PresentCancelCapture(); }
bool ACaptureRequest(void) { abort(); }
const uint8_t *ACaptureTake(void) { abort(); }
void ACaptureCancel(void) {}
bool ADisplay(const uint8_t *p) { bool ok=CtrVideo_PresentDisplay(p);if(ok && p) { ++admitted;memcpy(&last,p,sizeof(last)); }return ok; }
bool BDisplay(const uint8_t *p) { assert(!p);return true; }
static void SourceFrame(void)
{
    CtrVideoMemory saved=sMemory;
    sMemory=(CtrVideoMemory){source.vram,source.palette,source.oam,source.regs};
    sBattle=true;sStats.frames=source.frame-1;
    memcpy(gPlttBufferUnfaded,source.unfaded,sizeof(gPlttBufferUnfaded));
    memcpy(sLineScroll,source.scroll,sizeof(sLineScroll));sLineCount=source.lineCount;sLineMask=source.lineMask;
    sLineRegs=source.lineRegs?source.scanlines:NULL;
    sBoxSlide=source.boxSlide;sBoxDecided=source.boxDecided;sBoxHidden=source.boxHidden;sBoxBarsUp=source.boxBarsUp;
    sHud=(source.flags&CTR_PRESENT_HD)?&source:NULL;
    PresentCapture();sMemory=saved;sBattle=false;
}
static void Render(void)
{
    CtrVideoMemory saved=sMemory;
    PresentVideoSaved before;PresentSave();before=sPresentSaved;
    PresentBegin();
    if(sPresent.bound) {
        assert(CtrBattlePresent_Current()==sPresent.display && sMemory.vram==sPresent.display->vram);
        unsigned priority=0;int x=0,y=0,w=0,h=0;
        assert(OamPlace(0,false,&priority,&x,&y,&w,&h));
        static int previous=-999;
        if(x!=previous) ++movingFrames;
        previous=x;++localFrames;
    }
    PresentRestore();assert(!CtrBattlePresent_Current());
    assert(!memcmp(&saved,&sMemory,sizeof(saved)));
    PresentSave();assert(!memcmp(&before,&sPresentSaved,sizeof(before)));
}
static void Step(void)
{
    now+=16;++source.frame;SourceFrame();sends[0]=sends[1]=0;ATick();BTick();
    unsigned count=queued;queued=0;
    for(unsigned i=0;i<count;++i) {
        struct Packet p=queue[i];if(p.due>now) {queue[queued++]=p;continue;}
        if(dropChunk && p.bytes[0]==73) {dropChunk=false;continue;}
        if(corruptChunk && p.bytes[0]==73) {corruptChunk=false;p.bytes[p.length-1]^=7;}
        dispatching=true;
        if(p.from) AHandlePacket(2,p.bytes,p.length);else BHandlePacket(1,p.bytes,p.length);
        if(duplicate && p.bytes[0]==73) { if(p.from) AHandlePacket(2,p.bytes,p.length);else BHandlePacket(1,p.bytes,p.length); }
        dispatching=false;
    }
    Render();assert(own.magic==0x13579bdf && own.world.grid[0]==0x4567);
}
static void Steps(unsigned count) { while(count--)Step(); }
static void Load(const char *path,void *out,unsigned max)
{
    FILE *f=fopen(path,"rb");assert(f);assert(fread(out,1,max,f)>0);fclose(f);
}
static void InitState(void)
{
    memset(&source,0,sizeof(source));source.magic=CTR_PRESENT_MAGIC;source.frame=90;
    source.flags=CTR_PRESENT_HD;source.boxSlide=-1;
    for(unsigned i=0;i<128;++i) source.oam[4*i]=0x200;
    source.oam[0]=60;source.oam[1]=40;source.oam[2]=1;
    source.regs[0]=0x1f40;source.regs[4]=4;
    source.hud[0]=(CtrPresentHud){.used=1,.kind=0,.level=15,.hp=50,.maxHp=50,.hpPixels=48,.boxTile=512,.boxTiles=128};
    source.hud[0].name[0]=0xbb;source.hud[0].name[1]=0xff;
    Load("build/upstream/graphics/battle_environment/tall_grass/tiles.4bpp",source.vram,0x8000);
    Load("build/upstream/graphics/battle_environment/tall_grass/map.bin",source.vram+0x8000,0x800);
    Load("build/upstream/graphics/battle_environment/tall_grass/palette.gbapal",source.palette,32);
    Load("build/upstream/graphics/pokemon/treecko/back.4bpp",source.vram+0x10000,2048);
    Load("build/upstream/graphics/pokemon/zigzagoon/front.4bpp",source.vram+0x10800,2048);
    own.magic=0x13579bdf;own.world.grid[0]=0x4567;
    sMemory=(CtrVideoMemory){own.vram,own.palette,own.oam,own.regs};
}
static void Fresh(void)
{
    AReset();BReset();PresentShutdown();queued=0;blocked=dropChunk=corruptChunk=duplicate=false;delay=0;
    InitState();ASetState(2,true,false,false,true);BSetState(1,true,false,true,false);Steps(3);
}
static void WaitFirst(void)
{
    unsigned begin=now;
    for(unsigned i=0;i<1200 && !sPresent.active;++i) Step();
    assert(AHasFrame() && AIsWatching() && BHasViewer() && sPresent.active);
    printf("first presentation %u ms; raw %u; key %u bytes\n",now-begin,CTR_PRESENT_BYTES,lastKey);
}
static void CodecAndBounds(void)
{
    static uint8_t packed[CTR_PRESENT_ENCODED_MAX],decoded[CTR_PRESENT_BYTES+1];
    assert(CtrBattlePresent_Validate(&source));
    size_t size=CtrBattlePresent_Encode((uint8_t*)&source,NULL,sizeof(source),packed,sizeof(packed));
    assert(size && CtrBattlePresent_Decode(packed,size,NULL,decoded,sizeof(source)) && !memcmp(decoded,&source,sizeof(source)));
    first=source;first.world.mapCount=17;assert(!CtrBattlePresent_Validate(&first));
    first=source;first.world.mapCount=1;first.world.maps[0].id=0xffff;assert(!CtrBattlePresent_Validate(&first));
    first=source;first.world.gridWidth=65535;assert(!CtrBattlePresent_Validate(&first));
    first=source;first.world.gridWidth=128;first.world.gridHeight=80;assert(CtrBattlePresent_Validate(&first));
    ++first.world.gridHeight;assert(!CtrBattlePresent_Validate(&first));
    first=source;first.world.camera[2]=NAN;assert(!CtrBattlePresent_Validate(&first));
    first=source;first.hud[0].boxTile=1000;assert(!CtrBattlePresent_Validate(&first));
    uint32_t random=1;
    for(unsigned n=0;n<1000;++n) {
        unsigned bytes=n%sizeof(packed),count=n%1024;decoded[count]=0xa5;
        for(unsigned i=0;i<bytes;++i) { random=random*1664525u+1013904223u;packed[i]=random>>24; }
        (void)CtrBattlePresent_Decode(packed,bytes,NULL,decoded,count);assert(decoded[count]==0xa5);
    }
    first=source;last=source;last.frame+=6;last.oam[1]+=36;last.hud[0].hp=10;last.hud[0].hpPixels=10;
    last.vram[0x8000]^=1;last.hud[0].status=1;
    CtrBattlePresent_Interpolate(&first,&last,50,100,&source);
    assert(source.oam[1]==58 && source.hud[0].hp==30 && source.hud[0].status==0 && source.vram[0x8000]==first.vram[0x8000]);
    CtrBattlePresent_Interpolate(&first,&last,100,100,&source);assert(!memcmp(&source,&last,sizeof(source)));
    first=source;last=source;first.oam[0]=last.oam[0]=0x100|60;
    first.oam[3]=256;last.oam[3]=128;
    first.palette[0]=31;last.palette[0]=31<<10;
    first.lineMask=last.lineMask=1;first.lineCount=last.lineCount=240;
    first.scroll[0][9]=500;last.scroll[0][9]=12;
    CtrBattlePresent_Interpolate(&first,&last,50,100,&source);
    assert(source.oam[3]==192 && source.palette[0]==(15|(15<<10)) && source.scroll[0][9]==0);
}
static void Stream(void)
{
    Fresh();assert(ARequest());WaitFirst();Steps(15);
    unsigned begin=now,bytes=wireBytes,frames=localFrames,moves=movingFrames,updates=admitted;
    clock_t cpu=clock();
    for(unsigned i=0;i<240;++i) {
        source.oam[1]=40+(i%50);source.hud[0].hp=50-i%50;source.hud[0].hpPixels=48-i%40;
        /* Real VRAM remains populated while OAM/HP evolve every 16ms. */
        Step();
    }
    printf("movement: %.1f admitted/s, %.1f local/s, %.1f changed-position/s; %.1f KiB/s; host CPU %.3f ms/step; delta %u B\n",
        (admitted-updates)*1000.0/(now-begin),(localFrames-frames)*1000.0/(now-begin),(movingFrames-moves)*1000.0/(now-begin),
        (wireBytes-bytes)*1000.0/(now-begin)/1024,1000.0*(clock()-cpu)/CLOCKS_PER_SEC/240,lastDelta);
    assert(localFrames-frames==240 && movingFrames-moves>150 && admitted-updates>50);
    corruptChunk=true;source.hud[0].status=2;
    for(unsigned i=0;i<500 && last.hud[0].status!=2;++i) Step();
    assert(last.hud[0].status==2); /* NACK recovers using a fresh key/base. */
    /* Shipped Psychic background, with a native-sized palette, scanline
     * wave, text tile change and status result in the same ordered update. */
    memcpy(source.vram,attackTiles,sizeof(attackTiles));
    memcpy(source.palette,attackPalette,sizeof(attackPalette));
    Load("build/upstream/graphics/battle_anims/backgrounds/psychic.bin",source.vram+0x8000,4096);
    source.flags|=CTR_PRESENT_MOVE_BG;source.hud[0].status=1;source.lineCount=240;source.lineMask=4;
    for(unsigned y=0;y<240;++y) source.scroll[2][y]=(y%16)/2;
    unsigned attackStart=now,attackBefore=admitted;
    for(unsigned i=0;i<500 && (admitted==attackBefore || last.hud[0].status!=1);++i)Step();
    assert(last.hud[0].status==1 && !memcmp(last.vram,attackTiles,sizeof(attackTiles)));
    printf("native Psychic background + palette + scanline + status: %u ms, %u B delta, %u B native tiles\n",now-attackStart,lastDelta,(unsigned)sizeof(attackTiles));
    source.lineCount=240;source.lineMask=255;source.lineRegs=1;
    for(unsigned i=0;i<512;++i) source.palette[i]=(i*17)&32767;
    for(unsigned y=0;y<160;++y)for(unsigned r=0;r<35;++r) source.scanlines[y][r]=(y*r)&511;
    for(unsigned i=0;i<8*240;++i) ((uint16_t*)source.scroll)[i]=i&511;
    for(unsigned i=0;i<0x8000;++i) source.vram[i]=(i*7)^((i>>7)*13);
    source.flags|=CTR_PRESENT_MOVE_BG;source.hud[0].status=3;
    source.vram[0x8000]=0xab;unsigned start=now,old=admitted;
    duplicate=true;dropChunk=true;
    for(unsigned i=0;i<1000 && (admitted==old || last.hud[0].status!=3);++i)Step();
    assert(last.hud[0].status==3 && last.vram[0x8000]==0xab && last.scanlines[159][34]==((159*34)&511));
    printf("full attack/palette/scanline change with one lost chunk: %u ms, delta %u B; burst max %u\n",now-start,lastDelta,maxBurst);
    blocked=true;uint16_t held=0xffff,pressed=2;assert(AInputGate(&held,&pressed));
    assert(!held && !pressed && !sPresent.active && !AIsWatching());
    held=16;pressed=0;assert(!AInputGate(&held,&pressed) && held==16);
    unsigned before=fence;Render();assert(fence==before+1 && park==fence);
    AHandlePacket(2,stale.bytes,stale.length);assert(!sPresent.active);
    blocked=false;Steps(4);assert(!BHasViewer());
    Fresh();assert(ARequest());blocked=true;Steps(3);AStop();assert(!AIsWatching());
    Fresh();assert(ARequest());WaitFirst();ASetState(0,false,false,false,false);assert(!sPresent.active);Render();
    Fresh();assert(ARequest());WaitFirst();BSetState(1,true,false,false,true);Steps(4);assert(!AIsWatching());
    Fresh();assert(ARequest());WaitFirst();AReset();assert(!sPresent.active);Render();
    assert(fence==park && resetHud==park && releases==park*8);
}
static void VoxelTransport(void)
{
    Fresh();source.flags|=CTR_PRESENT_VOXEL;source.world.ready=1;source.world.layout=1;
    source.world.mapCount=source.world.borderCount=1;source.world.mapHash=123;
    source.world.maps[0]=(CtrPresentMap){.layout=1};source.world.borders[0]=(CtrPresentBorder){.layout=1,.drawable=1};
    source.world.gridWidth=45;source.world.gridHeight=44;source.world.camera[8]=30;source.world.camera[9]=45;
    uint16_t grid[30*30];Load("build/upstream/data/layouts/PetalburgCity/map.bin",grid,sizeof(grid));
    for(unsigned y=0;y<30;++y)memcpy(&source.world.grid[(y+7)*45+7],grid+y*30,60);
    assert(ARequest());WaitFirst();Steps(12);
    unsigned bytes=wireBytes,begin=now;
    for(unsigned i=0;i<60;++i) { source.world.camera[0]=5+i/10.0f;source.world.camera[3]=i/10.0f;Step(); }
    assert(last.world.ready && last.world.grid[7*45+7]==grid[0]);
    printf("voxel scenery + camera: key %u B, delta %u B; %.1f KiB/s; static grid preserved\n",lastKey,lastDelta,(wireBytes-bytes)*1000.0/(now-begin)/1024);
    AReset();Render();assert(!CtrBattlePresent_Current());
}
static void PlaybackCapacity(void)
{
    InitState();assert(CtrVideo_PresentDisplay((const uint8_t*)&source));
    ++source.frame;source.oam[1]+=12;assert(CtrVideo_PresentDisplay((const uint8_t*)&source));
    ++source.frame;source.oam[1]+=12;assert(CtrVideo_PresentDisplay((const uint8_t*)&source));
    ++source.frame;assert(!CtrVideo_PresentDisplay((const uint8_t*)&source));
    assert(sPresent.hasQueued && sPresent.latest==source.frame-1);
    now+=100;Render();assert(!sPresent.hasQueued);
    assert(CtrVideo_PresentDisplay((const uint8_t*)&source));
    assert(!CtrVideo_PresentDisplay((const uint8_t*)&source));
    assert(CtrVideo_PresentDisplay(NULL));Render();PresentShutdown();
}
int main(void)
{
    InitState();CodecAndBounds();PlaybackCapacity();Stream();VoxelTransport();AReset();BReset();PresentShutdown();
    printf("presentation transport/render lifecycle passed; playback/capture heap %u bytes, receiver transport %u bytes\n",5*CTR_PRESENT_BYTES,2*CTR_PRESENT_BYTES+CTR_PRESENT_ENCODED_MAX);
    return 0;
}
