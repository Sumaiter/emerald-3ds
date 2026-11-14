"""Run real wallet, shop, script and casino bodies against two endpoints."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess

from test_coop_bike_hooks import function, UPSTREAM

ROOT = Path(__file__).resolve().parents[2]
PREAMBLE = r'''
#define main WalletOriginalMain
#include "3ds_port/tests/coop_money_test.c"
#undef main
#define PLATFORM_3DS 1
#define MAX_MONEY 999999
static bool CtrExtras_Get(const char *name) { (void)name;return false; }
static u16 gSpecialVar_0x8005, gSpecialVar_Result;
void SetMoney(u32 *,u32);
u16 GetCoins(void); void SetCoins(u16); bool8 AddCoins(u16); bool8 RemoveCoins(u16);
struct Task { s16 data[16];void(*func)(u8); } gTasks[1];
#define tItemId data[5]
#define tItemCount data[1]
#define WIN_ITEM_LIST 0
#define WIN_MONEY 1
#define MART_TYPE_NORMAL 0
#define MART_TYPE_DECOR 1
#define GAME_STAT_SHOPPED 0
#define SE_SHOP 1
#define SE_PIN 2
static struct { u32 totalCost; } shopData,*sShopData=&shopData;
static struct { int martType; } sMartInfo;
static const u8 gText_YouDontHaveMoney[]={0},gText_HereYouGoThankYou[]={1},gText_NoMoreRoomForThis[]={2},
    gText_ThankYouIllSendItHome[]={3},gText_ThanksIllSendItHome[]={4},gText_SpaceForVar1Full[]={5};
static bool bagFull, decorFull;
static unsigned goods, services, releases;
static void(*messageCallback)(u8);
static void BuyMenuReturnToItemList(u8 t) { (void)t; }
static void BuyMenuSubtractMoney(u8);
static void Task_ReturnToItemListAfterItemPurchase(u8 t) { (void)t; }
static void Task_ReturnToItemListAfterDecorationPurchase(u8 t) { (void)t; }
static void PutWindowTilemap(int w) { (void)w; }
bool AddBagItem(u16 item,u16 count) { (void)item;(void)count;if(bagFull)return false;++goods;return true; }
static bool DecorationAdd(u16 item) { (void)item;if(decorFull)return false;++goods;return true; }
static void BuyMenuDisplayMessage(u8 t,const u8 *s,void(*cb)(u8)) { (void)t;(void)s;messageCallback=cb; }
static void RecordItemPurchase(u8 t) { (void)t; }
static void IncrementGameStat(int id) { (void)id; }
void PlaySE(u16 id) { (void)id; }
static void PrintMoneyAmountInMoneyBox(u8 w,int a,u8 s) { (void)w;(void)a;(void)s; }
struct ScriptContext { u32 amount;u8 ignore;bool stopped; };
static u32 ScriptReadWord(struct ScriptContext *ctx) { return ctx->amount; }
static u8 ScriptReadByte(struct ScriptContext *ctx) { return ctx->ignore; }
static u16 ScriptReadHalfword(struct ScriptContext *ctx) { return ctx->amount; }
static u16 VarGet(u16 id) { return id; }
static void StopScript(struct ScriptContext *ctx) { ctx->stopped=true; }
static bool8 ScrCmd_releaseall(struct ScriptContext *ctx) { (void)ctx;++releases;return false; }
#define SELECT_BUTTON 1
#define R_BUTTON 2
#define DPAD_DOWN 4
#define A_BUTTON 8
#define B_BUTTON 16
#define START_BUTTON 32
#define JOY_NEW(b) ((buttons & (b)) != 0)
#define JOY_HELD(b) JOY_NEW(b)
#define MAX_BET 3
#define DIG_DISPLAY_INSERT_BET 0
#define SLOTTASK_WAIT_INFO_BOX 1
#define SLOTTASK_START_SPIN 2
#define SLOTTASK_MSG_NEED_3_COINS 3
#define SLOTTASK_ASK_QUIT 4
static unsigned buttons,betEffects;
static struct { s16 coins,bet,state,payout; } slot,*sSlotMachine=&slot;
static void OpenInfoBox(int n) { (void)n; }
static void LightenBetTiles(int n) { (void)n;++betEffects; }
static bool IsFanfareTaskInactive(void) { return true; }
#define tState data[0]
#define tTimer data[1]
#define tCoins data[13]
#define tSelectionId data[4]
#define tMultiplier data[2]
static struct { u8 minBet,curBallNum,betSelection[6]; } roulette,*sRoulette=&roulette;
static void Task_AskKeepPlaying(u8 t) { (void)t; }
static void Task_StartSpin(u8 t) { (void)t;++betEffects; }
static int GetMultiplier(u8 slotId) { (void)slotId;return 2; }
static void SetMultiplierSprite(u8 slotId) { (void)slotId;++betEffects; }
static void SetCreditDigits(int n) { (void)n; }
'''

TESTS = r'''
int main(void)
{
    MoneySetup(1000,100); goods=0; bagFull=decorFull=false;
    gTasks[0].tItemId=1;gTasks[0].tItemCount=1;sShopData->totalCost=700;
    sMartInfo.martType=MART_TYPE_NORMAL;
    BuyMenuTryMakePurchase(0); CHECK(goods==1 && Cash()==300 && sWallet.useCount==700);
    MoneySwitch(&host); CHECK(!CtrCoopMoney_BeginUse(1,700));
    MoneySwitch(&guest); messageCallback(0); CHECK(Cash()==300 && !sWallet.useCount);
    MoneyDrain(); MoneySwitch(&host); CHECK(Cash()==300);

    MoneySetup(1000,100); bagFull=true; goods=0;
    BuyMenuTryMakePurchase(0); CHECK(Cash()==1000 && goods==0 && !sWallet.useCount);
    bagFull=false; decorFull=true;sMartInfo.martType=MART_TYPE_DECOR;
    BuyMenuTryMakePurchase(0); CHECK(Cash()==1000 && goods==0 && !sWallet.useCount);
    decorFull=false;

    struct ScriptContext ctx={.amount=700};
    CHECK(!ScrCmd_checkmoney(&ctx) && gSpecialVar_Result && Cash()==300);
    MoneySwitch(&host); CHECK(!CtrCoopMoney_BeginUse(1,700)); MoneySwitch(&guest);
    CHECK(!ScrCmd_removemoney(&ctx) && Cash()==300 && !sWallet.useCount);
    ctx.amount=700; CHECK(ScrCmd_removemoney(&ctx) && ctx.stopped && releases>0 && Cash()==300);
    MoneySetup(1000,100);ctx=(struct ScriptContext){.amount=250};
    ScrCmd_checkmoney(&ctx); CHECK(Cash()==750);CtrCoopMoney_RefundUse();CHECK(Cash()==1000);
    gSpecialVar_0x8005=300;
    CHECK(IsEnoughForCostInVar0x8005() && Cash()==700);
    services++;SubtractMoneyFromVar0x8005();CHECK(Cash()==700 && !sWallet.useCount);

    AddMoney(&gSaveBlock1Ptr->money,123); CHECK(Cash()==823);
    RemoveMoney(&gSaveBlock1Ptr->money,2000); CHECK(Cash()==0);
    AddMoney(&gSaveBlock1Ptr->money,0xFFFFFFFF); CHECK(Cash()==MAX_MONEY);
    SetMoney(&gSaveBlock1Ptr->money,GetMoney(&gSaveBlock1Ptr->money)/2);
    CHECK(Cash()==MAX_MONEY/2 && CtrCoopMoney_FinishBattle());MoneyDrain();
    MoneySwitch(&host);CHECK(Cash()==MAX_MONEY/2);

    MoneySetup(1000,2);slot=(__typeof__(slot)){.coins=2};buttons=R_BUTTON;betEffects=0;
    SlotTask_HandleBetInput(&gTasks[0]);CHECK(!betEffects && Coins()==2);
    slot.coins=999; /* stale casino copy after another player spends */
    SlotTask_HandleBetInput(&gTasks[0]);CHECK(!betEffects && Coins()==2);
    buttons=DPAD_DOWN;SlotTask_HandleBetInput(&gTasks[0]);CHECK(betEffects==1 && Coins()==1 && slot.bet==1);
    slot.payout=20;buttons=START_BUTTON;gTasks[0].tTimer=0;
    PayoutTask_GivePayout(&gTasks[0]);CHECK(Coins()==21 && slot.coins==21 && !slot.payout);
    MoneyDrain();MoneySwitch(&host);CHECK(Coins()==21);

    MoneySetup(1000,2);sRoulette->minBet=3;betEffects=0;gTasks[0].tCoins=999;
    Task_PlaceBet(0);CHECK(!betEffects && Coins()==2 && gTasks[0].func==Task_AskKeepPlaying);
    sRoulette->minBet=1;Task_PlaceBet(0);CHECK(betEffects==1 && Coins()==1);
    ctx=(struct ScriptContext){.amount=100};CHECK(ScrCmd_removecoins(&ctx) && ctx.stopped && Coins()==1);
    CHECK(AddCoins(100) && GetCoins()==101); CHECK(RemoveCoins(1) && GetCoins()==100);
    MoneyDrain();MoneySwitch(&host);CHECK(Coins()==100);

    CHECK(CtrCoopMoney_PrepareNativeLink() && !CtrCoopMoney_Active() && !sClientNonce);
    moneySessionKind=1;
    SetMoney(&gSaveBlock1Ptr->money,100);AddMoney(&gSaveBlock1Ptr->money,50);RemoveMoney(&gSaveBlock1Ptr->money,20);
    CHECK(GetMoney(&gSaveBlock1Ptr->money)==130);
    SetCoins(4);CHECK(AddCoins(2) && RemoveCoins(1) && GetCoins()==5);
    CtrCoopMoney_Tick();CHECK(GetMoney(&gSaveBlock1Ptr->money)==130 && GetCoins()==5);
    MoneySetup(1000,100);MoneySwitch(&host);CHECK(CtrCoopMoney_BeginUse(1,300));
    unsigned packets=tail;sWallet.pending=sWallet.reconciling=true;
    CtrCoopMoney_AfterStackReset();CHECK(!CtrCoopMoney_Active() && tail==packets);
    SetMoney(&gSaveBlock1Ptr->money,2222);SetCoins(42);CtrCoopMoney_Tick();
    CHECK(GetMoney(&gSaveBlock1Ptr->money)==2222 && GetCoins()==42);
    CtrCoopMoney_Session(true,false,guest.id,true);
    CHECK(GetMoney(&gSaveBlock1Ptr->money)==2222 && GetCoins()==42);
    printf("real shop/script/money/coin/casino bodies: %u failures\n",failures);
    return failures?1:0;
}
'''

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    parser.add_argument('--source', type=Path, default=UPSTREAM/'src')
    args = parser.parse_args()
    bodies = []
    for name, funcs in {
        'money.c': ['GetMoney','SetMoney','IsEnoughMoney','AddMoney','RemoveMoney',
                    'IsEnoughForCostInVar0x8005','SubtractMoneyFromVar0x8005'],
        'coins.c': ['GetCoins','SetCoins','AddCoins','RemoveCoins'],
        'shop.c': ['BuyMenuTryMakePurchase','BuyMenuSubtractMoney'],
        'scrcmd.c': ['ScrCmd_checkmoney','ScrCmd_removemoney','ScrCmd_removecoins'],
        'slot_machine.c': ['SlotTask_HandleBetInput','PayoutTask_GivePayout'],
        'roulette.c': ['Task_PlaceBet'],
    }.items():
        source = (args.source/name).read_text(encoding='utf-8')
        for f in funcs:
            body = function(source, f)
            if f == 'SlotTask_HandleBetInput':
                # Native function deliberately ignores its Task argument.
                body = '#pragma GCC diagnostic push\n#pragma GCC diagnostic ignored "-Wunused-parameter"\n' + body + '#pragma GCC diagnostic pop\n'
            bodies.append(body)
    # Casino exits must never overwrite newer remote changes with local copies.
    for name, func in [('slot_machine.c','SlotTask_EndGame'),('roulette.c','ExitRoulette')]:
        body = function((args.source/name).read_text(encoding='utf-8'),func)
        assert 'if (!CtrCoopMoney_Active())' in body
    out = ROOT/'build/coop-live'
    out.mkdir(parents=True,exist_ok=True)
    cfile=out/'money-native.c'; exe=out/'money-native.exe'
    cfile.write_text(PREAMBLE+'\n'.join(bodies)+TESTS,encoding='utf-8')
    subprocess.run(shlex.split(args.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT),
                    '-I'+str(ROOT/'3ds_port/tests/coop_host'),'-I'+str(ROOT/'3ds_port/include'),
                    str(cfile),str(ROOT/'3ds_port/src/3ds_coop_inventory.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)

if __name__=='__main__': main()
