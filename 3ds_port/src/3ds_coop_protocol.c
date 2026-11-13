#include <stddef.h>
#include <string.h>
#include "3ds_coop_protocol.h"

static uint8_t ActionDirection(uint8_t action, uint8_t bikeMode)
{
    if (action <= CTR_COOP_ACTION_FACE_RIGHT)
        return action + 1;
    if (action >= CTR_COOP_ACTION_WALK_NORMAL_DOWN && action <= CTR_COOP_ACTION_WALK_NORMAL_DOWN + 3)
        return action - CTR_COOP_ACTION_WALK_NORMAL_DOWN + 1;
    if (action >= CTR_COOP_ACTION_PLAYER_RUN_DOWN && action <= CTR_COOP_ACTION_PLAYER_RUN_DOWN + 3)
        return action - CTR_COOP_ACTION_PLAYER_RUN_DOWN + 1;
    if (bikeMode != CTR_COOP_BIKE_NONE)
    {
        static const uint8_t groups[] = {
            CTR_COOP_ACTION_JUMP_2_DOWN, CTR_COOP_ACTION_WALK_FAST_DOWN,
            CTR_COOP_ACTION_RIDE_WATER_CURRENT_DOWN, CTR_COOP_ACTION_WALK_FASTER_DOWN,
            CTR_COOP_ACTION_JUMP_DOWN,
        };
        for (unsigned i = 0; i < sizeof(groups); ++i)
            if (action >= groups[i] && action <= groups[i] + 3)
                return action - groups[i] + 1;
        if (action >= CTR_COOP_ACTION_WALK_IN_PLACE_SLOW_DOWN && action < CTR_COOP_ACTION_RIDE_WATER_CURRENT_DOWN)
            return (action - CTR_COOP_ACTION_WALK_IN_PLACE_SLOW_DOWN) % 4 + 1;
        if (action >= CTR_COOP_ACTION_JUMP_IN_PLACE_DOWN && action < CTR_COOP_ACTION_JUMP_IN_PLACE_DOWN + 4)
            return (action - CTR_COOP_ACTION_JUMP_IN_PLACE_DOWN) % 4 + 1;
        if (bikeMode == CTR_COOP_BIKE_ACRO && action >= CTR_COOP_ACTION_ACRO_FIRST && action <= CTR_COOP_ACTION_ACRO_LAST)
            return (action - CTR_COOP_ACTION_ACRO_FIRST) % 4 + 1;
    }
    return 0;
}

static bool IsInPlace(uint8_t action)
{
    return action <= CTR_COOP_ACTION_FACE_RIGHT
        || (action >= 0x19 && action <= 0x28)  /* walk/turn in place */
        || (action >= 0x46 && action <= 0x4D)  /* jump/turn in place */
        || (action >= 0x64 && action <= 0x73)  /* standing wheelie/pop/end/hop */
        || (action >= 0x7C && action <= 0x7F); /* wheelie against a wall */
}

static bool FacingMatches(uint8_t action, uint8_t bikeMode, uint8_t facing)
{
    if (bikeMode == CTR_COOP_BIKE_ACRO && action >= 0x4A && action <= 0x4D)
    {
        /* A turn-jump changes its facing halfway through the action. Both
         * ends of that axis are valid; it never moves to another tile. */
        return action < 0x4C ? facing <= 2 : facing >= 3;
    }
    return ActionDirection(action, bikeMode) == facing;
}

static bool IsMoving(uint8_t action)
{
    return !IsInPlace(action);
}

static bool IsTwoTileJump(uint8_t action)
{
    return (action >= 0x0C && action <= 0x0F) || (action >= 0x78 && action <= 0x7B);
}

