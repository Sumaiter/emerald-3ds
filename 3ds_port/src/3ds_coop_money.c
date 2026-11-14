#include "global.h"
#include "3ds_coop_money.h"
#include "3ds_coop.h"
#include "3ds_coop_inventory.h"
#include "3ds_coop_inventory_bridge.h"
#include "3ds_local.h"
#include "3ds_platform.h"
#include "3ds_bottom.h"
#include "battle.h"
#include "main.h"
#include "random.h"
#include "constants/local_link.h"
#include <string.h>

static CtrCoopInventory sWallet;
static bool sConnected, sGuest, sSaveApplied, sReinstall;
static uint32_t sClientNonce;

static uint32_t Capacity(void *user, uint8_t scope, uint16_t currency)
{
    (void)user;
    return scope == CTR_COOP_INVENTORY_BAG ? (currency == CTR_COOP_MONEY_CASH ? 999999
        : currency == CTR_COOP_MONEY_COINS ? 9999 : 0) : 0;
}
static uint32_t Read(void *user, uint8_t scope, uint16_t currency)
{
    (void)user;
    if (scope || !gSaveBlock1Ptr || !gSaveBlock2Ptr) return 0;
    if (currency == CTR_COOP_MONEY_CASH) return gSaveBlock1Ptr->money ^ gSaveBlock2Ptr->encryptionKey;
    if (currency == CTR_COOP_MONEY_COINS) return gSaveBlock1Ptr->coins ^ (uint16_t)gSaveBlock2Ptr->encryptionKey;
    return 0;
}
static bool Write(void *user, uint8_t scope, uint16_t currency, uint32_t quantity)
{
    if (!gSaveBlock1Ptr || !gSaveBlock2Ptr || quantity > Capacity(user, scope, currency)) return false;
    if (scope) return quantity == 0;
    if (currency == CTR_COOP_MONEY_CASH)
        gSaveBlock1Ptr->money = quantity ^ gSaveBlock2Ptr->encryptionKey;
    else if (currency == CTR_COOP_MONEY_COINS)
        gSaveBlock1Ptr->coins = (uint16_t)quantity ^ (uint16_t)gSaveBlock2Ptr->encryptionKey;
    else return false;
    return true;
}
static bool Install(void *user, const uint32_t quantity[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS], uint16_t items)
{
    if (items != 3 || !gSaveBlock1Ptr || !gSaveBlock2Ptr
     || quantity[0][1] > Capacity(user, 0, 1) || quantity[0][2] > Capacity(user, 0, 2)
     || quantity[1][1] || quantity[1][2]) return false;
    Write(user, 0, 1, quantity[0][1]); Write(user, 0, 2, quantity[0][2]);
    return true;
}
static bool Send(void *user, uint16_t peer, const uint8_t *data, uint16_t len)
{
    uint8_t packet[32];
    (void)user;
    if (!sConnected || !data || !len || len > sizeof(packet)
     || data[0] < CTR_COOP_INVENTORY_REQUEST || data[0] > CTR_COOP_INVENTORY_CONTROL) return false;
    memcpy(packet, data, len); packet[0] += CTR_COOP_MONEY_FIRST_PACKET - CTR_COOP_INVENTORY_REQUEST;
    return LocalLink_TrySend(peer, packet, len);
}
static uint32_t Clock(void *user) { (void)user; return LocalLink_Milliseconds(); }
static void Pump(void *user) { (void)user; CtrCoop_PumpInventory(); CtrPlatform_SleepUs(1000); }
static void Pause(void *user)
{
    (void)user; CtrBottom_CoopSyncWait(); CtrPlatform_CoopSyncWait(); CtrCoop_PumpInventory();
}
static bool Canonical(void *user)
{
    (void)user;
    return gSaveBlock1Ptr && gSaveBlock2Ptr
        && !(LocalLink_GetSession() == LOCAL_SESSION_IN && LocalLink_GetSessionKind() != LOCAL_KIND_COOP)
        && !(gMain.inBattle && ((gBattleTypeFlags & (BATTLE_TYPE_WALLY_TUTORIAL | BATTLE_TYPE_RECORDED))
            || ((gBattleTypeFlags & BATTLE_TYPE_LINK) && !CtrCoop_IsDoubleBattle())));
}
static bool Hooked(void)
{
    return sWallet.enabled && !sWallet.applying && sWallet.viewActive && Canonical(NULL);
}
static void WaitReady(void)
{
    while (!CtrCoopInventory_IsReady(&sWallet))
    { Pause(NULL); CtrCoopInventory_Tick(&sWallet); }
}
static bool Wait(CtrCoopInventoryResult result)
{
    while (result == CTR_INV_PENDING)
    { Pause(NULL); result = CtrCoopInventory_Resume(&sWallet); }
    return result == CTR_INV_ACCEPTED;
}

