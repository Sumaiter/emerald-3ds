/* Independent HP/disconnect/watch integration checks against the real backend.
 * The reusable session fixture supplies only engine/transport boundaries. */
#define main CoopSessionOriginalMain
#include "coop_session_test.c"
#undef main

static void LiveGuestParty(struct Pokemon party[PARTY_SIZE])
{
    memset(party,0,sizeof(struct Pokemon)*PARTY_SIZE);
    for(unsigned i=0;i<2;i++)
    {
        SetMon(&party[i],210+i);
        party[i].status=i?0:0x10;
        party[i].hp=i?0:37;
        party[i].maxHP=120;
        party[i].mail=4+i;
        /* PP/moves are part of BoxPokemon. Distinct non-full PP bytes must
         * survive every boxed/wire/save route along with the entire box. */
        for(unsigned j=0;j<4;j++)party[i].box.data[40+j]=(uint8_t)(i+j*3);
    }
}
static void LiveCheckParty(const struct Pokemon expected[PARTY_SIZE],const char *route)
{
    CHECK(gPlayerPartyCount==2,"%s restores exactly the two guest Pokemon",route);
    for(unsigned i=0;i<2;i++)
    {
        CHECK(gPlayerParty[i].hp==expected[i].hp,"%s preserves injured/fainted HP for slot %u",route,i);
        CHECK(gPlayerParty[i].status==expected[i].status,"%s preserves status for slot %u",route,i);
        CHECK(gPlayerParty[i].mail==expected[i].mail,"%s preserves mail for slot %u",route,i);
        CHECK(!memcmp(&gPlayerParty[i].box,&expected[i].box,sizeof(struct BoxPokemon)),
              "%s preserves the complete box payload including non-full PP for slot %u",route,i);
    }
}
static void TestLiveVitalsPersistence(void)
{
    struct Pokemon expected[PARTY_SIZE],ordinary,before;
    struct SaveBlock1 saved;
    struct PokemonStorage savedStorage;
    ReadyHost();LiveGuestParty(expected);
    StorePartyRow(CTR_COOP_STORAGE_GUEST_ROW,expected);
    memset(gPlayerParty,0xcc,sizeof(gPlayerParty));
    LoadPartyRow(CTR_COOP_STORAGE_GUEST_ROW,gPlayerParty);LiveCheckParty(expected,"row round trip");

    saved=sSaveBlock1;savedStorage=sStorage;
    ResetFixture();sSaveBlock1=saved;sStorage=savedStorage;
    LoadPartyRow(CTR_COOP_STORAGE_GUEST_ROW,gPlayerParty);LiveCheckParty(expected,"serialized SAV restart");
    PrepareHostSave(PEER_ID);
    sCoop.guest=true;sCoop.saveReady=true;
    memset(gPlayerParty,0,sizeof(gPlayerParty));ApplyHostSave();LiveCheckParty(expected,"host snapshot/rejoin");

    ordinary=expected[0];ordinary.hp=120;ordinary.status=0;ordinary.mail=0xff;before=ordinary;
    sStorage.boxes[0][0]=expected[0].box;
    CtrCoop_RestoreBoxVitals(&sStorage.boxes[0][0],&ordinary);
    CHECK(!memcmp(&ordinary,&before,sizeof(ordinary)),"ordinary PC box conversion has unchanged native vitals");
    CtrCoop_RestoreBoxVitals(&expected[0].box,&ordinary);
    CHECK(!memcmp(&ordinary,&before,sizeof(ordinary)),"a copied box outside the reserved row cannot inherit guest vitals");
    unsigned start=CTR_COOP_STORAGE_GUEST_ROW*CTR_COOP_STORAGE_ROW_SIZE;
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][start].data[40]^=1;
    CtrCoop_RestoreBoxVitals(&sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][start],&ordinary);
    CHECK(!memcmp(&ordinary,&before,sizeof(ordinary)),"changed box payload invalidates stale HP/status metadata");
    StorePartyRow(CTR_COOP_STORAGE_GUEST_ROW,expected);
    ordinary.maxHP=20;
    CtrCoop_RestoreBoxVitals(&sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][start],&ordinary);
    CHECK(ordinary.hp==20 && ordinary.status==0x10,"restored HP is clamped to newly calculated maximum");

    memset(&expected[0],0,sizeof(expected[0]));StorePartyRow(CTR_COOP_STORAGE_GUEST_ROW,expected);
    LoadPartyRow(CTR_COOP_STORAGE_GUEST_ROW,gPlayerParty);
    CHECK(gPlayerPartyCount==1 && gPlayerParty[0].box.species==211 && gPlayerParty[0].hp==0,
          "sparse guest row compacts mons while binding fainted HP to the original source slot");
}

