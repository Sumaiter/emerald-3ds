"""Reverse/apply patch 0097 in an isolated copy of the patched engine."""
from pathlib import Path
import shutil, subprocess, tempfile
from test_coop_bike_hooks import UPSTREAM, REPO
root=REPO
git=shutil.which('git')
if not git: raise SystemExit('git must be available on PATH for the presentation patch roundtrip')
with tempfile.TemporaryDirectory(prefix='present-patch-') as d:
    d=Path(d);(d/'src').mkdir()
    current=(UPSTREAM/'src/fieldmap.c').read_text(encoding='utf-8')
    (d/'src/fieldmap.c').write_text(current,encoding='utf-8',newline='\n')
    patch=str(root/'patches/pokeemerald/0097-coop-battle-presentation.patch')
    subprocess.run([git,'apply','--check','--reverse',patch],cwd=d,check=True)
    subprocess.run([git,'apply','--reverse',patch],cwd=d,check=True)
    # Optional local provenance check; not required for a clean CI checkout.
    baseline=root/'build/coop-nextwave/spectator-baseline/engine-fieldmap.c'
    if baseline.exists(): assert (d/'src/fieldmap.c').read_text(encoding='utf-8')==baseline.read_text(encoding='utf-8')
    subprocess.run([git,'apply','--check',patch],cwd=d,check=True)
    subprocess.run([git,'apply',patch],cwd=d,check=True)
    assert (d/'src/fieldmap.c').read_text(encoding='utf-8')==current
print('Patch 0097 isolated reverse/forward roundtrip passed; no saved baseline dependency')
