#include <stdio.h>
#include "3ds_coop_storage.h"

static int sFailures;

#define CHECK(condition, message) do { if (!(condition)) { sFailures++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, message); } } while (0)

int main(void)
{
    CHECK(!CtrCoopStorage_SlotLocked(false, false, CTR_COOP_STORAGE_BOX_ID, 0), "offline host row is available");
    CHECK(!CtrCoopStorage_SlotLocked(false, false, CTR_COOP_STORAGE_BOX_ID, 6), "offline guest row is available");
    CHECK(CtrCoopStorage_SlotLocked(true, false, CTR_COOP_STORAGE_BOX_ID, 0), "online host row is locked");
    CHECK(CtrCoopStorage_SlotLocked(true, false, CTR_COOP_STORAGE_BOX_ID, 5), "all host slots are locked");
    CHECK(!CtrCoopStorage_SlotLocked(true, false, CTR_COOP_STORAGE_BOX_ID, 6), "offline guest row remains available");
    CHECK(CtrCoopStorage_SlotLocked(false, true, CTR_COOP_STORAGE_BOX_ID, 6), "online guest row is locked");
    CHECK(CtrCoopStorage_SlotLocked(false, true, CTR_COOP_STORAGE_BOX_ID, 11), "all guest slots are locked");
    CHECK(!CtrCoopStorage_SlotLocked(true, true, CTR_COOP_STORAGE_BOX_ID, 12), "unused rows remain available");
    CHECK(!CtrCoopStorage_SlotLocked(true, true, CTR_COOP_STORAGE_BOX_ID, 29), "last unused slot remains available");
    CHECK(!CtrCoopStorage_SlotLocked(true, true, 0, 0), "ordinary boxes are never player rows");
    CHECK(!CtrCoopStorage_SlotLocked(true, true, 12, 0), "last ordinary box is never locked");
    CHECK(CtrCoopStorage_BoxRestricted(true, false, CTR_COOP_STORAGE_BOX_ID), "host restricts co-op box multi-move");
    CHECK(CtrCoopStorage_BoxRestricted(false, true, CTR_COOP_STORAGE_BOX_ID), "guest restricts co-op box multi-move");
    CHECK(!CtrCoopStorage_BoxRestricted(false, false, CTR_COOP_STORAGE_BOX_ID), "offline co-op box allows multi-move");
    CHECK(!CtrCoopStorage_BoxRestricted(true, true, 3), "ordinary boxes allow multi-move");
    if (sFailures != 0)
    {
        printf("%d co-op storage tests failed\n", sFailures);
        return 1;
    }
    puts("co-op storage tests passed");
    return 0;
}
