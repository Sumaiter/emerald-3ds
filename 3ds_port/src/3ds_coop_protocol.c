#include <stddef.h>
#include <string.h>
#include "3ds_coop_protocol.h"

static uint8_t ActionDirection(uint8_t action)
{
    if (action <= CTR_COOP_ACTION_FACE_RIGHT)
        return action + 1;
    if (action >= CTR_COOP_ACTION_WALK_NORMAL_DOWN && action <= CTR_COOP_ACTION_WALK_NORMAL_DOWN + 3)
        return action - CTR_COOP_ACTION_WALK_NORMAL_DOWN + 1;
    if (action >= CTR_COOP_ACTION_PLAYER_RUN_DOWN && action <= CTR_COOP_ACTION_PLAYER_RUN_DOWN + 3)
        return action - CTR_COOP_ACTION_PLAYER_RUN_DOWN + 1;
    return 0;
}

static bool IsMoving(uint8_t action)
{
    return action >= CTR_COOP_ACTION_WALK_NORMAL_DOWN;
}

static bool EncodeSnapshot(uint8_t type, const CtrCoopSnapshot *snapshot, uint8_t *out, uint16_t outSize)
{
    if (snapshot == NULL || out == NULL || outSize != CTR_COOP_PACKET_SIZE
     || ActionDirection(snapshot->movementActionId) != snapshot->facing || snapshot->graphicsId > CTR_COOP_GRAPHICS_ID_MAX)
        return false;
    out[0] = type;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    out[2] = snapshot->mapGroup;
    out[3] = snapshot->mapNum;
    out[4] = snapshot->mapLayoutId & 0xFF;
    out[5] = snapshot->mapLayoutId >> 8;
    out[6] = (uint16_t)snapshot->x & 0xFF;
    out[7] = (uint16_t)snapshot->x >> 8;
    out[8] = (uint16_t)snapshot->y & 0xFF;
    out[9] = (uint16_t)snapshot->y >> 8;
    out[10] = snapshot->facing;
    out[11] = snapshot->movementActionId;
    out[12] = snapshot->graphicsId;
    return true;
}

static bool DecodeSnapshot(uint8_t expectedType, const uint8_t *data, uint16_t size, CtrCoopSnapshot *out)
{
    CtrCoopSnapshot snapshot = {0};

    if (data == NULL || out == NULL || size != CTR_COOP_PACKET_SIZE
     || data[0] != expectedType || data[1] != CTR_COOP_PROTOCOL_VERSION)
        return false;
    snapshot.mapGroup = data[2];
    snapshot.mapNum = data[3];
    snapshot.mapLayoutId = data[4] | ((uint16_t)data[5] << 8);
    snapshot.x = (int16_t)(data[6] | ((uint16_t)data[7] << 8));
    snapshot.y = (int16_t)(data[8] | ((uint16_t)data[9] << 8));
    snapshot.facing = data[10];
    snapshot.movementActionId = data[11];
    snapshot.graphicsId = data[12];
    if (ActionDirection(snapshot.movementActionId) != snapshot.facing || snapshot.graphicsId > CTR_COOP_GRAPHICS_ID_MAX)
        return false;
    *out = snapshot;
    return true;
}

bool CtrCoopProtocol_Encode(const CtrCoopSnapshot *snapshot, uint8_t *out, uint16_t outSize)
{
    return EncodeSnapshot(CTR_COOP_PEER_PACKET, snapshot, out, outSize);
}

bool CtrCoopProtocol_Decode(const uint8_t *data, uint16_t size, CtrCoopSnapshot *out)
{
    return DecodeSnapshot(CTR_COOP_PEER_PACKET, data, size, out);
}

bool CtrCoopProtocol_EncodeSpawn(const CtrCoopSnapshot *snapshot, uint8_t *out, uint16_t outSize)
{
    return EncodeSnapshot(CTR_COOP_SPAWN_PACKET, snapshot, out, outSize);
}

bool CtrCoopProtocol_DecodeSpawn(const uint8_t *data, uint16_t size, CtrCoopSnapshot *out)
{
    return DecodeSnapshot(CTR_COOP_SPAWN_PACKET, data, size, out);
}

