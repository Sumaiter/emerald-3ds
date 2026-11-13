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
void CtrCoop_BattleTick(void);
bool CtrCoop_IsDoubleBattle(void);
bool CtrCoop_IsDoubleInitiator(void);
uint8_t CtrCoop_LocalMultiplayerId(void);
void CtrCoop_MenuFrame(void);
void CtrCoop_OverworldFrame(void);
bool CtrCoop_EnterHostWorld(void);
bool CtrCoop_HandlePacket(uint16_t from, const uint8_t *data, uint16_t len);
void CtrCoop_PeerLeft(uint16_t id);
void CtrCoop_SoftReset(void);

#endif // CTR_COOP_H
