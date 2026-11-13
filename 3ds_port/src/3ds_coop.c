#include <stdlib.h>
#include "global.h"
#include "3ds_coop.h"
#include "3ds_coop_protocol.h"
#include "3ds_coop_profile.h"
#include "3ds_coop_storage.h"
#include "3ds_coop_bike.h"
#include "3ds_local.h"
#include "3ds_log.h"
#include "characters.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "constants/battle.h"
#include "constants/coins.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "constants/flags.h"
#include "constants/local_link.h"
#include "constants/layouts.h"
#include "constants/map_groups.h"
#include "constants/species.h"
#include "decoration_inventory.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "item.h"
#include "link.h"
#include "load_save.h"
#include "main_menu.h"
#include "money.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "sound.h"
#include "constants/songs.h"

static struct
{
    bool wanted;
    bool guest;
    /* The host snapshot stays in RAM after leaving; protect the local SAV
     * until a reset restores the local game state. */
    bool guestWorldLoaded;
    bool authenticated;
    bool authSent;
    bool authRejected;
    bool peerAuthenticated;
    uint8_t peerName[CTR_COOP_PLAYER_NAME_SIZE];
    uint8_t hostName[CTR_COOP_PLAYER_NAME_SIZE];
    uint8_t pcNoticeName[CTR_COOP_PLAYER_NAME_SIZE];
    bool pcNotice;
    bool savePrepared;
    bool saveReady;
    bool peerSaveReady;
    bool peerTeamReady;
    bool initialTeamSelection;
    bool initialTeamCommitted;
    bool starterRequired;
    bool starterChosen;
    bool joinInfoReceived;
    bool storyInitialized;
    uint8_t storyFrame;
    uint8_t storyFlagsBaseline[NUM_FLAG_BYTES];
    uint16_t storyVarsBaseline[VARS_COUNT];
    uint8_t pcOwner;
    bool pcPending;
    bool pcOpen;
    uint8_t syncState;
    uint32_t syncOffset;
    uint32_t syncChecksum;
    bool battleActive;
    bool battleReservePending;
    bool battleReserved;
    bool battleReleasePending;
    uint16_t battleTrainerId;
    uint16_t peerTrainerId;
    bool peerBattleActive;
    uint16_t battleRetry;
    uint8_t battleSendState;
    uint32_t battleSendSize;
    uint32_t battleSendOffset;
    uint32_t battleSendChecksum;
    uint8_t battleReceiveState;
    uint32_t battleReceiveSize;
    uint32_t battleReceiveOffset;
    uint32_t battleReceiveChecksum;
    uint8_t doubleState;
    bool doubleInitiator;
    uint16_t doubleRetry;
    uint32_t doubleSendOffset;
    uint32_t doubleSendChecksum;
    uint32_t doubleReceiveOffset;
    uint32_t doubleReceiveChecksum;
    uint8_t logoutState;
    bool peerLoggedOut;
    uint16_t logoutRetry;
    uint32_t logoutSendOffset;
    uint32_t logoutSendChecksum;
    uint32_t logoutReceiveOffset;
    uint32_t logoutReceiveChecksum;
    uint32_t battleBaselineKey;
    uint16_t battleSendSequence;
    uint16_t battleAppliedSequence;
    uint32_t battleAppliedChecksum;
    bool battleHaveApplied;
    bool battleWaitingTrainer;
    bool battleTrainerWasDefeated;
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
    uint8_t npcIndex;
    bool npcCacheValid[OBJECT_EVENTS_COUNT];
    CtrCoopNpcSnapshot npcCache[OBJECT_EVENTS_COUNT];
    bool nameLimitRejected;
    bool partySending;
    bool partyReceiving;
    bool havePartyAck;
    uint8_t partyRetry;
    uint16_t partyOffset;
    uint32_t partySendChecksum;
    uint32_t partyReceiveChecksum;
    uint32_t partyAckChecksum;
    bool pcScriptWaiting;
    bool pcResumeScript;
    bool peerIdentified;
} sCoop = {.objectEventId = OBJECT_EVENTS_COUNT};

#define CTR_COOP_SAVE_SIZE (sizeof(struct SaveBlock2) + sizeof(struct SaveBlock1) + sizeof(struct PokemonStorage))
#define CTR_COOP_PC_SYNC_SIZE (sizeof(struct PokemonStorage) + sizeof(struct Pokemon) * PARTY_SIZE)
#define CTR_COOP_PARTY_SIZE (sizeof(struct Pokemon) * PARTY_SIZE)
#define PC_OWNER_NONE 0
#define PC_OWNER_HOST 1
#define PC_OWNER_GUEST 2
#define PC_SYNC_IDLE 0
#define PC_SYNC_SENDING 1
#define PC_SYNC_RECEIVING 2
#define BATTLE_SYNC_IDLE 0
#define BATTLE_SYNC_SENDING 1
#define BATTLE_SYNC_RECEIVING 2
#define BATTLE_SYNC_READY 3
#define BATTLE_HEADER_SIZE 20
#define BATTLE_STORAGE_RECORD_SIZE 82
#define BATTLE_ITEM_RECORD_SIZE 4
#define BATTLE_ITEM_SLOTS (PC_ITEMS_COUNT + BAG_ITEMS_COUNT + BAG_KEYITEMS_COUNT + BAG_POKEBALLS_COUNT + BAG_TMHM_COUNT + BAG_BERRIES_COUNT)
#define CTR_COOP_BATTLE_DATA_MAX (BATTLE_HEADER_SIZE + sizeof(struct Pokemon) * PARTY_SIZE + TOTAL_BOXES_COUNT * IN_BOX_COUNT * BATTLE_STORAGE_RECORD_SIZE + 4096)
#define DOUBLE_STATE_IDLE 0
#define DOUBLE_STATE_WAIT_ACCEPT 1
#define DOUBLE_STATE_WAIT_START 2
#define DOUBLE_STATE_SENDING 3
#define DOUBLE_STATE_RECEIVING 4
#define DOUBLE_STATE_ACTIVE 5
#define DOUBLE_HEADER_SIZE 32
#define DOUBLE_SETUP_SIZE (DOUBLE_HEADER_SIZE + sizeof(struct Pokemon) * PARTY_SIZE * 2)
#define LOGOUT_STATE_IDLE 0
#define LOGOUT_STATE_SENDING 1
#define LOGOUT_STATE_RECEIVING 2

static uint8_t sSaveData[CTR_COOP_SAVE_SIZE] ALIGNED(4);
/* Independent buffers: party changes never overwrite an in-flight PC/save. */
static uint8_t sPartySend[CTR_COOP_PARTY_SIZE] ALIGNED(4);
static uint8_t sPartyReceive[CTR_COOP_PARTY_SIZE] ALIGNED(4);
static uint8_t sBattleSendData[CTR_COOP_BATTLE_DATA_MAX] ALIGNED(4);
static uint8_t sBattleReceiveData[CTR_COOP_BATTLE_DATA_MAX] ALIGNED(4);
static uint8_t sDoubleSendData[DOUBLE_SETUP_SIZE] ALIGNED(4);
static uint8_t sDoubleReceiveData[DOUBLE_SETUP_SIZE] ALIGNED(4);
static uint8_t sLogoutSendData[sizeof(struct Pokemon) * PARTY_SIZE] ALIGNED(4);
static uint8_t sLogoutReceiveData[sizeof(struct Pokemon) * PARTY_SIZE] ALIGNED(4);
static struct SaveBlock1 sBattleSaveBaseline;
static struct PokemonStorage sBattleStorageBaseline;
static struct Pokemon sDoubleLocalParty[PARTY_SIZE];
static struct Pokemon sRemoteParty[PARTY_SIZE];
static bool sRemotePartyValid;
static uint32_t sDoubleOriginalFlags;

static bool SnapshotsEqual(const CtrCoopSnapshot *a, const CtrCoopSnapshot *b)
{
    return !CtrCoopProtocol_ShouldSend(a, b, 0);
}

static void ClearRemote(void)
{
    /* A departing guest must not end the host's local PC operation while
     * a Pokemon may still be held by the cursor. */
    bool hostPcOpen = !sCoop.guest && sCoop.pcOpen && sCoop.pcOwner == PC_OWNER_HOST;
    CtrCoopBike_ClearRemote();
    if (sCoop.pcScriptWaiting || sCoop.pcNotice)
        sCoop.pcResumeScript = true;
    sCoop.pcNotice = false;
    sCoop.pcScriptWaiting = false;
    if (sCoop.joinWarpPending)
        CtrLog_Write(CTR_LOG_GAME, "coop: join warp interrupted");
    sCoop.peerId = 0;
    sCoop.peerAuthenticated = false;
    sCoop.peerIdentified = false;
    sCoop.remoteValid = false;
    sCoop.haveApplied = false;
    sCoop.savePrepared = false;
    sCoop.saveReady = false;
    sCoop.peerSaveReady = false;
    sCoop.peerTeamReady = false;
    sCoop.initialTeamSelection = false;
    sCoop.initialTeamCommitted = false;
    sCoop.starterRequired = false;
    sCoop.starterChosen = false;
    sCoop.joinInfoReceived = false;
    sCoop.storyInitialized = false;
    sCoop.storyFrame = 0;
    sCoop.partySending = false;
    sCoop.partyReceiving = false;
    sCoop.havePartyAck = false;
    sCoop.partyRetry = 0;
    sCoop.partyOffset = 0;
    sCoop.pcOwner = hostPcOpen ? PC_OWNER_HOST : PC_OWNER_NONE;
    sCoop.pcPending = false;
    sCoop.pcOpen = hostPcOpen;
    sCoop.syncState = 0;
    sCoop.syncOffset = 0;
    sCoop.syncChecksum = 0;
    sCoop.battleReservePending = false;
    sCoop.battleReserved = false;
    sCoop.battleReleasePending = false;
    sCoop.battleTrainerId = 0;
    sCoop.peerTrainerId = 0;
    sCoop.peerBattleActive = false;
    sCoop.battleRetry = 0;
    sCoop.battleSendState = BATTLE_SYNC_IDLE;
    sCoop.battleReceiveState = BATTLE_SYNC_IDLE;
    sCoop.doubleState = DOUBLE_STATE_IDLE;
    sCoop.doubleInitiator = false;
    sCoop.doubleRetry = 0;
    sCoop.logoutState = LOGOUT_STATE_IDLE;
    sCoop.peerLoggedOut = false;
    sCoop.logoutRetry = 0;
    sRemotePartyValid = false;
    sCoop.battleSendSequence = 0;
    sCoop.battleHaveApplied = false;
    sCoop.battleWaitingTrainer = false;
    sCoop.saveOffset = 0;
    sCoop.saveChecksum = 0;
    sCoop.spawnValid = false;
    sCoop.joinWarpPending = false;
    sCoop.npcIndex = 0;
    memset(sCoop.npcCacheValid, 0, sizeof(sCoop.npcCacheValid));
    memset(sCoop.peerName, 0xFF, sizeof(sCoop.peerName));
}

static void ResetStoryBaseline(void)
{
    if (gSaveBlock1Ptr == NULL)
        return;
    memcpy(sCoop.storyFlagsBaseline, gSaveBlock1Ptr->flags, sizeof(sCoop.storyFlagsBaseline));
    memcpy(sCoop.storyVarsBaseline, gSaveBlock1Ptr->vars, sizeof(sCoop.storyVarsBaseline));
    sCoop.storyInitialized = true;
    sCoop.storyFrame = 0;
}

#define SPAWN_TABLE_MAGIC 0x5350
#define SPAWN_TABLE_VERSION 2
#define SPAWN_RECORD_COUNT CTR_COOP_MAX_GUEST_NAMES
#define SPAWN_LEGACY_RECORD_COUNT 4

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

#define COOP_STORAGE_MAGIC 0x4342
#define COOP_STORAGE_VERSION 1

struct CoopSaveExtension
{
    struct SpawnTable spawn;
    uint16_t storageMagic;
    uint16_t storageVersion;
};

struct LegacyCoopSaveExtension
{
    uint16_t magic;
    uint16_t version;
    struct SpawnRecord records[SPAWN_LEGACY_RECORD_COUNT];
    uint16_t storageMagic;
    uint16_t storageVersion;
};

_Static_assert(sizeof(struct CoopSaveExtension) <= sizeof(((struct SaveBlock1 *)0)->unused_3598),
    "Co-op save extension must fit in the reserved save area");

static const uint8_t sDefaultGuestName[] = {'G', 'A', 'S', 'T', 0xFF, 0xFF, 0xFF, 0xFF};

static struct CoopSaveExtension *SaveExtension(void)
{
    struct CoopSaveExtension *extension;

    if (gSaveBlock1Ptr == NULL)
        return NULL;
    extension = (struct CoopSaveExtension *)gSaveBlock1Ptr->unused_3598;
    if (extension->spawn.magic == SPAWN_TABLE_MAGIC && extension->spawn.version == 1)
    {
        const struct LegacyCoopSaveExtension *legacy = (const void *)extension;
        uint16_t storageMagic = legacy->storageMagic;
        uint16_t storageVersion = legacy->storageVersion;

        /* The first four records keep their offsets. Save the old box marker
         * before clearing the six new records that occupy its old location. */
        memset(&extension->spawn.records[SPAWN_LEGACY_RECORD_COUNT], 0,
            sizeof(extension->spawn.records) - SPAWN_LEGACY_RECORD_COUNT * sizeof(struct SpawnRecord));
        extension->storageMagic = storageMagic;
        extension->storageVersion = storageVersion;
        extension->spawn.version = SPAWN_TABLE_VERSION;
    }
    if (extension->spawn.magic != SPAWN_TABLE_MAGIC || extension->spawn.version != SPAWN_TABLE_VERSION)
    {
        memset(&extension->spawn, 0, sizeof(extension->spawn));
        extension->spawn.magic = SPAWN_TABLE_MAGIC;
        extension->spawn.version = SPAWN_TABLE_VERSION;
    }
    return extension;
}

static bool CoopStorageReady(void)
{
    const struct CoopSaveExtension *extension;

    if (gSaveBlock1Ptr == NULL || gPokemonStoragePtr == NULL)
        return false;
    extension = (const void *)gSaveBlock1Ptr->unused_3598;
    if (extension->spawn.magic != SPAWN_TABLE_MAGIC)
        return false;
    if (extension->spawn.version == 1)
    {
        const struct LegacyCoopSaveExtension *legacy = (const void *)extension;
        return legacy->storageMagic == COOP_STORAGE_MAGIC && legacy->storageVersion == COOP_STORAGE_VERSION;
    }
    return extension->spawn.version == SPAWN_TABLE_VERSION
        && extension->storageMagic == COOP_STORAGE_MAGIC && extension->storageVersion == COOP_STORAGE_VERSION;
}

static void StorePartyRow(uint8_t row, const struct Pokemon *party)
{
    uint8_t start = row * CTR_COOP_STORAGE_ROW_SIZE;

    if (gPokemonStoragePtr == NULL)
        return;
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        gPokemonStoragePtr->boxes[CTR_COOP_STORAGE_BOX_ID][start + i] = party[i].box;
}

