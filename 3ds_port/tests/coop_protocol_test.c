#include <stdio.h>
#include <string.h>
#include "3ds_coop_protocol.h"

static int sFailures;

#define CHECK(cond, ...) do { if (!(cond)) { sFailures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static CtrCoopSnapshot Base(void)
{
    CtrCoopSnapshot snapshot = {2, 7, 0x1234, -7, 9, 1, CTR_COOP_ACTION_FACE_DOWN, 100};
    return snapshot;
}

static void TestRoundtrip(void)
{
    CtrCoopSnapshot original = Base(), decoded;
    uint8_t packet[CTR_COOP_PACKET_SIZE];

    CHECK(CtrCoopProtocol_Encode(&original, packet, sizeof(packet)), "valid packet encodes");
    CHECK(CtrCoopProtocol_Decode(packet, sizeof(packet), &decoded), "valid packet decodes");
    CHECK(!CtrCoopProtocol_ShouldSend(&original, &decoded, 0), "roundtrip preserves every field");
    CHECK(packet[0] == CTR_COOP_PEER_PACKET && packet[1] == CTR_COOP_PROTOCOL_VERSION, "type and version on wire");
    CHECK(packet[6] == 0xF9 && packet[7] == 0xFF, "negative x is little endian");
    CHECK(!CtrCoopProtocol_Encode(&original, packet, sizeof(packet) - 1), "short output rejected");
    CHECK(!CtrCoopProtocol_Encode(NULL, packet, sizeof(packet)), "null encode rejected");
}

static void TestInvalidPackets(void)
{
    CtrCoopSnapshot original = Base(), decoded;
    uint8_t packet[CTR_COOP_PACKET_SIZE];

    CtrCoopProtocol_Encode(&original, packet, sizeof(packet));
    CHECK(!CtrCoopProtocol_Decode(packet, sizeof(packet) - 1, &decoded), "short packet rejected");
    packet[0]--;
    CHECK(!CtrCoopProtocol_Decode(packet, sizeof(packet), &decoded), "other peer traffic rejected");
    packet[0]++;
    packet[1]++;
    CHECK(!CtrCoopProtocol_Decode(packet, sizeof(packet), &decoded), "unknown version rejected");
    packet[1]--;
    packet[10] = 5;
    CHECK(!CtrCoopProtocol_Decode(packet, sizeof(packet), &decoded), "invalid facing rejected");
    packet[10] = 1;
    packet[11] = 4;
    CHECK(!CtrCoopProtocol_Decode(packet, sizeof(packet), &decoded), "unsupported action rejected");
    packet[11] = CTR_COOP_ACTION_WALK_NORMAL_DOWN + 1;
    CHECK(!CtrCoopProtocol_Decode(packet, sizeof(packet), &decoded), "action/facing mismatch rejected");
    packet[11] = CTR_COOP_ACTION_FACE_DOWN;
    packet[12] = CTR_COOP_GRAPHICS_ID_MAX + 1;
    CHECK(!CtrCoopProtocol_Decode(packet, sizeof(packet), &decoded), "invalid graphicsId rejected");
    CHECK(!CtrCoopProtocol_Decode(packet, sizeof(packet), NULL), "null decode rejected");
}

static void TestMapIdentity(void)
{
    CtrCoopSnapshot a = Base(), b = Base();

    CHECK(CtrCoopProtocol_SameMap(&a, &b), "same map and layout");
    b.mapLayoutId++;
    CHECK(!CtrCoopProtocol_SameMap(&a, &b), "different layout rejected");
    b = a;
    b.mapNum++;
    CHECK(!CtrCoopProtocol_SameMap(&a, &b), "different map rejected");
    b = a;
    b.mapGroup++;
    CHECK(!CtrCoopProtocol_SameMap(&a, &b), "different group rejected");
}

