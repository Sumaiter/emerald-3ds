#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/3ds_coop.c"

#ifndef CTR_COOP_TEST_MAX_SPRINT
#define CTR_COOP_TEST_MAX_SPRINT 8
#endif

#define MAX_SENT_PACKETS 2048
#define MAX_PACKET_SIZE 160
#define PEER_ID 2

struct SentPacket
{
    uint16_t to;
    uint16_t len;
    uint8_t data[MAX_PACKET_SIZE];
};

static struct SentPacket sSent[MAX_SENT_PACKETS];
static int sSentCount;
static int sFailures;
static int sSession = LOCAL_SESSION_IN;
static int sSessionKind = LOCAL_KIND_COOP;
static int sPeerCount = 1;
static int sPcOpenCount;
static int sWarpCount;
static int8_t sWarpX;
static int8_t sWarpY;
static MainCallback sMainCallback;
static struct CtrCoopProfile sProfile;
static struct SaveBlock2 sSaveBlock2;
static struct SaveBlock1 sSaveBlock1;
static struct PokemonStorage sStorage;

struct SaveBlock2 *gSaveBlock2Ptr = &sSaveBlock2;
struct SaveBlock1 *gSaveBlock1Ptr = &sSaveBlock1;
struct PokemonStorage *gPokemonStoragePtr = &sStorage;
struct Pokemon gPlayerParty[PARTY_SIZE];
struct Pokemon gEnemyParty[PARTY_SIZE];
uint8_t gPlayerPartyCount;
struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
struct PlayerAvatar gPlayerAvatar;
struct MapHeader gMapHeader;
uint32_t gBattleTypeFlags;
uint16_t gTrainerBattleOpponent_A;
uint16_t gTrainerBattleOpponent_B;
uint8_t gBattleEnvironment;
struct Main gMain;
uint32_t gRngValue;
uint8_t gWirelessCommType;
uint8_t gReceivedRemoteLinkPlayers;
struct LinkPlayer gLinkPlayers[2];
uint8_t gBattleOutcome;

#define CHECK(condition, ...) do { if (!(condition)) { sFailures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define XFAIL_CHECK(condition, ...) do { if (!(condition)) { sExpectedFailures++; printf("XFAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } else { sFailures++; printf("XPASS %s:%d: remove expected-failure marker: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static int sExpectedFailures;

static void SetMon(struct Pokemon *mon, uint16_t species)
{
    memset(mon, 0, sizeof(*mon));
    mon->box.species = species;
}

static void SetBoxMon(struct BoxPokemon *mon, uint16_t species)
{
    memset(mon, 0, sizeof(*mon));
    mon->species = species;
}

static void ClearPackets(void)
{
    sSentCount = 0;
}

static int CountPacket(uint8_t type)
{
    int count = 0;

    for (int i = 0; i < sSentCount; i++)
        if (sSent[i].data[0] == type)
            count++;
    return count;
}

static const struct SentPacket *LastPacket(uint8_t type)
{
    for (int i = sSentCount - 1; i >= 0; i--)
        if (sSent[i].data[0] == type)
            return &sSent[i];
    return NULL;
}

static void ResetFixture(void)
{
    static const uint8_t password[] = {1, 2, 3, 4};
    static const uint8_t name[] = {'G', 'A', 'S', 'T', 0xFF, 0xFF, 0xFF, 0xFF};

    memset(&sCoop, 0, sizeof(sCoop));
    sCoop.objectEventId = OBJECT_EVENTS_COUNT;
    memset(&sSaveBlock2, 0, sizeof(sSaveBlock2));
    memset(&sSaveBlock1, 0, sizeof(sSaveBlock1));
    memset(&sStorage, 0, sizeof(sStorage));
    memset(gPlayerParty, 0, sizeof(gPlayerParty));
    memset(gEnemyParty, 0, sizeof(gEnemyParty));
    memset(gObjectEvents, 0, sizeof(gObjectEvents));
    memset(&sProfile, 0xFF, sizeof(sProfile));
    memcpy(sProfile.hostPassword, password, sizeof(sProfile.hostPassword));
    memcpy(sProfile.guestPassword, password, sizeof(sProfile.guestPassword));
    memcpy(sProfile.guestName, name, sizeof(sProfile.guestName));
    sProfile.guestNameLength = 4;
    sProfile.hostGraphicsId = 100;
    sProfile.guestGraphicsId = 105;
    sProfile.guestName[7] = 0xFF;
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        SetMon(&gPlayerParty[i], i + 1);
    gPlayerPartyCount = PARTY_SIZE;
    gPlayerAvatar.objectEventId = 0;
    gObjectEvents[0].active = TRUE;
    gObjectEvents[0].currentCoords.x = 26;
    gObjectEvents[0].currentCoords.y = 23;
    sSaveBlock1.location.mapGroup = 0;
    sSaveBlock1.location.mapNum = 9;
    sSaveBlock1.pos.x = 19;
    sSaveBlock1.pos.y = 16;
    gMapHeader.mapLayoutId = LAYOUT_LITTLEROOT_TOWN;
    gBattleTypeFlags = 0;
    gTrainerBattleOpponent_A = 0;
    gTrainerBattleOpponent_B = 0;
    gBattleOutcome = 0;
    gWirelessCommType = 0;
    gReceivedRemoteLinkPlayers = 0;
#if CTR_COOP_TEST_MAX_SPRINT >= 6
    sRemotePartyValid = false;
#endif
    sSession = LOCAL_SESSION_IN;
    sSessionKind = LOCAL_KIND_COOP;
    sPeerCount = 1;
    sPcOpenCount = 0;
    sWarpCount = 0;
    sWarpX = 0;
    sWarpY = 0;
    sMainCallback = NULL;
    gMain.savedCallback = NULL;
    ClearPackets();
}

