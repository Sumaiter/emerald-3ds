"""Compile real patched engine function bodies against the two-peer fixture.

No engine/fixture source is rewritten. Generated C and binaries live in the
requested build/coop-live directory. Run with Python and MinGW gcc on PATH.
"""
from pathlib import Path
import argparse
import subprocess
from test_coop_bike_hooks import UPSTREAM

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    import re
    match = re.search(r"^(?:static )?(?:bool8|void) " + re.escape(name) + r"\([^;]*?\)\s*\{", source, re.M)
    if not match:
        raise AssertionError(f"missing native body: {name}")
    start = match.start()
    cursor = source.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[start:cursor]


def main():
    args = argparse.ArgumentParser()
    args.add_argument("--cc", default="gcc")
    cc = args.parse_args().cc
    engine = UPSTREAM / "src"
    bodies = []
    for filename, names in {
        "party_menu.c": ["ExecuteTableBasedItemEffect_"],
        "item_use.c": ["ItemUseInBattle_PokeBall", "ItemUseInBattle_StatIncrease", "Task_UseStatIncreaseItem"],
        "pokemon_storage_system.c": ["GiveChosenBagItem"],
    }.items():
        text = (engine / filename).read_text(encoding="utf-8")
        bodies.extend(function(text, n) for n in names)
    for filename, name in (("item_menu.c", "Task_BagMenu_HandleInput"), ("player_pc.c", "ItemStorage_ProcessInput")):
        body = function((engine / filename).read_text(encoding="utf-8"), name)
        prefix = body[:body.index("#endif") + len("#endif")]
        bodies.append(prefix + ("\n(void)listPosition;\n}" if name.startswith("Task_Bag") else "\n}"))
    for filename, name in (("item_menu.c", "CtrCoop_EngineInventoryChanged"), ("player_pc.c", "CtrPC_SharedInventoryDirty")):
        bodies.append(function((engine / filename).read_text(encoding="utf-8"), name))
    preamble = r'''
#define main inventory_core_main
#include "3ds_port/tests/coop_inventory_test.c"
#undef main
#define PLATFORM_3DS 1
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef int32_t s32;
typedef uint8_t bool8;
#define TRUE 1
#define FALSE 0
#define PARTY_ACTION_REUSABLE_ITEM 7
#define ITEM_SACRED_ASH 9
#define ITEM_NONE 0
#define MON_DATA_HELD_ITEM 1
#define PYRAMID_LOCATION_NONE 0
#define FONT_NORMAL 0
#define SE_USE_ITEM 1
struct Pokemon { int hp; } gPlayerParty[6];
struct { bool inBattle; } gMain;
struct { unsigned action; } gPartyMenu;
struct Task { s16 data[16]; void (*func)(u8); } gTasks[1];
static u8 gBattlerPartyIndexes[1], gBattlerInMenuId;
static u16 gSpecialVar_ItemId;
static bool sInPartyMenu, effectFails;
static unsigned effects, closed, notices;
static const u8 gText_WontHaveEffect[] = "no effect", gText_BoxFull[] = "full";
static bool8 IsItemFlute(u16 item) { return item == 8; }
static u8 GetPartyIdFromBattleSlot(u8 slot) { return slot; }
static bool8 ExecuteTableBasedItemEffect(struct Pokemon *mon, u16 item, u8 slot, u8 move)
{ (void)item; (void)slot; (void)move; effects++; if (effectFails) return TRUE; mon->hp++; return FALSE; }
static int CurrentBattlePyramidLocation(void) { return PYRAMID_LOCATION_NONE; }
static bool8 IsPlayerPartyAndPokemonStorageFull(void) { return FALSE; }
static bool8 RemoveBagItem(u16 item, u16 count)
{ int result = CtrCoopInventory_HookChange(0, item, -(int32_t)count); CHECK(result >= 0); return result == 1; }
static void CloseItemMessage(u8 task) { (void)task; }
static void Task_CloseBattlePyramidBagMessage(u8 task) { (void)task; }
static void Task_CloseStatIncreaseMessage(u8 task) { (void)task; }
static void Task_UseStatIncreaseItem(u8);
static void DisplayItemMessage(u8 task, int font, const u8 *text, void (*cb)(u8))
{ (void)task; (void)font; (void)text; (void)cb; notices++; }
static void DisplayItemMessageInBattlePyramid(u8 task, const u8 *text, void (*cb)(u8))
{ DisplayItemMessage(task, 0, text, cb); }
static void Task_FadeAndCloseBagMenu(u8 task) { (void)task; closed++; }
static void CloseBattlePyramidBag(u8 task) { (void)task; closed++; }
static void PlaySE(int effect) { (void)effect; }
static const u8 *UseStatIncreaseItem(u16 item) { (void)item; return gText_WontHaveEffect; }
static u8 GetCursorPosition(void) { return 0; }
static void SetMonData(struct Pokemon *mon, int field, const u16 *item)
{ (void)mon; (void)field; (void)item; effects++; }
static void SetCurrentBoxMonData(u8 pos, int field, const u16 *item)
{ (void)pos; (void)field; (void)item; effects++; }
static bool sCtrSharedBagDirty, sCtrSharedPCDirty;
static struct { bool active; } gPaletteFade;
static struct { u8 pocket; u16 scrollPosition[5], cursorPosition[5]; } gBagPosition;
static struct { u16 itemsAbove, cursorPos; } gPlayerPCItemPageInfo;
static int gMultiuseListMenuTemplate;
static unsigned refreshes, listBuilds, listCount;
#define tListTaskId data[0]
void CtrPC_SharedInventoryDirty(void);
static void BagDestroyPocketScrollArrowPair(void) {}
static void DestroyListMenuTask(int id, u16 *scroll, u16 *cursor) { (void)id; (void)scroll; (void)cursor; refreshes++; }
static void UpdatePocketItemLists(void) { listCount = 2; }
static void InitPocketListPositions(void) { gBagPosition.scrollPosition[0] = 0; gBagPosition.cursorPosition[0] = 1; }
static void LoadBagItemListBuffers(u8 pocket) { (void)pocket; }
static int ListMenuInit(int *list, u16 scroll, u16 cursor)
{ (void)list; CHECK(scroll + cursor < listCount); listBuilds++; return 0; }
static void CreatePocketScrollArrowPair(void) {}
static void ScheduleBgCopyTilemapToVram(int bg) { (void)bg; }
static void ItemStorage_RemoveScrollIndicator(void) {}
static void ItemStorage_AddScrollIndicator(void) {}
static void ItemStorage_CompactList(void) { listCount = 1; }
static void ItemStorage_CompactCursor(void) { gPlayerPCItemPageInfo.itemsAbove = 0; gPlayerPCItemPageInfo.cursorPos = 0; }
static void ItemStorage_RefreshListMenu(void) {}
'''
    checks = r'''
static void ResetEffects(void)
{ effects = closed = notices = 0; effectFails = false; memset(gPlayerParty, 0, sizeof(gPlayerParty)); gPartyMenu.action = 0; }
static void TakeLastOnHost(void)
{ CtrCoopInventory_Bind(&host.state); CHECK(CtrCoopInventory_Change(&host.state, 0, 1, -1) == CTR_INV_ACCEPTED); Drain(); CtrCoopInventory_Bind(&guest.state); }
int main(void) {
 Setup(); ResetEffects(); gSpecialVar_ItemId = 1; TakeLastOnHost();
 CHECK(ExecuteTableBasedItemEffect_(0, 1, 0) == TRUE);
 CHECK(effects == 0 && gPlayerParty[0].hp == 0);
 ItemUseInBattle_PokeBall(0); CHECK(closed == 0);
 ItemUseInBattle_StatIncrease(0); CHECK(effects == 0);
 GiveChosenBagItem(); CHECK(effects == 0);
 Setup(); ResetEffects();
 CHECK(ExecuteTableBasedItemEffect_(0, 1, 0) == FALSE);
 CHECK(effects == 1 && gPlayerParty[0].hp == 1);
 CHECK(RemoveBagItem(1, 1)); Drain();
 CHECK(Read(&host, 0, 1) == 0 && Read(&guest, 0, 1) == 0);
 Setup(); ResetEffects(); effectFails = true;
 CHECK(ExecuteTableBasedItemEffect_(0, 1, 0) == TRUE); Drain();
 CHECK(Read(&host, 0, 1) == 1 && !CtrCoopInventory_HasOpenUse(&host.state));
 Setup(); ResetEffects(); ItemUseInBattle_StatIncrease(0);
 CHECK(effects == 1 && gTasks[0].func == Task_UseStatIncreaseItem);
 gTasks[0].data[8] = 8; Task_UseStatIncreaseItem(0); Drain();
 CHECK(Read(&host, 0, 1) == 0 && !CtrCoopInventory_HasOpenUse(&host.state));
 Setup(); ResetEffects(); ItemUseInBattle_PokeBall(0); Drain();
 CHECK(closed == 1 && Read(&host, 0, 1) == 0);
 Setup(); ResetEffects(); GiveChosenBagItem(); Drain();
 CHECK(effects == 1 && Read(&host, 0, 1) == 0);
 gBagPosition.cursorPosition[0] = 29; gPlayerPCItemPageInfo.cursorPos = 49;
 CtrCoop_EngineInventoryChanged(); CHECK(refreshes == 0 && listBuilds == 0);
 CHECK(sCtrSharedBagDirty && sCtrSharedPCDirty);
 Task_BagMenu_HandleInput(0); ItemStorage_ProcessInput(0);
 CHECK(refreshes == 2 && listBuilds == 2 && !sCtrSharedBagDirty && !sCtrSharedPCDirty);
 CHECK(gBagPosition.cursorPosition[0] == 1 && gPlayerPCItemPageInfo.cursorPos == 0);
 printf("native inventory effect gates: %u failures\n", failures); return failures != 0;
}
'''
    out = ROOT / "build/coop-live"
    out.mkdir(parents=True, exist_ok=True)
    source = out / "native_inventory_gates.c"
    binary = out / "native_inventory_gates.exe"
    source.write_text(preamble + "\n".join(bodies) + checks, encoding="utf-8")
    subprocess.run([cc, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", "-I.", "-I3ds_port/include",
                    str(source), "3ds_port/src/3ds_coop_inventory.c", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