static void TestSendSchedule(void)
{
    CtrCoopSnapshot last = Base(), current = Base();

    CHECK(CtrCoopProtocol_ShouldSend(NULL, &current, 0), "first snapshot sends immediately");
    CHECK(!CtrCoopProtocol_ShouldSend(&last, &current, 59), "unchanged snapshot waits");
    CHECK(CtrCoopProtocol_ShouldSend(&last, &current, 60), "heartbeat sends unchanged snapshot");
    current.x++;
    CHECK(CtrCoopProtocol_ShouldSend(&last, &current, 0), "position change sends immediately");
    current = last;
    current.facing = 2;
    current.movementActionId = CTR_COOP_ACTION_FACE_UP;
    CHECK(CtrCoopProtocol_ShouldSend(&last, &current, 0), "facing change sends immediately");
    CHECK(!CtrCoopProtocol_ShouldSend(&last, NULL, 60), "missing current snapshot not sent");
}

static void TestMovementPlan(void)
{
    CtrCoopSnapshot snapshot = Base();

    snapshot.y = 10;
    snapshot.movementActionId = CTR_COOP_ACTION_WALK_NORMAL_DOWN;
    CHECK(CtrCoopProtocol_PlanMovement(&snapshot, -7, 9, true, false, false) == CTR_COOP_APPLY_NOTHING,
          "fresh spawn does not walk an extra tile");
    CHECK(CtrCoopProtocol_PlanMovement(&snapshot, -7, 9, false, true, false) == CTR_COOP_APPLY_NOTHING,
          "applied heartbeat does not restart movement");
    CHECK(CtrCoopProtocol_PlanMovement(&snapshot, -7, 9, false, false, true) == CTR_COOP_APPLY_NOTHING,
          "movement already in progress is not interrupted");
    CHECK(CtrCoopProtocol_PlanMovement(&snapshot, -7, 9, false, false, false) == CTR_COOP_APPLY_START,
          "one tile walk starts");

    snapshot.x = -6;
    snapshot.y = 9;
    snapshot.facing = 4;
    snapshot.movementActionId = CTR_COOP_ACTION_PLAYER_RUN_DOWN + 3;
    CHECK(CtrCoopProtocol_PlanMovement(&snapshot, -7, 9, false, false, false) == CTR_COOP_APPLY_START,
          "one tile run starts");

    snapshot = Base();
    snapshot.facing = 2;
    snapshot.movementActionId = CTR_COOP_ACTION_FACE_UP;
    CHECK(CtrCoopProtocol_PlanMovement(&snapshot, -7, 9, false, false, false) == CTR_COOP_APPLY_FACE,
          "stationary facing update");
    snapshot.x = -5;
    CHECK(CtrCoopProtocol_PlanMovement(&snapshot, -7, 9, false, false, false) == CTR_COOP_APPLY_SNAP,
          "jump snaps without a movement action");
    snapshot.x = -6;
    snapshot.movementActionId = CTR_COOP_ACTION_WALK_NORMAL_DOWN;
    CHECK(CtrCoopProtocol_PlanMovement(&snapshot, -7, 9, false, false, false) == CTR_COOP_APPLY_SNAP,
          "mismatched step snaps");
    CHECK(CtrCoopProtocol_PlanMovement(NULL, -7, 9, false, false, false) == CTR_COOP_APPLY_NOTHING,
          "missing snapshot does nothing");
}

static void TestAuthentication(void)
{
    static const uint8_t password[] = {4, 5, 6, 7};
    uint8_t decoded[4];
    uint8_t packet[CTR_COOP_AUTH_PACKET_SIZE];
    uint8_t result[CTR_COOP_AUTH_RESULT_SIZE];
    bool accepted;

    CHECK(CtrCoopProtocol_EncodeAuth(password, packet, sizeof(packet)), "auth packet encodes");
    CHECK(CtrCoopProtocol_DecodeAuth(packet, sizeof(packet), decoded), "auth packet decodes");
    CHECK(memcmp(password, decoded, sizeof(password)) == 0, "auth password roundtrip");
    CHECK(!CtrCoopProtocol_DecodeAuth(packet, sizeof(packet) - 1, decoded), "short auth rejected");
    packet[0]++;
    CHECK(!CtrCoopProtocol_DecodeAuth(packet, sizeof(packet), decoded), "wrong auth type rejected");
    CHECK(CtrCoopProtocol_EncodeAuthResult(true, result, sizeof(result)), "accepted result encodes");
    CHECK(CtrCoopProtocol_DecodeAuthResult(result, sizeof(result), &accepted) && accepted, "accepted result decodes");
    CHECK(CtrCoopProtocol_EncodeAuthResult(false, result, sizeof(result)), "rejected result encodes");
    CHECK(CtrCoopProtocol_DecodeAuthResult(result, sizeof(result), &accepted) && !accepted, "rejected result decodes");
    result[2] = 2;
    CHECK(!CtrCoopProtocol_DecodeAuthResult(result, sizeof(result), &accepted), "invalid auth result rejected");
}

