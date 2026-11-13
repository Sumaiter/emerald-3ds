#!/usr/bin/env python3
"""Reject incomplete/static walk art and verify native tiled frame layout."""
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"3ds_port/scripts"))
import pack_coop_walk as walk
from prepare_coop_walk import register_walk


class WalkingPackTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        self.image=Image.new("RGBA",(96,96))
        for f in range(9):
            self.image.putpixel(((f%3)*32+f+2,(f//3)*32+12),(248,0,0,255))
        self.image.save(self.root/"cook.png")
        self.manifest=dict(version=1,frame_width=32,frame_height=32,columns=3,frame_names=walk.FRAME_NAMES,
            entries=[dict(symbol="OBJ_EVENT_GFX_COOK",graphics_id=27,source="generated",image="cook.png")])

    def pack(self):
        path=self.root/"manifest.json"; path.write_text(json.dumps(self.manifest))
        return walk.pack(path,{"OBJ_EVENT_GFX_COOK":27})

    def test_each_frame_has_bounded_native_tiles_and_correct_palette(self):
        data=self.pack()
        self.assertEqual(struct.unpack_from("<4sHH",data),(b"CWK1",1,1))
        self.assertEqual(struct.unpack_from("<HBBI",data,8),(27,0,0,16))
        self.assertEqual(len(data),16+32+9*512)
        self.assertEqual(struct.unpack_from("<H",data,18),(31,))
        for f in range(9):
            tiled=data[48+f*512:48+(f+1)*512]
            x,y=f+2,12; offset=((y//8)*4+x//8)*32+(y%8)*4+(x%8)//2
            self.assertEqual((tiled[offset]>>((x%2)*4))&15,1)
            self.assertEqual(sum(bool(p) for p in tiled),1)

    def test_static_step_or_idle_repeat_is_rejected(self):
        self.image.paste(self.image.crop((0,0,32,32)),(0,32)); self.image.save(self.root/"cook.png")
        with self.assertRaisesRegex(ValueError,"distinct walking poses"): self.pack()

    def test_all_nine_frames_need_art_and_transparency(self):
        self.image.paste((0,0,0,0),(0,0,32,32)); self.image.save(self.root/"cook.png")
        with self.assertRaisesRegex(ValueError,"frame 0"): self.pack()

    def test_cell_boundaries_remain_transparent(self):
        self.image.putpixel((31,12),(248,0,0,255)); self.image.save(self.root/"cook.png")
        with self.assertRaisesRegex(ValueError,"cell boundary"): self.pack()

    def test_variable_generated_margins_cannot_mix_adjacent_drawings(self):
        # Separate existing drawings cross the nominal mathematical thirds.
        # Formatting must retain nine whole figures, with a shared footpoint.
        source=Image.new("RGBA",(240,240))
        for f in range(9):
            x=(f%3)*70+16; y=(f//3)*67+5
            source.paste((248,0,0,255),(x,y,x+18,y+52+(f%2)*4))
        registered=register_walk(source)
        for f in range(9):
            frame=registered.crop(((f%3)*32,(f//3)*32,(f%3+1)*32,(f//3+1)*32))
            bounds=frame.getchannel('A').getbbox()
            self.assertEqual(bounds[3],31)
            self.assertGreaterEqual(bounds[0],1)
            self.assertLessEqual(bounds[2],31)
        with self.assertRaisesRegex(ValueError,'nine separated'):
            register_walk(Image.new('RGBA',(96,96)))

    def test_wrong_size_alpha_and_color_budget_are_rejected(self):
        self.image.resize((192,192)).save(self.root/"cook.png")
        with self.assertRaisesRegex(ValueError,"96x96"): self.pack()
        self.image.putpixel((0,0),(255,0,0,128)); self.image.save(self.root/"cook.png")
        with self.assertRaisesRegex(ValueError,"binary"): self.pack()
        for x in range(16): self.image.putpixel((x,0),(x*8,0,0,255))
        self.image.save(self.root/"cook.png")
        with self.assertRaisesRegex(ValueError,"15 visible"): self.pack()

    def test_layout_coverage_and_path_escape_are_rejected(self):
        self.manifest["columns"]=9
        with self.assertRaisesRegex(ValueError,"layout"): self.pack()
        self.manifest["columns"]=3
        self.manifest["entries"].append(dict(self.manifest["entries"][0]))
        with self.assertRaisesRegex(ValueError,"exactly once"): self.pack()
        self.manifest["entries"].pop(); self.manifest["entries"][0]["image"]="../cook.png"
        with self.assertRaisesRegex(ValueError,"outside"): self.pack()

    def test_all_15_real_sheets_encode(self):
        upstream=ROOT if (ROOT/"src/multiplayer_menu.c").is_file() else ROOT/"build/upstream"
        expected=walk.identities(upstream)
        self.assertEqual(len(expected),15)
        data=walk.pack(ROOT/"3ds_port/assets/coop_walk/manifest.json",expected)
        self.assertEqual(len(data),8+15*(8+32+9*512))


if __name__=="__main__": unittest.main(verbosity=2)
