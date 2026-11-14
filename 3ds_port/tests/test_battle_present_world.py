"""Execute actual private-world, read-only map export and worker retirement bodies."""
import argparse, os, shlex, subprocess, tempfile
from pathlib import Path
from test_coop_bike_hooks import function, UPSTREAM
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--cc',default='gcc');a=p.parse_args();cc=shlex.split(a.cc)
 world=(ROOT/'3ds_port/src/voxel/voxel_world.c').read_text(encoding='utf-8')
 engine=(UPSTREAM/'src/fieldmap.c').read_text(encoding='utf-8')
 with tempfile.TemporaryDirectory(prefix='present-world-',dir=os.environ.get('TMPDIR')) as d:
  d=Path(d)
  initial=world[world.index('static VoxelMapInstance sInstances'):world.index('/*\n * Memo for')]
  placed=world[world.index('static VoxelMapInstance sPlaced'):world.index('static VoxelMapInstance sPlaced')+world[world.index('static VoxelMapInstance sPlaced'):].index('#endif')+6]
  names=['FillInstance','VoxelWorld_BuildInstances','VoxelWorld_InstanceCount','VoxelWorld_Instance','VoxelWorld_GetInstanceAt','GetRawBlock','VoxelWorld_GetMetatileId','VoxelWorld_BorderMetatile','VoxelWorld_PresentMatches','VoxelWorld_LiveDigest']
  (d/'present_world.inc').write_text(initial+placed+'\n'+''.join(function(world,n) for n in names)+function(engine,'CtrBattlePresent_CaptureBorders')+function(world,'VoxelWorld_PresentCapture'))
  flags=['-O2','-std=gnu99','-Wall','-Wextra','-Werror','-DCTR_BATTLE_PRESENTATION','-DPLATFORM_3DS','-I'+str(ROOT/'3ds_port/include'),'-I'+str(ROOT/'3ds_port/src'),'-I'+str(d)]
  exe=d/'world.exe';subprocess.run(cc+flags+[str(ROOT/'3ds_port/tests/battle_present_world_test.c'),str(ROOT/'3ds_port/src/3ds_battle_present.c'),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
  voxel=(ROOT/'3ds_port/src/voxel/ctr_voxel.c').read_text(encoding='utf-8')
  (d/'present_fence.inc').write_text(''.join(function(voxel,n) for n in ['StreamStop','AtlasJobCancel','JobCancel','CtrVoxel_ParkVram']))
  exe=d/'fence.exe';subprocess.run(cc+flags+[str(ROOT/'3ds_port/tests/battle_present_fence_test.c'),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
if __name__=='__main__':main()
