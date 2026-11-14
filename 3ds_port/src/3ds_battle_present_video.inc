/* Included by the compositor after its private declarations. The network
 * never binds a state or touches GPU resources. Ownership changes here,
 * outside an open frame, after GPU completion and the voxel worker join. */
#include "voxel/voxel_world.h"

static struct {
    CtrBattlePresentFrame *capture,*left,*right,*display,*queued;
    bool asked,ready,active,owned,interval,hasQueued,bound;
    uint32_t latest,start,duration;
} sPresent;

typedef struct {
    CtrVideoMemory memory;
    const uint16_t (*lineRegs)[CTR_LINE_REGS];
    uint16_t scroll[8][LINE_MAX],ext[CTR_EXT_PALETTES*16];
    uint32_t anchored[4],marks[8][4];
    int16_t anchors[128];
    uint8_t oamExt[128];
    unsigned lineMask,lineCount,extInUse,centred,tileBanks;
    bool stage,stageFaded,stageZoomed,battle,transition,holdTop,fieldBanner,inBattle;
    bool oamExtMarked;
} PresentVideoSaved;
static PresentVideoSaved sPresentSaved;

bool CtrVideo_PresentRequestCapture(void)
{
    if(!sPresent.capture) sPresent.capture=malloc(sizeof(*sPresent.capture));
    if(!sPresent.capture) return false;
    sPresent.asked=true;
    return true;
}
const uint8_t *CtrVideo_PresentTakeCapture(void)
{
    if(!sPresent.ready) return NULL;
    sPresent.ready=false;
    return (const uint8_t *)sPresent.capture;
}
void CtrVideo_PresentCancelCapture(void) { sPresent.asked=sPresent.ready=false; }

static unsigned PresentDuration(uint32_t a,uint32_t b)
{
    uint32_t frames=b-a;
    /* A pause in delivery is not a request to replay a long pause. Limit
     * catch-up latency to four local frames, including after a static menu. */
    if(frames>4) frames=4;
    return frames?frames*1000u/60u:16u;
}
bool CtrVideo_PresentDisplay(const uint8_t *bytes)
{
    if(!bytes) {
        sPresent.active=sPresent.interval=sPresent.hasQueued=false;
        sPresent.latest=0;
        /* The state buffers contain only CPU-side values. The GPU reads
         * the compositor's own textures, whose retirement is fenced below. */
        return true;
    }
    const CtrBattlePresentFrame *p=(const CtrBattlePresentFrame *)bytes;
    if(!CtrBattlePresent_Validate(p)) return false;
    if(sPresent.active && (p->frame==sPresent.latest || (uint32_t)(p->frame-sPresent.latest)>=0x80000000u))
        return false;
    if(sPresent.hasQueued) return false;
    if(!sPresent.left) sPresent.left=malloc(sizeof(*p));
    if(!sPresent.right) sPresent.right=malloc(sizeof(*p));
    if(!sPresent.display) sPresent.display=malloc(sizeof(*p));
    if(!sPresent.queued) sPresent.queued=malloc(sizeof(*p));
    if(!sPresent.left || !sPresent.right || !sPresent.display || !sPresent.queued) return false;
    if(!sPresent.active) {
        memcpy(sPresent.left,p,sizeof(*p));memcpy(sPresent.display,p,sizeof(*p));
        sPresent.active=true;sPresent.interval=false;
    } else if(!sPresent.interval) {
        memcpy(sPresent.right,p,sizeof(*p));sPresent.interval=true;
        sPresent.start=LocalLink_Milliseconds();
        sPresent.duration=PresentDuration(sPresent.left->frame,p->frame);
    } else {
        memcpy(sPresent.queued,p,sizeof(*p));sPresent.hasQueued=true;
    }
    sPresent.latest=p->frame;
    return true;
}

static void PresentAdvance(void)
{
    uint32_t now=LocalLink_Milliseconds();
    while(sPresent.interval && (uint32_t)(now-sPresent.start)>=sPresent.duration) {
        CtrBattlePresentFrame *old=sPresent.left;sPresent.left=sPresent.right;sPresent.right=old;
        sPresent.start+=sPresent.duration;sPresent.interval=false;
        if(sPresent.hasQueued) {
            old=sPresent.right;sPresent.right=sPresent.queued;sPresent.queued=old;
            sPresent.hasQueued=false;sPresent.interval=true;
            sPresent.duration=PresentDuration(sPresent.left->frame,sPresent.right->frame);
        }
    }
    if(sPresent.interval)
        CtrBattlePresent_Interpolate(sPresent.left,sPresent.right,now-sPresent.start,sPresent.duration,sPresent.display);
    else memcpy(sPresent.display,sPresent.left,sizeof(*sPresent.display));
}

