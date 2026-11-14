#include "3ds_coop_inventory.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>

#define REQUEST_SIZE 18
#define RESULT_SIZE 22
#define BEGIN_SIZE 20
#define ROW_SIZE 17
#define CONTROL_SIZE 11
#define SNAPSHOT_END 1
#define SNAPSHOT_ACK 2
#define RESULT_ACK 3
#define HELLO 4
#define HELLO_ACK 5
#define HELLO_RESUME 6
#define ADD 1
#define REMOVE 2
#define RESERVE_USE 3
#define FINISH_USE 4
#define REFUND_USE 5
#define TRANSFER 6
#define EXCHANGE 7

static CtrCoopInventory *sBound;

static uint16_t Get16(const uint8_t *p) { return p[0] | ((uint16_t)p[1] << 8); }
static uint32_t Get32(const uint8_t *p) { return Get16(p) | ((uint32_t)Get16(p + 2) << 16); }
static void Put16(uint8_t *p, uint16_t x) { p[0] = x; p[1] = x >> 8; }
static void Put32(uint8_t *p, uint32_t x) { Put16(p, x); Put16(p + 2, x >> 16); }
static uint32_t Now(CtrCoopInventory *s) { return s->io.clock(s->io.user); }
static bool Valid(const CtrCoopInventory *s, uint8_t scope, uint16_t item)
{
    return scope < CTR_COOP_INVENTORY_SCOPES && item != 0 && item < s->items;
}
static void Header(CtrCoopInventory *s, uint8_t *p, uint8_t type)
{
    p[0] = type; p[1] = CTR_COOP_INVENTORY_VERSION; Put32(p + 2, s->epoch);
}
static bool Send(CtrCoopInventory *s, const uint8_t *p, uint16_t size)
{
    return s->io.send(s->io.user, s->peer, p, size);
}
static bool Control(CtrCoopInventory *s, uint8_t kind, uint32_t id)
{
    uint8_t p[CONTROL_SIZE];
    Header(s, p, CTR_COOP_INVENTORY_CONTROL); Put32(p + 6, id); p[10] = kind; return Send(s, p, sizeof(p));
}
static void Result(CtrCoopInventory *s, uint32_t id, uint8_t scope, uint16_t item, bool accepted)
{
    uint8_t p[RESULT_SIZE];
    Header(s, p, CTR_COOP_INVENTORY_RESULT); Put32(p + 6, id); Put32(p + 10, s->revision);
    p[14] = scope; p[15] = accepted; Put16(p + 16, item); Put32(p + 18, s->quantity[scope][item]);
    if (!Send(s, p, sizeof(p)) && !id) s->updates[scope][item] = true;
}
static void Request(CtrCoopInventory *s)
{
    uint8_t p[REQUEST_SIZE];
    Header(s, p, CTR_COOP_INVENTORY_REQUEST); Put32(p + 6, s->pendingRequest);
    p[10] = s->pendingScope; p[11] = s->pendingKind;
    Put16(p + 12, s->pendingItem); Put32(p + 14, s->pendingCount);
    s->retryAt = Now(s); Send(s, p, sizeof(p));
}
static bool Install(CtrCoopInventory *s)
{
    bool ok;
    if (!s->viewActive || s->snapshotReceiving || !s->io.canonical(s->io.user))
        return false;
    s->applying = true;
    ok = s->io.install(s->io.user, s->quantity, s->items);
    s->applying = false;
    if (ok) s->dirty = false;
    return ok;
}
static bool Materialize(CtrCoopInventory *s, uint8_t scope, uint16_t item)
{
    bool ok;
    if (!s->viewActive || !s->io.canonical(s->io.user) || s->snapshotReceiving || s->dirty)
    {
        s->dirty = true;
        return false;
    }
    s->applying = true;
    ok = s->io.write(s->io.user, scope, item, s->quantity[scope][item]);
    s->applying = false;
    if (!ok) s->dirty = true;
    return ok;
}
static bool Commit(CtrCoopInventory *s, uint8_t scope, uint16_t item, uint8_t kind, uint32_t count, bool guest)
{
    uint32_t before = s->quantity[scope][item], after;
    bool ok = true;
    bool physical = s->viewActive && s->io.canonical(s->io.user);
    if (!count || kind < ADD || kind > EXCHANGE || s->revision == UINT32_MAX)
        return false;
    if (kind == EXCHANGE)
    {
        uint16_t other = count >> 16, amount = count & 0xFFFF;
        uint32_t otherBefore;
        if (scope != CTR_COOP_INVENTORY_BAG || !Valid(s, scope, other) || !amount
         || (physical && s->dirty) || (!physical && !s->io.validate) || amount > before) return false;
        if (other == item) return true;
        otherBefore = s->quantity[scope][other];
        if (otherBefore > UINT32_MAX - amount || s->hostUseCount[0] || s->hostUseCount[1]) return false;
        s->quantity[scope][item] -= amount; s->quantity[scope][other] += amount;
        s->applying = true;
        ok = physical ? s->io.install(s->io.user, s->quantity, s->items) : s->io.validate(s->io.user, s->quantity, s->items);
        s->applying = false;
        if (!ok) { s->quantity[scope][item] = before; s->quantity[scope][other] = otherBefore; return false; }
        if (!physical) s->dirty = true;
        s->revisionFor[scope][item] = s->revisionFor[scope][other] = ++s->revision;
        s->usedLive = true; Result(s, 0, scope, other, true); return true;
    }
    if (kind == TRANSFER)
    {
        uint8_t destination = 1 - scope;
        uint32_t targetBefore = s->quantity[destination][item];
        if ((physical && s->dirty) || (!physical && !s->io.validate) || count > before || targetBefore > UINT32_MAX - count
         || (destination == CTR_COOP_INVENTORY_BAG && (s->hostUseCount[0] || s->hostUseCount[1]))) return false;
        s->quantity[scope][item] -= count; s->quantity[destination][item] += count;
        s->applying = true;
        ok = physical ? s->io.install(s->io.user, s->quantity, s->items) : s->io.validate(s->io.user, s->quantity, s->items);
        s->applying = false;
        if (!ok) { s->quantity[scope][item] = before; s->quantity[destination][item] = targetBefore; return false; }
        if (!physical) s->dirty = true;
        s->revisionFor[scope][item] = s->revisionFor[destination][item] = ++s->revision;
        s->usedLive = true;
        Result(s, 0, destination, item, true); return true;
    }
    if (kind >= RESERVE_USE && scope != CTR_COOP_INVENTORY_BAG) return false;
    if (kind == RESERVE_USE && s->hostUseCount[guest]) return false;
    if ((kind == FINISH_USE || kind == REFUND_USE)
     && (s->hostUseItem[guest] != item || s->hostUseCount[guest] != count)) return false;
    /* Preserve refund capacity: while an effect has a debit reserved, reject
     * unrelated additions. Removals cannot consume the freed capacity. */
    if (kind == ADD && scope == CTR_COOP_INVENTORY_BAG && !s->io.capacity
     && (s->hostUseCount[0] || s->hostUseCount[1])) return false;
    if (kind == FINISH_USE)
    {
        s->hostUseCount[guest] = 0;
        return true;
    }
    if (kind == ADD || kind == REFUND_USE)
    {
        if (kind == ADD && s->io.capacity)
        {
            uint32_t maximum = s->io.capacity(s->io.user, scope, item);
            uint64_t held = 0, total = (uint64_t)before + count;
            for (unsigned owner = 0; owner < 2; owner++)
                if (s->hostUseItem[owner] == item && scope == CTR_COOP_INVENTORY_BAG)
                    held += s->hostUseCount[owner];
            if (held > maximum || before > maximum - held) return false;
            maximum -= (uint32_t)held;
            after = total > maximum ? maximum : (uint32_t)total;
        }
        else
        {
            if (count > UINT32_MAX - before) return false;
            after = before + count;
        }
    }
    else
    {
        if (count > before) return false;
        after = before - count;
    }
    /* A temporary native bag must never receive canonical writes. The ledger
     * remains authoritative; install it after that native bag is restored. */
    if (physical)
    {
        if (s->dirty) return false;
        s->applying = true;
        ok = s->io.write(s->io.user, scope, item, after);
        s->applying = false;
    }
    else
    {
        if (kind == ADD || kind == REFUND_USE)
        {
            if (!s->io.validate) return false;
            s->quantity[scope][item] = after;
            ok = s->io.validate(s->io.user, s->quantity, s->items);
            s->quantity[scope][item] = before;
            if (!ok) return false;
        }
        s->dirty = true;
    }
    if (!ok) return false;
    s->quantity[scope][item] = after;
    s->revisionFor[scope][item] = ++s->revision;
    s->usedLive = true;
    if (kind == RESERVE_USE) { s->hostUseItem[guest] = item; s->hostUseCount[guest] = count; }
    if (kind == REFUND_USE) s->hostUseCount[guest] = 0;
    return true;
}

