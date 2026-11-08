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

int main(void)
{
    TestRoundtrip();
    TestInvalidPackets();
    TestMapIdentity();
    TestSendSchedule();
    TestMovementPlan();
    TestAuthentication();
    if (sFailures != 0)
    {
        printf("%d co-op protocol tests failed\n", sFailures);
        return 1;
    }
    puts("co-op protocol tests passed");
    return 0;
}