static void TestSpawnIdentity(void)
{
    static const uint8_t name[] = {'G', 'A', 'S', 'T', 0xFF, 0xFF, 0xFF, 0xFF};
    CtrCoopSnapshot original = Base(), decoded;
    uint8_t decodedName[CTR_COOP_PLAYER_NAME_SIZE];
    uint8_t identity[CTR_COOP_IDENTITY_SIZE];
    uint8_t spawn[CTR_COOP_SPAWN_SIZE];

    CHECK(CtrCoopProtocol_EncodeIdentity(name, identity, sizeof(identity)), "identity encodes");
    CHECK(CtrCoopProtocol_DecodeIdentity(identity, sizeof(identity), decodedName), "identity decodes");
    CHECK(memcmp(name, decodedName, sizeof(name)) == 0, "player name roundtrip");
    CHECK(!CtrCoopProtocol_DecodeIdentity(identity, sizeof(identity) - 1, decodedName), "short identity rejected");
    identity[0]++;
    CHECK(!CtrCoopProtocol_DecodeIdentity(identity, sizeof(identity), decodedName), "wrong identity type rejected");
    CHECK(CtrCoopProtocol_EncodeSpawn(&original, spawn, sizeof(spawn)), "spawn encodes");
    CHECK(CtrCoopProtocol_DecodeSpawn(spawn, sizeof(spawn), &decoded), "spawn decodes");
    CHECK(!CtrCoopProtocol_ShouldSend(&original, &decoded, 0), "spawn preserves every field");
    CHECK(spawn[0] == CTR_COOP_SPAWN_PACKET, "spawn type on wire");
    CHECK(!CtrCoopProtocol_Decode(spawn, sizeof(spawn), &decoded), "spawn is not accepted as movement packet");
    spawn[12] = CTR_COOP_GRAPHICS_ID_MAX + 1;
    CHECK(!CtrCoopProtocol_DecodeSpawn(spawn, sizeof(spawn), &decoded), "invalid spawn graphics rejected");
}