bool CtrCoopInventory_Init(CtrCoopInventory *s, const CtrCoopInventoryIO *io, bool host,
    uint16_t peer, uint32_t epoch, uint16_t items)
{
    if (!s || !io || !io->read || !io->write || !io->install || !io->send
     || !io->pump || !io->clock || !io->pause || !io->canonical
     || !peer || items < 2 || items > CTR_COOP_INVENTORY_MAX_ITEMS || (host && !epoch)) return false;
    memset(s, 0, sizeof(*s)); s->io = *io; s->host = host; s->peer = peer;
    s->epoch = epoch; s->items = items; s->enabled = true; s->viewActive = true;
    s->ready = host; s->cachedAcked = true;
    if (host)
        for (uint8_t scope = 0; scope < CTR_COOP_INVENTORY_SCOPES; scope++)
            for (uint16_t item = 1; item < items; item++)
                s->quantity[scope][item] = io->read(io->user, scope, item);
    return true;
}
bool CtrCoopInventory_Reset(CtrCoopInventory *s)
{
    if (!s || s->pending || s->reconciling || s->useCount || s->hostUseCount[0] || s->hostUseCount[1]
     || (s->host && !s->cachedAcked)) return false;
    memset(s, 0, sizeof(*s)); return true;
}
bool CtrCoopInventory_RebindPeer(CtrCoopInventory *s, uint16_t peer)
{
    if (!s || !s->enabled || !peer) return false;
    s->peer = peer; s->retryAt = Now(s) - CTR_COOP_INVENTORY_RETRY_MS;
    if (s->host && !s->peerReady) s->snapshotRestart = true;
    if (!s->host && !s->ready)
    {
        /* Queued rows accepted by the old transport may have died with it.
         * Restart an incomplete join, rather than waiting for missing rows. */
        s->snapshotReceiving = false; s->snapshotRows = 0;
    }
    return true;
}
bool CtrCoopInventory_Hello(CtrCoopInventory *s, uint32_t nonce)
{
    uint8_t p[CONTROL_SIZE];
    if (!s || !s->enabled || s->host || !nonce || (s->clientNonce && s->clientNonce != nonce)) return false;
    s->clientNonce = nonce;
    Header(s, p, CTR_COOP_INVENTORY_CONTROL); Put32(p + 6, nonce); p[10] = s->ready ? HELLO_RESUME : HELLO;
    s->retryAt = Now(s); return Send(s, p, sizeof(p));
}
bool CtrCoopInventory_HasOpenUse(const CtrCoopInventory *s)
{
    return s && (s->useCount || s->hostUseCount[0] || s->hostUseCount[1]);
}
bool CtrCoopInventory_StartSnapshot(CtrCoopInventory *s)
{
    uint8_t p[BEGIN_SIZE];
    if (!s || !s->enabled || !s->host || !s->clientNonce || s->snapshotSending || !s->cachedAcked) return false;
    memcpy(s->frozen, s->quantity, sizeof(s->frozen)); s->snapshotRevision = s->revision;
    Header(s, p, CTR_COOP_INVENTORY_BEGIN); Put32(p + 6, s->snapshotRevision); Put16(p + 10, s->items);
    Put32(p + 12, s->lastRequest); Put32(p + 16, s->clientNonce);
    if (!Send(s, p, sizeof(p))) return false;
    s->snapshotCursor = 0; s->snapshotSending = true; s->peerReady = false; return true;
}
void CtrCoopInventory_Tick(CtrCoopInventory *s)
{
    if (!s || !s->enabled) return;
    if (s->host && s->helloPending && CtrCoopInventory_StartSnapshot(s)) s->helloPending = false;
    if (!s->host && !s->ready && !s->snapshotReceiving && s->clientNonce
     && (uint32_t)(Now(s) - s->retryAt) >= CTR_COOP_INVENTORY_RETRY_MS)
        CtrCoopInventory_Hello(s, s->clientNonce);
    if (s->snapshotSending)
    {
        for (unsigned n = 0; n < 8; n++)
        {
            uint8_t p[ROW_SIZE];
            uint16_t cursor = s->snapshotCursor, item = cursor % (s->items - 1) + 1;
            uint8_t scope = cursor / (s->items - 1);
            if (scope >= CTR_COOP_INVENTORY_SCOPES)
            {
                if (Control(s, SNAPSHOT_END, s->snapshotRevision)) s->snapshotSending = false;
                break;
            }
            Header(s, p, CTR_COOP_INVENTORY_ROW); Put32(p + 6, s->snapshotRevision);
            p[10] = scope; Put16(p + 11, item); Put32(p + 13, s->frozen[scope][item]);
            if (!Send(s, p, sizeof(p))) break;
            s->snapshotCursor++;
        }
    }
    if (s->host)
    {
        unsigned sent = 0;
        for (uint8_t scope = 0; scope < CTR_COOP_INVENTORY_SCOPES && sent < 8; scope++)
            for (uint16_t item = 1; item < s->items && sent < 8; item++)
                if (s->updates[scope][item])
                {
                    s->updates[scope][item] = false; Result(s, 0, scope, item, true); sent++;
                }
    }
    if (s->pending && !s->known && (uint32_t)(Now(s) - s->retryAt) >= CTR_COOP_INVENTORY_RETRY_MS) Request(s);
    if (s->dirty && s->viewActive && !s->snapshotReceiving && Install(s)
     && !s->host && !s->ready && s->snapshotRows == CTR_COOP_INVENTORY_SCOPES * (s->items - 1))
    {
        s->ready = true; Control(s, SNAPSHOT_ACK, s->snapshotRevision);
    }
    if (!s->host && s->ready && (uint32_t)(Now(s) - s->retryAt) >= CTR_COOP_INVENTORY_RETRY_MS)
    {
        Control(s, SNAPSHOT_ACK, s->snapshotRevision);
        if (s->pendingRequest && (!s->pending || s->known)) Control(s, RESULT_ACK, s->pendingRequest);
        s->retryAt = Now(s);
    }
}
bool CtrCoopInventory_HandlePacket(CtrCoopInventory *s, uint16_t from, const uint8_t *p, uint16_t size)
{
    uint32_t epoch, id, revision;
    uint16_t item;
    uint8_t scope;
    if (!s || !s->enabled || !p || !size || p[0] < CTR_COOP_INVENTORY_REQUEST || p[0] > CTR_COOP_INVENTORY_CONTROL) return false;
    if (from != s->peer || size < 6 || p[1] != CTR_COOP_INVENTORY_VERSION) return true;
    epoch = Get32(p + 2);
    if (p[0] == CTR_COOP_INVENTORY_CONTROL && size == CONTROL_SIZE && (p[10] == HELLO || p[10] == HELLO_RESUME) && s->host)
    {
        uint32_t nonce = Get32(p + 6);
        if (!nonce || (epoch && epoch != s->epoch)) return true;
        if (s->clientNonce != nonce)
        {
            if (s->clientNonce)
            {
                /* Fresh process cannot replay an old effect. Keep canonical
                 * quantities, abandon the old volatile intent, rotate epoch. */
                s->epoch += 0x9E3779B9u;
                if (!s->epoch) s->epoch = 1;
                s->lastRequest = 0; s->cachedAcked = true; s->hostUseCount[1] = 0;
                s->snapshotSending = false; s->peerReady = false;
            }
            s->clientNonce = nonce; s->helloPending = true;
        }
        else if (p[10] == HELLO_RESUME)
            s->peerReady = true;
        else if (!s->snapshotSending || s->snapshotRestart)
        {
            s->snapshotSending = false; s->helloPending = true; s->peerReady = false;
        }
        s->snapshotRestart = false;
        Control(s, HELLO_ACK, nonce);
        return true;
    }
    if (p[0] == CTR_COOP_INVENTORY_BEGIN)
    {
        if (s->host || size != BEGIN_SIZE || !epoch || (s->epoch && epoch != s->epoch)
         || Get16(p + 10) != s->items || !s->clientNonce || Get32(p + 16) != s->clientNonce
         || s->pending || s->ready || s->snapshotReceiving) return true;
        s->epoch = epoch; s->snapshotRevision = Get32(p + 6); s->snapshotRows = 0;
        s->nextRequest = Get32(p + 12);
        memset(s->seen, 0, sizeof(s->seen)); s->snapshotReceiving = true; return true;
    }
    if (!s->epoch || epoch != s->epoch) return true;
    if (p[0] == CTR_COOP_INVENTORY_REQUEST)
    {
        uint32_t count;
        uint8_t kind;
        if (!s->host || size != REQUEST_SIZE) return true;
        if (!s->peerReady && !s->snapshotSending && s->snapshotCursor == CTR_COOP_INVENTORY_SCOPES * (s->items - 1))
            s->peerReady = true; /* first RPC also acknowledges completed join */
        if (!s->peerReady) return true;
        id = Get32(p + 6); scope = p[10]; kind = p[11]; item = Get16(p + 12); count = Get32(p + 14);
        if (!Valid(s, scope, item) || !id || !count || kind < ADD || kind > EXCHANGE) return true;
        if (id == s->lastRequest)
        {
            if (scope == s->cachedScope && kind == s->cachedKind && item == s->cachedItem && count == s->cachedCount)
                Result(s, id, scope, item, s->cachedAccepted);
            return true;
        }
        if (s->lastRequest == UINT32_MAX || id != s->lastRequest + 1) return true;
        s->lastRequest = id; s->cachedScope = scope; s->cachedKind = kind; s->cachedItem = item; s->cachedCount = count;
        s->cachedAccepted = Commit(s, scope, item, kind, count, true); s->cachedAcked = false;
        Result(s, id, scope, item, s->cachedAccepted); return true;
    }
    if (p[0] == CTR_COOP_INVENTORY_RESULT)
    {
        if (s->host || size != RESULT_SIZE || (!s->ready && !s->snapshotReceiving)) return true;
        id = Get32(p + 6); revision = Get32(p + 10); scope = p[14]; item = Get16(p + 16);
        if (!Valid(s, scope, item) || p[15] > 1 || (id && (id != s->pendingRequest
          || scope != s->pendingScope || item != s->pendingItem))) return true;
        if (id && !s->pending) { Control(s, RESULT_ACK, id); return true; }
        if (revision >= s->revisionFor[scope][item])
        {
            s->quantity[scope][item] = Get32(p + 18); s->revisionFor[scope][item] = revision;
            if (revision > s->revision) s->revision = revision;
            if (revision) s->usedLive = true;
            Materialize(s, scope, item);
        }
        if (id)
        {
            s->known = true; s->accepted = p[15] != 0; Control(s, RESULT_ACK, id);
        }
        return true;
    }
    if (p[0] == CTR_COOP_INVENTORY_ROW)
    {
        if (s->host || size != ROW_SIZE || !s->snapshotReceiving || Get32(p + 6) != s->snapshotRevision) return true;
        scope = p[10]; item = Get16(p + 11);
        if (!Valid(s, scope, item)) return true;
        if (!s->seen[scope][item]) { s->seen[scope][item] = true; s->snapshotRows++; }
        if (s->revisionFor[scope][item] <= s->snapshotRevision)
        {
            s->quantity[scope][item] = Get32(p + 13); s->revisionFor[scope][item] = s->snapshotRevision;
        }
        return true;
    }
    if (p[0] == CTR_COOP_INVENTORY_CONTROL && size == CONTROL_SIZE)
    {
        id = Get32(p + 6);
        if (s->host)
        {
            if (p[10] == SNAPSHOT_ACK && id == s->snapshotRevision && !s->snapshotSending) s->peerReady = true;
            if (p[10] == RESULT_ACK && id == s->lastRequest) s->cachedAcked = true;
        }
        else if (p[10] == SNAPSHOT_END && s->snapshotReceiving && id == s->snapshotRevision
              && s->snapshotRows == CTR_COOP_INVENTORY_SCOPES * (s->items - 1))
        {
            s->snapshotReceiving = false; s->dirty = true;
            if (Install(s)) { s->ready = true; Control(s, SNAPSHOT_ACK, id); }
        }
        return true;
    }
    return true;
}
CtrCoopInventoryResult CtrCoopInventory_Resume(CtrCoopInventory *s)
{
    uint32_t start;
    if (!s || !s->pending) return CTR_INV_REJECTED;
    start = Now(s);
    for (unsigned pumps = 0; pumps < 4096; pumps++)
    {
        if (s->known && !s->dirty)
        {
            CtrCoopInventoryResult result = s->accepted ? CTR_INV_ACCEPTED : CTR_INV_REJECTED;
            s->pending = false; s->uncertain = false; return result;
        }
        if ((uint32_t)(Now(s) - start) >= CTR_COOP_INVENTORY_WAIT_MS) break;
        s->io.pump(s->io.user); CtrCoopInventory_Tick(s);
    }
    s->uncertain = true; return CTR_INV_PENDING;
}
static CtrCoopInventoryResult ChangeKind(CtrCoopInventory *s, uint8_t scope, uint16_t item, uint8_t kind, uint32_t count)
{
    if (!CtrCoopInventory_IsReady(s) || !Valid(s, scope, item) || !count || s->pending || s->applying) return CTR_INV_REJECTED;
    if (s->host)
    {
        bool accepted = Commit(s, scope, item, kind, count, false);
        if (accepted) Result(s, 0, scope, item, true);
        return accepted ? CTR_INV_ACCEPTED : CTR_INV_REJECTED;
    }
    if (s->nextRequest == UINT32_MAX) return CTR_INV_REJECTED;
    s->pendingRequest = ++s->nextRequest; s->pendingScope = scope; s->pendingKind = kind;
    s->pendingItem = item; s->pendingCount = count; s->pending = true; s->known = false;
    Request(s); return CtrCoopInventory_Resume(s);
}
CtrCoopInventoryResult CtrCoopInventory_Change(CtrCoopInventory *s, uint8_t scope, uint16_t item, int32_t delta)
{
    return ChangeKind(s, scope, item, delta < 0 ? REMOVE : ADD,
        delta < 0 ? (uint32_t)(-(int64_t)delta) : (uint32_t)delta);
}
uint32_t CtrCoopInventory_GetQuantity(const CtrCoopInventory *s, uint8_t scope, uint16_t item)
{
    return s && Valid(s, scope, item) ? s->quantity[scope][item] : 0;
}
bool CtrCoopInventory_IsReady(const CtrCoopInventory *s)
{
    return s && s->enabled && s->ready && s->viewActive && !s->dirty;
}
bool CtrCoopInventory_IsActive(const CtrCoopInventory *s) { return s && s->enabled; }
bool CtrCoopInventory_UsedLive(const CtrCoopInventory *s) { return s && s->usedLive; }
bool CtrCoopInventory_IsUncertain(const CtrCoopInventory *s) { return s && s->uncertain; }
bool CtrCoopInventory_Audit(const CtrCoopInventory *s)
{
    if (!CtrCoopInventory_IsReady(s) || !s->io.canonical(s->io.user)) return false;
    for (uint8_t scope = 0; scope < CTR_COOP_INVENTORY_SCOPES; scope++)
        for (uint16_t item = 1; item < s->items; item++)
            if (s->quantity[scope][item] != s->io.read(s->io.user, scope, item)) return false;
    return true;
}
bool CtrCoopInventory_Reconcile(CtrCoopInventory *s)
{
    if (!CtrCoopInventory_IsReady(s) || !s->io.canonical(s->io.user)) return false;
    if (s->pending && !s->reconcileWaiting) return false;
    /* Capture all physical observations before a pumping RPC can deliver a
     * host update and replace the local view. Sorting/rekeying sum to zero. */
    if (!s->reconciling)
    {
        for (uint8_t scope = 0; scope < CTR_COOP_INVENTORY_SCOPES; scope++)
            for (uint16_t item = 1; item < s->items; item++)
            {
                int64_t diff = (int64_t)s->io.read(s->io.user, scope, item) - s->quantity[scope][item];
                if (diff < INT32_MIN || diff > INT32_MAX) return false;
                s->reconcileDelta[scope][item] = diff;
            }
        s->reconcileCursor = 0; s->reconciling = true;
    }
    while (s->reconcileCursor < CTR_COOP_INVENTORY_SCOPES * (s->items - 1))
    {
        uint8_t scope = s->reconcileCursor / (s->items - 1);
        uint16_t item = s->reconcileCursor % (s->items - 1) + 1;
        CtrCoopInventoryResult result = CTR_INV_ACCEPTED;
        if (s->reconcileWaiting)
        {
            result = CtrCoopInventory_Resume(s);
            if (result != CTR_INV_PENDING) s->reconcileWaiting = false;
        }
        else if (s->reconcileDelta[scope][item])
        {
            result = CtrCoopInventory_Change(s, scope, item, s->reconcileDelta[scope][item]);
            if (result == CTR_INV_PENDING) s->reconcileWaiting = true;
        }
        if (result != CTR_INV_ACCEPTED) return false;
        s->reconcileCursor++;
    }
    s->reconciling = false;
    return CtrCoopInventory_Audit(s);
}
bool CtrCoopInventory_SetViewActive(CtrCoopInventory *s, bool active)
{
    if (!s || s->pending || s->useCount) return false;
    s->viewActive = active;
    if (!active) s->dirty = true;
    /* Wally restores the normal bag before clearing its tutorial battle
     * flag. Do not wait inside that callback for a flag which only the
     * blocked native caller can clear; Tick installs after the return. */
    if (active && s->dirty && s->io.canonical(s->io.user)) return Install(s);
    return true;
}
void CtrCoopInventory_Bind(CtrCoopInventory *s) { sBound = s; }
static CtrCoopInventoryResult Wait(CtrCoopInventory *s, CtrCoopInventoryResult result)
{
    while (result == CTR_INV_PENDING)
    {
        s->io.pause(s->io.user); result = CtrCoopInventory_Resume(s);
    }
    return result;
}
int CtrCoopInventory_HookChange(uint8_t scope, uint16_t item, int32_t delta)
{
    CtrCoopInventory *s = sBound;
    if (!s || !s->enabled || s->applying || !s->viewActive || !s->io.canonical(s->io.user)) return -1;
    if (!delta) return -1;
    if (scope == CTR_COOP_INVENTORY_BAG && delta < 0 && item == s->useItem && (uint32_t)(-(int64_t)delta) == s->useCount)
    {
        return CtrCoopInventory_FinishUse(s, item, s->useCount);
    }
    return Wait(s, CtrCoopInventory_Change(s, scope, item, delta)) == CTR_INV_ACCEPTED;
}
int CtrCoopInventory_HookTransfer(uint8_t source, uint16_t item, uint16_t count)
{
    CtrCoopInventory *s = sBound;
    if (!s || !s->enabled || s->applying || !s->viewActive || !s->io.canonical(s->io.user)) return -1;
    return Wait(s, ChangeKind(s, source, item, TRANSFER, count)) == CTR_INV_ACCEPTED;
}
int CtrCoopInventory_HookExchange(uint16_t removeItem, uint16_t returnItem, uint16_t count)
{
    CtrCoopInventory *s = sBound;
    if (!s || !s->enabled || s->applying || !s->viewActive || !s->io.canonical(s->io.user)) return -1;
    if (!removeItem) return CtrCoopInventory_HookChange(CTR_COOP_INVENTORY_BAG, returnItem, count);
    if (!returnItem) return CtrCoopInventory_HookChange(CTR_COOP_INVENTORY_BAG, removeItem, -(int32_t)count);
    return Wait(s, ChangeKind(s, CTR_COOP_INVENTORY_BAG, removeItem, EXCHANGE,
        ((uint32_t)returnItem << 16) | count)) == CTR_INV_ACCEPTED;
}
bool CtrCoopInventory_ReserveUse(CtrCoopInventory *s, uint16_t item, uint32_t count)
{
    int result;
    if (!s || !s->enabled || !s->viewActive || !s->io.canonical(s->io.user)) return true;
    if (!count) return !s->useCount;
    if (s->useCount) return false;
    result = Wait(s, ChangeKind(s, CTR_COOP_INVENTORY_BAG, item, RESERVE_USE, count));
    if (result != CTR_INV_ACCEPTED) return false;
    s->useItem = item; s->useCount = count; return true;
}
bool CtrCoopInventory_BeginUse(uint16_t item, uint16_t count)
{
    return CtrCoopInventory_ReserveUse(sBound, item, count);
}
bool CtrCoopInventory_FinishUse(CtrCoopInventory *s, uint16_t item, uint32_t count)
{
    if (!s || !s->enabled || !s->viewActive || !s->io.canonical(s->io.user)) return true;
    if (!count) return !s->useCount;
    if (!count || item != s->useItem || count != s->useCount) return false;
    if (Wait(s, ChangeKind(s, CTR_COOP_INVENTORY_BAG, item, FINISH_USE, count)) != CTR_INV_ACCEPTED) return false;
    s->useCount = 0; return true;
}
void CtrCoopInventory_RefundUseState(CtrCoopInventory *s)
{
    uint16_t item;
    uint32_t count;
    if (!s || !s->useCount) return;
    item = s->useItem; count = s->useCount;
    /* Keep the credit until its refund is confirmed. Native effects cannot
     * proceed while a full bag or uncertain link prevents that compensation. */
    while (Wait(s, ChangeKind(s, CTR_COOP_INVENTORY_BAG, item, REFUND_USE, count)) != CTR_INV_ACCEPTED)
        s->io.pause(s->io.user);
    s->useCount = 0;
}
void CtrCoopInventory_RefundUse(void) { CtrCoopInventory_RefundUseState(sBound); }
void CtrCoopInventory_TemporaryBag(bool temporary)
{
    if (!sBound || !sBound->enabled) return;
    while (!CtrCoopInventory_SetViewActive(sBound, !temporary)) sBound->io.pause(sBound->io.user);
}
