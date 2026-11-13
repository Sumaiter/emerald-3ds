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
static bool sFailBagChange;
static int8_t sWarpX;
static int8_t sWarpY;
static MainCallback sMainCallback;
static struct CtrCoopProfile sProfile;
static struct SaveBlock2 sSaveBlock2;
static struct SaveBlock1 sSaveBlock1;
static struct PokemonStorage sStorage;

uint8_t CtrCoopBike_LocalMode(void) { return CTR_COOP_BIKE_NONE; }
uint8_t CtrCoopBike_LocalMovementSequence(void) { return 0; }
bool CtrCoopBike_SetRemote(uint8_t id, uint8_t mode) { (void)id; (void)mode; return false; }
void CtrCoopBike_ClearRemote(void) {}
void CtrCoopBike_Reset(void) {}

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
    sFailBagChange = false;
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
    CtrCoop_ReleasePC();
    AuthenticateHost();
    sCoop.peerTeamReady = true;
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

static void TestGuestNameAfterHostSnapshot(void)
{
    static const uint8_t hostName[8] = {0xC2, 0xC9, 0xCD, 0xCE, EOS, EOS, EOS, EOS};
    static const uint8_t guestName[7] = {0xC1, 0xBB, 0xCD, 0xCE, 0xD5, 0xD6, 0xD7};
    static const uint8_t lengths[] = {0, 1, 4, 7, 8};
    static const uint8_t defaultName[8] = {0xC1, 0xBB, 0xCD, 0xCE, EOS, EOS, EOS, EOS};
    struct SaveBlock2 incoming2, expected2;
    struct SaveBlock1 incoming1 = {0};
    struct PokemonStorage incomingStorage = {0};
    uint8_t expectedName[8];

    for (unsigned i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++)
    {
        ResetFixture();
        sCoop.guest = true;
        memset(&incoming2, 0x35, sizeof(incoming2));
        memcpy(incoming2.playerName, hostName, sizeof(hostName));
        memset(sProfile.guestName, 0x99, sizeof(sProfile.guestName));
        memcpy(sProfile.guestName, guestName, sizeof(guestName));
        sProfile.guestNameLength = lengths[i];
        memset(expectedName, EOS, sizeof(expectedName));
        if (lengths[i] > 0 && lengths[i] <= CTR_COOP_GUEST_NAME_LENGTH)
            memcpy(expectedName, guestName, lengths[i]);
        else
            memcpy(expectedName, defaultName, sizeof(expectedName));
        BuildHostSnapshot(sSaveData, &incoming2, &incoming1, &incomingStorage);
        uint32_t checksum = CtrCoopProtocol_Checksum(sSaveData, sizeof(sSaveData));
        ApplyHostSave();
        CHECK(memcmp(sCoop.hostName, hostName, sizeof(hostName)) == 0,
            "PC notice retains the real host name before applying guest identity");
        CHECK(memcmp(gSaveBlock2Ptr->playerName, expectedName, sizeof(expectedName)) == 0,
            "scenario player name is the guest profile name, with a proper game-text terminator");
        expected2 = incoming2;
        memcpy(expected2.playerName, expectedName, sizeof(expectedName));
        CHECK(memcmp(gSaveBlock2Ptr, &expected2, sizeof(expected2)) == 0,
            "guest name substitution preserves all other host SaveBlock2 fields");
        CHECK(CtrCoopProtocol_Checksum(sSaveData, sizeof(sSaveData)) == checksum
           && memcmp(incoming2.playerName, hostName, sizeof(hostName)) == 0,
            "guest dialog name never changes the host snapshot or its checksum");
        ApplyHostSave();
        CHECK(memcmp(gSaveBlock2Ptr->playerName, expectedName, sizeof(expectedName)) == 0,
            "reapplying the host snapshot on rejoin keeps the guest's own name");
        CtrCoop_Stop();
        CHECK(CtrCoop_GuestSaveBlocked(), "guest identity substitution cannot permit writing the local SAV");
    }
    ResetFixture();
    memcpy(gSaveBlock2Ptr->playerName, hostName, sizeof(hostName));
    StartSession(false);
    CHECK(memcmp(gSaveBlock2Ptr->playerName, hostName, sizeof(hostName)) == 0,
        "hosting keeps the host's own scenario name");
}

