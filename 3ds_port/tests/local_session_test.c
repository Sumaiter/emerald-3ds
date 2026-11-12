#include <stdio.h>
#include <string.h>
#include "global.h"
#include "main.h"
#include "event_data.h"
#include "task.h"
#include "overworld.h"
#include "constants/local_link.h"
#include "constants/map_groups.h"
#include "local_link.h"

static int sFailures;
static uint8_t sState;
static uint8_t sKind;
static int sLeaves;
static bool8 sLocked;
static bool8 sScript;

#define CHECK(cond, ...) do { if (!(cond)) { sFailures++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

struct Main gMain;
struct SaveBlock1 *gSaveBlock1Ptr;
u16 gSpecialVar_0x800A;
u16 gSpecialVar_Result;
struct Task gTasks[NUM_TASKS];
void CB2_Overworld(void) {}

uint8_t LocalLink_GetSession(void) { return sState; }
uint8_t LocalLink_GetSessionKind(void) { return sKind; }
void LocalLink_StartSession(uint8_t kind) { (void)kind; }
void LocalLink_LeaveSession(void) { sLeaves++; }
bool8 ArePlayerFieldControlsLocked(void) { return sLocked; }
bool8 ScriptContext_IsEnabled(void) { return sScript; }
void ScriptContext_Enable(void) {}
u8 CreateTask(void (*task)(u8), u8 priority) { (void)task; (void)priority; return 0; }
void DestroyTask(u8 taskId) { (void)taskId; }

static void Reset(uint8_t state, uint8_t kind, uint8_t mapGroup, uint8_t mapNum)
{
    static struct SaveBlock1 save;
    memset(&save, 0, sizeof(save));
    save.location.mapGroup = mapGroup;
    save.location.mapNum = mapNum;
    gSaveBlock1Ptr = &save;
    gMain.callback2 = CB2_Overworld;
    sState = state;
    sKind = kind;
    sLeaves = 0;
    sLocked = FALSE;
    sScript = FALSE;
    sState = LOCAL_SESSION_NONE;
    LocalSession_Update();
    sState = state;
}

static void TestCoopOutsideLinkRoom(void)
{
    Reset(LOCAL_SESSION_IN, LOCAL_KIND_COOP, 0, 0);
    for (int i = 0; i < 90; i++)
        LocalSession_Update();
    CHECK(sLeaves == 0, "co-op stays active outside link rooms");
}

static void TestCounterOutsideLinkRoom(void)
{
    Reset(LOCAL_SESSION_IN, LOCAL_KIND_UNION_ROOM, 0, 0);
    for (int i = 0; i < 29; i++)
        LocalSession_Update();
    CHECK(sLeaves == 0, "counter remains before 30 frames");
    LocalSession_Update();
    CHECK(sLeaves == 1, "counter leaves at 30 frames");
}

static void TestCounterInsideLinkRoom(void)
{
    Reset(LOCAL_SESSION_IN, LOCAL_KIND_UNION_ROOM, MAP_GROUP(MAP_UNION_ROOM), MAP_NUM(MAP_UNION_ROOM));
    for (int i = 0; i < 90; i++)
        LocalSession_Update();
    CHECK(sLeaves == 0, "counter stays active inside link room");
}

static void TestNoSession(void)
{
    Reset(LOCAL_SESSION_NONE, LOCAL_KIND_UNION_ROOM, 0, 0);
    for (int i = 0; i < 90; i++)
        LocalSession_Update();
    CHECK(sLeaves == 0, "no active session is left again");
}

static void TestLockedAndScripted(void)
{
    Reset(LOCAL_SESSION_IN, LOCAL_KIND_UNION_ROOM, 0, 0);
    sLocked = TRUE;
    for (int i = 0; i < 90; i++)
        LocalSession_Update();
    CHECK(sLeaves == 0, "locked controls keep the session");
    sLocked = FALSE;
    sScript = TRUE;
    for (int i = 0; i < 90; i++)
        LocalSession_Update();
    CHECK(sLeaves == 0, "active script keeps the session");
}

int main(void)
{
    TestCoopOutsideLinkRoom();
    TestCounterOutsideLinkRoom();
    TestCounterInsideLinkRoom();
    TestNoSession();
    TestLockedAndScripted();
    if (sFailures)
    {
        printf("%d local session tests failed\n", sFailures);
        return 1;
    }
    puts("local session tests passed");
    return 0;
}
