#include "global.h"
#include "3ds_coop_inventory.h"
#include "3ds_coop_inventory_bridge.h"
#include "3ds_local.h"
#include "3ds_platform.h"
#include "3ds_bottom.h"
#include "item.h"
#include "battle.h"
#include "main.h"
#include "battle_pyramid.h"
#include "constants/local_link.h"
#include "random.h"
#include <limits.h>
#include <string.h>

static CtrCoopInventory sInventory;
static bool sConnected, sGuest, sSaveApplied, sReinstall;
static bool sKnownPeer, sResuming, sWantResume;
static uint32_t sClientNonce;
static struct SaveBlock1 sInventoryBackup;

static struct ItemSlot *Pocket(uint8_t scope, unsigned index, unsigned *count)
{
    if (scope == CTR_COOP_INVENTORY_PC)
    {
        *count = index == 0 ? PC_ITEMS_COUNT : 0;
        return gSaveBlock1Ptr->pcItems;
    }
    switch (index)
    {
    case 0: *count=BAG_ITEMS_COUNT; return gSaveBlock1Ptr->bagPocket_Items;
    case 1: *count=BAG_KEYITEMS_COUNT; return gSaveBlock1Ptr->bagPocket_KeyItems;
    case 2: *count=BAG_POKEBALLS_COUNT; return gSaveBlock1Ptr->bagPocket_PokeBalls;
    case 3: *count=BAG_TMHM_COUNT; return gSaveBlock1Ptr->bagPocket_TMHM;
    case 4: *count=BAG_BERRIES_COUNT; return gSaveBlock1Ptr->bagPocket_Berries;
    default: *count=0; return NULL;
    }
}

static uint32_t Read(void *user, uint8_t scope, uint16_t item)
{
    uint32_t total=0;
    (void)user;
    if (!gSaveBlock1Ptr || !gSaveBlock2Ptr) return 0;
    for (unsigned p=0;;++p)
    {
        unsigned count;
        struct ItemSlot *slots=Pocket(scope,p,&count);
        if (!count) break;
        for (unsigned i=0;i<count;++i)
            if (slots[i].itemId==item)
                total += slots[i].quantity ^ (scope==CTR_COOP_INVENTORY_BAG ? (uint16_t)gSaveBlock2Ptr->encryptionKey : 0);
    }
    return total;
}

/* Called with the inventory module's applying guard held. Native item hooks
 * fall through rather than making a second request for this materialization. */
static bool WriteRaw(uint8_t scope, uint16_t item, uint32_t quantity)
{
    struct ItemSlot candidate[BAG_TMHM_COUNT > PC_ITEMS_COUNT ? BAG_TMHM_COUNT : PC_ITEMS_COUNT];
    struct ItemSlot *slots;
    unsigned index=0,count;
    uint16_t key=scope==CTR_COOP_INVENTORY_BAG ? (uint16_t)gSaveBlock2Ptr->encryptionKey : 0;
    uint16_t capacity=MAX_PC_ITEM_CAPACITY;
    bool single=false;
    if (!item || item>=ITEMS_COUNT || scope>=CTR_COOP_INVENTORY_SCOPES) return false;
    if (scope==CTR_COOP_INVENTORY_BAG)
    {
        switch (GetItemPocket(item))
        {
        case POCKET_ITEMS: index=0;break;
        case POCKET_KEY_ITEMS: index=1;break;
        case POCKET_POKE_BALLS: index=2;break;
        case POCKET_TM_HM: index=3;single=true;break;
        case POCKET_BERRIES: index=4;single=true;break;
        default:return false;
        }
        capacity=index==4 ? MAX_BERRY_CAPACITY : MAX_BAG_ITEM_CAPACITY;
    }
    if (single && quantity>capacity) return false;
    slots=Pocket(scope,index,&count);
    if (count>ARRAY_COUNT(candidate)) return false;
    memcpy(candidate,slots,count*sizeof(*slots));
    /* Preserve unrelated slot order. Reuse this item's slots before empty
     * ones; never dispatch into a temporary Pyramid/tutorial container. */
    for (unsigned pass=0;pass<2;++pass)
        for (unsigned i=0;i<count;++i)
        {
            if ((pass==0 && candidate[i].itemId!=item)
             || (pass==1 && candidate[i].itemId!=ITEM_NONE)) continue;
            uint16_t amount=quantity>capacity ? capacity : (uint16_t)quantity;
            candidate[i].itemId=amount ? item : ITEM_NONE;
            candidate[i].quantity=amount^key;
            quantity-=amount;
        }
    if (quantity) return false;
    memcpy(slots,candidate,count*sizeof(*slots));
    return true;
}

static bool Write(void *user,uint8_t scope,uint16_t item,uint32_t quantity)
{
    (void)user;
    if (!gSaveBlock1Ptr || !gSaveBlock2Ptr) return false;
    sInventoryBackup=*gSaveBlock1Ptr;
    if (WriteRaw(scope,item,quantity))
    {
        CtrCoop_EngineInventoryChanged();
        return true;
    }
    *gSaveBlock1Ptr=sInventoryBackup;
    return false;
}

