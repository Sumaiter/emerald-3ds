#include <stdio.h>
#include <string.h>
#include "../src/3ds_coop.c"

struct SaveBlock1 *gSaveBlock1Ptr;
struct SaveBlock2 *gSaveBlock2Ptr;
struct PokemonStorage *gPokemonStoragePtr;
struct Pokemon gPlayerParty[PARTY_SIZE];
u8 gPlayerPartyCount;
struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
struct PlayerAvatar gPlayerAvatar;
struct MapHeader gMapHeader;
struct Main gMain;

static struct SaveBlock1 sSave1;
static struct SaveBlock2 sSave2;
static struct PokemonStorage sStorage;
static int sFailures;
static int sStarterOpened;
static int sPcOpened;
static int sScriptResumed;
static int sSent;
static uint8_t sLastPacket[CTR_COOP_SAVE_CHUNK_MAX];
static uint32_t sLastPacketSize;
static bool sRecordPackets;
static unsigned sPacketCount;
static uint8_t sPackets[16][CTR_COOP_SAVE_CHUNK_MAX];
static uint16_t sPacketSizes[16];
static int sSpriteMutations;
static int sRemovals;
static uint8_t sSession = LOCAL_SESSION_IN;
static int sPeerCount = 1;

#define CHECK(condition, message) do { if (!(condition)) { sFailures++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, message); } } while (0)

void CtrLog_Write(CtrLogCategory category, const char *format, ...) { (void)category; (void)format; }
uint8_t LocalLink_GetSession(void) { return sSession; }
uint8_t LocalLink_GetSessionKind(void) { return LOCAL_KIND_COOP; }
uint32_t LocalLink_Generation(void) { return 1; }
int LocalLink_GetPeers(struct LocalPeer *out, int max)
{
    if (max <= 0 || sPeerCount == 0) return 0;
    memset(out, 0, sizeof(*out));
    out->id = 2;
    return 1;
}
void LocalLink_Send(uint16_t dest, const void *data, uint32_t len)
{
    (void)dest;
    CHECK(len <= sizeof(sLastPacket), "test transport packet fits");
    if (len <= sizeof(sLastPacket)) memcpy(sLastPacket, data, len);
    sLastPacketSize = len;
    if (sRecordPackets)
    {
        CHECK(sPacketCount < ARRAY_COUNT(sPackets), "recorded party burst fits the test transport");
        if (sPacketCount < ARRAY_COUNT(sPackets))
        {
            memcpy(sPackets[sPacketCount], data, len);
            sPacketSizes[sPacketCount++] = len;
        }
    }
    sSent++;
}
void LocalLink_LeaveSession(void) { sSession = LOCAL_SESSION_NONE; }
void LocalLink_StartSession(uint8_t kind) { (void)kind; }
void LocalLink_DiscoverSessions(uint8_t kind) { (void)kind; }
void LocalLink_JoinSession(uint8_t kind, uint32_t token, const uint8_t *password) { (void)kind; (void)token; (void)password; }
void LocalLink_SetHostName(const uint8_t *name) { (void)name; }
void LocalLink_SetHostPassword(const uint8_t *password) { (void)password; }
int LocalLink_GetFoundSessions(struct LocalFoundSession *out, int max) { (void)out; (void)max; return 0; }
bool CtrCoopProfile_HasHostPassword(void) { return true; }
void PlaySE(u16 song) { (void)song; }
bool8 CB2_StarterSelectionActive(void) { return FALSE; }
void CB2_InitStarterSelection(void) {}
void CB2_Overworld(void) {}
static void MenuCallback(void) {}
void SetMainCallback2(MainCallback callback) { if (callback == CB2_InitStarterSelection) sStarterOpened++; }
void ShowPokemonStorageSystemPC(void) { sPcOpened++; }
void ScriptContext_Enable(void) { sScriptResumed++; }
void SetBagItemsPointers(void) {}
void SetDecorationInventoriesPointers(void) {}
void CopyPartyAndObjectsFromSave(void) {}
void LoadPlayerBag(void) {}
void ZeroMonData(struct Pokemon *mon) { memset(mon, 0, sizeof(*mon)); }
u32 GetBoxMonData(struct BoxPokemon *mon, s32 field) { (void)field; return mon->hasSpecies; }
void BoxMonToMon(const struct BoxPokemon *box, struct Pokemon *mon) { mon->box = *box; }
u8 CalculatePlayerPartyCount(void)
{
    u8 count = 0;
    while (count < PARTY_SIZE && gPlayerParty[count].box.hasSpecies) count++;
    return count;
}
u8 GetPlayerFacingDirection(void) { return DIR_SOUTH; }
u8 ObjectEventGetHeldMovementActionId(struct ObjectEvent *object) { (void)object; return 0; }
u32 GetBoxMonDataAt(u8 box, u8 position, s32 request) { return GetBoxMonData(&sStorage.boxes[box][position], request); }
s16 GetFirstFreeBoxSpot(u8 box)
{
    for (u8 i = 0; i < IN_BOX_COUNT; i++)
        if (!CtrCoop_StorageSlotLocked(box, i) && !sStorage.boxes[box][i].hasSpecies)
            return i;
    return -1;
}
void SetBoxMonAt(u8 box, u8 position, struct BoxPokemon *source) { sStorage.boxes[box][position] = *source; }
void ZeroBoxMonData(struct BoxPokemon *mon) { memset(mon, 0, sizeof(*mon)); }
void ZeroBoxMonAt(u8 box, u8 position) { ZeroBoxMonData(&sStorage.boxes[box][position]); }
void RemoveObjectEventByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup) { (void)localId; (void)mapNum; (void)mapGroup; sRemovals++; }
void SetWarpDestination(s8 mapGroup, s8 mapNum, s8 warpId, s8 x, s8 y) { (void)mapGroup; (void)mapNum; (void)warpId; (void)x; (void)y; }
void WarpIntoMap(void) {}
void LoadObjEventTemplatesFromHeader(void) {}
void SaveObjectEvents(void) {}
void CreateMon(struct Pokemon *mon, u16 species, u8 level, u8 fixedIV, bool8 hasFixedPersonality, u32 personality, u8 otIdType, u32 fixedOtId)
{
    (void)species; (void)level; (void)fixedIV; (void)hasFixedPersonality; (void)personality; (void)otIdType; (void)fixedOtId;
    ZeroMonData(mon);
    mon->box.hasSpecies = TRUE;
}
bool8 FlagGet(u16 id) { return (sSave1.flags[id / 8] & (1 << (id % 8))) != 0; }
bool8 ObjectEventSetHeldMovement(struct ObjectEvent *object, u8 action) { (void)object; (void)action; return FALSE; }
bool8 ObjectEventIsHeldMovementActive(struct ObjectEvent *object) { (void)object; return FALSE; }
u8 ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *object) { (void)object; return 0; }
void MoveObjectEventToMapCoords(struct ObjectEvent *object, s16 x, s16 y) { sSpriteMutations++; object->currentCoords.x = x; object->currentCoords.y = y; }
void ObjectEventTurn(struct ObjectEvent *object, u8 direction) { sSpriteMutations++; object->facingDirection = direction; }
u8 GetFaceDirectionMovementAction(u32 direction) { return direction - 1; }
bool8 TryGetObjectEventIdByLocalIdAndMap(u8 localId, u8 mapNum, u8 mapGroup, u8 *id)
{
    for (u8 i = 0; i < OBJECT_EVENTS_COUNT; i++)
        if (gObjectEvents[i].active && gObjectEvents[i].localId == localId
         && gObjectEvents[i].mapNum == mapNum && gObjectEvents[i].mapGroup == mapGroup)
        {
            *id = i;
            return FALSE;
        }
    return TRUE;
}
bool8 FreeObjectEventSlotNearPlayer(void) { return FALSE; }
u8 MapGridGetElevationAt(int x, int y) { (void)x; (void)y; return 0; }
u8 SpawnSpecialObjectEvent(struct ObjectEventTemplate *template) { (void)template; return OBJECT_EVENTS_COUNT; }
u8 TrySpawnObjectEvent(u8 localId, u8 mapNum, u8 mapGroup) { (void)localId; (void)mapNum; (void)mapGroup; return OBJECT_EVENTS_COUNT; }
void ObjectEventSetGraphicsId(struct ObjectEvent *object, u8 graphicsId) { object->graphicsId = graphicsId; }
const struct CtrCoopProfile *CtrCoopProfile_Get(void)
{
    static const struct CtrCoopProfile profile;
    return &profile;
}