static void TestSaveTransfer(void)
{
    uint8_t payload[CTR_COOP_SAVE_CHUNK_DATA_MAX];
    uint8_t decoded[CTR_COOP_SAVE_CHUNK_DATA_MAX];
    uint8_t begin[CTR_COOP_SAVE_BEGIN_SIZE];
    uint8_t request[CTR_COOP_SAVE_REQUEST_SIZE];
    uint8_t chunk[CTR_COOP_SAVE_CHUNK_MAX];
    uint32_t totalSize = 257, checksum, offset;
    uint16_t decodedSize;

    for (uint16_t i = 0; i < sizeof(payload); i++)
        payload[i] = i;
    checksum = CtrCoopProtocol_Checksum(payload, sizeof(payload));
    CHECK(checksum == CtrCoopProtocol_Checksum(payload, sizeof(payload)), "checksum is deterministic");
    payload[0]++;
    CHECK(checksum != CtrCoopProtocol_Checksum(payload, sizeof(payload)), "checksum detects changes");
    payload[0]--;

    CHECK(CtrCoopProtocol_EncodeSaveBegin(totalSize, checksum, begin, sizeof(begin)), "save begin encodes");
    CHECK(CtrCoopProtocol_DecodeSaveBegin(begin, sizeof(begin), &totalSize, &checksum), "save begin decodes");
    CHECK(totalSize == 257 && checksum == CtrCoopProtocol_Checksum(payload, sizeof(payload)), "save begin roundtrip");
    begin[1]++;
    CHECK(!CtrCoopProtocol_DecodeSaveBegin(begin, sizeof(begin), &totalSize, &checksum), "save begin version rejected");
    begin[1]--;
    CHECK(CtrCoopProtocol_EncodeSaveRequest(128, request, sizeof(request)), "save request encodes");
    CHECK(CtrCoopProtocol_DecodeSaveRequest(request, sizeof(request), &offset) && offset == 128, "request offset roundtrip");

    CHECK(CtrCoopProtocol_EncodeSaveChunk(0, payload, sizeof(payload), chunk, CTR_COOP_SAVE_CHUNK_MAX), "full chunk encodes");
    CHECK(CtrCoopProtocol_DecodeSaveChunk(chunk, CTR_COOP_SAVE_CHUNK_MAX, 0, decoded, sizeof(decoded), &decodedSize), "full chunk decodes");
    CHECK(decodedSize == sizeof(payload) && memcmp(payload, decoded, sizeof(payload)) == 0, "full chunk payload roundtrip");
    CHECK(!CtrCoopProtocol_DecodeSaveChunk(chunk, CTR_COOP_SAVE_CHUNK_MAX, 128, decoded, sizeof(decoded), &decodedSize), "wrong offset rejected");
    CHECK(!CtrCoopProtocol_DecodeSaveChunk(chunk, CTR_COOP_SAVE_CHUNK_MAX, 0, decoded, 1, &decodedSize), "short output rejected");
    chunk[7]++;
    CHECK(!CtrCoopProtocol_DecodeSaveChunk(chunk, CTR_COOP_SAVE_CHUNK_MAX, 0, decoded, sizeof(decoded), &decodedSize), "length mismatch rejected");
    CHECK(CtrCoopProtocol_EncodeSaveChunk(256, payload, 1, chunk, CTR_COOP_SAVE_CHUNK_HEADER + 1), "last short chunk encodes");
    CHECK(CtrCoopProtocol_DecodeSaveChunk(chunk, CTR_COOP_SAVE_CHUNK_HEADER + 1, 256, decoded, sizeof(decoded), &decodedSize) && decodedSize == 1, "last short chunk decodes");
    CHECK(!CtrCoopProtocol_EncodeSaveChunk(0, payload, 0, chunk, sizeof(chunk)), "empty chunk rejected");
}

static void TestPcSyncTransfer(void)
{
    uint8_t payload[CTR_COOP_SAVE_CHUNK_DATA_MAX];
    uint8_t decoded[CTR_COOP_SAVE_CHUNK_DATA_MAX];
    uint8_t begin[CTR_COOP_SAVE_BEGIN_SIZE];
    uint8_t request[CTR_COOP_SAVE_REQUEST_SIZE];
    uint8_t chunk[CTR_COOP_SAVE_CHUNK_MAX];
    uint32_t totalSize, checksum, offset;
    uint16_t decodedSize;

    for (uint16_t i = 0; i < sizeof(payload); i++)
        payload[i] = i ^ 0x5A;
    checksum = CtrCoopProtocol_Checksum(payload, sizeof(payload));
    CHECK(CtrCoopProtocol_EncodePcSyncBegin(34392, checksum, begin, sizeof(begin)), "PC sync begin encodes");
    CHECK(CtrCoopProtocol_DecodePcSyncBegin(begin, sizeof(begin), &totalSize, &checksum), "PC sync begin decodes");
    CHECK(totalSize == 34392, "PC sync size roundtrip");
    begin[0] = CTR_COOP_SAVE_BEGIN_PACKET;
    CHECK(!CtrCoopProtocol_DecodePcSyncBegin(begin, sizeof(begin), &totalSize, &checksum), "save begin is not PC sync");
    CHECK(CtrCoopProtocol_EncodePcSyncRequest(128, request, sizeof(request)), "PC sync request encodes");
    CHECK(CtrCoopProtocol_DecodePcSyncRequest(request, sizeof(request), &offset) && offset == 128, "PC sync request roundtrip");
    CHECK(!CtrCoopProtocol_DecodeSaveRequest(request, sizeof(request), &offset), "PC sync request is not save request");
    CHECK(CtrCoopProtocol_EncodePcSyncChunk(128, payload, sizeof(payload), chunk, sizeof(chunk)), "PC sync chunk encodes");
    CHECK(CtrCoopProtocol_DecodePcSyncChunk(chunk, sizeof(chunk), 128, decoded, sizeof(decoded), &decodedSize), "PC sync chunk decodes");
    CHECK(decodedSize == sizeof(payload) && memcmp(payload, decoded, sizeof(payload)) == 0, "PC sync payload roundtrip");
    CHECK(!CtrCoopProtocol_DecodeSaveChunk(chunk, sizeof(chunk), 128, decoded, sizeof(decoded), &decodedSize), "PC sync chunk is not save chunk");
}

