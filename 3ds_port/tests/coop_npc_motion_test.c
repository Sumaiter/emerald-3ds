#define CTR_COOP_CUSTOM_NPC_MOTION_HELPERS
#define main CoopSessionOriginalMain
#include "coop_session_test.c"
#undef main

static uint8_t sMotionAction[OBJECT_EVENTS_COUNT], sMotionFrames[OBJECT_EVENTS_COUNT], sMotionDirection[OBJECT_EVENTS_COUNT];
static unsigned sMotionSteps, sMotionPixels, sMotionRewards;
bool CtrCoop_EngineNpcMotionCapture(uint8_t group,uint8_t num,uint8_t id,CtrCoopNpcMotionPose *pose)
{
    uint8_t index;
    if (group!=sSaveBlock1.location.mapGroup || num!=sSaveBlock1.location.mapNum
     || TryGetObjectEventIdByLocalIdAndMap(id,num,group,&index) || gObjectEvents[index].isPlayer || id>=254 || id==126) return false;
    const struct ObjectEvent *o=&gObjectEvents[index];
    *pose=(CtrCoopNpcMotionPose){.group=group,.num=num,.id=id,.x=o->currentCoords.x,.y=o->currentCoords.y,
        .facing=o->facingDirection,.graphics=o->graphicsId,.action=sMotionAction[index],.flags=o->heldMovementActive?1:0};
    return true;
}
int CtrCoop_EngineNpcMotionApply(const CtrCoopNpcMotionPose *pose,CtrCoopNpcMotionPlayback *state,bool ending)
{
    uint8_t index;
    if (TryGetObjectEventIdByLocalIdAndMap(pose->id,pose->num,pose->group,&index)) return -1;
    struct ObjectEvent *o=&gObjectEvents[index];
    if (!state->attached) {state->attached=true;state->wasFrozen=o->frozen;}
    if (sMotionFrames[index])
    {
        sMotionPixels+=8;
        if (!--sMotionFrames[index])
        {
            uint8_t d=sMotionDirection[index];
            o->currentCoords.x+=d==4?1:d==3?-1:0;
            o->currentCoords.y+=d==1?1:d==2?-1:0;
            o->heldMovementActive=false;
        }
        return 0;
    }
    if (pose->x!=o->currentCoords.x || pose->y!=o->currentCoords.y)
    {
        sMotionDirection[index]=pose->x<o->currentCoords.x?3:pose->x>o->currentCoords.x?4:pose->y<o->currentCoords.y?2:1;
        sMotionFrames[index]=2;o->heldMovementActive=true;o->frozen=true;++sMotionSteps;
        return 0;
    }
    o->facingDirection=pose->facing;
    if (ending) o->frozen=state->wasFrozen;
    state->playedAction=pose->actionSerial;
    return 1;
}