/* Validate the canonical ledger without reading or modifying a temporary
 * tutorial/Pyramid bag. Capacity is shared by every item in a pocket. */
static bool Validate(void *user,const uint32_t quantities[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS],uint16_t items)
{
    const unsigned limits[6]={BAG_ITEMS_COUNT,BAG_KEYITEMS_COUNT,BAG_POKEBALLS_COUNT,
        BAG_TMHM_COUNT,BAG_BERRIES_COUNT,PC_ITEMS_COUNT};
    unsigned used[6]={0};
    (void)user;
    if (!items || items>ITEMS_COUNT || quantities[0][0] || quantities[1][0]) return false;
    for (uint16_t item=1;item<items;++item)
    {
        unsigned index;
        switch (GetItemPocket(item))
        {
        case POCKET_ITEMS:index=0;break;
        case POCKET_KEY_ITEMS:index=1;break;
        case POCKET_POKE_BALLS:index=2;break;
        case POCKET_TM_HM:index=3;break;
        case POCKET_BERRIES:index=4;break;
        default:
            if (quantities[0][item] || quantities[1][item]) return false;
            continue;
        }
        uint32_t bag=quantities[0][item],pc=quantities[1][item];
        unsigned capacity=index==4 ? MAX_BERRY_CAPACITY : MAX_BAG_ITEM_CAPACITY;
        if (index>=3 && bag>capacity) return false;
        if (bag) used[index]+=1+(bag-1)/capacity;
        if (pc) used[5]+=1+(pc-1)/MAX_PC_ITEM_CAPACITY;
        if (used[index]>limits[index] || used[5]>limits[5]) return false;
    }
    return true;
}

static bool Install(void *user,const uint32_t quantities[CTR_COOP_INVENTORY_SCOPES][CTR_COOP_INVENTORY_MAX_ITEMS],uint16_t items)
{
    (void)user;
    if (!gSaveBlock1Ptr || !gSaveBlock2Ptr || !Validate(user,quantities,items)) return false;
    sInventoryBackup=*gSaveBlock1Ptr;
    for (uint8_t scope=0;scope<CTR_COOP_INVENTORY_SCOPES;++scope)
        for (unsigned p=0;;++p)
        {
            unsigned count;
            struct ItemSlot *slots=Pocket(scope,p,&count);
            if (!count) break;
            for (unsigned i=0;i<count;++i)
            {
                slots[i].itemId=ITEM_NONE;
                slots[i].quantity=scope==CTR_COOP_INVENTORY_BAG ? (uint16_t)gSaveBlock2Ptr->encryptionKey : 0;
            }
        }
    for (uint8_t scope=0;scope<CTR_COOP_INVENTORY_SCOPES;++scope)
        for (uint16_t item=1;item<items;++item)
            if (quantities[scope][item] && !WriteRaw(scope,item,quantities[scope][item]))
            {
                *gSaveBlock1Ptr=sInventoryBackup;
                return false;
            }
    CtrCoop_EngineInventoryChanged();
    return true;
}

static bool Send(void *user,uint16_t peer,const uint8_t *data,uint16_t size)
{ (void)user; return sConnected && LocalLink_TrySend(peer,data,size); }
static uint32_t Clock(void *user) { (void)user; return LocalLink_Milliseconds(); }
static void Pump(void *user) { (void)user; CtrCoop_PumpInventory(); CtrPlatform_SleepUs(1000); }
static void Pause(void *user)
{
    (void)user;
    CtrBottom_CoopSyncWait();
    CtrPlatform_CoopSyncWait();
    CtrCoop_PumpInventory();
}
static bool Canonical(void *user)
{
    (void)user;
    return gSaveBlock1Ptr && gSaveBlock2Ptr && !InBattlePyramid_()
        && !(LocalLink_GetSession()==LOCAL_SESSION_IN && LocalLink_GetSessionKind()!=LOCAL_KIND_COOP)
        && !(gMain.inBattle && (gBattleTypeFlags & (BATTLE_TYPE_WALLY_TUTORIAL | BATTLE_TYPE_RECORDED | BATTLE_TYPE_LINK)));
}

void CtrCoopInventoryBridge_Session(bool active,bool guest,uint16_t peer,bool saveApplied)
{
    bool rebound=active && (!sConnected || sInventory.peer!=peer);
    if (rebound) { sKnownPeer=false; sResuming=false; sWantResume=guest && sInventory.enabled && sInventory.ready; }
    sConnected=active;
    sSaveApplied=saveApplied;
    if (!active || !peer) return;
    if (sInventory.enabled && sGuest!=guest)
    {
        if (!CtrCoopInventory_Reset(&sInventory)) return;
        CtrCoopInventory_Bind(NULL);
    }
    sGuest=guest;
    if (!sInventory.enabled)
    {
        const CtrCoopInventoryIO io={.read=Read,.write=Write,.install=Install,.send=Send,
            .pump=Pump,.clock=Clock,.pause=Pause,.canonical=Canonical,.validate=Validate};
        uint32_t nonce=((uint32_t)Random()<<16)^Random()^LocalLink_Milliseconds();
        if (!nonce) nonce=1;
        if (!sClientNonce) sClientNonce=nonce;
        if (!CtrCoopInventory_Init(&sInventory,&io,!guest,peer,guest?0:nonce,ITEMS_COUNT)) return;
        CtrCoopInventory_Bind(&sInventory);
        if (guest && !saveApplied) CtrCoopInventory_SetViewActive(&sInventory,false);
        rebound=true;
    }
    if (rebound)
    {
        CtrCoopInventory_RebindPeer(&sInventory,peer);
        if (guest) CtrCoopInventory_Hello(&sInventory,sClientNonce);
    }
    if (guest && saveApplied && !sInventory.viewActive && !sInventory.pending)
        CtrCoopInventory_SetViewActive(&sInventory,true);
}

