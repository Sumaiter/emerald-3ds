#define main CoopSessionOriginalMain
#include "coop_session_test.c"
#undef main

struct StoryEndpoint
{
    __typeof__(sCoop) coop;
    struct SaveBlock1 save;
    struct StoryObject objects[STORY_OBJECT_LIMIT];
    unsigned count;
    bool overflow;
    uint8_t flags[NUM_FLAG_BYTES], vars[sizeof(sStoryVarsTouched)];
    uint32_t epoch, prepared;
    struct EncounterKey actors[OBJECT_EVENTS_COUNT];
    uint8_t actorCount;
    bool actorOverflow;
};

static void StoryStore(struct StoryEndpoint *end)
{
    end->coop = sCoop; end->save = sSaveBlock1;
    memcpy(end->objects, sStoryObjects, sizeof(sStoryObjects)); end->count = sStoryObjectCount;
    end->overflow = sStoryObjectOverflow;
    memcpy(end->flags, sStoryFlagsTouched, sizeof(end->flags));
    memcpy(end->vars, sStoryVarsTouched, sizeof(end->vars));
    end->epoch = sStoryEpoch; end->prepared = sStoryPreparedEpoch;
    memcpy(end->actors, sEncounterActors, sizeof(end->actors));
    end->actorCount = sEncounterActorCount; end->actorOverflow = sEncounterActorsOverflow;
}

static void StoryLoad(const struct StoryEndpoint *end)
{
    sCoop = end->coop; sSaveBlock1 = end->save;
    memcpy(sStoryObjects, end->objects, sizeof(sStoryObjects)); sStoryObjectCount = end->count;
    sStoryObjectOverflow = end->overflow;
    memcpy(sStoryFlagsTouched, end->flags, sizeof(end->flags));
    memcpy(sStoryVarsTouched, end->vars, sizeof(end->vars));
    sStoryEpoch = end->epoch; sStoryPreparedEpoch = end->prepared;
    memcpy(sEncounterActors, end->actors, sizeof(end->actors));
    sEncounterActorCount = end->actorCount; sEncounterActorsOverflow = end->actorOverflow;
    gMain.callback2 = CB2_Overworld;
}

static void StoryReady(void)
{
    ReadyHost();
    CtrCoop_ClearStoryWorld();
    sSaveBlock1.location.mapGroup = 0; sSaveBlock1.location.mapNum = 18;
    /* Initial hide flags cleared by the actual Route103 RivalEnd. */
    sSaveBlock1.flags[0x379 / 8] |= 1 << (0x379 & 7);
    sSaveBlock1.flags[0x3D3 / 8] |= 1 << (0x3D3 & 7);
    ResetStoryBaseline();
    gMain.callback2 = CB2_Overworld;
    ClearPackets();
}

static void StoryBegin(bool guest)
{
    sCoop.guest = guest;
    sCoop.authenticated = sCoop.saveReady = true;
    CtrCoop_ScriptObject(0, 18, 2);
    CtrCoop_ScriptBegin((const void *)0x1000);
}

static void StoryGrant(void)
{
    struct SentPacket grant = *LastPacket(CTR_COOP_EVENT_REQUEST_PACKET);
    grant.data[0] = CTR_COOP_EVENT_RESULT_PACKET; grant.data[21] = 1;
    CtrCoop_HandlePacket(PEER_ID, grant.data, grant.len);
    CHECK(CtrCoop_ScriptPoll() == 1, "story waits for a matching host grant");
}

