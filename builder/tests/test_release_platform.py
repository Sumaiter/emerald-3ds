"""Platform-label and audit regression for the optional macOS package."""
import contextlib
import io
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
import build_release
import release_audit
import voxel_precompile


class ReleasePlatformTests(unittest.TestCase):
    def invoke(self, *options):
        argv = ["build_release.py", "--version", "0.5.0", "--rom", "synthetic.gba",
                "--gba-elf", "synthetic.elf", *options]
        with patch.object(sys, "argv", argv):
            build_release.main()

    def test_wrong_host_and_linux_target_are_rejected_before_build(self):
        for host, options in (("darwin", ()), ("win32", ("--platform", "macOS")),
                              ("linux", ("--platform", "Linux"))):
            with self.subTest(host=host, options=options), patch.object(sys, "platform", host), \
                    contextlib.redirect_stderr(io.StringIO()), \
                    patch.object(build_release, "run") as run:
                with self.assertRaises(SystemExit) as ctx:
                    self.invoke(*options)
                self.assertEqual(ctx.exception.code, 2)
                run.assert_not_called()

    def test_matching_host_or_explicit_no_executable_reaches_normal_checks(self):
        class ReachedPythonCheck(Exception):
            pass
        for host, options in (("win32", ()), ("darwin", ("--platform", "macOS")),
                              ("linux", ("--platform", "macOS", "--skip-exe"))):
            with self.subTest(host=host, options=options), patch.object(sys, "platform", host), \
                    patch.object(voxel_precompile, "check_python", side_effect=ReachedPythonCheck):
                with self.assertRaises(ReachedPythonCheck):
                    self.invoke(*options)

    def test_mac_executables_and_runtime_pass_real_size_audit(self):
        allow = release_audit.Allow.load(release_audit.DEFAULT_ALLOW)
        prefix = "Emerald3DS-v0.5.0-macOS/"
        names = [prefix + name for name in ("Emerald3DS-Builder", "emerald3ds-builder-cli",
                 "payload/Emerald3DS.3dsx", "payload/de/Emerald3DS.3dsx", "_internal/runtime.bin")]
        data = b"synthetic harmless size probe\n" * 40000
        entries = [(name, lambda: data) for name in names]
        self.assertEqual(release_audit.audit(entries, allow, [], None, "zip"), [])
        # The exception must not silently permit arbitrary oversized payloads.
        unexpected = [(prefix + "unlisted.bin", lambda: data)]
        self.assertTrue(any(f.rule == "too-large" for f in
                            release_audit.audit(unexpected, allow, [], None, "zip")))


if __name__ == "__main__":
    unittest.main()
