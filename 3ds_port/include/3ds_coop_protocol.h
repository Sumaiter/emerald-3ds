#ifndef CTR_COOP_PROTOCOL_H
#define CTR_COOP_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define CTR_COOP_PEER_PACKET 8
#define CTR_COOP_AUTH_PACKET 9
#define CTR_COOP_AUTH_RESULT_PACKET 10
#define CTR_COOP_IDENTITY_PACKET 11
#define CTR_COOP_SPAWN_PACKET 12
#define CTR_COOP_SAVE_BEGIN_PACKET 13
#define CTR_COOP_SAVE_CHUNK_PACKET 14
#define CTR_COOP_SAVE_REQUEST_PACKET 15
#define CTR_COOP_SAVE_READY_PACKET 16
#define CTR_COOP_PC_REQUEST_PACKET 17
#define CTR_COOP_PC_RESULT_PACKET 18
#define CTR_COOP_PC_RELEASE_PACKET 19
#define CTR_COOP_FLAG_PACKET 20
#define CTR_COOP_VAR_PACKET 21
#define CTR_COOP_PC_SYNC_BEGIN_PACKET 22
#define CTR_COOP_PC_SYNC_CHUNK_PACKET 23
#define CTR_COOP_PC_SYNC_REQUEST_PACKET 24
#define CTR_COOP_PC_SYNC_READY_PACKET 25
#define CTR_COOP_PC_CLAIM_PACKET 26
#define CTR_COOP_NPC_PACKET 27
#define CTR_COOP_NPC_PACKET_SIZE 12
#define CTR_COOP_JOIN_INFO_PACKET 28
#define CTR_COOP_JOIN_INFO_SIZE 3
#define CTR_COOP_JOIN_STARTER 1
#define CTR_COOP_NAME_LIMIT_PACKET 29
#define CTR_COOP_NAME_LIMIT_SIZE 2
#define CTR_COOP_PARTY_BEGIN_PACKET 30
#define CTR_COOP_PARTY_CHUNK_PACKET 31
#define CTR_COOP_PARTY_RESULT_PACKET 32
#define CTR_COOP_PARTY_RESULT_SIZE 3
#define CTR_COOP_BATTLE_RESERVE_PACKET 33
#define CTR_COOP_BATTLE_RESERVE_RESULT_PACKET 34
#define CTR_COOP_BATTLE_RELEASE_PACKET 35
#define CTR_COOP_BATTLE_BEGIN_PACKET 36
#define CTR_COOP_BATTLE_CHUNK_PACKET 37
#define CTR_COOP_BATTLE_REQUEST_PACKET 38
#define CTR_COOP_BATTLE_READY_PACKET 39
#define CTR_COOP_DOUBLE_REQUEST_PACKET 40
#define CTR_COOP_DOUBLE_ACCEPT_PACKET 41
#define CTR_COOP_DOUBLE_BEGIN_PACKET 42
#define CTR_COOP_DOUBLE_REQUEST_DATA_PACKET 43
#define CTR_COOP_DOUBLE_CHUNK_PACKET 44
#define CTR_COOP_DOUBLE_READY_PACKET 45
#define CTR_COOP_DOUBLE_DECLINE_PACKET 46
#define CTR_COOP_DOUBLE_REQUEST_SIZE 4
#define CTR_COOP_DOUBLE_CONTROL_SIZE 2
#define CTR_COOP_BATTLE_RESERVE_SIZE 4
#define CTR_COOP_BATTLE_RESERVE_RESULT_SIZE 4
#define CTR_COOP_PC_CONTROL_SIZE 2
#define CTR_COOP_PC_RESULT_SIZE 3
#define CTR_COOP_FLAG_SIZE 5
#define CTR_COOP_VAR_SIZE 6
#define CTR_COOP_AUTH_PACKET_SIZE 6
#define CTR_COOP_AUTH_RESULT_SIZE 3
#define CTR_COOP_IDENTITY_SIZE 10
#define CTR_COOP_SPAWN_SIZE 15
#define CTR_COOP_SAVE_BEGIN_SIZE 10
#define CTR_COOP_SAVE_REQUEST_SIZE 6
#define CTR_COOP_SAVE_CHUNK_HEADER 8
#define CTR_COOP_SAVE_CHUNK_DATA_MAX 128
#define CTR_COOP_SAVE_CHUNK_MAX (CTR_COOP_SAVE_CHUNK_HEADER + CTR_COOP_SAVE_CHUNK_DATA_MAX)
#define CTR_COOP_SAVE_READY_SIZE 2
#define CTR_COOP_PLAYER_NAME_SIZE 8
#define CTR_COOP_PROTOCOL_VERSION 12
#define CTR_COOP_GRAPHICS_ID_MAX 238
#define CTR_COOP_PACKET_SIZE 15
#define CTR_COOP_HEARTBEAT_FRAMES 60