static void TestNormalBattleLiveParty(void)
{
    struct Pokemon expected[PARTY_SIZE];
    uint8_t begin[CTR_COOP_SAVE_BEGIN_SIZE],packet[CTR_COOP_SAVE_CHUNK_MAX];
    ReadyHost();LiveGuestParty(expected);memcpy(gPlayerParty,expected,sizeof(expected));gPlayerPartyCount=2;
    sCoop.guest=true;sCoop.authenticated=sCoop.saveReady=sCoop.initialTeamCommitted=true;
    sCoop.battleActive=true;gMain.inBattle=true;gBattleTypeFlags=BATTLE_TYPE_TRAINER;
    SyncParty();
    CHECK(sCoop.partySending && LastPacket(CTR_COOP_PARTY_BEGIN_PACKET),"ordinary active combat sends a live party snapshot");
    CHECK(!memcmp(sPartySend,expected,sizeof(expected)),"live battle snapshot carries HP/status and PP without conversion");
    CHECK(SaveExtension()->guestVitals[0].hp==37 && SaveExtension()->guestVitals[1].hp==0,
          "live local battle also updates the durable guest-row vitals");

    ReadyHost();
    CHECK(CtrCoopProtocol_EncodePartyBegin(sizeof(expected),CtrCoopProtocol_Checksum((uint8_t *)expected,sizeof(expected)),
          begin,sizeof(begin)),"live party begin encodes");
    CtrCoop_HandlePacket(PEER_ID,begin,sizeof(begin));
    for(uint32_t offset=0;offset<sizeof(expected);)
    {
        uint16_t bytes=sizeof(expected)-offset;if(bytes>CTR_COOP_SAVE_CHUNK_DATA_MAX)bytes=CTR_COOP_SAVE_CHUNK_DATA_MAX;
        CHECK(CtrCoopProtocol_EncodePartyChunk(offset,(uint8_t *)expected+offset,bytes,packet,
              CTR_COOP_SAVE_CHUNK_HEADER+bytes),"live party chunk encodes");
        CtrCoop_HandlePacket(PEER_ID,packet,CTR_COOP_SAVE_CHUNK_HEADER+bytes);offset+=bytes;
    }
    CHECK(LastPacket(CTR_COOP_PARTY_RESULT_PACKET)!=NULL,"host accepts the complete live party");
    LoadPartyRow(CTR_COOP_STORAGE_GUEST_ROW,gPlayerParty);LiveCheckParty(expected,"received live battle snapshot");

    const uint32_t temporary[]={BATTLE_TYPE_FRONTIER,BATTLE_TYPE_INGAME_PARTNER,BATTLE_TYPE_SECRET_BASE,
        BATTLE_TYPE_EREADER_TRAINER,BATTLE_TYPE_WALLY_TUTORIAL,BATTLE_TYPE_RECORDED,BATTLE_TYPE_LINK};
    for(unsigned i=0;i<sizeof(temporary)/sizeof(*temporary);i++)
    {
        ReadyHost();LiveGuestParty(expected);StorePartyRow(CTR_COOP_STORAGE_GUEST_ROW,expected);
        sCoop.guest=true;sCoop.authenticated=sCoop.saveReady=sCoop.initialTeamCommitted=true;
        sCoop.battleActive=true;gMain.inBattle=true;gBattleTypeFlags=temporary[i];
        SetMon(&gPlayerParty[0],999);ClearPackets();SyncParty();
        CHECK(!LastPacket(CTR_COOP_PARTY_BEGIN_PACKET),"temporary battle mode %u cannot broadcast its working party",i);
        CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][CTR_COOP_STORAGE_ROW_SIZE].species==210,
              "temporary battle mode %u cannot overwrite the canonical row",i);
    }
}

