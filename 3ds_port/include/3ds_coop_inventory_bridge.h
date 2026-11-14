#ifndef CTR_COOP_INVENTORY_BRIDGE_H
#define CTR_COOP_INVENTORY_BRIDGE_H
#include <stdbool.h>
#include <stdint.h>

void CtrCoopInventoryBridge_Session(bool active, bool guest, uint16_t peer, bool saveApplied);
void CtrCoopInventoryBridge_AfterSaveCopy(void);
void CtrCoopInventoryBridge_Tick(void);
bool CtrCoopInventoryBridge_HandlePacket(uint16_t from, const uint8_t *data, uint16_t len);
bool CtrCoopInventoryBridge_Active(void);
bool CtrCoopInventoryBridge_Ready(void);
bool CtrCoopInventoryBridge_FinishBattle(void);
bool CtrCoopInventoryBridge_HasPending(void);
bool CtrCoopInventoryBridge_KnownPeer(void);
bool CtrCoopInventoryBridge_IsResuming(void);
void CtrCoopInventoryBridge_Reset(void);
bool CtrCoopInventoryBridge_PrepareNativeLink(void);
bool CtrCoopInventoryBridge_CanPrepareNativeLink(void);
/* Only after the old engine/effect stack has been discarded by longjmp. */
void CtrCoopInventoryBridge_AfterStackReset(void);
/* Network-only retry pump, implemented alongside the co-op state machine. */
void CtrCoop_PumpInventory(void);
void PortRfu_PumpCoopEvents(void);
#endif
