#ifndef CTR_COOP_BATTLE_H
#define CTR_COOP_BATTLE_H

#include <stdint.h>
#include <stdbool.h>

struct SpriteTemplate;

/* Local and cooperative partner controllers use their selected identities.
 * Native choices retain animation; custom art has independent frame tables. */
uint16_t CtrCoopBattle_BackPicId(uint16_t original);
bool CtrCoopBattle_Active(void);
void CtrCoopBattle_Apply(uint8_t battler, struct SpriteTemplate *template);
/* The throw animation moves the trainer into a newly allocated OBJ slot. */
void CtrCoopBattle_LoadPalette(uint8_t paletteNum);
uint16_t CtrCoopBattle_BattlerBackPicId(uint8_t battler, uint16_t original);
void CtrCoopBattle_LoadBattlerPalette(uint8_t battler, uint8_t paletteNum);

#endif