static void LoadPartyRow(uint8_t row, struct Pokemon *party)
{
    uint8_t start = row * CTR_COOP_STORAGE_ROW_SIZE;
    uint8_t count = 0;

    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
    {
        if (GetBoxMonData(&gPokemonStoragePtr->boxes[CTR_COOP_STORAGE_BOX_ID][start + i], MON_DATA_SANITY_HAS_SPECIES))
            BoxMonToMon(&gPokemonStoragePtr->boxes[CTR_COOP_STORAGE_BOX_ID][start + i], &party[count++]);
    }
    gPlayerPartyCount = CalculatePlayerPartyCount();
}

static bool PrepareCoopStorage(void)
{
    static const uint8_t boxName[BOX_NAME_LENGTH + 1] = {0xC4, 0xC8, 0xC8, 0xC9, EOS};
    struct CoopSaveExtension *extension = SaveExtension();
    uint16_t needed = 0;
    uint16_t available = 0;

    if (extension == NULL || gPokemonStoragePtr == NULL)
        return false;
    if (extension->storageMagic == COOP_STORAGE_MAGIC && extension->storageVersion == COOP_STORAGE_VERSION)
        return true;
    for (uint8_t box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (uint8_t position = 0; position < IN_BOX_COUNT; position++)
        {
            bool occupied = GetBoxMonDataAt(box, position, MON_DATA_SANITY_HAS_SPECIES) != 0;

            if (box == CTR_COOP_STORAGE_BOX_ID && occupied)
                needed++;
            else if (box != CTR_COOP_STORAGE_BOX_ID && !occupied)
                available++;
        }
    }
    if (needed > available)
    {
        CtrLog_Write(CTR_LOG_ERROR, "coop: no free box space for %u Pokémon", needed);
        return false;
    }
    for (uint8_t position = 0; position < IN_BOX_COUNT; position++)
    {
        struct BoxPokemon *source = &gPokemonStoragePtr->boxes[CTR_COOP_STORAGE_BOX_ID][position];

        if (!GetBoxMonData(source, MON_DATA_SANITY_HAS_SPECIES))
            continue;
        for (uint8_t box = 0; box < CTR_COOP_STORAGE_BOX_ID; box++)
        {
            s16 freePosition = GetFirstFreeBoxSpot(box);

            if (freePosition < 0)
                continue;
            SetBoxMonAt(box, freePosition, source);
            ZeroBoxMonData(source);
            break;
        }
    }
    for (uint8_t position = 0; position < IN_BOX_COUNT; position++)
        ZeroBoxMonAt(CTR_COOP_STORAGE_BOX_ID, position);
    memcpy(gPokemonStoragePtr->boxNames[CTR_COOP_STORAGE_BOX_ID], boxName, sizeof(boxName));
    extension->storageMagic = COOP_STORAGE_MAGIC;
    extension->storageVersion = COOP_STORAGE_VERSION;
    CtrLog_Write(CTR_LOG_GAME, "coop: storage box initialized, moved %u Pokémon", needed);
    return true;
}

static bool LocalSnapshot(CtrCoopSnapshot *out);

static void GuestName(uint8_t *out)
{
    const struct CtrCoopProfile *profile = CtrCoopProfile_Get();

    if (profile->guestNameLength > 0 && profile->guestNameLength <= CTR_COOP_GUEST_NAME_LENGTH)
        memcpy(out, profile->guestName, CTR_COOP_PLAYER_NAME_SIZE);
    else
        memcpy(out, sDefaultGuestName, CTR_COOP_PLAYER_NAME_SIZE);
}

static struct SpawnTable *ValidSpawnTable(void)
{
    struct CoopSaveExtension *extension = SaveExtension();

