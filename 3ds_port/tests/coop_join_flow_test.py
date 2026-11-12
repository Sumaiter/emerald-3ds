from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent
UPSTREAM = REPO / 'build/upstream'

failures = []

def check(condition, message):
    if not condition:
        failures.append(message)

menu = (UPSTREAM / 'src/main_menu.c').read_text(encoding='utf-8')
adapter = (REPO / '3ds_port/src/3ds_rfu_adapter.c').read_text(encoding='utf-8')
coop = (REPO / '3ds_port/src/3ds_coop.c').read_text(encoding='utf-8')

enter_world = coop.index('bool CtrCoop_EnterHostWorld(void)')
clear_view = coop.index('memset(gSaveBlock1Ptr->mapView, 0, sizeof(gSaveBlock1Ptr->mapView));', enter_world)
warp = coop.index('WarpIntoMap();', clear_view)
check(clear_view < warp,
      'Guest join must discard the stale saved mapView before the single map load')
reload_objects = coop.index('if (reloadObjects)', warp)
load_objects = coop.index('LoadObjEventTemplatesFromHeader();', reload_objects)
check(warp < reload_objects < load_objects,
      'Cross-map join must replace stale NPC templates after the warp')
check(coop.index('memset(&gObjectEvents[i], 0, sizeof(gObjectEvents[i]));', reload_objects) < load_objects,
      'Cross-map join must remove active NPCs from the old map')
check(coop.index('if (!gObjectEvents[i].isPlayer)', reload_objects) < load_objects,
      'Cross-map join must preserve the local player object')

prepare_save = coop.index('static void PrepareHostSave(uint16_t to)')
save_live = coop.index('SaveObjectEvents();', prepare_save)
copy_save = coop.index('memcpy(sSaveData + offset, gSaveBlock1Ptr', save_live)
check(prepare_save < save_live < copy_save,
      'Initial snapshot must contain live NPC objects rather than stale save data')
join_info = coop.index('SendJoinInfo(to);', save_live)
check(join_info < copy_save,
      'Host must tell the guest whether starter selection is required before the snapshot')
choose_starter = coop.index('bool CtrCoop_ChooseStarter(uint16_t species)')
check('!sCoop.starterRequired || sCoop.starterChosen' in coop,
      'Only new guests may create a starter')
check('species != SPECIES_TREECKO && species != SPECIES_TORCHIC && species != SPECIES_MUDKIP' in coop[choose_starter:],
      'Starter choice must be restricted to the three Hoenn starters')
check('CreateMon(&gPlayerParty[0], species, 5, 0, FALSE, 0, OT_ID_PLAYER_ID, 0);' in coop[choose_starter:],
      'Starter must be created at level five')
check('StorePartyRow(CTR_COOP_STORAGE_GUEST_ROW, gPlayerParty);' in coop[choose_starter:],
      'Starter must be stored in the guest co-op row')
check('(sCoop.starterRequired && !sCoop.starterChosen)' in coop,
      'World entry must wait until the starter has been chosen')
check('!sCoop.joinInfoReceived' in coop,
      'World entry must wait until the starter requirement has been received')
overworld_frame = coop.index('void CtrCoop_OverworldFrame(void)')
open_pc = coop.index('OpenPCWhenReady();', overworld_frame)
check('!sCoop.joinInfoReceived' in coop[overworld_frame:open_pc]
      and '(sCoop.starterRequired && !sCoop.starterChosen)' in coop[overworld_frame:open_pc],
      'The frame handler must not open the PC before join info and starter selection')
check('SetMainCallback2(CB2_InitStarterSelection);' in coop,
      'New guests must see the starter screen before the start-team PC')
check('if (!CB2_StarterSelectionActive())' in coop,
      'Starter screen must use the active task instead of repeatedly reinitializing')
check('gMain.callback2 != CB2_InitStarterSelection' not in coop,
      'Starter screen must not depend on the one-shot engine callback')
