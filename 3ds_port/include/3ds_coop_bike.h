#ifndef CTR_COOP_BIKE_H
#define CTR_COOP_BIKE_H

#include <stdbool.h>
#include <stdint.h>
#include "3ds_coop_protocol.h"

struct ObjectEvent;
struct ObjectEventGraphicsInfo;
struct Sprite;

/* Keep the selected identity in ObjectEvent.graphicsId, including on a bike.
 * Mode is separate; no graphics ID is taken from NPCs, scripts or followers. */
uint8_t CtrCoopBike_LocalAvatarGraphicsId(uint8_t state, uint8_t original);
uint8_t CtrCoopBike_LocalMode(void);
uint8_t CtrCoopBike_LocalMovementSequence(void);
/* Invoke after ObjectEventSetHeldMovement actually accepts a local action. */
void CtrCoopBike_MovementStarted(const struct ObjectEvent *object, uint8_t action);
uint8_t CtrCoopBike_LocalStateFlag(uint8_t original);
/* True only when a valid remote context changes and needs sprite rebinding.
 * Identical context or invalid values return false without mutation. */
bool CtrCoopBike_SetRemote(uint8_t graphicsId, uint8_t mode);
void CtrCoopBike_ClearRemote(void);

/* Called only for real object events and their spawn templates. A plain
 * GetObjectEventGraphicsInfo(id) remains independent of player state. */
const struct ObjectEventGraphicsInfo *CtrCoopBike_GraphicsInfo(
    uint8_t graphicsId, uint8_t localId, uint8_t movementType,
    const struct ObjectEventGraphicsInfo *original);
const struct ObjectEventGraphicsInfo *CtrCoopBike_ObjectGraphicsInfo(
    const struct ObjectEvent *object, const struct ObjectEventGraphicsInfo *original);
void CtrCoopBike_BindSprite(const struct ObjectEvent *object, struct Sprite *sprite);
/* Only actual selected players bypass writes to the shared NPC palettes. */
bool CtrCoopBike_UsesOwnPalette(uint8_t graphicsId, uint8_t localId, uint8_t movementType);
/* Engine hook: bounded lookup of original palette asset pointers, never art
 * copied into this module or the tracked generated-asset directory. */
const uint16_t *CtrCoopBike_ObjectPalette(uint16_t tag);
/* Copy the source palette to a player reflection, including clearing it on
 * dismount. darkPaletteTag=65535 uses source colors; a bridge palette tag uses
 * the native dark-blue reflection asset in a separate ext slot. Returns true
 * when the binding changed; unrelated objects remain untouched. */
bool CtrCoopBike_BindReflection(const struct ObjectEvent *object,
    const struct Sprite *source, struct Sprite *reflection, uint16_t darkPaletteTag);
/* Reserve enough tiles on foot too: mounting must not overrun a small NPC's
 * original 16x16 allocation. Use the same result when destroying the sprite. */
uint16_t CtrCoopBike_AllocationSize(uint8_t localId, uint8_t movementType, uint16_t original);
void CtrCoopBike_Reset(void);

#endif
