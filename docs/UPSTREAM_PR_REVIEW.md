# Upstream-PR-Entscheidungen

Stand: 2026-11-14. Repository: [ZallaxDev/emerald-3ds](https://github.com/ZallaxDev/emerald-3ds/pulls).

**12 uebernommen, 2 hier nicht anwendbar, 5 bewusst zurueckgestellt.**

Die vollstaendigen geprueften Commit-IDs und unsere Integrationscommits stehen in
[tools/upstream_pr_decisions.json](../tools/upstream_pr_decisions.json).

## Regel fuer den naechsten Abgleich

1. Vor jeder Upstream-PR-Pruefung die JSON-Liste laden.
2. Gleiche PR-Nummer und unveraenderter `reviewed_head`: Entscheidung uebernehmen und ueberspringen, auch wenn die PR weiterhin offen ist.
3. Neue PR oder neuer Head: nur die Aenderung seit dem gespeicherten Stand sowie ihre Kompatibilitaet pruefen; danach Entscheidung und Commit aktualisieren.
4. Eine unveraenderte, bewusst ausgelassene PR nur bei ausdruecklichem Nutzerauftrag oder konkret neuem Integrationsbedarf wieder oeffnen und den Anlass festhalten.
5. Bei gestapelten PRs die abgedeckten Nummern einzeln eintragen. Keine fremde Integration als Grundlage mitziehen, wenn nur die Topic-Aenderung geprueft wurde.

## Entscheidungen

| PR / gepruefter Head | Entscheidung | Umfang / Grund |
| --- | --- | --- |
| [#267](https://github.com/ZallaxDev/emerald-3ds/pull/267) `c41675cccb16` | Uebernommen | Steuerkreuz auf der Fliegenkarte; nur Topic-Aenderung, bestehende Koop-/Fly-Pfade erhalten, Wiederholung beim Oeffnen zurueckgesetzt. |
| [#266](https://github.com/ZallaxDev/emerald-3ds/pull/266) `89885a594c19` | Uebernommen | Sichtbare Wild-Pokemon behalten zusammengehoerige PID und IVs; als neuer Patch 0100 hinter unseren Koop-Patches portiert. Bereits gefangene Pokemon bleiben unveraendert. |
| [#263](https://github.com/ZallaxDev/emerald-3ds/pull/263) `1f1d2791c424` | Nicht anwendbar | Nicht anwendbar: unsere Runtime benutzt weder req.fill/sWideView noch JobSortParts/Boxes/SiteReach oder treeCrowns. Unsere reale Runtime-Suite bestand bereits vor der Integration. Keine neueren Renderer-Fixtures in aelteren Produktionscode kopieren. |
| [#262](https://github.com/ZallaxDev/emerald-3ds/pull/262) `286269615a5b` | Nicht anwendbar | Nicht anwendbar: unsere VoxelChunk-/DraftChunk-Version besitzt die angesprochenen treeShadow/treeCrowns/parts.on-Felder nicht; unsere Draft-Suite bestand bereits. Kein Produktionsfehler dieses Forks. |
| [#261](https://github.com/ZallaxDev/emerald-3ds/pull/261) `4ca7726d28d2` | Uebernommen | Verstaendlicher Pinball-ROM-Fehler fuer BPPE/BPPJ/BPPP. Emerald-Fingerprints bleiben verbindlich; zusaetzlich synthetische GBA-/ZIP-Regression. |
| [#210](https://github.com/ZallaxDev/emerald-3ds/pull/210) `c3136d2f4a00` | Uebernommen | Konsolen-Kartenkorrekturen Teil 4, zusammen mit den gestapelten Teilen 1..3; eigene vollstaendige Kartenreview-Historie erhalten. |
| [#209](https://github.com/ZallaxDev/emerald-3ds/pull/209) `22f99174b9b6` | Uebernommen | Franzoesische/italienische Startup-Fehler und START-Hinweis. Unser stdio-Backend erhalten; echte Quelltests fuer sieben Sprachen mit Windows-Adapter. |
| [#208](https://github.com/ZallaxDev/emerald-3ds/pull/208) `a9ec8a9a725d` | Uebernommen | Konsolen-Kartenkorrekturen Teil 3; im Stack von PR 210 uebernommen. |
| [#207](https://github.com/ZallaxDev/emerald-3ds/pull/207) `9c22debd1741` | Uebernommen | Konsolen-Kartenkorrekturen Teil 2; im Stack von PR 210 uebernommen. |
| [#206](https://github.com/ZallaxDev/emerald-3ds/pull/206) `e1c56f26e27b` | Uebernommen | Konsolen-Kartenkorrekturen Teil 1; im Stack von PR 210 uebernommen. |
| [#199](https://github.com/ZallaxDev/emerald-3ds/pull/199) `b7b1b340545d` | Uebernommen | Leere Untergrund-Kartenraender bleiben transparent statt als schwarze Wand vor der Figur zu stehen. |
| [#190](https://github.com/ZallaxDev/emerald-3ds/pull/190) `69945262e397` | Uebernommen | Linux/macOS-Builder-Anleitung. Auf passenden Koop-Quellstand und Payload eingeschraenkt; Original-Webbuilder ist kein Koop-Installer. |
| [#148](https://github.com/ZallaxDev/emerald-3ds/pull/148) `2ec0dc9bf628` | Bewusst ausgelassen | Bewusst zurueckgestellt: breite neue Intro-/Namenseingabe aus alter UI-Basis (Hit-ID-Umbau, 32-Byte-Namensfeld, alter Patch 0057 und eigene Sprachlokalisierung). Kein gezielter Koop-Fix und keine Hostregression fuer unsere aktuelle UI/Sprachen; benoetigt eigenstaendige Portierung statt Mitmerge. |
| [#140](https://github.com/ZallaxDev/emerald-3ds/pull/140) `98ce55264f59` | Uebernommen | Optionale zufaellige Boot-Seed-Einstellung; Standard folgt GBA-Fallback, neue Trainer-ID bleibt zeitbasiert. Alle sieben Sprachen und eigener Koop-/longjmp-Reset erhalten. |
| [#122](https://github.com/ZallaxDev/emerald-3ds/pull/122) `5a17510e4a5c` | Bewusst ausgelassen | Bewusst ausgelassen: exakter 2x-Kampfausschnitt schneidet aeussere Kaempfer im Doppelkampf ab (auch im PR beschrieben). Passt nicht zu unserem Sprint-6-Doppel-/Zuschauerpfad; kein geeigneter Standard. |
| [#111](https://github.com/ZallaxDev/emerald-3ds/pull/111) `4d06aff21ad6` | Bewusst ausgelassen | Bewusst zurueckgestellt: globale tiefste Wasser-Randebene ersetzt neuere lokale EdgeFill-Hoehen/Sea-Search unseres Renderers. Cache schluesselt nur Instance-Zeiger/Map-ID/Abmessungen, nicht wechselndes privates Zuschauer-Raster. Unveraendert koennte dies vorhandene Kartenkorrekturen oder Zuschauer-Karten falsch darstellen; benoetigt angepasste Cache-Invalidierung und gemeinsame Geometriepruefung. |
| [#70](https://github.com/ZallaxDev/emerald-3ds/pull/70) `7ca86c680cee` | Bewusst ausgelassen | Bewusst nicht pauschal uebernommen: alte Zusatz-Specs ueberlappen aktuelle Landmark-Modelle (u.a. Fortree-Treehouse, Safari-Tor, Space Center und Stadtgebaeude) und eigene vollstaendige Innenraeume. Doppelte Platzierungen/uebermalte Grundflaechen vermeiden. Einzelne noch fehlende Props waeren ein separater, auf Luecken begrenzter Modellauftrag. |
| [#68](https://github.com/ZallaxDev/emerald-3ds/pull/68) `d188bc72ae10` | Bewusst ausgelassen | Bewusst zurueckgestellt: umfangreiche Stereo-Renderpipeline mit optionalen externen Mon-/Back-Planes, keine Abdeckung unserer 99 Figuren und kein privater Zuschauer-PresentContext. Offene Performance-/Reviewpunkte (Old3DS-P99 39..50ms laut PR); unpassend waehrend bekannter Zuschauer-Ruckler. |
| [#39](https://github.com/ZallaxDev/emerald-3ds/pull/39) `0378e966bd89` | Uebernommen | Optionale macOS-Release-Paketierung; sieben Sprachen erhalten, Hostpruefung gegen falsch gelabelte PyInstaller-Binaerdateien ergaenzt. Kein nativer macOS-Build auf Windows behauptet. |

## Grenzen der Freigabe

Die Entscheidungen beziehen sich auf diesen Quellstand. Hintergrundtests und ARM-Build ersetzen keine Emulator-/Hardwareabnahme. Bereits bekannte Gras- und Zuschauer-Issues sowie die offenen Sprint-6-EP-/Preisgeldpunkte werden durch diese PRs nicht als behoben erklaert.
macOS-Paketierung wurde unter Windows nur auf CLI-/Paketierungslogik und Audit-Regeln geprueft, nicht als nativer macOS-Release gebaut.