static bool EncodeSnapshot(uint8_t type, const CtrCoopSnapshot *snapshot, uint8_t *out, uint16_t outSize)
{
    if (snapshot == NULL || out == NULL || outSize != CTR_COOP_PACKET_SIZE
     || snapshot->facing < 1 || snapshot->facing > 4
     || snapshot->bikeMode > CTR_COOP_BIKE_ACRO
     || !FacingMatches(snapshot->movementActionId, snapshot->bikeMode, snapshot->facing) || snapshot->graphicsId > CTR_COOP_GRAPHICS_ID_MAX)
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
    out[13] = snapshot->bikeMode;
    out[14] = snapshot->movementSequence;
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
    snapshot.bikeMode = data[13];
    snapshot.movementSequence = data[14];
    if (snapshot.facing < 1 || snapshot.facing > 4
     || snapshot.bikeMode > CTR_COOP_BIKE_ACRO
     || !FacingMatches(snapshot.movementActionId, snapshot.bikeMode, snapshot.facing) || snapshot.graphicsId > CTR_COOP_GRAPHICS_ID_MAX)
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

bool CtrCoopProtocol_EncodeNpc(const CtrCoopNpcSnapshot *snapshot, uint8_t *out, uint16_t outSize)
{
    if (snapshot == NULL || out == NULL || outSize != CTR_COOP_NPC_PACKET_SIZE
     || snapshot->localId == 0 || snapshot->facing < 1 || snapshot->facing > 4
     || snapshot->graphicsId > CTR_COOP_GRAPHICS_ID_MAX)
        return false;
    out[0] = CTR_COOP_NPC_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    out[2] = snapshot->localId;
    out[3] = snapshot->mapGroup;
    out[4] = snapshot->mapNum;
    out[5] = (uint16_t)snapshot->x & 0xFF;
    out[6] = (uint16_t)snapshot->x >> 8;
    out[7] = (uint16_t)snapshot->y & 0xFF;
    out[8] = (uint16_t)snapshot->y >> 8;
    out[9] = snapshot->facing;
    out[10] = snapshot->graphicsId;
    out[11] = 0;
    return true;
}

bool CtrCoopProtocol_DecodeNpc(const uint8_t *data, uint16_t size, CtrCoopNpcSnapshot *out)
{
    CtrCoopNpcSnapshot snapshot = {0};

    if (data == NULL || out == NULL || size != CTR_COOP_NPC_PACKET_SIZE
     || data[0] != CTR_COOP_NPC_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION
     || data[2] == 0 || data[9] < 1 || data[9] > 4
     || data[10] > CTR_COOP_GRAPHICS_ID_MAX || data[11] != 0)
        return false;
    snapshot.localId = data[2];
    snapshot.mapGroup = data[3];
    snapshot.mapNum = data[4];
    snapshot.x = (int16_t)(data[5] | ((uint16_t)data[6] << 8));
    snapshot.y = (int16_t)(data[7] | ((uint16_t)data[8] << 8));
    snapshot.facing = data[9];
    snapshot.graphicsId = data[10];
    *out = snapshot;
    return true;
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

bool CtrCoopProtocol_EncodeJoinInfo(bool starterRequired, uint8_t *out, uint16_t outSize)
{
    if (out == NULL || outSize != CTR_COOP_JOIN_INFO_SIZE)
        return false;
    out[0] = CTR_COOP_JOIN_INFO_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    out[2] = starterRequired ? CTR_COOP_JOIN_STARTER : 0;
    return true;
}

bool CtrCoopProtocol_DecodeJoinInfo(const uint8_t *data, uint16_t size, bool *starterRequired)
{
    if (data == NULL || starterRequired == NULL || size != CTR_COOP_JOIN_INFO_SIZE
     || data[0] != CTR_COOP_JOIN_INFO_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION
     || (data[2] & ~CTR_COOP_JOIN_STARTER) != 0)
        return false;
    *starterRequired = (data[2] & CTR_COOP_JOIN_STARTER) != 0;
    return true;
}

bool CtrCoopProtocol_EncodeNameLimit(uint8_t *out, uint16_t outSize)
{
    if (out == NULL || outSize != CTR_COOP_NAME_LIMIT_SIZE)
        return false;
    out[0] = CTR_COOP_NAME_LIMIT_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    return true;
}

bool CtrCoopProtocol_DecodeNameLimit(const uint8_t *data, uint16_t size)
{
    return data != NULL && size == CTR_COOP_NAME_LIMIT_SIZE
        && data[0] == CTR_COOP_NAME_LIMIT_PACKET && data[1] == CTR_COOP_PROTOCOL_VERSION;
}

bool CtrCoopProtocol_EncodePcResult(bool accepted, uint8_t *out, uint16_t outSize)
{
    if (out == NULL || outSize != CTR_COOP_PC_RESULT_SIZE)
        return false;
    out[0] = CTR_COOP_PC_RESULT_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    out[2] = accepted ? 1 : 0;
    return true;
}

bool CtrCoopProtocol_DecodePcResult(const uint8_t *data, uint16_t size, bool *accepted)
{
    if (data == NULL || accepted == NULL || size != CTR_COOP_PC_RESULT_SIZE
     || data[0] != CTR_COOP_PC_RESULT_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION || data[2] > 1)
        return false;
    *accepted = data[2] != 0;
    return true;
}

bool CtrCoopProtocol_EncodeFlag(uint16_t id, bool value, uint8_t *out, uint16_t outSize)
{
    if (out == NULL || outSize != CTR_COOP_FLAG_SIZE)
        return false;
    out[0] = CTR_COOP_FLAG_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    out[2] = id & 0xFF;
    out[3] = id >> 8;
    out[4] = value ? 1 : 0;
    return true;
}

bool CtrCoopProtocol_DecodeFlag(const uint8_t *data, uint16_t size, uint16_t *id, bool *value)
{
    if (data == NULL || id == NULL || value == NULL || size != CTR_COOP_FLAG_SIZE
     || data[0] != CTR_COOP_FLAG_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION || data[4] > 1)
        return false;
    *id = data[2] | ((uint16_t)data[3] << 8);
    *value = data[4] != 0;
    return true;
}

bool CtrCoopProtocol_EncodeVar(uint16_t id, uint16_t value, uint8_t *out, uint16_t outSize)
{
    if (out == NULL || outSize != CTR_COOP_VAR_SIZE)
        return false;
    out[0] = CTR_COOP_VAR_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    out[2] = id & 0xFF;
    out[3] = id >> 8;
    out[4] = value & 0xFF;
    out[5] = value >> 8;
    return true;
}

bool CtrCoopProtocol_DecodeVar(const uint8_t *data, uint16_t size, uint16_t *id, uint16_t *value)
{
    if (data == NULL || id == NULL || value == NULL || size != CTR_COOP_VAR_SIZE
     || data[0] != CTR_COOP_VAR_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION)
        return false;
    *id = data[2] | ((uint16_t)data[3] << 8);
    *value = data[4] | ((uint16_t)data[5] << 8);
    return true;
}

static void Put32(uint8_t *out, uint32_t value)
{
    out[0] = value & 0xFF;
    out[1] = (value >> 8) & 0xFF;
    out[2] = (value >> 16) & 0xFF;
    out[3] = value >> 24;
}

static uint32_t Get32(const uint8_t *data)
{
    return data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static bool EncodeTransferBegin(uint8_t type, uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize)
{
    if (out == NULL || totalSize == 0 || outSize != CTR_COOP_SAVE_BEGIN_SIZE)
        return false;
    out[0] = type;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    Put32(out + 2, totalSize);
    Put32(out + 6, checksum);
    return true;
}

static bool DecodeTransferBegin(uint8_t type, const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum)
{
    if (data == NULL || totalSize == NULL || checksum == NULL || size != CTR_COOP_SAVE_BEGIN_SIZE
     || data[0] != type || data[1] != CTR_COOP_PROTOCOL_VERSION || Get32(data + 2) == 0)
        return false;
    *totalSize = Get32(data + 2);
    *checksum = Get32(data + 6);
    return true;
}

static bool EncodeTransferRequest(uint8_t type, uint32_t offset, uint8_t *out, uint16_t outSize)
{
    if (out == NULL || outSize != CTR_COOP_SAVE_REQUEST_SIZE)
        return false;
    out[0] = type;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    Put32(out + 2, offset);
    return true;
}

static bool DecodeTransferRequest(uint8_t type, const uint8_t *data, uint16_t size, uint32_t *offset)
{
    if (data == NULL || offset == NULL || size != CTR_COOP_SAVE_REQUEST_SIZE
     || data[0] != type || data[1] != CTR_COOP_PROTOCOL_VERSION)
        return false;
    *offset = Get32(data + 2);
    return true;
}

static bool EncodeTransferChunk(uint8_t type, uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize)
{
    if (data == NULL || out == NULL || dataSize == 0 || dataSize > CTR_COOP_SAVE_CHUNK_DATA_MAX
     || outSize != CTR_COOP_SAVE_CHUNK_HEADER + dataSize)
        return false;
    out[0] = type;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    Put32(out + 2, offset);
    out[6] = dataSize & 0xFF;
    out[7] = dataSize >> 8;
    memcpy(out + CTR_COOP_SAVE_CHUNK_HEADER, data, dataSize);
    return true;
}

static bool DecodeTransferChunk(uint8_t type, const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize)
{
    uint16_t payload;

    if (data == NULL || outSize == NULL || size < CTR_COOP_SAVE_CHUNK_HEADER + 1 || size > CTR_COOP_SAVE_CHUNK_MAX)
        return false;
    payload = size - CTR_COOP_SAVE_CHUNK_HEADER;
    if (out == NULL || outCapacity < payload || data[0] != type
     || data[1] != CTR_COOP_PROTOCOL_VERSION || Get32(data + 2) != expectedOffset
     || (data[6] | ((uint16_t)data[7] << 8)) != payload)
        return false;
    memcpy(out, data + CTR_COOP_SAVE_CHUNK_HEADER, payload);
    *outSize = payload;
    return true;
}

bool CtrCoopProtocol_EncodeSaveBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferBegin(CTR_COOP_SAVE_BEGIN_PACKET, totalSize, checksum, out, outSize);
}

bool CtrCoopProtocol_DecodeSaveBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum)
{
    return DecodeTransferBegin(CTR_COOP_SAVE_BEGIN_PACKET, data, size, totalSize, checksum);
}

bool CtrCoopProtocol_EncodeSaveRequest(uint32_t offset, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferRequest(CTR_COOP_SAVE_REQUEST_PACKET, offset, out, outSize);
}

bool CtrCoopProtocol_DecodeSaveRequest(const uint8_t *data, uint16_t size, uint32_t *offset)
{
    return DecodeTransferRequest(CTR_COOP_SAVE_REQUEST_PACKET, data, size, offset);
}

bool CtrCoopProtocol_EncodeSaveChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferChunk(CTR_COOP_SAVE_CHUNK_PACKET, offset, data, dataSize, out, outSize);
}

