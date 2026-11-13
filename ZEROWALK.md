# Figuren mit ergänzten Laufbewegungen

Diese 15 Figuren sind im Quellstand wieder in der Multiplayer-Charakterauswahl enthalten. Für jede Figur liegen neun neu gezeichnete Laufbilder, 27 Fahrradbilder für Eilrad/Kunstrad und ein eigenes Kampfrückenbild vor. Die Auswahl umfasst damit 99 Figuren für Host und Gast.

Der ursprüngliche Ausschluss beruhte auf `object_event_graphics_info.h`, `object_event_pic_tables.h` und `object_event_anims.h`: Bei allen Einträgen verwendet die originale Tabelle für `ANIM_STD_GO_SOUTH` die Bildfolge `0, 0, 1` anstelle einer Schrittfolge. Diese originalen NPC-Daten bleiben erhalten. Nur ausgewählte Spieler und die Charaktervorschau verwenden die neuen Bilder.

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

## Umsetzung und Prüfung

Der Nutzer hat nach Sichtung der vollstaendigen HTML-Galerie die Grafiken
aller 99 Figuren freigegeben. Die Funktionsabnahme im Spiel bleibt separat.

- Laufbilder: `3ds_port/assets/coop_walk/`, neun 32×32-Bilder pro 96×96-Bogen. Standansichten Süd/Nord/West und je zwei echte Schrittposen; Osten wird gespiegelt. Die Bilder sind auf einen gemeinsamen Fußpunkt ausgerichtet und haben transparente Zellränder.
- Fahrrad- und Kampfdaten: je 15 Ergänzungen in `3ds_port/assets/coop_bike/` und `3ds_port/assets/coop_battle_back/`. Die bisherigen 158 PNGs der 84 vorhandenen Figuren sind anhand ihrer gespeicherten Hashes unverändert.
- Alle 45 Zeichnungen stammen aus dem eingebauten Imagegen-Werkzeug. Skripte formatieren Raster, Registrierung, Alpha und Palette; sie erzeugen keine Posen. Prompts, Provenienz, finale Hashes und Sichtprüfung stehen neben den Assets. Originalreferenzen bleiben im ignorierten Build-Verzeichnis.
- Spieleranbindung: `3ds_coop_walk.c/.h` über die bestehenden Fahrrad-Grafik-/Palettenpfade. Patch `0086-coop-zerowalk-avatars.patch` erweitert ausschließlich Auswahl und Vorschau. Original-IDs und NPCs bleiben erhalten.
- Bestanden: Laufzeit-C-Tests, erweiterte Fahrrad-C-Tests, echte Engine-Grafik-/Reflexionsproben und strikte Packer-Prüfungen für alle 99 Auswahlfiguren. Alle fünf finalen Vergleichsbögen wurden geprüft.

Gemeinsame Build-Anbindung und ARM-/Release-/PAK-Build sind durch den Hauptagenten abgeschlossen (Build 03ecd34074355974, ABI ebf0f1f4). Die vollstaendige HTML-Pruefgalerie fuer alle 99 Figuren liegt lokal unter `build/character-gallery/index.html`. Daten- und Animationspruefungen bestehen; die sichtbare Browserpruefung und interaktive Emulator-/Hardware-Abnahme bleiben nachfolgende Pruefschritte.

## Nicht betroffen

`QUINTY_PLUMP` verwendet keine Standardtabelle, besitzt mit `sAnim_QuintyPlumpGoSouth` aber eine echte Laufbewegung und bleibt auswählbar.

Zusätzlich wurden die Wasser-/Link-Sonderfiguren `SWIMMER_M`, `SWIMMER_F`, `HOT_SPRINGS_OLD_WOMAN`, `LINK_BRENDAN` und `LINK_MAY` unabhängig von ihren vorhandenen Laufbildern aus der Auswahl entfernt.
