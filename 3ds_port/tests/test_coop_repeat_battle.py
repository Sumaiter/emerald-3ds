"""Native repeatable test-battle return against the real root reservation code."""
import argparse,os,re,shlex,subprocess,tempfile
from pathlib import Path
from test_coop_bike_hooks import function,UPSTREAM,REPO

PRE=r'''
#define main OriginalSessionMain
#include "coop_session_test.c"
#undef main
typedef u32 bool32;
static bool8 sRoute101TestPartyActive;
static void LoadPlayerParty(void){memcpy(gPlayerParty,gSaveBlock1Ptr->playerParty,sizeof(gPlayerParty));}
static void CB2_ReturnToFieldContinueScriptPlayMapMusic(void){}
static void FlagSet(u16 flag){sSaveBlock1.flags[flag/8]|=1u<<(flag&7);}
static u16 GetTrainerAFlag(void){return TRAINER_FLAGS_START+gTrainerBattleOpponent_A;}
static u16 GetTrainerBFlag(void){return TRAINER_FLAGS_START+gTrainerBattleOpponent_B;}
'''
TEST=r'''
int main(void){
    (void)SetBattledTrainersFlags;
    for(unsigned role=0;role<2;++role)for(unsigned partner=0;partner<2;++partner){
        ReadyHost();sCoop.guest=role;
        if(role){sCoop.authenticated=true;sCoop.saveReady=true;sCoop.initialTeamCommitted=true;}
        gMain.callback2=CB2_Overworld;
        for(unsigned round=0;round<3;++round){
            gTrainerBattleOpponent_A=partner?856:855;gTrainerBattleOpponent_B=partner?857:0;
            sSaveBlock1.flags[(TRAINER_FLAGS_START+gTrainerBattleOpponent_A)/8]&=~(1u<<((TRAINER_FLAGS_START+gTrainerBattleOpponent_A)&7));
            gBattleTypeFlags=BATTLE_TYPE_TRAINER|BATTLE_TYPE_DOUBLE;sCoop.battleTrainerId=gTrainerBattleOpponent_A;
            CHECK(BeginLocalBattle(),"begin repeated fixture");
            sRoute101TestPartyActive=true;gBattleOutcome=B_OUTCOME_WON;
            CB2_EndRoute101TestBattle();AcknowledgeBattle();CtrCoop_BattleTick();
            CHECK(!sCoop.battleReleasePending,"victory releases fixture trainer reservation role %u partner %u round %u",role,partner,round);
            CHECK(!sRoute101TestPartyActive && sMainCallback==CB2_ReturnToFieldContinueScriptPlayMapMusic,"fixture resumes exact caller script");
            CHECK(FlagGet(TRAINER_FLAGS_START+gTrainerBattleOpponent_A),"won fixture flag is present before lease release");
        }
    }
    if(sFailures)return 1;
    puts("native test A/B wins release trainer reservation before repeated interaction, both roles passed");return 0;
}
'''
def main():
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=UPSTREAM/'src');p.add_argument('--cc',default='gcc');p.add_argument('--check-mutations',action='store_true');a=p.parse_args()
    source=(a.source/'battle_setup.c').read_text(encoding='utf-8')
    definitions=''.join('#ifndef '+name+'\n#define '+name+' '+value+'\n#endif\n' for name,value in re.findall(r'^#define\s+(B_OUTCOME_\w+)\s+(\d+)\s*$',(a.source.parent/'include/constants/battle.h').read_text(),re.M))
    native=''.join(function(source,n) for n in ['SetBattledTrainersFlags','IsPlayerDefeated','CB2_EndRoute101TestBattle'])
    variants=[('fixed',native,True)]
    if a.check_mutations:variants.append(('missing-win-flags',native.replace('SetBattledTrainersFlags();','(void)0; /* old: no flags */'),False))
    with tempfile.TemporaryDirectory(prefix='coop-repeat-',dir=os.environ.get('TMPDIR')) as d:
        for name,body,wanted in variants:
            c=Path(d)/(name+'.c');exe=c.with_suffix('.exe');c.write_text(PRE+definitions+body+TEST,encoding='utf-8')
            subprocess.run(shlex.split(a.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-DPLATFORM_3DS','-DCTR_COOP_TEST_MAX_SPRINT=6','-I'+str(REPO/'3ds_port/tests/coop_host'),'-I'+str(REPO/'3ds_port/include'),'-iquote'+str(REPO/'3ds_port/tests'),str(c),str(REPO/'3ds_port/src/3ds_coop_protocol.c'),str(REPO/'3ds_port/src/3ds_coop_storage.c'),'-o',str(exe)],check=True,timeout=120)
            r=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            if (r.returncode==0)!=wanted:raise RuntimeError(name+' unexpected result\n'+r.stdout+r.stderr)
            print(r.stdout.strip() if wanted else 'detected mutation: missing fixture win flags',flush=True)
if __name__=='__main__':main()