static void PresentCachesRetire(void)
{
    C3D_FrameSync();
    CtrVoxel_EndBattle();
    /* Park stops and joins StreamWorker, cancels AtlasJob/Job, and only then
     * releases their chunks and backing arenas. No old pointer survives. */
    CtrVoxel_ParkVram();
    StereoSurfaceRelease();BandsRelease();LayersRelease();SceneRelease();SlotsRelease();
    SceneBlendRelease();ObjWindowRelease();HudRelease();
    CtrBattleHud_PresentReset();
    memset(sHash,0,sizeof(sHash));sUsed=0;++sCacheGeneration;
    memset(sPalette,0xff,sizeof(sPalette));
    memset(sBoxEntries,0xff,sizeof(sBoxEntries));
    sSceneCached=sSceneEmpty=sEdgeStore.on=sEdgeStore.held=false;
    sSceneFailed=false;sBandsFailed=false;
    sBoxDecided=sBoxHidden=sBoxBarsUp=sBoxText=false;sBoxSlide=-1;sBoxBattleFrames=0;
    sHud=NULL;sHudCrisp=false;
    sSceneOn=NULL;BlendForget();
}

static void PresentMarks(uint32_t marks[8][4],bool save)
{
    uint32_t *ptrs[8]={sVoxelWeatherOam,sVoxelCloudOam,sScreenOam,sMachineOam,sFlyOam,
                       sFogOam,sLatticeOam,sParticleOam};
    for(unsigned i=0;i<8;++i) {
        if(save) memcpy(marks[i],ptrs[i],sizeof(marks[i]));
        else memcpy(ptrs[i],marks[i],sizeof(marks[i]));
    }
}

static void PresentSave(void)
{
    PresentVideoSaved *s=&sPresentSaved;
    s->memory=sMemory;s->lineRegs=sLineRegs;s->lineMask=sLineMask;s->lineCount=sLineCount;
    memcpy(s->scroll,sLineScroll,sizeof(s->scroll));memcpy(s->ext,sExtPltt,sizeof(s->ext));
    memcpy(s->anchored,sOamAnchored,sizeof(s->anchored));memcpy(s->anchors,sOamAnchor,sizeof(s->anchors));
    memcpy(s->oamExt,sOamExt,sizeof(s->oamExt));PresentMarks(s->marks,true);
    s->extInUse=sExtInUse;s->oamExtMarked=sOamExtMarked;
    s->stage=sStageRequested;s->stageFaded=sStageFadedRequested;s->stageZoomed=sStageZoomedRequested;
    s->centred=sCentredRequested;s->battle=sBattleRequested;s->transition=sTransitionRequested;
    s->holdTop=sHoldTop;s->tileBanks=sTileBanksLive;s->fieldBanner=sFieldBanner;s->inBattle=sInBattle;
}
static void PresentRestore(void)
{
    if(!sPresent.bound) return;
    PresentVideoSaved *s=&sPresentSaved;
    CtrBattlePresent_Bind(NULL);sPresent.bound=false;
    sMemory=s->memory;sLineRegs=s->lineRegs;sLineMask=s->lineMask;sLineCount=s->lineCount;
    memcpy(sLineScroll,s->scroll,sizeof(s->scroll));memcpy(sExtPltt,s->ext,sizeof(s->ext));
    memcpy(sOamAnchored,s->anchored,sizeof(s->anchored));memcpy(sOamAnchor,s->anchors,sizeof(s->anchors));
    memcpy(sOamExt,s->oamExt,sizeof(s->oamExt));PresentMarks(s->marks,false);
    sExtInUse=s->extInUse;sOamExtMarked=s->oamExtMarked;
    sStageRequested=s->stage;sStageFadedRequested=s->stageFaded;sStageZoomedRequested=s->stageZoomed;
    sCentredRequested=s->centred;sBattleRequested=s->battle;sTransitionRequested=s->transition;
    sHoldTop=s->holdTop;sTileBanksLive=s->tileBanks;sFieldBanner=s->fieldBanner;sInBattle=s->inBattle;
}
static void PresentBegin(void)
{
    if(sPresent.active) {
        PresentAdvance();
        const CtrBattlePresentFrame *p=sPresent.display;
        if((p->flags&CTR_PRESENT_VOXEL) && !VoxelWorld_PresentMatches(&p->world)) {
            CtrLog_Write(CTR_LOG_ERROR,"spectator presentation: invalid peer scenery assets; view canceled");
            CtrCoopSpectator_Stop();
        }
    }
    if(sPresent.owned!=sPresent.active) {
        PresentCachesRetire();sPresent.owned=sPresent.active;
        if(sPresent.active) CtrBattleHud_Prepare();
    }
    if(!sPresent.active) return;
    PresentSave();
    CtrBattlePresentFrame *p=sPresent.display;
    CtrBattlePresent_Bind(p);sPresent.bound=true;
    sMemory=(CtrVideoMemory){p->vram,p->palette,p->oam,p->regs};
    sLineRegs=p->lineRegs?p->scanlines:NULL;sLineMask=p->lineMask;sLineCount=p->lineCount;
    memcpy(sLineScroll,p->scroll,sizeof(sLineScroll));memcpy(sExtPltt,p->extPalette,sizeof(sExtPltt));
    memcpy(sOamAnchored,p->anchored,sizeof(sOamAnchored));memcpy(sOamAnchor,p->anchor,sizeof(sOamAnchor));
    memcpy(sOamExt,p->oamExt,sizeof(sOamExt));PresentMarks(p->oamMarks,false);
    sExtInUse=p->extInUse;sOamExtMarked=true;
    sStageRequested=sStageFadedRequested=sStageZoomedRequested=sTransitionRequested=false;
    sCentredRequested=CTR_CENTRED_NONE;sBattleRequested=true;
    sHoldTop=sFieldBanner=false;sTileBanksLive=0;
    CtrVideo_NotifyWrite(sMemory.vram,CTR_PRESENT_VRAM);
}

