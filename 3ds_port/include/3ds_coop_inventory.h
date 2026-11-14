#ifndef CTR_COOP_INVENTORY_H
#define CTR_COOP_INVENTORY_H

#include <stdbool.h>
#include <stdint.h>

#define CTR_COOP_INVENTORY_VERSION 18
#define CTR_COOP_INVENTORY_REQUEST 65
#define CTR_COOP_INVENTORY_RESULT 66
#define CTR_COOP_INVENTORY_BEGIN 67
#define CTR_COOP_INVENTORY_ROW 68
#define CTR_COOP_INVENTORY_CONTROL 69
#define CTR_COOP_INVENTORY_MAX_ITEMS 512
#define CTR_COOP_INVENTORY_BAG 0
#define CTR_COOP_INVENTORY_PC 1
#define CTR_COOP_INVENTORY_SCOPES 2
#define CTR_COOP_INVENTORY_WAIT_MS 500
#define CTR_COOP_INVENTORY_RETRY_MS 100

typedef enum {
    CTR_INV_REJECTED,
    CTR_INV_ACCEPTED,
    CTR_INV_PENDING,
} CtrCoopInventoryResult;

/* read sums all slots of one item, decoding the CURRENT save encryption key.
 * write is atomic absolute replacement, never a delta, and must bypass hooks.
 * install atomically replaces BOTH complete containers, including zeroes.
 * pump services transport/packet handlers only: never RunTasks/game callbacks.
 * pause displays a brief syncing notice and services/waits for the SAME epoch;
 * it must not reset/leave the transaction or run an item effect. */
typedef struct {
    void *user;
    uint32_t (*read)(void *, uint8_t scope, uint16_t item);
    bool (*write)(void *, uint8_t scope, uint16_t item, uint32_t quantity);
    bool (*install)(void *, const uint32_t quantities[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS], uint16_t items);
    bool (*send)(void *, uint16_t peer, const uint8_t *, uint16_t size);
    void (*pump)(void *);
    uint32_t (*clock)(void *);
    void (*pause)(void *);
    bool (*canonical)(void *); /* false for temporary tutorial/Frontier bags */
    /* Optional: independent saturating scalar counters (wallet), NOT shared
     * bag pockets. ADD clips to max minus refundable credits of this item.
     * NULL preserves strict all-or-nothing bag additions and refund capacity. */
    uint32_t (*capacity)(void *, uint8_t scope, uint16_t item);
    /* Optional pure capacity/ID validation of a full candidate. Never writes
     * temporary native views. Enables canonical ADD/refund while Wally's
     * temporary bag is displayed; install follows after native restore. */
    bool (*validate)(void *, const uint32_t quantities[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS], uint16_t items);
} CtrCoopInventoryIO;

typedef struct {
    CtrCoopInventoryIO io;
    uint32_t quantity[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS];
    uint32_t revisionFor[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS];
    uint32_t frozen[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS];
    int32_t reconcileDelta[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS];
    bool seen[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS];
    bool updates[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS];
    uint32_t epoch, revision, snapshotRevision, lastRequest, nextRequest, retryAt;
    uint32_t clientNonce;
    uint32_t pendingRequest, pendingCount, cachedCount;
    uint16_t peer, items, snapshotCursor, snapshotRows, pendingItem, cachedItem;
    uint8_t pendingScope, pendingKind, cachedScope, cachedKind;
    bool enabled, host, ready, peerReady, viewActive, dirty, applying;
    bool snapshotSending, snapshotReceiving, pending, known, accepted;
    bool helloPending;
    bool snapshotRestart;
    bool cachedAccepted, cachedAcked, uncertain, usedLive;
    uint16_t useItem;
    uint32_t useCount;
    uint16_t reconcileCursor;
    bool reconciling, reconcileWaiting;
    uint16_t hostUseItem[2];
    uint32_t hostUseCount[2];
} CtrCoopInventory;

/* Host epoch is a nonzero fresh session nonce. Guest epoch=0 learns it ONLY
 * from the authenticated host's initial snapshot; do not enable gameplay yet.
 * item count is ITEMS_COUNT, including ITEM_NONE, <= MAX_ITEMS. */
bool CtrCoopInventory_Init(CtrCoopInventory *, const CtrCoopInventoryIO *, bool host,
    uint16_t peer, uint32_t epoch, uint16_t items);
