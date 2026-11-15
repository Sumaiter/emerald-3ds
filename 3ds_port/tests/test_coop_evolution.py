"""Real owner restore, native evolution loop/Shedinja and authored return chain."""
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from test_coop_bike_hooks import function, UPSTREAM, REPO

PRE = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;
typedef bool bool8;
#define TRUE true
#define FALSE false
#define PARTY_SIZE 6
#define MULTI_PARTY_SIZE 3
#define MAX_FRONTIER_PARTY_SIZE 4
#define DOUBLE_STATE_ACTIVE 5
#define CONTEST_CATEGORIES_COUNT 5
#define MAIL_NONE 255
#define EVO_LEVEL_NINJASK 13
#define EVO_MODE_NORMAL 0
#define FLAG_SET_SEEN 2
#define FLAG_SET_CAUGHT 3
#define LANGUAGE_JAPANESE 1
#define MON_DATA_SPECIES 1
#define MON_DATA_LANGUAGE 2
#define MON_DATA_NICKNAME 3
#define MON_DATA_COOL_RIBBON 10
#define MON_DATA_CHAMPION_RIBBON 20
#define MON_DATA_UNUSED_RIBBONS 30
#define MON_DATA_HELD_ITEM 31
#define MON_DATA_MARKINGS 32
#define MON_DATA_ENCRYPT_SEPARATOR 33
#define MON_DATA_STATUS 34
#define MON_DATA_MAIL 35
#include "constants/battle.h"
#include "constants/battle_setup.h"
#include "constants/battle_frontier.h"
#include "constants/species.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s role%u kind%u count%u\n",__LINE__,#x,role,kind,count);exit(1);}}while(0)
struct Pokemon {u16 species,hp;u8 level,language;u32 experience;u8 bytes[88];};
_Static_assert(sizeof(struct Pokemon)==100,"native wire size");
static struct Pokemon gPlayerParty[6],sDoubleLocalParty[6],sRemoteParty[6],original[6],remoteBefore[6];
static struct {struct Pokemon playerParty[6];u8 playerPartyCount;} save1,*gSaveBlock1Ptr=&save1;
static struct {struct {u8 selectedPartyMons[4];} frontier;} save2,*gSaveBlock2Ptr=&save2;
static struct {bool guest,doublePartyRestored,doubleBorrowedPartner,doubleStoryPartner,doubleInitiator;unsigned doubleState;} sCoop;
static struct {u8 specialTrainerBattleType;} gBattleScripting;
static u8 gPlayerPartyCount,gLeveledUpInBattle;
static bool sRemotePartyValid,sRoute101TestPartyActive,sCoopEvolutionPartyFull,sCoopEvolutionPartySelected;
static unsigned role,kind,count,gBattleOutcome,gBattleTypeFlags,returns,evolutions;
static bool cancelEvolution;
static u32 gBitTable[]={1,2,4,8,16,32};
static void (*gBattlerControllerFuncs[4])(void),(*gCB2_AfterEvolution)(void),(*gBattleMainFunc)(void);
static struct {void (*callback2)(void);} gMain;
static struct {u16 method,targetSpecies;} gEvolutionTable[500][2];
static const u8 gSpeciesNames[500][12]={{0}},sText_ShedinjaJapaneseName[]={0};
static bool gfx;
static void FreeMonSpritesGfx(void){gfx=false;}
static void FreeAllWindowBuffers(void){}
static void ResetSpriteData(void){}
static void BattleControllerDummy(void){}
static void BattleMainCB2(void){}
static void EvolutionBusy(void){}
static void ReturnFromBattleToOverworld(void){++returns;}
static void TryEvolvePokemon(void);
static void WaitForEvoSceneToFinish(void);
static void EvolutionScene(struct Pokemon*,u16,bool,u8);
static bool CtrCoop_IsDoubleBattle(void){return sCoop.doubleState==DOUBLE_STATE_ACTIVE;}
static bool CtrCoop_UsesDoubleParty(void){return CtrCoop_IsDoubleBattle()&&!sCoop.doublePartyRestored;}
static bool CtrCoop_IsDoubleInitiator(void){return sCoop.doubleInitiator;}
static u8 CtrCoop_LocalMultiplayerId(void){return sCoop.guest;}
static u32 GetMonData(const struct Pokemon*p,unsigned field,...){return field==MON_DATA_LANGUAGE?p->language:p->species;}
static void SetMonData(struct Pokemon*p,unsigned field,const void*data){if(field==MON_DATA_SPECIES)memcpy(&p->species,data,2);}
static void ZeroMonData(struct Pokemon*p){memset(p,0,sizeof(*p));}
static void CopyMon(struct Pokemon*to,const struct Pokemon*from,unsigned n){memcpy(to,from,n);}
static void CalculateMonStats(struct Pokemon*p){p->hp=p->species==SPECIES_SHEDINJA?1:50;}
static unsigned SpeciesToNationalPokedexNum(unsigned n){return n;}
static void GetSetPokedexFlag(unsigned s,unsigned op){CHECK(s==SPECIES_SHEDINJA&&(op==FLAG_SET_SEEN||op==FLAG_SET_CAUGHT));}
static u16 GetEvolutionTargetSpecies(struct Pokemon*p,unsigned m,unsigned bits){(void)m;(void)bits;return p->species==SPECIES_NINCADA&&p->level>=20?SPECIES_NINJASK:SPECIES_NONE;}
static void CtrCoop_BattlePartyRestored(void){}
static void CtrCoop_PrepareDoubleReturn(void);
'''

TEST = r'''
static void CtrCoop_PrepareDoubleReturn(void){RestoreDoubleParty();}
static void EvolutionScene(struct Pokemon*p,u16 target,bool battle,u8 index){
    CHECK(battle&&!gfx&&index<gPlayerPartyCount&&p==gPlayerParty+index);
    gfx=true;++evolutions;u16 before=p->species;
    if(!cancelEvolution){p->species=target;CreateShedinja(before,p);}
    gfx=false;gMain.callback2=EvolutionBusy;
}
int main(void){
    gEvolutionTable[SPECIES_NINCADA][0].method=EVO_LEVEL_NINJASK;
    gEvolutionTable[SPECIES_NINCADA][1].targetSpecies=SPECIES_SHEDINJA;
    /* ordinary owner, test A, test B borrowed, and real selected story party */
    for(role=0;role<2;++role)for(kind=0;kind<4;++kind)for(count=1;count<=6;++count)
    for(unsigned invited=0;invited<2;++invited)for(unsigned cancel=0;cancel<2;++cancel){
        memset(&save1,0,sizeof(save1));memset(&save2,0,sizeof(save2));
        memset(original,0,sizeof(original));memset(&sCoop,0,sizeof(sCoop));
        sCoop.guest=role;sCoop.doubleState=DOUBLE_STATE_ACTIVE;sCoop.doubleInitiator=!invited;
        sCoop.doubleStoryPartner=kind>=2;sCoop.doubleBorrowedPartner=kind>=2&&invited;
        for(unsigned i=0;i<count;++i){original[i]=(struct Pokemon){.species=200+i,.hp=12+i,.level=20,.experience=1234+i};memset(original[i].bytes,30+i,sizeof(original[i].bytes));}
        unsigned chosen=kind==3&&count>=5?4:0;original[chosen].species=SPECIES_NINCADA;
        memcpy(save1.playerParty,original,sizeof(original));save1.playerPartyCount=count;
        memcpy(sDoubleLocalParty,original,sizeof(original));
        sRoute101TestPartyActive=kind==1||kind==2;
        gBattleScripting.specialTrainerBattleType=kind>=2?SPECIAL_BATTLE_STEVEN:0;
        /* The wire deliberately removes INGAME_PARTNER; provenance is backend state. */
        gBattleTypeFlags=BATTLE_TYPE_LINK|BATTLE_TYPE_MULTI|BATTLE_TYPE_DOUBLE;
        if(kind==3&&!invited){
            save2.frontier.selectedPartyMons[0]=chosen+1;
            save2.frontier.selectedPartyMons[1]=count>=2?2:0;
            save2.frontier.selectedPartyMons[2]=count>=4?4:0;
            for(unsigned i=0;i<3;++i){unsigned id=save2.frontier.selectedPartyMons[i];sDoubleLocalParty[i]=id?original[id-1]:(struct Pokemon){0};}
        }
        if(kind&&!invited)for(unsigned i=3;i<6;++i)sDoubleLocalParty[i]=(struct Pokemon){.species=kind>=2?SPECIES_ARON:0};
        for(unsigned i=0;i<6;++i)sRemoteParty[i]=(struct Pokemon){.species=300+i,.hp=22+i};
        memcpy(remoteBefore,sRemoteParty,sizeof(sRemoteParty));
        for(unsigned i=0;i<3;++i){
            gPlayerParty[(role?3:0)+i]=sCoop.doubleBorrowedPartner?(struct Pokemon){.species=SPECIES_ARON}:sDoubleLocalParty[i];
            gPlayerParty[(role?0:3)+i]=sRemoteParty[i];
        }
        gPlayerPartyCount=6;gLeveledUpInBattle=role?8:1;gBattleOutcome=B_OUTCOME_WON;
        returns=evolutions=0;gfx=true;cancelEvolution=cancel;
        CtrCoop_BeginBattleEvolutions();
        for(unsigned tick=0;tick<20&&!returns;++tick){gBattleMainFunc();if(gMain.callback2==EvolutionBusy)gMain.callback2=BattleMainCB2;}
        CHECK(returns==1);
        bool eligible=!sCoop.doubleBorrowedPartner;
        CHECK(evolutions==(eligible?1u:0u));
        unsigned gained=eligible&&!cancel&&count<6;
        CHECK(gPlayerPartyCount==count+gained);
        for(unsigned i=0;i<count;++i){struct Pokemon wanted=original[i];if(eligible&&!cancel&&i==chosen)wanted.species=SPECIES_NINJASK;CHECK(!memcmp(gPlayerParty+i,&wanted,sizeof(wanted)));}
        if(gained)CHECK(gPlayerParty[count].species==SPECIES_SHEDINJA&&gPlayerParty[count].hp==1);
        CtrCoop_EngineFinishEvolutionParty();
        if(!invited&&kind==3){SaveSelectedParty();LoadPlayerParty();}
        else if(!invited&&kind){
            /* Exact native fixture callback's first-three write + full load. */
            for(unsigned i=0;i<3;++i)gSaveBlock1Ptr->playerParty[i]=gPlayerParty[i];
            LoadPlayerParty();
        }
        CtrCoop_PrepareDoubleReturn(); /* idempotent, cannot undo evolution/Shedinja */
        CHECK(gPlayerPartyCount==count+gained);
        for(unsigned i=0;i<count;++i){struct Pokemon wanted=original[i];if(eligible&&!cancel&&i==chosen)wanted.species=SPECIES_NINJASK;CHECK(!memcmp(gPlayerParty+i,&wanted,sizeof(wanted)));}
        if(gained)CHECK(gPlayerParty[count].species==SPECIES_SHEDINJA);
        if(!sCoop.doubleStoryPartner||sCoop.doubleBorrowedPartner)CHECK(!memcmp(sRemoteParty,remoteBefore,sizeof(sRemoteParty)));
    }
    puts("native owner restore/evolution/Shedinja/selected-save/load: both roles, 1..6, invited/borrowed, cancel, reserves, story 5/2/4 passed");return 0;
}
'''

def main():
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=UPSTREAM/'src');p.add_argument('--cc',default='gcc');p.add_argument('--check-mutations',action='store_true');a=p.parse_args()
    coop=(REPO/'3ds_port/src/3ds_coop.c').read_text(encoding='utf-8')
    main_src=(a.source/'battle_main.c').read_text(encoding='utf-8')
    setup=(a.source/'battle_setup.c').read_text(encoding='utf-8')
    native=''.join(function(coop,n) for n in ['CtrCoop_IsStoryPartnerBattle','CtrCoop_BattleMonEarnsExp'])
    native+=function((a.source/'pokemon.c').read_text(encoding='utf-8'),'CalculatePlayerPartyCount')
    native+=function(coop,'RestoreDoubleParty')
    native+=''.join(function((a.source/'load_save.c').read_text(encoding='utf-8'),n) for n in ['SavePlayerParty','LoadPlayerParty'])
    native+=''.join(function(setup,n) for n in ['CtrCoop_EnginePrepareEvolutionParty','CtrCoop_EngineFinishEvolutionParty'])
    native+=function((a.source/'evolution_scene.c').read_text(encoding='utf-8'),'CreateShedinja')
    native+=''.join(function(main_src,n) for n in ['CtrCoop_BeginBattleEvolutions','TryEvolvePokemon','WaitForEvoSceneToFinish'])
    native+=function((a.source/'frontier_util.c').read_text(encoding='utf-8'),'SaveSelectedParty')
    variants=[('fixed',native,True)]
    if a.check_mutations:
        variants += [('composite-party',native.replace('    CtrCoop_PrepareDoubleReturn();','    (void)CtrCoop_PrepareDoubleReturn;'),False),('missing-full-owner-restore',native.replace('CtrCoop_EnginePrepareEvolutionParty(ownerLevelBits)','ownerLevelBits'),False),('lost-evolution-after-native-load',native.replace('    SavePlayerParty();','    (void)SavePlayerParty;'),False),('wrong-story-flag',native.replace('!CtrCoop_IsStoryPartnerBattle()','!(gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)'),False)]
    with tempfile.TemporaryDirectory(prefix='coop-evolution-',dir=os.environ.get('TMPDIR')) as d:
        d=Path(d)
        for name,body,wanted in variants:
            c=d/(name+'.c');exe=c.with_suffix('.exe');c.write_text(PRE+body+TEST,encoding='utf-8')
            subprocess.run(shlex.split(a.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-DPLATFORM_3DS','-iquote'+str(a.source.parent/'include'),str(c),'-o',str(exe)],check=True,timeout=120)
            r=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            if (r.returncode==0)!=wanted:raise RuntimeError(name+' unexpected result\n'+r.stdout+r.stderr)
            print(r.stdout.strip() if wanted else 'detected mutation: '+name,flush=True)
if __name__=='__main__':main()
