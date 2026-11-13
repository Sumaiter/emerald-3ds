#ifndef CTR_COOP_WALK_H
#define CTR_COOP_WALK_H

#include <stdint.h>
struct ObjectEventGraphicsInfo;
struct Sprite;

/* These accessors do not change global NPC graphics. Call GraphicsInfo only
 * after the existing player-role resolver accepts the object or explicitly
 * from the character-selection preview. Returned tables are immutable per ID. */
const struct ObjectEventGraphicsInfo *CtrCoopWalk_GraphicsInfo(uint8_t graphicsId,
    const struct ObjectEventGraphicsInfo *original);
const uint16_t *CtrCoopWalk_Palette(uint8_t graphicsId);
uint8_t CtrCoopWalk_CreatePreview(uint8_t graphicsId, void (*callback)(struct Sprite *),
    int16_t x, int16_t y, uint8_t subpriority);
/* Call after destroying the previous page, before loading the next page. */
void CtrCoopWalk_FreePreviewPalettes(void);
void CtrCoopWalk_Reset(void);

#endif
