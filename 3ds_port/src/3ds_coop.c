#include "global.h"
#include "3ds_coop.h"
#include "3ds_coop_protocol.h"
#include "3ds_coop_profile.h"
#include "3ds_local.h"
#include "3ds_log.h"
#include "characters.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "constants/local_link.h"
#include "constants/layouts.h"
#include "constants/map_groups.h"
#include "decoration_inventory.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "item.h"
#include "load_save.h"
#include "overworld.h"
#include "sound.h"
#include "constants/songs.h"

static struct
{
    bool wanted;
    bool guest;
    bool authenticated;
    bool authSent;
    bool authRejected;
    bool peerAuthenticated;
    uint8_t peerName[CTR_COOP_PLAYER_NAME_SIZE];
    bool savePrepared;
    bool saveReady;
    bool peerSaveReady;
    uint32_t saveOffset;
    uint32_t saveChecksum;
    bool spawnValid;
    bool joinWarpPending;
    CtrCoopSnapshot spawn;
    uint32_t joinToken;
    uint32_t generation;
    bool haveLastSent;
    uint16_t framesSinceLast;
    CtrCoopSnapshot lastSent;
    uint16_t peerId;
    bool remoteValid;
    CtrCoopSnapshot remote;
    uint8_t objectEventId;
    bool haveApplied;
    CtrCoopSnapshot applied;
} sCoop = {.objectEventId = OBJECT_EVENTS_COUNT};

#define CTR_COOP_SAVE_SIZE (sizeof(struct SaveBlock2) + sizeof(struct SaveBlock1) + sizeof(struct PokemonStorage))

static uint8_t sSaveData[CTR_COOP_SAVE_SIZE];

static bool SnapshotsEqual(const CtrCoopSnapshot *a, const CtrCoopSnapshot *b)
{
    return !CtrCoopProtocol_ShouldSend(a, b, 0);
}

static void ClearRemote(void)
{
    if (sCoop.joinWarpPending)
        CtrLog_Write(CTR_LOG_GAME, "coop: join warp interrupted");
    sCoop.peerId = 0;
    sCoop.remoteValid = false;
    sCoop.haveApplied = false;
    sCoop.savePrepared = false;
    sCoop.saveReady = false;
    sCoop.peerSaveReady = false;
    sCoop.saveOffset = 0;
    sCoop.saveChecksum = 0;
    sCoop.spawnValid = false;
    sCoop.joinWarpPending = false;
    memset(sCoop.peerName, 0xFF, sizeof(sCoop.peerName));
}

#define SPAWN_TABLE_MAGIC 0x5350
#define SPAWN_TABLE_VERSION 1
#define SPAWN_RECORD_COUNT 4

struct SpawnRecord
{
    uint8_t name[CTR_COOP_PLAYER_NAME_SIZE];
    uint8_t used;
    uint8_t mapGroup;
    uint8_t mapNum;
    uint16_t mapLayoutId;
    int16_t x;
    int16_t y;
    uint8_t facing;
    uint8_t graphicsId;
    uint8_t reserved[3];
};

struct SpawnTable
{
    uint16_t magic;
    uint16_t version;
    struct SpawnRecord records[SPAWN_RECORD_COUNT];
};

static const uint8_t sDefaultGuestName[] = {'G', 'A', 'S', 'T', 0xFF, 0xFF, 0xFF, 0xFF};

static bool LocalSnapshot(CtrCoopSnapshot *out);

static void GuestName(uint8_t *out)
{
    const struct CtrCoopProfile *profile = CtrCoopProfile_Get();

    if (profile->guestNameLength > 0 && profile->guestNameLength <= CTR_COOP_GUEST_NAME_LENGTH)
        memcpy(out, profile->guestName, CTR_COOP_PLAYER_NAME_SIZE);
    else
        memcpy(out, sDefaultGuestName, CTR_COOP_PLAYER_NAME_SIZE);
}

static struct SpawnTable *SpawnTable(void)
{
    if (gSaveBlock1Ptr == NULL)
        return NULL;
    return (struct SpawnTable *)gSaveBlock1Ptr->unused_3598;
}

static struct SpawnTable *ValidSpawnTable(void)
{
    struct SpawnTable *table = SpawnTable();

