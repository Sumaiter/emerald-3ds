# Cooperative bicycle art

The manifest covers all 84 selectable identities. There are 82 new Imagegen
sheets and two native mappings, preserving the original Brendan and May bike
art and animations. Each custom PNG contains 27 aligned 32x32 frames in a
9-column, 3-row sheet, with transparent palette index 0 and at most 15 visible
colors. The first nine frames also serve Mach riding; all 27 serve Acro riding.
East-facing frames are mirrored by the engine. Frame names and order are in
`manifest.json`.

`prompts.json` records the character-specific drawing prompts. Each generation
used a local identity reference and a local native bike layout reference.
`provenance.json` records original generated-file and final PNG hashes, reference
hashes, normalization, and the three Imagegen background cleanups. Original game
reference images remain in ignored `build/bike-art/references`; no original game
pixels or palettes are copied into this asset directory.

All 82 identities and all 27-frame sheets have been visually reviewed. The
packer verifies complete roster coverage, native mappings, dimensions, binary
alpha, the shared RGB555 palette limit, nonempty frames, and changing pedal and
wheelie frames. Preview contacts and full frame pages are available locally as
`build/bike-art/contact-*.png` and `build/bike-art/frames-*.png`.

From the repository root, with Pillow installed:

```sh
python 3ds_port/tests/test_coop_bike_pack.py
python 3ds_port/scripts/pack_coop_bike.py --manifest 3ds_port/assets/coop_bike/manifest.json --upstream build/upstream --output build/bike-art/bikes.bin
```