static void StoryFinishRival(uint16_t trainer)
{
    /* Additional deltas put the completion behind more than 128 packets. */
    for (uint16_t id = 64; id < 244; ++id)
    {
        sSaveBlock1.flags[id / 8] |= 1 << (id & 7);
        CtrCoop_StoryFlagChanged(id);
    }
    for (unsigned i = 0; i < 32; ++i)
    {
        sSaveBlock1.vars[i + 32] = i + 10;
        CtrCoop_StoryVarChanged(VARS_START + i + 32);
    }
    sSaveBlock1.flags[0x2D3 / 8] |= 1 << (0x2D3 & 7); /* removeobject's hide flag */
    sSaveBlock1.flags[0x379 / 8] &= ~(1 << (0x379 & 7));
    sSaveBlock1.flags[0x3D3 / 8] &= ~(1 << (0x3D3 & 7));
    sSaveBlock1.flags[(TRAINER_FLAGS_START + trainer) / 8] |= 1 << ((TRAINER_FLAGS_START + trainer) & 7);
    sSaveBlock1.vars[0x84] = 4; sSaveBlock1.vars[0xC7] = 1;
    CtrCoop_StoryObject(0, 18, 2, 8, 0, 0, 0);
    CtrCoop_StoryObject(0, 18, 3, 3, 44, 25, 1);
    CtrCoop_ScriptEnd();
}

static bool StoryCompleted(uint16_t trainer)
{
    for (uint16_t id = 64; id < 244; ++id) if (!FlagGet(id)) return false;
    for (unsigned i = 0; i < 32; ++i) if (sSaveBlock1.vars[i + 32] != i + 10) return false;
    return FlagGet(0x2D3) && !FlagGet(0x379) && !FlagGet(0x3D3)
        && FlagGet(TRAINER_FLAGS_START + trainer) && sSaveBlock1.vars[0x84] == 4
        && sSaveBlock1.vars[0xC7] == 1 && !StoryNpcAvailable(0, 18, 2);
}

static void TestRivalBothRolesAndVariants(void)
{
    static const uint16_t variants[] = {520, 523, 526, 529, 532, 535};
    static struct StoryEndpoint host, guest;
    static struct SentPacket outgoing[MAX_SENT_PACKETS], replies[MAX_SENT_PACKETS];
    for (unsigned role = 0; role < 2; ++role) for (unsigned variant = 0; variant < 6; ++variant)
    {
        unsigned sent = 0;
        StoryReady(); StoryStore(&host);
        sCoop.guest = true; sCoop.authenticated = sCoop.saveReady = true; StoryStore(&guest);
        struct StoryEndpoint *owner = role ? &guest : &host, *other = role ? &host : &guest;
        StoryLoad(owner); StoryBegin(role != 0);
        if (role)
        {
            CHECK(CtrCoop_ScriptPoll() == 0, "guest rival prelude waits");
            struct SentPacket request = *LastPacket(CTR_COOP_EVENT_REQUEST_PACKET);
            StoryStore(owner); StoryLoad(other); ClearPackets();
            CtrCoop_HandlePacket(PEER_ID, request.data, request.len);
            struct SentPacket grant = *LastPacket(CTR_COOP_EVENT_RESULT_PACKET);
            StoryStore(other); StoryLoad(owner);
            CtrCoop_HandlePacket(PEER_ID, grant.data, grant.len);
        }
        CHECK(CtrCoop_ScriptPoll() == 1, "owner receives its NPC lease");
        StoryFinishRival(variants[variant]); StoryStore(owner);
        for (unsigned tick = 0; tick < 500 && owner->coop.eventState != EVENT_IDLE; ++tick)
        {
            StoryLoad(owner); ClearPackets(); CtrCoop_StoryTick(); StoryStore(owner);
            int count = sSentCount;
            memcpy(outgoing, sSent, count * sizeof(outgoing[0])); sent += count;
            StoryLoad(other); ClearPackets();
            for (int i = 0; i < count; ++i)
            {
                if (outgoing[i].data[0] == CTR_COOP_EVENT_RELEASE_PACKET)
                    CHECK(StoryCompleted(variants[variant]), "all story/object deltas arrive before lease release");
                CHECK(CtrCoop_HandlePacket(PEER_ID, outgoing[i].data, outgoing[i].len), "central dispatcher consumes story wire packets");
            }
            StoryStore(other);
            int replyCount = sSentCount;
            memcpy(replies, sSent, replyCount * sizeof(replies[0]));
            StoryLoad(owner);
            for (int i = 0; i < replyCount; ++i)
                CtrCoop_HandlePacket(PEER_ID, replies[i].data, replies[i].len);
            StoryStore(owner);
        }
        CHECK(owner->coop.eventState == EVENT_IDLE && sent > 128, "large completion drains and releases with backpressure-aware progress");
        StoryLoad(other);
        CHECK(StoryCompleted(variants[variant]), "shared completion covers both genders/all starter variants and either owner");
        ClearPackets();
        sCoop.storyFrame = 14; SyncStoryState();
        CHECK(!CountPacket(CTR_COOP_FLAG_PACKET) && !CountPacket(CTR_COOP_VAR_PACKET), "receiver cannot echo stale host story over the finished guest state");
        CtrCoop_ScriptObject(0, 18, 2); CtrCoop_ScriptBegin((void *)0x9000);
        CHECK(CtrCoop_ScriptPoll() < 0, "different trainer/script variant cannot restart the completed NPC");
        CtrCoop_ScriptEnd();
        CtrCoop_ResetStory();
        CHECK(!StoryNpcAvailable(0, 18, 2) && FlagGet(0x2D3), "disconnect retains completed removal and persistent hide flag");
    }
}