static void ResetGuest(void)
{
    CtrCoop_SoftReset();
    memset(&sCoop, 0, sizeof(sCoop));
    memset(&sSave1, 0, sizeof(sSave1));
    memset(&sSave2, 0, sizeof(sSave2));
    memset(&sStorage, 0, sizeof(sStorage));
    memset(gPlayerParty, 0, sizeof(gPlayerParty));
    gPlayerPartyCount = 0;
    memset(gObjectEvents, 0, sizeof(gObjectEvents));
    memset(&gPlayerAvatar, 0, sizeof(gPlayerAvatar));
    memset(&gMapHeader, 0, sizeof(gMapHeader));
    gMain.callback2 = CB2_Overworld;
    gSaveBlock1Ptr = &sSave1;
    gSaveBlock2Ptr = &sSave2;
    gPokemonStoragePtr = &sStorage;
    sCoop.objectEventId = OBJECT_EVENTS_COUNT;
    sCoop.wanted = true;
    sCoop.guest = true;
    sCoop.authenticated = true;
    sCoop.peerId = 2;
    sCoop.generation = 1;
    sSession = LOCAL_SESSION_IN;
    sPeerCount = 1;
    sStarterOpened = sPcOpened = sSent = 0;
    sScriptResumed = 0;
    sSpriteMutations = sRemovals = 0;
    sLastPacketSize = 0;
    sRecordPackets = false;
    sPacketCount = 0;
}

static void TestJoinOrdering(void)
{
    ResetGuest();
    sCoop.starterRequired = true;
    sCoop.joinInfoReceived = true;
    CtrCoop_MenuFrame();
    CHECK(sStarterOpened == 0, "starter waits for the complete host snapshot");
    CHECK(!CtrCoop_ChooseStarter(SPECIES_TREECKO), "starter cannot be created before the snapshot is ready");
    sCoop.saveReady = true;
    sCoop.joinInfoReceived = false;
    CtrCoop_MenuFrame();
    CHECK(sStarterOpened == 0, "starter waits for join information");
    CHECK(!CtrCoop_ChooseStarter(SPECIES_TREECKO), "starter cannot be created before join information arrives");
    sCoop.joinInfoReceived = true;
    CtrCoop_MenuFrame();
    CHECK(sStarterOpened == 1, "starter opens once snapshot and join information are ready");
    CHECK(CtrCoop_ChooseStarter(SPECIES_TREECKO), "starter can be created after the snapshot and join information");
    CHECK(!CtrCoop_ChooseStarter(SPECIES_TORCHIC), "starter confirmation cannot create a second Pokemon");
}

static void TestGuestSaveProtection(void)
{
    ResetGuest();
    CtrCoop_Stop();
    CHECK(!CtrCoop_GuestSaveBlocked(), "cancelled search permits saving the original local world");
    ResetGuest();
    memset(sSaveData, 0, sizeof(sSaveData));
    ApplyHostSave();
    CtrCoop_Stop();
    CHECK(CtrCoop_GuestSaveBlocked(), "leaving the host world cannot overwrite the local save");
    CtrCoop_SoftReset();
    CHECK(!CtrCoop_GuestSaveBlocked(), "reset restores local save access");
    ResetGuest();
    ApplyHostSave();
    CtrCoop_PeerLeft(2);
    CHECK(!sCoop.authenticated && sCoop.peerId == 0, "disconnect invalidates the authenticated peer");
    sSession = LOCAL_SESSION_NONE;
    CtrCoop_OverworldFrame();
    CHECK(CtrCoop_GuestSaveBlocked(), "network loss preserves the local save guard");
    CtrCoop_SoftReset();
    sCoop.wanted = true;
    sCoop.guest = false;
    CHECK(!CtrCoop_GuestSaveBlocked(), "the host can still save its own world");
}

