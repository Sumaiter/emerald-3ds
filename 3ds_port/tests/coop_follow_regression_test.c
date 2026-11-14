/* Exercise the real session/backend, including packet authentication and the
 * ordinary-battle fallback. Engine movement/loss bodies have separate tests. */
#define main OriginalSessionMain
#include "coop_session_test.c"
#undef main

static void FollowPacket(uint8_t op,uint32_t token,bool ready)
{
    uint8_t p[CTR_COOP_FOLLOW_SIZE]={CTR_COOP_FOLLOW_PACKET,CTR_COOP_PROTOCOL_VERSION,op,ready};
    WriteDouble32(p,4,token);
    CtrCoop_HandlePacket(PEER_ID,p,sizeof(p));
}

static void FreeField(void)
{
    ReadyHost();
    gMain.callback2=CB2_Overworld;
    sCoop.remoteValid=true;sCoop.remote=RemoteAt(27,23);
}

static void JoinFollower(void)
{
    FreeField();
    CHECK(CtrCoop_TryFollowAt(27,23),"A targets free peer");
    FollowPacket(FOLLOW_ACCEPT,sFollow.token,true);
    CHECK(CtrCoop_IsFollowing() && sFollow.state==FOLLOW_FOLLOWER,"ACK establishes follower");
}

static void JoinLeader(void)
{
    FreeField();
    FollowPacket(FOLLOW_JOIN,41,true);
    CHECK(sFollow.state==FOLLOW_LEADER && sFollow.token==41,"peer A makes local player leader");
}

static void TestJoinAndLeave(void)
{
    uint16_t pressed,held;
    FreeField();
    CHECK(!CtrCoop_TryFollowAt(27,24),"A on wrong tile does nothing");
    sControlsLocked=true;CHECK(!CtrCoop_TryFollowAt(27,23),"local locked actor cannot join");
    sControlsLocked=false;
    CHECK(CtrCoop_TryFollowAt(27,23),"free local player requests following");
    uint32_t token=sFollow.token;
    FollowPacket(FOLLOW_ACCEPT,token+1,true);
    CHECK(sFollow.state==FOLLOW_PENDING,"stale/foreign acceptance cannot join");
    pressed=2;held=2|0x10;CtrCoop_FollowKeys(&pressed,&held);
    CHECK(!CtrCoop_IsFollowing() && pressed==0 && held==0x10,"B cancels immediately, preserves held direction");
    FollowPacket(FOLLOW_ACCEPT,token,true);
    CHECK(!CtrCoop_IsFollowing(),"late acceptance after B cannot rejoin");
    JoinFollower();
    sStoryTrySendBlocked=true;pressed=held=2;CtrCoop_FollowKeys(&pressed,&held);
    CHECK(!CtrCoop_IsFollowing() && sFollow.dirtyStop,"B works under transport backpressure");
    sStoryTrySendBlocked=false;FollowTick();
    CHECK(!sFollow.dirtyStop && LastPacket(CTR_COOP_FOLLOW_PACKET)->data[2]==FOLLOW_STOP,"stop is delivered when queue recovers");
    FreeField();sCoop.pcOpen=true;FollowPacket(FOLLOW_JOIN,31,true);
    CHECK(sFollow.state==FOLLOW_NONE && LastPacket(CTR_COOP_FOLLOW_PACKET)->data[2]==FOLLOW_DENY,"busy PC owner rejects join");
    FreeField();CHECK(CtrCoop_TryFollowAt(27,23),"pending timeout setup");
    for(unsigned i=0;i<300;++i)FollowTick();
    CHECK(!CtrCoop_IsFollowing(),"missing peer answer cannot trap field input");
    JoinLeader();CtrCoop_PeerLeft(PEER_ID);
    CHECK(!FollowPaired() && sFollow.count==0,"disconnect removes travelling membership");
}

static void TestNoFollowingCycle(void)
{
    for(unsigned guest=0;guest<2;++guest)
    {
        FreeField();sCoop.guest=guest;
        if(guest){sCoop.authenticated=true;sCoop.saveReady=true;sCoop.initialTeamCommitted=true;}
        CHECK(CtrCoop_TryFollowAt(27,23),"simultaneous A setup %u",guest);
        FollowPacket(FOLLOW_JOIN,72,true);
        CHECK(sFollow.state==(guest?FOLLOW_LEADER:FOLLOW_PENDING),"simultaneous A has deterministic host priority %u",guest);
    }
}

