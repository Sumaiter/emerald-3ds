#ifndef CTR_COOP_STORAGE_H
#define CTR_COOP_STORAGE_H

#include <stdbool.h>
#include <stdint.h>

#define CTR_COOP_STORAGE_BOX_ID 13
#define CTR_COOP_STORAGE_COLUMNS 6
#define CTR_COOP_STORAGE_HOST_ROW 0
#define CTR_COOP_STORAGE_GUEST_ROW 1
#define CTR_COOP_STORAGE_ROW_SIZE 6

bool CtrCoopStorage_SlotLocked(bool hostActive, bool guestActive, uint8_t boxId, uint8_t position);
bool CtrCoopStorage_BoxRestricted(bool hostActive, bool guestActive, uint8_t boxId);

#endif // CTR_COOP_STORAGE_H
