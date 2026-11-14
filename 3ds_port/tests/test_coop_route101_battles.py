"""Execute the repeatable Route 101 fixtures' real setup/return bodies."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess
import tempfile
from test_coop_bike_hooks import UPSTREAM, function

HEADER = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef bool bool8;
#define TRUE true
#define FALSE false
#define PARTY_SIZE 6
#define MULTI_PARTY_SIZE 3
#define TRAINER_FLAGS_START 0x500
#define MON_DATA_SPECIES 1
#define MON_DATA_IS_EGG 2
#define MON_DATA_HP 3
#define MON_DATA_OT_NAME 4
#define OT_ID_PRESET 1
#include "constants/species.h"
#include "constants/opponents.h"
#include "constants/trainers.h"
#include "constants/battle.h"
#include "constants/battle_frontier.h"
struct Pokemon {unsigned species,hp,egg,level,identity,status,pp,mail;};
static struct Pokemon gPlayerParty[6];
static u8 gPlayerPartyCount;
static struct {struct Pokemon playerParty[6]; u8 playerPartyCount;} save, *gSaveBlock1Ptr=&save;
static struct {void (*savedCallback)(void);} gMain;
static struct {unsigned specialTrainerBattleType;} gBattleScripting;
static struct {u8 trainerName[12];} gTrainers[TRAINERS_COUNT];
static u32 gBattleTypeFlags,gBattleOutcome;
static u16 gSpecialVar_0x8004,gTrainerBattleOpponent_A,gTrainerBattleOpponent_B,gPartnerTrainerId;
static u8 gApproachingTrainerId,sNoOfPossibleTrainerRetScripts,gNoOfApproachingTrainers,gWhichTrainerToFaceAfterBattle;
static bool sShouldCheckTrainerBScript;
static u8 *sTrainerADefeatSpeech,*sTrainerBDefeatSpeech;
static const u8 sRoute101TestDefeated[]={0xFF};
static unsigned clears,clearIds[4],starts,ends,restores,locked,frozen,stopped,inits;
static void (*callback)(void);
static unsigned GetMonData(const struct Pokemon *p,unsigned field){
    switch(field){case MON_DATA_SPECIES:return p->species;case MON_DATA_IS_EGG:return p->egg;case MON_DATA_HP:return p->hp;}assert(0);return 0;
}
static void InitTrainerBattleVariables(void){++inits;}
static void FlagClear(unsigned flag){assert(clears<4);clearIds[clears++]=flag;}
static void CreateMon(struct Pokemon *p,unsigned species,unsigned level,unsigned iv,bool fixed,unsigned personality,unsigned ot,unsigned id){
    (void)iv;(void)fixed;(void)personality;assert(ot==OT_ID_PRESET && id==61226);
    *p=(struct Pokemon){.species=species,.hp=20,.level=level,.identity=species};
}
static void SetMonData(struct Pokemon *p,unsigned field,const void *value){(void)p;assert(field==MON_DATA_OT_NAME);assert(value==gTrainers[TRAINER_STEVEN].trainerName);}
static void LockPlayerFieldControls(void){++locked;}
static void FreezeObjectEvents(void){++frozen;}
static void StopPlayerAvatar(void){++stopped;}
static void DoTrainerBattle(void){++starts;}
static void CtrCoop_BattlePartyRestored(void){++restores;}
static void CtrCoop_BattleEnd(void){++ends;assert(!memcmp(&gPlayerParty[3],&save.playerParty[3],sizeof(struct Pokemon)*3));}
static bool IsPlayerDefeated(unsigned outcome){return outcome==B_OUTCOME_LOST || outcome==B_OUTCOME_DREW;}
static void CB2_WhiteOut(void){}
static void CB2_ReturnToFieldContinueScriptPlayMapMusic(void){}
static void SetMainCallback2(void (*fn)(void)){callback=fn;}
'''

TEST = r'''
static void reset(void){
    memset(gPlayerParty,0,sizeof(gPlayerParty));memset(&save,0,sizeof(save));
    for(unsigned i=0;i<6;++i)gPlayerParty[i]=(struct Pokemon){.species=SPECIES_MUDKIP+i,.hp=17+i,.level=5,.identity=1234+i,.status=i,.pp=4+i,.mail=50+i};
    gPlayerPartyCount=6;gBattleTypeFlags=0;gBattleOutcome=B_OUTCOME_WON;
    clears=starts=ends=restores=locked=frozen=stopped=inits=0;callback=NULL;
}
int main(void){
    struct Pokemon original[6];
    reset();gSpecialVar_0x8004=2;assert(!Route101_StartTestBattle());assert(!starts && !clears && !locked);
    /* Reject fainted/egg teams before changing trainer flags, callbacks or RAM backups. */
    reset();gSpecialVar_0x8004=0;gPlayerParty[1].hp=0;gPlayerParty[2].egg=1;
    memcpy(original,gPlayerParty,sizeof(original));assert(!Route101_StartTestBattle());
    assert(!memcmp(original,gPlayerParty,sizeof(original)) && !starts && !clears && !locked);
    reset();gSpecialVar_0x8004=1;for(unsigned i=0;i<3;++i)gPlayerParty[i].hp=0;
    assert(!Route101_StartTestBattle());assert(!starts && !clears && !locked);
    for(unsigned outcome=B_OUTCOME_WON;outcome<=B_OUTCOME_DREW;++outcome){
        reset();gSpecialVar_0x8004=0;memcpy(original,gPlayerParty,sizeof(original));
        assert(Route101_StartTestBattle());assert(starts==1 && inits==2 && locked==1 && frozen==1 && stopped==1);
        assert(gBattleTypeFlags==(BATTLE_TYPE_TRAINER|BATTLE_TYPE_DOUBLE));
        assert(gTrainerBattleOpponent_A==TRAINER_ROUTE101_TEST_DOUBLE && gTrainerBattleOpponent_B==TRAINER_NONE);
        assert(!gPartnerTrainerId && !memcmp(original,gPlayerParty,sizeof(original)));
        assert(clears==1 && clearIds[0]==TRAINER_FLAGS_START+TRAINER_ROUTE101_TEST_DOUBLE);
        /* Both normal return and real whiteout callback stay in place. */
        memcpy(save.playerParty,gPlayerParty,sizeof(gPlayerParty));gBattleOutcome=outcome;
        gMain.savedCallback();assert(callback==(outcome==B_OUTCOME_WON?CB2_ReturnToFieldContinueScriptPlayMapMusic:CB2_WhiteOut));
        assert(!restores && ends==(outcome==B_OUTCOME_WON));
    }
    /* Full team and an early-game one-Pokemon team, including gaps before NPC slots. */
    for(unsigned count=1;count<=6;count+=5)for(unsigned outcome=B_OUTCOME_WON;outcome<=B_OUTCOME_DREW;++outcome){
        reset();gPlayerPartyCount=count;
        for(unsigned i=count;i<6;++i)memset(&gPlayerParty[i],0,sizeof(gPlayerParty[i]));
        memcpy(original,gPlayerParty,sizeof(original));gSpecialVar_0x8004=1;
        assert(Route101_StartTestBattle());assert(save.playerPartyCount==count && !memcmp(save.playerParty,original,sizeof(original)));
        assert(gPlayerPartyCount==6 && starts==1 && clears==2);
        assert(clearIds[0]==TRAINER_FLAGS_START+TRAINER_ROUTE101_TEST_PARTNER_A && clearIds[1]==TRAINER_FLAGS_START+TRAINER_ROUTE101_TEST_PARTNER_B);
        assert(gPartnerTrainerId==TRAINER_STEVEN_PARTNER);
        assert(gBattleTypeFlags==(BATTLE_TYPE_TRAINER|BATTLE_TYPE_DOUBLE|BATTLE_TYPE_MULTI|BATTLE_TYPE_TWO_OPPONENTS|BATTLE_TYPE_INGAME_PARTNER));
        assert(gPlayerParty[3].species==SPECIES_ARON && gPlayerParty[4].species==SPECIES_BALTOY && gPlayerParty[5].species==SPECIES_BELDUM);
        for(unsigned i=3;i<6;++i)assert(gPlayerParty[i].level==5);
        gPlayerParty[0].hp=1;gPlayerParty[0].status=8;gPlayerParty[0].pp=2;
        struct Pokemon changed=gPlayerParty[0];gBattleOutcome=outcome;
        gMain.savedCallback();
        assert(restores==1 && ends==(outcome==B_OUTCOME_WON) && gPlayerPartyCount==count);
        assert(!memcmp(&changed,&gPlayerParty[0],sizeof(changed)));
        assert(!memcmp(&original[3],&gPlayerParty[3],sizeof(struct Pokemon)*3));
        assert(callback==(outcome==B_OUTCOME_WON?CB2_ReturnToFieldContinueScriptPlayMapMusic:CB2_WhiteOut));
        /* A second encounter backs up the now-current real team, not the borrowed triplet. */
        clears=0;assert(Route101_StartTestBattle());gMain.savedCallback();
        assert(!memcmp(&original[3],&gPlayerParty[3],sizeof(struct Pokemon)*3));
    }
    puts("Route101 setup/guards/repeat/win/loss/draw: real RAM backup+restore, combat damage and all reserves preserved");
    return 0;
}
'''


def check_map_and_scripts(up):
    data = json.loads((up/'data/maps/Route101/map.json').read_text())
    fixtures = data['object_events'][-2:]
    assert len(data['object_events']) == 8
    assert [o['script'] for o in fixtures] == ['Route101_EventScript_TestDouble', 'Route101_EventScript_TestPartner']
    assert [o['graphics_id'] for o in fixtures] == ['OBJ_EVENT_GFX_YOUNGSTER', 'OBJ_EVENT_GFX_STEVEN']
    layouts = json.loads((up/'data/layouts/layouts.json').read_text())
    layout = next(x for x in layouts['layouts'] if x['id'] == 'LAYOUT_ROUTE101')
    raw = (up/layout['blockdata_filepath']).read_bytes()
    tiles = struct.unpack('<'+'H'*(len(raw)//2), raw)
    w, h = layout['width'], layout['height']
    for o in fixtures:
        assert o['trainer_type'] == 'TRAINER_TYPE_NONE' and o['flag'] == '0'
        x, y = o['x'], o['y']
        assert 0 < x < w-1 and 0 < y < h-1
        assert not ((tiles[y*w+x] >> 10) & 3)
        assert not ((tiles[(y+1)*w+x] >> 10) & 3), 'NPC must be approachable from below'
    script = (up/'data/maps/Route101/scripts.inc').read_text(encoding='utf-8')
    section = script[script.index('@ Repeatable fixtures:'):].split('Route101_MapScripts::')[0]
    assert not re.search(r'\b(setflag|clearflag|settrainerflag|cleartrainerflag|givepokemon|giveitem)\b', section)
    assert 'setvar VAR_0x8004, 0' in section and 'setvar VAR_0x8004, 1' in section
    assert re.search(r'specialvar VAR_RESULT, Route101_StartTestBattle\s+goto_if_eq VAR_RESULT, FALSE, \w+\s+waitstate', section)
    trainer_text = (up/'src/data/trainers.h').read_text(encoding='utf-8')
    party_text = (up/'src/data/trainer_parties.h').read_text(encoding='utf-8')
    for ident, count in [('Double', 4), ('PartnerA', 3), ('PartnerB', 3)]:
        match = re.search(r'sParty_Route101Test'+ident+r'\[\] = \{(.*?)\n\};', party_text, re.S)
        species = re.findall(r'\.species = (SPECIES_\w+)', match[1])
        assert len(species) == len(set(species)) == count
        assert re.findall(r'\.lvl = (\d+)', match[1]) == ['5']*count
        assert 'NO_ITEM_DEFAULT_MOVES(sParty_Route101Test'+ident+')' in trainer_text
    specials = (up/'data/specials.inc').read_text()
    assert specials.count('def_special Route101_StartTestBattle') == 1
    print('Route101: accessible optional NPCs, script resume, isolated flags and diverse Lv5 teams passed')


def check_map_generator(up):
    helper = Path(__file__).resolve().parents[2]/'tools/port_common/gen_missing_map_includes.py'
    spec = importlib.util.spec_from_file_location('map_includes', helper)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    with tempfile.TemporaryDirectory(prefix='route101-mapgen-') as temp:
        maps = Path(temp)/'maps'
        folder = maps/'Route101'
        folder.mkdir(parents=True)
        source = folder/'map.json'
        shutil.copyfile(up/'data/maps/Route101/map.json', source)
        module.MAPS_DIR = maps
        module.LAYOUTS_JSON = up/'data/layouts/layouts.json'
        module.MAPJSON = up/'tools/mapjson'/('mapjson.exe' if os.name == 'nt' else 'mapjson')
        module.main()
        events = folder/'events.inc'
        assert events.read_text().count('object_event ') == 8
        assert 'Route101_EventScript_TestDouble' in events.read_text()
        timestamp = events.stat().st_mtime_ns
        module.main()
        assert events.stat().st_mtime_ns == timestamp, 'unchanged maps should remain untouched'
        data = json.loads(source.read_text())
        data['object_events'][-2]['x'] = 9
        source.write_text(json.dumps(data), encoding='utf-8')
        # NTFS rounds nanoseconds to 100ns; use distinct past timestamps.
        for name in ('header.inc', 'events.inc', 'connections.inc'):
            os.utime(folder/name, ns=(timestamp-2_000_000_000, timestamp-2_000_000_000))
        os.utime(source, ns=(timestamp-1_000_000_000, timestamp-1_000_000_000))
        module.main()
        assert 'OBJ_EVENT_GFX_YOUNGSTER, 9, 3, 3' in events.read_text(), 'changed JSON must replace old includes'
        (folder/'header.inc').unlink()
        module.main()
        assert (folder/'header.inc').is_file()
    print('mapjson: fresh/missing/unchanged/edited map includes passed with the real compiler')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, default=UPSTREAM/'src')
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    parser.add_argument('--built', action='store_true', help='also check the actual generated map includes')
    args = parser.parse_args()
    up = args.source.parent
    check_map_and_scripts(up)
    check_map_generator(up)
    if args.built:
        events = (up/'data/maps/Route101/events.inc').read_text()
        assert events.count('object_event ') == 8
        assert 'object_event 7, OBJ_EVENT_GFX_YOUNGSTER, 8, 3, 3' in events
        assert 'object_event 8, OBJ_EVENT_GFX_STEVEN, 12, 3, 3' in events
        print('Built Route101 events contain both accessible test NPCs (local IDs 7/8)')
    source = (args.source/'battle_setup.c').read_text(encoding='utf-8')
    load = (args.source/'load_save.c').read_text(encoding='utf-8')
    bodies = ''.join(function(load, n) for n in ['SavePlayerParty', 'LoadPlayerParty'])
    bodies += function(source, 'CB2_EndRoute101TestBattle')+function(source, 'Route101_StartTestBattle')
    with tempfile.TemporaryDirectory(prefix='route101-battles-') as temp:
        c = Path(temp)/'test.c'
        exe = Path(temp)/('test.exe' if os.name == 'nt' else 'test')
        c.write_text(HEADER+bodies+TEST, encoding='utf-8')
        subprocess.run(shlex.split(args.cc)+['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror', '-DPLATFORM_3DS', '-iquote', str(up/'include'), str(c), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
