#include <stdio.h>
#include <string.h>
#include "global.h"

#define _(text) text
#define INCBIN_U16(...) {0}
#define INCBIN_U32(...) {0}
#include "starter_menu_engine.inc"

struct Main gMain;
struct Task gTasks[NUM_TASKS];
const u8 gSpeciesNames[NUM_SPECIES][POKEMON_NAME_LENGTH + 1] = {{0}};
u16 gSaveFileStatus;
u16 gPlttBufferUnfaded[PLTT_BUFFER_ALLOC];
u16 gPlttBufferFaded[PLTT_BUFFER_ALLOC];

static int sFailures;
static int sHighlightValue = -1;
static int sChosenSpecies;
static int sChooseCount;
static bool sChooseAccepted = true;
static int sAnimateCount;
static MainCallback sCallback;
static bool sPcOpen;
static int sStopCount;
static int sMainMenuCount;

static void JoinGamePrint(u8 windowId, const u8 *text) { (void)windowId; (void)text; }
static void InitJoinGameMenu(void) {}
static u32 InitMainMenu(bool8 returning) { (void)returning; sMainMenuCount++; return 0; }

#define CHECK(condition, message) do { if (!(condition)) { sFailures++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, message); } } while (0)

void SetGpuReg(u8 reg, u32 value)
{
    if (reg == REG_OFFSET_WIN0V)
        sHighlightValue = value;
}

void PlaySE(u16 song) { (void)song; }
void AnimateSprites(void) { sAnimateCount++; }
void BuildOamBuffer(void) {}
u8 UpdatePaletteFade(void) { return 0; }
struct WindowTemplate CreateWindowTemplate(u8 bg, u8 left, u8 top, u8 width, u8 height, u8 paletteNum, u16 baseBlock)
{
    struct WindowTemplate template = {0};
    (void)bg; (void)left; (void)top; (void)width; (void)height; (void)paletteNum; (void)baseBlock;
    return template;
}
void CreateYesNoMenu(const struct WindowTemplate *window, u16 baseTileNum, u8 paletteNum, u8 initialCursorPos)
{
    (void)window; (void)baseTileNum; (void)paletteNum; (void)initialCursorPos;
}
void SetMainCallback2(MainCallback callback) { sCallback = callback; gMain.callback2 = callback; }
void CB2_InitMultiplayerGuestPassword(void) {}
void CB2_ContinueSavedGame(void) {}
void CtrCoop_MenuFrame(void) {}
void CtrCoop_Stop(void) { sStopCount++; }
bool CtrCoop_PCOpen(void) { return sPcOpen; }
void CtrCoop_Search(void) {}
void CtrCoop_PrepareJoin(uint32_t token) { (void)token; }
void CtrCoop_JoinGame(uint32_t token, const uint8_t *password) { (void)token; (void)password; }
bool CtrCoop_EnterHostWorld(void) { return true; }
void CtrCoop_FinishJoin(void) {}
uint8_t CtrCoop_GetSessionState(void) { return LOCAL_SESSION_SEARCHING; }
int CtrCoop_GetFoundGames(struct CtrCoopFoundGame *games, int max) { (void)games; (void)max; return 0; }
bool CtrCoopProfile_HasGuestPassword(void) { return true; }
const struct CtrCoopProfile *CtrCoopProfile_Get(void)
{
    static const struct CtrCoopProfile profile;
    return &profile;
}
bool CtrCoop_ChooseStarter(uint16_t species)
{
    sChosenSpecies = species;
    sChooseCount++;
    return sChooseAccepted;
}

static void Press(uint16_t key)
{
    gMain.newKeys = key;
    CB2_MainMenu();
    gMain.newKeys = 0;
}

static void ResetStarter(void)
{
    ResetTasks();
    sHighlightValue = -1;
    sChosenSpecies = 0;
    sChooseCount = 0;
    sChooseAccepted = true;
    sAnimateCount = 0;
    sCallback = NULL;
    CB2_InitStarterSelection();
}