    if (table == NULL)
        return NULL;
    if (table->magic != SPAWN_TABLE_MAGIC || table->version != SPAWN_TABLE_VERSION)
    {
        memset(table, 0, sizeof(*table));
        table->magic = SPAWN_TABLE_MAGIC;
        table->version = SPAWN_TABLE_VERSION;
    }
    return table;
}

static struct SpawnRecord *FindSpawnRecord(const uint8_t *name, bool create)
{
    struct SpawnTable *table = ValidSpawnTable();
    struct SpawnRecord *freeRecord = NULL;

    if (table == NULL)
        return NULL;
    for (u8 i = 0; i < SPAWN_RECORD_COUNT; i++)
    {
        if (table->records[i].used && memcmp(table->records[i].name, name, CTR_COOP_PLAYER_NAME_SIZE) == 0)
            return &table->records[i];
        if (!table->records[i].used && freeRecord == NULL)
            freeRecord = &table->records[i];
    }
    if (!create || freeRecord == NULL)
        return NULL;
    freeRecord->used = TRUE;
    memcpy(freeRecord->name, name, CTR_COOP_PLAYER_NAME_SIZE);
    return freeRecord;
}

static void SnapshotFromRecord(CtrCoopSnapshot *snapshot, const struct SpawnRecord *record)
{
    *snapshot = (CtrCoopSnapshot){0};
    snapshot->mapGroup = record->mapGroup;
    snapshot->mapNum = record->mapNum;
    snapshot->mapLayoutId = record->mapLayoutId;
    snapshot->x = record->x;
    snapshot->y = record->y;
    snapshot->facing = record->facing;
    snapshot->graphicsId = record->graphicsId;
    snapshot->movementActionId = GetFaceDirectionMovementAction(record->facing);
}

static void StoreSpawnRecord(const CtrCoopSnapshot *snapshot)
{
    struct SpawnRecord *record = FindSpawnRecord(sCoop.peerName, TRUE);
    bool changed;

    if (record == NULL)
        return;
    changed = record->mapGroup != snapshot->mapGroup || record->mapNum != snapshot->mapNum
           || record->mapLayoutId != snapshot->mapLayoutId || record->x != snapshot->x
           || record->y != snapshot->y || record->facing != snapshot->facing
           || record->graphicsId != snapshot->graphicsId;
    record->mapGroup = snapshot->mapGroup;
    record->mapNum = snapshot->mapNum;
    record->mapLayoutId = snapshot->mapLayoutId;
    record->x = snapshot->x;
    record->y = snapshot->y;
    record->facing = snapshot->facing;
    record->graphicsId = snapshot->graphicsId;
    if (changed)
        CtrLog_Write(CTR_LOG_GAME, "coop: store %02x%02x%02x map %u.%u layout %u at %d,%d facing %u gfx %u",
            record->name[0], record->name[1], record->name[2], record->mapGroup, record->mapNum,
            record->mapLayoutId, record->x, record->y, record->facing, record->graphicsId);
}

static void SendGuestIdentity(uint16_t to)
{
    uint8_t name[CTR_COOP_PLAYER_NAME_SIZE];
    uint8_t packet[CTR_COOP_IDENTITY_SIZE];

    GuestName(name);
    CtrLog_Write(CTR_LOG_GAME, "coop: identify as %02x%02x%02x", name[0], name[1], name[2]);
    if (CtrCoopProtocol_EncodeIdentity(name, packet, sizeof(packet)))
        LocalLink_Send(to, packet, sizeof(packet));
}

static void SendSaveRequest(uint16_t to, uint32_t offset)
{
    uint8_t packet[CTR_COOP_SAVE_REQUEST_SIZE];

    if (CtrCoopProtocol_EncodeSaveRequest(offset, packet, sizeof(packet)))
        LocalLink_Send(to, packet, sizeof(packet));
}