void CtrCoopInventoryBridge_AfterSaveCopy(void) { sReinstall=true; }
void CtrCoopInventoryBridge_Tick(void)
{
    if (!sInventory.enabled) return;
    if (sReinstall && sSaveApplied && sInventory.ready && !sInventory.pending
     && !CtrCoopInventory_HasOpenUse(&sInventory))
    {
        if (CtrCoopInventory_SetViewActive(&sInventory,false)
         && CtrCoopInventory_SetViewActive(&sInventory,true)) sReinstall=false;
    }
    CtrCoopInventory_Tick(&sInventory);
}
bool CtrCoopInventoryBridge_HandlePacket(uint16_t from,const uint8_t *data,uint16_t len)
{
    if (!data || len<2) return false;
    uint32_t previous=sInventory.clientNonce, epoch=sInventory.epoch, nonce=0, wireEpoch=0;
    bool hello=len==11 && data[0]==CTR_COOP_INVENTORY_CONTROL && (data[10]==4 || data[10]==6);
    if (len>=10)
        for (unsigned i=0;i<4;++i)
        {
            wireEpoch|=(uint32_t)data[2+i]<<(8*i);
            nonce|=(uint32_t)data[6+i]<<(8*i);
        }
    bool handled=CtrCoopInventory_HandlePacket(&sInventory,from,data,len);
    if (sConnected && from==sInventory.peer && data[1]==CTR_COOP_INVENTORY_VERSION)
    {
        if (!sGuest && hello && nonce && sInventory.clientNonce==nonce
         && (!wireEpoch || wireEpoch==epoch))
        {
            sKnownPeer=true;
            sResuming=data[10]==6 && previous==nonce && epoch==wireEpoch;
        }
        if (sGuest && len==11 && data[0]==CTR_COOP_INVENTORY_CONTROL && data[10]==5
         && nonce==sClientNonce && wireEpoch==sInventory.epoch)
        {
            sKnownPeer=true; sResuming=sWantResume;
        }
    }
    return handled;
}
bool CtrCoopInventoryBridge_Active(void) { return CtrCoopInventory_IsActive(&sInventory); }
bool CtrCoopInventoryBridge_Ready(void) { return !sInventory.enabled || (CtrCoopInventory_IsReady(&sInventory) && !sReinstall); }
bool CtrCoopInventoryBridge_FinishBattle(void)
{ return !sInventory.enabled || (CtrCoopInventory_Reconcile(&sInventory) && CtrCoopInventory_Audit(&sInventory)); }
bool CtrCoopInventoryBridge_HasPending(void)
{ return sInventory.enabled && (sInventory.pending || CtrCoopInventory_HasOpenUse(&sInventory)); }
bool CtrCoopInventoryBridge_KnownPeer(void) { return sKnownPeer; }
bool CtrCoopInventoryBridge_IsResuming(void) { return sResuming; }
void CtrCoopInventoryBridge_Reset(void)
{
    if (CtrCoopInventory_Reset(&sInventory))
    {
        CtrCoopInventory_Bind(NULL); sConnected=false; sSaveApplied=false; sReinstall=false;
        sKnownPeer=sResuming=sWantResume=false;
        sClientNonce=0;
    }
}

bool CtrCoopInventoryBridge_PrepareNativeLink(void)
{
    CtrCoopInventoryBridge_Reset();
    return !sInventory.enabled;
}

bool CtrCoopInventoryBridge_CanPrepareNativeLink(void)
{
    return !sInventory.enabled || (!sInventory.pending && !sInventory.reconciling
        && !sInventory.useCount && !sInventory.hostUseCount[0] && !sInventory.hostUseCount[1]
        && (!sInventory.host || sInventory.cachedAcked));
}

void CtrCoopInventoryBridge_AfterStackReset(void)
{
    /* ALL old engine/effect callbacks have been abandoned. This process is
     * resetting its entire world, so its ledger must not cross into another
     * save. A host whose GUEST disconnected never runs this reset boundary:
     * that continuing host retains its committed decisions in ClearRemote. */
    CtrCoopInventory_Bind(NULL);
    memset(&sInventory,0,sizeof(sInventory));
    sConnected=sSaveApplied=sReinstall=false;
    sKnownPeer=sResuming=sWantResume=false;
    sClientNonce=0;
}