static void TestAuthenticationResult(void)
{
    uint8_t packet[CTR_COOP_AUTH_RESULT_SIZE];

    ResetGuest();
    sCoop.authenticated = false;
    sCoop.peerId = 0;
    CtrCoopProtocol_EncodeAuthResult(true, packet, sizeof(packet));
    CtrCoop_HandlePacket(2, packet, sizeof(packet));
    CHECK(!sCoop.authenticated, "unsolicited authentication result is ignored");
    sCoop.authSent = true;
    CtrCoop_HandlePacket(2, packet, sizeof(packet));
    CHECK(sCoop.authenticated && sCoop.peerId == 2, "authentication binds the selected peer");
    CHECK(sSent == 1, "authentication sends the guest identity");
    CtrCoop_HandlePacket(2, packet, sizeof(packet));
    CHECK(sSent == 1, "duplicate authentication result cannot restart the snapshot");
}

static void TestHostPasswordCheck(void)
{
    const uint8_t wrongPassword[] = {1, 2, 3, 4};
    const uint8_t rightPassword[] = {0, 0, 0, 0};
    uint8_t packet[CTR_COOP_AUTH_PACKET_SIZE];

    ResetGuest();
    sCoop.guest = false;
    CtrCoopProtocol_EncodeAuth(wrongPassword, packet, sizeof(packet));
    CtrCoop_HandlePacket(2, packet, sizeof(packet));
    CHECK(!sCoop.peerAuthenticated && sCoop.peerId == 0, "wrong password cannot authenticate with the host");
    CtrCoopProtocol_EncodeAuth(rightPassword, packet, sizeof(packet));
    CtrCoop_HandlePacket(3, packet, sizeof(packet));
    CHECK(!sCoop.peerAuthenticated, "valid password from another sender is ignored");
    CtrCoop_HandlePacket(2, packet, sizeof(packet));
    CHECK(sCoop.peerAuthenticated && sCoop.peerId == 2, "correct password authenticates the connected guest");
}

static void TestPacketAuthentication(void)
{
    uint8_t flag[CTR_COOP_FLAG_SIZE];
    uint8_t release[] = {CTR_COOP_PC_RELEASE_PACKET, CTR_COOP_PROTOCOL_VERSION};

    ResetGuest();
    sCoop.peerId = 2;
    ResetStoryBaseline();
    CtrCoopProtocol_EncodeFlag(1, true, flag, sizeof(flag));
    sCoop.authenticated = false;
    CtrCoop_HandlePacket(2, flag, sizeof(flag));
    CHECK(!FlagGet(1), "unauthenticated packets cannot change story flags");
    sCoop.authenticated = true;
    CtrCoop_HandlePacket(3, flag, sizeof(flag));
    CHECK(!FlagGet(1), "another sender cannot impersonate the authenticated peer");
    sCoop.saveReady = true;
    CtrCoop_HandlePacket(2, flag, sizeof(flag));
    CHECK(FlagGet(1), "authenticated peer can synchronize story flags");
    sCoop.pcOwner = PC_OWNER_GUEST;
    CtrCoop_HandlePacket(2, release, sizeof(release));
    CHECK(sCoop.pcOwner == PC_OWNER_GUEST, "remote release cannot unlock the local player's PC");
    sCoop.pcOwner = PC_OWNER_HOST;
    sCoop.syncState = PC_SYNC_RECEIVING;
    CtrCoop_HandlePacket(2, release, sizeof(release));
    CHECK(sCoop.pcOwner == PC_OWNER_HOST, "PC stays locked during an incomplete transfer");
    sCoop.syncState = PC_SYNC_IDLE;
    CtrCoop_HandlePacket(2, release, sizeof(release));
    CHECK(sCoop.pcOwner == PC_OWNER_NONE, "owner can release the PC after the transfer");
}

static void TestHostSnapshotThroughPackets(void)
{
    static uint8_t transfer[CTR_COOP_SAVE_SIZE] ALIGNED(4);
    uint8_t begin[CTR_COOP_SAVE_BEGIN_SIZE];
    uint8_t chunk[CTR_COOP_SAVE_CHUNK_MAX];
    uint8_t joinInfo[CTR_COOP_JOIN_INFO_SIZE];

    ResetGuest();
    memset(transfer, 0, sizeof(transfer));
    transfer[sizeof(struct SaveBlock2) + offsetof(struct SaveBlock1, flags) + 2] = 4;
    sSave1.flags[2] = 99;
    CtrCoopProtocol_EncodeJoinInfo(true, joinInfo, sizeof(joinInfo));
    CtrCoop_HandlePacket(2, joinInfo, sizeof(joinInfo));
    CtrCoopProtocol_EncodeSaveBegin(sizeof(transfer), CtrCoopProtocol_Checksum(transfer, sizeof(transfer)), begin, sizeof(begin));
    CtrCoop_HandlePacket(2, begin, sizeof(begin));
    for (uint32_t offset = 0; offset < sizeof(transfer); )
    {
        uint32_t size = sizeof(transfer) - offset;
        if (size > CTR_COOP_SAVE_CHUNK_DATA_MAX) size = CTR_COOP_SAVE_CHUNK_DATA_MAX;
        CtrCoopProtocol_EncodeSaveChunk(offset, transfer + offset, size, chunk, CTR_COOP_SAVE_CHUNK_HEADER + size);
        CtrCoop_MenuFrame();
        CHECK(sStarterOpened == 0 && sSave1.flags[2] == 99,
            "starter and live game state wait for the complete snapshot");
        CtrCoop_HandlePacket(2, chunk, CTR_COOP_SAVE_CHUNK_HEADER + size);
        offset += size;
    }
    CHECK(sCoop.saveReady && sSave1.flags[2] == 4, "authenticated snapshot is applied after checksum validation");
    CtrCoop_MenuFrame();
    CHECK(sStarterOpened == 1, "complete snapshot allows the starter menu");
    CtrCoop_Stop();
    CHECK(CtrCoop_GuestSaveBlocked(), "receiving the snapshot preserves save protection after leaving");
}