static void TestVisitedTrail(void)
{
    uint16_t pressed=0,held=0;
    JoinFollower();
    CtrCoop_FollowKeys(&pressed,&held);CHECK(!held,"stay one tile behind stationary leader");
    sCoop.remote=RemoteAt(27,24);FollowTick();
    CtrCoop_FollowKeys(&pressed,&held);CHECK(held==0x10,"walk first visited tile, not diagonal shortcut");
    gObjectEvents[0].currentCoords.x=27;
    sCoop.remote=RemoteAt(28,24);FollowTick();
    CtrCoop_FollowKeys(&pressed,&held);CHECK(held==0x80,"turn along leader's visited corner");
    gObjectEvents[0].currentCoords.y=24;
    CtrCoop_FollowKeys(&pressed,&held);CHECK(!held,"keep gap after corner");
    sCoop.remote.mapNum=10;sCoop.remote.mapLayoutId=15;sCoop.remote.x=14;sCoop.remote.y=12;
    FollowTick();
    /* Finish the old-map tail before changing maps. */
    CtrCoop_FollowKeys(&pressed,&held);CHECK(held==0x10,"finish previous-map trail first");
    gObjectEvents[0].currentCoords.x=28;
    CtrCoop_FollowKeys(&pressed,&held);
    CHECK(sFollowWarpCalls==1 && sFollowWarpGroup==0 && sFollowWarpNum==10
       && sFollowWarpX==14 && sFollowWarpY==12,"map change uses bounded native transition helper");
}

static void TestExplicitBattleGrouping(void)
{
    FreeField();gBattleTypeFlags=BATTLE_TYPE_TRAINER;gTrainerBattleOpponent_A=8;
    CHECK(CtrCoop_BattleStart() && !LastPacket(CTR_COOP_DOUBLE_REQUEST_PACKET),"nearby non-group single remains solo");
    FreeField();gBattleTypeFlags=BATTLE_TYPE_DOUBLE|BATTLE_TYPE_TRAINER;gTrainerBattleOpponent_A=8;
    CHECK(CtrCoop_BattleStart() && !LastPacket(CTR_COOP_DOUBLE_REQUEST_PACKET),"nearby non-group authored double remains solo");
    JoinLeader();gBattleTypeFlags=BATTLE_TYPE_TRAINER;gTrainerBattleOpponent_A=8;
    CHECK(CtrCoop_RestrictStoryParty(),"travelling leader's native story selection is restricted to first three");
    CHECK(!CtrCoop_BattleStart() && LastPacket(CTR_COOP_DOUBLE_REQUEST_PACKET),"leader single offers joint double only to explicit follower");
    CHECK(ReadDouble32(LastPacket(CTR_COOP_DOUBLE_REQUEST_PACKET)->data,4)==41,"invitation binds travelling token");
    struct Pokemon peerParty[6];
    for(unsigned i=0;i<6;++i)SetMon(&peerParty[i],70+i);
    uint32_t checksum=CtrCoopProtocol_Checksum((const uint8_t *)peerParty,sizeof(peerParty));
    uint8_t accept[CTR_COOP_DOUBLE_CONTROL_SIZE]={CTR_COOP_DOUBLE_ACCEPT_PACKET,CTR_COOP_PROTOCOL_VERSION,41,0,0,0};
    WriteDouble32(accept,6,checksum);
    CtrCoop_HandlePacket(PEER_ID,accept,sizeof(accept));
    CHECK(sCoop.doubleState==DOUBLE_STATE_WAIT_ACCEPT,"joint setup waits for the follower's exact current party, not a stale join cache");
    SendTransfer(CtrCoopProtocol_EncodePartyBegin,CtrCoopProtocol_EncodePartyChunk,
        (const uint8_t *)peerParty,sizeof(peerParty),checksum,true,0);
    CHECK(sCoop.doubleState==DOUBLE_STATE_SENDING,"joined single prepares common encounter");
    CHECK(sRemotePartyValid && !memcmp(sRemoteParty,peerParty,sizeof(peerParty)),"complete party update refreshes future encounter cache");
    CHECK((ReadDouble32(sDoubleSendData,0)&(BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI|BATTLE_TYPE_LINK))
       ==(BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI|BATTLE_TYPE_LINK),"single is converted into two-player double");
    CHECK(ReadDouble32(sDoubleSendData,20)==BATTLE_TYPE_TRAINER,"solo original flags retained for correct native return");
    JoinLeader();sFollow.peerReady=false;gBattleTypeFlags=BATTLE_TYPE_TRAINER;
    CHECK(CtrCoop_BattleStart() && !LastPacket(CTR_COOP_DOUBLE_REQUEST_PACKET),"busy follower cannot be pulled from menu/PC");
    JoinFollower();gBattleTypeFlags=BATTLE_TYPE_TRAINER;
    CHECK(CtrCoop_BattleStart() && !LastPacket(CTR_COOP_DOUBLE_REQUEST_PACKET),"follower never invites leader into an independent trigger");
    JoinFollower();sCoop.doubleState=DOUBLE_STATE_RECEIVING;
    uint16_t pressed=2,held=2;CtrCoop_FollowKeys(&pressed,&held);
    CHECK(sCoop.doubleState==DOUBLE_STATE_IDLE && !CtrCoop_IsFollowing(),"B cancels a not-yet-started receive without freezing follower");
    JoinLeader();sCoop.doubleState=DOUBLE_STATE_SENDING;
    FollowPacket(FOLLOW_STOP,41,true);
    CHECK(sCoop.doubleState==DOUBLE_STATE_LOCAL && !FollowPaired(),"peer B cancels frozen setup and leader continues solo");
    CHECK(CtrCoop_BattleStart() && sCoop.battleActive,"cancelled setup starts original local encounter once");
}