static void AuthenticateHost(void)
{
    uint8_t auth[CTR_COOP_AUTH_PACKET_SIZE];
    uint8_t identity[CTR_COOP_IDENTITY_SIZE];
    uint8_t ready[] = {CTR_COOP_SAVE_READY_PACKET, CTR_COOP_PROTOCOL_VERSION};

    CtrCoopProtocol_EncodeAuth(sProfile.guestPassword, auth, sizeof(auth));
    CtrCoop_HandlePacket(PEER_ID, auth, sizeof(auth));
    CtrCoopProtocol_EncodeIdentity(sProfile.guestName, identity, sizeof(identity));
    CtrCoop_HandlePacket(PEER_ID, identity, sizeof(identity));
    CtrCoop_HandlePacket(PEER_ID, ready, sizeof(ready));
}

static void ReadyHost(void)
{
    ResetFixture();
    CtrCoop_Toggle();
    AuthenticateHost();
    sCoop.peerTeamReady = true;
    ClearPackets();
}

typedef bool (*BeginEncoder)(uint32_t, uint32_t, uint8_t *, uint16_t);
typedef bool (*ChunkEncoder)(uint32_t, const uint8_t *, uint16_t, uint8_t *, uint16_t);

static void SendTransfer(BeginEncoder encodeBegin, ChunkEncoder encodeChunk,
    const uint8_t *payload, uint32_t size, uint32_t checksum, bool sendBegin, uint32_t startOffset)
{
    uint8_t packet[CTR_COOP_SAVE_CHUNK_MAX];

    if (sendBegin)
    {
        CHECK(encodeBegin(size, checksum, packet, CTR_COOP_SAVE_BEGIN_SIZE), "test begin encodes");
        CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_SAVE_BEGIN_SIZE);
    }
    for (uint32_t offset = startOffset; offset < size;)
    {
        uint16_t amount = size - offset;

        if (amount > CTR_COOP_SAVE_CHUNK_DATA_MAX)
            amount = CTR_COOP_SAVE_CHUNK_DATA_MAX;
        CHECK(encodeChunk(offset, payload + offset, amount, packet, CTR_COOP_SAVE_CHUNK_HEADER + amount), "test chunk encodes");
        CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_SAVE_CHUNK_HEADER + amount);
        offset += amount;
    }
}

static void BuildPcPayload(uint8_t *payload, struct PokemonStorage *storage, const struct Pokemon *party)
{
    memcpy(payload, storage, sizeof(*storage));
    memcpy(payload + sizeof(*storage), party, sizeof(struct Pokemon) * PARTY_SIZE);
}

static void TestStorageMigration(void)
{
    struct PokemonStorage before;
    struct CoopSaveExtension *extension = (struct CoopSaveExtension *)sSaveBlock1.unused_3598;
    int copies = 0;

    ResetFixture();
    SetBoxMon(&sStorage.boxes[0][0], 99);
    SetBoxMon(&sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0], 10);
    SetBoxMon(&sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][7], 20);
    CtrCoop_Toggle();
    CHECK(CtrCoop_IsEnabled(), "co-op starts with enough ordinary box space");
    CHECK(sStorage.boxes[0][1].species == 10 && sStorage.boxes[0][2].species == 20, "box 14 occupants move to free ordinary slots");
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][7].species == 0, "migrated guest-row slots are empty before team selection");
    CHECK(extension->storageMagic == COOP_STORAGE_MAGIC && extension->storageVersion == COOP_STORAGE_VERSION, "storage extension is versioned");
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].species == gPlayerParty[0].box.species, "host party mirrors to host row");
    for (uint8_t box = 0; box < TOTAL_BOXES_COUNT; box++)
        for (uint8_t position = 0; position < IN_BOX_COUNT; position++)
            if (sStorage.boxes[box][position].species == 10)
                copies++;
    CHECK(copies == 1, "migration neither duplicates nor loses Pokémon");

    ResetFixture();
    for (uint8_t box = 0; box < CTR_COOP_STORAGE_BOX_ID; box++)
        for (uint8_t position = 0; position < IN_BOX_COUNT; position++)
            SetBoxMon(&sStorage.boxes[box][position], 100 + box);
    SetBoxMon(&sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0], 55);
    before = sStorage;
    CtrCoop_Toggle();
    CHECK(!CtrCoop_IsEnabled(), "co-op refuses to start without migration space");
    CHECK(memcmp(&before, &sStorage, sizeof(before)) == 0, "failed migration leaves every box byte unchanged");
}

