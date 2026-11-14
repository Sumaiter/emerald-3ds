/* Explicit, session-local travelling group. No party/save/script mutation. */
#define FOLLOW_NONE 0
#define FOLLOW_PENDING 1
#define FOLLOW_FOLLOWER 2
#define FOLLOW_LEADER 3
#define FOLLOW_JOIN 1
#define FOLLOW_ACCEPT 2
#define FOLLOW_DENY 3
#define FOLLOW_STOP 4
#define FOLLOW_STATUS 5
#define FOLLOW_TRAIL_SIZE 64

static struct
{
    uint8_t state, retry;
    uint16_t age, peerAge, heartbeat;
    bool peerReady, dirtyStop;
    uint32_t serial, token, stoppedToken;
    CtrCoopSnapshot trail[FOLLOW_TRAIL_SIZE], last;
    uint8_t head, count;
    bool haveLast;
} sFollow;

static bool FollowFieldReady(void)
{
    return PartyPeerReady() && gMain.callback2 == CB2_Overworld && !gMain.inBattle
        && !sCoop.battleActive && sCoop.doubleState == DOUBLE_STATE_IDLE
        && !sCoop.pcOpen && !sCoop.pcPending && sCoop.pcOwner == PC_OWNER_NONE
        && sCoop.syncState == PC_SYNC_IDLE && sCoop.battleSendState == BATTLE_SYNC_IDLE
        && sCoop.battleReceiveState == BATTLE_SYNC_IDLE && !sCoop.eventStarting
        && sCoop.eventState == EVENT_IDLE && !ArePlayerFieldControlsLocked()
        && !ScriptContext_IsEnabled() && !CtrCoop_IsWatching()
        && CtrCoopInventoryBridge_Ready() && CtrCoopMoney_Ready();
}

static bool FollowSend(uint8_t op, uint32_t token)
{
    uint8_t packet[CTR_COOP_FOLLOW_SIZE] = {CTR_COOP_FOLLOW_PACKET, CTR_COOP_PROTOCOL_VERSION,
        op, FollowFieldReady() ? 1 : 0};
    WriteDouble32(packet, 4, token);
    return sCoop.peerId && LocalLink_TrySend(sCoop.peerId, packet, sizeof(packet));
}

static void FollowReset(void)
{
    /* Never reuse a token on a new connection within the same process. */
    uint32_t serial = sFollow.serial;
    memset(&sFollow, 0, sizeof(sFollow));
    sFollow.serial = serial;
    sFollow.peerAge = 0xFFFF;
}

static void FollowCancelBattleInvite(void)
{
    if (sCoop.doubleState == DOUBLE_STATE_WAIT_ACCEPT || sCoop.doubleState == DOUBLE_STATE_SENDING)
        sCoop.doubleState = DOUBLE_STATE_LOCAL;
    else if (sCoop.doubleState == DOUBLE_STATE_WAIT_START || sCoop.doubleState == DOUBLE_STATE_RECEIVING)
        sCoop.doubleState = DOUBLE_STATE_IDLE;
}

void CtrCoop_StopFollowing(void)
{
    if (sFollow.state == FOLLOW_NONE)
        return;
    sFollow.stoppedToken = sFollow.token;
    sFollow.dirtyStop = !FollowSend(FOLLOW_STOP, sFollow.token);
    sFollow.state = FOLLOW_NONE;
    FollowCancelBattleInvite();
    sFollow.count = 0;
    sFollow.haveLast = false;
}

bool CtrCoop_IsFollowing(void)
{
    return sFollow.state == FOLLOW_FOLLOWER || sFollow.state == FOLLOW_PENDING;
}

static bool FollowPaired(void)
{
    return sFollow.state == FOLLOW_LEADER || sFollow.state == FOLLOW_FOLLOWER;
}

static bool FollowBattlePartner(void)
{
    return FollowPaired() && sFollow.state == FOLLOW_LEADER && sFollow.peerReady && sFollow.peerAge < 180;
}

bool CtrCoop_RestrictStoryParty(void)
{
    return sFollow.state == FOLLOW_LEADER && PartyPeerReady()
        && LocalLink_GetSession() == LOCAL_SESSION_IN && LocalLink_GetSessionKind() == LOCAL_KIND_COOP;
}

static void FollowRecord(const CtrCoopSnapshot *pose)
{
    if (sFollow.state != FOLLOW_FOLLOWER)
        return;
    if (sFollow.haveLast && CtrCoopProtocol_SameMap(&sFollow.last, pose)
     && sFollow.last.x == pose->x && sFollow.last.y == pose->y)
        return;
    /* Keep actual visited tiles, including turns. Never cut a diagonal across
     * walls to chase only the most recent network coordinate. */
    if (sFollow.count == FOLLOW_TRAIL_SIZE)
    {
        CtrCoop_StopFollowing();
        return;
    }
    sFollow.trail[(sFollow.head + sFollow.count) % FOLLOW_TRAIL_SIZE] = *pose;
    ++sFollow.count;
    sFollow.last = *pose;
    sFollow.haveLast = true;
}

