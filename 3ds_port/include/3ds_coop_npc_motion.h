#ifndef CTR_COOP_NPC_MOTION_H
#define CTR_COOP_NPC_MOTION_H
#include <stdbool.h>
#include <stdint.h>
#define CTR_COOP_NPC_MOTION_PACKET 76
#define CTR_COOP_NPC_MOTION_END_PACKET 77
#define CTR_COOP_NPC_MOTION_SIZE 26
#define CTR_COOP_NPC_MOTION_END_SIZE 10
typedef struct {
    int16_t x, y;
    uint32_t actionSerial;
    uint8_t group, num, id, facing, graphics, action, flags;
} CtrCoopNpcMotionPose;
typedef struct {
    uint32_t playedAction;
    uint8_t objectId, spriteId;
    bool attached, wasFrozen;
} CtrCoopNpcMotionPlayback;
/* Network handler is called only after session authentication. */
bool CtrCoopNpcMotion_HandlePacket(uint16_t from, const uint8_t *, uint16_t len);
void CtrCoopNpcMotion_Tick(void);
void CtrCoopNpcMotion_End(void);
bool CtrCoopNpcMotion_Pending(void);
bool CtrCoopNpcMotion_Controls(uint8_t group, uint8_t num, uint8_t id);
/* Ordinary disconnect: stop publishing, settle already accepted live moves. */
void CtrCoopNpcMotion_Reset(void);
/* Only after the old game stack/world is actually discarded. */
void CtrCoopNpcMotion_AfterStackReset(void);
bool CtrCoopNpcMotion_Settling(void);
void CtrCoopNpcMotion_Track(uint8_t group, uint8_t num, uint8_t id);
void CtrCoopNpcMotion_Started(uint8_t group, uint8_t num, uint8_t id);
/* Native helpers never run ScriptContext, item effects or field specials.
 * Apply: -1 object unavailable, 0 animating/chasing, 1 settled at target. */
bool CtrCoop_EngineNpcMotionCapture(uint8_t group, uint8_t num, uint8_t id, CtrCoopNpcMotionPose *);
int CtrCoop_EngineNpcMotionApply(const CtrCoopNpcMotionPose *, CtrCoopNpcMotionPlayback *, bool ending);
#endif