static void TestHostJoinAndPcLock(void)
{
    uint8_t packet[CTR_COOP_SAVE_CHUNK_MAX];
    uint8_t identity[CTR_COOP_IDENTITY_SIZE];
    bool accepted;
    const struct SentPacket *result;

    ResetFixture();
    CtrCoop_Toggle();
    CtrCoopProtocol_EncodeIdentity(sProfile.guestName, identity, sizeof(identity));
    CtrCoop_HandlePacket(PEER_ID, identity, sizeof(identity));
    CHECK(CountPacket(CTR_COOP_SAVE_BEGIN_PACKET) == 0, "identity before authentication gets no save data");

    {
        const uint8_t wrong[] = {9, 9, 9, 9};
        CtrCoopProtocol_EncodeAuth(wrong, packet, CTR_COOP_AUTH_PACKET_SIZE);
        CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_AUTH_PACKET_SIZE);
        result = LastPacket(CTR_COOP_AUTH_RESULT_PACKET);
        CHECK(result != NULL && CtrCoopProtocol_DecodeAuthResult(result->data, result->len, &accepted) && !accepted, "wrong password is rejected");
    }
    CHECK(CountPacket(CTR_COOP_SAVE_BEGIN_PACKET) == 0, "wrong password gets no save data");
    AuthenticateHost();
    CHECK(CountPacket(CTR_COOP_SAVE_BEGIN_PACKET) == 1, "authenticated identity starts one host snapshot");

    packet[0] = CTR_COOP_PC_REQUEST_PACKET;
    packet[1] = CTR_COOP_PROTOCOL_VERSION;
    CtrCoop_ReleasePC();
    CHECK(CtrCoop_RequestPC(), "host acquires free PC");
    ClearPackets();
    CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_PC_CONTROL_SIZE);
    result = LastPacket(CTR_COOP_PC_RESULT_PACKET);
    CHECK(result != NULL && CtrCoopProtocol_DecodePcResult(result->data, result->len, &accepted) && !accepted, "guest is rejected while host owns PC");
    CHECK(LastPacket(CTR_COOP_PC_CLAIM_PACKET) != NULL, "host claim accompanies rejection");
    CtrCoop_ReleasePC();
    ClearPackets();
    CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_PC_CONTROL_SIZE);
    result = LastPacket(CTR_COOP_PC_RESULT_PACKET);
    CHECK(result != NULL && CtrCoopProtocol_DecodePcResult(result->data, result->len, &accepted) && accepted, "guest acquires released PC");
    CHECK(!CtrCoop_RequestPC(), "host cannot acquire guest-owned PC");
    CtrCoop_HandlePacket(PEER_ID, (const uint8_t[]){CTR_COOP_PC_RELEASE_PACKET, CTR_COOP_PROTOCOL_VERSION}, CTR_COOP_PC_CONTROL_SIZE);
    CHECK(CtrCoop_RequestPC(), "host acquires PC after guest release");
    CtrCoop_PeerLeft(PEER_ID);
    sCoop.peerAuthenticated = true;
    sCoop.peerTeamReady = true;
    sCoop.peerSaveReady = true;
    CtrCoop_ReleasePC();
    ClearPackets();
    CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_PC_CONTROL_SIZE);
    result = LastPacket(CTR_COOP_PC_RESULT_PACKET);
    CHECK(result != NULL && CtrCoopProtocol_DecodePcResult(result->data, result->len, &accepted) && accepted, "disconnect does not leave a stale PC lock");
}

static void TestPcTransactionAndSpawn(void)
{
    static uint8_t payload[CTR_COOP_PC_SYNC_SIZE];
    struct PokemonStorage incoming;
    struct Pokemon party[PARTY_SIZE];
    struct PokemonStorage original;
    CtrCoopSnapshot spawn;
    const struct SentPacket *packet;

    ReadyHost();
    sCoop.peerTeamReady = false;
    CtrCoop_HandlePacket(PEER_ID, (const uint8_t[]){CTR_COOP_PC_REQUEST_PACKET, CTR_COOP_PROTOCOL_VERSION}, CTR_COOP_PC_CONTROL_SIZE);
    memset(&incoming, 0, sizeof(incoming));
    SetBoxMon(&incoming.boxes[2][3], 77);
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        SetMon(&party[i], 31 + i);
    BuildPcPayload(payload, &incoming, party);
    original = sStorage;
    {
        const uint32_t checksum = CtrCoopProtocol_Checksum(payload, sizeof(payload));

        payload[200] ^= 1;
        SendTransfer(CtrCoopProtocol_EncodePcSyncBegin, CtrCoopProtocol_EncodePcSyncChunk,
            payload, sizeof(payload), checksum, true, 0);
        payload[200] ^= 1;
        CHECK(memcmp(&sStorage, &original, sizeof(sStorage)) == 0, "bad PC checksum applies no partial storage");
        CHECK(LastPacket(CTR_COOP_PC_SYNC_READY_PACKET) == NULL, "bad PC checksum is not confirmed");
        CHECK(LastPacket(CTR_COOP_SPAWN_PACKET) == NULL, "bad initial team does not open the world");
        packet = LastPacket(CTR_COOP_PC_SYNC_REQUEST_PACKET);
        CHECK(packet != NULL && packet->data[2] == 0 && packet->data[3] == 0 && packet->data[4] == 0 && packet->data[5] == 0, "bad PC checksum requests a complete retry");

        SendTransfer(CtrCoopProtocol_EncodePcSyncBegin, CtrCoopProtocol_EncodePcSyncChunk,
            payload, sizeof(payload), checksum, false, 0);
    }
    CHECK(LastPacket(CTR_COOP_PC_SYNC_READY_PACKET) != NULL, "complete valid PC transfer is confirmed (state %u, offset %lu)", sCoop.syncState, (unsigned long)sCoop.syncOffset);
    CHECK(sStorage.boxes[2][3].species == 77, "valid PC transfer replaces ordinary storage");
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][CTR_COOP_STORAGE_ROW_SIZE + i].species == 31 + i, "guest party returns to its own row");
    packet = LastPacket(CTR_COOP_SPAWN_PACKET);
    CHECK(packet != NULL && CtrCoopProtocol_DecodeSpawn(packet->data, packet->len, &spawn), "first confirmed team releases spawn");
    CHECK(spawn.mapGroup == 0 && spawn.mapNum == 9 && spawn.mapLayoutId == 10 && spawn.x == 19 && spawn.y == 16, "new profile uses fixed Littleroot spawn");

    spawn.mapGroup = 2;
    spawn.mapNum = 3;
    spawn.mapLayoutId = 44;
    spawn.x = 30;
    spawn.y = 40;
    spawn.facing = 2;
    spawn.movementActionId = CTR_COOP_ACTION_FACE_UP;
    spawn.graphicsId = 105;
    CtrCoopProtocol_Encode(&spawn, payload, CTR_COOP_PACKET_SIZE);
    CtrCoop_HandlePacket(PEER_ID, payload, CTR_COOP_PACKET_SIZE);
    BuildPcPayload(payload, &incoming, party);
    sCoop.pcOwner = PC_OWNER_GUEST;
    sCoop.peerTeamReady = false;
    ClearPackets();
    SendTransfer(CtrCoopProtocol_EncodePcSyncBegin, CtrCoopProtocol_EncodePcSyncChunk,
        payload, sizeof(payload), CtrCoopProtocol_Checksum(payload, sizeof(payload)), true, 0);
    packet = LastPacket(CTR_COOP_SPAWN_PACKET);
    CHECK(packet != NULL && CtrCoopProtocol_DecodeSpawn(packet->data, packet->len, &spawn), "returning profile gets another spawn");
    CHECK(spawn.mapGroup == 2 && spawn.mapNum == 3 && spawn.mapLayoutId == 44 && spawn.x == 30 && spawn.y == 40, "known profile reuses its saved position");
}