static void TestPcPreservesLiveParty(void)
{
    static uint8_t transfer[CTR_COOP_PC_SYNC_SIZE] ALIGNED(4);
    uint8_t begin[CTR_COOP_SAVE_BEGIN_SIZE];
    uint8_t chunk[CTR_COOP_SAVE_CHUNK_MAX];
    uint8_t release[] = {CTR_COOP_PC_RELEASE_PACKET, CTR_COOP_PROTOCOL_VERSION};

    ResetGuest();
    sCoop.pcOwner = PC_OWNER_HOST;
    memset(gPlayerParty, 0, sizeof(gPlayerParty));
    gPlayerParty[0].box.personality = 1234;
    memset(transfer, 0, sizeof(transfer));
    struct Pokemon *remoteParty = (struct Pokemon *)(transfer + sizeof(struct PokemonStorage));
    remoteParty[0].box.personality = 5678;
    sStorage.boxes[0][0].personality = 9999;
    CtrCoopProtocol_EncodePcSyncBegin(sizeof(transfer), CtrCoopProtocol_Checksum(transfer, sizeof(transfer)), begin, sizeof(begin));
    CtrCoop_HandlePacket(2, begin, sizeof(begin));
    for (uint32_t offset = 0; offset < sizeof(transfer); )
    {
        uint32_t size = sizeof(transfer) - offset;
        if (size > CTR_COOP_SAVE_CHUNK_DATA_MAX) size = CTR_COOP_SAVE_CHUNK_DATA_MAX;
        CtrCoopProtocol_EncodePcSyncChunk(offset, transfer + offset, size, chunk, CTR_COOP_SAVE_CHUNK_HEADER + size);
        CHECK(sStorage.boxes[0][0].personality == 9999, "incomplete PC transfer leaves live storage intact");
        CtrCoop_HandlePacket(2, chunk, CTR_COOP_SAVE_CHUNK_HEADER + size);
        offset += size;
    }
    CHECK(sCoop.syncState == PC_SYNC_IDLE && sCoop.pcOwner == PC_OWNER_HOST,
        "completed transfer waits for the owner's release");
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][CTR_COOP_STORAGE_ROW_SIZE].personality == 1234,
        "PC synchronization preserves the current local party row");
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].personality == 5678,
        "PC synchronization applies the other player's party row");
    CtrCoop_HandlePacket(2, release, sizeof(release));
    CHECK(sCoop.pcOwner == PC_OWNER_NONE, "completed PC transaction releases its lock");
}

static void TestSpritesWaitForOverworld(void)
{
    CtrCoopNpcSnapshot npc = {1, 0, 0, 40, 45, 1, 17};
    uint8_t packet[CTR_COOP_NPC_PACKET_SIZE];

    ResetGuest();
    sCoop.saveReady = sCoop.joinInfoReceived = true;
    gObjectEvents[1].active = TRUE;
    gObjectEvents[1].localId = 1;
    gObjectEvents[1].currentCoords.x = 7;
    gMain.callback2 = MenuCallback;
    CtrCoopProtocol_EncodeNpc(&npc, packet, sizeof(packet));
    CtrCoop_HandlePacket(2, packet, sizeof(packet));
    CHECK(sSpriteMutations == 0 && gObjectEvents[1].currentCoords.x == 7,
        "NPC packets cannot touch sprites owned by a menu or battle");
    gMain.callback2 = CB2_Overworld;
    CtrCoop_HandlePacket(2, packet, sizeof(packet));
    CHECK(gObjectEvents[1].currentCoords.x == 40 && gObjectEvents[1].frozen,
        "NPC synchronization resumes when the overworld is ready");

    gObjectEvents[0].active = gObjectEvents[0].isPlayer = TRUE;
    gObjectEvents[2].active = TRUE;
    gObjectEvents[2].localId = LOCALID_COOP_PLAYER;
    sCoop.objectEventId = 2;
    sCoop.remoteValid = true;
    sCoop.remote = (CtrCoopSnapshot){0, 0, 0, 10, 10, 1, CTR_COOP_ACTION_FACE_DOWN, 0};
    gMain.callback2 = MenuCallback;
    sSpriteMutations = 0;
    CtrCoop_OverworldFrame();
    CHECK(sSpriteMutations == 0, "remote avatar updates wait while menu sprites are active");
    CtrCoop_Stop();
    CtrCoop_OverworldFrame();
    CHECK(sRemovals == 0, "disconnect cleanup cannot remove a menu sprite");
    gMain.callback2 = CB2_Overworld;
    CtrCoop_OverworldFrame();
    CHECK(sRemovals == 1, "disconnect removes the avatar after returning to the overworld");
}