static void PresentCapture(void)
{
    if(!sPresent.asked || !sBattle || sPresent.bound || !sPresent.capture) return;
    CtrBattlePresentFrame *p=sPresent.capture;
    memset(p,0,sizeof(*p));p->magic=CTR_PRESENT_MAGIC;p->frame=sStats.frames+1;
    p->flags=(CtrSettings_Voxel() && CtrSettings_VoxelBattle()?CTR_PRESENT_VOXEL:0)
        | (CtrSettings_VoxelBlur()?CTR_PRESENT_BLUR:0) | (CtrBattleBg_MoveBgShown()?CTR_PRESENT_MOVE_BG:0)
        | (CtrBattleIntro_Sliding()?CTR_PRESENT_INTRO:0) | (sHud?CTR_PRESENT_HD:0);
    memcpy(p->vram,sMemory.vram,sizeof(p->vram));memcpy(p->regs,sMemory.regs,sizeof(p->regs));
    memcpy(p->palette,sMemory.palette,sizeof(p->palette));memcpy(p->unfaded,gPlttBufferUnfaded,sizeof(p->unfaded));
    memcpy(p->oam,sMemory.oam,sizeof(p->oam));memcpy(p->anchored,sOamAnchored,sizeof(p->anchored));
    memcpy(p->anchor,sOamAnchor,sizeof(p->anchor));memcpy(p->extPalette,sExtPltt,sizeof(p->extPalette));
    memcpy(p->oamExt,sOamExt,sizeof(p->oamExt));memcpy(p->scroll,sLineScroll,sizeof(p->scroll));
    PresentMarks(p->oamMarks,true);p->extInUse=sExtInUse;p->lineMask=sLineMask;p->lineCount=sLineCount;
    p->lineRegs=sLineRegs!=NULL;if(sLineRegs) memcpy(p->scanlines,sLineRegs,sizeof(p->scanlines));
    p->boxSlide=sBoxSlide;p->boxHidden=sBoxHidden;p->boxDecided=sBoxDecided;p->boxBarsUp=sBoxBarsUp;
    p->blurStrength=CtrSettings_VoxelBlurStrength();
    p->borderFollowsEdge=CtrSettings_GetInt("voxel_border_edge",1)!=0;
    p->measureGlow=CtrSettings_GetInt("measure_glow",1)!=0;
    CtrBattleHud_PresentCapture(p->hud);CtrVoxel_PresentCapture(&p->world);
    sPresent.ready=CtrBattlePresent_Validate(p);
}

static void PresentShutdown(void)
{
    PresentRestore();CtrBattlePresent_Bind(NULL);
    sPresent.active=false;
    if(sPresent.owned) PresentCachesRetire();
    free(sPresent.capture);free(sPresent.left);free(sPresent.right);free(sPresent.display);free(sPresent.queued);
    memset(&sPresent,0,sizeof(sPresent));
}
