"""Test the actual root inventory bridge with real engine structs and item metadata.

Production code is included unchanged, not copied into a stub adapter. Only its
external transport/UI/clock are simulated. MinGW gcc and Python suffice.
"""
from pathlib import Path
import argparse
import re
import subprocess
from test_coop_bike_hooks import UPSTREAM

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default="gcc")
    cc = parser.parse_args().cc
    data = (UPSTREAM / "src/data/items.h").read_text(encoding="utf-8")
    pockets = re.findall(r"\[(ITEM_\w+)\]\s*=\s*\{.*?\.pocket\s*=\s*(POCKET_\w+)", data, re.S)
    metadata = "static const u8 pockets[ITEMS_COUNT]={\n" + "\n".join(
        f"[{item}]={pocket}," for item, pocket in pockets) + "\n};\n"
    source = r'''
#include <stdlib.h>
#include "3ds_port/src/3ds_coop_inventory_bridge.c"
#include "constants/local_link.h"
#include <stdio.h>
static struct SaveBlock1 save1, before;
static struct SaveBlock2 save2;
struct SaveBlock1 *gSaveBlock1Ptr=&save1;
struct SaveBlock2 *gSaveBlock2Ptr=&save2;
struct Main gMain;
u32 gBattleTypeFlags;
static unsigned dirty, failures, now, session=LOCAL_SESSION_IN, kind=LOCAL_KIND_COOP;
static bool pyramid;
static bool doubleBattle;
bool CtrCoop_IsDoubleBattle(void) { return doubleBattle; }
static CtrCoopInventory remote;
static uint32_t remoteQty[2][CTR_COOP_INVENTORY_MAX_ITEMS];
static uint32_t ledger[2][CTR_COOP_INVENTORY_MAX_ITEMS];
static struct { bool toBridge; uint16_t size; uint8_t bytes[32]; } queue[16384];
static unsigned head, tail;
#define CHECK(x) do { if (!(x)) {printf("FAIL %d: %s\n",__LINE__,#x); ++failures;} } while(0)
METADATA
u8 GetItemPocket(u16 item) { return item<ITEMS_COUNT ? pockets[item] : 0; }
bool8 InBattlePyramid_(void) { return pyramid; }
u16 Random(void) { static u16 n=19; return ++n; }
uint8_t LocalLink_GetSession(void) { return session; }
uint8_t LocalLink_GetSessionKind(void) { return kind; }
uint32_t LocalLink_Milliseconds(void) { return now; }
void CtrCoop_EngineInventoryChanged(void) { ++dirty; }
void CtrPlatform_SleepUs(unsigned us) { (void)us; ++now; }
void CtrBottom_CoopSyncWait(void) {}
void CtrPlatform_CoopSyncWait(void) {}
static bool Enqueue(bool toBridge,const uint8_t *p,uint16_t n)
{
    if(tail>=ARRAY_COUNT(queue) || n>32) abort();
    queue[tail].toBridge=toBridge;queue[tail].size=n;memcpy(queue[tail++].bytes,p,n);return true;
}
bool LocalLink_TrySend(uint16_t peer,const void *p,uint32_t n)
{ (void)peer;return Enqueue(false,p,n); }
static bool RSend(void *u,uint16_t peer,const uint8_t *p,uint16_t n)
{ (void)u;(void)peer;return Enqueue(true,p,n); }
static uint32_t RRead(void *u,uint8_t scope,uint16_t item)
{ (void)u;return remoteQty[scope][item]; }
static bool RWrite(void *u,uint8_t scope,uint16_t item,uint32_t q)
{ (void)u;remoteQty[scope][item]=q;return true; }
static bool RInstall(void *u,const uint32_t q[2][512],uint16_t items)
{ (void)u;(void)items;memcpy(remoteQty,q,sizeof(remoteQty));return true; }
static uint32_t RClock(void *u) { (void)u;return now; }
void CtrCoop_PumpInventory(void)
{
    unsigned end=tail;
    while(head<end) {
        unsigned i=head++;
        if(queue[i].toBridge) CtrCoopInventoryBridge_HandlePacket(2,queue[i].bytes,queue[i].size);
        else CtrCoopInventory_HandlePacket(&remote,1,queue[i].bytes,queue[i].size);
    }
    ++now;
}
static void RPump(void *u) { (void)u;CtrCoop_PumpInventory(); }
static bool RCanonical(void *u) { (void)u;return true; }
static void Drive(unsigned ticks)
{
    for(unsigned i=0;i<ticks;++i) {
        CtrCoopInventoryBridge_Tick();CtrCoopInventory_Tick(&remote);CtrCoop_PumpInventory();++now;
    }
}
static void Clear(void)
{
    CtrCoopInventory_Bind(NULL);memset(&sInventory,0,sizeof(sInventory));
    sConnected=sGuest=sSaveApplied=sReinstall=sKnownPeer=sResuming=sWantResume=false;sClientNonce=0;
    memset(&save1,0,sizeof(save1));memset(&save2,0,sizeof(save2));save2.encryptionKey=0x1234ABCD;
    memset(&remote,0,sizeof(remote));memset(remoteQty,0,sizeof(remoteQty));memset(ledger,0,sizeof(ledger));
    memset(&gMain,0,sizeof(gMain));gBattleTypeFlags=0;session=LOCAL_SESSION_IN;kind=LOCAL_KIND_COOP;pyramid=false;
    head=tail=dirty=0;
    for(unsigned p=0;p<5;++p) {unsigned n;struct ItemSlot *s=Pocket(0,p,&n);for(unsigned i=0;i<n;++i)s[i].quantity=(u16)save2.encryptionKey;}
}
static void RemoteInit(bool host,uint32_t epoch)
{
    CtrCoopInventoryIO io={.read=RRead,.write=RWrite,.install=RInstall,.send=RSend,.pump=RPump,.clock=RClock,.pause=RPump,.canonical=RCanonical};
    CHECK(CtrCoopInventory_Init(&remote,&io,host,1,epoch,ITEMS_COUNT));
}
static void Adapter(void)
{
    Clear();
    CHECK(Write(NULL,0,ITEM_POTION,201));CHECK(dirty==1);
    CHECK(Read(NULL,0,ITEM_POTION)==201);CHECK(Read(NULL,1,ITEM_POTION)==0);
    CHECK(save1.bagPocket_Items[0].quantity==(99^(u16)save2.encryptionKey));
    CHECK(Write(NULL,1,ITEM_POTION,2001));CHECK(Read(NULL,1,ITEM_POTION)==2001);
    CHECK(save1.pcItems[0].quantity==999);CHECK(Read(NULL,0,ITEM_POTION)==201);
    CHECK(Write(NULL,0,ITEM_ORAN_BERRY,999));CHECK(Write(NULL,0,ITEM_TM01,99));
    before=save1;unsigned d=dirty;
    CHECK(!Write(NULL,0,ITEM_ORAN_BERRY,1000));CHECK(!memcmp(&before,&save1,sizeof(save1)));CHECK(dirty==d);
    CHECK(!Write(NULL,0,ITEM_TM01,100));CHECK(!memcmp(&before,&save1,sizeof(save1)));
    CHECK(!Write(NULL,0,ITEM_POTION,UINT32_MAX));CHECK(!memcmp(&before,&save1,sizeof(save1)));
    CHECK(!Write(NULL,1,ITEM_POTION,UINT32_MAX));CHECK(!memcmp(&before,&save1,sizeof(save1)));
    CHECK(!Write(NULL,2,ITEM_POTION,1));CHECK(!Write(NULL,0,ITEMS_COUNT,1));
    u16 old=(u16)save2.encryptionKey,newkey=0x42B7;
    for(unsigned p=0;p<5;++p) {unsigned n;struct ItemSlot *s=Pocket(0,p,&n);for(unsigned i=0;i<n;++i)s[i].quantity^=old^newkey;}
    save2.encryptionKey=0xCAFE0000|newkey;
    CHECK(Read(NULL,0,ITEM_POTION)==201);CHECK(Read(NULL,1,ITEM_POTION)==2001);
    CHECK(Write(NULL,0,ITEM_POTION,2));CHECK(Read(NULL,0,ITEM_POTION)==2);
    ledger[0][ITEM_POTION]=2970;ledger[1][ITEM_ORAN_BERRY]=49950;
    before=save1;d=dirty;CHECK(Validate(NULL,ledger,ITEMS_COUNT));
    CHECK(!memcmp(&before,&save1,sizeof(save1)));CHECK(dirty==d);
    ledger[0][ITEM_ANTIDOTE]=1;CHECK(!Validate(NULL,ledger,ITEMS_COUNT));
    CHECK(!Install(NULL,ledger,ITEMS_COUNT));CHECK(!memcmp(&before,&save1,sizeof(save1)));CHECK(dirty==d);
    ledger[0][ITEM_ANTIDOTE]=0;ledger[1][ITEM_ORAN_BERRY]++;
    CHECK(!Validate(NULL,ledger,ITEMS_COUNT));ledger[1][ITEM_ORAN_BERRY]=999;
    CHECK(Install(NULL,ledger,ITEMS_COUNT));CHECK(dirty==d+1);
    CHECK(Read(NULL,0,ITEM_POTION)==2970);CHECK(Read(NULL,1,ITEM_ORAN_BERRY)==999);
    CHECK(Read(NULL,1,ITEM_POTION)==0);CHECK(Read(NULL,0,ITEM_ORAN_BERRY)==0);CHECK(Read(NULL,0,ITEM_TM01)==0);
    for(unsigned i=1;i<PC_ITEMS_COUNT;++i)CHECK(save1.pcItems[i].quantity==0);
    CHECK(!Validate(NULL,ledger,ITEMS_COUNT+1));CHECK(!Validate(NULL,ledger,0));
    ledger[0][0]=1;CHECK(!Validate(NULL,ledger,ITEMS_COUNT));ledger[0][0]=0;
    CHECK(Canonical(NULL));gMain.inBattle=true;CHECK(Canonical(NULL));
    gBattleTypeFlags=BATTLE_TYPE_WALLY_TUTORIAL;CHECK(!Canonical(NULL));
    gBattleTypeFlags=BATTLE_TYPE_LINK;CHECK(!Canonical(NULL));doubleBattle=true;CHECK(Canonical(NULL));doubleBattle=false;
    gBattleTypeFlags=BATTLE_TYPE_RECORDED;CHECK(!Canonical(NULL));
    gMain.inBattle=false;pyramid=true;CHECK(!Canonical(NULL));pyramid=false;
    kind=LOCAL_KIND_UNION_ROOM;CHECK(!Canonical(NULL));kind=LOCAL_KIND_COOP;CHECK(Canonical(NULL));
}
static void HostHandshake(void)
{
    Clear();CHECK(Write(NULL,0,ITEM_POTION,5));
    CtrCoopInventoryBridge_Session(true,false,2,true);RemoteInit(false,0);
    CHECK(CtrCoopInventory_Hello(&remote,111));Drive(150);
    CHECK(CtrCoopInventoryBridge_KnownPeer());CHECK(!CtrCoopInventoryBridge_IsResuming());
    CHECK(remote.ready);CHECK(remoteQty[0][ITEM_POTION]==5);
    uint32_t epoch=sInventory.epoch;
    CtrCoopInventoryBridge_Session(false,false,0,true);CtrCoopInventoryBridge_Session(true,false,2,true);
    CHECK(CtrCoopInventory_Hello(&remote,111));Drive(3);
    CHECK(CtrCoopInventoryBridge_IsResuming());CHECK(sInventory.epoch==epoch);
    memset(&remote,0,sizeof(remote));RemoteInit(false,0);CHECK(CtrCoopInventory_Hello(&remote,222));Drive(150);
    CHECK(CtrCoopInventoryBridge_KnownPeer());CHECK(!CtrCoopInventoryBridge_IsResuming());
    CHECK(sInventory.epoch!=epoch);CHECK(remote.ready);CHECK(remoteQty[0][ITEM_POTION]==5);
    CHECK(sInventory.lastRequest==0);
}
static void GuestHandshake(void)
{
    Clear();remoteQty[0][ITEM_POTION]=7;RemoteInit(true,0x87654321);
    CtrCoopInventoryBridge_Session(true,true,2,false);Drive(150);
    CHECK(!CtrCoopInventoryBridge_Ready());CHECK(!sInventory.viewActive);
    CtrCoopInventoryBridge_Session(true,true,2,true);Drive(150);
    CHECK(CtrCoopInventoryBridge_Ready());CHECK(Read(NULL,0,ITEM_POTION)==7);
    CHECK(!CtrCoopInventoryBridge_IsResuming());
    uint32_t nonce=sClientNonce,epoch=sInventory.epoch;
    CtrCoopInventoryBridge_Session(false,true,0,true);CtrCoopInventoryBridge_Session(true,true,2,true);Drive(5);
    CHECK(CtrCoopInventoryBridge_IsResuming());CHECK(sClientNonce==nonce);CHECK(sInventory.epoch==epoch);
    CHECK(Write(NULL,0,ITEM_POTION,91));CtrCoopInventoryBridge_AfterSaveCopy();Drive(2);
    CHECK(Read(NULL,0,ITEM_POTION)==7);CHECK(CtrCoopInventoryBridge_Ready());
    CtrCoopInventoryBridge_AfterStackReset();CHECK(!sInventory.enabled);CHECK(sClientNonce==0);
    CtrCoopInventoryBridge_Session(true,true,2,false);Drive(150);
    CHECK(sClientNonce!=nonce);CHECK(sInventory.epoch!=epoch);CHECK(!CtrCoopInventoryBridge_IsResuming());
    CtrCoopInventoryBridge_Session(true,true,2,true);Drive(150);CHECK(CtrCoopInventoryBridge_Ready());
    CHECK(Read(NULL,0,ITEM_POTION)==7);
}
static void HostWorldReset(void)
{
    Clear();CHECK(Write(NULL,0,ITEM_POTION,5));
    CtrCoopInventoryBridge_Session(true,false,2,true);
    CHECK(CtrCoopInventory_ReserveUse(&sInventory,ITEM_POTION,1));
    CHECK(Read(NULL,0,ITEM_POTION)==4);CHECK(CtrCoopInventory_HasOpenUse(&sInventory));
    sInventory.cachedAcked=false;
    uint32_t epoch=sInventory.epoch;
    CtrCoopInventory journal=sInventory;
    CtrCoopInventoryBridge_Session(false,false,0,true);
    CHECK(!memcmp(&journal,&sInventory,sizeof(journal)));
    CHECK(!CtrCoopInventoryBridge_CanPrepareNativeLink());
    /* This API is called only after a real engine stack unwind. It must
     * discard even the host's open credit and unacknowledged decision. */
    CtrCoopInventoryBridge_AfterStackReset();
    CHECK(!sInventory.enabled);CHECK(!CtrCoopInventoryBridge_HasPending());
    CHECK(!sClientNonce);CHECK(!sConnected);CHECK(CtrCoopInventory_HookChange(0,ITEM_POTION,1)==-1);
    CHECK(Write(NULL,0,ITEM_POTION,9));
    CtrCoopInventoryBridge_Session(true,false,2,true);
    CHECK(CtrCoopInventoryBridge_Ready());CHECK(sInventory.epoch!=epoch);
    CHECK(CtrCoopInventory_GetQuantity(&sInventory,0,ITEM_POTION)==9);
    CHECK(!CtrCoopInventory_HasOpenUse(&sInventory));CHECK(sInventory.lastRequest==0);
    uint8_t stale[18]={CTR_COOP_INVENTORY_REQUEST,CTR_COOP_INVENTORY_VERSION};
    for(unsigned i=0;i<4;++i)stale[2+i]=epoch>>(8*i);
    stale[6]=1;stale[11]=1;stale[12]=ITEM_POTION;stale[14]=2;
    CHECK(CtrCoopInventoryBridge_HandlePacket(2,stale,sizeof(stale)));
    CHECK(Read(NULL,0,ITEM_POTION)==9);CHECK(sInventory.lastRequest==0);
}
static void Temporary(void)
{
    Clear();CHECK(Write(NULL,0,ITEM_POTION,2));CtrCoopInventoryBridge_Session(true,false,2,true);
    gMain.inBattle=true;gBattleTypeFlags=BATTLE_TYPE_WALLY_TUTORIAL;
    CHECK(CtrCoopInventory_SetViewActive(&sInventory,false));before=save1;
    uint8_t request[18]={CTR_COOP_INVENTORY_REQUEST,CTR_COOP_INVENTORY_VERSION};
    for(unsigned i=0;i<4;++i)request[2+i]=sInventory.epoch>>(8*i);
    request[6]=1;request[10]=0;request[11]=1;request[12]=ITEM_POTION;request[14]=2;
    sInventory.peerReady=true;
    CHECK(CtrCoopInventoryBridge_HandlePacket(2,request,sizeof(request)));
    CHECK(sInventory.quantity[0][ITEM_POTION]==4);CHECK(sInventory.cachedAccepted);
    CHECK(!CtrCoopInventoryBridge_CanPrepareNativeLink());
    CHECK(!CtrCoopInventoryBridge_PrepareNativeLink());CHECK(sInventory.enabled);
    Drive(2);CHECK(!memcmp(&before,&save1,sizeof(save1)));
    CHECK(Validate(NULL,sInventory.quantity,ITEMS_COUNT));CHECK(!memcmp(&before,&save1,sizeof(save1)));
    CHECK(CtrCoopInventory_SetViewActive(&sInventory,true));Drive(2);
    CHECK(!memcmp(&before,&save1,sizeof(save1)));
    gMain.inBattle=false;gBattleTypeFlags=0;Drive(2);CHECK(Read(NULL,0,ITEM_POTION)==4);
    uint8_t ack[11]={CTR_COOP_INVENTORY_CONTROL,CTR_COOP_INVENTORY_VERSION};
    for(unsigned i=0;i<4;++i)ack[2+i]=sInventory.epoch>>(8*i);
    ack[6]=1;ack[10]=3;CHECK(CtrCoopInventoryBridge_HandlePacket(2,ack,sizeof(ack)));
    CHECK(CtrCoopInventoryBridge_CanPrepareNativeLink());
    CHECK(CtrCoopInventoryBridge_PrepareNativeLink());CHECK(!CtrCoopInventoryBridge_Active());
    CHECK(Write(NULL,0,ITEM_POTION,3));CtrCoopInventoryBridge_Session(true,false,2,true);
    CHECK(CtrCoopInventory_GetQuantity(&sInventory,0,ITEM_POTION)==3);
}
int main(void)
{
    Adapter();HostHandshake();GuestHandshake();HostWorldReset();Temporary();
    printf("actual inventory bridge: %u failures\n",failures);return failures?1:0;
}
'''.replace("METADATA", metadata)
    build = ROOT / "build/coop-live"
    build.mkdir(parents=True, exist_ok=True)
    cfile = build / "inventory_bridge_test.c"
    cfile.write_text(source, encoding="utf-8")
    exe = build / "inventory_bridge_test.exe"
    flags = ["-std=gnu99", "-O2", "-Wall", "-Wextra", "-Werror", "-Wno-pointer-to-int-cast",
             "-Wno-int-to-pointer-cast", "-D__APPLE__", "-DPORTABLE", "-DPLATFORM_3DS", "-DPORT_BRIDGE"]
    includes = [".", "3ds_port/include", str(UPSTREAM / "include"), str(UPSTREAM / "include/constants"), str(UPSTREAM), "3ds_port/compat"]
    command = [cc, *flags, *[f"-iquote{x}" for x in includes], str(cfile),
               "3ds_port/src/3ds_coop_inventory.c", "-o", str(exe)]
    subprocess.run(command, cwd=ROOT, check=True)
    subprocess.run([str(exe)], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
