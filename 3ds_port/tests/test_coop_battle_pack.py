"""Coverage, palette transparency and tile-order checks for battle avatar art."""
import importlib.util
import json
import struct
import tempfile
import unittest
from pathlib import Path

from PIL import Image

SPEC = importlib.util.spec_from_file_location("back_pack", Path(__file__).parents[1] / "scripts/pack_coop_battle_back.py")
PACK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACK)


class BattleBackPackTest(unittest.TestCase):
    def test_tiles_and_palette(self):
        with tempfile.TemporaryDirectory() as directory:
            image = Image.new("RGBA", (64, 64))
            # Adjacent pixels share one byte; a new tile starts at byte 32.
            image.putpixel((0, 0), (248, 0, 0, 255))
            image.putpixel((1, 0), (0, 248, 0, 255))
            image.putpixel((8, 0), (0, 0, 248, 255))
            image.putpixel((0, 8), (0, 0, 248, 255))
            path = Path(directory) / "back.png"
            image.save(path)
            blob = PACK.encode_image(path)
            self.assertEqual(len(blob), 2080)
            self.assertEqual(struct.unpack_from("<4H", blob), (0, 31, 31 << 5, 31 << 10))
            self.assertEqual(blob[32], 0x21)
            self.assertEqual(blob[32 + 32], 3)
            self.assertEqual(blob[32 + 8 * 32], 3)
            self.assertEqual(blob[33], 0)

    def test_roster_and_native_mapping(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "manifest.json"
            entries = [dict(symbol="OBJ_EVENT_GFX_RED", graphics_id=83,
                            source="native", native_back_pic_id=2)]
            expected = {"OBJ_EVENT_GFX_RED": 83}
            path.write_text(json.dumps(dict(entries=entries)))
            blob = PACK.pack(path, expected)
            self.assertEqual(len(blob), 16)  # no native pixels embedded
            self.assertEqual(struct.unpack("<4sHHHBBI", blob), (b"CBK1", 1, 1, 83, 2, 0, 0))
            with self.assertRaises(ValueError):
                PACK.pack(path, {**expected, "OBJ_EVENT_GFX_LEAF": 84})
            entries.append(entries[0])
            path.write_text(json.dumps(dict(entries=entries)))
            with self.assertRaises(ValueError):
                PACK.pack(path, expected)
            entries = entries[:1]
            entries[0]["native_back_pic_id"] = 3
            path.write_text(json.dumps(dict(entries=entries)))
            with self.assertRaises(ValueError):
                PACK.pack(path, expected)

    def test_bad_image_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.png"
            image = Image.new("RGBA", (64, 64))
            image.putpixel((0, 0), (255, 0, 0, 128))
            image.save(path)
            with self.assertRaises(ValueError):
                PACK.encode_image(path)
            for x in range(16):
                image.putpixel((x, 0), (x * 8, 0, 0, 255))
            image.save(path)
            with self.assertRaises(ValueError):
                PACK.encode_image(path)


if __name__ == "__main__":
    unittest.main()
