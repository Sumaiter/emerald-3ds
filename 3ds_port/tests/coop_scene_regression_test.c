/* Reuse the host engine/transport fixture, with a separate entry point. */
#define main CoopSessionOriginalMain
#include "coop_session_test.c"
#undef main

static void SceneGuest(void)
{
    ReadyHost();
    sCoop.guest = true;
    sCoop.authenticated = sCoop.saveReady = true;
    gMain.callback2 = CB2_Overworld;
    sSaveBlock1.location.mapGroup = 24;
    sSaveBlock1.location.mapNum = 4;
    for (uint8_t i = 1; i <= 3; ++i)
    {
        gObjectEvents[i].active = true;
        gObjectEvents[i].localId = i + 4; /* Briney, Grunt, Peeko */
        gObjectEvents[i].mapGroup = 24;
        gObjectEvents[i].mapNum = 4;
        gObjectEvents[i].graphicsId = 1;
        gObjectEvents[i].facingDirection = 1;
        gObjectEvents[i].currentCoords.x = 20;
        gObjectEvents[i].currentCoords.y = 11;
    }
}

static void SceneSnapshot(uint8_t localId)
{
    CtrCoopNpcSnapshot snapshot = {.localId = localId, .mapGroup = 24, .mapNum = 4,
        .x = 21, .y = 12, .facing = 4, .graphicsId = 1};
    uint8_t packet[CTR_COOP_NPC_PACKET_SIZE];
    CHECK(CtrCoopProtocol_EncodeNpc(&snapshot, packet, sizeof(packet)), "NPC fixture encodes");
    CtrCoop_HandlePacket(PEER_ID, packet, sizeof(packet));
}

static void SceneResult(bool accepted)
{
    struct SentPacket result = *LastPacket(CTR_COOP_EVENT_REQUEST_PACKET);
    result.data[0] = CTR_COOP_EVENT_RESULT_PACKET;
    result.data[21] = accepted;
    CtrCoop_HandlePacket(PEER_ID, result.data, result.len);
}

static void SceneAcquire(void)
{
    CtrCoop_ScriptObject(24, 4, 6);
    CtrCoop_ScriptBegin((void *)0x1000);
    CHECK(CtrCoop_ScriptPoll() == 0, "guest waits for the root NPC");
    SceneResult(true);
    CHECK(CtrCoop_ScriptPoll() == 1, "guest runs only after grant");
}

static void SceneCheckPose(uint8_t index, bool protected)
{
    CHECK(gObjectEvents[index].currentCoords.x == (protected ? 20 : 21)
       && gObjectEvents[index].currentCoords.y == (protected ? 11 : 12)
       && gObjectEvents[index].facingDirection == (protected ? 1 : 4),
       "NPC %u pose is %s", index + 4, protected ? "protected" : "synchronized");
}

static void TestSceneActors(void)
{
    SceneGuest();
    SceneSnapshot(7);
    SceneCheckPose(3, false); /* Ordinary NPC sync must still work. */
    SceneGuest();
    SceneAcquire();
    CtrCoop_ScriptActor(24, 4, 5);
    CtrCoop_ScriptActor(24, 4, 7);
    CHECK(sCoop.eventCount == 1 && sCoop.eventKeys[0].localId == 6,
          "dynamic scene actors do not mutate the wire lease/release identity");
    gObjectEvents[3].heldMovementActive = true;
    SceneSnapshot(7);
    SceneCheckPose(3, true);
    CHECK(gObjectEvents[3].heldMovementActive, "Peeko's movement survives a host snapshot");
    gObjectEvents[3].heldMovementActive = false;
    SceneSnapshot(7);
    SceneCheckPose(3, true); /* Dialogue between two movements is protected too. */
    SceneSnapshot(5);
    SceneCheckPose(1, true);
    CHECK(OwnsEncounterNpc(24, 4, 7), "removed actors remain protected against respawn");
    CHECK(!OwnsEncounterNpc(24, 5, 7), "same local ID on another map remains independent");

    CtrCoop_ScriptEnd();
    sCoop.battleSendState = BATTLE_SYNC_SENDING;
    TickEncounter();
    SceneSnapshot(7);
    SceneCheckPose(3, true); /* Retain ownership through result/story drain. */
    sCoop.battleSendState = BATTLE_SYNC_IDLE;
    TickEncounter();
    CHECK(sCoop.eventState == EVENT_RELEASING, "finished guest scene begins release");
    SceneSnapshot(7);
    SceneCheckPose(3, false);

    struct SentPacket ack = *LastPacket(CTR_COOP_EVENT_RELEASE_PACKET);
    ack.data[0] = CTR_COOP_EVENT_ACK_PACKET;
    ack.data[21] = 1;
    CtrCoop_HandlePacket(PEER_ID, ack.data, ack.len);
    SceneAcquire();
    CHECK(!OwnsEncounterNpc(24, 4, 5) && !OwnsEncounterNpc(24, 4, 7),
          "next NPC event does not inherit previous scene actors");
    SceneSnapshot(5);
    SceneCheckPose(1, false);
}

