/* Actual merged backend: final team handoff, retry and safe disconnect. */
#define main CoopSessionOriginalMain
#include "coop_session_test.c"
#undef main

static void LogoutBegin(const struct Pokemon *party)
{
    uint8_t begin[CTR_COOP_SAVE_BEGIN_SIZE];
    CtrCoopProtocol_EncodeLogoutBegin(sizeof(sLogoutReceiveData),
        CtrCoopProtocol_Checksum((const uint8_t *)party, sizeof(sLogoutReceiveData)), begin, sizeof(begin));
    CtrCoop_HandlePacket(PEER_ID, begin, sizeof(begin));
}

static void TestRetryAndCommit(void)
{
    struct Pokemon party[PARTY_SIZE];
    uint8_t packet[CTR_COOP_SAVE_CHUNK_MAX];
    ReadyHost();
    for (unsigned i=0;i<PARTY_SIZE;++i) { SetMon(&party[i],70+i);party[i].data[0]=i+17; }
    LogoutBegin(party);
    CHECK(sCoop.logoutState==LOGOUT_STATE_RECEIVING,"logout admitted");
    CHECK(!CtrCoop_RequestPC(),"host cannot acquire PC during row transfer");
    CtrCoopProtocol_EncodeLogoutChunk(0,(const uint8_t *)party,128,packet,CTR_COOP_SAVE_CHUNK_MAX);
    CtrCoop_HandlePacket(PEER_ID,packet,CTR_COOP_SAVE_CHUNK_MAX);
    CHECK(sCoop.logoutReceiveOffset==128,"partial chunk progresses");
    LogoutBegin(party);
    CHECK(sCoop.logoutReceiveOffset==128,"duplicate BEGIN resumes rather than resetting partial team");
    ClearPackets();
    for(unsigned i=0;i<30;++i) TickLogout();
    CHECK(LastPacket(CTR_COOP_LOGOUT_REQUEST_PACKET)!=NULL,"host retries a lost request");
    sStoryTrySendBlocked=true;
    SendTransfer(NULL,CtrCoopProtocol_EncodeLogoutChunk,(const uint8_t *)party,sizeof(party),0,false,128);
    CHECK(sCoop.peerLoggedOut && sDisconnectSavePending,"complete logout atomically commits and schedules save");
    CHECK(memcmp(sRemoteParty,party,sizeof(party))==0,"complete party payload retained");
    CHECK(!CtrCoop_StorageSlotLocked(CTR_COOP_STORAGE_BOX_ID,CTR_COOP_STORAGE_ROW_SIZE),"complete team unlocks guest row");
    sStoryTrySendBlocked=false;ClearPackets();LogoutBegin(party);
    CHECK(LastPacket(CTR_COOP_LOGOUT_READY_PACKET)!=NULL,"lost final ACK is replayed for matching BEGIN");
    struct Pokemon oldParty[PARTY_SIZE];memcpy(oldParty,party,sizeof(party));oldParty[0].box.species=2;
    SendTransfer(CtrCoopProtocol_EncodePartyBegin,CtrCoopProtocol_EncodePartyChunk,
        (const uint8_t *)oldParty,sizeof(oldParty),CtrCoopProtocol_Checksum((const uint8_t *)oldParty,sizeof(oldParty)),true,0);
    CHECK(memcmp(sRemoteParty,party,sizeof(party))==0,"late party packets cannot overwrite committed logout");
    gMain.callback2=CB2_Overworld;TickDisconnectSave();
    CHECK(sAutoSaves==1 && !sDisconnectSavePending,"safe host persists final returned team exactly once");
}