static void AcceptGuestSnapshot(void)
{
    uint8_t result[CTR_COOP_AUTH_RESULT_SIZE];
    uint8_t info[CTR_COOP_JOIN_INFO_SIZE];

    CtrCoop_MenuFrame();
    CHECK(LastPacket(CTR_COOP_AUTH_PACKET) != NULL, "guest sends authentication once in session");
    CtrCoopProtocol_EncodeAuthResult(true, result, sizeof(result));
    CtrCoop_HandlePacket(PEER_ID, result, sizeof(result));
    CHECK(LastPacket(CTR_COOP_IDENTITY_PACKET) != NULL, "accepted guest sends profile identity");
    CtrCoopProtocol_EncodeJoinInfo(false, info, sizeof(info));
    CtrCoop_HandlePacket(PEER_ID, info, sizeof(info));
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
    CHECK(memcmp(gSaveBlock2Ptr->playerName, sProfile.guestName, sizeof(gSaveBlock2Ptr->playerName)) == 0,
        "packet-based join uses the guest's own name for scenario placeholders");
    CHECK(LastPacket(CTR_COOP_SAVE_READY_PACKET) != NULL, "valid initial snapshot is confirmed");
    CHECK(CtrCoop_InitialTeamSelection(), "loaded guest team still needs host confirmation");
    CHECK(CtrCoop_GetSessionState() == LOCAL_SESSION_SEARCHING, "world stays closed until team confirmation");
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        CHECK(gPlayerParty[i].box.species == 41 + i, "guest receives its saved row instead of host party");

    ClearPackets();
    CtrCoop_MenuFrame();
    CHECK(LastPacket(CTR_COOP_PC_REQUEST_PACKET) != NULL, "team selection requests global PC lock");
    CtrCoopProtocol_EncodePcResult(true, packet, CTR_COOP_PC_RESULT_SIZE);
    CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_PC_RESULT_SIZE);
    CtrCoop_MenuFrame();
    CHECK(sPcOpenCount == 0, "saved guest team is submitted without opening a PC menu");
    CHECK(!CtrCoop_InitialTeamSelection(), "saved team is submitted automatically after the lock grant");
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
    CHECK(packet != NULL && CtrCoopProtocol_DecodeBattleReserveResult(packet->data, packet->len, &trainerId, &accepted) && accepted, "repeated reservation acknowledges the same owner without starting another battle");
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
    CtrCoopSnapshot remote = {0, 9, LAYOUT_LITTLEROOT_TOWN, x, y, DIR_SOUTH, CTR_COOP_ACTION_FACE_DOWN, 105, 0, 0};
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

#if CTR_COOP_TEST_MAX_SPRINT >= 5
static void SetBattleMoney(uint32_t value) { sSaveBlock1.money = value ^ sSaveBlock2.encryptionKey; }
static uint32_t BattleMoney(void) { return sSaveBlock1.money ^ sSaveBlock2.encryptionKey; }
static uint16_t BattleCoins(void) { return sSaveBlock1.coins ^ (uint16_t)sSaveBlock2.encryptionKey; }