static void PrepareHostSave(uint16_t to)
{
    uint8_t packet[CTR_COOP_SAVE_BEGIN_SIZE];
    uint32_t offset = 0;

    if (gSaveBlock2Ptr == NULL || gSaveBlock1Ptr == NULL || gPokemonStoragePtr == NULL)
        return;
    memcpy(sSaveData + offset, gSaveBlock2Ptr, sizeof(*gSaveBlock2Ptr));
    offset += sizeof(*gSaveBlock2Ptr);
    memcpy(sSaveData + offset, gSaveBlock1Ptr, sizeof(*gSaveBlock1Ptr));
    offset += sizeof(*gSaveBlock1Ptr);
    memcpy(sSaveData + offset, gPokemonStoragePtr, sizeof(*gPokemonStoragePtr));
    sCoop.saveOffset = 0;
    sCoop.saveChecksum = CtrCoopProtocol_Checksum(sSaveData, sizeof(sSaveData));
    sCoop.savePrepared = true;
    if (CtrCoopProtocol_EncodeSaveBegin(sizeof(sSaveData), sCoop.saveChecksum, packet, sizeof(packet)))
    {
        CtrLog_Write(CTR_LOG_GAME, "coop: host save begin %lu bytes checksum %08lx",
            (unsigned long)sizeof(sSaveData), (unsigned long)sCoop.saveChecksum);
        LocalLink_Send(to, packet, sizeof(packet));
    }
}

