/* Local story intents survive stale host echoes until the ordered event
 * release/ACK boundary. Special vars (notably VAR_RESULT) are never shared. */
#define STORY_OBJECT_PACKET 75
#define STORY_OBJECT_PACKET_SIZE 15
#define STORY_OBJECT_LIMIT 128
/* Native TEMP_FLAGS_START..END and TEMP_VARS_START..END are map-local.
 * Sharing them lets an unrelated map load reset an in-progress scene. */
#define STORY_FIRST_FLAG 0x20
#define STORY_FIRST_VAR (VARS_START + 0x10)
struct StoryObject
{
    uint8_t group, num, id, fields, movement;
    int16_t x, y;
    uint32_t token;
    bool dirty, pendingLive;
};
static struct StoryObject sStoryObjects[STORY_OBJECT_LIMIT];
static unsigned sStoryObjectCount;
static bool sStoryObjectOverflow, sApplyingStoryWorld;
static uint8_t sStoryFlagsTouched[NUM_FLAG_BYTES];
static uint8_t sStoryVarsTouched[(VARS_COUNT + 7) / 8];
static uint32_t sStoryEpoch, sStoryPreparedEpoch;

static bool StoryLocalIntent(void)
{
    return sCoop.eventState == EVENT_HELD || sCoop.eventState == EVENT_RELEASING;
}

void CtrCoop_StoryFlagChanged(uint16_t id)
{
    if (!sApplyingStoryWorld && StoryLocalIntent() && id >= STORY_FIRST_FLAG && id < FLAGS_COUNT)
        sStoryFlagsTouched[id / 8] |= 1 << (id & 7);
}

void CtrCoop_StoryVarChanged(uint16_t id)
{
    if (!sApplyingStoryWorld && StoryLocalIntent() && id >= STORY_FIRST_VAR && id < VARS_START + VARS_COUNT)
    {
        unsigned index = id - VARS_START;
        sStoryVarsTouched[index / 8] |= 1 << (index & 7);
    }
}

static struct StoryObject *StoryObjectSlot(uint8_t group, uint8_t num, uint8_t id)
{
    struct StoryObject *slot = NULL;
    for (unsigned i = 0; i < sStoryObjectCount; ++i)
    {
        struct StoryObject *object = &sStoryObjects[i];
        if (object->group == group && object->num == num && object->id == id)
            return object;
        if (!object->dirty && gSaveBlock1Ptr != NULL
         && (object->group != gSaveBlock1Ptr->location.mapGroup || object->num != gSaveBlock1Ptr->location.mapNum))
            slot = object;
    }
    if (sStoryObjectCount < STORY_OBJECT_LIMIT)
        slot = &sStoryObjects[sStoryObjectCount++];
    if (slot)
    {
        memset(slot, 0, sizeof(*slot));
        slot->group = group; slot->num = num; slot->id = id;
    }
    return slot;
}

static void MergeStoryObject(struct StoryObject *object, uint8_t fields, int16_t x, int16_t y, uint8_t movement)
{
    if (fields & 1) { object->x = x; object->y = y; }
    if (fields & 2) object->movement = movement;
    if (fields & 12) object->fields &= ~12;
    object->fields |= fields;
}

void CtrCoop_StoryObject(uint8_t group, uint8_t num, uint8_t id, uint8_t fields,
                         int16_t x, int16_t y, uint8_t movement)
{
    struct StoryObject *object;
    if (!sCoop.wanted || sApplyingStoryWorld || sCoop.eventState != EVENT_HELD
     || !id || id == 255 || id == LOCALID_COOP_PLAYER || !fields || fields > 15 || (fields & 12) == 12)
        return;
    object = StoryObjectSlot(group, num, id);
    if (!object)
    {
        sStoryObjectOverflow = true;
        return;
    }
    MergeStoryObject(object, fields, x, y, movement);
    object->token = sCoop.eventToken;
    object->dirty = true;
    CtrCoop_ScriptActor(group, num, id);
}

static bool StoryObjectsPending(void)
{
    if (sStoryObjectOverflow) return true;
    for (unsigned i = 0; i < sStoryObjectCount; ++i)
        if (sStoryObjects[i].dirty) return true;
    return false;
}

