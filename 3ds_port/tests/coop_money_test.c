#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/3ds_coop_money.c"

#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)
struct SaveBlock1 *gSaveBlock1Ptr;
struct SaveBlock2 *gSaveBlock2Ptr;
struct Main gMain;
uint32_t gBattleTypeFlags;
static bool doubleBattle;
bool CtrCoop_IsDoubleBattle(void) { return doubleBattle; }
typedef struct {
    CtrCoopInventory wallet;
    struct SaveBlock1 save;
    struct SaveBlock2 save2;
    bool connected, guest, saveApplied, reinstall;
    uint32_t nonce;
    uint16_t id;
} MoneyEndpoint;
typedef struct { uint16_t from,to,len; uint8_t data[32]; } MoneyMessage;
static MoneyEndpoint host, guest, *current;
static MoneyMessage queue[4096], lastMoneyRequest;
static unsigned head, tail, failures, pauses;
static uint32_t moneyClock, randomNonce;
static uint8_t moneySessionKind, moneySessionStatus;
static bool dropMoneyResult, recoverMoneyOnPause, racePurchase;

static void MoneyStore(void)
{
    if (!current) return;
    current->wallet=sWallet; current->connected=sConnected; current->guest=sGuest;
    current->saveApplied=sSaveApplied; current->reinstall=sReinstall; current->nonce=sClientNonce;
}
static void MoneySwitch(MoneyEndpoint *e)
{
    MoneyStore(); current=e; sWallet=e->wallet; sConnected=e->connected; sGuest=e->guest;
    sSaveApplied=e->saveApplied; sReinstall=e->reinstall; sClientNonce=e->nonce;
    gSaveBlock1Ptr=&e->save; gSaveBlock2Ptr=&e->save2;
}
uint16_t Random(void) { return ++randomNonce; }
uint32_t LocalLink_Milliseconds(void) { return moneyClock; }
uint8_t LocalLink_GetSession(void) { return moneySessionStatus; }
uint8_t LocalLink_GetSessionKind(void) { return moneySessionKind; }
bool LocalLink_TrySend(uint16_t to,const void *data,uint32_t len)
{
    const uint8_t *p=data;
    CHECK(len<=32 && p[0]>=80 && p[0]<=84);
    if (p[0]==80) lastMoneyRequest=(MoneyMessage){.from=current->id,.to=to,.len=len};
    if (p[0]==80) memcpy(lastMoneyRequest.data,p,len);
    if (dropMoneyResult && p[0]==81) return true;
    CHECK(tail<4096);
    if (tail>=4096 || len>32) exit(2);
    queue[tail]=(MoneyMessage){.from=current->id,.to=to,.len=len};
    memcpy(queue[tail++].data,p,len); return true;
}
void CtrPlatform_SleepUs(unsigned us) { (void)us; }
void CtrBottom_CoopSyncWait(void) { pauses++; CHECK(pauses<100); if(pauses>=100)exit(2); }
void CtrPlatform_CoopSyncWait(void) { if(recoverMoneyOnPause)dropMoneyResult=false; }
void CtrCoop_PumpInventory(void)
{
    MoneyEndpoint *origin=current; moneyClock+=5;
    if (racePurchase)
    {
        racePurchase=false; MoneySwitch(&host);
        CHECK(CtrCoopMoney_BeginUse(CTR_COOP_MONEY_CASH,700));
    }
    if (head<tail)
    {
        MoneyMessage m=queue[head++]; MoneySwitch(m.to==host.id?&host:&guest);
        CHECK(CtrCoopMoney_HandlePacket(m.from,m.data,m.len));
    }
    MoneySwitch(&host); CtrCoopMoney_Tick();
    MoneySwitch(&guest); CtrCoopMoney_Tick(); MoneySwitch(origin);
}
static void MoneyDrain(void)
{
    for(unsigned i=0;i<3000 && (head<tail || host.wallet.snapshotSending || guest.wallet.snapshotReceiving);++i)
        CtrCoop_PumpInventory();
    CHECK(head==tail); head=tail=0;
}
static uint32_t Cash(void) { return gSaveBlock1Ptr->money ^ gSaveBlock2Ptr->encryptionKey; }
static uint32_t Coins(void) { return gSaveBlock1Ptr->coins ^ (uint16_t)gSaveBlock2Ptr->encryptionKey; }
static void MoneySetup(uint32_t cash,uint16_t coins)
{
    current=NULL; memset(&host,0,sizeof(host)); memset(&guest,0,sizeof(guest));
    memset(&sWallet,0,sizeof(sWallet)); sConnected=sGuest=sSaveApplied=sReinstall=false; sClientNonce=0;
    head=tail=pauses=0; dropMoneyResult=recoverMoneyOnPause=racePurchase=false;
    host.id=11; guest.id=22; host.save2.encryptionKey=0x12345678; guest.save2.encryptionKey=0x98765432;
    host.save.money=cash^host.save2.encryptionKey; host.save.coins=coins^(uint16_t)host.save2.encryptionKey;
    guest.save.money=777^guest.save2.encryptionKey; guest.save.coins=3^(uint16_t)guest.save2.encryptionKey;
    gMain.inBattle=false; gBattleTypeFlags=0;
    moneySessionKind=LOCAL_KIND_COOP; moneySessionStatus=LOCAL_SESSION_IN;
    MoneySwitch(&host); CtrCoopMoney_Session(true,false,guest.id,true);
    MoneySwitch(&guest); CtrCoopMoney_Session(true,true,host.id,true);
    MoneyDrain();
    CHECK(CtrCoopMoney_Ready() && Cash()==cash && Coins()==coins);
    MoneySwitch(&host); CHECK(CtrCoopMoney_Ready()); MoneySwitch(&guest);
}