#if CTR_COOP_TEST_MAX_SPRINT >= 6
static void TestDoubleResultHeader(void)
{
    static uint8_t payload[BATTLE_HEADER_SIZE + sizeof(struct Pokemon) * PARTY_SIZE + BATTLE_STORAGE_RECORD_SIZE];
    uint32_t size;

    ReadyHost();
    CHECK(BATTLE_HEADER_SIZE % 4 == 0, "double result keeps party data aligned");
    CHECK(BeginLocalBattle(), "double result fixture begins a local battle");
    sCoop.doubleState = DOUBLE_STATE_ACTIVE;
    sCoop.doubleInitiator = true;
    SendBattleResult();
    CHECK(ReadBattle16(sBattleSendData, 14) == 1 && ReadBattle16(sBattleSendData, 16) == 1,
          "double result has separate sequence and battle-kind fields");

    ReadyHost();
    SetBattleMoney(100);
    size = BuildBattlePayload(payload, 91);
    payload[4] = 50;
    payload[14] = 1;
    payload[16] = 1;
    SendTransfer(CtrCoopProtocol_EncodeBattleBegin, CtrCoopProtocol_EncodeBattleChunk,
        payload, size, CtrCoopProtocol_Checksum(payload, size), true, 0);
    CHECK(BattleMoney() == 100 && sStorage.boxes[0][0].species == 91,
          "double result applies storage without duplicating the shared reward");
    CHECK(sRemotePartyValid && sRemoteParty[0].box.species == 71,
          "double result refreshes the remote party cache");
    ClearPackets();
    SendTransfer(CtrCoopProtocol_EncodeBattleBegin, CtrCoopProtocol_EncodeBattleChunk,
        payload, size, CtrCoopProtocol_Checksum(payload, size), true, 0);
    CHECK(BattleMoney() == 100 && sStorage.boxes[0][1].species == 0
       && LastPacket(CTR_COOP_BATTLE_READY_PACKET) != NULL,
          "double result replay is acknowledged without a second catch or reward");
}
#endif

static void TestBattleEncryptedDelta(void)
{
    uint32_t offset;

    ReadyHost();
    sSaveBlock2.encryptionKey = 0x12345678;
    SetBattleMoney(1000);
    sSaveBlock1.coins = 50 ^ (uint16_t)sSaveBlock2.encryptionKey;
    sSaveBlock1.bagPocket_Items[0].itemId = 13;
    sSaveBlock1.bagPocket_Items[0].quantity = 4 ^ (uint16_t)sSaveBlock2.encryptionKey;
    CHECK(CtrCoop_BattleStart(), "host starts its own wild battle");
    /* CB2_InitBattle moves/rekeys the save blocks after our start hook. */
    sSaveBlock2.encryptionKey = 0x87654321;
    SetBattleMoney(1100);
    sSaveBlock1.coins = 53 ^ (uint16_t)sSaveBlock2.encryptionKey;
    sSaveBlock1.bagPocket_Items[0].quantity = 3 ^ (uint16_t)sSaveBlock2.encryptionKey;
    CtrCoop_BattleEnd();
    CHECK(ReadBattle32(sBattleSendData, 4) == 100, "money delta uses the original and current encryption keys");
    CHECK(ReadBattle16(sBattleSendData, 8) == 3, "coin delta is computed from decoded quantities");
    CHECK(ReadBattle16(sBattleSendData, 10) == 0 && ReadBattle16(sBattleSendData, 12) == 1,
        "rekeying itself does not generate inventory or storage changes");
    offset = BATTLE_HEADER_SIZE + sizeof(gPlayerParty);
    CHECK(ReadBattle16(sBattleSendData, offset) == 13
       && (int16_t)ReadBattle16(sBattleSendData, offset + 2) == -1, "used item has a single decoded delta");
    CHECK(ReadBattle16(sBattleSendData, 14) == 1, "each frozen result has a sequence number");
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].species == 1, "host result stays in the host row");
}

static uint32_t BuildRewardPayload(uint8_t *payload, uint16_t sequence)
{
    uint32_t size = BuildBattlePayload(payload, 91);
    payload[4] = 50;
    payload[8] = 2;
    payload[12] = 1;
    payload[14] = sequence & 255;
    payload[15] = sequence >> 8;
    payload[size++] = 13;
    payload[size++] = 0;
    payload[size++] = 1;
    payload[size++] = 0;
    return size;
}

static void ReceiveBattlePayload(const uint8_t *payload, uint32_t size)
{
    SendTransfer(CtrCoopProtocol_EncodeBattleBegin, CtrCoopProtocol_EncodeBattleChunk,
        payload, size, CtrCoopProtocol_Checksum(payload, size), true, 0);
}

