#ifndef CTR_COOP_TEST_GLOBAL_H
#define CTR_COOP_TEST_GLOBAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;

#define TRUE 1
#define FALSE 0
#define ALIGNED(size) __attribute__((aligned(size)))
#define STATIC_ASSERT(condition, name) typedef char static_assert_##name[(condition) ? 1 : -1]

#define PARTY_SIZE 6
#define MULTI_PARTY_SIZE 3
#define TOTAL_BOXES_COUNT 14
#define IN_BOX_COUNT 30
#define BOX_NAME_LENGTH 8
#define EOS 0xFF
#define MAP_OFFSET 7
#define OBJECT_EVENTS_COUNT 16
#define LOCALID_COOP_PLAYER 0xFE
#define NUM_OBJ_EVENT_GFX 239
#define OBJ_KIND_NORMAL 0
#define MOVEMENT_TYPE_FACE_DOWN 0
#define DIR_SOUTH 1
#define MAP_LITTLEROOT_TOWN 0x0009
#define MAP_GROUP(map) ((map) >> 8)
#define MAP_NUM(map) ((map) & 0xFF)
#define LAYOUT_LITTLEROOT_TOWN 10
#define NUM_FLAG_BYTES 256
#define VARS_COUNT 256
#define VARS_START 0x4000
#define SPECIAL_VARS_START 0x4100
#define SPECIAL_FLAGS_START 0x800
#define TRAINER_FLAGS_START 0x100
#define MON_DATA_SANITY_HAS_SPECIES 1
#define PC_ITEMS_COUNT 50
#define BAG_ITEMS_COUNT 30
#define BAG_KEYITEMS_COUNT 30
#define BAG_POKEBALLS_COUNT 16
#define BAG_TMHM_COUNT 64
#define BAG_BERRIES_COUNT 46
#define ITEM_NONE 0
#define MAX_COINS 9999
#define BATTLE_TYPE_DOUBLE 0x0001
#define BATTLE_TYPE_TRAINER 0x0002
#define BATTLE_TYPE_LINK 0x0004
#define BATTLE_TYPE_MULTI 0x0008
#define BATTLE_TYPE_IS_MASTER 0x0010
#define BATTLE_TYPE_LINK_IN_BATTLE 0x0020
#define B_OUTCOME_WON 1
#define B_OUTCOME_RAN 4
#define VERSION_EMERALD 1
#define SE_SUCCESS 1
#define SE_FAILURE 2
#define LOCAL_KIND_COOP 7
#define LOCAL_SESSION_NONE 0
#define LOCAL_SESSION_SEARCHING 1
#define LOCAL_SESSION_IN 2
#define LOCAL_SESSION_AUTH_FAILED 3

struct BoxPokemon
{
    uint16_t species;
    uint8_t data[78];
};

struct Pokemon
{
    struct BoxPokemon box;
    uint8_t data[20];
};

struct ItemSlot
{
    uint16_t itemId;
    uint16_t quantity;
};

struct SaveBlock2
{
    uint8_t playerName[8];
    uint8_t padding[3876];
};

struct SaveBlock1
{
    union
    {
        uint8_t raw[15752];
        struct
        {
            struct Pokemon playerParty[PARTY_SIZE];
            uint8_t playerPartyCount;
            struct
            {
                uint8_t mapGroup;
                uint8_t mapNum;
            } location;
            struct
            {
                int16_t x;
                int16_t y;
            } pos;
            uint32_t money;
            uint16_t coins;
            struct ItemSlot pcItems[PC_ITEMS_COUNT];
            struct ItemSlot bagPocket_Items[BAG_ITEMS_COUNT];
            struct ItemSlot bagPocket_KeyItems[BAG_KEYITEMS_COUNT];
            struct ItemSlot bagPocket_PokeBalls[BAG_POKEBALLS_COUNT];
            struct ItemSlot bagPocket_TMHM[BAG_TMHM_COUNT];
            struct ItemSlot bagPocket_Berries[BAG_BERRIES_COUNT];
            uint8_t flags[NUM_FLAG_BYTES];
            uint16_t vars[VARS_COUNT];
            uint8_t unused_3598[256];
        };
    };
};

struct PokemonStorage
{
    union
    {
        uint8_t raw[33744];
        struct
        {
            struct BoxPokemon boxes[TOTAL_BOXES_COUNT][IN_BOX_COUNT];
            uint8_t boxNames[TOTAL_BOXES_COUNT][BOX_NAME_LENGTH + 1];
            uint8_t currentBox;
            uint8_t wallpapers[TOTAL_BOXES_COUNT];
            uint8_t padding[3];
        };
    };
};

struct Coords16
{
    int16_t x;
    int16_t y;
};