static void TestPcMutualExclusion(void)
{
    uint8_t request[] = {CTR_COOP_PC_REQUEST_PACKET, CTR_COOP_PROTOCOL_VERSION};
    uint8_t release[] = {CTR_COOP_PC_RELEASE_PACKET, CTR_COOP_PROTOCOL_VERSION};
    uint8_t claim[] = {CTR_COOP_PC_CLAIM_PACKET, CTR_COOP_PROTOCOL_VERSION};
    uint8_t grant[CTR_COOP_PC_RESULT_SIZE];

    ResetGuest();
    sCoop.guest = false;
    sCoop.peerAuthenticated = sCoop.peerSaveReady = true;
    CHECK(CtrCoop_RequestPC(), "host can acquire a free PC");
    CtrCoop_HandlePacket(2, request, sizeof(request));
    CHECK(sCoop.pcOwner == PC_OWNER_HOST, "guest request cannot steal the host's PC");
    CtrCoop_ReleasePC();
    CtrCoop_HandlePacket(2, request, sizeof(request));
    CHECK(sCoop.pcOwner == PC_OWNER_GUEST, "host grants the released PC to the guest");
    CHECK(!CtrCoop_RequestPC(), "host cannot acquire the guest's PC");
    CtrCoop_HandlePacket(2, release, sizeof(release));
    CHECK(CtrCoop_RequestPC(), "host can reacquire the PC after the guest releases it");

    ResetGuest();
    sCoop.saveReady = true;
    CHECK(!CtrCoop_RequestPC() && sCoop.pcPending, "guest waits for the host's grant");
    CtrCoop_HandlePacket(2, claim, sizeof(claim));
    CHECK(sCoop.pcOwner == PC_OWNER_HOST && !sCoop.pcPending, "host claim resolves a simultaneous PC request");
    CtrCoopProtocol_EncodePcResult(true, grant, sizeof(grant));
    CtrCoop_HandlePacket(2, grant, sizeof(grant));
    CHECK(sCoop.pcOwner == PC_OWNER_HOST, "stale grant cannot override the host's current ownership");
}

static void TestPcDenialResumesScript(void)
{
    uint8_t result[CTR_COOP_PC_RESULT_SIZE];
    uint8_t claim[] = {CTR_COOP_PC_CLAIM_PACKET, CTR_COOP_PROTOCOL_VERSION};

    ResetGuest();
    sCoop.guest = false;
    sCoop.pcOwner = PC_OWNER_GUEST;
    CHECK(!CtrCoop_RequestPC(), "occupied PC is denied");
    CHECK(sScriptResumed == 0, "denial waits until the special's waitstate");
    CtrCoop_OverworldFrame();
    CHECK(sScriptResumed == 1, "denied host PC returns to the outer PC menu");
    CtrCoop_OverworldFrame();
    CHECK(sScriptResumed == 1, "denial resumes the script once");

    ResetGuest();
    sCoop.partySending = true;
    CHECK(!CtrCoop_RequestPC(), "party synchronization temporarily blocks PC access");
    CtrCoop_OverworldFrame();
    CHECK(sScriptResumed == 1, "party synchronization denial cannot strand the PC script");

    ResetGuest();
    sCoop.saveReady = true;
    CHECK(!CtrCoop_RequestPC() && sCoop.pcPending, "guest waits asynchronously");
    CtrCoopProtocol_EncodePcResult(false, result, sizeof(result));
    CtrCoop_HandlePacket(2, result, sizeof(result));
    CtrCoop_OverworldFrame();
    CHECK(sScriptResumed == 1, "asynchronous denial resumes the guest's script");

    ResetGuest();
    sCoop.saveReady = true;
    CtrCoop_RequestPC();
    CtrCoop_HandlePacket(2, claim, sizeof(claim));
    CtrCoop_OverworldFrame();
    CHECK(sScriptResumed == 1, "simultaneous host claim resumes the guest's script");

    ResetGuest();
    sCoop.saveReady = true;
    CtrCoop_RequestPC();
    sSession = LOCAL_SESSION_NONE;
    CtrCoop_OverworldFrame();
    CtrCoop_OverworldFrame();
    CHECK(sScriptResumed == 1, "disconnect releases a pending PC script");

    ResetGuest();
    gMain.callback2 = MenuCallback;
    sCoop.saveReady = true;
    CtrCoop_RequestPC();
    CtrCoop_HandlePacket(2, result, sizeof(result));
    gMain.callback2 = CB2_Overworld;
    CtrCoop_OverworldFrame();
    CHECK(sScriptResumed == 0, "initial join PC denial does not resume an unrelated field script");
}

static void TestPcDisconnectAndDeferredJoin(void)
{
    uint8_t identity[CTR_COOP_IDENTITY_SIZE];
    uint8_t name[CTR_COOP_PLAYER_NAME_SIZE] = {0xBB, EOS};
    struct BoxPokemon mon = {0};
    mon.hasSpecies = TRUE;
    mon.personality = 12345;

    ResetGuest();
    sCoop.guest = false;
    sCoop.peerAuthenticated = true;
    CHECK(CtrCoop_RequestPC(), "host acquires the PC before a new join");
    CtrCoopProtocol_EncodeIdentity(name, identity, sizeof(identity));
    CtrCoop_HandlePacket(2, identity, sizeof(identity));
    CHECK(sCoop.peerIdentified && !sCoop.savePrepared, "join waits for the host's complete PC operation");
    CtrCoop_OverworldFrame();
    CHECK(!sCoop.savePrepared && sCoop.pcOpen, "held cursor state is never sent as a join snapshot");
    gPlayerParty[0].box = mon;
    CtrCoop_CommitPC();
    CHECK(sCoop.pcOwner == PC_OWNER_NONE && sCoop.syncState == PC_SYNC_IDLE,
        "PC exit commits locally before the joining guest is ready");
    CtrCoop_OverworldFrame();
    CHECK(sCoop.savePrepared, "queued join starts after the PC visit ends");
    const struct PokemonStorage *snapshot = (const void *)(sSaveData + sizeof(sSave1) + sizeof(sSave2));
    CHECK(snapshot->boxes[CTR_COOP_STORAGE_BOX_ID][0].personality == mon.personality,
        "deferred snapshot contains the completed party");

    ResetGuest();
    sCoop.guest = false;
    SaveExtension()->storageMagic = COOP_STORAGE_MAGIC;
    SaveExtension()->storageVersion = COOP_STORAGE_VERSION;
    gPlayerParty[0].box = mon;
    SyncParty();
    CHECK(CtrCoop_RequestPC(), "host enters a new PC visit");
    ZeroMonData(&gPlayerParty[0]); // Held by the PC cursor.
    CtrCoop_PeerLeft(2);
    CHECK(sCoop.pcOwner == PC_OWNER_HOST && sCoop.pcOpen,
        "departing guest preserves an open local host PC");
    SyncParty();
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].personality == mon.personality,
        "disconnect cannot mirror an incomplete held-cursor party");
    gPlayerParty[0].box = mon;
    sPeerCount = 0;
    CtrCoop_CommitPC();
    CHECK(sCoop.pcOwner == PC_OWNER_NONE && !sCoop.pcOpen && sCoop.syncState == PC_SYNC_IDLE,
        "host can finish the PC visit after its guest disconnects");
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].personality == mon.personality,
        "completed host party remains mirrored without a peer");
    CHECK(CtrCoop_RequestPC(), "host PC opens without a connected guest");
    CtrCoop_SoftReset();
    CHECK(!sCoop.pcOpen && sCoop.pcOwner == PC_OWNER_NONE, "reset discards the runtime PC ownership");

    ResetGuest();
    sCoop.wanted = false;
    sCoop.guest = false;
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0] = mon;
    CHECK(CtrCoop_RequestPC(), "ordinary solo PC remains available");
    CtrCoop_CommitPC();
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].personality == mon.personality && !CoopStorageReady(),
        "solo PC exit does not reserve or overwrite the original final box");
}

