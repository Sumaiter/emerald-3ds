#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "3ds_battle_present.h"
#include "voxel/voxel_world.h"
typedef uint16_t u16;typedef uint8_t u8;typedef int32_t s32;
#define MAP_OFFSET 7
#define MAP_OFFSET_W 15
#define MAP_OFFSET_H 14
#define MAPGRID_METATILE_ID_MASK 1023
#define MAP_TYPE_INDOOR 1
#define MAP_TYPE_SECRET_BASE 2
#define VOXEL_PLACED_MAX 64
struct MapLayout { int width,height;const u16 *map,*border;const void *primaryTileset,*secondaryTileset; };
struct MapHeader { const struct MapLayout *mapLayout;unsigned mapLayoutId,mapType; };
struct BackupMapLayout { int width,height;u16 *map; };
static struct MapHeader gMapHeader;
static struct { struct { int mapGroup,mapNum; } location;unsigned mapLayoutId; } save,*gSaveBlock1Ptr=&save;
static u16 ownGrid[45*44],romGrid[30*30],border[4]={11,12,13,14};
static struct BackupMapLayout gBackupMapLayout={45,44,ownGrid};
static const struct MapLayout asset={30,30,romGrid,border,border,border};
static const struct MapHeader header={&asset,1,0};
static unsigned lookups,fieldReads,updates;
static const struct MapLayout *Port_GetMapLayoutById(unsigned id) { assert(id==1);++lookups;return &asset; }
static const struct MapHeader *Port_GetMapHeaderByGroupAndId(unsigned g,unsigned n) { assert(!g && n<2);++lookups;return &header; }
static const void *Port_ResolveAssetPointer(const void *p) { return p; }
const uint16_t *Voxel_ResolveMap(const void *p) { return ((const struct MapLayout*)p)->map; }
static void VoxelBuildings_SetVariantFits(bool (*f)(const VoxelMapInstance*,unsigned,unsigned)) { (void)f; }
static void VoxelBuildings_SetDrawings(const uint16_t *(*f)(int,int*,int*)) { (void)f; }
static bool VariantFits(const VoxelMapInstance *m,unsigned l,unsigned t) { (void)m;(void)l;(void)t;return true; }
const uint16_t *VoxelWorld_LayoutDrawing(int i,int *w,int *h) { (void)i;*w=*h=30;return romGrid; }
static int CurrentLift(int g,int n) { (void)g;(void)n;++fieldReads;return 5; }
static void AddConnections(const VoxelMapInstance *m,bool a,bool b) { (void)m;(void)a;(void)b;++fieldReads; }
static void RememberLift(const VoxelMapInstance *m) { (void)m;++fieldReads; }
static int EdgeFillAt(int x,int y) { (void)x;(void)y;return -1; }
static const u16 *MapGridGetCameraBorderAt(int x,int y,u8 *i) { *i=(x+y)&3;++fieldReads;return border; }
static struct { unsigned count; } sBorderMemo;
static bool MemoGet(const void *m,const void *key,const void **out) { (void)m;(void)key;(void)out;return false; }
static void MemoPut(void *m,const void *key,const void *out) { (void)m;(void)key;(void)out; }
struct CameraMap { const struct MapHeader *header;u16 id;int16_t x,y,phaseX,phaseY;bool border; };
static struct CameraMap sCameraMaps[16];
static unsigned sCameraMapCount;
static void UpdateCameraMaps(void) { ++updates; }
VoxelWeatherClass VoxelWorld_Weather(void) { return VOXEL_WEATHER_CLEAR; }
float VoxelWorld_WeatherAmount(void) { return 0; }
float VoxelWorld_Lightning(void) { return 0; }
float VoxelWorld_FogDensity(void) { return 0; }
bool VoxelWorld_ScreenFade(float *f,float rgb[3]) { *f=0;memset(rgb,0,3*sizeof(float));return false; }
unsigned VoxelWorld_ClosedEdges(void) { return 0; }
bool VoxelWorld_Underground(void) { return false; }
bool VoxelWorld_MapIsConnected(void) { return true; }
#include "present_world.inc"
int main(void)
{
    static CtrBattlePresentFrame p,original;
    gMapHeader=header;save.location.mapNum=1;save.mapLayoutId=1;
    ownGrid[7+7*45]=77;romGrid[0]=88;
    VoxelWorld_BuildInstances();assert(VoxelWorld_GetMetatileId(0,0)==77);
    const VoxelMapInstance *field=VoxelWorld_Instance(0);VoxelMapInstance snapshot=*field;
    unsigned reads=fieldReads;
    p.magic=CTR_PRESENT_MAGIC;p.frame=1;p.flags=CTR_PRESENT_VOXEL;p.world.ready=1;
    p.world.layout=1;p.world.mapCount=p.world.borderCount=1;p.world.mapHash=123;
    p.world.maps[0]=(CtrPresentMap){1,0,0,0,19,0};p.world.borders[0]=(CtrPresentBorder){.layout=1,.drawable=1};
    p.world.gridWidth=45;p.world.gridHeight=44;p.world.grid[7+7*45]=99;
    p.world.camera[8]=30;p.world.camera[9]=45;
    assert(CtrBattlePresent_Validate(&p) && VoxelWorld_PresentMatches(&p.world));
    CtrBattlePresent_Bind(&p);VoxelWorld_BuildInstances();
    assert(VoxelWorld_Instance(0)!=field && VoxelWorld_Instance(0)->mapNum==0 && VoxelWorld_Instance(0)->lift==19);
    assert(VoxelWorld_GetMetatileId(0,0)==99 && VoxelWorld_BorderMetatile(-1,-1)==14);
    assert(VoxelWorld_LiveDigest()==123 && fieldReads==reads);
    original=p;p.world.maps[1]=(CtrPresentMap){1,1,30,0,19,0};p.world.mapCount=2;
    assert(CtrBattlePresent_Validate(&p));VoxelWorld_BuildInstances();assert(VoxelWorld_GetMetatileId(30,0)==88);
    assert(!memcmp(field,&snapshot,sizeof(snapshot)) && ownGrid[7+7*45]==77 && save.location.mapNum==1);
    CtrBattlePresent_Bind(NULL);assert(VoxelWorld_Instance(0)==field && VoxelWorld_GetMetatileId(0,0)==77);
    unsigned before=lookups;p=original;p.world.maps[0].id=0xffff;
    assert(!VoxelWorld_PresentMatches(&p.world) && lookups==before);
    p=original;p.world.borders[0].layout=65535;assert(!VoxelWorld_PresentMatches(&p.world) && lookups==before);
    p=original;p.world.gridWidth=46;assert(!VoxelWorld_PresentMatches(&p.world));
    p=original;p.world.maps[0].x=32767;assert(!VoxelWorld_PresentMatches(&p.world));
    p=original;p.world.mapCount=17;assert(!VoxelWorld_PresentMatches(&p.world));
    sCameraMapCount=16;
    for(unsigned i=0;i<16;++i) sCameraMaps[i]=(struct CameraMap){&gMapHeader,i,12,-4,2,3,true};
    struct { CtrPresentBorder borders[16];uint32_t canary; } exported={.canary=0xabcd1234};
    assert(CtrBattlePresent_CaptureBorders(exported.borders)==16 && updates==1 && exported.canary==0xabcd1234);
    assert(exported.borders[15].layout==1 && exported.borders[15].id==15 && exported.borders[15].phaseY==3);
    CtrPresentWorld captured={.ready=1};VoxelWorld_PresentCapture(&captured);
    assert(captured.mapNum==1 && captured.mapCount==1 && captured.maps[0].lift==5 && captured.borderCount==16);
    assert(captured.gridWidth==45 && captured.gridHeight==44 && captured.grid[7+7*45]==77 && captured.mapHash);
    unsigned hash=captured.mapHash;captured.camera[0]=5;VoxelWorld_PresentCapture(&captured);assert(hash==captured.mapHash);
    ownGrid[7+7*45]=78;VoxelWorld_PresentCapture(&captured);assert(hash!=captured.mapHash);
    puts("actual world bodies: different-map grid/instances/borders, bounds-before-lookup, unchanged own world and bounded engine export passed");
    return 0;
}