static void TestStoryBackpressureAndEchoes(void)
{
    StoryReady(); StoryBegin(true); CHECK(CtrCoop_ScriptPoll() == 0, "guest waits"); StoryGrant();
    sSaveBlock1.flags[10] |= 1; CtrCoop_StoryFlagChanged(80);
    sSaveBlock1.vars[25] = 77; CtrCoop_StoryVarChanged(VARS_START + 25);
    CtrCoop_StoryObject(0, 18, 2, 8, 0, 0, 0);
    CtrCoop_ScriptEnd(); ClearPackets(); sStoryTrySendBlocked = true;
    for (unsigned i = 0; i < 40; ++i) CtrCoop_StoryTick();
    CHECK(sCoop.eventState == EVENT_HELD && StoryChangesPending() && StoryObjectsPending(), "failed enqueue retains every delta and the lease");
    CHECK(!sSentCount && !(sCoop.storyFlagsBaseline[10] & 1) && !sCoop.storyVarsBaseline[25], "backpressure never advances unsent baselines");
    sStoryTrySendBlocked = false;
    for (unsigned i = 0; i < 4; ++i) CtrCoop_StoryTick();
    CHECK(sCoop.eventState == EVENT_RELEASING && !StoryChangesPending() && !StoryObjectsPending(), "resuming transport drains all records before release");
    ApplyRemoteFlag(80, false); ApplyRemoteVar(VARS_START + 25, 0);
    CHECK(FlagGet(80) && sSaveBlock1.vars[25] == 77, "old host echoes cannot roll back sent event intents awaiting release ACK");
    uint8_t vars[sizeof(sStoryVarsTouched)]; memcpy(vars, sStoryVarsTouched, sizeof(vars));
    CtrCoop_StoryVarChanged(0x800D); ApplyRemoteVar(0x800D, 0x9999);
    CHECK(!memcmp(vars, sStoryVarsTouched, sizeof(vars)), "VAR_RESULT and all special vars stay local");
}

static void TestStaleContextsAndFailure(void)
{
    StoryReady(); StoryBegin(true); CHECK(CtrCoop_ScriptPoll() == 0, "stale context requests ownership");
    ApplyRemoteVar(VARS_START + 20, 4);
    struct SentPacket grant = *LastPacket(CTR_COOP_EVENT_REQUEST_PACKET);
    grant.data[0] = CTR_COOP_EVENT_RESULT_PACKET; grant.data[21] = 1;
    CtrCoop_HandlePacket(PEER_ID, grant.data, grant.len);
    CHECK(CtrCoop_ScriptPoll() < 0, "story revision invalidates a preselected map/NPC context before bytecode");
    CtrCoop_ScriptEnd(); TickEncounter();
    CHECK(sCoop.eventState == EVENT_RELEASING, "stale granted context still releases its real remote lease");

    StoryReady(); sStoryNpcAvailable = false; StoryBegin(false);
    CHECK(CtrCoop_ScriptPoll() < 0 && sCoop.eventState == EVENT_IDLE, "native current hide flag blocks a stale active NPC");
    for (unsigned outcome = 0; outcome < 2; ++outcome)
    {
        StoryReady(); StoryBegin(false); CHECK(CtrCoop_ScriptPoll() == 1, "failed attempt owns the root NPC");
        gBattleOutcome = outcome ? B_OUTCOME_RAN : B_OUTCOME_LOST;
        CtrCoop_ScriptEnd(); TickEncounter();
        CHECK(!FlagGet(0x2D3) && !FlagGet(0x82) && StoryNpcAvailable(0, 18, 2), "loss/flight alone cannot synthesize shared story completion");
    }
    StoryReady();
    for (uint16_t id = 64; id < 93; ++id) sSaveBlock1.flags[id / 8] |= 1 << (id & 7);
    sCoop.storyFrame = 14; ClearPackets(); SyncStoryState();
    CHECK(CountPacket(CTR_COOP_FLAG_PACKET) == 8 && StoryChangesPending(), "normal story sync remains bounded at eight deltas per fifteen frames");
    ClearPackets(); for (unsigned i = 0; i < 14; ++i) SyncStoryState();
    CHECK(!sSentCount, "ordinary story sync respects the fifteen-frame cadence");
}