#define CTR_COOP_ACTION_FACE_DOWN 0x00
#define CTR_COOP_ACTION_FACE_UP 0x01
#define CTR_COOP_ACTION_FACE_LEFT 0x02
#define CTR_COOP_ACTION_FACE_RIGHT 0x03
#define CTR_COOP_ACTION_WALK_NORMAL_DOWN 0x08
#define CTR_COOP_ACTION_PLAYER_RUN_DOWN 0x35
#define CTR_COOP_ACTION_JUMP_2_DOWN 0x0C
#define CTR_COOP_ACTION_WALK_FAST_DOWN 0x15
#define CTR_COOP_ACTION_WALK_IN_PLACE_SLOW_DOWN 0x19
#define CTR_COOP_ACTION_RIDE_WATER_CURRENT_DOWN 0x29
#define CTR_COOP_ACTION_WALK_FASTER_DOWN 0x2D
#define CTR_COOP_ACTION_JUMP_DOWN 0x42
#define CTR_COOP_ACTION_JUMP_IN_PLACE_DOWN 0x46
#define CTR_COOP_ACTION_JUMP_TURN_DOWN_UP 0x4A
#define CTR_COOP_ACTION_ACRO_FIRST 0x64
#define CTR_COOP_ACTION_ACRO_LAST 0x8B
#define CTR_COOP_BIKE_NONE 0
#define CTR_COOP_BIKE_MACH 1
#define CTR_COOP_BIKE_ACRO 2

typedef struct
{
    uint8_t mapGroup;
    uint8_t mapNum;
    uint16_t mapLayoutId;
    int16_t x;
    int16_t y;
    uint8_t facing;
    uint8_t movementActionId;
    uint8_t graphicsId;
    uint8_t bikeMode;
    uint8_t movementSequence;
} CtrCoopSnapshot;

typedef struct
{
    uint8_t localId;
    uint8_t mapGroup;
    uint8_t mapNum;
    int16_t x;
    int16_t y;
    uint8_t facing;
    uint8_t graphicsId;
} CtrCoopNpcSnapshot;

typedef enum
{
    CTR_COOP_APPLY_NOTHING,
    CTR_COOP_APPLY_START,
    CTR_COOP_APPLY_SNAP,
    CTR_COOP_APPLY_FACE,
} CtrCoopApply;