int main(void)
{
    setbuf(stdout, NULL);
    ResetTasks();
    sPcOpen = true;
    sJoining = TRUE;
    u8 joinTask = CreateTask(Task_JoinGameSearch, 0);
    gMain.newKeys = B_BUTTON;
    Task_JoinGameSearch(joinTask);
    gMain.newKeys = 0;
    CHECK(sStopCount == 0 && sJoining, "B in the initial PC does not cancel the parent join task");
    sPcOpen = false;
    gMain.newKeys = B_BUTTON;
    Task_JoinGameSearch(joinTask);
    gMain.newKeys = 0;
    CHECK(sStopCount == 1 && !sJoining && sMainMenuCount == 1,
          "B outside the initial PC cancels joining and returns to the main menu");
    ResetStarter();
    CHECK(FuncIsActiveTask(Task_StarterSelection), "starter task is created");
    CHECK(CB2_StarterSelectionActive(), "active starter task is detected");
    CHECK(sCallback == CB2_MainMenu, "starter initialization hands control back to the task scheduler");
    CHECK(gTasks[0].data[0] == 0, "Treecko is initially selected");
    CHECK(sHighlightValue == MENU_WIN_VCOORDS(8), "initial highlight is Treecko");

    Press(DPAD_DOWN);
    CHECK(gTasks[0].data[0] == 1 && sHighlightValue == MENU_WIN_VCOORDS(1), "down selects Torchic");
    Press(DPAD_DOWN);
    CHECK(gTasks[0].data[0] == 2 && sHighlightValue == MENU_WIN_VCOORDS(4), "down selects Mudkip");
    Press(DPAD_DOWN);
    CHECK(gTasks[0].data[0] == 0 && sHighlightValue == MENU_WIN_VCOORDS(8), "down wraps to Treecko");
    Press(DPAD_UP);
    CHECK(gTasks[0].data[0] == 2 && sHighlightValue == MENU_WIN_VCOORDS(4), "up wraps to Mudkip");
    Press(DPAD_UP);
    CHECK(gTasks[0].data[0] == 1 && sHighlightValue == MENU_WIN_VCOORDS(1), "up selects Torchic");

    Press(A_BUTTON);
    CHECK(sChooseCount == 1, "A confirms exactly one starter");
    CHECK(sChosenSpecies == SPECIES_TORCHIC, "A confirms the highlighted species");
    CHECK(sCallback == CB2_InitJoinGameMenu, "confirmed starter returns to join menu");
    CHECK(!CB2_StarterSelectionActive(), "confirmed starter task is closed");
    CHECK(sAnimateCount == 5, "callback handoff skips rendering after the starter task closes");

    ResetStarter();
    sChooseAccepted = false;
    Press(A_BUTTON);
    CHECK(CB2_StarterSelectionActive() && sCallback == CB2_MainMenu,
          "a rejected starter keeps the selection active");
    CHECK(sChooseCount == 1 && sAnimateCount == 1,
          "a rejected starter is attempted once and the menu keeps rendering");
    sChooseAccepted = true;
    Press(A_BUTTON);
    CHECK(sChooseCount == 2 && !CB2_StarterSelectionActive(),
          "retry after a rejected starter succeeds without leaving an active task");

    ResetTasks();
    CreateTask(TaskDummy, 0);
    joinTask = CreateTask(Task_JoinGameSearch, 0);
    CB2_InitStarterSelection();
    CHECK(!FuncIsActiveTask(Task_JoinGameSearch) && CB2_StarterSelectionActive(),
          "starter initialization replaces a join task outside slot zero");
    CHECK(GetTaskCount() == 2 && FuncIsActiveTask(TaskDummy),
          "replacing the join task preserves unrelated scheduled tasks");

    ResetStarter();
    Press(A_BUTTON);
    CHECK(sChosenSpecies == SPECIES_TREECKO, "Treecko can be confirmed");

    ResetStarter();
    Press(DPAD_DOWN);
    Press(DPAD_DOWN);
    Press(A_BUTTON);
    CHECK(sChosenSpecies == SPECIES_MUDKIP, "Mudkip can be confirmed");

    if (sFailures != 0)
    {
        printf("%d starter menu tests failed\n", sFailures);
        return 1;
    }
    puts("starter menu tests passed");
    return 0;
}