static void TestBattleResultExactlyOnce(void)
{
    uint8_t payload[BATTLE_HEADER_SIZE + sizeof(gPlayerParty) + BATTLE_STORAGE_RECORD_SIZE + BATTLE_ITEM_RECORD_SIZE];
    uint32_t size = BuildRewardPayload(payload, 1);
    struct PokemonStorage before;

    ReadyHost();
    sSaveBlock2.encryptionKey = 0x12345678;
    SetBattleMoney(100);
    sSaveBlock1.coins = 10 ^ (uint16_t)sSaveBlock2.encryptionKey;
    ReceiveBattlePayload(payload, size);
    CHECK(BattleMoney() == 150 && BattleCoins() == 12, "remote reward is decoded into the local encrypted save");
    CHECK(sSaveBlock1.bagPocket_Items[0].itemId == 13
       && (sSaveBlock1.bagPocket_Items[0].quantity ^ (uint16_t)sSaveBlock2.encryptionKey) == 1,
        "remote item is applied once");
    before = sStorage;
    ClearPackets();
    ReceiveBattlePayload(payload, size);
    CHECK(BattleMoney() == 150 && BattleCoins() == 12 && memcmp(&before, &sStorage, sizeof(before)) == 0,
        "lost acknowledgement cannot duplicate money, coins or a relocated catch");
    CHECK((sSaveBlock1.bagPocket_Items[0].quantity ^ (uint16_t)sSaveBlock2.encryptionKey) == 1
       && LastPacket(CTR_COOP_BATTLE_READY_PACKET) != NULL, "duplicate is acknowledged without adding another item");
    payload[14] = 2;
    ReceiveBattlePayload(payload, size);
    CHECK(BattleMoney() == 200, "a later battle with identical rewards still applies");
    payload[14] = 1;
    ReceiveBattlePayload(payload, size);
    CHECK(BattleMoney() == 200, "an older result cannot replay after a newer battle");
}

static void TestConcurrentBattleAndPcResults(void)
{
    uint8_t payload[BATTLE_HEADER_SIZE + sizeof(gPlayerParty) + BATTLE_STORAGE_RECORD_SIZE + BATTLE_ITEM_RECORD_SIZE];
    uint32_t size = BuildRewardPayload(payload, 1);

    ReadyHost();
    SetBattleMoney(100);
    CHECK(CtrCoop_BattleStart(), "local battle can run while the peer fights independently");
    SetBattleMoney(120);
    ReceiveBattlePayload(payload, size);
    CHECK(BattleMoney() == 120 && sCoop.battleReceiveState == BATTLE_SYNC_READY,
        "peer result is held until the local battle has frozen its own changes");
    CHECK(LastPacket(CTR_COOP_BATTLE_READY_PACKET) == NULL, "queued result is not acknowledged before application");
    CtrCoop_BattleEnd();
    CHECK(ReadBattle32(sBattleSendData, 4) == 20, "local result excludes the other player's reward");
    CtrCoop_BattleTick();
    CHECK(BattleMoney() == 170 && LastPacket(CTR_COOP_BATTLE_READY_PACKET) != NULL,
        "queued result applies after local battle end");
    CHECK(ReadBattle32(sBattleSendData, 4) == 20, "receiving a result never mutates the frozen outgoing result");

    ReadyHost();
    CHECK(CtrCoop_RequestPC(), "host opens PC before peer battle ends");
    ReceiveBattlePayload(payload, size);
    CHECK(sStorage.boxes[0][0].species == 0 && sCoop.battleReceiveState == BATTLE_SYNC_READY,
        "incoming catch cannot overwrite a live PC transaction");
    CtrCoop_ReleasePC();
    CtrCoop_BattleTick();
    CHECK(sStorage.boxes[0][0].species == 91, "catch applies after the PC is released");
}