static void TestRetainedDisconnectResultAndReset(void)
{
    uint8_t payload[BATTLE_HEADER_SIZE+sizeof(gPlayerParty)+BATTLE_STORAGE_RECORD_SIZE+BATTLE_ITEM_RECORD_SIZE];
    struct Pokemon guest[PARTY_SIZE];
    ReadyHost();gMain.callback2=CB2_Overworld;SetBattleMoney(1000);CHECK(CtrCoop_BattleStart(),"host local battle starts");
    uint32_t size=BuildRewardPayload(payload,9);LiveGuestParty(guest);
    memcpy(payload+BATTLE_HEADER_SIZE,guest,sizeof(guest));ReceiveBattlePayload(payload,size);
    CHECK(sCoop.battleReceiveState==BATTLE_SYNC_READY && BattleMoney()==1000,"host battle defers complete guest rewards");
    ClearRemote();CHECK(sCoop.battleReceiveState==BATTLE_SYNC_READY,"disconnect retains the complete pending result");
    TickDisconnectSave();CHECK(!sAutoSaves && CtrCoop_DisconnectSavePending(),"host autosave waits for local battle and deferred result");
    CtrCoop_BattleEnd();CtrCoop_BattleTick();
    CHECK(sCoop.battleReceiveState==BATTLE_SYNC_IDLE && BattleMoney()==1050,"retained result applies after the local battle");
    CHECK(sStorage.boxes[0][0].species==91 && SaveExtension()->guestVitals[1].hp==0,
          "disconnect preserves both incoming capture and fainted guest party metadata");
    TickDisconnectSave();CHECK(sAutoSaves==1 && !CtrCoop_DisconnectSavePending(),"settled disconnected world is autosaved once");

    ReadyHost();CHECK(CtrCoop_BattleStart(),"reset probe host battle starts");
    ReceiveBattlePayload(payload,size);ClearRemote();
    CHECK(sCoop.battleReceiveState==BATTLE_SYNC_READY && CtrCoop_DisconnectSavePending(),"reset probe has both deferred operations");
    CtrCoop_SoftReset();
    CHECK(sCoop.battleReceiveState==BATTLE_SYNC_IDLE && !CtrCoop_DisconnectSavePending() && !sDisconnectSaveRetry,
          "explicit save/title restart discards previous-world result and autosave intent");
    SetBattleMoney(777);gMain.callback2=CB2_Overworld;CtrCoop_BattleTick();TickDisconnectSave();
    CHECK(BattleMoney()==777 && !sAutoSaves,"new save context receives neither old rewards nor an automatic old-world save");
}

static void SetLiveBlock(unsigned block,bool enabled)
{
    switch(block)
    {
    case 0:sCoop.battleActive=enabled;break;
    case 1:gMain.inBattle=enabled;break;
    case 2:gMain.callback2=enabled?CB2_InitBattle:CB2_Overworld;break;
    case 3:sCoop.pcOpen=enabled;break;
    case 4:sCoop.pcPending=enabled;break;
    case 5:sCoop.pcOwner=enabled?PC_OWNER_HOST:PC_OWNER_NONE;break;
    case 6:sCoop.syncState=enabled?PC_SYNC_RECEIVING:PC_SYNC_IDLE;break;
    case 7:sCoop.eventStarting=enabled;break;
    case 8:sCoop.eventState=enabled?EVENT_HELD:EVENT_IDLE;break;
    case 9:sControlsLocked=enabled;break;
    case 10:sScriptEnabled=enabled;break;
    case 11:gSaveBlock1Ptr=enabled?NULL:&sSaveBlock1;break;
    case 12:sCoop.battleReceiveState=enabled?BATTLE_SYNC_READY:BATTLE_SYNC_IDLE;break;
    }
}
static void TestHostSafeSaveWaitAndRetry(void)
{
    for(unsigned block=0;block<13;block++)
    {
        ReadyHost();gMain.callback2=CB2_Overworld;ClearRemote();SetLiveBlock(block,true);
        TickDisconnectSave();CHECK(!sAutoSaves && CtrCoop_DisconnectSavePending(),"unsafe host save condition %u waits",block);
        SetLiveBlock(block,false);TickDisconnectSave();
        CHECK(sAutoSaves==1 && !CtrCoop_DisconnectSavePending(),"host saves after unsafe condition %u clears",block);
    }
    ReadyHost();gMain.callback2=CB2_Overworld;ClearRemote();sAutoSaveSucceeds=false;
    TickDisconnectSave();CHECK(sAutoSaves==1 && CtrCoop_DisconnectSavePending(),"failed save retains its pending intent");
    for(unsigned i=0;i<120;i++)TickDisconnectSave();
    CHECK(sAutoSaves==1 && !sDisconnectSaveRetry,"failure retry is bounded to the configured delay");
    sAutoSaveSucceeds=true;TickDisconnectSave();CHECK(sAutoSaves==2 && !CtrCoop_DisconnectSavePending(),"save retries and clears intent only on success");
    ReadyHost();sCoop.guest=true;EXPECT_GUEST_MENU(ClearRemote());TickDisconnectSave();CHECK(!sAutoSaves && !CtrCoop_DisconnectSavePending(),"guest departure discards its stack without writing the host file locally");
}