static void SyncStoryObjects(uint16_t peer, uint8_t *sent)
{
    for (unsigned i = 0; i < sStoryObjectCount && *sent < 8; ++i)
    {
        struct StoryObject *object = &sStoryObjects[i];
        uint8_t packet[STORY_OBJECT_PACKET_SIZE] = {STORY_OBJECT_PACKET, CTR_COOP_PROTOCOL_VERSION};
        if (!object->dirty) continue;
        for (unsigned b = 0; b < 4; ++b) packet[2 + b] = object->token >> (b * 8);
        packet[6] = object->group; packet[7] = object->num; packet[8] = object->id; packet[9] = object->fields;
        packet[10] = (uint16_t)object->x; packet[11] = (uint16_t)object->x >> 8;
        packet[12] = (uint16_t)object->y; packet[13] = (uint16_t)object->y >> 8;
        packet[14] = object->movement;
        if (!LocalLink_TrySend(peer, packet, sizeof(packet))) return;
        object->dirty = false;
        ++*sent;
    }
}

static bool StoryNpcAvailable(uint8_t group, uint8_t num, uint8_t id)
{
    for (unsigned i = 0; i < sStoryObjectCount; ++i)
        if (sStoryObjects[i].group == group && sStoryObjects[i].num == num && sStoryObjects[i].id == id)
        {
            if (sStoryObjects[i].fields & 8) return false;
            if (sStoryObjects[i].fields & 4) return true;
        }
    return CtrCoop_EngineStoryNpcAvailable(group, num, id);
}

bool CtrCoop_HandleStoryPacket(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint32_t token = 0;
    struct StoryObject *object;
    if (!data || !len || data[0] != STORY_OBJECT_PACKET) return false;
    if (!sCoop.wanted || from != sCoop.peerId
     || (sCoop.guest ? !sCoop.authenticated : !sCoop.peerAuthenticated)
     || len != STORY_OBJECT_PACKET_SIZE || data[1] != CTR_COOP_PROTOCOL_VERSION
     || !data[8] || data[8] == 255 || data[8] == LOCALID_COOP_PLAYER
     || !data[9] || data[9] > 15 || (data[9] & 12) == 12) return true;
    for (unsigned b = 0; b < 4; ++b) token |= (uint32_t)data[2 + b] << (b * 8);
    if (!token || (!sCoop.guest && (token != sCoop.peerEventToken || !sCoop.peerEventCount))) return true;
    object = StoryObjectSlot(data[6], data[7], data[8]);
    if (!object || (sCoop.guest && object->dirty)) return true;
    MergeStoryObject(object, data[9], (int16_t)(data[10] | ((uint16_t)data[11] << 8)),
                     (int16_t)(data[12] | ((uint16_t)data[13] << 8)), data[14]);
    object->token = token;
    object->dirty = false;
    object->pendingLive = true;
    ++sStoryEpoch;
    return true;
}

void CtrCoop_ResetStory(void)
{
    /* Disconnect autosave must retain accepted world changes. The sprite pool
     * may currently belong to a battle/menu: materialize save metadata only. */
    sApplyingStoryWorld = true;
    if (gSaveBlock1Ptr != NULL)
        for (unsigned i = 0; i < sStoryObjectCount; ++i)
        {
            struct StoryObject *object = &sStoryObjects[i];
            CtrCoop_EngineStoryObject(object->group, object->num, object->id,
                object->fields | (gMain.callback2 == CB2_Overworld
                    && !OwnsEncounterNpc(object->group, object->num, object->id)
                    && !CtrCoopNpcMotion_Controls(object->group, object->num, object->id) ? 0 : 16),
                object->x, object->y, object->movement);
            object->dirty = false;
        }
    sStoryObjectOverflow = false;
    sApplyingStoryWorld = false;
    memset(sStoryFlagsTouched, 0, sizeof(sStoryFlagsTouched));
    memset(sStoryVarsTouched, 0, sizeof(sStoryVarsTouched));
    sStoryPreparedEpoch = ++sStoryEpoch;
}

void CtrCoop_ClearStoryWorld(void)
{
    sStoryObjectCount = 0;
    CtrCoop_ResetStory();
}
