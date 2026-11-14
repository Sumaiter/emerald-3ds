#ifndef CTR_COOP_H
#define CTR_COOP_H

#include <stdbool.h>
#include <stdint.h>

#define CTR_COOP_VERSION "0.1.0"
#define CTR_COOP_MAX_GUEST_NAMES 10
/* A co-op status, outside the transport's LOCAL_SESSION_* values. */
#define CTR_COOP_SESSION_NAME_LIMIT 0x80

void CtrCoop_Toggle(void);
bool CtrCoop_IsEnabled(void);
bool CtrCoop_PrepareNativeLink(void);
bool CtrCoop_UseCustomGraphicsId(void);
uint8_t CtrCoop_LocalGraphicsId(void);
struct CtrCoopFoundGame
{
    uint32_t token;
    uint8_t hostName[8];
};

void CtrCoop_Search(void);
void CtrCoop_PrepareJoin(uint32_t token);
void CtrCoop_JoinPrepared(void);
void CtrCoop_JoinGame(uint32_t token, const uint8_t *password);
void CtrCoop_Stop(void);
bool CtrCoop_GuestSaveBlocked(void);
uint8_t CtrCoop_GetSessionState(void);
int CtrCoop_GetFoundGames(struct CtrCoopFoundGame *games, int max);
bool CtrCoop_RequestPC(void);
/* Game-encoded PC owner's name while the bottom-screen notice is modal. */
const uint8_t *CtrCoop_PCNoticeName(void);
void CtrCoop_DismissPCNotice(void);
bool CtrCoop_PCOpen(void);
void CtrCoop_ReleasePC(void);
void CtrCoop_CommitPC(void);
bool CtrCoop_InitialTeamSelection(void);
bool CtrCoop_ChooseStarter(uint16_t species);
bool CtrCoop_StorageSlotLocked(uint8_t boxId, uint8_t position);
bool CtrCoop_StorageBoxRestricted(uint8_t boxId);
void CtrCoop_FinishJoin(void);
bool CtrCoop_BattleStart(void);
void CtrCoop_BattleEnd(void);
/* Temporary Frontier/partner parties must never replace the saved full team.
 * A native LoadPlayerParty marks a candidate; the field tick validates it. */
void CtrCoop_BattleEndAfterPartyRestore(void);
void CtrCoop_BattlePartyRestored(void);
/* SaveGameFrontier loads the full team only for saving, then undoes the load. */
void CtrCoop_IgnoreNextPartyRestore(void);
void CtrCoop_TryFinishBattleReturn(void);
void CtrCoop_ResetBattleReturn(void);
void CtrCoop_BattleTick(void);
/* A on the other avatar or its current opponent, without starting a script. */
bool CtrCoop_TryWatchAt(int16_t x, int16_t y);
bool CtrCoop_IsWatching(void);
const uint8_t *CtrCoop_WatchingName(void);
struct BoxPokemon;
struct Pokemon;
/* Preserve the offline guest row's HP/status when native box conversion runs. */
void CtrCoop_RestoreBoxVitals(const struct BoxPokemon *source, struct Pokemon *dest);
bool CtrCoop_DisconnectSavePending(void);
/* Engine side: save only from a stable, unlocked overworld callback. */
bool CtrCoop_EngineAutoSave(void);
/* A corrected remote tile is a step, not a newly spawned standing avatar. */
void CtrCoop_EngineRemoteStepEffects(uint8_t objectEventId);
/* Claim field events before their first bytecode command. NPC identity is
 * independent of trainer/starter/gender variants. Two approaching trainers
 * are claimed atomically. Poll: -1 cancel, 0 wait, 1 execute. */
void CtrCoop_ScriptObject(uint8_t mapGroup, uint8_t mapNum, uint8_t localId);
/* Protect additional actors controlled by the granted local script from host
 * NPC snapshots through dialogue/removal and until its lease is released. */
void CtrCoop_ScriptActor(uint8_t mapGroup, uint8_t mapNum, uint8_t localId);
void CtrCoop_ScriptBegin(const void *script);
int CtrCoop_ScriptPoll(void);
void CtrCoop_ScriptEnd(void);
const uint8_t *CtrCoop_NoticeName(void);
bool CtrCoop_NoticeIsInteraction(void);
void CtrCoop_DismissNotice(void);
bool CtrCoop_IsDoubleBattle(void);
bool CtrCoop_IsDoubleInitiator(void);
/* Restore the original full party before native defeat/field callbacks. */
void CtrCoop_PrepareDoubleReturn(void);
uint8_t CtrCoop_LocalMultiplayerId(void);
void CtrCoop_MenuFrame(void);
void CtrCoop_OverworldFrame(void);
bool CtrCoop_EnterHostWorld(void);
bool CtrCoop_HandlePacket(uint16_t from, const uint8_t *data, uint16_t len);
void CtrCoop_PeerLeft(uint16_t id);
/* Frame and synchronous RPC paths: a departed host discards the guest stack. */
void CtrCoop_CheckHostConnection(void);
/* Called only after CtrGame_Init's longjmp re-entry, before resetting RAM. */
void CtrCoop_AfterStackReset(void);
void CtrCoop_SoftReset(void);

/* Ordered story/object drain and native world reconciliation. */
void CtrCoop_StoryTick(void);
void CtrCoop_ResetStory(void);
void CtrCoop_ClearStoryWorld(void);
bool CtrCoop_StoryOwnsNpc(uint8_t group, uint8_t num, uint8_t localId);
bool CtrCoop_StoryNpcVisible(uint8_t group, uint8_t num, uint8_t localId);
bool CtrCoop_HandleStoryPacket(uint16_t from, const uint8_t *data, uint16_t len);
void CtrCoop_StoryFlagChanged(uint16_t id);
void CtrCoop_StoryVarChanged(uint16_t id);
void CtrCoop_StoryObject(uint8_t group, uint8_t num, uint8_t localId, uint8_t fields,
                         int16_t x, int16_t y, uint8_t movementType);
bool CtrCoop_EngineStoryNpcAvailable(uint8_t group, uint8_t num, uint8_t localId);
void CtrCoop_EngineStoryRefresh(void);
void CtrCoop_EngineStoryTemplateChanged(uint8_t localId, uint8_t fields);
void CtrCoop_EngineStoryObject(uint8_t group, uint8_t num, uint8_t localId, uint8_t fields,
                              int16_t x, int16_t y, uint8_t movementType);

#endif // CTR_COOP_H