bool CtrCoopProtocol_EncodeIdentity(const uint8_t *name, uint8_t *out, uint16_t outSize)
{
    if (name == NULL || out == NULL || outSize != CTR_COOP_IDENTITY_SIZE)
        return false;
    out[0] = CTR_COOP_IDENTITY_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    memcpy(out + 2, name, CTR_COOP_PLAYER_NAME_SIZE);
    return true;
}

bool CtrCoopProtocol_DecodeIdentity(const uint8_t *data, uint16_t size, uint8_t *name)
{
    if (data == NULL || name == NULL || size != CTR_COOP_IDENTITY_SIZE
     || data[0] != CTR_COOP_IDENTITY_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION)
        return false;
    memcpy(name, data + 2, CTR_COOP_PLAYER_NAME_SIZE);
    return true;
}

bool CtrCoopProtocol_EncodeAuth(const uint8_t *password, uint8_t *out, uint16_t outSize)
{
    if (password == NULL || out == NULL || outSize != CTR_COOP_AUTH_PACKET_SIZE)
        return false;
    out[0] = CTR_COOP_AUTH_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    memcpy(out + 2, password, 4);
    return true;
}

bool CtrCoopProtocol_DecodeAuth(const uint8_t *data, uint16_t size, uint8_t *password)
{
    if (data == NULL || password == NULL || size != CTR_COOP_AUTH_PACKET_SIZE
     || data[0] != CTR_COOP_AUTH_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION)
        return false;
    memcpy(password, data + 2, 4);
    return true;
}

bool CtrCoopProtocol_EncodeAuthResult(bool accepted, uint8_t *out, uint16_t outSize)
{
    if (out == NULL || outSize != CTR_COOP_AUTH_RESULT_SIZE)
        return false;
    out[0] = CTR_COOP_AUTH_RESULT_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    out[2] = accepted ? 1 : 0;
    return true;
}

bool CtrCoopProtocol_DecodeAuthResult(const uint8_t *data, uint16_t size, bool *accepted)
{
    if (data == NULL || accepted == NULL || size != CTR_COOP_AUTH_RESULT_SIZE
     || data[0] != CTR_COOP_AUTH_RESULT_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION || data[2] > 1)
        return false;
    *accepted = data[2] != 0;
    return true;
}

bool CtrCoopProtocol_SameMap(const CtrCoopSnapshot *a, const CtrCoopSnapshot *b)
{
    return a != NULL && b != NULL && a->mapGroup == b->mapGroup
        && a->mapNum == b->mapNum && a->mapLayoutId == b->mapLayoutId;
}

bool CtrCoopProtocol_ShouldSend(const CtrCoopSnapshot *last, const CtrCoopSnapshot *current, uint16_t framesSinceLast)
{
    if (current == NULL)
        return false;
    if (last == NULL)
        return true;
    if (last->mapGroup != current->mapGroup || last->mapNum != current->mapNum
     || last->mapLayoutId != current->mapLayoutId || last->x != current->x || last->y != current->y
     || last->facing != current->facing || last->movementActionId != current->movementActionId
     || last->graphicsId != current->graphicsId)
        return true;
    return framesSinceLast >= CTR_COOP_HEARTBEAT_FRAMES;
}

CtrCoopApply CtrCoopProtocol_PlanMovement(const CtrCoopSnapshot *snapshot,
    int16_t currentX, int16_t currentY, bool freshSpawn, bool alreadyApplied, bool movementInProgress)
{
    int dx, dy;
    uint8_t direction;

    if (snapshot == NULL || freshSpawn || alreadyApplied || movementInProgress)
        return CTR_COOP_APPLY_NOTHING;
    dx = snapshot->x - currentX;
    dy = snapshot->y - currentY;
    if (dx == 0 && dy == 0)
        return CTR_COOP_APPLY_FACE;
    if (!IsMoving(snapshot->movementActionId))
        return CTR_COOP_APPLY_SNAP;
    direction = snapshot->facing;
    if ((direction == 1 && dx == 0 && dy == 1)
     || (direction == 2 && dx == 0 && dy == -1)
     || (direction == 3 && dx == -1 && dy == 0)
     || (direction == 4 && dx == 1 && dy == 0))
        return CTR_COOP_APPLY_START;
    return CTR_COOP_APPLY_SNAP;
}
