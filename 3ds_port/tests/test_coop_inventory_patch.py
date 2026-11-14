"""Produce patch 0090 from the saved, untouched per-source baselines."""
from pathlib import Path
import difflib
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCES = ("item.c", "item_use.c", "party_menu.c", "item_menu.c", "player_pc.c",
           "pokemon_storage_system.c", "berry_blender.c", "lilycove_lady.c", "battle_controller_player.c")


def main():
    chunks = []
    for name in SOURCES:
        before = (ROOT / "build/coop-live/baseline" / name).read_text(encoding="utf-8").splitlines(keepends=True)
        after = (ROOT / "build/upstream/src" / name).read_text(encoding="utf-8").splitlines(keepends=True)
        diff = "".join(difflib.unified_diff(before, after, fromfile=f"a/src/{name}", tofile=f"b/src/{name}"))
        if diff:
            chunks.append(f"diff --git a/src/{name} b/src/{name}\n" + diff)
    patch = ROOT / "patches/pokeemerald/0090-coop-live-inventory.patch"
    patch.write_text("".join(chunks), encoding="utf-8", newline="\n")
    subprocess.run(["git", "apply", "--check", "--reverse", "--directory=build/upstream", str(patch)], cwd=ROOT, check=True)
    print(f"inventory engine patch: {len(chunks)} source files; reverse application validated")


if __name__ == "__main__":
    main()