static void TestPCControl(void)
{
    uint8_t packet[CTR_COOP_PC_RESULT_SIZE];
    bool accepted;

    CHECK(CtrCoopProtocol_EncodePcResult(true, packet, sizeof(packet)), "PC grant encodes");
    CHECK(CtrCoopProtocol_DecodePcResult(packet, sizeof(packet), &accepted) && accepted, "PC grant decodes");
    CHECK(CtrCoopProtocol_EncodePcResult(false, packet, sizeof(packet)), "PC rejection encodes");
    CHECK(CtrCoopProtocol_DecodePcResult(packet, sizeof(packet), &accepted) && !accepted, "PC rejection decodes");
    packet[2] = 2;
    CHECK(!CtrCoopProtocol_DecodePcResult(packet, sizeof(packet), &accepted), "invalid PC result rejected");
    packet[2] = 1;
    packet[0] = CTR_COOP_PC_REQUEST_PACKET;
    CHECK(!CtrCoopProtocol_DecodePcResult(packet, sizeof(packet), &accepted), "PC request is not a result");
    packet[0] = CTR_COOP_PC_RESULT_PACKET;
    CHECK(!CtrCoopProtocol_DecodePcResult(packet, sizeof(packet) - 1, &accepted), "short PC result rejected");
    CHECK(!CtrCoopProtocol_EncodePcResult(true, packet, sizeof(packet) - 1), "short PC output rejected");
}

static void TestBattleProtocol(void)
{
    uint8_t reserve[CTR_COOP_BATTLE_RESERVE_SIZE];
    uint8_t result[CTR_COOP_BATTLE_RESERVE_RESULT_SIZE];
    uint8_t begin[CTR_COOP_SAVE_BEGIN_SIZE];
    uint8_t request[CTR_COOP_SAVE_REQUEST_SIZE];
    uint8_t chunk[CTR_COOP_SAVE_CHUNK_HEADER + 3];
    uint8_t payload[] = {1, 2, 3};
    uint8_t decodedPayload[3];
    uint16_t trainerId, decodedSize;
    uint32_t totalSize, checksum, offset;
    bool accepted;

    CHECK(CtrCoopProtocol_EncodeBattleReserve(321, reserve, sizeof(reserve)), "battle reservation encodes");
    CHECK(CtrCoopProtocol_DecodeBattleReserve(reserve, sizeof(reserve), &trainerId) && trainerId == 321, "battle reservation roundtrip");
    CHECK(!CtrCoopProtocol_EncodeBattleReserve(0, reserve, sizeof(reserve)), "zero trainer rejected");
    CHECK(CtrCoopProtocol_EncodeBattleReserveResult(321, true, result, sizeof(result)), "battle grant encodes");
    CHECK(CtrCoopProtocol_DecodeBattleReserveResult(result, sizeof(result), &trainerId, &accepted) && trainerId == 321 && accepted, "battle grant roundtrip");
    result[3] &= 0x7F;
    CHECK(CtrCoopProtocol_DecodeBattleReserveResult(result, sizeof(result), &trainerId, &accepted) && !accepted, "battle rejection decodes");
    CHECK(CtrCoopProtocol_EncodeBattleBegin(616, 0x12345678, begin, sizeof(begin)), "battle begin encodes");
    CHECK(CtrCoopProtocol_DecodeBattleBegin(begin, sizeof(begin), &totalSize, &checksum) && totalSize == 616 && checksum == 0x12345678, "battle begin roundtrip");
    CHECK(!CtrCoopProtocol_DecodePcSyncBegin(begin, sizeof(begin), &totalSize, &checksum), "battle begin is not PC sync");
    CHECK(CtrCoopProtocol_EncodeBattleRequest(128, request, sizeof(request)), "battle request encodes");
    CHECK(CtrCoopProtocol_DecodeBattleRequest(request, sizeof(request), &offset) && offset == 128, "battle request roundtrip");
    CHECK(CtrCoopProtocol_EncodeBattleChunk(0, payload, sizeof(payload), chunk, sizeof(chunk)), "battle chunk encodes");
    CHECK(CtrCoopProtocol_DecodeBattleChunk(chunk, sizeof(chunk), 0, decodedPayload, sizeof(decodedPayload), &decodedSize), "battle chunk decodes");
    CHECK(decodedSize == sizeof(payload) && memcmp(payload, decodedPayload, sizeof(payload)) == 0, "battle payload roundtrip");
    CHECK(CtrCoopProtocol_EncodeDoubleBegin(1232, 0x89ABCDEF, begin, sizeof(begin)), "double begin encodes");
    CHECK(CtrCoopProtocol_DecodeDoubleBegin(begin, sizeof(begin), &totalSize, &checksum) && totalSize == 1232 && checksum == 0x89ABCDEF, "double begin roundtrip");
    CHECK(!CtrCoopProtocol_DecodeBattleBegin(begin, sizeof(begin), &totalSize, &checksum), "double begin is not battle result");
    CHECK(CtrCoopProtocol_EncodeDoubleRequestData(128, request, sizeof(request)), "double request encodes");
    CHECK(CtrCoopProtocol_DecodeDoubleRequestData(request, sizeof(request), &offset) && offset == 128, "double request roundtrip");
    CHECK(!CtrCoopProtocol_DecodeBattleRequest(request, sizeof(request), &offset), "double request is not battle request");
    CHECK(CtrCoopProtocol_EncodeDoubleChunk(0, payload, sizeof(payload), chunk, sizeof(chunk)), "double chunk encodes");
    CHECK(CtrCoopProtocol_DecodeDoubleChunk(chunk, sizeof(chunk), 0, decodedPayload, sizeof(decodedPayload), &decodedSize), "double chunk decodes");
    CHECK(decodedSize == sizeof(payload) && memcmp(payload, decodedPayload, sizeof(payload)) == 0, "double payload roundtrip");
    CHECK(!CtrCoopProtocol_DecodeBattleChunk(chunk, sizeof(chunk), 0, decodedPayload, sizeof(decodedPayload), &decodedSize), "double chunk is not battle chunk");
}

