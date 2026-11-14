#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define U64_MAX UINT64_MAX
#define AHEAD_DONE 3
#define VOXEL_BUILDING_PAGES 2
#define VOXEL_ATLAS_SLOTS 2
#define VOXEL_CHUNK_SLOTS 2
#define VOXEL_PAGE_PIECES 3
#define CTR_LOG_VIDEO 0
static struct { void *thread,*buffer;bool quit;int wake; } sStream;
static struct { int state,request,result,failed; } sAssetAhead;
static struct { struct { bool active; } job;void *slot;bool ready; } sAtlasJob;
static struct { bool active;uint64_t ticks; } sJob;
static struct { struct { void *data; } tex; } sPageSlots[2],sAtlases[2];
static bool sReady=true,sParked,sAtlasCapped=true,sHaveBuildings=true,sViewReady=true;
static unsigned sPreviousOriginCount=4;
static unsigned sJobCancels,sequence,joined,frees,adopted;
static float sCancelledBuildMs;
static void *sPageBlock=(void*)3,*sChunkBlock=(void*)4;
static int sChunkArena,sPageArena,sChunkPieces[3],sPagePieces[3],sMeshMapGroup=1,sMeshMapNum=1;
static void LightEvent_Signal(int *e) { assert(e==&sStream.wake && sStream.quit && sequence==1);sequence=2; }
static void threadJoin(void *thread,uint64_t timeout) { assert(thread==(void*)1 && timeout==U64_MAX && sequence==2);sequence=3;++joined; }
static void threadFree(void *thread) { assert(thread==(void*)1 && sequence==3); }
static void CtrAssets_PrefetchAdopt(int *r,int result) { assert(sequence==3 && r==&sAssetAhead.request && result==9);++adopted; }
static void linearFree(void *p) { assert(sequence==3 && p==(void*)2);sequence=4; }
static void VoxelAtlas_JobCancel(void *job) { assert(job==&sAtlasJob.job && sequence==4);sAtlasJob.job.active=false; }
static float TicksMs(uint64_t n) { assert(sequence==4);return n; }
static void LampsRelease(bool all) { assert(all && sequence==1); }
static void CheckReferences(void) {
    assert(!sStream.thread && !sStream.buffer && !sAtlasJob.job.active && !sAtlasJob.slot && !sAtlasJob.ready && !sJob.active);
}
static void ReleaseAllChunks(void) { CheckReferences(); }
static void StaticMeshesFree(void) { CheckReferences(); }
static void PageFree(void *p) { CheckReferences();memset(p,0,sizeof(sPageSlots[0]));++frees; }
static void FreeAtlasTextures(void *p) { CheckReferences();memset(p,0,sizeof(sAtlases[0]));++frees; }
static void AnimForget(void *p) { (void)p;CheckReferences(); }
static void VoxelArena_Init(void *arena,void *data,unsigned size,unsigned alignment,void *pieces,unsigned n) {
    (void)arena;(void)pieces;assert(!data && !size && (alignment==16 || alignment==128) && n==3);CheckReferences();
}
static void vramFree(void *p) { assert(p==(void*)3 || p==(void*)4);CheckReferences();++frees; }
static unsigned vramSpaceFree(void) { return 123; }
static void CtrLog_Write(unsigned level,const char *fmt,unsigned long size) { (void)level;(void)fmt;assert(size==123); }
#include "present_fence.inc"
int main(void)
{
    /* The compositor probe separately verifies FrameSync precedes this body. */
    sequence=1;sStream.thread=(void*)1;sStream.buffer=(void*)2;
    sAssetAhead.state=AHEAD_DONE;sAssetAhead.result=9;
    sAtlasJob.job.active=true;sAtlasJob.slot=(void*)7;sAtlasJob.ready=true;sJob.active=true;sJob.ticks=12;
    for(unsigned i=0;i<2;++i) { sPageSlots[i].tex.data=(void*)8;sAtlases[i].tex.data=(void*)9; }
    CtrVoxel_ParkVram();
    assert(joined==1 && adopted==1 && frees==6 && sParked && !sHaveBuildings && !sPageBlock && !sChunkBlock);
    assert(sMeshMapGroup==-1 && sMeshMapNum==-1 && !sAtlasCapped && sJobCancels==1 && sCancelledBuildMs==12);
    assert(!sPreviousOriginCount && !sViewReady);
    CtrVoxel_ParkVram();assert(joined==1 && frees==6);
    puts("actual ParkVram/StreamStop/AtlasJobCancel/JobCancel: joined worker before freeing atlas/page/chunk references, repeat stop safe");
    return 0;
}