void CtrCoopMoney_Session(bool active, bool guest, uint16_t peer, bool saveApplied)
{
    bool rebound = active && (!sConnected || sWallet.peer != peer);
    sConnected = active; sSaveApplied = saveApplied;
    /* A disconnected host keeps its authoritative ledger and can keep playing.
     * A disconnected guest retains unresolved requests for the same epoch. */
    if (!active || !peer) return;
    if (sWallet.enabled && sGuest != guest && !CtrCoopInventory_Reset(&sWallet)) return;
    sGuest = guest;
    if (!sWallet.enabled)
    {
        const CtrCoopInventoryIO io = {.read=Read, .write=Write, .install=Install, .send=Send,
            .pump=Pump, .clock=Clock, .pause=Pause, .canonical=Canonical, .capacity=Capacity};
        uint32_t nonce = ((uint32_t)Random() << 16) ^ Random() ^ LocalLink_Milliseconds();
        if (!nonce) nonce = 1;
        if (!sClientNonce) sClientNonce = nonce;
        if (!CtrCoopInventory_Init(&sWallet, &io, !guest, peer, guest ? 0 : nonce, 3)) return;
        if (guest && !saveApplied) CtrCoopInventory_SetViewActive(&sWallet, false);
        rebound = true;
    }
    if (rebound)
    {
        CtrCoopInventory_RebindPeer(&sWallet, peer);
        if (guest) CtrCoopInventory_Hello(&sWallet, sClientNonce);
    }
    if (guest && saveApplied && !sWallet.viewActive && !sWallet.pending)
        CtrCoopInventory_SetViewActive(&sWallet, true);
}
void CtrCoopMoney_AfterSaveCopy(void) { sReinstall = true; }
void CtrCoopMoney_Tick(void)
{
    if (!sWallet.enabled) return;
    if (sReinstall && sSaveApplied && sWallet.ready && !sWallet.pending
     && !CtrCoopInventory_HasOpenUse(&sWallet))
    {
        if (CtrCoopInventory_SetViewActive(&sWallet, false)
         && CtrCoopInventory_SetViewActive(&sWallet, true)) sReinstall = false;
    }
    CtrCoopInventory_Tick(&sWallet);
}
bool CtrCoopMoney_HandlePacket(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint8_t packet[32];
    if (!data || !len || data[0] < CTR_COOP_MONEY_FIRST_PACKET || data[0] > CTR_COOP_MONEY_LAST_PACKET) return false;
    if (!sConnected || len > sizeof(packet)) return true;
    memcpy(packet, data, len); packet[0] -= CTR_COOP_MONEY_FIRST_PACKET - CTR_COOP_INVENTORY_REQUEST;
    return CtrCoopInventory_HandlePacket(&sWallet, from, packet, len);
}
bool CtrCoopMoney_Active(void) { return sWallet.enabled; }
bool CtrCoopMoney_Ready(void) { return !sWallet.enabled || (CtrCoopInventory_IsReady(&sWallet) && !sReinstall); }
bool CtrCoopMoney_UsedLive(void) { return CtrCoopInventory_UsedLive(&sWallet); }
bool CtrCoopMoney_FinishBattle(void)
{ return !sWallet.enabled || (CtrCoopInventory_Reconcile(&sWallet) && CtrCoopInventory_Audit(&sWallet)); }
bool CtrCoopMoney_HasPending(void)
{ return sWallet.enabled && (sWallet.pending || CtrCoopInventory_HasOpenUse(&sWallet)); }
void CtrCoopMoney_Reset(void)
{
    (void)CtrCoopMoney_PrepareNativeLink();
}
bool CtrCoopMoney_CanPrepareNativeLink(void)
{
    return !sWallet.enabled || (!sWallet.pending && !sWallet.reconciling
        && !CtrCoopInventory_HasOpenUse(&sWallet) && (!sWallet.host || sWallet.cachedAcked));
}
bool CtrCoopMoney_PrepareNativeLink(void)
{
    if (sWallet.enabled && !CtrCoopInventory_Reset(&sWallet)) return false;
    sConnected = sGuest = sSaveApplied = sReinstall = false;
    sClientNonce = 0;
    return true;
}
void CtrCoopMoney_AfterStackReset(void)
{
    /* The entire game stack/world is gone. Neither role may carry its old
     * ledger into the next save; ordinary peer loss never calls this hook. */
    memset(&sWallet, 0, sizeof(sWallet));
    sConnected = sGuest = sSaveApplied = sReinstall = false;
    sClientNonce = 0;
}
bool CtrCoopMoney_HookRead(uint16_t currency, uint32_t *value)
{
    if (!Hooked() || !value || !Capacity(NULL, 0, currency)) return false;
    WaitReady(); *value = CtrCoopInventory_GetQuantity(&sWallet, 0, currency); return true;
}
bool CtrCoopMoney_BeginUse(uint16_t currency, uint32_t count)
{
    if (!Hooked()) return Read(NULL, 0, currency) >= count;
    WaitReady();
    if (sWallet.useCount)
    {
        if (sWallet.useItem == currency && sWallet.useCount == count) return true;
        CtrCoopMoney_RefundUse();
    }
    return CtrCoopInventory_ReserveUse(&sWallet, currency, count);
}
void CtrCoopMoney_RefundUse(void) { CtrCoopInventory_RefundUseState(&sWallet); }
bool CtrCoopMoney_Spend(uint16_t currency, uint32_t count)
{
    if (!Hooked()) return false;
    WaitReady();
    if (sWallet.useCount == count && sWallet.useItem == currency)
        return CtrCoopInventory_FinishUse(&sWallet, currency, count);
    return CtrCoopMoney_BeginUse(currency, count) && CtrCoopInventory_FinishUse(&sWallet, currency, count);
}
int CtrCoopMoney_HookChange(uint16_t currency, bool add, uint32_t count)
{
    uint32_t current, maximum = Capacity(NULL, 0, currency);
    if (!Hooked() || !maximum) return -1;
    WaitReady();
    if (!count) return 1;
    if (!add && sWallet.useItem == currency && sWallet.useCount == count)
        return CtrCoopInventory_FinishUse(&sWallet, currency, count);
    current = CtrCoopInventory_GetQuantity(&sWallet, 0, currency);
    if (add)
    {
        if (currency == CTR_COOP_MONEY_COINS && current >= maximum) return 0;
        if (count > maximum) count = maximum;
    }
    else
    {
        if (currency == CTR_COOP_MONEY_COINS && count > current) return 0;
        if (count > current) count = current;
        if (!count) return 1;
    }
    for (;;)
    {
        if (Wait(CtrCoopInventory_Change(&sWallet, 0, currency, add ? (int32_t)count : -(int32_t)count))) return 1;
        if (add || currency == CTR_COOP_MONEY_COINS) return 0;
        /* Native cash penalties clamp to zero. A concurrent purchase may
         * lower the host balance after this endpoint selected the amount. */
        current = CtrCoopInventory_GetQuantity(&sWallet, 0, currency);
        if (count > current) count = current; else return 0;
        if (!count) return 1;
    }
}
int CtrCoopMoney_HookSet(uint16_t currency, uint32_t value)
{
    uint32_t current;
    if (!CtrCoopMoney_HookRead(currency, &current)) return -1;
    uint32_t maximum = Capacity(NULL, 0, currency);
    if (value > maximum) value = maximum;
    return CtrCoopMoney_HookChange(currency, value >= current, value >= current ? value - current : current - value);
}