static void TestPendingApproachPose(void)
{
    SceneGuest();
    CtrCoop_ScriptObject(24, 4, 6);
    CtrCoop_ScriptObject(24, 4, 7);
    CtrCoop_ScriptBegin((void *)0x2000);
    SceneSnapshot(6);
    SceneCheckPose(2, true); /* Before even the first poll. */
    CHECK(CtrCoop_ScriptPoll() == 0, "approach is pending");
    CtrCoop_ScriptActor(24, 4, 5); /* No script bytecode has been granted yet. */
    SceneSnapshot(5);
    SceneCheckPose(1, false);
    SceneSnapshot(6);
    SceneSnapshot(7);
    SceneCheckPose(2, true);
    SceneCheckPose(3, true); /* Both approaching trainers retain their pose. */
    SceneResult(true);
    CHECK(CtrCoop_ScriptPoll() == 1, "approach grant resumes the original path");
    SceneCheckPose(2, true);
    SceneCheckPose(3, true);
    CHECK(!OwnsEncounterNpc(24, 4, 5), "unrelated pending actor was never captured");
}

static void TestSceneDenialAndCancel(void)
{
    SceneGuest();
    CtrCoop_ScriptObject(24, 4, 6);
    CtrCoop_ScriptBegin((void *)0x3000);
    CHECK(CtrCoop_ScriptPoll() == 0, "denial fixture waits");
    SceneResult(false);
    SceneSnapshot(6);
    SceneCheckPose(2, false);
    CHECK(CtrCoop_ScriptPoll() < 0, "denied approach cannot start");
    CtrCoop_ScriptEnd();
    CHECK(!OwnsEncounterNpc(24, 4, 6), "denied NPC has no residual protection");

    SceneGuest();
    CtrCoop_ScriptObject(24, 4, 6);
    CtrCoop_ScriptBegin((void *)0x3000);
    CtrCoop_ScriptPoll();
    CtrCoop_ScriptEnd();
    CHECK(sCoop.eventState == EVENT_PENDING && sCoop.eventEnding,
          "cancel may wait for its remote release acknowledgement");
    SceneSnapshot(6);
    SceneCheckPose(2, false);

    SceneGuest();
    CtrCoop_ScriptObject(24, 4, 6);
    CtrCoop_ScriptBegin((void *)0x3000);
    CtrCoop_ScriptPoll();
    sCoop.eventWait = 599;
    CHECK(CtrCoop_ScriptPoll() < 0, "unanswered approach times out");
    CtrCoop_ScriptEnd();
    SceneSnapshot(6);
    SceneCheckPose(2, false);
}

static void TestSceneDisconnectAndReset(void)
{
    SceneGuest();
    CtrCoop_ScriptObject(24, 4, 6);
    CtrCoop_ScriptBegin((void *)0x4000);
    CtrCoop_ScriptPoll();
    EXPECT_GUEST_MENU(CtrCoop_PeerLeft(PEER_ID));
    CHECK(!OwnsEncounterNpc(24, 4, 6) && !sCoop.eventStarting,
          "disconnect discards the guest's ungranted scene stack");

    SceneGuest();
    SceneAcquire();
    CtrCoop_ScriptActor(24, 4, 7);
    EXPECT_GUEST_MENU(CtrCoop_PeerLeft(PEER_ID));
    CHECK(!OwnsEncounterNpc(24, 4, 7), "guest disconnect abandons even its granted scene and actor protection");
    SceneGuest();
    SceneAcquire();
    CHECK(!OwnsEncounterNpc(24, 4, 7), "new session does not inherit disconnected actors");
}

static void TestSceneOverflowAndNewPreparedEvent(void)
{
    SceneGuest();
    SceneAcquire();
    for (uint8_t id = 1; id <= OBJECT_EVENTS_COUNT + 1; ++id)
        CtrCoop_ScriptActor(24, 4, id);
    CHECK(OwnsEncounterNpc(24, 4, 99), "actor overflow retains conservative scene protection");
    CtrCoop_ScriptEnd();
    TickEncounter();
    CHECK(!OwnsEncounterNpc(24, 4, 99), "overflow protection ends with the lease");
    CtrCoop_ScriptObject(24, 4, 5);
    CtrCoop_ScriptBegin((void *)0x5000);
    SceneSnapshot(5);
    SceneCheckPose(1, true); /* New approach waits for old release ACK. */
    struct SentPacket ack = *LastPacket(CTR_COOP_EVENT_RELEASE_PACKET);
    ack.data[0] = CTR_COOP_EVENT_ACK_PACKET;
    ack.data[21] = 1;
    CtrCoop_HandlePacket(PEER_ID, ack.data, ack.len);
    CHECK(CtrCoop_ScriptPoll() == 0, "next approach issues a fresh request");
    SceneResult(true);
    CHECK(CtrCoop_ScriptPoll() == 1, "next approach acquires normally");
    CHECK(!OwnsEncounterNpc(24, 4, 99), "fresh event resets actor overflow");
    SceneSnapshot(7);
    SceneCheckPose(3, false);
}

int main(void)
{
    TestSceneActors();
    TestPendingApproachPose();
    TestSceneDenialAndCancel();
    TestSceneDisconnectAndReset();
    TestSceneOverflowAndNewPreparedEvent();
    printf("scene/approach regression tests: %d failures\n", sFailures);
    return sFailures ? 1 : 0;
}