bool CtrCoopProtocol_Encode(const CtrCoopSnapshot *snapshot, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_Decode(const uint8_t *data, uint16_t size, CtrCoopSnapshot *out);
bool CtrCoopProtocol_EncodeAuth(const uint8_t *password, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeAuth(const uint8_t *data, uint16_t size, uint8_t *password);
bool CtrCoopProtocol_EncodeAuthResult(bool accepted, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeAuthResult(const uint8_t *data, uint16_t size, bool *accepted);
bool CtrCoopProtocol_EncodeJoinInfo(bool starterRequired, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeJoinInfo(const uint8_t *data, uint16_t size, bool *starterRequired);
bool CtrCoopProtocol_EncodeNameLimit(uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeNameLimit(const uint8_t *data, uint16_t size);
bool CtrCoopProtocol_EncodeIdentity(const uint8_t *name, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeIdentity(const uint8_t *data, uint16_t size, uint8_t *name);
bool CtrCoopProtocol_EncodeSpawn(const CtrCoopSnapshot *snapshot, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeSpawn(const uint8_t *data, uint16_t size, CtrCoopSnapshot *out);
bool CtrCoopProtocol_EncodeNpc(const CtrCoopNpcSnapshot *snapshot, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeNpc(const uint8_t *data, uint16_t size, CtrCoopNpcSnapshot *out);
bool CtrCoopProtocol_EncodeSaveBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeSaveBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum);
bool CtrCoopProtocol_EncodeSaveRequest(uint32_t offset, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeSaveRequest(const uint8_t *data, uint16_t size, uint32_t *offset);
bool CtrCoopProtocol_EncodeSaveChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeSaveChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize);
bool CtrCoopProtocol_EncodePcSyncBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodePcSyncBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum);
bool CtrCoopProtocol_EncodePcSyncRequest(uint32_t offset, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodePcSyncRequest(const uint8_t *data, uint16_t size, uint32_t *offset);
bool CtrCoopProtocol_EncodePcSyncChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodePcSyncChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize);
bool CtrCoopProtocol_EncodeBattleReserve(uint16_t trainerId, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeBattleReserve(const uint8_t *data, uint16_t size, uint16_t *trainerId);
bool CtrCoopProtocol_EncodeBattleReserveResult(uint16_t trainerId, bool accepted, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeBattleReserveResult(const uint8_t *data, uint16_t size, uint16_t *trainerId, bool *accepted);
bool CtrCoopProtocol_EncodeBattleBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeBattleBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum);
bool CtrCoopProtocol_EncodeBattleRequest(uint32_t offset, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeBattleRequest(const uint8_t *data, uint16_t size, uint32_t *offset);
bool CtrCoopProtocol_EncodeBattleChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeBattleChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize);
bool CtrCoopProtocol_EncodeDoubleBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeDoubleBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum);
bool CtrCoopProtocol_EncodeDoubleRequestData(uint32_t offset, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeDoubleRequestData(const uint8_t *data, uint16_t size, uint32_t *offset);
bool CtrCoopProtocol_EncodeDoubleChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeDoubleChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize);
uint32_t CtrCoopProtocol_Checksum(const uint8_t *data, uint32_t size);
bool CtrCoopProtocol_EncodePartyBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodePartyBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum);
bool CtrCoopProtocol_EncodePartyChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodePartyChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize);
bool CtrCoopProtocol_EncodePartyResult(bool accepted, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodePartyResult(const uint8_t *data, uint16_t size, bool *accepted);
bool CtrCoopProtocol_EncodePcResult(bool accepted, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodePcResult(const uint8_t *data, uint16_t size, bool *accepted);
bool CtrCoopProtocol_EncodeFlag(uint16_t id, bool value, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeFlag(const uint8_t *data, uint16_t size, uint16_t *id, bool *value);
bool CtrCoopProtocol_EncodeVar(uint16_t id, uint16_t value, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeVar(const uint8_t *data, uint16_t size, uint16_t *id, uint16_t *value);
bool CtrCoopProtocol_SameMap(const CtrCoopSnapshot *a, const CtrCoopSnapshot *b);
bool CtrCoopProtocol_ShouldSend(const CtrCoopSnapshot *last, const CtrCoopSnapshot *current, uint16_t framesSinceLast);
CtrCoopApply CtrCoopProtocol_PlanMovement(const CtrCoopSnapshot *snapshot,
    int16_t currentX, int16_t currentY, bool freshSpawn, bool alreadyApplied, bool movementInProgress);

#endif // CTR_COOP_PROTOCOL_H