static void BuildHostSnapshot(uint8_t *payload, struct SaveBlock2 *save2, struct SaveBlock1 *save1, struct PokemonStorage *storage)
{
    uint32_t offset = 0;

    memcpy(payload + offset, save2, sizeof(*save2));
    offset += sizeof(*save2);
    memcpy(payload + offset, save1, sizeof(*save1));
    offset += sizeof(*save1);
    memcpy(payload + offset, storage, sizeof(*storage));
}

static void AcceptGuestSnapshot(void)
{
    uint8_t result[CTR_COOP_AUTH_RESULT_SIZE];

    CtrCoop_MenuFrame();
    CHECK(LastPacket(CTR_COOP_AUTH_PACKET) != NULL, "guest sends authentication once in session");
    CtrCoopProtocol_EncodeAuthResult(true, result, sizeof(result));
    CtrCoop_HandlePacket(PEER_ID, result, sizeof(result));
    CHECK(LastPacket(CTR_COOP_IDENTITY_PACKET) != NULL, "accepted guest sends profile identity");
}

static void TestGuestJoinFlow(void)
{
    static uint8_t payload[CTR_COOP_SAVE_SIZE];
    struct SaveBlock2 incoming2;
    struct SaveBlock1 incoming1;
    struct PokemonStorage incomingStorage;
    struct SaveBlock1 original1;
    CtrCoopSnapshot spawn;
    uint8_t packet[CTR_COOP_SPAWN_SIZE];

    ResetFixture();
    memset(&incoming2, 0, sizeof(incoming2));
    memset(&incoming1, 0, sizeof(incoming1));
    memset(&incomingStorage, 0, sizeof(incomingStorage));
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        SetBoxMon(&incomingStorage.boxes[CTR_COOP_STORAGE_BOX_ID][CTR_COOP_STORAGE_ROW_SIZE + i], 41 + i);
    incoming1.money = 9876;
    BuildHostSnapshot(payload, &incoming2, &incoming1, &incomingStorage);
    CtrCoop_JoinGame(1234, sProfile.guestPassword);
    AcceptGuestSnapshot();
    original1 = sSaveBlock1;
    SendTransfer(CtrCoopProtocol_EncodeSaveBegin, CtrCoopProtocol_EncodeSaveChunk,
        payload, sizeof(payload), CtrCoopProtocol_Checksum(payload, sizeof(payload)) ^ 1, true, 0);
    CHECK(!CtrCoop_IsEnabled(), "corrupt initial snapshot aborts join");
    CHECK(memcmp(&sSaveBlock1, &original1, sizeof(sSaveBlock1)) == 0, "corrupt initial snapshot leaves guest RAM save unchanged");
    CHECK(CtrCoop_GuestSaveBlocked() == false, "aborted join releases guest save block");

    ResetFixture();
    BuildHostSnapshot(payload, &incoming2, &incoming1, &incomingStorage);
    CtrCoop_JoinGame(1234, sProfile.guestPassword);
    AcceptGuestSnapshot();
    SendTransfer(CtrCoopProtocol_EncodeSaveBegin, CtrCoopProtocol_EncodeSaveChunk,
        payload, sizeof(payload), CtrCoopProtocol_Checksum(payload, sizeof(payload)), true, 0);
    CHECK(sSaveBlock1.money == 9876, "valid initial snapshot is applied");
    CHECK(LastPacket(CTR_COOP_SAVE_READY_PACKET) != NULL, "valid initial snapshot is confirmed");
    CHECK(CtrCoop_InitialTeamSelection(), "first join requires team selection");
    CHECK(CtrCoop_GetSessionState() == LOCAL_SESSION_SEARCHING, "world stays closed during initial team selection");
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        CHECK(gPlayerParty[i].box.species == 41 + i, "guest receives its saved row instead of host party");

    ClearPackets();
    CtrCoop_MenuFrame();
    CHECK(LastPacket(CTR_COOP_PC_REQUEST_PACKET) != NULL, "team selection requests global PC lock");
    CtrCoopProtocol_EncodePcResult(true, packet, CTR_COOP_PC_RESULT_SIZE);
    CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_PC_RESULT_SIZE);
    CtrCoop_MenuFrame();
    CHECK(sPcOpenCount == 1, "guest PC opens only after lock grant");
    CtrCoop_CommitPC();
    CHECK(!CtrCoop_InitialTeamSelection(), "team commit completes local selection");
    CHECK(LastPacket(CTR_COOP_PC_SYNC_BEGIN_PACKET) != NULL, "team commit starts atomic PC transfer");

    spawn.mapGroup = 0;
    spawn.mapNum = 9;
    spawn.mapLayoutId = 10;
    spawn.x = 19;
    spawn.y = 16;
    spawn.facing = DIR_SOUTH;
    spawn.movementActionId = CTR_COOP_ACTION_FACE_DOWN;
    spawn.graphicsId = 105;
    CtrCoopProtocol_EncodeSpawn(&spawn, packet, sizeof(packet));
    CtrCoop_HandlePacket(PEER_ID, packet, sizeof(packet));
    CHECK(CtrCoop_GetSessionState() == LOCAL_SESSION_SEARCHING, "spawn alone does not finish unconfirmed team commit");
    CtrCoop_HandlePacket(PEER_ID, (const uint8_t[]){CTR_COOP_PC_SYNC_READY_PACKET, CTR_COOP_PROTOCOL_VERSION}, CTR_COOP_SAVE_READY_SIZE);
    CHECK(CtrCoop_GetSessionState() == LOCAL_SESSION_IN, "confirmed team and spawn finish join state");
    CHECK(CtrCoop_EnterHostWorld(), "finished join enters host world");
    CHECK(sWarpCount == 1 && sWarpX == 12 && sWarpY == 9, "warp removes map coordinate offset exactly once");
}

