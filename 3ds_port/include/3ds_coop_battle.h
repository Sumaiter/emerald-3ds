#ifndef CTR_COOP_BATTLE_H
#define CTR_COOP_BATTLE_H

#include <stdint.h>
#include <stdbool.h>

struct SpriteTemplate;

/* Only the local player and Safari controllers call these hooks. Native
 * choices keep their original frames; new art supplies safe static frames. */
uint16_t CtrCoopBattle_BackPicId(uint16_t original);
bool CtrCoopBattle_Active(void);
void CtrCoopBattle_Apply(uint8_t battler, struct SpriteTemplate *template);
/* The throw animation moves the trainer into a newly allocated OBJ slot. */
void CtrCoopBattle_LoadPalette(uint8_t paletteNum);

#endif