check('return FuncIsActiveTask(Task_StarterSelection);' in menu,
      'Starter active check must inspect the real task state')
check('JoinGameHighlight(0);' in menu,
      'Starter screen must initialize the selection highlight')
check('SetMainCallback2(CB2_MainMenu);' in menu,
      'Starter initialization must return to the callback that runs tasks')
check('static const u16 sStarterSpecies[] = {SPECIES_TREECKO, SPECIES_TORCHIC, SPECIES_MUDKIP};' in menu,
      'Starter screen must offer exactly the three Hoenn starters')
check('if (JOY_NEW(A_BUTTON) && CtrCoop_ChooseStarter(sStarterSpecies[selected]))' in menu,
      'Starter screen must commit the selected species')
check('if (!sCoop.guest)\n        SyncHostNpc(&peers[0]);' in coop,
      'Host must keep streaming authoritative NPC snapshots')
check('if (data[0] == CTR_COOP_NPC_PACKET)' in coop,
      'Guest must handle host NPC packets')
apply_npc = coop.index('static void ApplyHostNpc(const CtrCoopNpcSnapshot *snapshot)')
check('MoveObjectEventToMapCoords(object, snapshot->x, snapshot->y);' in coop[apply_npc:],
      'Guest must apply the authoritative NPC position')
check('object->frozen = TRUE;' in coop[apply_npc:],
      'Guest NPCs must not wander independently between host updates')
check('id < FLAGS_COUNT && sent < 8' in coop,
      'Story flag sending must stay inside the flag baseline')
check('id >= FLAGS_COUNT' in coop,
      'Received flags must be rejected outside the flag baseline')
check('id < SPECIAL_FLAGS_START && sent < 8' not in coop,
      'Story flag sending must not overrun the co-op state into SPECIAL_FLAGS_START')
check('id < VARS_START + VARS_COUNT && sent < 8' in coop,
      'Story sending must stay inside the variable baseline')
check('id >= VARS_START + VARS_COUNT' in coop,
      'Received variables must be rejected outside the variable baseline')
check('id < SPECIAL_VARS_START && sent < 8' not in coop,
      'Story sending must not overrun the co-op state into SPECIAL_VARS_START')

continue_join = menu.index('static void ContinueJoinedGame(void)')
enter_host = menu.index('CtrCoop_EnterHostWorld();', continue_join)
continue_saved = menu.index('CB2_ContinueSavedGame();', enter_host)
check(enter_host < continue_saved,
      'ContinueJoinedGame must set the spawn before the single map load')
check(menu.find('InitMap();', continue_saved) == -1,
      'ContinueJoinedGame must not initialize the map twice')

join_task = menu.index('static void Task_JoinGameSearch(u8 taskId)')
cached_game = menu.index('games[0] = sJoiningGame;', join_task)
check(cached_game > join_task,
      'Joining must retain the selected world when discovery stops returning it')
check(menu.index('JoinGamePrint(0, JOIN_CONNECTING_DOTS[gTasks[taskId].data[3]]);', cached_game) > cached_game,
      'Joining must display CONNECTING instead of SEARCH')
check('if (state == LOCAL_SESSION_NONE && !sJoining)' in menu[join_task:],
      'Joining must not restart discovery and clear the selected world')
check('else if (!found && !sJoining && gTasks[taskId].data[4] >= 600)' in menu[join_task:],
      'Joining must not display the no-open-world hints')
check('if (state != LOCAL_SESSION_IN && !sJoining)' in menu[join_task:],
      'Joining must keep the menu selection locked')

local_update = adapter.index('LocalSession_Update();')
coop_frame = adapter.index('CtrCoop_OverworldFrame();', local_update)
check(local_update < coop_frame,
      'PortRfu_Frame must run CtrCoop_OverworldFrame after LocalSession_Update')

if failures:
    for failure in failures:
        print('FAIL', failure)
    raise SystemExit(1)
print('co-op join flow tests passed')