static void TestBusyAndTimeout(void)
{
    struct Pokemon party[PARTY_SIZE];
    ReadyHost();memcpy(party,gPlayerParty,sizeof(party));
    sCoop.pcOpen=true;sCoop.pcOwner=PC_OWNER_HOST;LogoutBegin(party);
    CHECK(sCoop.logoutState==LOGOUT_STATE_IDLE && sCoop.pcOpen,"logout cannot replace cursor-owned PC state");
    sCoop.pcOpen=false;sCoop.pcOwner=PC_OWNER_NONE;LogoutBegin(party);
    for(unsigned i=0;i<600;++i) TickLogout();
    CHECK(sCoop.logoutState==LOGOUT_STATE_IDLE && !sCoop.peerLoggedOut,"partial timeout never commits");
    CHECK(CtrCoop_StorageSlotLocked(CTR_COOP_STORAGE_BOX_ID,CTR_COOP_STORAGE_ROW_SIZE),"partial timeout does not unlock connected guest row");
    CHECK(!sDisconnectSavePending,"partial payload cannot schedule save");
    CtrCoop_PeerLeft(PEER_ID);
    CHECK(!CtrCoop_StorageSlotLocked(CTR_COOP_STORAGE_BOX_ID,CTR_COOP_STORAGE_ROW_SIZE),"actual disconnect releases stale guest lock");

    ReadyHost();gMain.callback2=CB2_InitBattle;gMain.inBattle=true;sCoop.battleActive=true;
    CtrCoop_PeerLeft(PEER_ID);
    CHECK(sCoop.battleActive && gMain.callback2==CB2_InitBattle && !sMainCallback,
          "guest departure cannot terminate an independent host battle");
}

static void ReadyLogoutGuest(void)
{
    ReadyHost();sCoop.guest=true;sCoop.authenticated=true;sCoop.saveReady=true;
    sCoop.initialTeamCommitted=true;sCoop.spawnValid=true;sCoop.guestWorldLoaded=true;
    sCoop.peerId=PEER_ID;gMain.callback2=CB2_Overworld;
}

static void TestGuestCompletion(void)
{
    ReadyLogoutGuest();CtrCoop_Stop();
    CHECK(CtrCoop_Leaving() && sCoop.authenticated,"safe guest waits for final row ACK");
    CHECK(LastPacket(CTR_COOP_LOGOUT_BEGIN_PACKET)!=NULL,"guest initiates final snapshot");
    uint8_t request[CTR_COOP_SAVE_REQUEST_SIZE];
    CtrCoopProtocol_EncodeLogoutRequest(0,request,sizeof(request));ClearPackets();
    CtrCoop_HandlePacket(PEER_ID+1,request,sizeof(request));
    CHECK(!LastPacket(CTR_COOP_LOGOUT_CHUNK_PACKET),"unrelated peer cannot pull final team");
    CtrCoop_HandlePacket(PEER_ID,request,sizeof(request));
    CHECK(LastPacket(CTR_COOP_LOGOUT_CHUNK_PACKET)!=NULL,"authenticated host pulls frozen team");
    EXPECT_GUEST_MENU(CtrCoop_HandlePacket(PEER_ID,(const uint8_t[]){CTR_COOP_LOGOUT_READY_PACKET,CTR_COOP_PROTOCOL_VERSION},2));
    CHECK(!CtrCoop_Leaving() && !sCoop.guestWorldLoaded,"ACK exits through complete stack reset");

    ReadyLogoutGuest();CtrCoop_Stop();
    EXPECT_GUEST_MENU(for(unsigned i=0;i<600;++i) TickLogout());
    CHECK(!CtrCoop_Leaving(),"unreachable host cannot trap guest in logout forever");
    ReadyLogoutGuest();gMain.inBattle=true;sCoop.battleActive=true;
    EXPECT_GUEST_MENU(CtrCoop_Stop());
    CHECK(!LastPacket(CTR_COOP_LOGOUT_BEGIN_PACKET),"combat/temporary team cannot become final canonical row");
}

int main(void)
{
    TestRetryAndCommit();TestBusyAndTimeout();TestGuestCompletion();
    if(sFailures)return 1;
    puts("logout: atomic row, retries/ACK replay, stale packets, PC/save guards, guest menu and independent host battle passed");
    return 0;
}
