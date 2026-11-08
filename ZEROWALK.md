# Figuren ohne Laufbewegung

Diese 15 Figuren wurden aus der Multiplayer-Charakterauswahl entfernt, weil ihre Südlaufanimation keine tatsächliche Bewegung darstellt. Sie bleiben hier dokumentiert, damit sie nach einer Ergänzung eigener Laufbilder wieder aufgenommen werden können.

Geprüft wurden `object_event_graphics_info.h`, `object_event_pic_tables.h` und `object_event_anims.h`. Bei allen Einträgen verwendet `ANIM_STD_GO_SOUTH` die Bildfolge `0, 0, 1` mit nur zwei Ausrichtungsbildern anstelle einer Schrittfolge.

| Anzeigename | Grafik-ID |
|---|---|
| COOK | `OBJ_EVENT_GFX_COOK` |
| OLD MAN | `OBJ_EVENT_GFX_OLD_MAN` |
| SIDNEY | `OBJ_EVENT_GFX_SIDNEY` |
| PHOEBE | `OBJ_EVENT_GFX_PHOEBE` |
| GLACIA | `OBJ_EVENT_GFX_GLACIA` |
| DRAKE | `OBJ_EVENT_GFX_DRAKE` |
| ROXANNE | `OBJ_EVENT_GFX_ROXANNE` |
| BRAWLY | `OBJ_EVENT_GFX_BRAWLY` |
| WATTSON | `OBJ_EVENT_GFX_WATTSON` |
| FLANNERY | `OBJ_EVENT_GFX_FLANNERY` |
| WINONA | `OBJ_EVENT_GFX_WINONA` |
| LIZA | `OBJ_EVENT_GFX_LIZA` |
| TATE | `OBJ_EVENT_GFX_TATE` |
| GAMEBOY KID | `OBJ_EVENT_GFX_GAMEBOY_KID` |
| MYSTERY GIFT MAN | `OBJ_EVENT_GFX_MYSTERY_GIFT_MAN` |

## Nicht betroffen

`QUINTY_PLUMP` verwendet keine Standardtabelle, besitzt mit `sAnim_QuintyPlumpGoSouth` aber eine echte Laufbewegung und bleibt auswählbar.

Zusätzlich wurden die Wasser-/Link-Sonderfiguren `SWIMMER_M`, `SWIMMER_F`, `HOT_SPRINGS_OLD_WOMAN`, `LINK_BRENDAN` und `LINK_MAY` unabhängig von ihren vorhandenen Laufbildern aus der Auswahl entfernt.