#if CTR_COOP_TEST_MAX_SPRINT >= 7
static void TestLogoutTransaction(void)
{
    struct Pokemon party[PARTY_SIZE];
    struct BoxPokemon original[PARTY_SIZE];

    ReadyHost();
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
    {
        SetMon(&party[i], 61 + i);
        SetBoxMon(&original[i], 21 + i);
        sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][CTR_COOP_STORAGE_ROW_SIZE + i] = original[i];
    }
    {
        const uint32_t checksum = CtrCoopProtocol_Checksum((const uint8_t *)party, sizeof(party));

        party[0].data[0] ^= 1;
        SendTransfer(CtrCoopProtocol_EncodeLogoutBegin, CtrCoopProtocol_EncodeLogoutChunk,
            (const uint8_t *)party, sizeof(party), checksum, true, 0);
        party[0].data[0] ^= 1;
        for (uint8_t i = 0; i < PARTY_SIZE; i++)
            CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][CTR_COOP_STORAGE_ROW_SIZE + i].species == 21 + i, "bad logout checksum preserves old row");
        CHECK(LastPacket(CTR_COOP_LOGOUT_READY_PACKET) == NULL, "bad logout is not confirmed");

        SendTransfer(CtrCoopProtocol_EncodeLogoutBegin, CtrCoopProtocol_EncodeLogoutChunk,
            (const uint8_t *)party, sizeof(party), checksum, false, 0);
    }
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][CTR_COOP_STORAGE_ROW_SIZE + i].species == 61 + i, "logout returns every party slot");
    CHECK(!sCoop.peerTeamReady && !CtrCoop_StorageSlotLocked(CTR_COOP_STORAGE_BOX_ID, CTR_COOP_STORAGE_ROW_SIZE), "confirmed logout unlocks guest row");
    CHECK(LastPacket(CTR_COOP_LOGOUT_READY_PACKET) != NULL, "guest disconnect is released only after complete team return");
}
#endif

