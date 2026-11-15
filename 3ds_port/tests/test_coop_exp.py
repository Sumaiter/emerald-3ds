"""Compile actual EXP script, wire and player-controller bodies at O2/Werror."""
import argparse,os,shlex,subprocess,tempfile
from pathlib import Path
from test_coop_bike_hooks import function,UPSTREAM,REPO
def main():
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=UPSTREAM/'src');p.add_argument('--cc',default='gcc');p.add_argument('--check-mutations',action='store_true');a=p.parse_args()
    cmds=(a.source/'battle_script_commands.c').read_text(encoding='utf-8');player=(a.source/'battle_controller_player.c').read_text(encoding='utf-8');control=(a.source/'battle_controllers.c').read_text(encoding='utf-8')
    native=''.join(function(control,n) for n in ['BtlController_EmitExpUpdate','CtrCoop_EmitPartyProgress','CtrCoop_ApplyPartyProgress','CtrCoop_EmitLearnMove'])+''.join(function(player,n) for n in ['CompleteOnInactiveTextPrinter','Task_GiveExpToMon','Task_LaunchLvlUpAnim','Task_UpdateLvlInHealthbox','DestroyExpTaskAndCompleteOnInactiveTextPrinter','PlayerHandleExpUpdate'])+function(cmds,'Cmd_getexp')+'static u8 sCoopLearnMoveState;\n'+''.join(function(player,n) for n in ['CoopChooseLearnMove','PlayerHandleYesNoBox'])+function(cmds,'CoopLearnMove')
    native=function(cmds,'CoopIsTradedExpMon')+native
    main_source=(a.source/'battle_main.c').read_text(encoding='utf-8')
    native+=''.join(function(main_source,n) for n in ['CtrCoop_BeginBattleEvolutions','TryEvolvePokemon','WaitForEvoSceneToFinish'])
    variants=[('fixed',native,True)]
    if a.check_mutations:
        variants.extend([('old-link-skip',native.replace('&& !CtrCoop_IsDoubleBattle()','',1),False),('double-exp-owner-reply',native.replace('if (!CtrCoop_IsDoubleBattle() || GetBattlerMultiplayerId(battler) == GetMultiplayerId())','if (TRUE)',1),False),('no-progress-mirror',native.replace('memcpy(mon, gBattleBufferA[gActiveBattler] + 4, sizeof(*mon));','/* missing final synchronization */'),False)])
        variants.append(('evolution-graphics-leak',native.replace('    FreeMonSpritesGfx();','    (void)FreeMonSpritesGfx;'),False))
    with tempfile.TemporaryDirectory(prefix='coop-exp-',dir=os.environ.get('TMPDIR')) as d:
        d=Path(d)
        pokemon=(a.source.parent/'include/pokemon.h').read_text()
        (d/'exp_mon_fields.inc').write_text(pokemon[pokemon.index('enum {'):pokemon.index('struct PokemonSubstruct0')])
        for name,body,wanted in variants:
            (d/'exp_native.inc').write_text(body,encoding='utf-8');exe=d/(name+'.exe')
            subprocess.run(shlex.split(a.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-DPLATFORM_3DS','-I'+str(d),'-iquote'+str(a.source.parent/'include'),str(REPO/'3ds_port/tests/coop_exp_native_test.c'),'-o',str(exe)],check=True,timeout=120)
            r=subprocess.run([str(exe)],capture_output=True,text=True,timeout=15)
            if (r.returncode==0)!=wanted:raise RuntimeError(name+' unexpected result\n'+r.stdout+r.stderr)
            print(r.stdout.strip() if wanted else 'detected mutation: '+name,flush=True)
if __name__=='__main__':main()
