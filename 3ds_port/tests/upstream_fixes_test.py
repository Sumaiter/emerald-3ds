"""Run the imported Fly/visible-IV production bodies without emulator input."""
import argparse
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from test_coop_bike_hooks import function, UPSTREAM

PORT = Path(__file__).resolve().parents[1]


def run(cc, name, source):
    with tempfile.TemporaryDirectory(prefix=name) as tmp:
        path = Path(tmp) / "test.c"
        exe = Path(tmp) / "test.exe"
        path.write_text(source, encoding="utf-8")
        subprocess.run([*shlex.split(cc), "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                        str(path), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


FLY = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
typedef uint8_t u8; typedef uint16_t u16;
#define DPAD_RIGHT 1
#define DPAD_LEFT 2
#define DPAD_UP 4
#define DPAD_DOWN 8
#define MAP_PIC_W 64
#define MAP_PIC_H 48
#define MAPSEC_NONE 0xffff
static u8 sPickX, sPickY, sPickPos, sFlyDpadFrames;
static u16 sPickMapsec, cells[6][8];
static bool sMapZoom;
static unsigned zooms;
static u16 GetRegionMapSecIdAt(int x, int y) {
    assert(x>=0 && x<8 && y>=0 && y<6); return cells[y][x];
}
static u8 CtrRegionMap_PosWithinMapSec(int x, int y, u16 sec) { (void)sec; return x+y; }
static int PickPanelWidth(u16 sec, u8 pos) { (void)sec; return pos+1; }
static void MapZoomTo(int x, int y, int width) {
    assert(x==sPickX && y==sPickY && width==sPickPos+1); zooms++;
}
'''

FLY_MAIN = r'''
int main(void) {
    for(int y=0;y<6;y++) for(int x=0;x<8;x++) cells[y][x]=MAPSEC_NONE;
    sPickX=1;sPickY=1;sPickMapsec=1;cells[1][1]=1;cells[1][4]=7;
    FlyMapDpad(DPAD_RIGHT,DPAD_RIGHT);
    assert(sPickX==4 && sPickY==1 && sPickMapsec==7 && zooms==0);
    StepPickCell(DPAD_RIGHT); /* no next land: retain complete selection */
    assert(sPickX==4 && sPickY==1 && sPickMapsec==7);
    sMapZoom=true; cells[4][7]=9; StepPickCell(DPAD_RIGHT|DPAD_DOWN);
    assert(sPickX==7 && sPickY==4 && sPickMapsec==9 && zooms==1);
    StepPickCell(DPAD_RIGHT);assert(zooms==1);
    StepPickCell(DPAD_LEFT|DPAD_UP);assert(sPickX==4 && sPickY==1 && zooms==2);
    StepPickCell(DPAD_UP);assert(sPickY==1);
    for(int x=0;x<8;x++)cells[1][x]=x+1;
    sPickX=0;sPickY=1;sFlyDpadFrames=0;sMapZoom=false;
    FlyMapDpad(DPAD_RIGHT,DPAD_RIGHT);assert(sPickX==1);
    for(int i=0;i<15;i++)FlyMapDpad(DPAD_RIGHT,0);
    assert(sPickX==1);FlyMapDpad(DPAD_RIGHT,0);assert(sPickX==2);
    for(int i=0;i<3;i++)FlyMapDpad(DPAD_RIGHT,0);
    assert(sPickX==2);FlyMapDpad(DPAD_RIGHT,0);assert(sPickX==3);
    FlyMapDpad(0,0);assert(sFlyDpadFrames==0);
    FlyMapDpad(DPAD_LEFT,DPAD_LEFT);assert(sPickX==2 && sFlyDpadFrames==0);
    puts("PASS imported Fly bodies: gaps, edges, diagonal, zoom, initial delay, repeat, release");
}
'''

VISIBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;
#define TRUE 1
#define USE_RANDOM_IVS 32
#define OT_ID_PLAYER_ID 0
#define MAX_IV_MASK 31
#define CTR_LOG_GAME 1
enum {MON_DATA_HP_IV,MON_DATA_ATK_IV,MON_DATA_DEF_IV,MON_DATA_SPEED_IV,MON_DATA_SPATK_IV,MON_DATA_SPDEF_IV};
struct Pokemon {u32 personality;u16 species;u8 level,iv[6];};
struct VisibleWild {u32 personality;u16 ivs[2],species;u8 level;};
static struct Pokemon gEnemyParty[6];
static u32 rng=123,reads,stats,battles,removed,created;
static u16 Random(void) {rng=rng*1103515245u+24691u;reads++;return rng>>16;}
static u32 ShinyOdds_WildPersonality(void) {u32 lo=Random();return lo|((u32)Random()<<16);}
static void SetMonData(struct Pokemon *mon,u8 field,const void *value) {assert(field<6);mon->iv[field]=*(const u32*)value;}
static void CalculateMonStats(struct Pokemon *mon) {(void)mon;stats++;}
static void ZeroEnemyPartyMons(void) {memset(gEnemyParty,0,sizeof(gEnemyParty));}
static void CreateMon(struct Pokemon *mon,u16 species,u8 level,u8 iv,bool fixed,u32 pid,u8 ot,u32 id) {
    assert(iv==USE_RANDOM_IVS && fixed && ot==OT_ID_PLAYER_ID && id==0);
    mon->species=species;mon->level=level;mon->personality=pid;
    for(unsigned k=0;k<6;k++)mon->iv[k]=Random()&31;
    created++;
}
static void CtrLog_Write(unsigned cat,const char *fmt,...) {(void)cat;(void)fmt;}
static void VisibleRemove(struct VisibleWild *v) {(void)v;assert(stats==created);removed++;}
static void BattleSetup_StartWildBattle(void) {assert(removed==created);battles++;}
'''

VISIBLE_MAIN = r'''
int main(void) {
    for(unsigned trial=0;trial<256;trial++) {
        struct VisibleWild v={0};u16 words[2];u32 before=reads;
        v.species=1+trial;v.level=5+(trial%50);
        v.personality=VisibleRollPersonality();VisibleRollIvs(&v);
        assert(reads==before+4);words[0]=v.ivs[0];words[1]=v.ivs[1];
        /* Time and walking RNG between spawn and contact must not reroll IVs. */
        for(unsigned i=0;i<trial;i++)Random();
        VisibleStartBattle(&v);
        assert(gEnemyParty[0].personality==v.personality);
        assert(gEnemyParty[0].species==v.species && gEnemyParty[0].level==v.level);
        for(unsigned i=0;i<2;i++)for(unsigned j=0;j<3;j++)
            assert(gEnemyParty[0].iv[i*3+j]==((words[i]>>(j*5))&31));
        assert(stats==trial+1 && battles==trial+1);
    }
    puts("PASS visible wild production bodies: consecutive rolls, deferred contact, all six IVs, stats before battle");
}
'''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cc", default="cc")
    ap.add_argument("--source-tree", type=Path, default=UPSTREAM)
    args = ap.parse_args()
    ui = (PORT / "src/3ds_bottom_ui.c").read_text(encoding="utf-8")
    inc = (PORT / "src/3ds_bottom_field.inc").read_text(encoding="utf-8")
    opened = function(ui, "CtrBottom_OpenFlyMap")
    assert "sFlyDpadFrames = 0;" in opened
    defines = "\n".join(re.findall(r"^#define FLY_DPAD_(?:DELAY|REPEAT) \d+", ui, re.M))
    run(args.cc, "fly-import-", FLY + defines + "\n" + function(inc, "StepPickCell")
        + function(ui, "FlyMapDpad") + FLY_MAIN)
    wild = (args.source_tree / "src/wild_encounter.c").read_text(encoding="utf-8")
    assert re.search(r"v->personality = VisibleRollPersonality\(\);\s*VisibleRollIvs\(v\);", wild)
    run(args.cc, "visible-import-", VISIBLE.replace("#include <stdio.h>", "#include <stdio.h>\n#include <stdbool.h>")
        + "\n".join(function(wild, name) for name in
                     ("VisibleRollPersonality", "VisibleRollIvs", "VisibleSetIvs", "VisibleStartBattle"))
        + VISIBLE_MAIN)


if __name__ == "__main__":
    main()