#if CTR_COOP_TEST_MAX_SPRINT >= 5
static void TestTrainerReservation(void)
{
    uint8_t reserve[CTR_COOP_BATTLE_RESERVE_SIZE];
    uint8_t result[CTR_COOP_BATTLE_RESERVE_RESULT_SIZE];
    const struct SentPacket *packet;
    uint16_t trainerId;
    bool accepted;

    ReadyHost();
    CtrCoopProtocol_EncodeBattleReserve(7, reserve, sizeof(reserve));
    CtrCoop_HandlePacket(PEER_ID, reserve, sizeof(reserve));
    packet = LastPacket(CTR_COOP_BATTLE_RESERVE_RESULT_PACKET);
    CHECK(packet != NULL && CtrCoopProtocol_DecodeBattleReserveResult(packet->data, packet->len, &trainerId, &accepted) && trainerId == 7 && accepted, "first trainer reservation succeeds");
    CtrCoop_HandlePacket(PEER_ID, reserve, sizeof(reserve));
    packet = LastPacket(CTR_COOP_BATTLE_RESERVE_RESULT_PACKET);
    CHECK(packet != NULL && CtrCoopProtocol_DecodeBattleReserveResult(packet->data, packet->len, &trainerId, &accepted) && !accepted, "same trainer cannot be reserved twice");
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    gTrainerBattleOpponent_A = 7;
    CHECK(!CtrCoop_BattleStart(), "host cannot start guest-reserved trainer");

    CtrCoopProtocol_EncodeBattleReserve(8, result, sizeof(result));
    CtrCoop_HandlePacket(PEER_ID, (const uint8_t[]){CTR_COOP_BATTLE_RELEASE_PACKET, CTR_COOP_PROTOCOL_VERSION, 8, 0}, CTR_COOP_BATTLE_RESERVE_SIZE);
    CtrCoop_HandlePacket(PEER_ID, result, sizeof(result));
    packet = LastPacket(CTR_COOP_BATTLE_RESERVE_RESULT_PACKET);
    CHECK(packet != NULL && CtrCoopProtocol_DecodeBattleReserveResult(packet->data, packet->len, &trainerId, &accepted) && !accepted, "wrong release does not free reservation");
    CtrCoop_HandlePacket(PEER_ID, (const uint8_t[]){CTR_COOP_BATTLE_RELEASE_PACKET, CTR_COOP_PROTOCOL_VERSION, 7, 0}, CTR_COOP_BATTLE_RESERVE_SIZE);
    CtrCoop_HandlePacket(PEER_ID, reserve, sizeof(reserve));
    packet = LastPacket(CTR_COOP_BATTLE_RESERVE_RESULT_PACKET);
    CHECK(packet != NULL && CtrCoopProtocol_DecodeBattleReserveResult(packet->data, packet->len, &trainerId, &accepted) && trainerId == 7 && accepted, "matching release permits a new reservation");
}
#endif

#if CTR_COOP_TEST_MAX_SPRINT >= 6
static CtrCoopSnapshot RemoteAt(int16_t x, int16_t y)
{
    CtrCoopSnapshot remote = {0, 9, LAYOUT_LITTLEROOT_TOWN, x, y, DIR_SOUTH, CTR_COOP_ACTION_FACE_DOWN, 105};
    return remote;
}

static void TestDoubleBattleState(void)
{
    uint8_t request[] = {CTR_COOP_DOUBLE_REQUEST_PACKET, CTR_COOP_PROTOCOL_VERSION, 0, 0};

    ReadyHost();
    sCoop.remoteValid = true;
    sCoop.remote = RemoteAt(29, 23);
    gBattleTypeFlags = BATTLE_TYPE_DOUBLE;
    CHECK(!CtrCoop_BattleStart(), "available nearby partner waits for double-battle handshake");
    CHECK(LastPacket(CTR_COOP_DOUBLE_REQUEST_PACKET) != NULL, "nearby double battle sends request");
    CtrCoop_HandlePacket(PEER_ID, (const uint8_t[]){CTR_COOP_DOUBLE_DECLINE_PACKET, CTR_COOP_PROTOCOL_VERSION}, CTR_COOP_DOUBLE_CONTROL_SIZE);
    ClearPackets();
    XFAIL_CHECK(CtrCoop_BattleStart() && LastPacket(CTR_COOP_DOUBLE_REQUEST_PACKET) == NULL, "Sprint 6: a declined partner falls back to a local double battle without a new request");

    ReadyHost();
    sCoop.remoteValid = true;
    sCoop.remote = RemoteAt(34, 23);
    gBattleTypeFlags = BATTLE_TYPE_DOUBLE;
    CHECK(CtrCoop_BattleStart(), "distant partner falls back to original local double battle");
    CHECK(sCoop.battleActive && LastPacket(CTR_COOP_DOUBLE_REQUEST_PACKET) == NULL, "fallback starts without network handshake");

    ReadyHost();
    sCoop.remoteValid = true;
    sCoop.remote = RemoteAt(27, 24);
    CtrCoop_HandlePacket(PEER_ID, request, sizeof(request));
    CHECK(LastPacket(CTR_COOP_DOUBLE_ACCEPT_PACKET) != NULL && sCoop.doubleState == DOUBLE_STATE_WAIT_START, "available partner accepts incoming double battle");
    sCoop.pcOpen = true;
    CtrCoop_HandlePacket(PEER_ID, request, sizeof(request));
    CHECK(LastPacket(CTR_COOP_DOUBLE_DECLINE_PACKET) != NULL, "PC use makes partner unavailable");
}
#endif

#if CTR_COOP_TEST_MAX_SPRINT >= 5
static uint32_t BuildBattlePayload(uint8_t *payload, uint16_t incomingSpecies)
{
    struct Pokemon party[PARTY_SIZE];
    struct BoxPokemon incoming;

    memset(payload, 0, BATTLE_HEADER_SIZE + sizeof(party) + BATTLE_STORAGE_RECORD_SIZE);
    memcpy(payload, "CTB1", 4);
    payload[10] = 1;
    payload[11] = 0;
    payload[12] = 0;
    payload[13] = 0;
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        SetMon(&party[i], 71 + i);
    memcpy(payload + BATTLE_HEADER_SIZE, party, sizeof(party));
    payload[BATTLE_HEADER_SIZE + sizeof(party)] = 0;
    payload[BATTLE_HEADER_SIZE + sizeof(party) + 1] = 0;
    SetBoxMon(&incoming, incomingSpecies);
    memcpy(payload + BATTLE_HEADER_SIZE + sizeof(party) + 2, &incoming, sizeof(incoming));
    return BATTLE_HEADER_SIZE + sizeof(party) + BATTLE_STORAGE_RECORD_SIZE;
}

