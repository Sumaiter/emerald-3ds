"""Build two isolated copies of the real spectator endpoint and join them in a lossy test link."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
API = ('SetState Request Stop Reset Tick IsWatching HasViewer HasFrame CanWatchPeer '
       'HandlePacket InputGate Encode Decode Checksum').split()
MOCKS = {'LocalLink_TrySendLowPriority': 'Send', 'LocalLink_Milliseconds': 'Now',
         'CtrVideo_SpectatorRequestCapture': 'CaptureRequest',
         'CtrVideo_SpectatorTakeCapture': 'CaptureTake',
         'CtrVideo_SpectatorCancelCapture': 'CaptureCancel', 'CtrVideo_SpectatorDisplay': 'Display'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('HOSTCC', 'gcc'))
    args = parser.parse_args()
    cc = shlex.split(args.cc)
    flags = ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / '3ds_port/include')]
    with tempfile.TemporaryDirectory(prefix='spectator-', dir=os.environ.get('TMPDIR')) as folder:
        folder = Path(folder)
        objects = []
        for endpoint in ('A', 'B'):
            names = {'CtrCoopSpectator_' + name: endpoint + name for name in API}
            names.update({name: endpoint + replacement for name, replacement in MOCKS.items()})
            target = folder / (endpoint + '.o')
            subprocess.run(cc + flags + ['-D' + key + '=' + value for key, value in names.items()] +
                           ['-c', str(ROOT / '3ds_port/src/3ds_coop_spectator.c'), '-o', str(target)], check=True)
            objects.append(str(target))
        exe = folder / ('spectator.exe' if os.name == 'nt' else 'spectator')
        subprocess.run(cc + flags + [str(ROOT / '3ds_port/tests/coop_spectator_test.c')] +
                       objects + ['-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
