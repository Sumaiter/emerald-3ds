#include "3ds_coop_storage.h"

bool CtrCoopStorage_SlotLocked(bool hostActive, bool guestActive, uint8_t boxId, uint8_t position)
{
    if (boxId != CTR_COOP_STORAGE_BOX_ID || position >= CTR_COOP_STORAGE_ROW_SIZE * 2)
        return false;
    if (position < CTR_COOP_STORAGE_ROW_SIZE)
        return hostActive;
    return guestActive;
}

bool CtrCoopStorage_BoxRestricted(bool hostActive, bool guestActive, uint8_t boxId)
{
    return boxId == CTR_COOP_STORAGE_BOX_ID && (hostActive || guestActive);
}