static void TestBattleInventoryRollbackAndRetry(void)
{
    uint8_t payload[BATTLE_HEADER_SIZE + sizeof(gPlayerParty) + BATTLE_STORAGE_RECORD_SIZE + BATTLE_ITEM_RECORD_SIZE];
    uint32_t size = BuildRewardPayload(payload, 1);
    struct PokemonStorage before;

    ReadyHost();
    before = sStorage;
    SetBattleMoney(100);
    sFailBagChange = true;
    ReceiveBattlePayload(payload, size);
    CHECK(BattleMoney() == 100 && memcmp(&before, &sStorage, sizeof(before)) == 0,
        "failed item application leaves money, catches and party rows unchanged");
    CHECK(LastPacket(CTR_COOP_BATTLE_READY_PACKET) == NULL, "failed result is not acknowledged");
    sFailBagChange = false;
    ReceiveBattlePayload(payload, size);
    CHECK(BattleMoney() == 150 && sStorage.boxes[0][0].species == 91,
        "same sequence can retry successfully after storage becomes available");
}

static void TestBattleAndPcArbitration(void)
{
    uint8_t packet[CTR_COOP_BATTLE_RESERVE_SIZE];
    const struct SentPacket *last;
    uint16_t trainer;
    bool accepted;

    ReadyHost();
    CHECK(CtrCoop_BattleStart(), "host starts wild battle");
    CHECK(!CtrCoop_RequestPC(), "host PC stays closed while its own battle is active");
    CtrCoop_HandlePacket(PEER_ID,(uint8_t[]){CTR_COOP_PC_REQUEST_PACKET,CTR_COOP_PROTOCOL_VERSION},CTR_COOP_PC_CONTROL_SIZE);
    last=LastPacket(CTR_COOP_PC_RESULT_PACKET);
    CHECK(last && CtrCoopProtocol_DecodePcResult(last->data,last->len,&accepted) && !accepted,
        "guest PC request cannot race an active host battle");
    CtrCoopProtocol_EncodeBattleReserve(0,packet,sizeof(packet));
    CtrCoop_HandlePacket(PEER_ID,packet,sizeof(packet));
    last=LastPacket(CTR_COOP_BATTLE_RESERVE_RESULT_PACKET);
    CHECK(last && CtrCoopProtocol_DecodeBattleReserveResult(last->data,last->len,&trainer,&accepted) && accepted,
        "host and guest can still fight wild battles concurrently");

    ReadyHost();
    CtrCoopProtocol_EncodeBattleReserve(0,packet,sizeof(packet));
    CtrCoop_HandlePacket(PEER_ID,packet,sizeof(packet));
    CHECK(sCoop.peerBattleActive && !CtrCoop_RequestPC(), "guest battle grant protects storage before the guest starts");
    CtrCoop_HandlePacket(PEER_ID,(uint8_t[]){CTR_COOP_BATTLE_RELEASE_PACKET,CTR_COOP_PROTOCOL_VERSION,0,0},sizeof(packet));
    CHECK(!sCoop.peerBattleActive && CtrCoop_RequestPC(), "wild battle release unlocks the PC again");
    CtrCoopProtocol_EncodeBattleReserve(0,packet,sizeof(packet));
    CtrCoop_HandlePacket(PEER_ID,packet,sizeof(packet));
    last=LastPacket(CTR_COOP_BATTLE_RESERVE_RESULT_PACKET);
    CHECK(last && CtrCoopProtocol_DecodeBattleReserveResult(last->data,last->len,&trainer,&accepted) && !accepted,
        "a pending guest battle waits for an already open PC visit");
}

static void TestMultipleBattleCatchDestinations(void)
{
    uint8_t payload[BATTLE_HEADER_SIZE + sizeof(gPlayerParty) + BATTLE_STORAGE_RECORD_SIZE * 2];
    struct BoxPokemon second;
    uint32_t size = BuildBattlePayload(payload, 91);

    ReadyHost();
    SetBoxMon(&sStorage.boxes[0][0], 90);
    payload[10] = 2;
    payload[size] = 0;
    payload[size+1] = 1;
    SetBoxMon(&second,92);
    memcpy(payload+size+2,&second,sizeof(second));
    size += BATTLE_STORAGE_RECORD_SIZE;
    ReceiveBattlePayload(payload,size);
    CHECK(sStorage.boxes[0][0].species == 90 && sStorage.boxes[0][1].species == 92
       && sStorage.boxes[0][2].species == 91, "relocation reserves later direct catch targets without overwriting either catch");

    ReadyHost();
    payload[BATTLE_HEADER_SIZE + sizeof(gPlayerParty)] = CTR_COOP_STORAGE_BOX_ID;
    ReceiveBattlePayload(payload,size);
    CHECK(LastPacket(CTR_COOP_BATTLE_READY_PACKET) == NULL
       && sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].species == 1,
        "battle catch records cannot overwrite a protected player row");
}

