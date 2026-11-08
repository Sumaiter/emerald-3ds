#include "global.h"
#include "3ds_coop.h"
#include "3ds_coop_protocol.h"
#include "3ds_coop_profile.h"
#include "3ds_local.h"
#include "characters.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "constants/local_link.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
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

static bool SnapshotsEqual(const CtrCoopSnapshot *a, const CtrCoopSnapshot *b)
{
    return !CtrCoopProtocol_ShouldSend(a, b, 0);
}

static void ClearRemote(void)
{
    sCoop.peerId = 0;
    sCoop.remoteValid = false;
    sCoop.haveApplied = false;
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
    if (sCoop.wanted || LocalLink_GetSessionKind() == LOCAL_KIND_COOP)
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
    if (sCoop.guest && state == LOCAL_SESSION_IN && !sCoop.authenticated)
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
    if ((sCoop.guest && !sCoop.authenticated) || (!sCoop.guest && !sCoop.peerAuthenticated))
        return;
    if (!LocalSnapshot(&local))
    {
        RemoveRemoteObject();
        return;
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
    if (data[0] != CTR_COOP_PEER_PACKET)
        return false;
    if ((sCoop.guest && !sCoop.authenticated) || (!sCoop.guest && !sCoop.peerAuthenticated))
        return true;
    if (!CtrCoopProtocol_Decode(data, len, &remote))
        return true;
    sCoop.peerId = from;
    sCoop.remote = remote;
    sCoop.remoteValid = true;
    return true;
}

void CtrCoop_PeerLeft(uint16_t id)
{
    if (sCoop.peerId == id || sCoop.peerId == 0)
    {
        ClearRemote();
        sCoop.peerAuthenticated = false;
        sCoop.authSent = false;
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