bool CtrCoopProtocol_DecodeSaveChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize)
{
    return DecodeTransferChunk(CTR_COOP_SAVE_CHUNK_PACKET, data, size, expectedOffset, out, outCapacity, outSize);
}

bool CtrCoopProtocol_EncodePcSyncBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferBegin(CTR_COOP_PC_SYNC_BEGIN_PACKET, totalSize, checksum, out, outSize);
}

bool CtrCoopProtocol_DecodePcSyncBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum)
{
    return DecodeTransferBegin(CTR_COOP_PC_SYNC_BEGIN_PACKET, data, size, totalSize, checksum);
}

bool CtrCoopProtocol_EncodePcSyncRequest(uint32_t offset, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferRequest(CTR_COOP_PC_SYNC_REQUEST_PACKET, offset, out, outSize);
}

bool CtrCoopProtocol_DecodePcSyncRequest(const uint8_t *data, uint16_t size, uint32_t *offset)
{
    return DecodeTransferRequest(CTR_COOP_PC_SYNC_REQUEST_PACKET, data, size, offset);
}

bool CtrCoopProtocol_EncodePcSyncChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferChunk(CTR_COOP_PC_SYNC_CHUNK_PACKET, offset, data, dataSize, out, outSize);
}

bool CtrCoopProtocol_DecodePcSyncChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize)
{
    return DecodeTransferChunk(CTR_COOP_PC_SYNC_CHUNK_PACKET, data, size, expectedOffset, out, outCapacity, outSize);
}