struct ObjectEvent
{
    uint8_t active;
    uint8_t localId;
    uint8_t mapGroup;
    uint8_t mapNum;
    uint8_t graphicsId;
    struct Coords16 currentCoords;
};

struct ObjectEventTemplate
{
    uint8_t localId;
    uint8_t graphicsId;
    uint8_t kind;
    int16_t x;
    int16_t y;
    uint8_t elevation;
    uint8_t movementType;
    const void *script;
    uint16_t flagId;
};

struct PlayerAvatar
{
    uint8_t objectEventId;
};

struct MapHeader
{
    uint16_t mapLayoutId;
};

typedef void (*MainCallback)(void);
struct Main
{
    MainCallback savedCallback;
};

struct LinkPlayer
{
    uint16_t id;
    uint16_t version;
};

extern struct SaveBlock2 *gSaveBlock2Ptr;
extern struct SaveBlock1 *gSaveBlock1Ptr;
extern struct PokemonStorage *gPokemonStoragePtr;
extern struct Pokemon gPlayerParty[PARTY_SIZE];
extern struct Pokemon gEnemyParty[PARTY_SIZE];
extern uint8_t gPlayerPartyCount;
extern struct ObjectEvent gObjectEvents[OBJECT_EVENTS_COUNT];
extern struct PlayerAvatar gPlayerAvatar;
extern struct MapHeader gMapHeader;
extern uint32_t gBattleTypeFlags;
extern uint16_t gTrainerBattleOpponent_A;
extern uint16_t gTrainerBattleOpponent_B;
extern uint8_t gBattleEnvironment;
extern struct Main gMain;
extern uint32_t gRngValue;
extern uint8_t gWirelessCommType;
extern uint8_t gReceivedRemoteLinkPlayers;
extern struct LinkPlayer gLinkPlayers[2];
extern uint8_t gBattleOutcome;

void ZeroMonData(struct Pokemon *mon);
uint32_t GetBoxMonData(const struct BoxPokemon *mon, int attribute);
void BoxMonToMon(const struct BoxPokemon *boxMon, struct Pokemon *mon);
uint8_t CalculatePlayerPartyCount(void);
uint32_t GetBoxMonDataAt(uint8_t box, uint8_t position, int attribute);
s16 GetFirstFreeBoxSpot(uint8_t box);
void SetBoxMonAt(uint8_t box, uint8_t position, const struct BoxPokemon *mon);
void ZeroBoxMonData(struct BoxPokemon *mon);
void ZeroBoxMonAt(uint8_t box, uint8_t position);
void SetBagItemsPointers(void);
void SetDecorationInventoriesPointers(void);
void CopyPartyAndObjectsFromSave(void);
void LoadPlayerBag(void);
uint8_t GetFaceDirectionMovementAction(uint8_t direction);
void SetWarpDestination(uint8_t mapGroup, uint8_t mapNum, int8_t warpId, int8_t x, int8_t y);
void WarpIntoMap(void);
void RemoveObjectEvent(struct ObjectEvent *objectEvent);
uint8_t GetPlayerFacingDirection(void);
uint8_t ObjectEventGetHeldMovementActionId(struct ObjectEvent *objectEvent);
uint8_t TryGetObjectEventIdByLocalIdAndMap(uint8_t localId, uint8_t mapNum, uint8_t mapGroup, uint8_t *objectEventId);
bool FreeObjectEventSlotNearPlayer(void);
uint8_t MapGridGetElevationAt(int16_t x, int16_t y);
uint8_t SpawnSpecialObjectEvent(const struct ObjectEventTemplate *objectEventTemplate);
void ObjectEventTurn(struct ObjectEvent *objectEvent, uint8_t direction);
void ObjectEventClearHeldMovementIfFinished(struct ObjectEvent *objectEvent);
bool ObjectEventIsHeldMovementActive(struct ObjectEvent *objectEvent);
void ObjectEventSetHeldMovement(struct ObjectEvent *objectEvent, uint8_t movementActionId);
void MoveObjectEventToMapCoords(struct ObjectEvent *objectEvent, int16_t x, int16_t y);
void ShowPokemonStorageSystemPC(void);
void PlaySE(uint16_t song);
uint16_t Random(void);
void CB2_InitBattle(void);
void CB2_InitEndLinkBattle(void);
void SetMainCallback2(MainCallback callback);
bool AddBagItem(uint16_t itemId, uint16_t quantity);
bool RemoveBagItem(uint16_t itemId, uint16_t quantity);
uint32_t GetMoney(uint32_t *money);
void AddMoney(uint32_t *money, uint32_t amount);
void RemoveMoney(uint32_t *money, uint32_t amount);
bool FlagGet(uint16_t id);

#endif