static void TestStoryUpdates(void)
{
    uint8_t flag[CTR_COOP_FLAG_SIZE];
    uint8_t var[CTR_COOP_VAR_SIZE];
    uint16_t id, value;
    bool enabled;

    CHECK(CtrCoopProtocol_EncodeFlag(0x1234, true, flag, sizeof(flag)), "flag encodes");
    CHECK(CtrCoopProtocol_DecodeFlag(flag, sizeof(flag), &id, &enabled) && id == 0x1234 && enabled, "flag roundtrip");
    flag[4] = 0;
    CHECK(CtrCoopProtocol_DecodeFlag(flag, sizeof(flag), &id, &enabled) && !enabled, "flag clear decodes");
    flag[4] = 2;
    CHECK(!CtrCoopProtocol_DecodeFlag(flag, sizeof(flag), &id, &enabled), "invalid flag value rejected");
    flag[4] = 1;
    CHECK(!CtrCoopProtocol_DecodeFlag(flag, sizeof(flag) - 1, &id, &enabled), "short flag rejected");
    CHECK(CtrCoopProtocol_EncodeVar(0x4567, 0xAB89, var, sizeof(var)), "var encodes");
    CHECK(CtrCoopProtocol_DecodeVar(var, sizeof(var), &id, &value) && id == 0x4567 && value == 0xAB89, "var roundtrip");
    var[1]++;
    CHECK(!CtrCoopProtocol_DecodeVar(var, sizeof(var), &id, &value), "var version rejected");
}

int main(void)
{
    TestRoundtrip();
    TestInvalidPackets();
    TestMapIdentity();
    TestSendSchedule();
    TestMovementPlan();
    TestAuthentication();
    TestSpawnIdentity();
    TestSaveTransfer();
    TestPcSyncTransfer();
    TestPCControl();
    TestBattleProtocol();
    TestStoryUpdates();
    if (sFailures != 0)
    {
        printf("%d co-op protocol tests failed\n", sFailures);
        return 1;
    }
    puts("co-op protocol tests passed");
    return 0;
}