static void TestLocalScratchAndInvalidObjects(void)
{
    StoryReady(); StoryBegin(false);
    CHECK(CtrCoop_ScriptPoll() == 1, "scratch test owns its local scene");
    for (uint16_t id = 0; id < STORY_FIRST_FLAG; ++id)
    {
        sSaveBlock1.flags[id / 8] |= 1 << (id & 7);
        CtrCoop_StoryFlagChanged(id);
        ApplyRemoteFlag(id, false);
        CHECK((sSaveBlock1.flags[id / 8] & (1 << (id & 7))) != 0,
            "remote map-local flags cannot reset a local scene");
    }
    for (unsigned i = 0; i < STORY_FIRST_VAR - VARS_START; ++i)
    {
        sSaveBlock1.vars[i] = i + 1;
        CtrCoop_StoryVarChanged(VARS_START + i);
        ApplyRemoteVar(VARS_START + i, 0);
        CHECK(sSaveBlock1.vars[i] == i + 1, "remote TEMP vars cannot reset local script scratch");
    }
    CtrCoop_ScriptEnd(); ClearPackets(); CtrCoop_StoryTick();
    CHECK(sCoop.eventState == EVENT_IDLE && !StoryChangesPending()
        && !CountPacket(CTR_COOP_FLAG_PACKET) && !CountPacket(CTR_COOP_VAR_PACKET),
        "temporary scratch neither transmits nor holds a finished event lease");

    StoryReady();
    sCoop.peerEventToken = 42; sCoop.peerEventCount = 1;
    uint8_t packet[STORY_OBJECT_PACKET_SIZE] = {STORY_OBJECT_PACKET, CTR_COOP_PROTOCOL_VERSION,
        42, 0, 0, 0, 0, 18, 2, 8, 0, 0, 0, 0, 0};
    CtrCoop_HandleStoryPacket(PEER_ID, packet, sizeof(packet) - 1);
    packet[2] = 41; CtrCoop_HandleStoryPacket(PEER_ID, packet, sizeof(packet));
    packet[2] = 42; packet[9] = 12; CtrCoop_HandleStoryPacket(PEER_ID, packet, sizeof(packet));
    packet[9] = 8; packet[1] ^= 1; CtrCoop_HandleStoryPacket(PEER_ID, packet, sizeof(packet));
    CHECK(!sStoryObjectCount, "truncated, stale-token, contradictory and wrong-version intents are discarded");
    packet[1] = CTR_COOP_PROTOCOL_VERSION;
    CtrCoop_HandleStoryPacket(PEER_ID, packet, sizeof(packet));
    CHECK(sStoryObjectCount == 1 && !StoryNpcAvailable(0, 18, 2), "matching atomic object intent is accepted");
}

int main(void)
{
    TestRivalBothRolesAndVariants();
    TestStoryBackpressureAndEchoes();
    TestStaleContextsAndFailure();
    TestLocalScratchAndInvalidObjects();
    printf("story completion regressions: %d failures\n", sFailures);
    return sFailures ? 1 : 0;
}