static void TestStoryNpcOwnership(void)
{
    for(unsigned leaderGuest=0;leaderGuest<2;++leaderGuest)
    {
        static uint8_t payload[DOUBLE_SETUP_SIZE];
        struct Pokemon followerBefore[PARTY_SIZE],remoteBefore[PARTY_SIZE];
        FreeField();sCoop.guest=leaderGuest;sCoop.doubleInitiator=true;
        for(unsigned i=0;i<PARTY_SIZE;++i){SetMon(&gPlayerParty[i],10+i);SetMon(&sRemoteParty[i],30+i);}
        sRemotePartyValid=true;
        gBattleTypeFlags=BATTLE_TYPE_TRAINER|BATTLE_TYPE_DOUBLE|BATTLE_TYPE_TWO_OPPONENTS|BATTLE_TYPE_MULTI|BATTLE_TYPE_INGAME_PARTNER;
        PrepareDoubleSetup();memcpy(payload,sDoubleSendData,sizeof(payload));
        const struct Pokemon *composite=(const struct Pokemon *)(payload+DOUBLE_HEADER_SIZE+sizeof(gEnemyParty));
        CHECK(composite[leaderGuest?0:3].box.species==13,"follower receives native NPC team in role %u",leaderGuest);
        memcpy(remoteBefore,sRemoteParty,sizeof(remoteBefore));ConfigureDoubleBattle(payload);
        RestoreDoubleParty();CHECK(!memcmp(sRemoteParty,remoteBefore,sizeof(remoteBefore)),"NPC cannot overwrite actual follower cache");
        FreeField();sCoop.guest=!leaderGuest;
        for(unsigned i=0;i<PARTY_SIZE;++i)SetMon(&gPlayerParty[i],90+i);
        memcpy(followerBefore,gPlayerParty,sizeof(followerBefore));memcpy(sDoubleLocalParty,gPlayerParty,sizeof(sDoubleLocalParty));
        ConfigureDoubleBattle(payload);
        CHECK(CtrCoop_UsesNpcPartner(),"follower uses temporary NPC party");
        gPlayerParty[leaderGuest?0:3].hp=1;
        RestoreDoubleParty();
        CHECK(!memcmp(gPlayerParty,followerBefore,sizeof(followerBefore)),"all six own Pokemon survive temporary NPC control unchanged");
        gBattleOutcome=B_OUTCOME_LOST;
        ReturnFromRemoteDoubleBattle();
        CHECK(sMainCallback==CB2_ReturnToFieldFadeFromBlack && !memcmp(gPlayerParty,followerBefore,sizeof(followerBefore)),
              "borrowed-team defeat neither heals nor replaces the follower's real team");
        CtrCoop_FinishDoubleTransport();
        CHECK(!(gBattleTypeFlags&BATTLE_TYPE_INGAME_PARTNER),"follower returns without foreign special-party lifecycle");
    }
}

int main(void)
{
    setvbuf(stdout,NULL,_IONBF,0);
    TestJoinAndLeave();TestNoFollowingCycle();TestVisitedTrail();TestExplicitBattleGrouping();TestStoryNpcOwnership();
    FreeField();sCoop.doubleState=DOUBLE_STATE_ACTIVE;sCoop.doubleInitiator=false;
    CHECK(!CtrCoop_ApplyBlackoutMoney(),"non-initiator does not halve shared money again");
    sCoop.doubleInitiator=true;CHECK(CtrCoop_ApplyBlackoutMoney(),"initiator owns one shared defeat debit");
    if(sFailures){printf("%d follow regressions failed\n",sFailures);return 1;}
    puts("explicit follow group, B exit, trail, authenticated battle join, and NPC party ownership passed");
    return 0;
}
