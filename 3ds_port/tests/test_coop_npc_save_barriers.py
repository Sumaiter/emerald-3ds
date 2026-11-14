"""Probe actual root save/autosave barriers around an accepted NPC end tail."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PROBE = r'''
int main(void)
{
    MotionReadyFixture();
    sCoop.peerEventToken=1;sCoop.peerEventCount=1;
    sCoop.peerEventKeys[0]=(struct EncounterKey){0,10,2,0};
    uint8_t pose[CTR_COOP_NPC_MOTION_SIZE]={76,CTR_COOP_PROTOCOL_VERSION};
    MotionPut32(pose+2,1);MotionPut32(pose+6,1);
    pose[10]=0;pose[11]=10;pose[12]=2;pose[13]=1;
    pose[14]=20;pose[16]=20;pose[18]=0xFF;pose[24]=1;
    uint8_t end[CTR_COOP_NPC_MOTION_END_SIZE]={77,CTR_COOP_PROTOCOL_VERSION};
    MotionPut32(end+2,1);MotionPut32(end+6,2);
    CHECK(CtrCoop_HandlePacket(PEER_ID,pose,sizeof(pose)),"root dispatches accepted pose");
    CHECK(CtrCoop_HandlePacket(PEER_ID,end,sizeof(end)),"root dispatches ordered end");
    CHECK(CtrCoopNpcMotion_Settling(),"end leaves native movement to settle");
    ClearRemote();
    CHECK(CtrCoopNpcMotion_Settling() && sDisconnectSavePending,"disconnect retains tail and requests autosave");
    sCoop.savePrepared=false;ClearPackets();
    PrepareHostSave(PEER_ID);TickDisconnectSave();
    CHECK(!sCoop.savePrepared && !CountPacket(CTR_COOP_SAVE_BEGIN_PACKET),"new guest world cannot freeze unfinished tail");
    CHECK(!sAutoSaves && sDisconnectSavePending,"autosave cannot persist unfinished tail");
    for(unsigned frame=0;frame<12 && CtrCoopNpcMotion_Settling();++frame)CtrCoop_StoryTick();
    CHECK(!CtrCoopNpcMotion_Settling() && gObjectEvents[1].currentCoords.y==20,"accepted tail settles without authenticated peer");
    PrepareHostSave(PEER_ID);TickDisconnectSave();
    CHECK(sCoop.savePrepared && CountPacket(CTR_COOP_SAVE_BEGIN_PACKET)==1,"new guest snapshot resumes after settling");
    CHECK(sAutoSaves==1 && !sDisconnectSavePending,"safe host autosave resumes exactly once");
    printf("NPC end/disconnect root save barriers: %d failures\n",sFailures);
    return sFailures?1:0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    args = parser.parse_args()
    source = (ROOT/'3ds_port/tests/coop_npc_motion_test.c').read_text(encoding='utf-8')
    assert source.count('int main(void)') == 1
    source = source.replace('int main(void)', 'int NpcOriginalMain(void)') + PROBE
    out = ROOT/'build/coop-fixes-20261114'
    out.mkdir(parents=True, exist_ok=True)
    cfile = out/'npc-save-barriers.c';exe = out/'npc-save-barriers.exe'
    cfile.write_text(source, encoding='utf-8')
    subprocess.run(shlex.split(args.cc)+['-std=gnu99','-O2','-Wall','-Wextra','-Werror',
                   '-DCTR_COOP_TEST_MAX_SPRINT=5',
                   '-I'+str(ROOT/'3ds_port/tests/coop_host'),'-I'+str(ROOT/'3ds_port/include'),
                   '-iquote'+str(ROOT/'3ds_port/tests'),str(cfile),
                   str(ROOT/'3ds_port/src/3ds_coop_protocol.c'),str(ROOT/'3ds_port/src/3ds_coop_storage.c'),
                   '-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)


if __name__ == '__main__':
    main()
