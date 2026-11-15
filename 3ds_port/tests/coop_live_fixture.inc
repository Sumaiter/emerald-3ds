/* External engine/presentation seams for the existing backend fixtures.
 * The spectator and renderer themselves have separate real-module tests. */
#include <setjmp.h>
static jmp_buf sCoopMenuEntry;
static bool sCoopMenuScope;
static unsigned sCoopMenuReturns;
/* The production API cannot return. Model its unwind, then reset only from
 * the surviving test entry (the game's setjmp re-entry has a separate test). */
#define EXPECT_GUEST_MENU(action) do { \
    sCoopMenuScope=true; \
    if (setjmp(sCoopMenuEntry)==0) { action; CHECK(false,"missing guest stack unwind"); } \
    sCoopMenuScope=false; \
    CtrCoop_AfterStackReset(); CtrCoop_SoftReset(); \
} while (0)
static bool sStoryTrySendBlocked;
static bool sStoryNpcAvailable = true;
static bool sControlsLocked, sScriptEnabled;
static bool sWatchReady, sWatching;
static unsigned sWatchRequests, sAutoSaves;
static bool sAutoSaveSucceeds = true;
static bool sLiveInventoryActive, sLiveMoneyActive;
static unsigned sFollowWarpCalls;
static uint8_t sFollowWarpGroup, sFollowWarpNum;
static int16_t sFollowWarpX, sFollowWarpY;
static bool sPrepareFollowBattle = true;
bool CtrCoop_EnginePrepareFollowBattle(void) { return sPrepareFollowBattle; }
bool CtrCoop_EngineTestPartyActive(void) { return false; }
bool CtrCoop_EngineFollowWarp(uint8_t group,uint8_t num,uint16_t layout,int16_t x,int16_t y)
{
    (void)layout; ++sFollowWarpCalls; sFollowWarpGroup=group;sFollowWarpNum=num;
    sFollowWarpX=x;sFollowWarpY=y;return true;
}
void CB2_WhiteOut(void) {}
void CB2_ReturnToFieldFadeFromBlack(void) {}

uint32_t LocalLink_Milliseconds(void) { return 1000; }
void PortRfu_PumpCoopEvents(void) {}
void CtrCoopInventoryBridge_Session(bool active,bool guest,uint16_t peer,bool applied)
{ (void)active;(void)guest;(void)peer;(void)applied; }
void CtrCoopInventoryBridge_AfterSaveCopy(void) {}
void CtrCoopInventoryBridge_Tick(void) {}
bool CtrCoopInventoryBridge_HandlePacket(uint16_t from,const uint8_t *data,uint16_t len)
{ (void)from;(void)data;(void)len;return true; }
bool CtrCoopInventoryBridge_Active(void) { return sLiveInventoryActive; }
bool CtrCoopInventoryBridge_Ready(void) { return true; }
bool CtrCoopInventoryBridge_FinishBattle(void) { return true; }
bool CtrCoopInventoryBridge_HasPending(void) { return false; }
bool CtrCoopInventoryBridge_KnownPeer(void) { return true; }
bool CtrCoopInventoryBridge_IsResuming(void) { return false; }
void CtrCoopInventoryBridge_Reset(void) {}
void CtrCoopInventoryBridge_AfterStackReset(void) {}
bool CtrCoopInventoryBridge_PrepareNativeLink(void) { return true; }
bool CtrCoopInventoryBridge_CanPrepareNativeLink(void) { return true; }
void CtrCoopMoney_Session(bool active,bool guest,uint16_t peer,bool applied)
{ (void)active;(void)guest;(void)peer;(void)applied; }
void CtrCoopMoney_AfterSaveCopy(void) {}
void CtrCoopMoney_Tick(void) {}
bool CtrCoopMoney_HandlePacket(uint16_t from,const uint8_t *data,uint16_t len)
{ (void)from;(void)data;(void)len;return true; }
bool CtrCoopMoney_Active(void) { return sLiveMoneyActive; }
bool CtrCoopMoney_Ready(void) { return true; }
bool CtrCoopMoney_FinishBattle(void) { return true; }
bool CtrCoopMoney_HasPending(void) { return false; }
void CtrCoopMoney_Reset(void) {}
void CtrCoopMoney_AfterStackReset(void) {}
bool CtrCoopMoney_PrepareNativeLink(void) { return true; }
bool CtrCoopMoney_CanPrepareNativeLink(void) { return true; }
#ifndef CTR_COOP_CUSTOM_RETURN_TO_MENU
void CtrGame_ReturnToMenu(void)
{
    ++sCoopMenuReturns;
    if (!sCoopMenuScope) { fputs("unexpected guest stack unwind\n",stderr); __builtin_abort(); }
    longjmp(sCoopMenuEntry,1);
}
#endif
void CtrCoopMoney_RefundUse(void) {}