bool CtrCoopProtocol_EncodePartyBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferBegin(CTR_COOP_PARTY_BEGIN_PACKET, totalSize, checksum, out, outSize);
}

bool CtrCoopProtocol_DecodePartyBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum)
{
    return DecodeTransferBegin(CTR_COOP_PARTY_BEGIN_PACKET, data, size, totalSize, checksum);
}

bool CtrCoopProtocol_EncodePartyChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferChunk(CTR_COOP_PARTY_CHUNK_PACKET, offset, data, dataSize, out, outSize);
}

bool CtrCoopProtocol_DecodePartyChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize)
{
    return DecodeTransferChunk(CTR_COOP_PARTY_CHUNK_PACKET, data, size, expectedOffset, out, outCapacity, outSize);
}

bool CtrCoopProtocol_EncodePartyResult(bool accepted, uint8_t *out, uint16_t outSize)
{
    if (out == NULL || outSize != CTR_COOP_PARTY_RESULT_SIZE)
        return false;
    out[0] = CTR_COOP_PARTY_RESULT_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    out[2] = accepted ? 1 : 0;
    return true;
}

bool CtrCoopProtocol_DecodePartyResult(const uint8_t *data, uint16_t size, bool *accepted)
{
    if (data == NULL || accepted == NULL || size != CTR_COOP_PARTY_RESULT_SIZE
     || data[0] != CTR_COOP_PARTY_RESULT_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION || data[2] > 1)
        return false;
    *accepted = data[2] != 0;
    return true;
}