static void TestRacingPurchasesAndRefund(void)
{
    MoneySetup(1000,100); racePurchase=true;
    CHECK(!CtrCoopMoney_BeginUse(1,700));
    CHECK(Cash()==300); /* host reservation won before guest request */
    MoneySwitch(&host); CHECK(CtrCoopMoney_Spend(1,700)); CHECK(Cash()==300);
    MoneyDrain(); MoneySwitch(&guest);
    CHECK(CtrCoopMoney_BeginUse(1,250)); CHECK(Cash()==50);
    CtrCoopMoney_RefundUse(); CHECK(Cash()==300); /* full bag/no effect */
    CtrCoopMoney_RefundUse(); CHECK(Cash()==300);
    CHECK(CtrCoopMoney_BeginUse(1,200)); CHECK(CtrCoopMoney_HookChange(1,false,200)==1);
    CHECK(Cash()==100); MoneyDrain(); MoneySwitch(&host); CHECK(Cash()==100);
}
static void TestRetryAndSeparateSingleton(void)
{
    MoneySetup(200000,20);
    CtrCoopInventory dummy={0}; CtrCoopInventory_Bind(&dummy);
    dropMoneyResult=recoverMoneyOnPause=true;
    CHECK(CtrCoopMoney_BeginUse(1,100000)); CHECK(pauses>0 && Cash()==100000);
    MoneyMessage duplicate=lastMoneyRequest;
    CHECK(CtrCoopMoney_Spend(1,100000)); MoneyDrain();
    MoneySwitch(&host); uint32_t before=Cash();
    CtrCoopMoney_HandlePacket(duplicate.from,duplicate.data,duplicate.len);
    CHECK(Cash()==before); /* old reservation cannot debit twice */
    CHECK(CtrCoopInventory_HookChange(0,1,1)==-1); /* money did not rebind items */
    CtrCoopInventory_Bind(NULL); MoneyDrain();
    MoneySwitch(&guest); dropMoneyResult=recoverMoneyOnPause=true;
    CHECK(CtrCoopMoney_HookChange(1,true,1234)==1 && Cash()==101234);
    duplicate=lastMoneyRequest; MoneyDrain(); MoneySwitch(&host);
    before=Cash(); CtrCoopMoney_HandlePacket(duplicate.from,duplicate.data,duplicate.len);
    CHECK(Cash()==before && before==101234); MoneyDrain();
}
static void TestGainsCapsDisconnectAndRekey(void)
{
    MoneySetup(999999,9998);
    CHECK(CtrCoopMoney_BeginUse(1,50000)); MoneyDrain(); MoneySwitch(&host);
    CtrCoopMoney_Session(false,false,0,true);
    CHECK(CtrCoopMoney_HookChange(1,true,100000)==1 && Cash()==949999);
    CHECK(CtrCoopMoney_HookChange(2,true,100)==1 && Coins()==9999);
    CHECK(CtrCoopMoney_HookChange(1,false,100)==1 && Cash()==949899);
    CtrCoopMoney_Session(true,false,guest.id,true); MoneyDrain(); MoneySwitch(&guest);
    CtrCoopMoney_RefundUse(); CHECK(Cash()==999899);
    CHECK(CtrCoopMoney_HookChange(1,true,0xFFFFFFFF)==1 && Cash()==999999);
    MoneyDrain();
    uint32_t value=Cash(), oldKey=gSaveBlock2Ptr->encryptionKey;
    gSaveBlock2Ptr->encryptionKey^=0xABCDEF01;
    gSaveBlock1Ptr->money^=oldKey^gSaveBlock2Ptr->encryptionKey;
    gSaveBlock1Ptr->coins^=(uint16_t)(oldKey^gSaveBlock2Ptr->encryptionKey);
    uint32_t revision=sWallet.revision;
    CHECK(CtrCoopMoney_FinishBattle() && Cash()==value && sWallet.revision==revision);
    CHECK(CtrCoopMoney_HookSet(1,value/2)==1 && Cash()==value/2); /* whiteout */
    CHECK(CtrCoopMoney_FinishBattle()); MoneyDrain();
    MoneySwitch(&host); CHECK(Cash()==value/2);
}
static void TestSaveCopyAndTemporaryBattle(void)
{
    MoneySetup(4000,150); CHECK(CtrCoopMoney_HookChange(1,true,321)==1); MoneyDrain();
    uint32_t revision=sWallet.revision;
    gSaveBlock1Ptr->money=777^gSaveBlock2Ptr->encryptionKey;
    CtrCoopMoney_AfterSaveCopy(); CtrCoopMoney_Tick();
    CHECK(Cash()==4321 && sWallet.revision==revision && CtrCoopMoney_Ready());
    gMain.inBattle=true; gBattleTypeFlags=BATTLE_TYPE_WALLY_TUTORIAL;
    CHECK(CtrCoopMoney_HookChange(1,true,100)==-1);
    gMain.inBattle=false; gBattleTypeFlags=0;
    CHECK(CtrCoopMoney_FinishBattle() && sWallet.revision==revision);
    uint8_t unrelated[]={65,16}; CHECK(!CtrCoopMoney_HandlePacket(host.id,unrelated,2));
    uint8_t truncated[]={80,16}; CHECK(CtrCoopMoney_HandlePacket(host.id,truncated,2));
}
static void TestNativePreflightAndStackReset(void)
{
    MoneySetup(1000,100); MoneySwitch(&host);
    sWallet.pending=true;
    CtrCoopInventory original=sWallet; uint32_t nonce=sClientNonce;
    CHECK(!CtrCoopMoney_CanPrepareNativeLink() && !CtrCoopMoney_PrepareNativeLink() && !memcmp(&original,&sWallet,sizeof(original))
        && sClientNonce==nonce && sConnected);
    sWallet.pending=false;
    sWallet.cachedAcked=false; original=sWallet;
    CHECK(!CtrCoopMoney_CanPrepareNativeLink() && !CtrCoopMoney_PrepareNativeLink()
        && !memcmp(&original,&sWallet,sizeof(original)));
    sWallet.cachedAcked=true;
    CHECK(CtrCoopMoney_BeginUse(1,300)); original=sWallet;
    CHECK(!CtrCoopMoney_PrepareNativeLink() && Cash()==700
        && !memcmp(&original,&sWallet,sizeof(original)));
    CtrCoopMoney_Session(false,false,0,true); /* ordinary peer loss retains host world */
    CHECK(!memcmp(&original,&sWallet,sizeof(original)) && Cash()==700 && sClientNonce==nonce);
    CtrCoopMoney_RefundUse(); MoneyDrain();
    sReinstall=true;
    CHECK(CtrCoopMoney_CanPrepareNativeLink() && CtrCoopMoney_PrepareNativeLink() && !sWallet.enabled && !sConnected
        && !sGuest && !sSaveApplied && !sReinstall && !sClientNonce);
    CHECK(CtrCoopMoney_PrepareNativeLink());
    moneySessionKind=1; /* Union Room/Berry Crush native session */
    gSaveBlock1Ptr->money=888^gSaveBlock2Ptr->encryptionKey;
    CtrCoopMoney_Tick(); CHECK(Cash()==888 && CtrCoopMoney_HookChange(1,true,1)==-1);

    MoneySetup(1000,100); MoneySwitch(&host); moneySessionKind=1;
    uint32_t value=0;
    CHECK(!CtrCoopMoney_HookRead(1,&value) && CtrCoopMoney_HookSet(1,99)==-1
        && CtrCoopMoney_HookChange(2,false,1)==-1);

    MoneySetup(1000,100); CHECK(CtrCoopMoney_BeginUse(1,300)); MoneyDrain();
    nonce=sClientNonce; sWallet.pending=sWallet.reconciling=true;
    unsigned packets=tail;
    CtrCoopMoney_AfterStackReset();
    CHECK(!sWallet.enabled && !sWallet.pending && !sWallet.useCount && !sWallet.reconciling
        && !sClientNonce && !sConnected && Cash()==700 && tail==packets);
    MoneySwitch(&host);
    CHECK(Cash()==700 && sWallet.hostUseCount[1]==300); /* no synthetic refund */
    MoneySwitch(&guest); CtrCoopMoney_Session(true,true,host.id,true); MoneyDrain();
    CHECK(sClientNonce && sClientNonce!=nonce && Cash()==700 && CtrCoopMoney_Ready());
    MoneySwitch(&host); CHECK(Cash()==700 && !sWallet.hostUseCount[1]);
    CtrCoopMoney_Reset(); CHECK(!sClientNonce && !CtrCoopMoney_Active());

    MoneySetup(1000,100); MoneySwitch(&host);
    CHECK(CtrCoopMoney_BeginUse(1,300));
    nonce=sClientNonce; sWallet.pending=sWallet.reconciling=true; sWallet.cachedAcked=false;
    packets=tail;
    CtrCoopMoney_AfterStackReset(); /* a real host reset also abandons its old world */
    CHECK(!sWallet.enabled && !sWallet.pending && !sWallet.useCount && !sWallet.reconciling
        && !sWallet.hostUseCount[0] && !sWallet.hostUseCount[1] && !sClientNonce
        && !sConnected && !sGuest && !sSaveApplied && !sReinstall && Cash()==700 && tail==packets);
    gSaveBlock1Ptr->money=2222^gSaveBlock2Ptr->encryptionKey;
    gSaveBlock1Ptr->coins=42^(uint16_t)gSaveBlock2Ptr->encryptionKey;
    CtrCoopMoney_Tick(); CHECK(Cash()==2222 && Coins()==42);
    CtrCoopMoney_Session(true,false,guest.id,true);
    CHECK(sClientNonce && sClientNonce!=nonce && sWallet.quantity[0][1]==2222
        && sWallet.quantity[0][2]==42); /* import new save, never restore the old ledger */
}
int main(void)
{
    TestRacingPurchasesAndRefund(); TestRetryAndSeparateSingleton();
    TestGainsCapsDisconnectAndRekey(); TestSaveCopyAndTemporaryBattle();
    TestNativePreflightAndStackReset();
    gMain.inBattle=true;gBattleTypeFlags=BATTLE_TYPE_LINK;
    CHECK(!Canonical(NULL));doubleBattle=true;CHECK(Canonical(NULL));doubleBattle=false;
    gMain.inBattle=false;gBattleTypeFlags=0;
    printf("shared wallet regressions: %u failures\n",failures);
    return failures?1:0;
}