bool CtrCoop_TryFollowAt(int16_t x, int16_t y)
{
    CtrCoopSnapshot local;
    if (!FollowFieldReady() || sFollow.state != FOLLOW_NONE || !sCoop.remoteValid
     || !LocalSnapshot(&local) || !CtrCoopProtocol_SameMap(&local, &sCoop.remote)
     || sCoop.remote.x != x || sCoop.remote.y != y)
        return false;
    sFollow.serial = (sFollow.serial + 1) & 0x7FFFFFFFu;
    if (!sFollow.serial) ++sFollow.serial;
    sFollow.token = sFollow.serial | (sCoop.guest ? 0x80000000u : 0);
    sFollow.state = FOLLOW_PENDING;
    sFollow.age = sFollow.retry = 0;
    FollowSend(FOLLOW_JOIN, sFollow.token);
    return true;
}

static bool HandleFollow(const uint8_t *packet, uint16_t size)
{
    if (size != CTR_COOP_FOLLOW_SIZE || packet[1] != CTR_COOP_PROTOCOL_VERSION || packet[3] > 1)
        return true;
    uint8_t op = packet[2];
    uint32_t token = ReadDouble32(packet, 4);
    if (op < FOLLOW_JOIN || op > FOLLOW_STATUS)
        return true;
    sFollow.peerReady = packet[3] != 0;
    sFollow.peerAge = 0;
    if (op == FOLLOW_STATUS)
        return true;
    if (!token)
        return true;
    if (op == FOLLOW_JOIN)
    {
        if (sFollow.state == FOLLOW_LEADER && sFollow.token == token)
            FollowSend(FOLLOW_ACCEPT, token);
        else if (FollowFieldReady() && (sFollow.state == FOLLOW_NONE
              || (sCoop.guest && sFollow.state == FOLLOW_PENDING)))
        {
            /* Simultaneous A: host's request wins, so no following cycle. */
            sFollow.state = FOLLOW_LEADER;
            sFollow.token = token;
            sFollow.age = 0;
            sFollow.count = 0;
            FollowSend(FOLLOW_ACCEPT, token);
        }
        else
            FollowSend(FOLLOW_DENY, token);
        return true;
    }
    if (token != sFollow.token)
        return true;
    if (op == FOLLOW_ACCEPT && sFollow.state == FOLLOW_PENDING)
    {
        sFollow.state = FOLLOW_FOLLOWER;
        sFollow.count = sFollow.head = 0;
        sFollow.haveLast = false;
        if (sCoop.remoteValid) FollowRecord(&sCoop.remote);
    }
    else if ((op == FOLLOW_DENY && sFollow.state == FOLLOW_PENDING) || op == FOLLOW_STOP)
    {
        sFollow.state = FOLLOW_NONE;
        FollowCancelBattleInvite();
        sFollow.count = 0;
        sFollow.haveLast = false;
    }
    return true;
}

static void FollowTick(void)
{
    if (sFollow.peerAge != 0xFFFF) ++sFollow.peerAge;
    if (!PartyPeerReady())
        return;
    if (sFollow.dirtyStop && FollowSend(FOLLOW_STOP, sFollow.stoppedToken))
        sFollow.dirtyStop = false;
    if (++sFollow.heartbeat >= 15)
    {
        if (FollowSend(FOLLOW_STATUS, sFollow.token)) sFollow.heartbeat = 0;
    }
    if (sFollow.state == FOLLOW_PENDING)
    {
        if (++sFollow.age >= 300)
            CtrCoop_StopFollowing();
        else if (++sFollow.retry >= 30)
        {
            sFollow.retry = 0;
            FollowSend(FOLLOW_JOIN, sFollow.token);
        }
    }
    if (sCoop.remoteValid) FollowRecord(&sCoop.remote);
}

/* Called before native field input decoding. B remains responsive even when
 * the leader is in dialogue. Native collision/ledge/warp movement is retained. */
void CtrCoop_FollowKeys(uint16_t *pressed, uint16_t *held)
{
    CtrCoopSnapshot local;
    if (!CtrCoop_IsFollowing()) return;
    if (*pressed & 2) /* B_BUTTON */
    {
        CtrCoop_StopFollowing();
        *pressed &= ~2;
        *held &= ~2;
        return;
    }
    *pressed = *held = 0;
    if (sFollow.state != FOLLOW_FOLLOWER || sCoop.doubleState != DOUBLE_STATE_IDLE || !LocalSnapshot(&local)) return;
    while (sFollow.count)
    {
        CtrCoopSnapshot *target = &sFollow.trail[sFollow.head];
        if (!CtrCoopProtocol_SameMap(&local, target))
        {
            if (FollowFieldReady() && sFollow.peerReady)
                CtrCoop_EngineFollowWarp(target->mapGroup, target->mapNum, target->mapLayoutId,
                    target->x, target->y);
            return;
        }
        if (target->x == local.x && target->y == local.y)
        {
            sFollow.head = (sFollow.head + 1) % FOLLOW_TRAIL_SIZE;
            --sFollow.count;
            continue;
        }
        /* The newest tile is occupied by the leader. Follow one tile behind. */
        int dx = target->x-local.x, dy = target->y-local.y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if (sFollow.count == 1 && dx + dy <= 1)
            return;
        if (target->x < local.x) *held = 0x20;
        else if (target->x > local.x) *held = 0x10;
        else if (target->y < local.y) *held = 0x40;
        else if (target->y > local.y) *held = 0x80;
        return;
    }
}
