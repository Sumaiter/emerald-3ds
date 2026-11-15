/* Real packet dispatcher + follow trail + remote playback across burst turns.
 * Native coordinate timing: a step changes currentCoords on its FIRST frame. */
#define CTR_COOP_CUSTOM_MOVEMENT
#define main OriginalSessionMain
#include "coop_session_test.c"
#undef main

static unsigned starts, snaps, remaining;
static uint8_t actions[32];
void ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *o) { (void)o; }
bool ObjectEventIsHeldMovementActive(struct ObjectEvent *o) { return o->heldMovementActive; }
void ObjectEventSetHeldMovement(struct ObjectEvent *o,uint8_t action)
{
    CHECK(!o->heldMovementActive,"never replace an unfinished native step");
    CHECK(starts<sizeof(actions),"bounded playback");actions[starts++]=action;
    unsigned direction=(action-CTR_COOP_ACTION_PLAYER_RUN_DOWN)%4+1;
    o->currentCoords.x+=(direction==4)-(direction==3);
    o->currentCoords.y+=(direction==1)-(direction==2);
    o->heldMovementActive=1;remaining=8;
}
void MoveObjectEventToMapCoords(struct ObjectEvent *o,int16_t x,int16_t y)
{++snaps;o->currentCoords.x=x;o->currentCoords.y=y;}

static void ReceivePose(int x,int y,unsigned direction)
{
    CtrCoopSnapshot pose=RemoteAt(x,y);
    uint8_t p[CTR_COOP_PACKET_SIZE];pose.facing=direction;
    pose.movementActionId=CTR_COOP_ACTION_PLAYER_RUN_DOWN+direction-1;
    CHECK(CtrCoopProtocol_Encode(&pose,p,sizeof(p)),"encode visited run tile");
    CtrCoop_HandlePacket(PEER_ID,p,sizeof(p));
}

int main(void)
{
    ReadyHost();gMain.callback2=CB2_Overworld;
    sCoop.remoteValid=true;sCoop.remote=RemoteAt(27,23);
    CtrCoopSnapshot local;CHECK(LocalSnapshot(&local),"local player ready");
    UpdateRemoteObject(&local);CHECK(RemoteObjectValid(),"remote actor spawned");
    struct ObjectEvent *remote=&gObjectEvents[sCoop.objectEventId];
    starts=snaps=0;
    /* Three packets delivered by ONE pump, before any render or FollowTick. */
    sFollow.state=FOLLOW_FOLLOWER;sFollow.haveLast=false;sFollow.count=0;
    FollowRecord(&sCoop.remote);
    ReceivePose(28,23,4);ReceivePose(28,24,1);ReceivePose(29,24,4);
    CHECK(sFollow.count==4,"all corner waypoints retained before the field tick");
    CHECK(sRemoteSteps.count==3,"all received motion targets retained");
    for(unsigned frame=0;frame<26;++frame)
    {
        if(remaining && --remaining==0)remote->heldMovementActive=0;
        UpdateRemoteObject(&local);
        if(frame<8)CHECK(remote->currentCoords.x==28 && remote->currentCoords.y==23,"first straight step completes before turning");
        if(frame>=8 && frame<16)CHECK(remote->currentCoords.x==28 && remote->currentCoords.y==24,"corner is visited, never cut diagonally");
    }
    CHECK(starts==3 && !snaps,"three smooth native steps and zero teleports");
    CHECK(actions[0]==0x38 && actions[1]==0x35 && actions[2]==0x38,"source directions and running speed replayed exactly");
    CHECK(remote->currentCoords.x==29 && remote->currentCoords.y==24,"final remote tile correct");
    /* Follow input uses the queued run action, with a single occupied tile gap. */
    gObjectEvents[0].currentCoords.x=27;gObjectEvents[0].currentCoords.y=23;
    uint16_t pressed=0,held=0;CtrCoop_FollowKeys(&pressed,&held);
    CHECK(held==0x10 && CtrCoop_FollowRunning(),"follower matches the leader's run even with its own B released");
    gObjectEvents[0].currentCoords.x=28;CtrCoop_FollowKeys(&pressed,&held);
    CHECK(held==0x80,"follower takes the same corner");
    gObjectEvents[0].currentCoords.y=24;CtrCoop_FollowKeys(&pressed,&held);
    CHECK(!held,"one tile behind leader, without walking onto it");
    gMain.inBattle=true;ReceivePose(30,24,4);
    CHECK(!sRemoteSteps.count,"hidden movement is not replayed after returning from a battle/menu");
    gMain.inBattle=false;
    ClearRemote();CHECK(!sRemoteSteps.count && !sFollow.count,"disconnect discards old movement queues");
    ReadyHost();CHECK(LocalSnapshot(&local),"snapshot retry setup");
    struct LocalPeer peer={.id=PEER_ID};sCoop.haveLastSent=false;sCoop.framesSinceLast=60;
    sStoryTrySendBlocked=true;SendLocalSnapshot(&local,&peer);
    CHECK(!sCoop.haveLastSent && sCoop.framesSinceLast==60,"queue rejection does not acknowledge an unsent position");
    sStoryTrySendBlocked=false;SendLocalSnapshot(&local,&peer);
    CHECK(sCoop.haveLastSent && !sCoop.framesSinceLast,"position retries immediately when the queue resumes");
    if(sFailures)return 1;
    puts("burst turns: exact tile trail, native step completion, run matching, no teleports and one-tile follow gap passed");return 0;
}
