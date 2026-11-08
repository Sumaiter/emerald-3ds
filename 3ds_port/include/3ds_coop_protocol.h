#ifndef CTR_COOP_PROTOCOL_H
#define CTR_COOP_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define CTR_COOP_PEER_PACKET 8
#define CTR_COOP_AUTH_PACKET 9
#define CTR_COOP_AUTH_RESULT_PACKET 10
#define CTR_COOP_AUTH_PACKET_SIZE 6
#define CTR_COOP_AUTH_RESULT_SIZE 3
#define CTR_COOP_PROTOCOL_VERSION 2
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
bool CtrCoopProtocol_SameMap(const CtrCoopSnapshot *a, const CtrCoopSnapshot *b);
bool CtrCoopProtocol_ShouldSend(const CtrCoopSnapshot *last, const CtrCoopSnapshot *current, uint16_t framesSinceLast);
CtrCoopApply CtrCoopProtocol_PlanMovement(const CtrCoopSnapshot *snapshot,
    int16_t currentX, int16_t currentY, bool freshSpawn, bool alreadyApplied, bool movementInProgress);

#endif // CTR_COOP_PROTOCOL_H