static void TestBattleTransferResumeAndReservationRetry(void)
{
    uint8_t payload[BATTLE_HEADER_SIZE + sizeof(gPlayerParty) + BATTLE_STORAGE_RECORD_SIZE];
    uint8_t packet[CTR_COOP_SAVE_CHUNK_MAX];
    uint32_t size = BuildBattlePayload(payload, 91), offset;
    const struct SentPacket *last;
    bool accepted;
    uint16_t trainer;

    ReadyHost();
    CtrCoopProtocol_EncodeBattleBegin(size, CtrCoopProtocol_Checksum(payload, size), packet, CTR_COOP_SAVE_BEGIN_SIZE);
    CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_SAVE_BEGIN_SIZE);
    CtrCoopProtocol_EncodeBattleChunk(0, payload, CTR_COOP_SAVE_CHUNK_DATA_MAX, packet, CTR_COOP_SAVE_CHUNK_MAX);
    CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_SAVE_CHUNK_MAX);
    CtrCoopProtocol_EncodeBattleBegin(size, CtrCoopProtocol_Checksum(payload, size), packet, CTR_COOP_SAVE_BEGIN_SIZE);
    CtrCoop_HandlePacket(PEER_ID, packet, CTR_COOP_SAVE_BEGIN_SIZE);
    last = LastPacket(CTR_COOP_BATTLE_REQUEST_PACKET);
    CHECK(last && CtrCoopProtocol_DecodeBattleRequest(last->data,last->len,&offset) && offset == CTR_COOP_SAVE_CHUNK_DATA_MAX,
        "sender retry resumes a stalled transfer at the next missing chunk");

    ReadyHost();
    sCoop.guest = true;
    sCoop.authenticated = sCoop.saveReady = true;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    gTrainerBattleOpponent_A = 7;
    CHECK(!CtrCoop_BattleStart(), "guest waits for trainer reservation");
    ClearPackets();
    for (int i=0; i<120; i++) CtrCoop_BattleTick();
    CHECK(CountPacket(CTR_COOP_BATTLE_RESERVE_PACKET) == 1, "missing reservation acknowledgement is retried");
    CtrCoopProtocol_EncodeBattleReserveResult(7,true,packet,CTR_COOP_BATTLE_RESERVE_RESULT_SIZE);
    CtrCoop_HandlePacket(PEER_ID,packet,CTR_COOP_BATTLE_RESERVE_RESULT_SIZE);
    CHECK(CtrCoop_BattleStart(), "guest starts only after reservation grant");
    CHECK(CtrCoop_BattleStart(), "battle initialization does not start a second transaction");
    CtrCoopProtocol_EncodeBattleReserveResult(7,true,packet,CTR_COOP_BATTLE_RESERVE_RESULT_SIZE);
    CHECK(CtrCoopProtocol_DecodeBattleReserveResult(packet,CTR_COOP_BATTLE_RESERVE_RESULT_SIZE,&trainer,&accepted)
       && trainer == 7 && accepted, "reservation response remains valid under the integrated protocol");
    gBattleOutcome = B_OUTCOME_RAN;
    CtrCoop_BattleEnd();
    ClearPackets();
    ReleaseTrainerReservation();
    CHECK(CountPacket(CTR_COOP_BATTLE_RELEASE_PACKET) == 0,
        "PC protection remains held until the battle result is confirmed");
    CtrCoop_HandlePacket(PEER_ID,(uint8_t[]){CTR_COOP_BATTLE_READY_PACKET,CTR_COOP_PROTOCOL_VERSION},CTR_COOP_SAVE_READY_SIZE);
    ReleaseTrainerReservation();
    CHECK(CountPacket(CTR_COOP_BATTLE_RELEASE_PACKET) == 1,
        "confirmed battle result permits the reservation to be released");

    ReadyHost();
    sCoop.peerTrainerId = 7;
    sCoop.peerBattleActive = true;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    gTrainerBattleOpponent_A = 7;
    gMain.savedCallback = CB2_Overworld;
    CHECK(!CtrCoop_BattleStart(), "host waits while guest owns trainer");
    sSaveBlock1.flags[(TRAINER_FLAGS_START+7)/8] |= 1 << ((TRAINER_FLAGS_START+7)&7);
    CHECK(!CtrCoop_BattleStart() && !sCoop.battleActive && sMainCallback == CB2_Overworld,
        "trainer defeated by peer cancels waiting battle without another reward");
}
#endif