bool CtrCoopProtocol_EncodeBattleReserve(uint16_t trainerId, uint8_t *out, uint16_t outSize)
{
    if (out == NULL || trainerId > 0x7FFF || outSize != CTR_COOP_BATTLE_RESERVE_SIZE)
        return false;
    out[0] = CTR_COOP_BATTLE_RESERVE_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    out[2] = trainerId & 0xFF;
    out[3] = trainerId >> 8;
    return true;
}

bool CtrCoopProtocol_DecodeBattleReserve(const uint8_t *data, uint16_t size, uint16_t *trainerId)
{
    if (data == NULL || trainerId == NULL || size != CTR_COOP_BATTLE_RESERVE_SIZE
     || data[0] != CTR_COOP_BATTLE_RESERVE_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION
     || (data[3] & 0x80) != 0)
        return false;
    *trainerId = data[2] | ((uint16_t)data[3] << 8);
    return true;
}

bool CtrCoopProtocol_EncodeBattleReserveResult(uint16_t trainerId, bool accepted, uint8_t *out, uint16_t outSize)
{
    if (out == NULL || trainerId > 0x7FFF || outSize != CTR_COOP_BATTLE_RESERVE_RESULT_SIZE)
        return false;
    out[0] = CTR_COOP_BATTLE_RESERVE_RESULT_PACKET;
    out[1] = CTR_COOP_PROTOCOL_VERSION;
    out[2] = trainerId & 0xFF;
    out[3] = (trainerId >> 8) | (accepted ? 0x80 : 0);
    return true;
}

bool CtrCoopProtocol_DecodeBattleReserveResult(const uint8_t *data, uint16_t size, uint16_t *trainerId, bool *accepted)
{
    if (data == NULL || trainerId == NULL || accepted == NULL || size != CTR_COOP_BATTLE_RESERVE_RESULT_SIZE
     || data[0] != CTR_COOP_BATTLE_RESERVE_RESULT_PACKET || data[1] != CTR_COOP_PROTOCOL_VERSION)
        return false;
    *trainerId = data[2] | ((uint16_t)(data[3] & 0x7F) << 8);
    *accepted = (data[3] & 0x80) != 0;
    return true;
}

bool CtrCoopProtocol_EncodeBattleBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferBegin(CTR_COOP_BATTLE_BEGIN_PACKET, totalSize, checksum, out, outSize);
}

bool CtrCoopProtocol_DecodeBattleBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum)
{
    return DecodeTransferBegin(CTR_COOP_BATTLE_BEGIN_PACKET, data, size, totalSize, checksum);
}

bool CtrCoopProtocol_EncodeBattleRequest(uint32_t offset, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferRequest(CTR_COOP_BATTLE_REQUEST_PACKET, offset, out, outSize);
}