static void TestStorageMigrationAndInterruptedPc(void)
{
    static struct PokemonStorage previous;
    static uint8_t transfer[CTR_COOP_PC_SYNC_SIZE] ALIGNED(4);
    uint8_t begin[CTR_COOP_SAVE_BEGIN_SIZE], chunk[CTR_COOP_SAVE_CHUNK_MAX];

    ResetGuest();
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][7].hasSpecies = TRUE;
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][7].personality = 789;
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][11].hasSpecies = TRUE;
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][11].personality = 321;
    LoadPartyRow(CTR_COOP_STORAGE_GUEST_ROW, gPlayerParty);
    CHECK(gPlayerPartyCount == 2 && gPlayerParty[0].box.personality == 789 && gPlayerParty[1].box.personality == 321,
        "returning guest compacts holes left by offline withdrawals without losing Pokemon");
    CHECK(!gPlayerParty[2].box.hasSpecies && !gPlayerParty[5].box.hasSpecies,
        "compacted guest party clears every unused slot");

    ResetGuest();
    sCoop.guest = false;
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].hasSpecies = TRUE;
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].personality = 123;
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][29].hasSpecies = TRUE;
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][29].personality = 456;
    CHECK(PrepareCoopStorage(), "ordinary boxes receive pre-existing co-op box contents");
    CHECK(sStorage.boxes[0][0].personality == 123 && sStorage.boxes[0][1].personality == 456,
        "migration preserves both Pokemon including the final box slot");
    CHECK(!sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].hasSpecies
       && !sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][29].hasSpecies, "migration clears the reserved box");
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality = 789;
    CHECK(PrepareCoopStorage() && sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality == 789,
        "reopening the world never migrates an initialized guest row again");

    ResetGuest();
    sCoop.guest = false;
    for (u8 box = 0; box < TOTAL_BOXES_COUNT; box++)
        for (u8 i = 0; i < IN_BOX_COUNT; i++)
            sStorage.boxes[box][i].hasSpecies = TRUE;
    previous = sStorage;
    CHECK(!PrepareCoopStorage(), "full ordinary boxes refuse co-op initialization");
    CHECK(memcmp(&previous, &sStorage, sizeof(previous)) == 0 && !CoopStorageReady(),
        "failed migration leaves every Pokemon and the storage marker intact");

    ResetGuest();
    sCoop.guest = false;
    sCoop.peerAuthenticated = sCoop.peerSaveReady = true;
    sCoop.pcOwner = PC_OWNER_GUEST;
    sStorage.boxes[0][0].hasSpecies = TRUE;
    sStorage.boxes[0][0].personality = 987;
    previous = sStorage;
    memset(transfer, 0, sizeof(transfer));
    CtrCoopProtocol_EncodePcSyncBegin(sizeof(transfer), CtrCoopProtocol_Checksum(transfer, sizeof(transfer)), begin, sizeof(begin));
    CtrCoop_HandlePacket(2, begin, sizeof(begin));
    CtrCoopProtocol_EncodePcSyncChunk(0, transfer, 128, chunk, CTR_COOP_SAVE_CHUNK_HEADER + 128);
    CtrCoop_HandlePacket(2, chunk, CTR_COOP_SAVE_CHUNK_HEADER + 128);
    CHECK(sCoop.syncState == PC_SYNC_RECEIVING && memcmp(&previous, &sStorage, sizeof(previous)) == 0,
        "partial guest PC transfer cannot modify host storage");
    CtrCoop_PeerLeft(2);
    CtrCoop_HandlePacket(2, chunk, CTR_COOP_SAVE_CHUNK_HEADER + 128);
    CHECK(sCoop.syncState == PC_SYNC_IDLE && sCoop.pcOwner == PC_OWNER_NONE
       && memcmp(&previous, &sStorage, sizeof(previous)) == 0,
        "disconnect discards the partial PC operation and ignores late chunks");
}

static void TestSavedAvatarCleanup(void)
{
    ResetGuest();
    sCoop.wanted = false;
    gObjectEvents[2].active = TRUE;
    gObjectEvents[2].localId = LOCALID_COOP_PLAYER;
    gMain.callback2 = MenuCallback;
    CtrCoop_OverworldFrame();
    CHECK(sRemovals == 0, "restored avatar cleanup waits until field sprites are ready");
    gMain.callback2 = CB2_Overworld;
    CtrCoop_OverworldFrame();
    CHECK(sRemovals == 1, "loading a solo save removes an avatar left by a previous co-op session");
}