static void TestBattleStorageCollision(void)
{
    static uint8_t payload[BATTLE_HEADER_SIZE + sizeof(struct Pokemon) * PARTY_SIZE + BATTLE_STORAGE_RECORD_SIZE];
    struct PokemonStorage before;
    uint32_t size;

    ReadyHost();
    SetBoxMon(&sStorage.boxes[0][0], 90);
    size = BuildBattlePayload(payload, 91);
    SendTransfer(CtrCoopProtocol_EncodeBattleBegin, CtrCoopProtocol_EncodeBattleChunk,
        payload, size, CtrCoopProtocol_Checksum(payload, size), true, 0);
    CHECK(sStorage.boxes[0][0].species == 90, "collision keeps original occupant in place");
    CHECK(sStorage.boxes[0][1].species == 91, "collision moves incoming catch to a free slot");
    CHECK(LastPacket(CTR_COOP_BATTLE_READY_PACKET) != NULL, "relocated battle result is confirmed");
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][CTR_COOP_STORAGE_ROW_SIZE + i].species == 71 + i, "battle result updates correct player row");

    ReadyHost();
    for (uint8_t box = 0; box < TOTAL_BOXES_COUNT; box++)
        for (uint8_t position = box == CTR_COOP_STORAGE_BOX_ID ? CTR_COOP_STORAGE_ROW_SIZE * 2 : 0; position < IN_BOX_COUNT; position++)
            SetBoxMon(&sStorage.boxes[box][position], 100 + box);
    before = sStorage;
    SendTransfer(CtrCoopProtocol_EncodeBattleBegin, CtrCoopProtocol_EncodeBattleChunk,
        payload, size, CtrCoopProtocol_Checksum(payload, size), true, 0);
    CHECK(memcmp(&before, &sStorage, sizeof(before)) == 0, "full storage rejects battle result without partial writes");
    CHECK(LastPacket(CTR_COOP_BATTLE_READY_PACKET) == NULL, "unapplied battle result is not confirmed");
    SetBoxMon(&sStorage.boxes[0][1], 0);
    SendTransfer(CtrCoopProtocol_EncodeBattleBegin, CtrCoopProtocol_EncodeBattleChunk,
        payload, size, CtrCoopProtocol_Checksum(payload, size), true, 0);
    CHECK(sStorage.boxes[0][1].species == 91 && LastPacket(CTR_COOP_BATTLE_READY_PACKET) != NULL, "fresh begin retries a previously unapplied battle result");
}
#endif

int main(void)
{
    TestStorageMigration();
    TestHostJoinAndPcLock();
    TestPcTransactionAndSpawn();
    TestGuestJoinFlow();
#if CTR_COOP_TEST_MAX_SPRINT >= 7
    TestLogoutTransaction();
#endif
#if CTR_COOP_TEST_MAX_SPRINT >= 5
    TestTrainerReservation();
    TestBattleStorageCollision();
#endif
#if CTR_COOP_TEST_MAX_SPRINT >= 6
    TestDoubleBattleState();
#endif
    if (sFailures != 0)
    {
        printf("%d co-op session tests failed\n", sFailures);
        return 1;
    }
    printf("co-op session tests passed (%d expected acceptance failure%s)\n", sExpectedFailures, sExpectedFailures == 1 ? "" : "s");
    return 0;
}

void LocalLink_Send(uint16_t dest, const void *data, uint32_t len)
{
    if (sSentCount >= MAX_SENT_PACKETS || len > MAX_PACKET_SIZE)
    {
        sFailures++;
        return;
    }
    sSent[sSentCount].to = dest;
    sSent[sSentCount].len = len;
    memcpy(sSent[sSentCount].data, data, len);
    sSentCount++;
}

int LocalLink_GetPeers(struct LocalPeer *out, int max)
{
    if (sPeerCount == 0 || max == 0)
        return 0;
    memset(out, 0, sizeof(*out));
    out[0].valid = TRUE;
    out[0].id = PEER_ID;
    return 1;
}

uint8_t LocalLink_GetSession(void) { return sSession; }
uint8_t LocalLink_GetSessionKind(void) { return sSessionKind; }
uint32_t LocalLink_Generation(void) { return 1; }
void LocalLink_StartSession(uint8_t kind) { sSessionKind = kind; sSession = LOCAL_SESSION_IN; }
void LocalLink_JoinSession(uint8_t kind, uint32_t token, const uint8_t *password) { (void)token; (void)password; sSessionKind = kind; sSession = LOCAL_SESSION_IN; }
void LocalLink_DiscoverSessions(uint8_t kind) { (void)kind; sSession = LOCAL_SESSION_SEARCHING; }
void LocalLink_LeaveSession(void) { sSession = LOCAL_SESSION_NONE; }
void LocalLink_SetHostName(const uint8_t *name) { (void)name; }
void LocalLink_SetHostPassword(const uint8_t *password) { (void)password; }
int LocalLink_GetFoundSessions(struct LocalFoundSession *out, int max) { (void)out; (void)max; return 0; }

