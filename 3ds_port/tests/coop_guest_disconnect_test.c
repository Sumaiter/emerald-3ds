/* Real backend, transport seams from the session fixture; no game/SAV I/O. */
#include <setjmp.h>
#define CTR_COOP_CUSTOM_RETURN_TO_MENU
#define main CoopSessionOriginalMain
#include "coop_session_test.c"
#undef main

static jmp_buf sGuestEntry;
static unsigned sMenuReturns;
static volatile unsigned sOldEffectRuns;

void CtrGame_ReturnToMenu(void)
{
    ++sMenuReturns;
    longjmp(sGuestEntry, 1);
}

static void Guest(void)
{
    ReadyHost();
    sCoop.guest=true;
    sCoop.guestWorldLoaded=true;
    sCoop.authenticated=true;
    sCoop.saveReady=true;
    sCoop.peerId=PEER_ID;
    sMenuReturns=0;
    sOldEffectRuns=0;
}

static void LoseHost(unsigned way)
{
    if (way==0) CtrCoop_PeerLeft(PEER_ID);
    if (way==1) { sSession=LOCAL_SESSION_NONE; CtrCoop_OverworldFrame(); }
    if (way==2) { sPeerCount=0; CtrCoop_OverworldFrame(); }
    if (way==3) { sSessionKind=0; CtrCoop_CheckHostConnection(); }
    if (way==4) CtrCoop_Stop();
    if (way==5) { sSession=LOCAL_SESSION_NONE; CtrCoop_PumpInventory(); }
    if (way==6) { sPeerCount=0; sInventoryPumping=true; CtrCoop_PumpInventory(); }
    if (way==7) ClearRemote();
    ++sOldEffectRuns; /* Native item/money/script continuation must be unreachable. */
}

static void TestGuestLoss(void)
{
    for (volatile unsigned mode=0;mode<6;++mode)
    for (volatile unsigned way=0;way<8;++way)
    {
        Guest();
        if (mode==1) { sCoop.battleActive=true; gMain.inBattle=true; }
        if (mode==2) { sCoop.pcOpen=true; sCoop.pcOwner=PC_OWNER_GUEST; }
        if (mode==3) { gMain.callback2=NULL; sControlsLocked=true; }
        if (mode==4) sWatching=true;
        if (mode==5) { sCoop.eventState=EVENT_HELD; sCoop.eventStarting=true; }
        if (setjmp(sGuestEntry)==0)
        {
            LoseHost(way);
            CHECK(false,"guest loss must leave the whole engine stack");
        }
        CHECK(sMenuReturns==1 && sOldEffectRuns==0,"no old effect after host loss (%u/%u)",mode,way);
        CHECK(sCoop.guestWorldLoaded && sAutoSaves==0,"no premature world/save mutation");
        CtrCoop_AfterStackReset();
        CtrCoop_SoftReset();
        CHECK(!sCoop.wanted && !sCoop.guestWorldLoaded && !sCoop.authenticated,
              "new native boot cannot inherit guest world");
        CHECK(!sInventoryPumping && !sResumeSharedWorld && !sWatching,
              "pump/watch/reconnect are cleared after stack abandonment");
        CHECK(!sDisconnectSavePending && sAutoSaves==0 && sSession==LOCAL_SESSION_NONE,
              "guest never requests a save during reset");
    }
}

static void TestUnaffectedContexts(void)
{
    ReadyHost(); sMenuReturns=0;
    CtrCoop_PeerLeft(PEER_ID);
    CHECK(!sMenuReturns && sCoop.wanted,"host remains in its world after guest loss");
    ReadyHost(); sCoop.guest=true; sCoop.authenticated=false; sCoop.guestWorldLoaded=false;
    sSession=LOCAL_SESSION_SEARCHING;
    CtrCoop_CheckHostConnection();
    CtrCoop_Stop();
    CHECK(!sMenuReturns,"a search that never joined has no guest world to unwind");
    Guest();
    CtrCoop_CheckHostConnection();
    CtrCoop_PumpInventory();
    CtrCoop_PeerLeft(PEER_ID+1);
    CHECK(!sMenuReturns && sCoop.authenticated,"healthy session/unrelated leave stay connected");
    /* A second join starts clean rather than continuing a suspended effect. */
    CtrCoop_SoftReset();
    CtrCoop_JoinGame(1234,CtrCoopProfile_Get()->guestPassword);
    CHECK(!sCoop.guestWorldLoaded && !sCoop.authenticated && !sResumeSharedWorld,
          "new join requires fresh authentication and snapshot");
}

int main(void)
{
    TestGuestLoss();
    TestUnaffectedContexts();
    if(sFailures) return 1;
    puts("guest disconnect: 48 context/loss paths discard effect stacks; host/search/rejoin passed");
    return 0;
}