int main(void)
{
    TestGuestNameAfterHostSnapshot();
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
    TestBattleEncryptedDelta();
    TestBattleResultExactlyOnce();
    TestConcurrentBattleAndPcResults();
    TestBattleInventoryRollbackAndRetry();
    TestBattleTransferResumeAndReservationRetry();
    TestMultipleBattleCatchDestinations();
    TestBattleAndPcArbitration();
#endif
#if CTR_COOP_TEST_MAX_SPRINT >= 6
    TestDoubleBattleState();
    TestDoubleResultHeader();
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
void CB2_Overworld(void) {}
void CB2_InitStarterSelection(void) {}
bool8 CB2_StarterSelectionActive(void) { return FALSE; }
void SaveObjectEvents(void) {}
void LoadObjEventTemplatesFromHeader(void) {}
void RemoveObjectEventByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup) { (void)localId; (void)mapNum; (void)mapGroup; }
void ObjectEventSetGraphicsId(struct ObjectEvent *object, u8 graphics) { object->graphicsId = graphics; }
u8 TrySpawnObjectEvent(u8 localId, u8 mapNum, u8 mapGroup) { (void)localId; (void)mapNum; (void)mapGroup; return OBJECT_EVENTS_COUNT; }
void ScriptContext_Enable(void) {}
void CreateMon(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, bool8 hasFixedPersonality, u32 personality, u8 otIdType, u32 fixedOtId)
{
    (void)level; (void)fixedIV; (void)hasFixedPersonality; (void)personality; (void)otIdType; (void)fixedOtId;
    SetMon(mon, species);
}
uint16_t Random(void) { return 0x1234; }
void CB2_InitBattle(void) {}
void CB2_InitEndLinkBattle(void) {}
void SetMainCallback2(MainCallback callback) { sMainCallback = callback; }
bool AddBagItem(uint16_t itemId, uint16_t quantity)
{
    struct ItemSlot *slot = &sSaveBlock1.bagPocket_Items[0];
    if (sFailBagChange) return false;
    uint16_t count = slot->itemId == itemId ? slot->quantity ^ (uint16_t)sSaveBlock2.encryptionKey : 0;
    slot->itemId = itemId;
    slot->quantity = (count + quantity) ^ (uint16_t)sSaveBlock2.encryptionKey;
    return true;
}
bool RemoveBagItem(uint16_t itemId, uint16_t quantity) { (void)itemId; (void)quantity; return true; }
uint32_t GetMoney(uint32_t *money) { return *money ^ sSaveBlock2.encryptionKey; }
void AddMoney(uint32_t *money, uint32_t amount) { *money = (GetMoney(money) + amount) ^ sSaveBlock2.encryptionKey; }
void RemoveMoney(uint32_t *money, uint32_t amount) { uint32_t value = GetMoney(money); *money = (value > amount ? value - amount : 0) ^ sSaveBlock2.encryptionKey; }
bool FlagGet(uint16_t id) { return id != 0 && id < SPECIAL_FLAGS_START && (sSaveBlock1.flags[id / 8] & (1 << (id & 7))) != 0; }
