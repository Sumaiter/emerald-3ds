"""Check the review gallery against real engine timing, flips and asset coverage."""
import unittest
from pathlib import Path

from build_character_gallery import ROOT, Engine, animation, build


class CharacterGalleryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.engine = Engine(ROOT / "build/upstream")

    def test_native_custom_walk_keeps_flip_and_timing(self):
        normal = animation(self.engine.anims, "sAnim_GoSouth")
        special = animation(self.engine.anims, "sAnim_QuintyPlumpGoSouth")
        self.assertEqual([c["index"] for c in normal["frames"]], [3, 0, 4, 0])
        self.assertEqual([c["duration"] for c in normal["frames"]], [8] * 4)
        self.assertEqual([c["index"] for c in special["frames"]], [3, 0, 3, 0])
        self.assertEqual([c["hflip"] for c in special["frames"]], [False, False, True, False])
        self.assertTrue(special["loop"])

    def test_native_right_direction_is_mirrored(self):
        sprite = self.engine.native("OBJ_EVENT_GFX_NINJA_BOY")
        east = next(c for c in sprite["animations"] if c["id"] == "ANIM_STD_GO_EAST")
        west = next(c for c in sprite["animations"] if c["id"] == "ANIM_STD_GO_WEST")
        self.assertEqual([f["index"] for f in east["frames"]], [f["index"] for f in west["frames"]])
        self.assertTrue(all(f["hflip"] for f in east["frames"]))
        self.assertFalse(any(f["hflip"] for f in west["frames"]))

    def test_all_current_and_zerowalk_characters_are_present(self):
        data = build(ROOT / "build/upstream", allow_pending=True, expect_count=99)
        self.assertEqual(data["count"], 99)
        self.assertEqual(sum(c["new"] for c in data["characters"]), 15)
        self.assertEqual(len({c["symbol"] for c in data["characters"]}), 99)
        self.assertEqual(len({c["id"] for c in data["characters"]}), 99)
        for c in data["characters"]:
            self.assertIsNotNone(c["variants"]["original"])
            if not c["new"]:
                self.assertFalse(c["missing"], c["name"])
                self.assertTrue(all(c["variants"][key] for key in ("walk", "mach", "acro", "battle")))
                self.assertEqual(c["variants"]["mach"]["count"], 9)
                self.assertEqual(c["variants"]["acro"]["count"], 27)
        brendan = next(c for c in data["characters"] if c["symbol"] == "OBJ_EVENT_GFX_RIVAL_BRENDAN_NORMAL")
        self.assertEqual(brendan["variants"]["mach"]["count"], 9)
        self.assertEqual(brendan["variants"]["acro"]["count"], 27)
        throw = next(c for c in brendan["variants"]["battle"]["animations"] if c["action"] == "BATTLE_THROW")
        self.assertFalse(throw["loop"])
        self.assertEqual([f["duration"] for f in throw["frames"]], [24, 9, 24, 9, 50])
        if data["pending"]:
            with self.assertRaisesRegex(ValueError, "Gallery incomplete"):
                build(ROOT / "build/upstream", expect_count=99)
        else:
            self.assertTrue(build(ROOT / "build/upstream", expect_count=99)["complete"])
            for character in data["characters"]:
                if not character["new"]:
                    continue
                run = next(c for c in character["variants"]["walk"]["animations"] if c["id"] == "ANIM_RUN_EAST")
                self.assertEqual([f["duration"] for f in run["frames"]], [5, 3, 5, 3])
                self.assertTrue(all(f["hflip"] for f in run["frames"]))


if __name__ == "__main__":
    unittest.main()
