#!/usr/bin/env python3
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
from PIL import Image

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/pack_coop_bike.py"
spec = importlib.util.spec_from_file_location("bike_pack", SCRIPT)
bike = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bike)


class BicyclePackTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.image = Image.new("RGBA", (288, 96))
        for frame in range(27):
            self.image.putpixel(((frame % 9)*32 + frame % 27, (frame // 9)*32 + 12), (248, 0, 0, 255))
        self.image.save(self.root / "rider.png")
        self.expected = {"OBJ_EVENT_GFX_NINJA_BOY":5, "OBJ_EVENT_GFX_RIVAL_MAY_NORMAL":105}
        self.entries = [dict(symbol="OBJ_EVENT_GFX_NINJA_BOY", graphics_id=5, source="generated", image="rider.png"),
                        dict(symbol="OBJ_EVENT_GFX_RIVAL_MAY_NORMAL", graphics_id=105, source="native", native_graphics_ids=[106,107])]

    def pack(self):
        path = self.root / "manifest.json"
        path.write_text(json.dumps(dict(entries=self.entries)), encoding="utf-8")
        return bike.pack(path, self.expected)

    def test_tile_order_and_all_27_frame_offsets(self):
        data = self.pack()
        self.assertEqual(struct.unpack_from("<4sHH", data), (b"CBI1",1,2))
        self.assertEqual(struct.unpack_from("<BBBBII",data,8), (5,255,255,0,32,0))
        self.assertEqual(struct.unpack_from("<BBBBII",data,20), (105,106,107,0,0,0))
        self.assertEqual(len(data),32+13856)
        self.assertEqual(struct.unpack_from("<H",data,34)[0],31)  # red RGB555
        for frame in range(27):
            tiled = data[64+frame*512:64+(frame+1)*512]
            x,y=frame%27,12
            offset=((y//8)*4+x//8)*32+(y%8)*4+(x%8)//2
            self.assertEqual((tiled[offset] >> ((x%2)*4)) & 15,1)
            self.assertEqual(sum(p!=0 for p in tiled),1)

    def test_manifest_must_cover_every_selected_character_once(self):
        self.entries.pop()
        with self.assertRaises(ValueError): self.pack()
        self.entries.append(dict(self.entries[0]))
        with self.assertRaises(ValueError): self.pack()

    def test_native_mapping_cannot_replace_character(self):
        self.entries[1]["native_graphics_ids"]=[101,102]
        with self.assertRaises(ValueError): self.pack()

    def test_transparency_and_size(self):
        self.image.putpixel((0,0),(255,0,0,128)); self.image.save(self.root/"rider.png")
        with self.assertRaisesRegex(ValueError,"binary"): self.pack()
        self.image.resize((576,192)).save(self.root/"rider.png")
        with self.assertRaisesRegex(ValueError,"288x96"): self.pack()

    def test_empty_or_repeated_animation_frames_rejected(self):
        self.image.paste((0,0,0,0),(0,0,32,32)); self.image.save(self.root/"rider.png")
        with self.assertRaisesRegex(ValueError,"frame 0"): self.pack()
        self.image=Image.new("RGBA",(288,96))
        for frame in range(27): self.image.putpixel(((frame%9)*32+12,(frame//9)*32+12),(248,0,0,255))
        self.image.save(self.root/"rider.png")
        with self.assertRaisesRegex(ValueError,"do not animate"): self.pack()

    def test_missing_art_and_path_escape_rejected(self):
        self.entries[0]["image"]="missing.png"
        with self.assertRaises(FileNotFoundError): self.pack()
        self.entries[0]["image"]="../outside.png"
        with self.assertRaisesRegex(ValueError,"outside"): self.pack()

    def test_real_manifest_covers_all_99_identities(self):
        repo = SCRIPT.parents[2]
        upstream = repo if (repo / "src/multiplayer_menu.c").is_file() else repo / "build/upstream"
        expected = bike.choices(upstream)
        manifest = json.loads((repo/"3ds_port/assets/coop_bike/manifest.json").read_text())
        self.assertEqual(len(expected),99)
        self.assertEqual({e["symbol"]:e["graphics_id"] for e in manifest["entries"]},expected)
        self.assertEqual(sum(e["source"]=="native" for e in manifest["entries"]),2)


if __name__=="__main__": unittest.main(verbosity=2)