bool LocalLink_TrySend(uint16_t dest, const void *data, uint32_t len)
{
    if (sStoryTrySendBlocked) return false;
    LocalLink_Send(dest, data, len);
    return true;
}
bool8 ArePlayerFieldControlsLocked(void) { return sControlsLocked; }
bool8 ScriptContext_IsEnabled(void) { return sScriptEnabled; }
#ifndef CTR_COOP_CUSTOM_NPC_MOTION_HELPERS
bool CtrCoop_EngineNpcMotionCapture(uint8_t group,uint8_t num,uint8_t id,CtrCoopNpcMotionPose *pose)
{ (void)group;(void)num;(void)id;(void)pose;return false; }
int CtrCoop_EngineNpcMotionApply(const CtrCoopNpcMotionPose *pose,CtrCoopNpcMotionPlayback *playback,bool ending)
{ (void)pose;(void)playback;(void)ending;return 1; }
#endif
bool CtrCoop_EngineStoryNpcAvailable(uint8_t group, uint8_t num, uint8_t id)
{ (void)group; (void)num; (void)id; return sStoryNpcAvailable; }
void CtrCoop_EngineStoryRefresh(void) {}
void CtrCoop_EngineStoryObject(uint8_t group,uint8_t num,uint8_t id,uint8_t fields,
                             int16_t x,int16_t y,uint8_t movement)
{ (void)group;(void)num;(void)id;(void)fields;(void)x;(void)y;(void)movement; }
bool CtrCoop_EngineAutoSave(void) { ++sAutoSaves; return sAutoSaveSucceeds; }
void CtrCoopSpectator_SetState(uint16_t peer,bool connected,bool other,bool local,bool can)
{ (void)peer;(void)other;(void)local; if (!connected || !can) sWatching=false; }
void CtrCoopSpectator_Tick(void) {}
bool CtrCoopSpectator_Request(void) { ++sWatchRequests; sWatching=sWatchReady; return sWatching; }
void CtrCoopSpectator_Stop(void) { sWatching=false; }
bool CtrCoopSpectator_IsWatching(void) { return sWatching; }
bool CtrCoopSpectator_CanWatchPeer(void) { return sWatchReady; }
bool CtrCoopSpectator_HandlePacket(uint16_t from,const uint8_t *data,uint16_t len)
{ (void)from;(void)data;(void)len;return true; }
void CtrCoopSpectator_Reset(void) { sWatching=false; }

void CtrCoop_EngineRemoteStepEffects(uint8_t objectEventId) { (void)objectEventId; }

static void ResetLiveFixture(void)
{
    sStoryTrySendBlocked=false;
    sStoryNpcAvailable=true;
    sControlsLocked=sScriptEnabled=sWatchReady=sWatching=false;
    sWatchRequests=sAutoSaves=0;
    sAutoSaveSucceeds=true;
    sDisconnectSavePending=false;
    sDisconnectSaveRetry=0;
    sLiveBattleEndPending=sResumeSharedWorld=false;
    sLiveInventoryActive=sLiveMoneyActive=false;
    sCoopMenuReturns=0;
}
