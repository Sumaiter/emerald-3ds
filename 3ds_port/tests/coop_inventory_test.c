#include "3ds_coop_inventory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ITEMS 12
#define CAPACITY 32
#define CHECK(x) do { if (!(x)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); failures++; } } while (0)
typedef struct { uint16_t item, quantity; } Slot;
typedef struct Endpoint {
    CtrCoopInventory state;
    Slot slots[2][CAPACITY];
    uint16_t key, id;
    unsigned writes, pauses;
    bool canonical, failInstall, failWrite;
} Endpoint;
typedef struct { uint16_t from, to, size; uint8_t data[32]; } Message;
static Endpoint host, guest;
static Message queue[4096], lastRequest, lastResult;
static unsigned head, tail, failures;
static uint32_t clockMs;
static bool dropReplies, dropFirstReply, dropAck, dropAll, raceHost, recoverOnPause, sendBlocked;

static uint32_t Read(void *u, uint8_t scope, uint16_t item)
{
    Endpoint *e = u;
    uint32_t total = 0;
    for (unsigned n = 0; n < CAPACITY; n++)
        if (e->slots[scope][n].item == item)
            total += e->slots[scope][n].quantity ^ (scope == CTR_COOP_INVENTORY_BAG ? e->key : 0);
    return total;
}
static bool Write(void *u, uint8_t scope, uint16_t item, uint32_t quantity)
{
    Endpoint *e = u;
    unsigned slot = CAPACITY;
    if (e->failWrite || quantity > 999) return false;
    for (unsigned n = 0; n < CAPACITY; n++)
        if (e->slots[scope][n].item == item) { slot = n; break; }
    if (slot == CAPACITY)
        for (unsigned n = 0; n < CAPACITY; n++)
            if (!e->slots[scope][n].item) { slot = n; break; }
    if (slot == CAPACITY) return !quantity;
    e->slots[scope][slot].item = quantity ? item : 0;
    e->slots[scope][slot].quantity = quantity ^ (scope == CTR_COOP_INVENTORY_BAG ? e->key : 0);
    e->writes++;
    if (e->state.applying)
        CHECK(CtrCoopInventory_Change(&e->state, scope, item, 1) == CTR_INV_REJECTED); /* callback suppression */
    return true;
}
static bool Install(void *u, const uint32_t qty[2][CTR_COOP_INVENTORY_MAX_ITEMS], uint16_t items)
{
    Endpoint *e = u;
    Slot slots[2][CAPACITY] = {0};
    if (e->failInstall) return false;
    for (unsigned s = 0; s < 2; s++)
    {
        unsigned cursor = 0;
        for (unsigned i = 1; i < items; i++)
            if (qty[s][i])
            {
                if (cursor == CAPACITY || qty[s][i] > 999) return false;
                slots[s][cursor].item = i;
                slots[s][cursor++].quantity = qty[s][i] ^ (s == CTR_COOP_INVENTORY_BAG ? e->key : 0);
            }
    }
    memcpy(e->slots, slots, sizeof(slots)); return true;
}
static bool Send(void *u, uint16_t peer, const uint8_t *p, uint16_t size)
{
    Endpoint *e = u;
    Message m = {.from = e->id, .to = peer, .size = size};
    CHECK(size <= sizeof(m.data));
    if (size > sizeof(m.data)) return false;
    if (sendBlocked) return false;
    memcpy(m.data, p, size);
    if (p[0] == CTR_COOP_INVENTORY_REQUEST) lastRequest = m;
    if (p[0] == CTR_COOP_INVENTORY_RESULT) lastResult = m;
    if (dropAll || (p[0] == CTR_COOP_INVENTORY_RESULT && (dropReplies || dropFirstReply)))
    { dropFirstReply = false; return true; }
    if (dropAck && p[0] == CTR_COOP_INVENTORY_CONTROL && p[10] == 3) return true;
    CHECK(tail < sizeof(queue) / sizeof(queue[0]));
    if (tail < sizeof(queue) / sizeof(queue[0])) queue[tail++] = m;
    return true;
}
static uint32_t Clock(void *u) { (void)u; return clockMs; }
static bool Canonical(void *u) { return ((Endpoint *)u)->canonical; }
static uint32_t ScalarCapacity(void *u, uint8_t scope, uint16_t item)
{ (void)u; (void)scope; (void)item; return 999; }
static bool Validate(void *u, const uint32_t qty[2][CTR_COOP_INVENTORY_MAX_ITEMS], uint16_t items)
{
    Endpoint *e = u;
    if (e->failInstall) return false;
    for (unsigned s = 0; s < 2; s++)
    {
        unsigned occupied = 0;
        for (unsigned i = 1; i < items; i++)
        {
            if (qty[s][i] > 999) return false;
            occupied += qty[s][i] != 0;
        }
        if (occupied > CAPACITY) return false;
    }
    return true;
}
static void Deliver(Message m)
{
    Endpoint *e = m.to == host.id ? &host : &guest;
    CtrCoopInventory *bound = e == &host ? &guest.state : &host.state;
    CtrCoopInventory_Bind(&e->state);
    CtrCoopInventory_HandlePacket(&e->state, m.from, m.data, m.size);
    CtrCoopInventory_Bind(bound);
}
static void Pump(void *u)
{
    (void)u;
    clockMs++;
    if (raceHost)
    {
        raceHost = false;
        CtrCoopInventory_Bind(&host.state);
        CHECK(CtrCoopInventory_Change(&host.state, 0, 1, -1) == CTR_INV_ACCEPTED);
        CtrCoopInventory_Bind(&guest.state);
    }
    if (head < tail) Deliver(queue[head++]);
    CtrCoopInventory_Bind(&host.state); CtrCoopInventory_Tick(&host.state);
    CtrCoopInventory_Bind(&guest.state); CtrCoopInventory_Tick(&guest.state);
}
static void Pause(void *u)
{
    Endpoint *e = u; e->pauses++;
    if (recoverOnPause) { dropReplies = false; dropAll = false; }
    Pump(u);
    CHECK(e->pauses < 20);
    if (e->pauses >= 20) exit(2);
}
static void Drain(void)
{
    for (unsigned n = 0; n < 1000 && (head < tail || host.state.snapshotSending); n++) Pump(NULL);
    CHECK(head == tail);
    head = tail = 0;
}
static void Setup(void)
{
    CtrCoopInventoryIO io = {0};
    CtrCoopInventory_Bind(NULL);
    memset(&host, 0, sizeof(host)); memset(&guest, 0, sizeof(guest));
    host.id = 1; guest.id = 2; host.key = 0x1234; guest.key = 0x4321;
    host.canonical = guest.canonical = true;
    head = tail = 0; clockMs = 0;
    dropReplies = dropFirstReply = dropAck = dropAll = raceHost = recoverOnPause = false;
    sendBlocked = false;
    host.slots[0][0] = (Slot){1, 1 ^ host.key}; host.slots[1][0] = (Slot){2, 3};
    guest.slots[0][0] = (Slot){3, 90 ^ guest.key};
    io.read = Read; io.write = Write; io.install = Install; io.send = Send;
    io.pump = Pump; io.clock = Clock; io.pause = Pause; io.canonical = Canonical;
    io.user = &host; CHECK(CtrCoopInventory_Init(&host.state, &io, true, 2, 0x12345678, ITEMS));
    io.user = &guest; CHECK(CtrCoopInventory_Init(&guest.state, &io, false, 1, 0, ITEMS));
    CHECK(!CtrCoopInventory_IsReady(&guest.state));
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, -1) == CTR_INV_REJECTED);
    CHECK(CtrCoopInventory_Hello(&guest.state, 0xA0B0C0D0)); Drain();
    CHECK(CtrCoopInventory_IsReady(&guest.state) && host.state.peerReady);
    CHECK(Read(&guest, 0, 1) == 1 && Read(&guest, 0, 3) == 0 && Read(&guest, 1, 2) == 3);
    CtrCoopInventory_Bind(&guest.state);
}
static void TestLiveAndLastItem(void)
{
    Setup(); raceHost = true;
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, -1) == CTR_INV_REJECTED); Drain();
    CHECK(Read(&host, 0, 1) == 0 && Read(&guest, 0, 1) == 0);
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, 4) == CTR_INV_ACCEPTED); Drain();
    CHECK(Read(&host, 0, 1) == 4 && Read(&guest, 0, 1) == 4);
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, -1) == CTR_INV_ACCEPTED); Drain();
    CHECK(Read(&host, 0, 1) == 3 && Read(&guest, 0, 1) == 3);
    CHECK(CtrCoopInventory_Change(&guest.state, 1, 2, -2) == CTR_INV_ACCEPTED); Drain();
    CHECK(Read(&host, 1, 2) == 1 && Read(&guest, 0, 2) == 0);
    CHECK(CtrCoopInventory_Audit(&guest.state) && CtrCoopInventory_Audit(&host.state));
}
static void TestRetriesAndUncertainty(void)
{
    Setup(); dropFirstReply = true;
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, 2) == CTR_INV_ACCEPTED); Drain();
    CHECK(Read(&host, 0, 1) == 3);
    Message old = lastRequest;
    Deliver(old); Drain(); CHECK(Read(&host, 0, 1) == 3);
    old.data[14] ^= 1; Deliver(old); Drain(); CHECK(Read(&host, 0, 1) == 3);
    dropReplies = true;
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, -1) == CTR_INV_PENDING);
    CHECK(CtrCoopInventory_IsUncertain(&guest.state));
    CHECK(!CtrCoopInventory_Reset(&guest.state) && !CtrCoopInventory_Reset(&host.state));
    CHECK(Read(&host, 0, 1) == 2);
    dropReplies = false;
    CHECK(CtrCoopInventory_Resume(&guest.state) == CTR_INV_ACCEPTED); Drain();
    CHECK(Read(&host, 0, 1) == 2 && Read(&guest, 0, 1) == 2);
    Deliver(old); Drain(); CHECK(Read(&host, 0, 1) == 2); /* stale request */
    CHECK(CtrCoopInventory_Reset(&guest.state) && CtrCoopInventory_Reset(&host.state));
}
static void TestUseAndRefund(void)
{
    Setup(); CHECK(CtrCoopInventory_BeginUse(1, 1)); Drain();
    CHECK(Read(&host, 0, 1) == 0 && Read(&guest, 0, 1) == 0);
    CtrCoopInventory_Bind(&host.state);
    CHECK(!CtrCoopInventory_BeginUse(1, 1)); /* no second effect from last item */
    CHECK(CtrCoopInventory_Change(&host.state, 0, 3, 1) == CTR_INV_REJECTED); /* refund capacity */
    CtrCoopInventory_Bind(&guest.state); CtrCoopInventory_RefundUse(); Drain();
    CHECK(Read(&host, 0, 1) == 1 && Read(&guest, 0, 1) == 1);
    CHECK(CtrCoopInventory_BeginUse(1, 1));
    CHECK(CtrCoopInventory_HookChange(0, 1, -1) == 1); Drain();
    CHECK(Read(&host, 0, 1) == 0 && Read(&guest, 0, 1) == 0 && !host.state.hostUseCount[1]);
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 3, 1) == CTR_INV_ACCEPTED); Drain();
    dropReplies = true; recoverOnPause = true;
    CHECK(CtrCoopInventory_BeginUse(3, 1)); CHECK(guest.pauses == 1);
    CHECK(CtrCoopInventory_HookChange(0, 3, -1) == 1); Drain();
    CHECK(Read(&host, 0, 3) == 0 && Read(&guest, 0, 3) == 0);
}
static void TestRekeyTemporaryAndFailures(void)
{
    Setup();
    uint16_t oldKey = guest.key; guest.key = 0x9ABC;
    for (unsigned n = 0; n < CAPACITY; n++) guest.slots[0][n].quantity ^= oldKey ^ guest.key;
    Slot swap = guest.slots[0][0]; guest.slots[0][0] = guest.slots[0][4]; guest.slots[0][4] = swap;
    CHECK(CtrCoopInventory_Audit(&guest.state)); /* rekey/sort is not an acquisition */
    CHECK(CtrCoopInventory_SetViewActive(&guest.state, false)); guest.canonical = false;
    CHECK(CtrCoopInventory_HookChange(0, 1, 99) == -1);
    CtrCoopInventory_Bind(&host.state);
    CHECK(CtrCoopInventory_Change(&host.state, 0, 1, 2) == CTR_INV_ACCEPTED); Drain();
    CHECK(Read(&guest, 0, 1) == 1 && CtrCoopInventory_GetQuantity(&guest.state, 0, 1) == 3);
    CHECK(CtrCoopInventory_SetViewActive(&guest.state, true)); /* tutorial flag still set; no callback deadlock */
    CHECK(guest.state.dirty && Read(&guest, 0, 1) == 1);
    guest.canonical = true; Pump(NULL); Drain();
    CHECK(Read(&guest, 0, 1) == 3);
    CtrCoopInventory_Bind(&guest.state); host.failWrite = true;
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, 1) == CTR_INV_REJECTED); Drain(); host.failWrite = false;
    CHECK(Read(&guest, 0, 1) == 3 && Read(&host, 0, 1) == 3);
    dropAck = true; CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, 1) == CTR_INV_ACCEPTED); Drain();
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, 1) == CTR_INV_ACCEPTED); Drain(); /* implicit ack */
    dropAck = false;
    for (unsigned n = 0; n < 120; n++) Pump(NULL);
    Drain(); CHECK(host.state.cachedAcked && Read(&host, 0, 1) == 5);
}
static void TestMalformedAndOffline(void)
{
    Setup();
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, 1) == CTR_INV_ACCEPTED); Drain();
    uint32_t before = Read(&host, 0, 1);
    Message m = lastRequest;
    for (unsigned size = 0; size < m.size; size++)
        CtrCoopInventory_HandlePacket(&host.state, m.from, m.data, size);
    m.data[1]--; Deliver(m); m.data[1]++;
    m.data[2] ^= 1; Deliver(m); m.data[2] ^= 1;
    m.from = 99; Deliver(m); m.from = guest.id;
    m.data[10] = 2; Deliver(m); Drain();
    CHECK(Read(&host, 0, 1) == before);
    CHECK(!CtrCoopInventory_HandlePacket(&host.state, 2, NULL, 0));
    CHECK(CtrCoopInventory_Change(&guest.state, 2, 1, 1) == CTR_INV_REJECTED);
    CHECK(CtrCoopInventory_Change(&guest.state, 0, ITEMS, 1) == CTR_INV_REJECTED);
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 0, 1) == CTR_INV_REJECTED);
    CtrCoopInventory_Bind(NULL); CHECK(CtrCoopInventory_HookChange(0, 1, 1) == -1);
    CHECK(CtrCoopInventory_BeginUse(1, 1)); CtrCoopInventory_RefundUse();
}
static void TestTransfersBackpressureAndReconnect(void)
{
    Setup();
    CHECK(CtrCoopInventory_HookTransfer(1, 2, 3) == 1); Drain();
    CHECK(Read(&host, 1, 2) == 0 && Read(&guest, 1, 2) == 0
        && Read(&host, 0, 2) == 3 && Read(&guest, 0, 2) == 3);
    CtrCoopInventory_Bind(&host.state);
    CHECK(CtrCoopInventory_HookTransfer(1, 2, 1) == 0); /* same last PC items already withdrawn */
    CtrCoopInventory_Bind(&guest.state);
    CHECK(CtrCoopInventory_HookTransfer(0, 2, 2) == 1); Drain();
    CHECK(Read(&host, 1, 2) == 2 && Read(&guest, 1, 2) == 2 && Read(&host, 0, 2) == 1);
    host.failInstall = true;
    CHECK(CtrCoopInventory_HookTransfer(0, 2, 1) == 0); Drain(); host.failInstall = false;
    CHECK(Read(&host, 0, 2) == 1 && Read(&host, 1, 2) == 2);
    CtrCoopInventory_Bind(&host.state); sendBlocked = true;
    CHECK(CtrCoopInventory_HookChange(0, 1, 1) == 1);
    CHECK(Read(&guest, 0, 1) == 1); sendBlocked = false; Pump(NULL); Drain();
    CHECK(Read(&guest, 0, 1) == 2);
    CtrCoopInventory_Bind(&guest.state); dropReplies = true;
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, -1) == CTR_INV_PENDING);
    Message wrong = lastResult; wrong.data[6] ^= 1; Deliver(wrong);
    CHECK(!guest.state.known && !CtrCoopInventory_Reset(&guest.state));
    guest.id = 3; CHECK(CtrCoopInventory_RebindPeer(&host.state, guest.id));
    CHECK(CtrCoopInventory_RebindPeer(&guest.state, host.id)); dropReplies = false;
    uint32_t epoch = host.state.epoch;
    CHECK(CtrCoopInventory_Hello(&guest.state, guest.state.clientNonce));
    CHECK(CtrCoopInventory_Resume(&guest.state) == CTR_INV_ACCEPTED); Drain();
    CHECK(host.state.epoch == epoch);
    CHECK(Read(&host, 0, 1) == 1 && Read(&guest, 0, 1) == 1);
}
static void TestSnapshotAndLegacyAudit(void)
{
    Setup();
    CtrCoopInventoryIO io = guest.state.io;
    CHECK(CtrCoopInventory_Reset(&guest.state));
    CHECK(CtrCoopInventory_Init(&guest.state, &io, false, 1, 0, ITEMS));
    CHECK(CtrCoopInventory_Hello(&guest.state, host.state.clientNonce));
    sendBlocked = true; CHECK(!CtrCoopInventory_StartSnapshot(&host.state));
    CHECK(!host.state.snapshotSending); sendBlocked = false;
    CHECK(CtrCoopInventory_StartSnapshot(&host.state));
    CtrCoopInventory_Bind(&host.state);
    CHECK(CtrCoopInventory_Change(&host.state, 0, 1, 4) == CTR_INV_ACCEPTED);
    guest.failInstall = true; Drain();
    CHECK(!CtrCoopInventory_IsReady(&guest.state)); guest.failInstall = false; Pump(NULL); Drain();
    CHECK(CtrCoopInventory_IsReady(&guest.state) && Read(&guest, 0, 1) == 5); /* stale snapshot row cannot overwrite new revision */
    CtrCoopInventory_Bind(NULL); CHECK(Write(&guest, 0, 3, 2)); CHECK(Write(&guest, 1, 4, 3));
    CtrCoopInventory_Bind(&guest.state); CHECK(!CtrCoopInventory_Audit(&guest.state));
    dropReplies = true;
    CHECK(!CtrCoopInventory_Reconcile(&guest.state)); /* unresolved batch survives stack return */
    CHECK(guest.state.reconciling && guest.state.reconcileWaiting);
    dropReplies = false; CHECK(CtrCoopInventory_Reconcile(&guest.state)); Drain();
    CHECK(Read(&host, 0, 3) == 2 && Read(&host, 1, 4) == 3);
    CHECK(CtrCoopInventory_Audit(&guest.state) && CtrCoopInventory_UsedLive(&guest.state));
    unsigned before = host.writes;
    CHECK(CtrCoopInventory_Reconcile(&guest.state)); Drain();
    CHECK(host.writes == before); /* no end-of-battle double application */
}
static void TestNewProcessHandshake(void)
{
    Setup();
    CHECK(CtrCoopInventory_BeginUse(1, 1)); Drain();
    CHECK(host.state.hostUseCount[1] == 1 && Read(&host, 0, 1) == 0);
    Message stale = lastRequest;
    uint32_t oldEpoch = host.state.epoch;
    CtrCoopInventoryIO io = guest.state.io;
    /* A genuinely new process has no old stack/effect/intent to resume. */
    memset(&guest.state, 0, sizeof(guest.state));
    CHECK(CtrCoopInventory_Init(&guest.state, &io, false, 1, 0, ITEMS));
    CHECK(CtrCoopInventory_Hello(&guest.state, 0xA0B0C0D1)); Drain();
    CHECK(CtrCoopInventory_IsReady(&guest.state) && host.state.epoch != oldEpoch);
    CHECK(!host.state.hostUseCount[1] && Read(&guest, 0, 1) == 0);
    Deliver(stale); Drain(); CHECK(Read(&host, 0, 1) == 0);
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 1, 2) == CTR_INV_ACCEPTED); Drain();
    CHECK(Read(&host, 0, 1) == 2 && Read(&guest, 0, 1) == 2);
    CHECK(guest.state.nextRequest == 1 && host.state.lastRequest == 1);
}
static void TestHeldExchangeAndLongCredits(void)
{
    Setup();
    CHECK(CtrCoopInventory_HookExchange(1, 3, 1) == 1); Drain();
    CHECK(Read(&host, 0, 1) == 0 && Read(&host, 0, 3) == 1 && Read(&guest, 0, 3) == 1);
    CtrCoopInventory_Bind(&host.state); CHECK(CtrCoopInventory_HookExchange(1, 4, 1) == 0);
    CtrCoopInventory_Bind(&guest.state); host.failInstall = true;
    CHECK(CtrCoopInventory_HookExchange(3, 1, 1) == 0); Drain(); host.failInstall = false;
    CHECK(Read(&host, 0, 3) == 1 && Read(&host, 0, 1) == 0);
    CHECK(CtrCoopInventory_HookExchange(3, 1, 1) == 1); Drain();
    CHECK(CtrCoopInventory_BeginUse(1, 1));
    CHECK(!CtrCoopInventory_BeginUse(1, 1)); /* one pre-effect debit authorizes one effect */
    CtrCoopInventory_RefundUse(); Drain();
    CHECK(CtrCoopInventory_ReserveUse(&guest.state, 1, 0));
    CHECK(CtrCoopInventory_FinishUse(&guest.state, 1, 0));
    /* Full 32-bit counts are retained even with a small-capacity test adapter.
     * A large reservation is definitively rejected, never truncated to 16b. */
    CHECK(!CtrCoopInventory_ReserveUse(&guest.state, 1, 65537)); Drain();
    CHECK(Read(&host, 0, 1) == 1);
}
static void TestScalarCapacityAndPartialJoin(void)
{
    Setup();
    host.state.io.capacity = guest.state.io.capacity = ScalarCapacity;
    CHECK(CtrCoopInventory_ReserveUse(&guest.state, 1, 1)); Drain();
    CtrCoopInventory_Bind(&host.state);
    CHECK(CtrCoopInventory_Change(&host.state, 0, 2, 100) == CTR_INV_ACCEPTED); Drain();
    CHECK(Read(&guest, 0, 2) == 100); /* other currency is independent */
    CHECK(CtrCoopInventory_Change(&host.state, 0, 1, 999) == CTR_INV_ACCEPTED); Drain();
    CHECK(Read(&host, 0, 1) == 998 && Read(&guest, 0, 1) == 998);
    CtrCoopInventory_RefundUseState(&guest.state); Drain();
    CHECK(Read(&host, 0, 1) == 999 && Read(&guest, 0, 1) == 999);
    CtrCoopInventoryIO io = guest.state.io;
    memset(&guest.state, 0, sizeof(guest.state));
    CHECK(CtrCoopInventory_Init(&guest.state, &io, false, 1, 0, ITEMS));
    CHECK(CtrCoopInventory_Hello(&guest.state, 0xB0C0D0E0));
    for (unsigned n = 0; n < 5; n++) Pump(NULL);
    CHECK(guest.state.snapshotReceiving && !guest.state.ready);
    head = tail = 0; /* old link drops accepted-but-undelivered snapshot rows */
    CHECK(CtrCoopInventory_RebindPeer(&host.state, guest.id));
    CHECK(CtrCoopInventory_RebindPeer(&guest.state, host.id));
    CHECK(CtrCoopInventory_Hello(&guest.state, guest.state.clientNonce)); Drain();
    CHECK(CtrCoopInventory_IsReady(&guest.state));
    CHECK(Read(&guest, 0, 1) == 999 && Read(&guest, 0, 2) == 100);
}
static void TestHostTemporaryValidation(void)
{
    Setup();
    host.state.io.validate = Validate;
    CHECK(CtrCoopInventory_SetViewActive(&host.state, false)); host.canonical = false;
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 3, 2) == CTR_INV_ACCEPTED); Drain();
    CHECK(Read(&host, 0, 3) == 0 && host.state.quantity[0][3] == 2 && Read(&guest, 0, 3) == 2);
    host.failInstall = true;
    CHECK(CtrCoopInventory_Change(&guest.state, 0, 4, 1) == CTR_INV_REJECTED); Drain(); host.failInstall = false;
    CHECK(host.state.quantity[0][4] == 0);
    CHECK(CtrCoopInventory_ReserveUse(&guest.state, 1, 1)); Drain();
    CtrCoopInventory_RefundUseState(&guest.state); Drain();
    CHECK(host.state.quantity[0][1] == 1 && Read(&guest, 0, 1) == 1);
    host.canonical = true; CHECK(CtrCoopInventory_SetViewActive(&host.state, true));
    CHECK(CtrCoopInventory_Audit(&host.state) && Read(&host, 0, 3) == 2);
}
int main(void)
{
    TestLiveAndLastItem(); TestRetriesAndUncertainty(); TestUseAndRefund();
    TestRekeyTemporaryAndFailures(); TestMalformedAndOffline();
    TestTransfersBackpressureAndReconnect(); TestSnapshotAndLegacyAudit();
    TestNewProcessHandshake();
    TestHeldExchangeAndLongCredits();
    TestScalarCapacityAndPartialJoin();
    TestHostTemporaryValidation();
    printf("co-op live inventory tests: %u failures\n", failures); return failures != 0;
}