static void TestTenSpawnNamesAndMigration(void)
{
    struct LegacyExtension
    {
        uint16_t magic, version;
        struct SpawnRecord records[4];
        uint16_t storageMagic, storageVersion;
    } legacy = {0};
    struct SaveBlock1 persisted;
    uint8_t name[CTR_COOP_PLAYER_NAME_SIZE];
    CtrCoopSnapshot spawn = {0, 9, 10, 19, 16, 1, CTR_COOP_ACTION_FACE_DOWN, 100};

    ResetGuest();
    legacy.magic = SPAWN_TABLE_MAGIC;
    legacy.version = 1;
    legacy.storageMagic = COOP_STORAGE_MAGIC;
    legacy.storageVersion = COOP_STORAGE_VERSION;
    for (u8 i = 0; i < 4; i++)
    {
        memset(legacy.records[i].name, 0xFF, sizeof(name));
        legacy.records[i].name[0] = 0xBB + i;
        legacy.records[i].used = TRUE;
        legacy.records[i].mapNum = 9;
        legacy.records[i].mapLayoutId = 10;
        legacy.records[i].x = 19 + i;
        legacy.records[i].y = 16;
        legacy.records[i].facing = 1;
        legacy.records[i].graphicsId = 100;
    }
    memcpy(sSave1.unused_3598, &legacy, sizeof(legacy));
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].hasSpecies = TRUE;
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality = 12345;
    CHECK(PrepareCoopStorage(), "legacy co-op storage can be opened");
    CHECK(ValidSpawnTable()->version == 2, "four-name table migrates to version two");
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality == 12345,
        "migration preserves the reserved guest row and its storage marker");
    for (u8 i = 0; i < 10; i++)
    {
        memset(name, 0xFF, sizeof(name));
        name[0] = 0xBB + i;
        if (i < 4)
        {
            struct SpawnRecord *record = FindSpawnRecord(name, false);
            CHECK(record != NULL && record->x == 19 + i, "existing guest position survives migration");
        }
        memcpy(sCoop.peerName, name, sizeof(name));
        spawn.x = 19 + i;
        StoreSpawnRecord(&spawn);
        CHECK(FindSpawnRecord(name, false) != NULL, "all ten distinct guest names can be stored");
    }
    persisted = sSave1;
    memset(sSave1.unused_3598, 0, sizeof(sSave1.unused_3598));
    sSave1 = persisted;
    for (u8 i = 0; i < 10; i++)
    {
        name[0] = 0xBB + i;
        struct SpawnRecord *record = FindSpawnRecord(name, false);
        CHECK(record != NULL && record->x == 19 + i && record->y == 16,
            "all ten positions survive save-block restoration");
    }
    name[0] = 0xBB + 10;
    CHECK(FindSpawnRecord(name, true) == NULL, "an eleventh name never replaces an existing guest");
    CHECK(memcmp(&persisted, &sSave1, sizeof(sSave1)) == 0, "full table leaves the saved world unchanged");

    sCoop.guest = false;
    sCoop.peerAuthenticated = true;
    sCoop.peerId = 2;
    uint8_t identity[CTR_COOP_IDENTITY_SIZE];
    CtrCoopProtocol_EncodeIdentity(name, identity, sizeof(identity));
    CtrCoop_HandlePacket(2, identity, sizeof(identity));
    CHECK(!sCoop.savePrepared && !sCoop.peerAuthenticated,
        "an eleventh guest is rejected before receiving the host save");
    CHECK(CtrCoopProtocol_DecodeNameLimit(sLastPacket, sLastPacketSize),
        "full name table sends an explicit name-limit result");
    CHECK(memcmp(&persisted, &sSave1, sizeof(sSave1)) == 0,
        "rejecting the eleventh guest preserves all ten saved positions");
    name[0] = 0xBB;
    sCoop.peerAuthenticated = true;
    sCoop.peerId = 2;
    CtrCoopProtocol_EncodeIdentity(name, identity, sizeof(identity));
    CtrCoop_HandlePacket(2, identity, sizeof(identity));
    CHECK(sCoop.savePrepared, "an existing guest can still rejoin a full ten-name table");
}

static void TestNameLimitResult(void)
{
    uint8_t packet[CTR_COOP_NAME_LIMIT_SIZE];

    ResetGuest();
    CtrCoopProtocol_EncodeNameLimit(packet, sizeof(packet));
    sCoop.authenticated = false;
    CtrCoop_HandlePacket(2, packet, sizeof(packet));
    CHECK(!sCoop.nameLimitRejected, "unauthenticated name-limit result is ignored");
    sCoop.authenticated = true;
    CtrCoop_HandlePacket(3, packet, sizeof(packet));
    CHECK(!sCoop.nameLimitRejected, "another sender cannot reject the guest");
    CtrCoop_HandlePacket(2, packet, sizeof(packet));
    CHECK(CtrCoop_GetSessionState() == CTR_COOP_SESSION_NAME_LIMIT,
        "guest reports the name limit separately from a password failure");
    CHECK(!sCoop.authenticated && !sCoop.saveReady && !sCoop.guestWorldLoaded,
        "name-limit rejection never applies the host world");
    CtrCoop_OverworldFrame();
    CHECK(CtrCoop_GetSessionState() == CTR_COOP_SESSION_NAME_LIMIT,
        "name-limit message remains visible after transport disconnection");
    CtrCoop_Stop();
    CHECK(CtrCoop_GetSessionState() == LOCAL_SESSION_NONE,
        "cancelling a rejected join clears the name-limit message");
}

