#ifndef CTR_COOP_MONEY_H
#define CTR_COOP_MONEY_H
#include <stdbool.h>
#include <stdint.h>

#define CTR_COOP_MONEY_FIRST_PACKET 80
#define CTR_COOP_MONEY_LAST_PACKET 84
#define CTR_COOP_MONEY_CASH 1
#define CTR_COOP_MONEY_COINS 2

void CtrCoopMoney_Session(bool active, bool guest, uint16_t peer, bool saveApplied);
void CtrCoopMoney_AfterSaveCopy(void);
void CtrCoopMoney_Tick(void);
bool CtrCoopMoney_HandlePacket(uint16_t from, const uint8_t *data, uint16_t len);
bool CtrCoopMoney_Active(void);
bool CtrCoopMoney_Ready(void);
bool CtrCoopMoney_UsedLive(void);
bool CtrCoopMoney_FinishBattle(void);
bool CtrCoopMoney_HasPending(void);
void CtrCoopMoney_Reset(void);
/* A native link session may start only after every wallet intent is resolved. */
bool CtrCoopMoney_CanPrepareNativeLink(void);
bool CtrCoopMoney_PrepareNativeLink(void);
/* ONLY after the whole game stack has actually been unwound. Discard its
 * abandoned continuation without refund, replay, or writes to either save. */
void CtrCoopMoney_AfterStackReset(void);
/* Hook results: -1 native/offline/temporary; 0 failure; 1 materialized.
 * The wallet owns an independent ledger; never bind the item singleton. */
bool CtrCoopMoney_HookRead(uint16_t currency, uint32_t *value);
int CtrCoopMoney_HookChange(uint16_t currency, bool add, uint32_t count);
int CtrCoopMoney_HookSet(uint16_t currency, uint32_t value);
/* Reserve before granting goods/services. Native subtraction consumes the
 * credit without another debit. Refund at failed effect/cancel/script end. */
bool CtrCoopMoney_BeginUse(uint16_t currency, uint32_t count);
void CtrCoopMoney_RefundUse(void);
bool CtrCoopMoney_Spend(uint16_t currency, uint32_t count);
#endif
