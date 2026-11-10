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
#define CTR_COOP_PC_CONTROL_SIZE 2
#define CTR_COOP_PC_RESULT_SIZE 3
#define CTR_COOP_FLAG_SIZE 5
#define CTR_COOP_VAR_SIZE 6
#define CTR_COOP_AUTH_PACKET_SIZE 6
#define CTR_COOP_AUTH_RESULT_SIZE 3
#define CTR_COOP_IDENTITY_SIZE 10
#define CTR_COOP_SPAWN_SIZE 13
#define CTR_COOP_SAVE_BEGIN_SIZE 10
#define CTR_COOP_SAVE_REQUEST_SIZE 6
#define CTR_COOP_SAVE_CHUNK_HEADER 8
#define CTR_COOP_SAVE_CHUNK_DATA_MAX 128
#define CTR_COOP_SAVE_CHUNK_MAX (CTR_COOP_SAVE_CHUNK_HEADER + CTR_COOP_SAVE_CHUNK_DATA_MAX)
#define CTR_COOP_SAVE_READY_SIZE 2
#define CTR_COOP_PLAYER_NAME_SIZE 8
#define CTR_COOP_PROTOCOL_VERSION 5
#define CTR_COOP_GRAPHICS_ID_MAX 238
#define CTR_COOP_PACKET_SIZE 13
#define CTR_COOP_HEARTBEAT_FRAMES 60

#define CTR_COOP_ACTION_FACE_DOWN 0x00
#define CTR_COOP_ACTION_FACE_UP 0x01
#define CTR_COOP_ACTION_FACE_LEFT 0x02
#define CTR_COOP_ACTION_FACE_RIGHT 0x03
#define CTR_COOP_ACTION_WALK_NORMAL_DOWN 0x08
#define CTR_COOP_ACTION_PLAYER_RUN_DOWN 0x35

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
} CtrCoopSnapshot;

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
bool CtrCoopProtocol_EncodeIdentity(const uint8_t *name, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeIdentity(const uint8_t *data, uint16_t size, uint8_t *name);
bool CtrCoopProtocol_EncodeSpawn(const CtrCoopSnapshot *snapshot, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeSpawn(const uint8_t *data, uint16_t size, CtrCoopSnapshot *out);
bool CtrCoopProtocol_EncodeSaveBegin(uint32_t totalSize, uint32_t checksum, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeSaveBegin(const uint8_t *data, uint16_t size, uint32_t *totalSize, uint32_t *checksum);
bool CtrCoopProtocol_EncodeSaveRequest(uint32_t offset, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeSaveRequest(const uint8_t *data, uint16_t size, uint32_t *offset);
bool CtrCoopProtocol_EncodeSaveChunk(uint32_t offset, const uint8_t *data, uint16_t dataSize, uint8_t *out, uint16_t outSize);
bool CtrCoopProtocol_DecodeSaveChunk(const uint8_t *data, uint16_t size, uint32_t expectedOffset,
    uint8_t *out, uint16_t outCapacity, uint16_t *outSize);
uint32_t CtrCoopProtocol_Checksum(const uint8_t *data, uint32_t size);
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