static void ReadyPartyPeer(bool guest)
{
    ResetGuest();
    CHECK(PrepareCoopStorage(), "party test storage is prepared");
    sCoop.guest = guest;
    sCoop.saveReady = sCoop.peerSaveReady = sCoop.peerTeamReady = true;
    sCoop.initialTeamCommitted = sCoop.peerAuthenticated = sCoop.joinInfoReceived = true;
}

static void TestContinuousPartySync(void)
{
    uint8_t transfer[6][CTR_COOP_SAVE_CHUNK_MAX];
    uint16_t sizes[6];
    uint8_t result[CTR_COOP_PARTY_RESULT_SIZE];
    bool accepted;

    ReadyPartyPeer(true);
    gPlayerParty[0].box.personality = 111;
    gPlayerParty[3].box.personality = 333;
    sRecordPackets = true;
    gMain.callback2 = MenuCallback;
    CtrCoop_OverworldFrame();
    CHECK(sPacketCount == 6 && sCoop.partySending, "party change sends a complete six-message burst even in a menu");
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality == 111,
        "local party immediately mirrors to its own co-op row");
    memcpy(transfer, sPackets, sizeof(transfer));
    memcpy(sizes, sPacketSizes, sizeof(sizes));
    gPlayerParty[0].box.personality = 222;
    SyncParty();
    CHECK(sPacketCount == 6, "an outstanding party burst is coalesced instead of flooding the transport");
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality == 222,
        "local mirror stays current while an older burst is in flight");
    CHECK(((struct Pokemon *)sPartySend)[0].box.personality == 111, "in-flight party snapshot is immutable");
    CtrCoopProtocol_EncodePartyResult(true, result, sizeof(result));
    CtrCoop_HandlePacket(2, result, sizeof(result));
    SyncParty();
    CHECK(sPacketCount == 12 && ((struct Pokemon *)sPartySend)[0].box.personality == 222,
        "changes made while waiting for confirmation are sent immediately afterwards");

    ReadyPartyPeer(false);
    gPlayerParty[0].box.personality = 999;
    SyncParty();
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality = 42;
    for (u8 i = 0; i < 6; i++)
    {
        CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality == 42,
            "partial party transfer leaves the previous guest row intact");
        CtrCoop_HandlePacket(2, transfer[i], sizes[i]);
    }
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality == 111
       && sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][9].personality == 333,
        "host atomically receives the guest's complete party row");
    CHECK(gPlayerParty[0].box.personality == 999 && sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].personality == 999,
        "guest updates never replace the host's party or own row");
    CHECK(CtrCoopProtocol_DecodePartyResult(sLastPacket, sLastPacketSize, &accepted) && accepted,
        "party is acknowledged only after complete application");
    CtrCoop_PeerLeft(2);
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality == 111
       && !CtrCoop_StorageSlotLocked(CTR_COOP_STORAGE_BOX_ID, 6),
        "power-off keeps the last complete guest party and unlocks its row");

    ReadyPartyPeer(false);
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality = 42;
    CtrCoop_HandlePacket(2, transfer[0], sizes[0]);
    CtrCoop_HandlePacket(2, transfer[1], sizes[1]);
    CtrCoop_PeerLeft(2);
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality == 42 && !sCoop.partyReceiving,
        "power-off during a partial transfer preserves the previous complete party");

    ReadyPartyPeer(false);
    sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality = 42;
    transfer[1][CTR_COOP_SAVE_CHUNK_HEADER] ^= 1;
    for (u8 i = 0; i < 6; i++) CtrCoop_HandlePacket(2, transfer[i], sizes[i]);
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][6].personality == 42,
        "bad checksum cannot replace the host's guest row");
    CHECK(CtrCoopProtocol_DecodePartyResult(sLastPacket, sLastPacketSize, &accepted) && !accepted,
        "bad checksum asks the sender to retry its current party");
    transfer[1][CTR_COOP_SAVE_CHUNK_HEADER] ^= 1;

    ReadyPartyPeer(false);
    SyncParty();
    CHECK(!CtrCoop_RequestPC(), "PC cannot open while a party confirmation is outstanding");
    CtrCoopProtocol_EncodePartyResult(true, result, sizeof(result));
    CtrCoop_HandlePacket(2, result, sizeof(result));
    CHECK(CtrCoop_RequestPC(), "PC can open after the party is confirmed");
    CtrCoop_HandlePacket(2, transfer[0], sizes[0]);
    CHECK(!sCoop.partyReceiving, "a crossing party begin waits until the PC transaction is finished");
    CHECK(CtrCoopProtocol_DecodePartyResult(sLastPacket, sLastPacketSize, &accepted) && !accepted,
        "busy PC explicitly requests a later party retry");
    CtrCoop_ReleasePC();
    CtrCoop_Stop();
    gPlayerParty[0].box.personality = 777;
    CtrCoop_OverworldFrame();
    CHECK(sStorage.boxes[CTR_COOP_STORAGE_BOX_ID][0].personality == 777,
        "host mirror remains current after disabling wireless");
    CHECK(CtrCoop_StorageSlotLocked(CTR_COOP_STORAGE_BOX_ID, 0),
        "the host's mirrored party cannot be withdrawn as duplicate Pokemon when wireless is off");
}

int main(void)
{
    TestJoinOrdering();
    TestGuestSaveProtection();
    TestAuthenticationResult();
    TestHostPasswordCheck();
    TestPacketAuthentication();
    TestHostSnapshotThroughPackets();
    TestPcPreservesLiveParty();
    TestSpritesWaitForOverworld();
    TestPcMutualExclusion();
    TestPcDenialResumesScript();
    TestPcDisconnectAndDeferredJoin();
    TestStorageMigrationAndInterruptedPc();
    TestSavedAvatarCleanup();
    TestTenSpawnNamesAndMigration();
    TestNameLimitResult();
    TestContinuousPartySync();
    if (sFailures) return 1;
    puts("co-op runtime tests passed");
    return 0;
}
