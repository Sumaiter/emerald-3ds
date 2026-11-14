/* Visual-only ownership stream. Latest targets coalesce under backpressure;
 * native held movement animates catch-up, never a second field script. */
struct NpcMotionLocal {
    CtrCoopNpcMotionPose pose;
    bool used, dirty;
};
struct NpcMotionRemote {
    CtrCoopNpcMotionPose pose;
    CtrCoopNpcMotionPlayback playback;
    bool used, ending;
};
static struct NpcMotionLocal sNpcMotionLocal[OBJECT_EVENTS_COUNT];
static struct NpcMotionRemote sNpcMotionRemote[OBJECT_EVENTS_COUNT];
static uint32_t sNpcMotionTxToken, sNpcMotionTxSequence, sNpcMotionRxToken, sNpcMotionRxSequence;
static uint8_t sNpcMotionCursor;
static bool sNpcMotionTxEnding, sNpcMotionRxClosed;

static bool MotionKey(const CtrCoopNpcMotionPose *p, uint8_t g, uint8_t n, uint8_t id)
{ return p->group == g && p->num == n && (!id || p->id == id); }
static bool MotionReady(void)
{
    return sCoop.wanted && sCoop.peerId && gSaveBlock1Ptr
        && (sCoop.guest ? sCoop.authenticated && sCoop.saveReady : sCoop.peerAuthenticated && sCoop.peerSaveReady);
}
static bool MotionPoseValid(const CtrCoopNpcMotionPose *p)
{
    return p->id && p->id < 254 && p->id != LOCALID_COOP_PLAYER && p->facing >= 1 && p->facing <= 4
        && p->x >= 0 && p->y >= 0 && p->x <= 1023 && p->y <= 1023
        && p->graphics <= CTR_COOP_GRAPHICS_ID_MAX && p->flags <= 3
        && (p->action <= 0x9D || p->action == 0xFF);
}
static bool MotionSame(const CtrCoopNpcMotionPose *a, const CtrCoopNpcMotionPose *b)
{
    return a->x == b->x && a->y == b->y && a->facing == b->facing && a->graphics == b->graphics
        && a->action == b->action && a->flags == b->flags && a->actionSerial == b->actionSerial;
}
static void MotionCapture(void)
{
    if (gMain.callback2 != CB2_Overworld) return;
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i)
    {
        struct NpcMotionLocal *actor = &sNpcMotionLocal[i];
        CtrCoopNpcMotionPose pose;
        if (!actor->used || !CtrCoop_EngineNpcMotionCapture(actor->pose.group, actor->pose.num, actor->pose.id, &pose)) continue;
        pose.actionSerial = actor->pose.actionSerial;
        if (MotionPoseValid(&pose) && !MotionSame(&pose, &actor->pose))
        { actor->pose = pose; actor->dirty = true; }
    }
}
void CtrCoopNpcMotion_Track(uint8_t group, uint8_t num, uint8_t id)
{
    struct NpcMotionLocal *slot = NULL;
    CtrCoopNpcMotionPose pose;
    if (!MotionReady() || sCoop.eventState != EVENT_HELD || sCoop.eventEnding || sNpcMotionTxEnding
     || !id || id >= 254 || id == LOCALID_COOP_PLAYER || !OwnsEncounterNpc(group, num, id)
     || !CtrCoop_EngineNpcMotionCapture(group, num, id, &pose) || !MotionPoseValid(&pose)) return;
    if (sNpcMotionTxToken != sCoop.eventToken)
    {
        memset(sNpcMotionLocal, 0, sizeof(sNpcMotionLocal));
        sNpcMotionTxToken = sCoop.eventToken; sNpcMotionTxSequence = 0;
    }
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i)
    {
        if (sNpcMotionLocal[i].used && MotionKey(&sNpcMotionLocal[i].pose, group, num, id)) return;
        if (!sNpcMotionLocal[i].used) slot = &sNpcMotionLocal[i];
    }
    if (!slot)
        for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i)
            if (!sNpcMotionLocal[i].dirty
             && !CtrCoop_EngineNpcMotionCapture(sNpcMotionLocal[i].pose.group, sNpcMotionLocal[i].pose.num,
                                               sNpcMotionLocal[i].pose.id, &pose))
            { slot = &sNpcMotionLocal[i]; break; }
    if (!slot || !CtrCoop_EngineNpcMotionCapture(group, num, id, &pose)) return;
    pose.actionSerial = 0; slot->pose = pose; slot->used = slot->dirty = true;
}
void CtrCoopNpcMotion_Started(uint8_t group, uint8_t num, uint8_t id)
{
    CtrCoopNpcMotion_Track(group, num, id);
    if (!MotionReady() || sCoop.eventState != EVENT_HELD || sCoop.eventEnding || sNpcMotionTxEnding) return;
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i)
        if (sNpcMotionLocal[i].used && MotionKey(&sNpcMotionLocal[i].pose, group, num, id))
        { if (++sNpcMotionLocal[i].pose.actionSerial == 0) ++sNpcMotionLocal[i].pose.actionSerial;
          sNpcMotionLocal[i].dirty = true; return; }
}
void CtrCoopNpcMotion_End(void)
{
    if (!sNpcMotionTxToken) return;
    MotionCapture(); sNpcMotionTxEnding = true;
}
bool CtrCoopNpcMotion_Pending(void)
{
    if (sNpcMotionTxEnding) return true;
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i) if (sNpcMotionLocal[i].dirty) return true;
    return false;
}
static uint32_t MotionGet32(const uint8_t *p)
{ return p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static void MotionPut32(uint8_t *p, uint32_t value)
{ for (unsigned i = 0; i < 4; ++i) p[i] = value >> (8 * i); }
static void MotionSend(void)
{
    if (!MotionReady() || !sNpcMotionTxToken || sNpcMotionTxSequence == UINT32_MAX) return;
    for (unsigned n = 0, sent = 0; n < OBJECT_EVENTS_COUNT && sent < 2; ++n)
    {
        struct NpcMotionLocal *actor = &sNpcMotionLocal[sNpcMotionCursor];
        sNpcMotionCursor = (sNpcMotionCursor + 1) % OBJECT_EVENTS_COUNT;
        if (actor->used && actor->dirty)
        {
            const CtrCoopNpcMotionPose *p = &actor->pose;
            uint8_t packet[CTR_COOP_NPC_MOTION_SIZE] = {CTR_COOP_NPC_MOTION_PACKET, CTR_COOP_PROTOCOL_VERSION};
            MotionPut32(packet + 2, sNpcMotionTxToken); MotionPut32(packet + 6, sNpcMotionTxSequence + 1);
            packet[10] = p->group; packet[11] = p->num; packet[12] = p->id; packet[13] = p->facing;
            packet[14] = p->x; packet[15] = (uint16_t)p->x >> 8; packet[16] = p->y; packet[17] = (uint16_t)p->y >> 8;
            packet[18] = p->action; packet[19] = p->flags; MotionPut32(packet + 20, p->actionSerial);
            packet[24] = p->graphics;
            if (!LocalLink_TrySend(sCoop.peerId, packet, sizeof(packet))) return;
            ++sNpcMotionTxSequence; actor->dirty = false; ++sent;
            if (sNpcMotionTxSequence == UINT32_MAX) return;
        }
    }
    if (sNpcMotionTxEnding)
    {
        for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i) if (sNpcMotionLocal[i].dirty) return;
        uint8_t packet[CTR_COOP_NPC_MOTION_END_SIZE] = {CTR_COOP_NPC_MOTION_END_PACKET, CTR_COOP_PROTOCOL_VERSION};
        MotionPut32(packet + 2, sNpcMotionTxToken); MotionPut32(packet + 6, sNpcMotionTxSequence + 1);
        if (!LocalLink_TrySend(sCoop.peerId, packet, sizeof(packet))) return;
        memset(sNpcMotionLocal, 0, sizeof(sNpcMotionLocal));
        sNpcMotionTxEnding = false; sNpcMotionTxToken = sNpcMotionTxSequence = 0;
    }
}
bool CtrCoopNpcMotion_Controls(uint8_t group, uint8_t num, uint8_t id)
{
    /* Suppress the old position-only path on BOTH ends. In particular a host
     * must not publish a teleport before this actor's first admitted pose. */
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i)
        if (sNpcMotionLocal[i].used && MotionKey(&sNpcMotionLocal[i].pose, group, num, id)) return true;
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i)
        if (sNpcMotionRemote[i].used && MotionKey(&sNpcMotionRemote[i].pose, group, num, id)) return true;
    return false;
}
static bool MotionSettlingAt(uint8_t group, uint8_t num, uint8_t id)
{
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i)
        if (sNpcMotionRemote[i].used && sNpcMotionRemote[i].ending
         && MotionKey(&sNpcMotionRemote[i].pose, group, num, id)) return true;
    return false;
}
bool CtrCoopNpcMotion_Settling(void)
{
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i)
        if (sNpcMotionRemote[i].used && sNpcMotionRemote[i].ending) return true;
    return false;
}
void CtrCoopNpcMotion_Tick(void)
{
    if (MotionReady()) { MotionCapture(); MotionSend(); }
    if (!gSaveBlock1Ptr || gMain.callback2 != CB2_Overworld) return;
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i)
    {
        struct NpcMotionRemote *actor = &sNpcMotionRemote[i];
        if (!actor->used) continue;
        if (actor->pose.group != gSaveBlock1Ptr->location.mapGroup || actor->pose.num != gSaveBlock1Ptr->location.mapNum)
        { actor->used = false; continue; }
        if (sCoop.eventState == EVENT_HELD && !sCoop.eventStarting
         && OwnsEncounterNpc(actor->pose.group, actor->pose.num, actor->pose.id)) continue;
        int settled = CtrCoop_EngineNpcMotionApply(&actor->pose, &actor->playback, actor->ending);
        if (settled < 0 || (settled && actor->ending)) actor->used = false;
    }
}
bool CtrCoopNpcMotion_HandlePacket(uint16_t from, const uint8_t *data, uint16_t len)
{
    CtrCoopNpcMotionPose pose = {0};
    bool end;
    if (!data || !len || (data[0] != CTR_COOP_NPC_MOTION_PACKET && data[0] != CTR_COOP_NPC_MOTION_END_PACKET)) return false;
    end = data[0] == CTR_COOP_NPC_MOTION_END_PACKET;
    if (!MotionReady() || from != sCoop.peerId || len != (end ? CTR_COOP_NPC_MOTION_END_SIZE : CTR_COOP_NPC_MOTION_SIZE)
     || data[1] != CTR_COOP_PROTOCOL_VERSION) return true;
    uint32_t token = MotionGet32(data + 2), seq = MotionGet32(data + 6);
    if (!token || !seq || (!sCoop.guest && (token != sCoop.peerEventToken || !sCoop.peerEventCount))) return true;
    if (!end)
    {
        pose = (CtrCoopNpcMotionPose){.group=data[10],.num=data[11],.id=data[12],.facing=data[13],
            .x=(int16_t)(data[14] | ((uint16_t)data[15] << 8)),.y=(int16_t)(data[16] | ((uint16_t)data[17] << 8)),
            .action=data[18],.flags=data[19],.actionSerial=MotionGet32(data + 20),.graphics=data[24]};
        if (data[25] || !MotionPoseValid(&pose)) return true;
        if (!sCoop.guest)
        {
            bool mapAllowed = false;
            for (unsigned i = 0; i < sCoop.peerEventCount; ++i)
                if (sCoop.peerEventKeys[i].mapGroup == pose.group && sCoop.peerEventKeys[i].mapNum == pose.num) mapAllowed = true;
            if (!mapAllowed) return true;
        }
        if (sCoop.eventState == EVENT_HELD && !sCoop.eventStarting && OwnsEncounterNpc(pose.group, pose.num, pose.id)) return true;
    }
    if (token != sNpcMotionRxToken)
    {
        if (sNpcMotionRxToken && (int32_t)(token - sNpcMotionRxToken) <= 0) return true;
        for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i) if (sNpcMotionRemote[i].used) sNpcMotionRemote[i].ending = true;
        sNpcMotionRxToken = token; sNpcMotionRxSequence = 0; sNpcMotionRxClosed = false;
    }
    if (sNpcMotionRxClosed || seq <= sNpcMotionRxSequence) return true;
    if (end)
    {
        sNpcMotionRxSequence = seq; sNpcMotionRxClosed = true;
        for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i) if (sNpcMotionRemote[i].used) sNpcMotionRemote[i].ending = true;
        return true;
    }
    struct NpcMotionRemote *slot = NULL;
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i)
    {
        if (sNpcMotionRemote[i].used && MotionKey(&sNpcMotionRemote[i].pose, pose.group, pose.num, pose.id))
        { slot = &sNpcMotionRemote[i]; break; }
        if (!sNpcMotionRemote[i].used) slot = &sNpcMotionRemote[i];
    }
    if (!slot) return true;
    if (!slot->used) memset(slot, 0, sizeof(*slot));
    slot->pose = pose; slot->used = true; slot->ending = false; sNpcMotionRxSequence = seq;
    return true;
}
void CtrCoopNpcMotion_Reset(void)
{
    memset(sNpcMotionLocal, 0, sizeof(sNpcMotionLocal));
    sNpcMotionTxToken = sNpcMotionTxSequence = 0; sNpcMotionTxEnding = false;
    /* A new authenticated guest process starts its event serial at one.
     * Keep accepted visual tails, but retire the old session watermark. */
    sNpcMotionRxToken = sNpcMotionRxSequence = 0; sNpcMotionRxClosed = false;
    for (unsigned i = 0; i < OBJECT_EVENTS_COUNT; ++i) if (sNpcMotionRemote[i].used) sNpcMotionRemote[i].ending = true;
}
void CtrCoopNpcMotion_AfterStackReset(void)
{
    memset(sNpcMotionLocal, 0, sizeof(sNpcMotionLocal)); memset(sNpcMotionRemote, 0, sizeof(sNpcMotionRemote));
    sNpcMotionTxToken = sNpcMotionTxSequence = sNpcMotionRxToken = sNpcMotionRxSequence = 0;
    sNpcMotionTxEnding = sNpcMotionRxClosed = false; sNpcMotionCursor = 0;
}