static void WatchFixture(bool npc)
{
    ReadyHost();gMain.callback2=CB2_Overworld;sWatchReady=true;
    sCoop.remoteValid=true;sCoop.remote=(CtrCoopSnapshot){.mapGroup=0,.mapNum=9,.mapLayoutId=10,.x=27,.y=23};
    if(npc)
    {
        gObjectEvents[1].active=true;gObjectEvents[1].localId=6;
        gObjectEvents[1].mapGroup=0;gObjectEvents[1].mapNum=9;
        gObjectEvents[1].currentCoords.x=25;gObjectEvents[1].currentCoords.y=23;
        sCoop.peerEventCount=1;sCoop.peerEventKeys[0]=(struct EncounterKey){0,9,6,123};
    }
}
static void TestBothWatchTargetsAndOwnLocks(void)
{
    for(unsigned npc=0;npc<2;npc++)
    {
        WatchFixture(npc);CHECK(CtrCoop_TryWatchAt(npc?25:27,23),"A can watch through %s",npc?"leased battle NPC":"remote player avatar");
        CHECK(sWatching && sWatchRequests==1 && !sControlsLocked && !sScriptEnabled
           && gMain.callback2==CB2_Overworld && !gMain.inBattle,"watch request changes no engine callback or own lock");
        for(unsigned block=0;block<12;block++)
        {
            WatchFixture(npc);SetLiveBlock(block,true);
            CHECK(!CtrCoop_TryWatchAt(npc?25:27,23) && !sWatchRequests,
                  "own busy condition %u blocks target %u",block,npc);
            SetLiveBlock(block,false);
        }
        WatchFixture(npc);CHECK(!CtrCoop_TryWatchAt(npc?25:27,22),"a different tile does not start watching");
    }
    WatchFixture(true);sCoop.peerEventKeys[0].mapNum=10;
    CHECK(!CtrCoop_TryWatchAt(25,23),"same local NPC ID on another map does not match the battle lease");
    WatchFixture(true);gObjectEvents[1].active=false;CHECK(!CtrCoop_TryWatchAt(25,23),"inactive battle NPC cannot be used as a target");
    WatchFixture(false);sCoop.remote.mapNum=10;CHECK(!CtrCoop_TryWatchAt(27,23),"other-map remote position cannot be targeted");
    WatchFixture(false);sWatchReady=false;CHECK(!CtrCoop_TryWatchAt(27,23),"peer eligibility is required before A interaction");
}
int main(void)
{
    TestLiveVitalsPersistence();TestNormalBattleLiveParty();TestRetainedDisconnectResultAndReset();
    TestHostSafeSaveWaitAndRetry();TestBothWatchTargetsAndOwnLocks();
    if(sFailures)return 1;
    puts("independent co-op live HP/disconnect/autosave/watch integration regressions passed");
    return 0;
}