const struct CtrCoopProfile *CtrCoopProfile_Get(void) { return &sProfile; }
bool CtrCoopProfile_HasHostPassword(void) { return sProfile.hostPassword[0] != 0xFF; }

void CtrLog_Write(CtrLogCategory category, const char *format, ...)
{
    (void)category;
    (void)format;
}

void ZeroMonData(struct Pokemon *mon) { memset(mon, 0, sizeof(*mon)); }
uint32_t GetBoxMonData(const struct BoxPokemon *mon, int attribute) { (void)attribute; return mon->species; }
void BoxMonToMon(const struct BoxPokemon *boxMon, struct Pokemon *mon) { ZeroMonData(mon); mon->box = *boxMon; }
uint8_t CalculatePlayerPartyCount(void) { uint8_t count = 0; while (count < PARTY_SIZE && gPlayerParty[count].box.species != 0) count++; return count; }
uint32_t GetBoxMonDataAt(uint8_t box, uint8_t position, int attribute) { return GetBoxMonData(&gPokemonStoragePtr->boxes[box][position], attribute); }
s16 GetFirstFreeBoxSpot(uint8_t box) { for (uint8_t i = 0; i < IN_BOX_COUNT; i++) if (sStorage.boxes[box][i].species == 0) return i; return -1; }
void SetBoxMonAt(uint8_t box, uint8_t position, const struct BoxPokemon *mon) { gPokemonStoragePtr->boxes[box][position] = *mon; }
void ZeroBoxMonData(struct BoxPokemon *mon) { memset(mon, 0, sizeof(*mon)); }
void ZeroBoxMonAt(uint8_t box, uint8_t position) { ZeroBoxMonData(&gPokemonStoragePtr->boxes[box][position]); }
void SetBagItemsPointers(void) {}
void SetDecorationInventoriesPointers(void) {}
void CopyPartyAndObjectsFromSave(void) {}
void LoadPlayerBag(void) {}
uint8_t GetFaceDirectionMovementAction(uint8_t direction) { return direction >= 1 && direction <= 4 ? direction - 1 : CTR_COOP_ACTION_FACE_DOWN; }
void SetWarpDestination(uint8_t mapGroup, uint8_t mapNum, int8_t warpId, int8_t x, int8_t y) { (void)mapGroup; (void)mapNum; (void)warpId; sWarpX = x; sWarpY = y; }
void WarpIntoMap(void) { sWarpCount++; }
void RemoveObjectEvent(struct ObjectEvent *objectEvent) { objectEvent->active = FALSE; }
uint8_t GetPlayerFacingDirection(void) { return DIR_SOUTH; }
uint8_t ObjectEventGetHeldMovementActionId(struct ObjectEvent *objectEvent) { (void)objectEvent; return CTR_COOP_ACTION_FACE_DOWN; }
uint8_t TryGetObjectEventIdByLocalIdAndMap(uint8_t localId, uint8_t mapNum, uint8_t mapGroup, uint8_t *objectEventId) { (void)localId; (void)mapNum; (void)mapGroup; (void)objectEventId; return 1; }
bool FreeObjectEventSlotNearPlayer(void) { return true; }
uint8_t MapGridGetElevationAt(int16_t x, int16_t y) { (void)x; (void)y; return 0; }
uint8_t SpawnSpecialObjectEvent(const struct ObjectEventTemplate *objectEventTemplate) { (void)objectEventTemplate; return OBJECT_EVENTS_COUNT; }
void ObjectEventTurn(struct ObjectEvent *objectEvent, uint8_t direction) { (void)objectEvent; (void)direction; }
void ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *objectEvent) { (void)objectEvent; }
bool ObjectEventIsHeldMovementActive(struct ObjectEvent *objectEvent) { (void)objectEvent; return false; }
void ObjectEventSetHeldMovement(struct ObjectEvent *objectEvent, uint8_t movementActionId) { (void)objectEvent; (void)movementActionId; }
void MoveObjectEventToMapCoords(struct ObjectEvent *objectEvent, int16_t x, int16_t y) { objectEvent->currentCoords.x = x; objectEvent->currentCoords.y = y; }
void ShowPokemonStorageSystemPC(void) { sPcOpenCount++; }
void PlaySE(uint16_t song) { (void)song; }
uint16_t Random(void) { return 0x1234; }
void CB2_InitBattle(void) {}
void CB2_InitEndLinkBattle(void) {}
void SetMainCallback2(MainCallback callback) { sMainCallback = callback; }
bool AddBagItem(uint16_t itemId, uint16_t quantity) { (void)itemId; (void)quantity; return true; }
bool RemoveBagItem(uint16_t itemId, uint16_t quantity) { (void)itemId; (void)quantity; return true; }
uint32_t GetMoney(uint32_t *money) { return *money; }
void AddMoney(uint32_t *money, uint32_t amount) { *money += amount; }
void RemoveMoney(uint32_t *money, uint32_t amount) { if (*money > amount) *money -= amount; else *money = 0; }
bool FlagGet(uint16_t id) { return id != 0 && id < SPECIAL_FLAGS_START && (sSaveBlock1.flags[id / 8] & (1 << (id & 7))) != 0; }