static void SendHostSaveChunk(uint16_t to, uint32_t offset)
{
    uint8_t packet[CTR_COOP_SAVE_CHUNK_MAX];
    uint16_t size;

    if (!sCoop.savePrepared || offset >= sizeof(sSaveData))
        return;
    size = sizeof(sSaveData) - offset;
    if (size > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        size = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (CtrCoopProtocol_EncodeSaveChunk(offset, sSaveData + offset, size, packet, CTR_COOP_SAVE_CHUNK_HEADER + size))
        LocalLink_Send(to, packet, CTR_COOP_SAVE_CHUNK_HEADER + size);
}

static void ApplyHostSave(void)
{
    uint32_t offset = 0;

    memcpy(gSaveBlock2Ptr, sSaveData + offset, sizeof(*gSaveBlock2Ptr));
    offset += sizeof(*gSaveBlock2Ptr);
    memcpy(gSaveBlock1Ptr, sSaveData + offset, sizeof(*gSaveBlock1Ptr));
    offset += sizeof(*gSaveBlock1Ptr);
    memcpy(gPokemonStoragePtr, sSaveData + offset, sizeof(*gPokemonStoragePtr));
    SetBagItemsPointers();
    SetDecorationInventoriesPointers();
    CopyPartyAndObjectsFromSave();
    LoadPlayerBag();
}

static void HandleSaveBegin(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint32_t totalSize, checksum;

    if (!CtrCoopProtocol_DecodeSaveBegin(data, len, &totalSize, &checksum) || totalSize != sizeof(sSaveData))
        return;
    sCoop.savePrepared = true;
    sCoop.saveReady = false;
    sCoop.saveOffset = 0;
    sCoop.saveChecksum = checksum;
    SendSaveRequest(from, 0);
}

static void HandleSaveChunk(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint16_t got, expected;
    uint8_t ready[CTR_COOP_SAVE_READY_SIZE] = {CTR_COOP_SAVE_READY_PACKET, CTR_COOP_PROTOCOL_VERSION};

    if (!sCoop.savePrepared || sCoop.saveReady || sCoop.saveOffset >= sizeof(sSaveData))
        return;
    if (!CtrCoopProtocol_DecodeSaveChunk(data, len, sCoop.saveOffset,
            sSaveData + sCoop.saveOffset, sizeof(sSaveData) - sCoop.saveOffset, &got))
        return;
    expected = sizeof(sSaveData) - sCoop.saveOffset;
    if (expected > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        expected = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (got != expected)
        return;
    sCoop.saveOffset += got;
    if (sCoop.saveOffset < sizeof(sSaveData))
    {
        SendSaveRequest(from, sCoop.saveOffset);
        return;
    }
    if (CtrCoopProtocol_Checksum(sSaveData, sizeof(sSaveData)) != sCoop.saveChecksum)
    {
        CtrLog_Write(CTR_LOG_ERROR, "coop: host save checksum mismatch");
        CtrCoop_Stop();
        return;
    }
    ApplyHostSave();
    sCoop.saveReady = true;
    CtrLog_Write(CTR_LOG_GAME, "coop: host save applied %lu bytes", (unsigned long)sCoop.saveOffset);
    LocalLink_Send(from, ready, sizeof(ready));
}

static void SetFirstSpawn(CtrCoopSnapshot *spawn)
{
    spawn->mapGroup = MAP_GROUP(MAP_LITTLEROOT_TOWN);
    spawn->mapNum = MAP_NUM(MAP_LITTLEROOT_TOWN);
    spawn->mapLayoutId = LAYOUT_LITTLEROOT_TOWN;
    spawn->x = 19;
    spawn->y = 16;
    spawn->facing = DIR_SOUTH;
}

static void SendHostSpawn(uint16_t to)
{
    struct SpawnRecord *record = FindSpawnRecord(sCoop.peerName, FALSE);
    CtrCoopSnapshot spawn;
    uint8_t packet[CTR_COOP_SPAWN_SIZE];

    if (record != NULL)
    {
        SnapshotFromRecord(&spawn, record);
    }
    else
    {
        if (!LocalSnapshot(&spawn))
            return;
        SetFirstSpawn(&spawn);
    }
    spawn.movementActionId = GetFaceDirectionMovementAction(spawn.facing);
    CtrLog_Write(CTR_LOG_GAME, "coop: send %s spawn for %02x%02x%02x map %u.%u layout %u at %d,%d facing %u gfx %u",
        record != NULL ? "saved" : "Littleroot", sCoop.peerName[0], sCoop.peerName[1], sCoop.peerName[2],
        spawn.mapGroup, spawn.mapNum, spawn.mapLayoutId, spawn.x, spawn.y, spawn.facing, spawn.graphicsId);
    if (CtrCoopProtocol_EncodeSpawn(&spawn, packet, sizeof(packet)))
        LocalLink_Send(to, packet, sizeof(packet));
}

bool CtrCoop_EnterHostWorld(void)
{
    CtrCoopSnapshot old;

    if (!sCoop.spawnValid || (sCoop.guest && !sCoop.saveReady))
        return FALSE;
    sCoop.spawnValid = FALSE;
    sCoop.joinWarpPending = true;
    sCoop.generation = LocalLink_Generation();
    if (LocalSnapshot(&old))
        CtrLog_Write(CTR_LOG_GAME, "coop: join warp from map %u.%u at %d,%d to map %u.%u at %d,%d",
            old.mapGroup, old.mapNum, old.x, old.y, sCoop.spawn.mapGroup, sCoop.spawn.mapNum,
            sCoop.spawn.x, sCoop.spawn.y);
    SetWarpDestination(sCoop.spawn.mapGroup, sCoop.spawn.mapNum, -1,
        sCoop.spawn.x - MAP_OFFSET, sCoop.spawn.y - MAP_OFFSET);
    WarpIntoMap();
    return TRUE;
}

static bool RemoteObjectValid(void)
{
    const struct ObjectEvent *objectEvent;

    if (sCoop.objectEventId >= OBJECT_EVENTS_COUNT)
        return false;
    objectEvent = &gObjectEvents[sCoop.objectEventId];
    return objectEvent->active && objectEvent->localId == LOCALID_COOP_PLAYER
        && objectEvent->mapGroup == gSaveBlock1Ptr->location.mapGroup
        && objectEvent->mapNum == gSaveBlock1Ptr->location.mapNum;
}

static void RemoveRemoteObject(void)
{
    if (RemoteObjectValid())
        RemoveObjectEvent(&gObjectEvents[sCoop.objectEventId]);
    sCoop.objectEventId = OBJECT_EVENTS_COUNT;
    sCoop.haveApplied = false;
}

bool CtrCoop_UseCustomGraphicsId(void)
{
    return !sCoop.guest || sCoop.authenticated;
}

uint8_t CtrCoop_LocalGraphicsId(void)
{
    const struct CtrCoopProfile *profile = CtrCoopProfile_Get();

    return sCoop.guest ? profile->guestGraphicsId : profile->hostGraphicsId;
}

static bool LocalSnapshot(CtrCoopSnapshot *out)
{
    const struct ObjectEvent *player;
    uint8_t action;

    if (gSaveBlock1Ptr == NULL || gSaveBlock2Ptr == NULL || gPlayerAvatar.objectEventId >= OBJECT_EVENTS_COUNT)
        return false;
    player = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!player->active)
        return false;
    *out = (CtrCoopSnapshot){0};
    out->mapGroup = gSaveBlock1Ptr->location.mapGroup;
    out->mapNum = gSaveBlock1Ptr->location.mapNum;
    out->mapLayoutId = gMapHeader.mapLayoutId;
    out->x = player->currentCoords.x;
    out->y = player->currentCoords.y;
    out->facing = GetPlayerFacingDirection();
    action = ObjectEventGetHeldMovementActionId((struct ObjectEvent *)player);
    out->movementActionId = action;
    out->graphicsId = CtrCoop_LocalGraphicsId();
    return true;
}

static void SendLocalSnapshot(const CtrCoopSnapshot *local, const struct LocalPeer *peer)
{
    uint8_t packet[CTR_COOP_PACKET_SIZE];

    if (!CtrCoopProtocol_ShouldSend(sCoop.haveLastSent ? &sCoop.lastSent : NULL, local, sCoop.framesSinceLast))
        return;
    if (!CtrCoopProtocol_Encode(local, packet, sizeof(packet)))
    {
        CtrCoopSnapshot stationary = *local;
        stationary.movementActionId = GetFaceDirectionMovementAction(local->facing);
        if (!CtrCoopProtocol_Encode(&stationary, packet, sizeof(packet)))
            return;
        sCoop.lastSent = stationary;
    }
    else
    {
        sCoop.lastSent = *local;
    }
    LocalLink_Send(peer->id, packet, sizeof(packet));
    sCoop.haveLastSent = true;
    sCoop.framesSinceLast = 0;
}

static bool SpawnRemoteObject(void)
{
    struct ObjectEventTemplate objectEventTemplate = {0};
    uint8_t objectEventId;

    if (sCoop.remote.graphicsId >= NUM_OBJ_EVENT_GFX)
        return false;
    if (TryGetObjectEventIdByLocalIdAndMap(LOCALID_COOP_PLAYER,
            gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, &objectEventId) == 0)
    {
        sCoop.objectEventId = objectEventId;
        return false;
    }
    if (!FreeObjectEventSlotNearPlayer())
        return false;
    objectEventTemplate.localId = LOCALID_COOP_PLAYER;
    objectEventTemplate.graphicsId = sCoop.remote.graphicsId;
    objectEventTemplate.kind = OBJ_KIND_NORMAL;
    objectEventTemplate.x = sCoop.remote.x - MAP_OFFSET;
    objectEventTemplate.y = sCoop.remote.y - MAP_OFFSET;
    objectEventTemplate.elevation = MapGridGetElevationAt(sCoop.remote.x, sCoop.remote.y);
    objectEventTemplate.movementType = MOVEMENT_TYPE_FACE_DOWN;
    objectEventTemplate.script = NULL;
    objectEventTemplate.flagId = 0;
    sCoop.objectEventId = SpawnSpecialObjectEvent(&objectEventTemplate);
    if (!RemoteObjectValid())
    {
        sCoop.objectEventId = OBJECT_EVENTS_COUNT;
        return false;
    }
    ObjectEventTurn(&gObjectEvents[sCoop.objectEventId], sCoop.remote.facing);
    sCoop.applied = sCoop.remote;
    sCoop.haveApplied = true;
    return true;
}

static void UpdateRemoteObject(const CtrCoopSnapshot *local)
{
    struct ObjectEvent *objectEvent;
    CtrCoopApply apply;
    bool freshSpawn = false;
    bool alreadyApplied;

    if (!sCoop.remoteValid || !CtrCoopProtocol_SameMap(&sCoop.remote, local))
    {
        RemoveRemoteObject();
        return;
    }
    if (!RemoteObjectValid())
        freshSpawn = SpawnRemoteObject();
    if (!RemoteObjectValid())
        return;
    objectEvent = &gObjectEvents[sCoop.objectEventId];
    if (objectEvent->graphicsId != sCoop.remote.graphicsId)
    {
        RemoveRemoteObject();
        return;
    }
    ObjectEventClearHeldMovementIfFinished(objectEvent);
    alreadyApplied = sCoop.haveApplied && SnapshotsEqual(&sCoop.applied, &sCoop.remote);
    apply = CtrCoopProtocol_PlanMovement(&sCoop.remote, objectEvent->currentCoords.x, objectEvent->currentCoords.y,
        freshSpawn, alreadyApplied, ObjectEventIsHeldMovementActive(objectEvent));
    switch (apply)
    {
    case CTR_COOP_APPLY_START:
        ObjectEventSetHeldMovement(objectEvent, sCoop.remote.movementActionId);
        break;
    case CTR_COOP_APPLY_SNAP:
        MoveObjectEventToMapCoords(objectEvent, sCoop.remote.x, sCoop.remote.y);
        ObjectEventTurn(objectEvent, sCoop.remote.facing);
        break;
    case CTR_COOP_APPLY_FACE:
        ObjectEventTurn(objectEvent, sCoop.remote.facing);
        break;
    case CTR_COOP_APPLY_NOTHING:
        return;
    }
    sCoop.applied = sCoop.remote;
    sCoop.haveApplied = true;
}

static void StartSession(bool searchOnly)
{
    sCoop.wanted = true;
    sCoop.guest = searchOnly;
    sCoop.authenticated = !searchOnly;
    sCoop.authSent = false;
    sCoop.authRejected = false;
    sCoop.peerAuthenticated = false;
    sCoop.generation = LocalLink_Generation();
    sCoop.haveLastSent = false;
    sCoop.framesSinceLast = 0;
    ClearRemote();
    if (searchOnly)
    {
        LocalLink_DiscoverSessions(LOCAL_KIND_COOP);
    }
    else
    {
        LocalLink_SetHostName(gSaveBlock2Ptr ? gSaveBlock2Ptr->playerName : NULL);
        LocalLink_SetHostPassword(CtrCoopProfile_Get()->hostPassword);
        LocalLink_StartSession(LOCAL_KIND_COOP);
    }
    PlaySE(SE_SUCCESS);
}

void CtrCoop_Toggle(void)
{
    if (sCoop.wanted)
        CtrCoop_Stop();
    else if (!CtrCoopProfile_HasHostPassword())
        PlaySE(SE_FAILURE);
    else
        StartSession(false);
}

bool CtrCoop_IsEnabled(void)
{
    return sCoop.wanted;
}

void CtrCoop_Search(void)
{
    if (!sCoop.wanted)
        StartSession(true);
    else if (LocalLink_GetSession() == LOCAL_SESSION_NONE)
        LocalLink_DiscoverSessions(LOCAL_KIND_COOP);
}

void CtrCoop_PrepareJoin(uint32_t token)
{
    sCoop.joinToken = token;
}

void CtrCoop_JoinPrepared(void)
{
    CtrCoop_JoinGame(sCoop.joinToken, CtrCoopProfile_Get()->guestPassword);
}

void CtrCoop_JoinGame(uint32_t token, const uint8_t *password)
{
    sCoop.wanted = true;
    sCoop.guest = true;
    sCoop.authenticated = false;
    sCoop.authSent = false;
    sCoop.authRejected = false;
    sCoop.peerAuthenticated = false;
    sCoop.generation = LocalLink_Generation();
    sCoop.haveLastSent = false;
    sCoop.framesSinceLast = 0;
    ClearRemote();
    LocalLink_JoinSession(LOCAL_KIND_COOP, token, password);
    PlaySE(SE_SUCCESS);
}

void CtrCoop_Stop(void)
{
    sCoop.wanted = false;
    sCoop.authenticated = false;
    sCoop.authSent = false;
    sCoop.authRejected = false;
    sCoop.peerAuthenticated = false;
    ClearRemote();
    LocalLink_LeaveSession();
}

bool CtrCoop_GuestSaveBlocked(void)
{
    return sCoop.wanted && sCoop.guest;
}

uint8_t CtrCoop_GetSessionState(void)
{
    uint8_t state;

    if (!sCoop.wanted)
        return LOCAL_SESSION_NONE;
    if (sCoop.authRejected)
        return LOCAL_SESSION_AUTH_FAILED;
    state = LocalLink_GetSession();
    if (sCoop.guest && state == LOCAL_SESSION_IN
     && (!sCoop.authenticated || !sCoop.saveReady || !sCoop.spawnValid))
        return LOCAL_SESSION_SEARCHING;
    return state;
}

int CtrCoop_GetFoundGames(struct CtrCoopFoundGame *games, int max)
{
    struct LocalFoundSession found[LOCAL_MAX_FOUND];
    int count = LocalLink_GetFoundSessions(found, max < LOCAL_MAX_FOUND ? max : LOCAL_MAX_FOUND);

    for (int i = 0; i < count; i++)
    {
        games[i].token = found[i].token;
        memcpy(games[i].hostName, found[i].hostName, sizeof(games[i].hostName));
        games[i].hostName[7] = EOS;
    }
    return count;
}

void CtrCoop_MenuFrame(void)
{
    struct LocalPeer peers[1];
    uint8_t packet[CTR_COOP_AUTH_PACKET_SIZE];

    if (!sCoop.guest || !sCoop.wanted || sCoop.authenticated || sCoop.authRejected)
        return;
    if (LocalLink_GetSession() != LOCAL_SESSION_IN)
    {
        sCoop.authSent = false;
        return;
    }
    if (sCoop.authSent || LocalLink_GetPeers(peers, 1) == 0)
        return;
    if (CtrCoopProtocol_EncodeAuth(CtrCoopProfile_Get()->guestPassword, packet, sizeof(packet)))
    {
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
        sCoop.authSent = true;
    }
}

void CtrCoop_OverworldFrame(void)
{
    struct LocalPeer peers[1];
    CtrCoopSnapshot local;
    uint32_t generation;
    uint8_t state;
    int peerCount;

    if (!sCoop.wanted && !sCoop.remoteValid && sCoop.objectEventId == OBJECT_EVENTS_COUNT)
        return;
    state = LocalLink_GetSession();
    if (sCoop.wanted && ((state != LOCAL_SESSION_SEARCHING && state != LOCAL_SESSION_IN)
     || (state == LOCAL_SESSION_IN && LocalLink_GetSessionKind() != LOCAL_KIND_COOP)))
    {
        sCoop.wanted = false;
        ClearRemote();
    }
    generation = LocalLink_Generation();
    if (generation != sCoop.generation)
    {
        sCoop.generation = generation;
        sCoop.haveLastSent = false;
        ClearRemote();
    }
    if (!sCoop.remoteValid)
        RemoveRemoteObject();
    if ((sCoop.guest && (!sCoop.authenticated || !sCoop.saveReady))
     || (!sCoop.guest && (!sCoop.peerAuthenticated || !sCoop.peerSaveReady)))
        return;
    if (!LocalSnapshot(&local))
    {
        RemoveRemoteObject();
        return;
    }
    if (sCoop.guest && sCoop.joinWarpPending)
    {
        if (!CtrCoopProtocol_SameMap(&local, &sCoop.spawn)
         || gSaveBlock1Ptr->pos.x != sCoop.spawn.x - MAP_OFFSET
         || gSaveBlock1Ptr->pos.y != sCoop.spawn.y - MAP_OFFSET)
        {
            RemoveRemoteObject();
            return;
        }
        if (local.x != sCoop.spawn.x || local.y != sCoop.spawn.y)
        {
            MoveObjectEventToMapCoords(&gObjectEvents[gPlayerAvatar.objectEventId], sCoop.spawn.x, sCoop.spawn.y);
            local.x = sCoop.spawn.x;
            local.y = sCoop.spawn.y;
        }
        ObjectEventTurn(&gObjectEvents[gPlayerAvatar.objectEventId], sCoop.spawn.facing);
        sCoop.joinWarpPending = false;
        CtrLog_Write(CTR_LOG_GAME, "coop: join warp complete at %d,%d", local.x, local.y);
        local.facing = sCoop.spawn.facing;
        local.movementActionId = GetFaceDirectionMovementAction(local.facing);
    }
    UpdateRemoteObject(&local);
    if (!sCoop.wanted || state != LOCAL_SESSION_IN)
        return;
    peerCount = LocalLink_GetPeers(peers, 1);
    if (peerCount == 0)
    {
        ClearRemote();
        return;
    }
    if (sCoop.remoteValid && sCoop.peerId != peers[0].id)
        ClearRemote();
    if (sCoop.framesSinceLast != UINT16_MAX)
        sCoop.framesSinceLast++;
    SendLocalSnapshot(&local, &peers[0]);
}

bool CtrCoop_HandlePacket(uint16_t from, const uint8_t *data, uint16_t len)
{
    CtrCoopSnapshot remote;
    uint8_t password[4];
    uint8_t result[CTR_COOP_AUTH_RESULT_SIZE];
    bool accepted;

    if (len == 0)
        return false;
    if (!sCoop.wanted || LocalLink_GetSessionKind() != LOCAL_KIND_COOP)
        return false;
    if (data[0] == CTR_COOP_AUTH_PACKET)
    {
        if (sCoop.guest || !CtrCoopProtocol_DecodeAuth(data, len, password))
            return true;
        accepted = memcmp(password, CtrCoopProfile_Get()->hostPassword, sizeof(password)) == 0;
        sCoop.peerAuthenticated = accepted;
        if (accepted)
            sCoop.peerId = from;
        else
            ClearRemote();
        if (CtrCoopProtocol_EncodeAuthResult(accepted, result, sizeof(result)))
            LocalLink_Send(from, result, sizeof(result));
        return true;
    }
    if (data[0] == CTR_COOP_AUTH_RESULT_PACKET)
    {
        if (!sCoop.guest || !CtrCoopProtocol_DecodeAuthResult(data, len, &accepted))
            return true;
        if (accepted)
        {
            sCoop.authenticated = true;
            sCoop.authRejected = false;
            SendGuestIdentity(from);
        }
        else
        {
            sCoop.authenticated = false;
            sCoop.authRejected = true;
            ClearRemote();
            LocalLink_LeaveSession();
        }
        return true;
    }
    if (data[0] == CTR_COOP_IDENTITY_PACKET)
    {
        if (sCoop.guest || !sCoop.peerAuthenticated)
            return true;
        if (CtrCoopProtocol_DecodeIdentity(data, len, sCoop.peerName))
            PrepareHostSave(from);
        return true;
    }
    if (data[0] == CTR_COOP_SAVE_BEGIN_PACKET)
    {
        if (sCoop.guest && sCoop.authenticated)
            HandleSaveBegin(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_SAVE_REQUEST_PACKET)
    {
        uint32_t offset;

        if (sCoop.guest || !sCoop.peerAuthenticated || !sCoop.savePrepared)
            return true;
        if (CtrCoopProtocol_DecodeSaveRequest(data, len, &offset))
            SendHostSaveChunk(from, offset);
        return true;
    }
    if (data[0] == CTR_COOP_SAVE_CHUNK_PACKET)
    {
        if (sCoop.guest && sCoop.authenticated)
            HandleSaveChunk(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_SAVE_READY_PACKET)
    {
        if (!sCoop.guest && sCoop.peerAuthenticated && len == CTR_COOP_SAVE_READY_SIZE
         && data[1] == CTR_COOP_PROTOCOL_VERSION)
        {
            sCoop.peerSaveReady = true;
            SendHostSpawn(from);
        }
        return true;
    }
    if (data[0] == CTR_COOP_SPAWN_PACKET)
    {
        if (!sCoop.guest || !sCoop.authenticated || !sCoop.saveReady)
            return true;
        if (CtrCoopProtocol_DecodeSpawn(data, len, &remote))
        {
            sCoop.spawn = remote;
            sCoop.spawnValid = true;
        }
        return true;
    }
    if (data[0] != CTR_COOP_PEER_PACKET)
        return false;
    if ((sCoop.guest && (!sCoop.authenticated || !sCoop.saveReady))
     || (!sCoop.guest && (!sCoop.peerAuthenticated || !sCoop.peerSaveReady)))
        return true;
    if (!CtrCoopProtocol_Decode(data, len, &remote))
        return true;
    if (!sCoop.guest)
        StoreSpawnRecord(&remote);
    sCoop.peerId = from;
    sCoop.remote = remote;
    sCoop.remoteValid = true;
    return true;
}

void CtrCoop_PeerLeft(uint16_t id)
{
    if (sCoop.peerId == id || sCoop.peerId == 0)
    {
        bool restart = !sCoop.guest && sCoop.wanted;

        ClearRemote();
        sCoop.peerAuthenticated = false;
        sCoop.authSent = false;
        if (restart)
            LocalLink_StartSession(LOCAL_KIND_COOP);
    }
}

void CtrCoop_SoftReset(void)
{
    sCoop.wanted = false;
    sCoop.authenticated = false;
    sCoop.authSent = false;
    sCoop.authRejected = false;
    sCoop.peerAuthenticated = false;
    sCoop.objectEventId = OBJECT_EVENTS_COUNT;
    sCoop.haveLastSent = false;
    ClearRemote();
    if (LocalLink_GetSessionKind() == LOCAL_KIND_COOP)
        LocalLink_LeaveSession();
}