struct MotionEndpoint {
    __typeof__(sCoop) coop;struct SaveBlock1 save;struct ObjectEvent objects[OBJECT_EVENTS_COUNT];
    struct NpcMotionLocal tx[OBJECT_EVENTS_COUNT];struct NpcMotionRemote rx[OBJECT_EVENTS_COUNT];
    uint32_t txToken,txSequence,rxToken,rxSequence;uint8_t cursor;bool txEnding,rxClosed;
    struct EncounterKey actors[OBJECT_EVENTS_COUNT];uint8_t count;bool overflow;
    uint8_t action[OBJECT_EVENTS_COUNT],frames[OBJECT_EVENTS_COUNT],direction[OBJECT_EVENTS_COUNT];
    unsigned steps,pixels,rewards;
    struct StoryObject story[STORY_OBJECT_LIMIT];unsigned storyCount;bool storyOverflow;
    uint8_t touchedFlags[NUM_FLAG_BYTES],touchedVars[sizeof(sStoryVarsTouched)];uint32_t epoch,prepared;
};
static void MotionStore(struct MotionEndpoint *e)
{
    e->coop=sCoop;e->save=sSaveBlock1;memcpy(e->objects,gObjectEvents,sizeof(e->objects));
    memcpy(e->tx,sNpcMotionLocal,sizeof(e->tx));memcpy(e->rx,sNpcMotionRemote,sizeof(e->rx));
    e->txToken=sNpcMotionTxToken;e->txSequence=sNpcMotionTxSequence;e->rxToken=sNpcMotionRxToken;e->rxSequence=sNpcMotionRxSequence;
    e->cursor=sNpcMotionCursor;e->txEnding=sNpcMotionTxEnding;e->rxClosed=sNpcMotionRxClosed;
    memcpy(e->actors,sEncounterActors,sizeof(e->actors));e->count=sEncounterActorCount;e->overflow=sEncounterActorsOverflow;
    memcpy(e->action,sMotionAction,sizeof(e->action));memcpy(e->frames,sMotionFrames,sizeof(e->frames));memcpy(e->direction,sMotionDirection,sizeof(e->direction));
    e->steps=sMotionSteps;e->pixels=sMotionPixels;e->rewards=sMotionRewards;
    memcpy(e->story,sStoryObjects,sizeof(e->story));e->storyCount=sStoryObjectCount;e->storyOverflow=sStoryObjectOverflow;
    memcpy(e->touchedFlags,sStoryFlagsTouched,sizeof(e->touchedFlags));memcpy(e->touchedVars,sStoryVarsTouched,sizeof(e->touchedVars));
    e->epoch=sStoryEpoch;e->prepared=sStoryPreparedEpoch;
}
static void MotionLoad(const struct MotionEndpoint *e)
{
    sCoop=e->coop;sSaveBlock1=e->save;memcpy(gObjectEvents,e->objects,sizeof(e->objects));
    memcpy(sNpcMotionLocal,e->tx,sizeof(e->tx));memcpy(sNpcMotionRemote,e->rx,sizeof(e->rx));
    sNpcMotionTxToken=e->txToken;sNpcMotionTxSequence=e->txSequence;sNpcMotionRxToken=e->rxToken;sNpcMotionRxSequence=e->rxSequence;
    sNpcMotionCursor=e->cursor;sNpcMotionTxEnding=e->txEnding;sNpcMotionRxClosed=e->rxClosed;
    memcpy(sEncounterActors,e->actors,sizeof(e->actors));sEncounterActorCount=e->count;sEncounterActorsOverflow=e->overflow;
    memcpy(sMotionAction,e->action,sizeof(e->action));memcpy(sMotionFrames,e->frames,sizeof(e->frames));memcpy(sMotionDirection,e->direction,sizeof(e->direction));
    sMotionSteps=e->steps;sMotionPixels=e->pixels;sMotionRewards=e->rewards;
    memcpy(sStoryObjects,e->story,sizeof(e->story));sStoryObjectCount=e->storyCount;sStoryObjectOverflow=e->storyOverflow;
    memcpy(sStoryFlagsTouched,e->touchedFlags,sizeof(e->touchedFlags));memcpy(sStoryVarsTouched,e->touchedVars,sizeof(e->touchedVars));
    sStoryEpoch=e->epoch;sStoryPreparedEpoch=e->prepared;gMain.callback2=CB2_Overworld;
}
static void MotionReadyFixture(void)
{
    ReadyHost();CtrCoopNpcMotion_AfterStackReset();CtrCoop_ClearStoryWorld();
    sSaveBlock1.location.mapGroup=0;sSaveBlock1.location.mapNum=10;
    memset(gObjectEvents,0,sizeof(gObjectEvents));memset(sMotionFrames,0,sizeof(sMotionFrames));
    memset(sMotionAction,0xFF,sizeof(sMotionAction));sMotionSteps=sMotionPixels=sMotionRewards=0;
    sEncounterActorCount=0;sEncounterActorsOverflow=false;gMain.callback2=CB2_Overworld;
    for(unsigned i=1;i<=2;++i) gObjectEvents[i]=(struct ObjectEvent){.active=true,.localId=i+1,.mapGroup=0,.mapNum=10,
        .graphicsId=1,.facingDirection=1,.currentCoords={20,21}};
    ResetStoryBaseline();ClearPackets();
}
static void MotionBegin(bool guest)
{
    sCoop.guest=guest;sCoop.authenticated=sCoop.saveReady=true;
    CtrCoop_ScriptObject(0,10,2);CtrCoop_ScriptBegin((void *)0x1234);
}
static void MotionExchange(struct MotionEndpoint *a,struct MotionEndpoint *b)
{
    static struct SentPacket packets[MAX_SENT_PACKETS], replies[MAX_SENT_PACKETS];
    MotionLoad(a);ClearPackets();CtrCoop_StoryTick();MotionStore(a);
    int count=sSentCount;memcpy(packets,sSent,count*sizeof(packets[0]));
    MotionLoad(b);ClearPackets();
    for(int i=0;i<count;++i) CHECK(CtrCoop_HandlePacket(PEER_ID,packets[i].data,packets[i].len),"dispatcher accepts motion and ordered event packets");
    CtrCoop_StoryTick();MotionStore(b);
    count=sSentCount;memcpy(replies,sSent,count*sizeof(replies[0]));
    MotionLoad(a);ClearPackets();
    for(int i=0;i<count;++i) CHECK(CtrCoop_HandlePacket(PEER_ID,replies[i].data,replies[i].len),"reverse dispatcher accepts motion ACK-free stream");
    MotionStore(a);
}
static void TestGuideBothRoles(void)
{
    static struct MotionEndpoint host,guest;
    /* Actual Oldale EmployeeSouth: left, up*2, right, up*5, face down. */
    static const uint8_t south[]={3,2,2,4,2,2,2,2,2};
    for(unsigned role=0;role<2;++role) for(unsigned variant=0;variant<3;++variant)
    {
        MotionReadyFixture();MotionStore(&host);sCoop.guest=true;sCoop.authenticated=sCoop.saveReady=true;MotionStore(&guest);
        struct MotionEndpoint *owner=role?&guest:&host,*other=role?&host:&guest;
        MotionLoad(owner);MotionBegin(role!=0);
        if(role)
        {
            CHECK(!CtrCoop_ScriptPoll(),"guest guide waits for NPC lease");struct SentPacket request=*LastPacket(CTR_COOP_EVENT_REQUEST_PACKET);
            MotionStore(owner);MotionLoad(other);ClearPackets();CtrCoop_HandlePacket(PEER_ID,request.data,request.len);
            struct SentPacket grant=*LastPacket(CTR_COOP_EVENT_RESULT_PACKET);MotionStore(other);MotionLoad(owner);
            CtrCoop_HandlePacket(PEER_ID,grant.data,grant.len);
        }
        CHECK(CtrCoop_ScriptPoll()==1,"guide owner alone starts script");CtrCoop_ScriptActor(0,10,2);MotionStore(owner);
        unsigned steps=variant==0?sizeof(south):7;
        for(unsigned step=0;step<steps;++step)
        {
            uint8_t d=variant==0?south[step]:2;
            MotionLoad(owner);gObjectEvents[1].heldMovementActive=true;sMotionAction[1]=8+d-1;
            CtrCoopNpcMotion_Started(0,10,2);
            gObjectEvents[1].currentCoords.x+=d==4?1:d==3?-1:0;gObjectEvents[1].currentCoords.y+=d==1?1:d==2?-1:0;
            gObjectEvents[1].facingDirection=d;MotionStore(owner);
            for(unsigned frame=0;frame<4;++frame) MotionExchange(owner,other);
            MotionLoad(other);
            CHECK(gObjectEvents[1].currentCoords.y<21 || gObjectEvents[1].currentCoords.x!=20,
                "other endpoint sees movement during guide, before permanent XY or script end");
            CHECK(!sMotionRewards,"remote visual processing never grants another potion");
            CHECK(!NpcFromObject(1,&sCoop.npcCache[1]),"host cannot echo a stale snapshot for a guest-controlled actor");
            CHECK(NpcFromObject(2,&sCoop.npcCache[2]),"independent NPC snapshots remain enabled");
            int16_t x=gObjectEvents[1].currentCoords.x,y=gObjectEvents[1].currentCoords.y;
            CtrCoopNpcSnapshot stale={.mapGroup=0,.mapNum=10,.localId=2,.x=20,.y=21,.facing=1,.graphicsId=1};
            ApplyHostNpc(&stale);CHECK(gObjectEvents[1].currentCoords.x==x && gObjectEvents[1].currentCoords.y==y,"legacy snapshots cannot reset live playback");
            MotionStore(other);
        }
        MotionLoad(owner);gObjectEvents[1].heldMovementActive=false;gObjectEvents[1].facingDirection=1;sMotionAction[1]=0xFF;
        ++sMotionRewards;CtrCoop_ScriptEnd();MotionStore(owner);
        for(unsigned frame=0;frame<100 && (owner->coop.eventState!=EVENT_IDLE || other->rx[0].used);++frame) MotionExchange(owner,other);
        for(unsigned frame=0;frame<12;++frame) MotionExchange(owner,other);
        MotionLoad(other);
        CHECK(gObjectEvents[1].currentCoords.x==20 && gObjectEvents[1].currentCoords.y==14 && gObjectEvents[1].facingDirection==1,
            "all three guide routes end at native mart coordinates in either role");
        CHECK(!CtrCoopNpcMotion_Controls(0,10,2) && !CtrCoopNpcMotion_Settling() && sMotionSteps>=7 && sMotionPixels>=112,
            "end marker drains animated tail then releases snapshot protection");
        CHECK(owner->rewards==1 && !sMotionRewards,"one potion effect stays with exactly one executing endpoint");
    }
}
static void TestMotionLifecycleAndValidation(void)
{
    MotionReadyFixture();MotionBegin(false);CHECK(CtrCoop_ScriptPoll()==1,"host lease");
    CtrCoopNpcMotion_Started(0,10,2);gObjectEvents[1].currentCoords.y=20;
    sStoryTrySendBlocked=true;CtrCoop_ScriptEnd();
    for(unsigned i=0;i<30;++i) CtrCoop_StoryTick();
    CHECK(sCoop.eventState==EVENT_HELD && CtrCoopNpcMotion_Pending() && !sNpcMotionTxSequence,
        "backpressure preserves latest pose and ordered end before lease release");
    CHECK(!NpcFromObject(1,&sCoop.npcCache[1]),"host legacy sender cannot teleport an actor ahead of its blocked first motion pose");
    sStoryTrySendBlocked=false;ClearPackets();CtrCoop_StoryTick();
    CHECK(CountPacket(76)==1 && CountPacket(77)==1 && !CtrCoopNpcMotion_Pending(),"recovery sends final coalesced pose and end exactly once");
    struct SentPacket pose=*LastPacket(76),end=*LastPacket(77);
    MotionPut32(pose.data+2,500);MotionPut32(end.data+2,500);
    CHECK(LastPacket(76)<LastPacket(77),"final pose is queued before end marker");
    MotionReadyFixture();sCoop.guest=true;sCoop.authenticated=sCoop.saveReady=true;
    uint8_t invalid[CTR_COOP_NPC_MOTION_SIZE];memcpy(invalid,pose.data,sizeof(invalid));invalid[12]=254;
    CtrCoopNpcMotion_HandlePacket(PEER_ID,invalid,sizeof(invalid));
    invalid[12]=255;CtrCoopNpcMotion_HandlePacket(PEER_ID,invalid,sizeof(invalid));
    invalid[12]=126;CtrCoopNpcMotion_HandlePacket(PEER_ID,invalid,sizeof(invalid));
    invalid[12]=2;invalid[15]=0x80;CtrCoopNpcMotion_HandlePacket(PEER_ID,invalid,sizeof(invalid));
    CHECK(!CtrCoopNpcMotion_Controls(0,10,0),"player, follower and invalid coordinate packets are discarded");
    CtrCoopNpcMotion_HandlePacket(PEER_ID,pose.data,pose.len);
    CtrCoopNpcMotion_HandlePacket(PEER_ID,end.data,end.len);
    uint32_t seq=sNpcMotionRxSequence;CtrCoopNpcMotion_HandlePacket(PEER_ID,pose.data,pose.len);
    CHECK(sNpcMotionRxSequence==seq && CtrCoopNpcMotion_Settling(),"stale pose cannot reopen a closed stream");
    gMain.callback2=NULL;CtrCoop_StoryTick();
    CHECK(gObjectEvents[1].currentCoords.y==21 && CtrCoopNpcMotion_Settling(),"battle/menu sprite pool is never mutated and retains latest accepted target");
    gMain.callback2=CB2_Overworld;
    CtrCoopNpcMotion_Reset();sCoop.wanted=false;
    sCoop.guest=false;sCoop.savePrepared=false;ClearPackets();PrepareHostSave(PEER_ID);
    CHECK(!sCoop.savePrepared && !CountPacket(CTR_COOP_SAVE_BEGIN_PACKET),"new guest snapshot waits for previous accepted visual tail");
    for(unsigned i=0;i<10;++i) CtrCoop_StoryTick();
    CHECK(!CtrCoopNpcMotion_Settling() && gObjectEvents[1].currentCoords.y==20,"disconnect settles accepted native tail with wanted false");
    PrepareHostSave(PEER_ID);
    CHECK(sCoop.savePrepared && CountPacket(CTR_COOP_SAVE_BEGIN_PACKET)==1,"host snapshot proceeds once the actor is settled");
    CHECK(!sNpcMotionRxToken && !sNpcMotionRxSequence,"disconnect retires old session serial watermark after accepting its tail");
    sCoop.wanted=true;sCoop.guest=false;sCoop.peerAuthenticated=sCoop.peerSaveReady=true;
    sCoop.peerEventToken=1;sCoop.peerEventCount=1;sCoop.peerEventKeys[0]=(struct EncounterKey){0,10,2,0};
    pose.data[2]=1;pose.data[3]=pose.data[4]=pose.data[5]=0;MotionPut32(pose.data+6,1);pose.data[16]=19;
    CtrCoopNpcMotion_HandlePacket(PEER_ID,pose.data,pose.len);
    CHECK(CtrCoopNpcMotion_Controls(0,10,2) && sNpcMotionRxToken==1,"new guest process token one is accepted after previous session");
    CtrCoopNpcMotion_HandlePacket(PEER_ID,end.data,end.len);
    CHECK(!sNpcMotionRxClosed && sNpcMotionRxToken==1,"old session token 500 cannot close a fresh guest token-one stream");
    CtrCoopNpcMotion_AfterStackReset();CHECK(!sNpcMotionRxToken && !CtrCoopNpcMotion_Controls(0,10,0),"real world reset discards every motion generation");

    MotionReadyFixture();sCoop.guest=true;sCoop.authenticated=sCoop.saveReady=true;
    CtrCoopNpcMotion_HandlePacket(PEER_ID,pose.data,pose.len);
    sSaveBlock1.location.mapNum=11;CtrCoop_StoryTick();
    CHECK(!CtrCoopNpcMotion_Controls(0,10,2) && gObjectEvents[1].currentCoords.y==21,"map change never applies a previous-map actor to reused slots");
    MotionReadyFixture();MotionBegin(true);CHECK(!CtrCoop_ScriptPoll(),"prepared/pending cannot publish motion");
    CtrCoopNpcMotion_Started(0,10,2);CtrCoop_ScriptEnd();ClearPackets();CtrCoopNpcMotion_Tick();
    CHECK(!CountPacket(76) && !CountPacket(77),"ungranted cancellation has no movement side effects or end packets");
}
int main(void)
{
    TestGuideBothRoles();TestMotionLifecycleAndValidation();
    printf("NPC motion regressions: %d failures\n",sFailures);return sFailures?1:0;
}