bool CtrCoopProtocol_DecodeBattleRequest(const uint8_t *data, uint16_t size, uint32_t *offset)
{
    return DecodeTransferRequest(CTR_COOP_BATTLE_REQUEST_PACKET, data, size, offset);
}

bool CtrCoopProtocol_EncodeBattleChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferChunk(CTR_COOP_BATTLE_CHUNK_PACKET, offset, data, dataSize, out, outSize);
}

bool CtrCoopProtocol_DecodeBattleChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize)
{
    return DecodeTransferChunk(CTR_COOP_BATTLE_CHUNK_PACKET, data, size, expectedOffset, out, outCapacity, outSize);
}

bool CtrCoopProtocol_EncodeDoubleBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferBegin(CTR_COOP_DOUBLE_BEGIN_PACKET, totalSize, checksum, out, outSize);
}

bool CtrCoopProtocol_DecodeDoubleBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum)
{
    return DecodeTransferBegin(CTR_COOP_DOUBLE_BEGIN_PACKET, data, size, totalSize, checksum);
}

bool CtrCoopProtocol_EncodeDoubleRequestData(uint32_t offset, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferRequest(CTR_COOP_DOUBLE_REQUEST_DATA_PACKET, offset, out, outSize);
}

bool CtrCoopProtocol_DecodeDoubleRequestData(const uint8_t *data, uint16_t size, uint32_t *offset)
{
    return DecodeTransferRequest(CTR_COOP_DOUBLE_REQUEST_DATA_PACKET, data, size, offset);
}

bool CtrCoopProtocol_EncodeDoubleChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize)
{
    return EncodeTransferChunk(CTR_COOP_DOUBLE_CHUNK_PACKET, offset, data, dataSize, out, outSize);
}

bool CtrCoopProtocol_DecodeDoubleChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize)
{
    return DecodeTransferChunk(CTR_COOP_DOUBLE_CHUNK_PACKET, data, size, expectedOffset, out, outCapacity, outSize);
}

uint32_t CtrCoopProtocol_Checksum(const uint8_t *data, uint32_t size)
{
    uint32_t hash = 2166136261u;

    if (data == NULL && size != 0)
        return 0;
    for (uint32_t i = 0; i < size; i++)
    {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
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
     || last->graphicsId != current->graphicsId || last->bikeMode != current->bikeMode
     || last->movementSequence != current->movementSequence)
        return true;
    return framesSinceLast >= CTR_COOP_HEARTBEAT_FRAMES;
}

CtrCoopApply CtrCoopProtocol_PlanMovement(const CtrCoopSnapshot *snapshot,
    int16_t currentX, int16_t currentY, bool freshSpawn, bool alreadyApplied, bool movementInProgress)
{
    int dx, dy;
    uint8_t direction;

    if (snapshot == NULL || alreadyApplied || movementInProgress)
        return CTR_COOP_APPLY_NOTHING;
    dx = snapshot->x - currentX;
    dy = snapshot->y - currentY;
    if (freshSpawn && (dx != 0 || dy != 0 || snapshot->bikeMode == CTR_COOP_BIKE_NONE
     || !IsInPlace(snapshot->movementActionId) || snapshot->movementActionId <= CTR_COOP_ACTION_FACE_RIGHT))
        return CTR_COOP_APPLY_NOTHING;
    if (dx == 0 && dy == 0)
        return snapshot->bikeMode != CTR_COOP_BIKE_NONE && IsInPlace(snapshot->movementActionId)
            && snapshot->movementActionId > CTR_COOP_ACTION_FACE_RIGHT ? CTR_COOP_APPLY_START : CTR_COOP_APPLY_FACE;
    if (!IsMoving(snapshot->movementActionId))
        return CTR_COOP_APPLY_SNAP;
    direction = snapshot->facing;
    int distance = IsTwoTileJump(snapshot->movementActionId) ? 2 : 1;
    if ((direction == 1 && dx == 0 && dy == distance)
     || (direction == 2 && dx == 0 && dy == -distance)
     || (direction == 3 && dx == -distance && dy == 0)
     || (direction == 4 && dx == distance && dy == 0))
        return CTR_COOP_APPLY_START;
    return CTR_COOP_APPLY_SNAP;
}