/* Refuses to erase an unresolved operation or unacknowledged host decision. */
bool CtrCoopInventory_Reset(CtrCoopInventory *);
/* Authenticated reconnect of the SAME player and epoch; retains all intent,
 * decisions and reserve credits. Never Init/Reset to reconnect an open RPC. */
bool CtrCoopInventory_RebindPeer(CtrCoopInventory *, uint16_t peer);
/* Fresh random nonzero nonce per guest PROCESS. Retain it for RAM reconnect.
 * A different authenticated client nonce rotates the host epoch/counter and
 * abandons old guest credits without refunding already canonical debits. */
bool CtrCoopInventory_Hello(CtrCoopInventory *, uint32_t clientNonce);
bool CtrCoopInventory_HasOpenUse(const CtrCoopInventory *);
bool CtrCoopInventory_StartSnapshot(CtrCoopInventory *);
void CtrCoopInventory_Tick(CtrCoopInventory *);
bool CtrCoopInventory_HandlePacket(CtrCoopInventory *, uint16_t from, const uint8_t *, uint16_t size);
CtrCoopInventoryResult CtrCoopInventory_Change(CtrCoopInventory *, uint8_t scope, uint16_t item, int32_t delta);
CtrCoopInventoryResult CtrCoopInventory_Resume(CtrCoopInventory *);
uint32_t CtrCoopInventory_GetQuantity(const CtrCoopInventory *, uint8_t scope, uint16_t item);
bool CtrCoopInventory_IsReady(const CtrCoopInventory *);
bool CtrCoopInventory_IsActive(const CtrCoopInventory *);
bool CtrCoopInventory_UsedLive(const CtrCoopInventory *);
bool CtrCoopInventory_IsUncertain(const CtrCoopInventory *);
bool CtrCoopInventory_Audit(const CtrCoopInventory *);
/* Import ONLY verified canonical raw writes (e.g. legacy engine paths) as
 * live transactions. Call before battle result freezing; false defers freeze.
 * Never call on tutorial/Frontier temporary copies or a stale save snapshot. */
bool CtrCoopInventory_Reconcile(CtrCoopInventory *);
bool CtrCoopInventory_SetViewActive(CtrCoopInventory *, bool active);

/* Singleton bridge for item.c. -1: offline/temporary view, use native function;
 * 0/1: definitive failure/success, mutation already materialized. On ambiguity
 * the bridge PAUSES before returning; bool is never an invented timeout abort.
 * Bind NULL only after Reset succeeds. Applying callbacks bypass this bridge. */
void CtrCoopInventory_Bind(CtrCoopInventory *);
int CtrCoopInventory_HookChange(uint8_t scope, uint16_t item, int32_t delta);
/* Atomic transfer between BAG and PC; handled success replaces BOTH native
 * Add(destination)/Remove(source). The menu must skip its deferred remove. */
int CtrCoopInventory_HookTransfer(uint8_t source, uint16_t item, uint16_t count);
/* Atomic held-item exchange: bag debit + return of the previous held item.
 * Mutates BAG only; caller changes the Pokemon only after definitive success. */
int CtrCoopInventory_HookExchange(uint16_t removeItem, uint16_t returnItem, uint16_t count);
/* Reserve BEFORE native effects. Native RemoveBagItem consumes this credit
 * without a second debit. RefundUse on no-effect/cancel waits for definitive
 * refund. One native item-use flow may hold a credit at a time. */
bool CtrCoopInventory_BeginUse(uint16_t item, uint16_t count);
void CtrCoopInventory_RefundUse(void);
/* State-based form for a second independent ledger (e.g. shared wallet).
 * Uses scope BAG=0. Caller owns its wire remapping; singleton stays items. */
bool CtrCoopInventory_ReserveUse(CtrCoopInventory *, uint16_t item, uint32_t count);
bool CtrCoopInventory_FinishUse(CtrCoopInventory *, uint16_t item, uint32_t count);
void CtrCoopInventory_RefundUseState(CtrCoopInventory *);
void CtrCoopInventory_TemporaryBag(bool temporary);
/* Engine implementation marks both bag/PC menu views dirty; safe list tasks
 * rebuild on their next tick. Never call RunTasks from a write callback. */
void CtrCoop_EngineInventoryChanged(void);

#endif