    return extension == NULL ? NULL : &extension->spawn;
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

static bool HasRoomForGuestName(const uint8_t *name)
{
    struct SpawnTable *table = ValidSpawnTable();

    if (table == NULL)
        return false;
    if (FindSpawnRecord(name, false) != NULL)
        return true;
    for (u8 i = 0; i < SPAWN_RECORD_COUNT; i++)
        if (!table->records[i].used)
            return true;
    return false;
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

static bool GuestRowHasMons(void)
{
    uint8_t start = CTR_COOP_STORAGE_GUEST_ROW * CTR_COOP_STORAGE_ROW_SIZE;

    for (uint8_t i = 0; i < CTR_COOP_STORAGE_ROW_SIZE; i++)
    {
        if (GetBoxMonData(&gPokemonStoragePtr->boxes[CTR_COOP_STORAGE_BOX_ID][start + i], MON_DATA_SANITY_HAS_SPECIES))
            return true;
    }
    return false;
}

static void SendJoinInfo(uint16_t to)
{
    bool starterRequired = FindSpawnRecord(sCoop.peerName, FALSE) == NULL && !GuestRowHasMons();
    uint8_t packet[CTR_COOP_JOIN_INFO_SIZE];

    if (CtrCoopProtocol_EncodeJoinInfo(starterRequired, packet, sizeof(packet)))
        LocalLink_Send(to, packet, sizeof(packet));
    CtrLog_Write(CTR_LOG_GAME, "coop: starter selection %s", starterRequired ? "required" : "skipped");
}

static void PrepareHostSave(uint16_t to)
{
    uint8_t packet[CTR_COOP_SAVE_BEGIN_SIZE];
    uint32_t offset = 0;

    if (gSaveBlock2Ptr == NULL || gSaveBlock1Ptr == NULL || gPokemonStoragePtr == NULL || !PrepareCoopStorage())
        return;
    SaveObjectEvents();
    StorePartyRow(CTR_COOP_STORAGE_HOST_ROW, gPlayerParty);
    SendJoinInfo(to);
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

    sCoop.guestWorldLoaded = true;
    memcpy(gSaveBlock2Ptr, sSaveData + offset, sizeof(*gSaveBlock2Ptr));
    memcpy(sCoop.hostName, gSaveBlock2Ptr->playerName, sizeof(sCoop.hostName));
    {
        const struct CtrCoopProfile *profile = CtrCoopProfile_Get();
        /* Story placeholders, trainer-card text and newly created Pokemon
         * all read this local identity from SaveBlock2. Keep the shared
         * host snapshot intact and substitute only the guest's RAM name. */
        static const uint8_t defaultName[] = {0xC1, 0xBB, 0xCD, 0xCE}; /* GAST */
        memset(gSaveBlock2Ptr->playerName, EOS, sizeof(gSaveBlock2Ptr->playerName));
        if (profile->guestNameLength > 0 && profile->guestNameLength <= CTR_COOP_GUEST_NAME_LENGTH)
            memcpy(gSaveBlock2Ptr->playerName, profile->guestName, profile->guestNameLength);
        else
            memcpy(gSaveBlock2Ptr->playerName, defaultName, sizeof(defaultName));
    }
    offset += sizeof(*gSaveBlock2Ptr);
    memcpy(gSaveBlock1Ptr, sSaveData + offset, sizeof(*gSaveBlock1Ptr));
    offset += sizeof(*gSaveBlock1Ptr);
    memcpy(gPokemonStoragePtr, sSaveData + offset, sizeof(*gPokemonStoragePtr));
    SetBagItemsPointers();
    SetDecorationInventoriesPointers();
    CopyPartyAndObjectsFromSave();
    LoadPlayerBag();
    memcpy(sRemoteParty, gSaveBlock1Ptr->playerParty, sizeof(sRemoteParty));
    sRemotePartyValid = true;
    LoadPartyRow(CTR_COOP_STORAGE_GUEST_ROW, gPlayerParty);
    ResetStoryBaseline();
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
    sCoop.initialTeamSelection = true;
    sCoop.initialTeamCommitted = false;
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
    bool reloadObjects;

    if (!sCoop.spawnValid || (sCoop.guest && (!sCoop.saveReady || !sCoop.joinInfoReceived
     || (sCoop.starterRequired && !sCoop.starterChosen))))
        return FALSE;
    sCoop.spawnValid = FALSE;
    sCoop.joinWarpPending = true;
    sCoop.generation = LocalLink_Generation();
    reloadObjects = gSaveBlock1Ptr->location.mapGroup != sCoop.spawn.mapGroup
        || gSaveBlock1Ptr->location.mapNum != sCoop.spawn.mapNum;
    if (LocalSnapshot(&old))
        CtrLog_Write(CTR_LOG_GAME, "coop: join warp from map %u.%u at %d,%d to map %u.%u at %d,%d",
            old.mapGroup, old.mapNum, old.x, old.y, sCoop.spawn.mapGroup, sCoop.spawn.mapNum,
            sCoop.spawn.x, sCoop.spawn.y);
    memset(gSaveBlock1Ptr->mapView, 0, sizeof(gSaveBlock1Ptr->mapView));
    SetWarpDestination(sCoop.spawn.mapGroup, sCoop.spawn.mapNum, -1,
        sCoop.spawn.x - MAP_OFFSET, sCoop.spawn.y - MAP_OFFSET);
    WarpIntoMap();
    if (reloadObjects)
    {
        for (u8 i = 0; i < OBJECT_EVENTS_COUNT; i++)
        {
            if (!gObjectEvents[i].isPlayer)
            {
                memset(&gObjectEvents[i], 0, sizeof(gObjectEvents[i]));
                memset(&gSaveBlock1Ptr->objectEvents[i], 0, sizeof(gSaveBlock1Ptr->objectEvents[i]));
            }
        }
        LoadObjEventTemplatesFromHeader();
    }
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
    CtrCoopBike_ClearRemote();
    /* A host save can restore the special avatar before any session has
     * assigned its runtime slot. Find that saved object as well. */
    if (sCoop.objectEventId >= OBJECT_EVENTS_COUNT && gSaveBlock1Ptr != NULL)
        TryGetObjectEventIdByLocalIdAndMap(LOCALID_COOP_PLAYER,
            gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, &sCoop.objectEventId);
    if (RemoteObjectValid())
        RemoveObjectEventByLocalIdAndMap(LOCALID_COOP_PLAYER, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
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
    out->bikeMode = CtrCoopBike_LocalMode();
    out->movementSequence = CtrCoopBike_LocalMovementSequence();
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

static bool NpcFromObject(uint8_t index, CtrCoopNpcSnapshot *out)
{
    const struct ObjectEvent *object = &gObjectEvents[index];

    if (!object->active || object->isPlayer || object->localId == LOCALID_COOP_PLAYER
     || object->mapGroup != gSaveBlock1Ptr->location.mapGroup
     || object->mapNum != gSaveBlock1Ptr->location.mapNum
     || object->facingDirection < 1 || object->facingDirection > 4
     || object->graphicsId > CTR_COOP_GRAPHICS_ID_MAX)
        return false;
    out->localId = object->localId;
    out->mapGroup = object->mapGroup;
    out->mapNum = object->mapNum;
    out->x = object->currentCoords.x;
    out->y = object->currentCoords.y;
    out->facing = object->facingDirection;
    out->graphicsId = object->graphicsId;
    return true;
}

static bool NpcSnapshotsEqual(const CtrCoopNpcSnapshot *a, const CtrCoopNpcSnapshot *b)
{
    return a->localId == b->localId && a->mapGroup == b->mapGroup && a->mapNum == b->mapNum
        && a->x == b->x && a->y == b->y && a->facing == b->facing && a->graphicsId == b->graphicsId;
}

static bool SendHostNpc(const struct LocalPeer *peer, uint8_t index)
{
    CtrCoopNpcSnapshot snapshot;
    uint8_t packet[CTR_COOP_NPC_PACKET_SIZE];

    if (!NpcFromObject(index, &snapshot))
        return false;
    if (CtrCoopProtocol_EncodeNpc(&snapshot, packet, sizeof(packet)))
    {
        LocalLink_Send(peer->id, packet, sizeof(packet));
        sCoop.npcCache[index] = snapshot;
        sCoop.npcCacheValid[index] = true;
        return true;
    }
    return false;
}

static void SyncHostNpc(const struct LocalPeer *peer)
{
    CtrCoopNpcSnapshot snapshot;

    for (uint8_t i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (NpcFromObject(i, &snapshot)
         && (!sCoop.npcCacheValid[i] || !NpcSnapshotsEqual(&sCoop.npcCache[i], &snapshot)))
        {
            SendHostNpc(peer, i);
            return;
        }
    }
    for (uint8_t count = 0; count < OBJECT_EVENTS_COUNT; count++)
    {
        uint8_t index = sCoop.npcIndex;

        sCoop.npcIndex = (sCoop.npcIndex + 1) % OBJECT_EVENTS_COUNT;
        if (SendHostNpc(peer, index))
            return;
    }
}

static void ApplyHostNpc(const CtrCoopNpcSnapshot *snapshot)
{
    uint8_t objectEventId;
    struct ObjectEvent *object;

    /* Menus and battles reuse gSprites while field object IDs remain live.
     * The host's round-robin snapshots refresh NPCs after the field returns. */
    if (gMain.callback2 != CB2_Overworld || snapshot->localId == LOCALID_COOP_PLAYER
     || snapshot->mapGroup != gSaveBlock1Ptr->location.mapGroup
     || snapshot->mapNum != gSaveBlock1Ptr->location.mapNum)
        return;
    if (TryGetObjectEventIdByLocalIdAndMap(snapshot->localId, snapshot->mapNum, snapshot->mapGroup, &objectEventId))
    {
        if (!FreeObjectEventSlotNearPlayer())
            return;
        objectEventId = TrySpawnObjectEvent(snapshot->localId, snapshot->mapNum, snapshot->mapGroup);
        if (objectEventId >= OBJECT_EVENTS_COUNT)
            return;
    }
    object = &gObjectEvents[objectEventId];
    if (object->isPlayer)
        return;
    if (object->graphicsId != snapshot->graphicsId)
        ObjectEventSetGraphicsId(object, snapshot->graphicsId);
    object->singleMovementActive = FALSE;
    object->heldMovementActive = FALSE;
    MoveObjectEventToMapCoords(object, snapshot->x, snapshot->y);
    ObjectEventTurn(object, snapshot->facing);
    object->frozen = TRUE;
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
    CtrCoopBike_SetRemote(sCoop.remote.graphicsId, sCoop.remote.bikeMode);
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
    if (sCoop.haveApplied && sCoop.applied.bikeMode != sCoop.remote.bikeMode
     && ObjectEventIsHeldMovementActive(objectEvent))
        return;
    if (CtrCoopBike_SetRemote(sCoop.remote.graphicsId, sCoop.remote.bikeMode))
    {
        /* The base identity stays the same while mounting/dismounting.
         * Rebind its sprite and palette in place, without losing the actor. */
        ObjectEventSetGraphicsId(objectEvent, sCoop.remote.graphicsId);
        sCoop.haveApplied = false;
    }
    alreadyApplied = !freshSpawn && sCoop.haveApplied && SnapshotsEqual(&sCoop.applied, &sCoop.remote);
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

void CtrCoop_ReleasePC(void)
{
    struct LocalPeer peers[1];
    uint8_t packet[CTR_COOP_PC_CONTROL_SIZE] = {CTR_COOP_PC_RELEASE_PACKET, CTR_COOP_PROTOCOL_VERSION};

    if (sCoop.pcOwner != PC_OWNER_NONE && LocalLink_GetPeers(peers, 1) != 0)
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
    sCoop.pcOwner = PC_OWNER_NONE;
    sCoop.pcPending = false;
    sCoop.pcOpen = false;
}

bool CtrCoop_PCOpen(void)
{
    return sCoop.pcOpen;
}

bool CtrCoop_InitialTeamSelection(void)
{
    return sCoop.guest && sCoop.initialTeamSelection && !sCoop.initialTeamCommitted;
}

bool CtrCoop_ChooseStarter(uint16_t species)
{
    uint8_t start = CTR_COOP_STORAGE_GUEST_ROW * CTR_COOP_STORAGE_ROW_SIZE;

    if (!sCoop.guest || !sCoop.authenticated || !sCoop.saveReady || !sCoop.joinInfoReceived
     || !sCoop.starterRequired || sCoop.starterChosen)
        return false;
    if (species != SPECIES_TREECKO && species != SPECIES_TORCHIC && species != SPECIES_MUDKIP)
        return false;
    for (uint8_t i = 0; i < CTR_COOP_STORAGE_ROW_SIZE; i++)
    {
        if (GetBoxMonData(&gPokemonStoragePtr->boxes[CTR_COOP_STORAGE_BOX_ID][start + i], MON_DATA_SANITY_HAS_SPECIES))
            return false;
    }
    for (uint8_t i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gPlayerParty[i]);
    CreateMon(&gPlayerParty[0], species, 5, 0, FALSE, 0, OT_ID_PLAYER_ID, 0);
    gPlayerPartyCount = 1;
    StorePartyRow(CTR_COOP_STORAGE_GUEST_ROW, gPlayerParty);
    sCoop.starterChosen = true;
    CtrLog_Write(CTR_LOG_GAME, "coop: starter %u stored in guest row", species);
    return true;
}

void CtrCoop_FinishJoin(void)
{
    if (sCoop.guest && sCoop.saveReady && gPokemonStoragePtr != NULL)
        LoadPartyRow(CTR_COOP_STORAGE_GUEST_ROW, gPlayerParty);
}

bool CtrCoop_StorageSlotLocked(uint8_t boxId, uint8_t position)
{
    /* The host's mirrored party still exists when the radio is off. */
    bool hostActive = sCoop.guest ? sCoop.wanted && sCoop.authenticated : CoopStorageReady();
    bool guestActive = sCoop.guest ? sCoop.authenticated : sCoop.peerAuthenticated && sCoop.peerSaveReady && !sCoop.peerLoggedOut;

    return CtrCoopStorage_SlotLocked(hostActive, guestActive, boxId, position);
}

bool CtrCoop_StorageBoxRestricted(uint8_t boxId)
{
    bool hostActive = sCoop.guest ? sCoop.wanted && sCoop.authenticated : CoopStorageReady();
    bool guestActive = sCoop.guest ? sCoop.authenticated : sCoop.peerAuthenticated && sCoop.peerSaveReady && !sCoop.peerLoggedOut;

    return CtrCoopStorage_BoxRestricted(hostActive, guestActive, boxId);
}

bool CtrCoop_IsDoubleBattle(void)
{
    return sCoop.doubleState == DOUBLE_STATE_ACTIVE;
}

bool CtrCoop_IsDoubleInitiator(void)
{
    return sCoop.doubleInitiator;
}

uint8_t CtrCoop_LocalMultiplayerId(void)
{
    return sCoop.guest ? 1 : 0;
}

static bool DoublePeerAvailable(void)
{
    CtrCoopSnapshot local;

    if (!sCoop.wanted || !sCoop.remoteValid || sCoop.battleActive || sCoop.battleReservePending
     || sCoop.pcOpen || sCoop.doubleState != DOUBLE_STATE_IDLE || !LocalSnapshot(&local))
        return false;
    return CtrCoopProtocol_SameMap(&local, &sCoop.remote)
        && abs(local.x - sCoop.remote.x) <= 3 && abs(local.y - sCoop.remote.y) <= 3;
}

static void WriteDouble32(uint8_t *data, uint32_t offset, uint32_t value)
{
    data[offset] = value & 0xFF;
    data[offset + 1] = (value >> 8) & 0xFF;
    data[offset + 2] = (value >> 16) & 0xFF;
    data[offset + 3] = value >> 24;
}

static uint32_t ReadDouble32(const uint8_t *data, uint32_t offset)
{
    return data[offset] | ((uint32_t)data[offset + 1] << 8)
        | ((uint32_t)data[offset + 2] << 16) | ((uint32_t)data[offset + 3] << 24);
}

static void SendDoubleRequest(void)
{
    struct LocalPeer peers[1];
    uint8_t packet[CTR_COOP_DOUBLE_REQUEST_SIZE] = {
        CTR_COOP_DOUBLE_REQUEST_PACKET, CTR_COOP_PROTOCOL_VERSION,
        sCoop.battleTrainerId & 0xFF, sCoop.battleTrainerId >> 8
    };

    if (LocalLink_GetPeers(peers, 1) != 0)
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
}

static void SendDoubleBegin(void)
{
    struct LocalPeer peers[1];
    uint8_t packet[CTR_COOP_SAVE_BEGIN_SIZE];

    if (sCoop.doubleState != DOUBLE_STATE_SENDING || LocalLink_GetPeers(peers, 1) == 0)
        return;
    if (CtrCoopProtocol_EncodeDoubleBegin(DOUBLE_SETUP_SIZE, sCoop.doubleSendChecksum, packet, sizeof(packet)))
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
}

static void BuildDoubleParty(uint8_t *out)
{
    struct Pokemon hostParty[PARTY_SIZE];
    struct Pokemon guestParty[PARTY_SIZE];

    memcpy(sDoubleLocalParty, gPlayerParty, sizeof(sDoubleLocalParty));
    if (sCoop.guest)
    {
        memcpy(guestParty, gPlayerParty, sizeof(guestParty));
        if (sRemotePartyValid)
            memcpy(hostParty, sRemoteParty, sizeof(hostParty));
        else
            LoadPartyRow(CTR_COOP_STORAGE_HOST_ROW, hostParty);
    }
    else
    {
        memcpy(hostParty, gPlayerParty, sizeof(hostParty));
        if (sRemotePartyValid)
            memcpy(guestParty, sRemoteParty, sizeof(guestParty));
        else
            LoadPartyRow(CTR_COOP_STORAGE_GUEST_ROW, guestParty);
    }
    if (!sRemotePartyValid)
    {
        memcpy(sRemoteParty, sCoop.guest ? hostParty : guestParty, sizeof(sRemoteParty));
        sRemotePartyValid = true;
    }
    memcpy(out, hostParty, sizeof(struct Pokemon) * MULTI_PARTY_SIZE);
    memcpy(out + sizeof(struct Pokemon) * MULTI_PARTY_SIZE, guestParty, sizeof(struct Pokemon) * MULTI_PARTY_SIZE);
}

static void ConfigureDoubleBattle(const uint8_t *data)
{
    uint32_t flags = ReadDouble32(data, 0);

    sDoubleOriginalFlags = flags & ~(BATTLE_TYPE_LINK | BATTLE_TYPE_MULTI | BATTLE_TYPE_IS_MASTER | BATTLE_TYPE_LINK_IN_BATTLE);
    memcpy(gEnemyParty, data + DOUBLE_HEADER_SIZE, sizeof(gEnemyParty));
    memcpy(gPlayerParty, data + DOUBLE_HEADER_SIZE + sizeof(gEnemyParty), sizeof(gPlayerParty));
    gPlayerPartyCount = CalculatePlayerPartyCount();
    gBattleTypeFlags = flags;
    if (!sCoop.guest)
        gBattleTypeFlags |= BATTLE_TYPE_IS_MASTER;
    else
        gBattleTypeFlags &= ~BATTLE_TYPE_IS_MASTER;
    gRngValue = ReadDouble32(data, 4);
    gTrainerBattleOpponent_A = data[8] | ((uint16_t)data[9] << 8);
    gTrainerBattleOpponent_B = data[10] | ((uint16_t)data[11] << 8);
    gMain.savedCallback = (MainCallback)(uintptr_t)ReadDouble32(data, 12);
    gBattleEnvironment = data[16];
    gWirelessCommType = 1;
    gReceivedRemoteLinkPlayers = 1;
    memset(gLinkPlayers, 0, sizeof(gLinkPlayers));
    gLinkPlayers[0].id = 0;
    gLinkPlayers[0].version = VERSION_EMERALD;
    gLinkPlayers[1].id = 1;
    gLinkPlayers[1].version = VERSION_EMERALD;
    sCoop.doubleState = DOUBLE_STATE_ACTIVE;
    SetMainCallback2(CB2_InitBattle);
}

static void PrepareDoubleSetup(void)
{
    uint32_t flags = gBattleTypeFlags | BATTLE_TYPE_LINK | BATTLE_TYPE_MULTI;

    sDoubleOriginalFlags = gBattleTypeFlags;
    if (!sCoop.guest)
        flags |= BATTLE_TYPE_IS_MASTER;
    else
        flags &= ~BATTLE_TYPE_IS_MASTER;
    WriteDouble32(sDoubleSendData, 0, flags);
    WriteDouble32(sDoubleSendData, 4, Random());
    sDoubleSendData[8] = gTrainerBattleOpponent_A & 0xFF;
    sDoubleSendData[9] = gTrainerBattleOpponent_A >> 8;
    sDoubleSendData[10] = gTrainerBattleOpponent_B & 0xFF;
    sDoubleSendData[11] = gTrainerBattleOpponent_B >> 8;
    WriteDouble32(sDoubleSendData, 12, (uintptr_t)gMain.savedCallback);
    sDoubleSendData[16] = gBattleEnvironment;
    memset(sDoubleSendData + 17, 0, DOUBLE_HEADER_SIZE - 17);
    memcpy(sDoubleSendData + DOUBLE_HEADER_SIZE, gEnemyParty, sizeof(gEnemyParty));
    BuildDoubleParty(sDoubleSendData + DOUBLE_HEADER_SIZE + sizeof(gEnemyParty));
    sCoop.doubleSendOffset = 0;
    sCoop.doubleSendChecksum = CtrCoopProtocol_Checksum(sDoubleSendData, DOUBLE_SETUP_SIZE);
    sCoop.doubleState = DOUBLE_STATE_SENDING;
    sCoop.doubleRetry = 0;
    SendDoubleBegin();
}

static void SendDoubleRequestData(uint16_t to, uint32_t offset)
{
    uint8_t packet[CTR_COOP_SAVE_REQUEST_SIZE];

    if (CtrCoopProtocol_EncodeDoubleRequestData(offset, packet, sizeof(packet)))
        LocalLink_Send(to, packet, sizeof(packet));
}

static void SendDoubleChunk(uint16_t to, uint32_t offset)
{
    uint8_t packet[CTR_COOP_SAVE_CHUNK_MAX];
    uint16_t size;

    if (sCoop.doubleState != DOUBLE_STATE_SENDING || offset >= DOUBLE_SETUP_SIZE)
        return;
    size = DOUBLE_SETUP_SIZE - offset;
    if (size > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        size = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (CtrCoopProtocol_EncodeDoubleChunk(offset, sDoubleSendData + offset, size, packet, CTR_COOP_SAVE_CHUNK_HEADER + size))
        LocalLink_Send(to, packet, CTR_COOP_SAVE_CHUNK_HEADER + size);
}

static void HandleDoubleRequest(uint16_t from)
{
    uint8_t packet[CTR_COOP_DOUBLE_CONTROL_SIZE] = {
        DoublePeerAvailable() ? CTR_COOP_DOUBLE_ACCEPT_PACKET : CTR_COOP_DOUBLE_DECLINE_PACKET,
        CTR_COOP_PROTOCOL_VERSION
    };

    if (packet[0] == CTR_COOP_DOUBLE_ACCEPT_PACKET)
    {
        sCoop.doubleInitiator = false;
        sCoop.doubleState = DOUBLE_STATE_WAIT_START;
        sCoop.doubleRetry = 0;
    }
    LocalLink_Send(from, packet, sizeof(packet));
}

static void HandleDoubleBegin(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint32_t totalSize, checksum;

    if (sCoop.doubleState != DOUBLE_STATE_WAIT_START
     || !CtrCoopProtocol_DecodeDoubleBegin(data, len, &totalSize, &checksum)
     || totalSize != DOUBLE_SETUP_SIZE)
        return;
    sCoop.doubleReceiveOffset = 0;
    sCoop.doubleReceiveChecksum = checksum;
    sCoop.doubleState = DOUBLE_STATE_RECEIVING;
    SendDoubleRequestData(from, 0);
}

static void HandleDoubleChunk(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint16_t got, expected;
    uint8_t ready[CTR_COOP_DOUBLE_CONTROL_SIZE] = {CTR_COOP_DOUBLE_READY_PACKET, CTR_COOP_PROTOCOL_VERSION};

    if (sCoop.doubleState != DOUBLE_STATE_RECEIVING || sCoop.doubleReceiveOffset >= DOUBLE_SETUP_SIZE)
        return;
    if (!CtrCoopProtocol_DecodeDoubleChunk(data, len, sCoop.doubleReceiveOffset,
            sDoubleReceiveData + sCoop.doubleReceiveOffset, DOUBLE_SETUP_SIZE - sCoop.doubleReceiveOffset, &got))
        return;
    expected = DOUBLE_SETUP_SIZE - sCoop.doubleReceiveOffset;
    if (expected > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        expected = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (got != expected)
        return;
    sCoop.doubleReceiveOffset += got;
    if (sCoop.doubleReceiveOffset < DOUBLE_SETUP_SIZE)
    {
        SendDoubleRequestData(from, sCoop.doubleReceiveOffset);
        return;
    }
    if (CtrCoopProtocol_Checksum(sDoubleReceiveData, DOUBLE_SETUP_SIZE) != sCoop.doubleReceiveChecksum)
    {
        sCoop.doubleReceiveOffset = 0;
        SendDoubleRequestData(from, 0);
        return;
    }
    memcpy(sDoubleLocalParty, gPlayerParty, sizeof(sDoubleLocalParty));
    ConfigureDoubleBattle(sDoubleReceiveData);
    LocalLink_Send(from, ready, sizeof(ready));
}

static bool PartyPeerReady(void)
{
    return sCoop.wanted && (sCoop.guest
        ? sCoop.authenticated && sCoop.saveReady && sCoop.initialTeamCommitted
        : sCoop.peerAuthenticated && sCoop.peerTeamReady);
}

static void SyncParty(void)
{
    struct LocalPeer peers[1];
    uint8_t packet[CTR_COOP_SAVE_CHUNK_MAX];
    uint8_t own = sCoop.guest ? PC_OWNER_GUEST : PC_OWNER_HOST;
    uint32_t checksum;

    if (sCoop.battleActive || !CoopStorageReady() || (sCoop.guest && !PartyPeerReady()))
        return;
    SaveExtension();
    /* A mon held by the PC cursor is an unfinished transaction. Keep its
     * previous row until CommitPC captures the complete operation. */
    if (sCoop.pcOwner != own || !sCoop.pcOpen)
        StorePartyRow(sCoop.guest ? CTR_COOP_STORAGE_GUEST_ROW : CTR_COOP_STORAGE_HOST_ROW, gPlayerParty);
    if (!PartyPeerReady() || LocalLink_GetSession() != LOCAL_SESSION_IN
     || sCoop.pcOwner != PC_OWNER_NONE || sCoop.pcPending || sCoop.syncState != PC_SYNC_IDLE
     || sCoop.partySending)
        return;
    if (sCoop.partyRetry != 0)
    {
        sCoop.partyRetry--;
        return;
    }
    checksum = CtrCoopProtocol_Checksum((const uint8_t *)gPlayerParty, sizeof(gPlayerParty));
    if (sCoop.havePartyAck && checksum == sCoop.partyAckChecksum)
        return;
    if (LocalLink_GetPeers(peers, 1) == 0)
        return;
    memcpy(sPartySend, gPlayerParty, sizeof(sPartySend));
    sCoop.partySendChecksum = checksum;
    sCoop.partySending = true;
    CtrCoopProtocol_EncodePartyBegin(sizeof(sPartySend), checksum, packet, CTR_COOP_SAVE_BEGIN_SIZE);
    LocalLink_Send(peers[0].id, packet, CTR_COOP_SAVE_BEGIN_SIZE);
    /* Six reliable messages fit in one UDS frame. Push the frozen party in
     * one batch instead of taking a request round trip for every chunk. */
    for (uint32_t offset = 0; offset < sizeof(sPartySend); )
    {
        uint16_t size = sizeof(sPartySend) - offset;
        if (size > CTR_COOP_SAVE_CHUNK_DATA_MAX)
            size = CTR_COOP_SAVE_CHUNK_DATA_MAX;
        CtrCoopProtocol_EncodePartyChunk(offset, sPartySend + offset, size, packet, CTR_COOP_SAVE_CHUNK_HEADER + size);
        LocalLink_Send(peers[0].id, packet, CTR_COOP_SAVE_CHUNK_HEADER + size);
        offset += size;
    }
}

static void SendPartyResult(uint16_t to, bool accepted)
{
    uint8_t packet[CTR_COOP_PARTY_RESULT_SIZE];

    CtrCoopProtocol_EncodePartyResult(accepted, packet, sizeof(packet));
    LocalLink_Send(to, packet, sizeof(packet));
}

static void HandlePartyBegin(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint32_t size, checksum;

    if (!CtrCoopProtocol_DecodePartyBegin(data, len, &size, &checksum))
        return;
    if (size != sizeof(sPartyReceive) || !PartyPeerReady() || !CoopStorageReady() || sCoop.pcOwner != PC_OWNER_NONE
     || sCoop.pcPending || sCoop.syncState != PC_SYNC_IDLE || sCoop.partyReceiving
     || sCoop.battleReceiveState != BATTLE_SYNC_IDLE)
    {
        SendPartyResult(from, false);
        return;
    }
    sCoop.partyOffset = 0;
    sCoop.partyReceiveChecksum = checksum;
    sCoop.partyReceiving = true;
}

static void HandlePartyChunk(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint16_t got;
    uint16_t expected;

    if (!sCoop.partyReceiving)
        return;
    expected = sizeof(sPartyReceive) - sCoop.partyOffset;
    if (expected > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        expected = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (!CtrCoopProtocol_DecodePartyChunk(data, len, sCoop.partyOffset,
            sPartyReceive + sCoop.partyOffset, sizeof(sPartyReceive) - sCoop.partyOffset, &got)
     || got != expected)
    {
        sCoop.partyReceiving = false;
        SendPartyResult(from, false);
        return;
    }
    sCoop.partyOffset += got;
    if (sCoop.partyOffset != sizeof(sPartyReceive))
        return;
    sCoop.partyReceiving = false;
    if (CtrCoopProtocol_Checksum(sPartyReceive, sizeof(sPartyReceive)) != sCoop.partyReceiveChecksum)
    {
        SendPartyResult(from, false);
        return;
    }
    StorePartyRow(sCoop.guest ? CTR_COOP_STORAGE_HOST_ROW : CTR_COOP_STORAGE_GUEST_ROW,
        (const struct Pokemon *)sPartyReceive);
    SendPartyResult(from, true);
    CtrLog_Write(CTR_LOG_GAME, "coop: party row updated (%s)", sCoop.guest ? "host" : "guest");
}

static void SendBattleResult(void);
static void SendBattleBegin(void);
static void TickLogout(void);
static void SendFlagUpdate(uint16_t to, uint16_t id, bool value);
static void FinishBattleReceive(uint16_t from);

static bool BeginLocalBattle(void)
{
    if (gSaveBlock1Ptr == NULL || gPokemonStoragePtr == NULL)
        return false;
    memcpy(&sBattleSaveBaseline, gSaveBlock1Ptr, sizeof(sBattleSaveBaseline));
    memcpy(&sBattleStorageBaseline, gPokemonStoragePtr, sizeof(sBattleStorageBaseline));
    sCoop.battleBaselineKey = gSaveBlock2Ptr->encryptionKey;
    sCoop.battleActive = true;
    sCoop.battleWaitingTrainer = false;
    sCoop.battleRetry = 0;
    CtrLog_Write(CTR_LOG_GAME, "coop: battle start for %s, trainer %u", sCoop.guest ? "guest" : "host", sCoop.battleTrainerId);
    return true;
}

static bool BeginBattleOrDouble(void)
{
    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
        return BeginLocalBattle();
    if (sCoop.doubleState == DOUBLE_STATE_ACTIVE)
        return BeginLocalBattle();
    if (sCoop.doubleState != DOUBLE_STATE_IDLE)
    {
        if (sCoop.doubleRetry != 0)
            sCoop.doubleRetry--;
        return false;
    }
    if (!DoublePeerAvailable())
        return BeginLocalBattle();
    sCoop.doubleInitiator = true;
    sCoop.doubleState = DOUBLE_STATE_WAIT_ACCEPT;
    sCoop.doubleRetry = 0;
    SendDoubleRequest();
    return false;
}

bool CtrCoop_BattleStart(void)
{
    uint8_t packet[CTR_COOP_BATTLE_RESERVE_SIZE];
    struct LocalPeer peers[1];
    uint16_t trainerId;

    if (sCoop.doubleState == DOUBLE_STATE_ACTIVE)
        return BeginLocalBattle();
    if (sCoop.battleActive || !sCoop.wanted || (gBattleTypeFlags & BATTLE_TYPE_LINK))
        return true;
    if (sCoop.battleSendState != BATTLE_SYNC_IDLE || sCoop.battleReleasePending
     || sCoop.battleReceiveState != BATTLE_SYNC_IDLE || sCoop.pcOwner != PC_OWNER_NONE
     || sCoop.syncState != PC_SYNC_IDLE)
        return false;
    /* Zero reserves an independent wild battle without locking a trainer.
     * The host arbitrates this against PC claims before either side mutates storage. */
    trainerId = (gBattleTypeFlags & BATTLE_TYPE_TRAINER) ? gTrainerBattleOpponent_A : 0;
    if (trainerId != 0 && sCoop.battleWaitingTrainer && sCoop.battleTrainerId == trainerId
     && !sCoop.battleTrainerWasDefeated && FlagGet(TRAINER_FLAGS_START + trainerId))
    {
        /* The other player finished this trainer while our transition was
         * waiting. Resume the existing field callback without a second fight. */
        sCoop.battleWaitingTrainer = false;
        sCoop.battleReservePending = false;
        sCoop.battleReserved = false;
        sCoop.battleTrainerId = 0;
        gBattleOutcome = B_OUTCOME_RAN;
        SetMainCallback2(gMain.savedCallback);
        return false;
    }
    if (!sCoop.battleWaitingTrainer || sCoop.battleTrainerId != trainerId)
        sCoop.battleTrainerWasDefeated = trainerId != 0 && FlagGet(TRAINER_FLAGS_START + trainerId);
    sCoop.battleWaitingTrainer = true;
    sCoop.battleTrainerId = trainerId;
    if (!sCoop.guest)
    {
        if (trainerId != 0 && sCoop.peerBattleActive && sCoop.peerTrainerId == trainerId)
            return false;
        sCoop.battleReserved = true;
        return BeginBattleOrDouble();
    }
    if (sCoop.battleReserved)
        return BeginBattleOrDouble();
    if (sCoop.battleReservePending || !sCoop.authenticated || !sCoop.saveReady)
        return false;
    if (sCoop.battleRetry != 0)
    {
        sCoop.battleRetry--;
        return false;
    }
    if (LocalLink_GetPeers(peers, 1) == 0)
        return false;
    if (CtrCoopProtocol_EncodeBattleReserve(trainerId, packet, sizeof(packet)))
    {
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
        sCoop.battleReservePending = true;
        sCoop.battleRetry = 60;
    }
    return false;
}

static void RestoreDoubleParty(void)
{
    uint8_t ownStart = sCoop.guest ? MULTI_PARTY_SIZE : 0;
    uint8_t remoteStart = sCoop.guest ? 0 : MULTI_PARTY_SIZE;

    if (sCoop.doubleState != DOUBLE_STATE_ACTIVE)
        return;
    for (uint8_t i = 0; i < MULTI_PARTY_SIZE; i++)
    {
        sDoubleLocalParty[ownStart + i] = gPlayerParty[ownStart + i];
        sRemoteParty[remoteStart + i] = gPlayerParty[remoteStart + i];
    }
    memcpy(gPlayerParty, sDoubleLocalParty, sizeof(gPlayerParty));
    gPlayerPartyCount = CalculatePlayerPartyCount();
    sRemotePartyValid = true;
}

void CtrCoop_BattleEnd(void)
{
    if (!sCoop.battleActive)
        return;
    RestoreDoubleParty();
    SendBattleResult();
    if (sCoop.doubleState == DOUBLE_STATE_ACTIVE)
    {
        sCoop.doubleState = DOUBLE_STATE_IDLE;
        sCoop.doubleInitiator = false;
        gBattleTypeFlags = sDoubleOriginalFlags;
        gWirelessCommType = 0;
        gReceivedRemoteLinkPlayers = 0;
    }
    sCoop.battleActive = false;
    sCoop.battleReservePending = false;
    sCoop.battleReserved = false;
    sCoop.battleReleasePending = sCoop.wanted && (sCoop.guest || sCoop.battleTrainerId != 0);
    if (!sCoop.battleReleasePending)
        sCoop.battleTrainerId = 0;
    sCoop.battleRetry = 0;
}

static void ReleaseTrainerReservation(void)
{
    struct LocalPeer peers[1];
    uint8_t packet[CTR_COOP_BATTLE_RESERVE_SIZE];
    uint16_t trainerFlag;
    uint8_t mask;

    if (!sCoop.battleReleasePending || sCoop.battleSendState != BATTLE_SYNC_IDLE)
        return;
    if (sCoop.battleTrainerId != 0 && gBattleOutcome == B_OUTCOME_WON)
    {
        trainerFlag = TRAINER_FLAGS_START + sCoop.battleTrainerId;
        if (!FlagGet(trainerFlag))
            return;
        if (sCoop.guest)
        {
            mask = 1 << (trainerFlag & 7);
            if ((sCoop.storyFlagsBaseline[trainerFlag / 8] & mask) == 0)
            {
                if (LocalLink_GetPeers(peers, 1) == 0)
                    return;
                sCoop.storyFlagsBaseline[trainerFlag / 8] |= mask;
                SendFlagUpdate(peers[0].id, trainerFlag, true);
            }
        }
    }
    if (sCoop.guest)
    {
        if (LocalLink_GetPeers(peers, 1) == 0)
            return;
        packet[0] = CTR_COOP_BATTLE_RELEASE_PACKET;
        packet[1] = CTR_COOP_PROTOCOL_VERSION;
        packet[2] = sCoop.battleTrainerId & 0xFF;
        packet[3] = sCoop.battleTrainerId >> 8;
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
    }
    sCoop.battleReleasePending = false;
    sCoop.battleTrainerId = 0;
}

void CtrCoop_BattleTick(void)
{
    TickLogout();
    if (sCoop.battleReceiveState == BATTLE_SYNC_READY && !sCoop.battleActive
     && sCoop.pcOwner == PC_OWNER_NONE && !sCoop.pcPending
     && sCoop.syncState == PC_SYNC_IDLE && !sCoop.partyReceiving)
        FinishBattleReceive(sCoop.peerId);
    if (sCoop.battleReservePending && ++sCoop.battleRetry >= 120)
    {
        struct LocalPeer peers[1];
        uint8_t packet[CTR_COOP_BATTLE_RESERVE_SIZE];
        sCoop.battleRetry = 0;
        if (LocalLink_GetPeers(peers, 1) != 0
         && CtrCoopProtocol_EncodeBattleReserve(sCoop.battleTrainerId, packet, sizeof(packet)))
            LocalLink_Send(peers[0].id, packet, sizeof(packet));
    }
    if (sCoop.battleSendState == BATTLE_SYNC_SENDING && ++sCoop.battleRetry >= 120)
    {
        sCoop.battleRetry = 0;
        SendBattleBegin();
    }
    if ((sCoop.doubleState == DOUBLE_STATE_WAIT_ACCEPT || sCoop.doubleState == DOUBLE_STATE_SENDING)
     && ++sCoop.doubleRetry >= 120)
    {
        sCoop.doubleRetry = 0;
        if (sCoop.doubleState == DOUBLE_STATE_WAIT_ACCEPT)
            SendDoubleRequest();
        else
            SendDoubleBegin();
    }
}

static void WriteBattle16(uint32_t offset, uint16_t value)
{
    sBattleSendData[offset] = value & 0xFF;
    sBattleSendData[offset + 1] = value >> 8;
}

static void WriteBattle32(uint32_t offset, uint32_t value)
{
    sBattleSendData[offset] = value & 0xFF;
    sBattleSendData[offset + 1] = (value >> 8) & 0xFF;
    sBattleSendData[offset + 2] = (value >> 16) & 0xFF;
    sBattleSendData[offset + 3] = value >> 24;
}

static uint16_t ReadBattle16(const uint8_t *data, uint32_t offset)
{
    return data[offset] | ((uint16_t)data[offset + 1] << 8);
}

static uint32_t ReadBattle32(const uint8_t *data, uint32_t offset)
{
    return data[offset] | ((uint32_t)data[offset + 1] << 8)
        | ((uint32_t)data[offset + 2] << 16) | ((uint32_t)data[offset + 3] << 24);
}

static void AddBattleItemDelta(uint32_t *offset, uint16_t *count, uint16_t itemId, int32_t delta)
{
    if (itemId == ITEM_NONE || delta == 0)
        return;
    WriteBattle16(*offset, itemId);
    WriteBattle16(*offset + 2, (uint16_t)(int16_t)delta);
    *offset += BATTLE_ITEM_RECORD_SIZE;
    (*count)++;
}

static void AddBattleItemArrayDeltas(const struct ItemSlot *before, const struct ItemSlot *after,
    uint16_t slots, bool encrypted, uint32_t *offset, uint16_t *count)
{
    for (uint16_t i = 0; i < slots; i++)
    {
        uint16_t oldQuantity = before[i].quantity ^ (encrypted ? (uint16_t)sCoop.battleBaselineKey : 0);
        uint16_t newQuantity = after[i].quantity ^ (encrypted ? (uint16_t)gSaveBlock2Ptr->encryptionKey : 0);
        int32_t delta = (int32_t)newQuantity - oldQuantity;

        if (before[i].itemId == after[i].itemId)
        {
            AddBattleItemDelta(offset, count, after[i].itemId, delta);
        }
        else
        {
            AddBattleItemDelta(offset, count, before[i].itemId, -(int32_t)oldQuantity);
            AddBattleItemDelta(offset, count, after[i].itemId, newQuantity);
        }
    }
}

static void SendBattleBegin(void)
{
    struct LocalPeer peers[1];
    uint8_t packet[CTR_COOP_SAVE_BEGIN_SIZE];

    if (sCoop.battleSendState != BATTLE_SYNC_SENDING || LocalLink_GetPeers(peers, 1) == 0)
        return;
    if (CtrCoopProtocol_EncodeBattleBegin(sCoop.battleSendSize, sCoop.battleSendChecksum, packet, sizeof(packet)))
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
}

static void SendBattleResult(void)
{
    uint32_t offset = BATTLE_HEADER_SIZE;
    uint16_t storageCount = 0;
    uint16_t itemCount = 0;
    int32_t moneyDelta;
    int16_t coinsDelta;
    uint8_t row = sCoop.guest ? CTR_COOP_STORAGE_GUEST_ROW : CTR_COOP_STORAGE_HOST_ROW;
    struct LocalPeer peers[1];

    StorePartyRow(row, gPlayerParty);
    if (!sCoop.guest)
    {
        gSaveBlock1Ptr->playerPartyCount = gPlayerPartyCount;
        memcpy(gSaveBlock1Ptr->playerParty, gPlayerParty, sizeof(gPlayerParty));
    }
    if ((sCoop.doubleState == DOUBLE_STATE_ACTIVE && !sCoop.doubleInitiator)
     || !sCoop.wanted || sCoop.battleSendState != BATTLE_SYNC_IDLE
     || (sCoop.guest && !sCoop.authenticated) || (!sCoop.guest && !sCoop.peerSaveReady)
     || LocalLink_GetPeers(peers, 1) == 0)
        return;
    memcpy(sBattleSendData + offset, gPlayerParty, sizeof(gPlayerParty));
    offset += sizeof(gPlayerParty);
    for (uint8_t box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (uint8_t position = 0; position < IN_BOX_COUNT; position++)
        {
            const struct BoxPokemon *before = &sBattleStorageBaseline.boxes[box][position];
            const struct BoxPokemon *after = &gPokemonStoragePtr->boxes[box][position];

            if (box == CTR_COOP_STORAGE_BOX_ID && position < CTR_COOP_STORAGE_ROW_SIZE * 2)
                continue;
            if (memcmp(before, after, sizeof(*before)) == 0)
                continue;
            sBattleSendData[offset++] = box;
            sBattleSendData[offset++] = position;
            memcpy(sBattleSendData + offset, after, sizeof(*after));
            offset += sizeof(*after);
            storageCount++;
        }
    }
    AddBattleItemArrayDeltas(sBattleSaveBaseline.pcItems, gSaveBlock1Ptr->pcItems, PC_ITEMS_COUNT, false, &offset, &itemCount);
    AddBattleItemArrayDeltas(sBattleSaveBaseline.bagPocket_Items, gSaveBlock1Ptr->bagPocket_Items, BAG_ITEMS_COUNT, true, &offset, &itemCount);
    AddBattleItemArrayDeltas(sBattleSaveBaseline.bagPocket_KeyItems, gSaveBlock1Ptr->bagPocket_KeyItems, BAG_KEYITEMS_COUNT, true, &offset, &itemCount);
    AddBattleItemArrayDeltas(sBattleSaveBaseline.bagPocket_PokeBalls, gSaveBlock1Ptr->bagPocket_PokeBalls, BAG_POKEBALLS_COUNT, true, &offset, &itemCount);
    AddBattleItemArrayDeltas(sBattleSaveBaseline.bagPocket_TMHM, gSaveBlock1Ptr->bagPocket_TMHM, BAG_TMHM_COUNT, true, &offset, &itemCount);
    AddBattleItemArrayDeltas(sBattleSaveBaseline.bagPocket_Berries, gSaveBlock1Ptr->bagPocket_Berries, BAG_BERRIES_COUNT, true, &offset, &itemCount);
    moneyDelta = (int32_t)GetMoney(&gSaveBlock1Ptr->money) - (int32_t)(sBattleSaveBaseline.money ^ sCoop.battleBaselineKey);
    coinsDelta = (int16_t)((int32_t)(gSaveBlock1Ptr->coins ^ (uint16_t)gSaveBlock2Ptr->encryptionKey)
        - (sBattleSaveBaseline.coins ^ (uint16_t)sCoop.battleBaselineKey));
    WriteBattle32(0, 0x31425443);
    WriteBattle32(4, (uint32_t)moneyDelta);
    WriteBattle16(8, (uint16_t)coinsDelta);
    WriteBattle16(10, storageCount);
    WriteBattle16(12, itemCount);
    WriteBattle16(14, ++sCoop.battleSendSequence);
    WriteBattle16(16, sCoop.doubleState == DOUBLE_STATE_ACTIVE ? 1 : 0);
    WriteBattle16(18, 0);
    sCoop.battleSendSize = offset;
    sCoop.battleSendOffset = 0;
    sCoop.battleSendChecksum = CtrCoopProtocol_Checksum(sBattleSendData, offset);
    sCoop.battleSendState = BATTLE_SYNC_SENDING;
    sCoop.battleRetry = 0;
    CtrLog_Write(CTR_LOG_GAME, "coop: battle result %lu bytes, %u storage and %u item changes",
        (unsigned long)offset, storageCount, itemCount);
    SendBattleBegin();
}

static void SendBattleRequest(uint16_t to, uint32_t offset)
{
    uint8_t packet[CTR_COOP_SAVE_REQUEST_SIZE];

    if (CtrCoopProtocol_EncodeBattleRequest(offset, packet, sizeof(packet)))
        LocalLink_Send(to, packet, sizeof(packet));
}

static void SendBattleChunk(uint16_t to, uint32_t offset)
{
    uint8_t packet[CTR_COOP_SAVE_CHUNK_MAX];
    uint16_t size;

    if (sCoop.battleSendState != BATTLE_SYNC_SENDING || offset >= sCoop.battleSendSize)
        return;
    size = sCoop.battleSendSize - offset;
    if (size > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        size = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (CtrCoopProtocol_EncodeBattleChunk(offset, sBattleSendData + offset, size, packet, CTR_COOP_SAVE_CHUNK_HEADER + size))
        LocalLink_Send(to, packet, CTR_COOP_SAVE_CHUNK_HEADER + size);
}

static void HandleBattleBegin(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint32_t totalSize, checksum;

    if (!CtrCoopProtocol_DecodeBattleBegin(data, len, &totalSize, &checksum)
     || totalSize < BATTLE_HEADER_SIZE + sizeof(struct Pokemon) * PARTY_SIZE
     || totalSize > CTR_COOP_BATTLE_DATA_MAX)
        return;
    if (sCoop.battleReceiveState == BATTLE_SYNC_RECEIVING
     && totalSize == sCoop.battleReceiveSize && checksum == sCoop.battleReceiveChecksum)
    {
        SendBattleRequest(from, sCoop.battleReceiveOffset);
        return;
    }
    if (sCoop.battleReceiveState != BATTLE_SYNC_IDLE)
        return;
    sCoop.battleReceiveSize = totalSize;
    sCoop.battleReceiveChecksum = checksum;
    sCoop.battleReceiveOffset = 0;
    sCoop.battleReceiveState = BATTLE_SYNC_RECEIVING;
    SendBattleRequest(from, 0);
}

static bool ApplyBattleResult(void)
{
    uint16_t storageCount, itemCount;
    uint32_t offset = BATTLE_HEADER_SIZE + sizeof(struct Pokemon) * PARTY_SIZE;
    uint32_t expectedSize;
    uint16_t destinations[TOTAL_BOXES_COUNT * IN_BOX_COUNT];
    bool occupied[TOTAL_BOXES_COUNT * IN_BOX_COUNT];
    bool mentioned[TOTAL_BOXES_COUNT * IN_BOX_COUNT] = {false};
    uint8_t remoteRow = sCoop.guest ? CTR_COOP_STORAGE_HOST_ROW : CTR_COOP_STORAGE_GUEST_ROW;
    const struct Pokemon *remoteParty = (const struct Pokemon *)(sBattleReceiveData + BATTLE_HEADER_SIZE);

    if (ReadBattle32(sBattleReceiveData, 0) != 0x31425443)
        return false;
    storageCount = ReadBattle16(sBattleReceiveData, 10);
    itemCount = ReadBattle16(sBattleReceiveData, 12);
    if (storageCount > TOTAL_BOXES_COUNT * IN_BOX_COUNT || itemCount > BATTLE_ITEM_SLOTS * 2)
        return false;
    expectedSize = offset + storageCount * BATTLE_STORAGE_RECORD_SIZE + itemCount * BATTLE_ITEM_RECORD_SIZE;
    if (expectedSize != sCoop.battleReceiveSize)
        return false;
    for (uint16_t slot = 0; slot < TOTAL_BOXES_COUNT * IN_BOX_COUNT; slot++)
    {
        uint8_t box = slot / IN_BOX_COUNT, position = slot % IN_BOX_COUNT;
        occupied[slot] = (box == CTR_COOP_STORAGE_BOX_ID && position < CTR_COOP_STORAGE_ROW_SIZE * 2)
                     || GetBoxMonDataAt(box, position, MON_DATA_SANITY_HAS_SPECIES) != 0;
    }
    /* Reserve every direct destination before planning relocations, so an
     * earlier collision can never consume a later catch's empty target. */
    for (uint16_t i = 0; i < storageCount; i++)
    {
        uint32_t record = offset + i * BATTLE_STORAGE_RECORD_SIZE;
        uint8_t box = sBattleReceiveData[record], position = sBattleReceiveData[record + 1];
        struct BoxPokemon incoming;
        uint16_t slot = (uint16_t)box * IN_BOX_COUNT + position;

        memcpy(&incoming, sBattleReceiveData + record + 2, sizeof(incoming));

        if (box >= TOTAL_BOXES_COUNT || position >= IN_BOX_COUNT
         || (box == CTR_COOP_STORAGE_BOX_ID && position < CTR_COOP_STORAGE_ROW_SIZE * 2)
         || mentioned[slot])
            return false;
        mentioned[slot] = true;
        destinations[i] = UINT16_MAX;
        if (!GetBoxMonData(&incoming, MON_DATA_SANITY_HAS_SPECIES))
            continue;
        if (!occupied[slot] || memcmp(&incoming, &gPokemonStoragePtr->boxes[box][position], sizeof(incoming)) == 0)
        {
            destinations[i] = slot;
            occupied[slot] = true;
        }
    }
    for (uint16_t i = 0; i < storageCount; i++)
    {
        uint32_t record = offset + i * BATTLE_STORAGE_RECORD_SIZE;
        struct BoxPokemon incoming;
        uint16_t slot;

        memcpy(&incoming, sBattleReceiveData + record + 2, sizeof(incoming));

        if (destinations[i] != UINT16_MAX || !GetBoxMonData(&incoming, MON_DATA_SANITY_HAS_SPECIES))
            continue;
        for (slot = 0; slot < TOTAL_BOXES_COUNT * IN_BOX_COUNT && occupied[slot]; slot++) {}
        if (slot == TOTAL_BOXES_COUNT * IN_BOX_COUNT)
            return false;
        destinations[i] = slot;
        occupied[slot] = true;
    }
    /* FinishBattleReceive runs outside a local battle, so its old baseline
     * can safely back up inventory for an all-or-nothing application. */
    memcpy(&sBattleSaveBaseline, gSaveBlock1Ptr, sizeof(sBattleSaveBaseline));
    offset += storageCount * BATTLE_STORAGE_RECORD_SIZE;
    if (ReadBattle16(sBattleReceiveData, 16) == 0)
    {
        for (uint16_t i = 0; i < itemCount; i++)
        {
            uint16_t itemId = ReadBattle16(sBattleReceiveData, offset);
            int16_t delta = (int16_t)ReadBattle16(sBattleReceiveData, offset + 2);

            if ((delta > 0 && !AddBagItem(itemId, delta))
             || (delta < 0 && !RemoveBagItem(itemId, -(int32_t)delta)))
            {
                memcpy(gSaveBlock1Ptr, &sBattleSaveBaseline, sizeof(sBattleSaveBaseline));
                return false;
            }
            offset += BATTLE_ITEM_RECORD_SIZE;
        }
        if ((int32_t)ReadBattle32(sBattleReceiveData, 4) >= 0)
            AddMoney(&gSaveBlock1Ptr->money, ReadBattle32(sBattleReceiveData, 4));
        else
            RemoveMoney(&gSaveBlock1Ptr->money, -(int32_t)ReadBattle32(sBattleReceiveData, 4));
        {
            int32_t coins = (int32_t)(gSaveBlock1Ptr->coins ^ (uint16_t)gSaveBlock2Ptr->encryptionKey)
                + (int16_t)ReadBattle16(sBattleReceiveData, 8);

            if (coins < 0)
                coins = 0;
            else if (coins > MAX_COINS)
                coins = MAX_COINS;
            gSaveBlock1Ptr->coins = (uint16_t)coins ^ (uint16_t)gSaveBlock2Ptr->encryptionKey;
        }
    }
    for (uint16_t i = 0; i < storageCount; i++)
    {
        uint16_t slot = destinations[i];
        struct BoxPokemon incoming;
        memcpy(&incoming, sBattleReceiveData + BATTLE_HEADER_SIZE + sizeof(struct Pokemon) * PARTY_SIZE
            + i * BATTLE_STORAGE_RECORD_SIZE + 2, sizeof(incoming));
        if (slot != UINT16_MAX)
            SetBoxMonAt(slot / IN_BOX_COUNT, slot % IN_BOX_COUNT, &incoming);
    }
    memcpy(sRemoteParty, remoteParty, sizeof(sRemoteParty));
    sRemotePartyValid = true;
    StorePartyRow(remoteRow, remoteParty);
    return true;
}
static void HandleBattleChunk(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint16_t got, expected;

    if (sCoop.battleReceiveState != BATTLE_SYNC_RECEIVING || sCoop.battleReceiveOffset >= sCoop.battleReceiveSize)
        return;
    if (!CtrCoopProtocol_DecodeBattleChunk(data, len, sCoop.battleReceiveOffset,
            sBattleReceiveData + sCoop.battleReceiveOffset,
            sCoop.battleReceiveSize - sCoop.battleReceiveOffset, &got))
        return;
    expected = sCoop.battleReceiveSize - sCoop.battleReceiveOffset;
    if (expected > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        expected = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (got != expected)
        return;
    sCoop.battleReceiveOffset += got;
    if (sCoop.battleReceiveOffset < sCoop.battleReceiveSize)
    {
        SendBattleRequest(from, sCoop.battleReceiveOffset);
        return;
    }
    if (CtrCoopProtocol_Checksum(sBattleReceiveData, sCoop.battleReceiveSize) != sCoop.battleReceiveChecksum)
    {
        CtrLog_Write(CTR_LOG_ERROR, "coop: battle result checksum mismatch, retrying");
        sCoop.battleReceiveOffset = 0;
        SendBattleRequest(from, 0);
        return;
    }
    sCoop.battleReceiveState = BATTLE_SYNC_READY;
    CtrCoop_BattleTick();
}

static void FinishBattleReceive(uint16_t from)
{
    uint16_t sequence = ReadBattle16(sBattleReceiveData, 14);
    bool duplicate = sCoop.battleHaveApplied && sequence == sCoop.battleAppliedSequence;

    sCoop.battleReceiveState = BATTLE_SYNC_IDLE;
    if (duplicate && sCoop.battleReceiveChecksum != sCoop.battleAppliedChecksum)
        return;
    if (sCoop.battleHaveApplied && !duplicate
     && (uint16_t)(sequence - sCoop.battleAppliedSequence) >= 0x8000)
        return;
    if (duplicate || ApplyBattleResult())
    {
        uint8_t ready[CTR_COOP_SAVE_READY_SIZE] = {CTR_COOP_BATTLE_READY_PACKET, CTR_COOP_PROTOCOL_VERSION};

        sCoop.battleHaveApplied = true;
        sCoop.battleAppliedSequence = sequence;
        sCoop.battleAppliedChecksum = sCoop.battleReceiveChecksum;
        LocalLink_Send(from, ready, sizeof(ready));
        CtrLog_Write(CTR_LOG_GAME, "coop: battle result %s", duplicate ? "already applied" : "applied");
    }
}

static void SendPcSyncRequest(uint16_t to, uint32_t offset)
{
    uint8_t packet[CTR_COOP_SAVE_REQUEST_SIZE];

    if (CtrCoopProtocol_EncodePcSyncRequest(offset, packet, sizeof(packet)))
        LocalLink_Send(to, packet, sizeof(packet));
}

void CtrCoop_CommitPC(void)
{
    struct LocalPeer peers[1];
    uint8_t packet[CTR_COOP_SAVE_BEGIN_SIZE];
    uint32_t offset = 0;
    uint8_t row = sCoop.guest ? CTR_COOP_STORAGE_GUEST_ROW : CTR_COOP_STORAGE_HOST_ROW;

    if (sCoop.syncState != PC_SYNC_IDLE)
        return;
    if (CoopStorageReady())
        StorePartyRow(row, gPlayerParty);
    /* Commit the local host operation even if its guest left the PC visit.
     * The next join receives this complete state in its initial snapshot. */
    if (!sCoop.wanted || LocalLink_GetPeers(peers, 1) == 0
     || (!sCoop.guest && !sCoop.peerSaveReady))
    {
        CtrCoop_ReleasePC();
        return;
    }
    if (gPokemonStoragePtr == NULL)
        return;
    memcpy(sSaveData + offset, gPokemonStoragePtr, sizeof(*gPokemonStoragePtr));
    offset += sizeof(*gPokemonStoragePtr);
    memcpy(sSaveData + offset, gPlayerParty, sizeof(gPlayerParty));
    sCoop.syncOffset = 0;
    sCoop.syncChecksum = CtrCoopProtocol_Checksum(sSaveData, CTR_COOP_PC_SYNC_SIZE);
    sCoop.syncState = PC_SYNC_SENDING;
    if (sCoop.guest && sCoop.initialTeamSelection)
        sCoop.initialTeamCommitted = true;
    if (CtrCoopProtocol_EncodePcSyncBegin(CTR_COOP_PC_SYNC_SIZE, sCoop.syncChecksum, packet, sizeof(packet)))
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
    CtrLog_Write(CTR_LOG_GAME, "coop: PC commit begin %lu bytes", (unsigned long)CTR_COOP_PC_SYNC_SIZE);
}

static void SendPcSyncChunk(uint16_t to, uint32_t offset)
{
    uint8_t packet[CTR_COOP_SAVE_CHUNK_MAX];
    uint16_t size;

    if (sCoop.syncState != PC_SYNC_SENDING || offset >= CTR_COOP_PC_SYNC_SIZE)
        return;
    size = CTR_COOP_PC_SYNC_SIZE - offset;
    if (size > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        size = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (CtrCoopProtocol_EncodePcSyncChunk(offset, sSaveData + offset, size, packet, CTR_COOP_SAVE_CHUNK_HEADER + size))
        LocalLink_Send(to, packet, CTR_COOP_SAVE_CHUNK_HEADER + size);
}

static void HandlePcSyncBegin(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint32_t totalSize, checksum;

    if (sCoop.syncState != PC_SYNC_IDLE || !CtrCoopProtocol_DecodePcSyncBegin(data, len, &totalSize, &checksum)
     || totalSize != CTR_COOP_PC_SYNC_SIZE)
        return;
    sCoop.syncOffset = 0;
    sCoop.syncChecksum = checksum;
    sCoop.syncState = PC_SYNC_RECEIVING;
    SendPcSyncRequest(from, 0);
}

static void ApplyPcSync(uint16_t from)
{
    uint8_t ready[CTR_COOP_SAVE_READY_SIZE] = {CTR_COOP_PC_SYNC_READY_PACKET, CTR_COOP_PROTOCOL_VERSION};
    uint8_t remoteRow = sCoop.guest ? CTR_COOP_STORAGE_HOST_ROW : CTR_COOP_STORAGE_GUEST_ROW;
    const struct Pokemon *remoteParty = (const struct Pokemon *)(sSaveData + sizeof(struct PokemonStorage));

    memcpy(gPokemonStoragePtr, sSaveData, sizeof(*gPokemonStoragePtr));
    memcpy(sRemoteParty, remoteParty, sizeof(sRemoteParty));
    sRemotePartyValid = true;
    StorePartyRow(remoteRow, remoteParty);
    /* The other player's snapshot can predate changes to our live party. */
    StorePartyRow(sCoop.guest ? CTR_COOP_STORAGE_GUEST_ROW : CTR_COOP_STORAGE_HOST_ROW, gPlayerParty);
    sCoop.havePartyAck = false;
    sCoop.syncState = PC_SYNC_IDLE;
    sCoop.syncOffset = 0;
    LocalLink_Send(from, ready, sizeof(ready));
    CtrLog_Write(CTR_LOG_GAME, "coop: PC commit applied");
    if (!sCoop.guest && !sCoop.peerTeamReady)
    {
        sCoop.peerTeamReady = true;
        SendHostSpawn(from);
    }
}

static void HandlePcSyncChunk(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint16_t got, expected;

    if (sCoop.syncState != PC_SYNC_RECEIVING || sCoop.syncOffset >= CTR_COOP_PC_SYNC_SIZE)
        return;
    if (!CtrCoopProtocol_DecodePcSyncChunk(data, len, sCoop.syncOffset,
            sSaveData + sCoop.syncOffset, CTR_COOP_PC_SYNC_SIZE - sCoop.syncOffset, &got))
        return;
    expected = CTR_COOP_PC_SYNC_SIZE - sCoop.syncOffset;
    if (expected > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        expected = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (got != expected)
        return;
    sCoop.syncOffset += got;
    if (sCoop.syncOffset < CTR_COOP_PC_SYNC_SIZE)
    {
        SendPcSyncRequest(from, sCoop.syncOffset);
        return;
    }
    if (CtrCoopProtocol_Checksum(sSaveData, CTR_COOP_PC_SYNC_SIZE) != sCoop.syncChecksum)
    {
        CtrLog_Write(CTR_LOG_ERROR, "coop: PC commit checksum mismatch, retrying");
        sCoop.syncOffset = 0;
        SendPcSyncRequest(from, 0);
        return;
    }
    ApplyPcSync(from);
}

static bool DenyPC(uint8_t owner)
{
    /* Hold the PC special's waitstate until the named notice is dismissed.
     * Transient synchronization failures still resume on the next frame. */
    if (gMain.callback2 == CB2_Overworld || sCoop.pcScriptWaiting)
    {
        if (owner != PC_OWNER_NONE)
        {
            const uint8_t *name = owner == PC_OWNER_HOST ? sCoop.hostName : sCoop.peerName;
            static const uint8_t host[] = {0xC2, 0xC9, 0xCD, 0xCE, EOS}; /* HOST */
            static const uint8_t guest[] = {0xC1, 0xBB, 0xCD, 0xCE, EOS}; /* GAST */
            bool hasText = false;
            for (unsigned i = 0; i < sizeof(sCoop.pcNoticeName) - 1 && name[i] != EOS; ++i)
                hasText |= name[i] != 0;
            memset(sCoop.pcNoticeName, EOS, sizeof(sCoop.pcNoticeName));
            /* The legacy default network identity is ASCII; display it in
             * game encoding without changing its saved spawn-table key. */
            if (!hasText || (owner == PC_OWNER_GUEST && memcmp(name, sDefaultGuestName, sizeof(sDefaultGuestName)) == 0))
                memcpy(sCoop.pcNoticeName, owner == PC_OWNER_HOST ? host : guest, sizeof(host));
            else
                memcpy(sCoop.pcNoticeName, name, sizeof(sCoop.pcNoticeName) - 1);
            sCoop.pcNotice = true;
            sCoop.pcResumeScript = false;
        }
        else if (!sCoop.pcNotice)
            sCoop.pcResumeScript = true;
    }
    sCoop.pcScriptWaiting = false;
    PlaySE(SE_FAILURE);
    return false;
}

const uint8_t *CtrCoop_PCNoticeName(void)
{
    return sCoop.pcNotice ? sCoop.pcNoticeName : NULL;
}

void CtrCoop_DismissPCNotice(void)
{
    if (!sCoop.pcNotice)
        return;
    sCoop.pcNotice = false;
    sCoop.pcResumeScript = true;
}

bool CtrCoop_RequestPC(void)
{
    struct LocalPeer peers[1];
    uint8_t packet[CTR_COOP_PC_CONTROL_SIZE] = {CTR_COOP_PC_REQUEST_PACKET, CTR_COOP_PROTOCOL_VERSION};
    uint8_t own = sCoop.guest ? PC_OWNER_GUEST : PC_OWNER_HOST;

    if (!sCoop.wanted)
    {
        if (sCoop.guestWorldLoaded)
            return DenyPC(PC_OWNER_NONE);
        sCoop.pcOwner = own;
        sCoop.pcOpen = true;
        return true;
    }
    if (sCoop.syncState != PC_SYNC_IDLE || sCoop.partySending || sCoop.partyReceiving
     || sCoop.battleActive || sCoop.peerBattleActive
     || sCoop.battleSendState != BATTLE_SYNC_IDLE || sCoop.battleReceiveState != BATTLE_SYNC_IDLE)
        return DenyPC(PC_OWNER_NONE);
    if (sCoop.pcOwner == own)
    {
        sCoop.pcScriptWaiting = false;
        sCoop.pcResumeScript = false;
        sCoop.pcOpen = true;
        return true;
    }
    if (sCoop.guest)
    {
        if (sCoop.pcPending)
            return false;
        if (sCoop.pcOwner == PC_OWNER_HOST || !sCoop.authenticated || !sCoop.saveReady
         || LocalLink_GetSession() != LOCAL_SESSION_IN || LocalLink_GetPeers(peers, 1) == 0)
        {
            return DenyPC(sCoop.pcOwner == PC_OWNER_HOST ? PC_OWNER_HOST : PC_OWNER_NONE);
        }
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
        sCoop.pcPending = true;
        sCoop.pcScriptWaiting = gMain.callback2 == CB2_Overworld;
        return false;
    }
    if (sCoop.pcOwner == PC_OWNER_GUEST)
    {
        return DenyPC(PC_OWNER_GUEST);
    }
    if (sCoop.peerAuthenticated && sCoop.savePrepared && !sCoop.peerSaveReady)
    {
        return DenyPC(PC_OWNER_NONE);
    }
    sCoop.pcOwner = PC_OWNER_HOST;
    sCoop.pcResumeScript = false;
    sCoop.pcOpen = true;
    packet[0] = CTR_COOP_PC_CLAIM_PACKET;
    if (LocalLink_GetPeers(peers, 1) != 0)
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
    CtrLog_Write(CTR_LOG_GAME, "coop: PC granted to host");
    return true;
}

static void OpenPCWhenReady(void)
{
    uint8_t own = sCoop.guest ? PC_OWNER_GUEST : PC_OWNER_HOST;

    if (!sCoop.wanted || sCoop.pcPending || sCoop.pcOpen || sCoop.pcOwner != own)
        return;
    sCoop.pcOpen = true;
    if (sCoop.guest && sCoop.initialTeamSelection && !sCoop.initialTeamCommitted
     && gPlayerPartyCount != 0)
    {
        /* A starter choice or saved guest row already supplies the team.
         * Commit it under the host's lock and wait for the assigned spawn. */
        CtrCoop_CommitPC();
        CtrLog_Write(CTR_LOG_GAME, "coop: guest team submitted, waiting for host spawn");
        return;
    }
    CtrLog_Write(CTR_LOG_GAME, "coop: PC opened for %s", sCoop.guest ? "guest" : "host");
    ShowPokemonStorageSystemPC();
}

static void StartSession(bool searchOnly)
{
    if (!searchOnly)
    {
        if (!PrepareCoopStorage())
        {
            PlaySE(SE_FAILURE);
            return;
        }
        StorePartyRow(CTR_COOP_STORAGE_HOST_ROW, gPlayerParty);
    }
    sCoop.wanted = true;
    sCoop.guest = searchOnly;
    sCoop.authenticated = !searchOnly;
    sCoop.authSent = false;
    sCoop.authRejected = false;
    sCoop.nameLimitRejected = false;
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
    sCoop.nameLimitRejected = false;
    sCoop.peerAuthenticated = false;
    sCoop.generation = LocalLink_Generation();
    sCoop.haveLastSent = false;
    sCoop.framesSinceLast = 0;
    ClearRemote();
    LocalLink_JoinSession(LOCAL_KIND_COOP, token, password);
    PlaySE(SE_SUCCESS);
}

static void FinishStop(void)
{
    if (!sCoop.guest && sRemotePartyValid)
        StorePartyRow(CTR_COOP_STORAGE_GUEST_ROW, sRemoteParty);
    CtrCoop_ReleasePC();
    sCoop.wanted = false;
    sCoop.authenticated = false;
    sCoop.authSent = false;
    sCoop.authRejected = false;
    sCoop.nameLimitRejected = false;
    sCoop.peerAuthenticated = false;
    ClearRemote();
    LocalLink_LeaveSession();
}

static void SendLogoutBegin(void)
{
    struct LocalPeer peers[1];
    uint8_t packet[CTR_COOP_SAVE_BEGIN_SIZE];

    if (sCoop.logoutState != LOGOUT_STATE_SENDING || LocalLink_GetPeers(peers, 1) == 0)
        return;
    if (CtrCoopProtocol_EncodeLogoutBegin(sizeof(sLogoutSendData), sCoop.logoutSendChecksum, packet, sizeof(packet)))
        LocalLink_Send(peers[0].id, packet, sizeof(packet));
}

static void SendLogoutRequest(uint16_t to, uint32_t offset)
{
    uint8_t packet[CTR_COOP_SAVE_REQUEST_SIZE];

    if (CtrCoopProtocol_EncodeLogoutRequest(offset, packet, sizeof(packet)))
        LocalLink_Send(to, packet, sizeof(packet));
}

static void SendLogoutChunk(uint16_t to, uint32_t offset)
{
    uint8_t packet[CTR_COOP_SAVE_CHUNK_MAX];
    uint16_t size;

    if (sCoop.logoutState != LOGOUT_STATE_SENDING || offset >= sizeof(sLogoutSendData))
        return;
    size = sizeof(sLogoutSendData) - offset;
    if (size > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        size = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (CtrCoopProtocol_EncodeLogoutChunk(offset, sLogoutSendData + offset, size, packet, CTR_COOP_SAVE_CHUNK_HEADER + size))
        LocalLink_Send(to, packet, CTR_COOP_SAVE_CHUNK_HEADER + size);
}

static void HandleLogoutBegin(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint32_t totalSize, checksum;

    if (sCoop.guest || !sCoop.peerAuthenticated || !sCoop.peerSaveReady
     || sCoop.logoutState != LOGOUT_STATE_IDLE
     || !CtrCoopProtocol_DecodeLogoutBegin(data, len, &totalSize, &checksum)
     || totalSize != sizeof(sLogoutReceiveData))
        return;
    sCoop.logoutReceiveOffset = 0;
    sCoop.logoutReceiveChecksum = checksum;
    sCoop.logoutState = LOGOUT_STATE_RECEIVING;
    sCoop.logoutRetry = 0;
    SendLogoutRequest(from, 0);
}

static void HandleLogoutChunk(uint16_t from, const uint8_t *data, uint16_t len)
{
    uint16_t got, expected;
    uint8_t ready[CTR_COOP_SAVE_READY_SIZE] = {CTR_COOP_LOGOUT_READY_PACKET, CTR_COOP_PROTOCOL_VERSION};

    if (sCoop.guest || sCoop.logoutState != LOGOUT_STATE_RECEIVING
     || sCoop.logoutReceiveOffset >= sizeof(sLogoutReceiveData))
        return;
    if (!CtrCoopProtocol_DecodeLogoutChunk(data, len, sCoop.logoutReceiveOffset,
            sLogoutReceiveData + sCoop.logoutReceiveOffset,
            sizeof(sLogoutReceiveData) - sCoop.logoutReceiveOffset, &got))
        return;
    expected = sizeof(sLogoutReceiveData) - sCoop.logoutReceiveOffset;
    if (expected > CTR_COOP_SAVE_CHUNK_DATA_MAX)
        expected = CTR_COOP_SAVE_CHUNK_DATA_MAX;
    if (got != expected)
        return;
    sCoop.logoutReceiveOffset += got;
    if (sCoop.logoutReceiveOffset < sizeof(sLogoutReceiveData))
    {
        SendLogoutRequest(from, sCoop.logoutReceiveOffset);
        return;
    }
    if (CtrCoopProtocol_Checksum(sLogoutReceiveData, sizeof(sLogoutReceiveData)) != sCoop.logoutReceiveChecksum)
    {
        sCoop.logoutReceiveOffset = 0;
        SendLogoutRequest(from, 0);
        return;
    }
    memcpy(sRemoteParty, sLogoutReceiveData, sizeof(sRemoteParty));
    sRemotePartyValid = true;
    StorePartyRow(CTR_COOP_STORAGE_GUEST_ROW, sRemoteParty);
    sCoop.peerTeamReady = false;
    sCoop.peerLoggedOut = true;
    sCoop.logoutState = LOGOUT_STATE_IDLE;
    LocalLink_Send(from, ready, sizeof(ready));
    CtrLog_Write(CTR_LOG_GAME, "coop: guest party returned before logout");
}

static void TickLogout(void)
{
    if (sCoop.logoutState == LOGOUT_STATE_IDLE)
        return;
    sCoop.logoutRetry++;
    if (sCoop.logoutState == LOGOUT_STATE_SENDING && sCoop.logoutRetry % 120 == 0)
        SendLogoutBegin();
    if (sCoop.logoutRetry >= 600)
    {
        if (sCoop.guest && sCoop.logoutState == LOGOUT_STATE_SENDING)
            FinishStop();
        else
            sCoop.logoutState = LOGOUT_STATE_IDLE;
    }
}

void CtrCoop_Stop(void)
{
    struct LocalPeer peers[1];

    if (sCoop.guest && sCoop.wanted && sCoop.authenticated && sCoop.saveReady
     && sCoop.initialTeamCommitted && sCoop.spawnValid
     && sCoop.logoutState == LOGOUT_STATE_IDLE && LocalLink_GetPeers(peers, 1) != 0)
    {
        memcpy(sLogoutSendData, gPlayerParty, sizeof(sLogoutSendData));
        sCoop.logoutSendOffset = 0;
        sCoop.logoutSendChecksum = CtrCoopProtocol_Checksum(sLogoutSendData, sizeof(sLogoutSendData));
        sCoop.logoutState = LOGOUT_STATE_SENDING;
        sCoop.logoutRetry = 0;
        SendLogoutBegin();
        return;
    }
    if (sCoop.logoutState == LOGOUT_STATE_SENDING)
        return;
    FinishStop();
}

bool CtrCoop_GuestSaveBlocked(void)
{
    return sCoop.guestWorldLoaded || (sCoop.wanted && sCoop.guest);
}

uint8_t CtrCoop_GetSessionState(void)
{
    uint8_t state;

    if (sCoop.nameLimitRejected)
        return CTR_COOP_SESSION_NAME_LIMIT;
    if (!sCoop.wanted)
        return LOCAL_SESSION_NONE;
    if (sCoop.authRejected)
        return LOCAL_SESSION_AUTH_FAILED;
    state = LocalLink_GetSession();
    if (sCoop.guest && sCoop.logoutState != LOGOUT_STATE_IDLE)
        return LOCAL_SESSION_SEARCHING;
    if (sCoop.guest && state == LOCAL_SESSION_IN
     && (!sCoop.authenticated || !sCoop.saveReady || !sCoop.joinInfoReceived
      || !sCoop.initialTeamCommitted || (sCoop.starterRequired && !sCoop.starterChosen)
      || sCoop.syncState != PC_SYNC_IDLE || !sCoop.spawnValid))
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

    TickLogout();
    if (!sCoop.guest || !sCoop.wanted || sCoop.authRejected)
        return;
    if (sCoop.authenticated)
    {
        if (!sCoop.saveReady || !sCoop.joinInfoReceived)
            return;
        if (sCoop.starterRequired && !sCoop.starterChosen)
        {
            if (!CB2_StarterSelectionActive())
                SetMainCallback2(CB2_InitStarterSelection);
            return;
        }
        if (sCoop.initialTeamSelection && !sCoop.initialTeamCommitted
         && sCoop.pcOwner == PC_OWNER_NONE && !sCoop.pcPending)
            CtrCoop_RequestPC();
        OpenPCWhenReady();
        return;
    }
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

static void SendFlagUpdate(uint16_t to, uint16_t id, bool value)
{
    uint8_t packet[CTR_COOP_FLAG_SIZE];

    if (CtrCoopProtocol_EncodeFlag(id, value, packet, sizeof(packet)))
        LocalLink_Send(to, packet, sizeof(packet));
}

static void SendVarUpdate(uint16_t to, uint16_t id, uint16_t value)
{
    uint8_t packet[CTR_COOP_VAR_SIZE];

    if (CtrCoopProtocol_EncodeVar(id, value, packet, sizeof(packet)))
        LocalLink_Send(to, packet, sizeof(packet));
}

static void SyncStoryState(void)
{
    struct LocalPeer peers[1];
    uint8_t sent = 0;

    if (!sCoop.storyInitialized)
    {
        ResetStoryBaseline();
        return;
    }
    if (++sCoop.storyFrame < 15)
        return;
    sCoop.storyFrame = 0;
    if (gSaveBlock1Ptr == NULL || LocalLink_GetSession() != LOCAL_SESSION_IN
     || LocalLink_GetPeers(peers, 1) == 0)
        return;
    for (uint16_t id = 1; id < FLAGS_COUNT && sent < 8; id++)
    {
        uint8_t mask = 1 << (id & 7);
        bool value = FlagGet(id);

        if (((sCoop.storyFlagsBaseline[id / 8] & mask) != 0) == value)
            continue;
        if (value)
            sCoop.storyFlagsBaseline[id / 8] |= mask;
        else
            sCoop.storyFlagsBaseline[id / 8] &= ~mask;
        SendFlagUpdate(peers[0].id, id, value);
        sent++;
    }
    for (uint16_t id = VARS_START; id < VARS_START + VARS_COUNT && sent < 8; id++)
    {
        uint16_t index = id - VARS_START;
        uint16_t value = gSaveBlock1Ptr->vars[index];

        if (sCoop.storyVarsBaseline[index] == value)
            continue;
        sCoop.storyVarsBaseline[index] = value;
        SendVarUpdate(peers[0].id, id, value);
        sent++;
    }
}

static void ApplyRemoteFlag(uint16_t id, bool value)
{
    uint8_t mask;

    if (id == 0 || id >= FLAGS_COUNT || !sCoop.storyInitialized)
        return;
    mask = 1 << (id & 7);
    if (value)
    {
        gSaveBlock1Ptr->flags[id / 8] |= mask;
        sCoop.storyFlagsBaseline[id / 8] |= mask;
    }
    else
    {
        gSaveBlock1Ptr->flags[id / 8] &= ~mask;
        sCoop.storyFlagsBaseline[id / 8] &= ~mask;
    }
}

static void ApplyRemoteVar(uint16_t id, uint16_t value)
{
    uint16_t index;

    if (id < VARS_START || id >= VARS_START + VARS_COUNT || !sCoop.storyInitialized)
        return;
    index = id - VARS_START;
    gSaveBlock1Ptr->vars[index] = value;
    sCoop.storyVarsBaseline[index] = value;
}

void CtrCoop_OverworldFrame(void)
{
    struct LocalPeer peers[1];
    CtrCoopSnapshot local;
    uint32_t generation;
    uint8_t state;
    int peerCount;

    if (sCoop.pcResumeScript && gMain.callback2 == CB2_Overworld)
    {
        sCoop.pcResumeScript = false;
        ScriptContext_Enable();
    }
    if (!sCoop.wanted && !sCoop.remoteValid && sCoop.objectEventId == OBJECT_EVENTS_COUNT)
    {
        SyncParty();
        if (gMain.callback2 == CB2_Overworld && gSaveBlock1Ptr != NULL)
            RemoveRemoteObject();
        return;
    }
    CtrCoop_BattleTick();
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
    SyncParty();
    if (!sCoop.guest && sCoop.wanted && sCoop.peerAuthenticated && sCoop.peerIdentified
     && !sCoop.savePrepared && !sCoop.battleActive && sCoop.pcOwner == PC_OWNER_NONE && sCoop.syncState == PC_SYNC_IDLE)
        PrepareHostSave(sCoop.peerId);
    if (!sCoop.remoteValid && gMain.callback2 == CB2_Overworld)
        RemoveRemoteObject();
    if ((sCoop.guest && (!sCoop.authenticated || !sCoop.saveReady || !sCoop.joinInfoReceived
      || (sCoop.starterRequired && !sCoop.starterChosen)))
     || (!sCoop.guest && (!sCoop.peerAuthenticated || !sCoop.peerSaveReady)))
        return;
    OpenPCWhenReady();
    SyncStoryState();
    /* Keep transport and story state running, but only mutate field sprites
     * after the engine has rebuilt them on returning from a menu or battle. */
    if (gMain.callback2 != CB2_Overworld)
        return;
    ReleaseTrainerReservation();
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
    if (!sCoop.guest)
        SyncHostNpc(&peers[0]);
}

bool CtrCoop_HandlePacket(uint16_t from, const uint8_t *data, uint16_t len)
{
    struct LocalPeer peers[1];
    CtrCoopSnapshot remote;
    uint8_t password[4];
    uint8_t result[CTR_COOP_AUTH_RESULT_SIZE];
    bool accepted;

    if (data == NULL || len == 0)
        return false;
    if (!sCoop.wanted || LocalLink_GetSessionKind() != LOCAL_KIND_COOP)
        return false;
    if (data[0] < CTR_COOP_PEER_PACKET || data[0] > CTR_COOP_LOGOUT_READY_PACKET)
        return false;
    if (LocalLink_GetSession() != LOCAL_SESSION_IN || LocalLink_GetPeers(peers, 1) == 0 || peers[0].id != from)
        return true;
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
        if (!sCoop.guest || !sCoop.authSent || sCoop.authenticated
         || !CtrCoopProtocol_DecodeAuthResult(data, len, &accepted))
            return true;
        if (accepted)
        {
            sCoop.peerId = from;
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
    if (from != sCoop.peerId || (sCoop.guest ? !sCoop.authenticated : !sCoop.peerAuthenticated))
        return true;
    if (data[0] == CTR_COOP_PARTY_BEGIN_PACKET)
    {
        HandlePartyBegin(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_PARTY_CHUNK_PACKET)
    {
        HandlePartyChunk(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_PARTY_RESULT_PACKET)
    {
        if (sCoop.partySending && CtrCoopProtocol_DecodePartyResult(data, len, &accepted))
        {
            sCoop.partySending = false;
            if (accepted)
            {
                sCoop.havePartyAck = true;
                sCoop.partyAckChecksum = sCoop.partySendChecksum;
            }
            else
                sCoop.partyRetry = 15;
        }
        return true;
    }
    if (data[0] == CTR_COOP_IDENTITY_PACKET)
    {
        if (sCoop.guest || !sCoop.peerAuthenticated)
            return true;
        if (CtrCoopProtocol_DecodeIdentity(data, len, sCoop.peerName))
        {
            if (!HasRoomForGuestName(sCoop.peerName))
            {
                uint8_t packet[CTR_COOP_NAME_LIMIT_SIZE];

                if (CtrCoopProtocol_EncodeNameLimit(packet, sizeof(packet)))
                    LocalLink_Send(from, packet, sizeof(packet));
                CtrLog_Write(CTR_LOG_GAME, "coop: guest name limit reached (%u)", SPAWN_RECORD_COUNT);
                ClearRemote();
                return true;
            }
            sCoop.peerIdentified = true;
            if (!sCoop.battleActive && sCoop.pcOwner == PC_OWNER_NONE && sCoop.syncState == PC_SYNC_IDLE)
                PrepareHostSave(from);
        }
        return true;
    }
    if (data[0] == CTR_COOP_NAME_LIMIT_PACKET)
    {
        if (sCoop.guest && CtrCoopProtocol_DecodeNameLimit(data, len))
        {
            sCoop.nameLimitRejected = true;
            sCoop.authenticated = false;
            ClearRemote();
            LocalLink_LeaveSession();
        }
        return true;
    }
    if (data[0] == CTR_COOP_JOIN_INFO_PACKET)
    {
        bool starterRequired;

        if (sCoop.guest && sCoop.authenticated && !sCoop.starterChosen
         && CtrCoopProtocol_DecodeJoinInfo(data, len, &starterRequired))
        {
            sCoop.starterRequired = starterRequired;
            sCoop.joinInfoReceived = true;
        }
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
    if (data[0] == CTR_COOP_NPC_PACKET)
    {
        CtrCoopNpcSnapshot npc;

        if (sCoop.guest && sCoop.authenticated && sCoop.saveReady
         && CtrCoopProtocol_DecodeNpc(data, len, &npc))
            ApplyHostNpc(&npc);
        return true;
    }
    if (data[0] == CTR_COOP_PC_REQUEST_PACKET)
    {
        if (sCoop.guest || len != CTR_COOP_PC_CONTROL_SIZE || data[1] != CTR_COOP_PROTOCOL_VERSION)
            return true;
        accepted = sCoop.peerAuthenticated && sCoop.peerSaveReady && sCoop.pcOwner != PC_OWNER_HOST
                && sCoop.syncState == PC_SYNC_IDLE && !sCoop.partySending && !sCoop.partyReceiving
                && sCoop.battleSendState == BATTLE_SYNC_IDLE && sCoop.battleReceiveState == BATTLE_SYNC_IDLE
                && !sCoop.battleActive && !sCoop.peerBattleActive;
        if (accepted)
            sCoop.pcOwner = PC_OWNER_GUEST;
        if (!accepted && sCoop.pcOwner == PC_OWNER_HOST)
        {
            uint8_t claim[CTR_COOP_PC_CONTROL_SIZE] = {CTR_COOP_PC_CLAIM_PACKET, CTR_COOP_PROTOCOL_VERSION};

            LocalLink_Send(from, claim, sizeof(claim));
        }
        if (CtrCoopProtocol_EncodePcResult(accepted, result, sizeof(result)))
            LocalLink_Send(from, result, CTR_COOP_PC_RESULT_SIZE);
        CtrLog_Write(CTR_LOG_GAME, "coop: PC request %s", accepted ? "granted" : "rejected");
        return true;
    }
    if (data[0] == CTR_COOP_PC_RESULT_PACKET)
    {
        if (!sCoop.guest || !sCoop.pcPending || !CtrCoopProtocol_DecodePcResult(data, len, &accepted))
            return true;
        sCoop.pcPending = false;
        if (accepted)
            sCoop.pcOwner = PC_OWNER_GUEST;
        else
        {
            sCoop.pcOwner = PC_OWNER_NONE;
            DenyPC(PC_OWNER_NONE);
        }
        return true;
    }
    if (data[0] == CTR_COOP_PC_CLAIM_PACKET)
    {
        if (sCoop.guest && len == CTR_COOP_PC_CONTROL_SIZE && data[1] == CTR_COOP_PROTOCOL_VERSION
         && sCoop.pcOwner != PC_OWNER_GUEST && sCoop.syncState == PC_SYNC_IDLE)
        {
            if (sCoop.pcPending || sCoop.pcResumeScript)
                DenyPC(PC_OWNER_HOST);
            sCoop.pcOwner = PC_OWNER_HOST;
            sCoop.pcPending = false;
            sCoop.pcOpen = false;
        }
        return true;
    }
    if (data[0] == CTR_COOP_PC_RELEASE_PACKET)
    {
        uint8_t remoteOwner = sCoop.guest ? PC_OWNER_HOST : PC_OWNER_GUEST;

        if (len == CTR_COOP_PC_CONTROL_SIZE && data[1] == CTR_COOP_PROTOCOL_VERSION
         && sCoop.pcOwner == remoteOwner && sCoop.syncState == PC_SYNC_IDLE)
        {
            sCoop.pcOwner = PC_OWNER_NONE;
            sCoop.pcPending = false;
            sCoop.pcOpen = false;
            CtrLog_Write(CTR_LOG_GAME, "coop: PC released");
        }
        return true;
    }
    if (data[0] == CTR_COOP_PC_SYNC_BEGIN_PACKET)
    {
        uint8_t remoteOwner = sCoop.guest ? PC_OWNER_HOST : PC_OWNER_GUEST;

        if (sCoop.pcOwner == remoteOwner)
            HandlePcSyncBegin(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_PC_SYNC_REQUEST_PACKET)
    {
        uint32_t offset;

        if (sCoop.syncState == PC_SYNC_SENDING && CtrCoopProtocol_DecodePcSyncRequest(data, len, &offset))
            SendPcSyncChunk(from, offset);
        return true;
    }
    if (data[0] == CTR_COOP_PC_SYNC_CHUNK_PACKET)
    {
        HandlePcSyncChunk(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_PC_SYNC_READY_PACKET)
    {
        if (sCoop.syncState == PC_SYNC_SENDING && len == CTR_COOP_SAVE_READY_SIZE
         && data[1] == CTR_COOP_PROTOCOL_VERSION)
        {
            sCoop.syncState = PC_SYNC_IDLE;
            sCoop.syncOffset = 0;
            sCoop.havePartyAck = false;
            CtrCoop_ReleasePC();
            CtrLog_Write(CTR_LOG_GAME, "coop: PC commit confirmed");
        }
        return true;
    }
    if (data[0] == CTR_COOP_BATTLE_RESERVE_PACKET)
    {
        uint16_t trainerId;
        uint8_t reserveResult[CTR_COOP_BATTLE_RESERVE_RESULT_SIZE];

        if (sCoop.guest || !sCoop.peerSaveReady || !CtrCoopProtocol_DecodeBattleReserve(data, len, &trainerId))
            return true;
        accepted = (!sCoop.peerBattleActive || sCoop.peerTrainerId == trainerId)
                && (trainerId == 0 || !sCoop.battleActive || sCoop.battleTrainerId != trainerId)
                && (trainerId == 0 || !sCoop.battleReleasePending || sCoop.battleTrainerId != trainerId)
                && sCoop.pcOwner == PC_OWNER_NONE && !sCoop.pcPending && sCoop.syncState == PC_SYNC_IDLE;
        if (accepted)
        {
            sCoop.peerTrainerId = trainerId;
            sCoop.peerBattleActive = true;
        }
        if (CtrCoopProtocol_EncodeBattleReserveResult(trainerId, accepted, reserveResult, sizeof(reserveResult)))
            LocalLink_Send(from, reserveResult, sizeof(reserveResult));
        CtrLog_Write(CTR_LOG_GAME, "coop: trainer %u reservation %s", trainerId, accepted ? "granted" : "rejected");
        return true;
    }
    if (data[0] == CTR_COOP_BATTLE_RESERVE_RESULT_PACKET)
    {
        uint16_t trainerId;

        if (!sCoop.guest || !sCoop.battleReservePending
         || !CtrCoopProtocol_DecodeBattleReserveResult(data, len, &trainerId, &accepted)
         || trainerId != sCoop.battleTrainerId)
            return true;
        sCoop.battleReservePending = false;
        sCoop.battleReserved = accepted;
        sCoop.battleRetry = accepted ? 0 : 60;
        return true;
    }
    if (data[0] == CTR_COOP_BATTLE_RELEASE_PACKET)
    {
        uint16_t trainerId;

        if (!sCoop.guest && len == CTR_COOP_BATTLE_RESERVE_SIZE && data[1] == CTR_COOP_PROTOCOL_VERSION)
        {
            trainerId = data[2] | ((uint16_t)data[3] << 8);
            if (sCoop.peerTrainerId == trainerId)
            {
                sCoop.peerTrainerId = 0;
                sCoop.peerBattleActive = false;
            }
        }
        return true;
    }
    if (data[0] == CTR_COOP_BATTLE_BEGIN_PACKET)
    {
        if ((sCoop.guest && sCoop.saveReady) || (!sCoop.guest && sCoop.peerSaveReady))
            HandleBattleBegin(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_BATTLE_REQUEST_PACKET)
    {
        uint32_t offset;

        if (sCoop.battleSendState == BATTLE_SYNC_SENDING
         && CtrCoopProtocol_DecodeBattleRequest(data, len, &offset))
            SendBattleChunk(from, offset);
        return true;
    }
    if (data[0] == CTR_COOP_BATTLE_CHUNK_PACKET)
    {
        HandleBattleChunk(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_BATTLE_READY_PACKET)
    {
        if (sCoop.battleSendState == BATTLE_SYNC_SENDING && len == CTR_COOP_SAVE_READY_SIZE
         && data[1] == CTR_COOP_PROTOCOL_VERSION)
        {
            sCoop.battleSendState = BATTLE_SYNC_IDLE;
            sCoop.battleSendOffset = 0;
            sCoop.battleRetry = 0;
            CtrLog_Write(CTR_LOG_GAME, "coop: battle result confirmed");
        }
        return true;
    }
    if (data[0] == CTR_COOP_DOUBLE_REQUEST_PACKET)
    {
        if (len == CTR_COOP_DOUBLE_REQUEST_SIZE && data[1] == CTR_COOP_PROTOCOL_VERSION)
            HandleDoubleRequest(from);
        return true;
    }
    if (data[0] == CTR_COOP_DOUBLE_ACCEPT_PACKET)
    {
        if (sCoop.doubleState == DOUBLE_STATE_WAIT_ACCEPT && len == CTR_COOP_DOUBLE_CONTROL_SIZE
         && data[1] == CTR_COOP_PROTOCOL_VERSION)
            PrepareDoubleSetup();
        return true;
    }
    if (data[0] == CTR_COOP_DOUBLE_DECLINE_PACKET)
    {
        if (sCoop.doubleState == DOUBLE_STATE_WAIT_ACCEPT && len == CTR_COOP_DOUBLE_CONTROL_SIZE
         && data[1] == CTR_COOP_PROTOCOL_VERSION)
        {
            sCoop.doubleState = DOUBLE_STATE_IDLE;
            sCoop.doubleRetry = 120;
        }
        return true;
    }
    if (data[0] == CTR_COOP_DOUBLE_BEGIN_PACKET)
    {
        HandleDoubleBegin(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_DOUBLE_REQUEST_DATA_PACKET)
    {
        uint32_t offset;

        if (sCoop.doubleState == DOUBLE_STATE_SENDING
         && CtrCoopProtocol_DecodeDoubleRequestData(data, len, &offset))
            SendDoubleChunk(from, offset);
        return true;
    }
    if (data[0] == CTR_COOP_DOUBLE_CHUNK_PACKET)
    {
        HandleDoubleChunk(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_DOUBLE_READY_PACKET)
    {
        if (sCoop.doubleState == DOUBLE_STATE_SENDING && len == CTR_COOP_DOUBLE_CONTROL_SIZE
         && data[1] == CTR_COOP_PROTOCOL_VERSION)
            ConfigureDoubleBattle(sDoubleSendData);
        return true;
    }
    if (data[0] == CTR_COOP_LOGOUT_BEGIN_PACKET)
    {
        HandleLogoutBegin(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_LOGOUT_REQUEST_PACKET)
    {
        uint32_t offset;

        if (sCoop.logoutState == LOGOUT_STATE_SENDING
         && CtrCoopProtocol_DecodeLogoutRequest(data, len, &offset))
            SendLogoutChunk(from, offset);
        return true;
    }
    if (data[0] == CTR_COOP_LOGOUT_CHUNK_PACKET)
    {
        HandleLogoutChunk(from, data, len);
        return true;
    }
    if (data[0] == CTR_COOP_LOGOUT_READY_PACKET)
    {
        if (sCoop.guest && sCoop.logoutState == LOGOUT_STATE_SENDING
         && len == CTR_COOP_SAVE_READY_SIZE && data[1] == CTR_COOP_PROTOCOL_VERSION)
            FinishStop();
        return true;
    }
    if (data[0] == CTR_COOP_FLAG_PACKET)
    {
        uint16_t id;
        bool value;

        if (CtrCoopProtocol_DecodeFlag(data, len, &id, &value))
        {
            ApplyRemoteFlag(id, value);
            if (!sCoop.guest)
                SendFlagUpdate(from, id, value);
        }
        return true;
    }
    if (data[0] == CTR_COOP_VAR_PACKET)
    {
        uint16_t id, value;

        if (CtrCoopProtocol_DecodeVar(data, len, &id, &value))
        {
            ApplyRemoteVar(id, value);
            if (!sCoop.guest)
                SendVarUpdate(from, id, value);
        }
        return true;
    }
    if (data[0] != CTR_COOP_PEER_PACKET)
        return false;
    if ((sCoop.guest && (!sCoop.authenticated || !sCoop.saveReady || !sCoop.joinInfoReceived
      || (sCoop.starterRequired && !sCoop.starterChosen)))
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
        bool activeBattle = sCoop.battleActive;
        bool pendingBattle = sCoop.battleReservePending || sCoop.battleReserved
                          || sCoop.doubleState != DOUBLE_STATE_IDLE;

        if (activeBattle && sCoop.doubleState == DOUBLE_STATE_ACTIVE)
        {
            RestoreDoubleParty();
            gBattleTypeFlags = sDoubleOriginalFlags;
            gWirelessCommType = 0;
            gReceivedRemoteLinkPlayers = 0;
        }
        if (activeBattle)
        {
            StorePartyRow(sCoop.guest ? CTR_COOP_STORAGE_GUEST_ROW : CTR_COOP_STORAGE_HOST_ROW, gPlayerParty);
            if (!sCoop.guest)
            {
                gSaveBlock1Ptr->playerPartyCount = gPlayerPartyCount;
                memcpy(gSaveBlock1Ptr->playerParty, gPlayerParty, sizeof(gPlayerParty));
            }
        }
        ClearRemote();
        sCoop.battleActive = false;
        sCoop.peerAuthenticated = false;
        if (sCoop.guest)
            sCoop.authenticated = false;
        sCoop.authSent = false;
        if (activeBattle)
        {
            gBattleOutcome = B_OUTCOME_RAN;
            SetMainCallback2(CB2_InitEndLinkBattle);
        }
        else if (pendingBattle)
        {
            SetMainCallback2(gMain.savedCallback);
        }
        if (restart)
            LocalLink_StartSession(LOCAL_KIND_COOP);
    }
}

void CtrCoop_SoftReset(void)
{
    sCoop.pcOpen = false;
    sCoop.wanted = false;
    sCoop.guest = false;
    sCoop.guestWorldLoaded = false;
    sCoop.authenticated = false;
    sCoop.authSent = false;
    sCoop.authRejected = false;
    sCoop.nameLimitRejected = false;
    sCoop.peerAuthenticated = false;
    sCoop.objectEventId = OBJECT_EVENTS_COUNT;
    sCoop.haveLastSent = false;
    ClearRemote();
    sCoop.pcResumeScript = false;
    CtrCoopBike_Reset();
    if (LocalLink_GetSessionKind() == LOCAL_KIND_COOP)
        LocalLink_LeaveSession();
}
