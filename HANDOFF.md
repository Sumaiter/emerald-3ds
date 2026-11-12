# Übergabe: Emerald 3DS Local Co-op

Stand: 2026-11-11

Dieser Stand ist eine Entwicklungsversion, **keine vollständig spielbare Alpha**. Sprint 1 und Sprint 2 sind geprüft. Sprint 3 und Sprint 4 sind in Arbeit; Host-Spielstand, Spawn, Kartenwechsel, Bewegung, Kollisionen und unabhängige Menüs funktionieren im Emulator. Die neuen Join-, NPC-, Starter-, Box- und PC-Abläufe müssen noch gemeinsam im Emulator abgenommen werden.

## Repository

- Fork: `https://github.com/Sumaiter/emerald-3ds`
- Original: `https://github.com/ZallaxDev/emerald-3ds`
- Veröffentlichung und Ersteinrichtung erfolgen über `main`.
- Letzter Übergabe-Commit: `0cdbc1c Document sprint 3 handoff and PC setup`.
- Die neueren Snapshot- und Spawn-Änderungen werden mit dieser Übergabe veröffentlicht.

## Mit zwei Azahar-Instanzen bestätigt

- Der Gast lädt seine Welt nicht mehr aus seiner lokalen SAV-Datei.
- Der Host überträgt `SaveBlock2`, `SaveBlock1` und `PokemonStorage` mit Prüfsumme vollständig in den RAM des Gastes: **53.380 Byte** in 128-Byte-Chunks.
- Der Gast übernimmt den Zustand erst nach vollständigem Empfang und korrekter Prüfsumme. Party, ObjectEvents, Beutel und Verschlüsselungsschlüssel werden daraus initialisiert.
- Ein neuer Gastname startet an einem festen, freien Punkt in Wurzelheim, auch wenn der Host sich an einem anderen Ort befindet.
- Nach Bewegung, Host-Speichern und erneutem Beitritt erscheint der Gast wieder an derselben gespeicherten Position.
- Kartenwechsel durch Türen funktionieren einschließlich Verschwinden, gemeinsamer Sichtbarkeit und Rejoin an der richtigen Position.
- Die Karte wird beim Beitritt genau einmal durch `CB2_ContinueSavedGame()` aufgebaut. Ein zusätzlicher `InitMap()`-Aufruf wurde entfernt, weil er die Kartendarstellung beschädigte.
- Host und Gast sehen sich und können sich bewegen.
- Die lokale Gast-SAV bleibt bytegenau unverändert; vorher und nachher war der SHA-256 identisch.

Die wichtigen Protokollmeldungen sind `coop: host save applied 53380 bytes` und `coop: join warp complete at X,Y`.

## Verbindliche Spawnregel

- **Neuer Gastname:** fester Spawn in Wurzelheim (`0.9`, Layout `10`, Position `19,16`).
- **Bekannter Gastname:** immer die letzte unter diesem Namen gespeicherte Position.
- Es gibt **keine Stadt- oder Spawn-Auswahl**. Ein zwischenzeitlich eingebautes Auswahlmenü wurde auf Nutzerentscheid wieder entfernt.
- Die Rückkehr von BB ohne dieses Menü ist nach der Entfernung noch nicht erneut bestätigt; das zugrundeliegende Laden des gespeicherten Spawns war zuvor bereits geprüft.

## Implementierung

Geänderte Dateien:

- `3ds_port/include/3ds_coop.h`
- `3ds_port/include/3ds_coop_protocol.h`
- `3ds_port/src/3ds_coop.c`
- `3ds_port/src/3ds_coop_protocol.c`
- `3ds_port/tests/coop_protocol_test.c`
- `patches/pokeemerald/0083-local-coop.patch`

Das Protokoll hat Version 7. Es enthält Authentifizierung, Identität, Starteranforderung, Spawn, Snapshot-Beginn, Chunk-Anforderung, Chunk, Übernahmebestätigung, NPC-Positionen, PC-Sperre, atomare PC-Übertragungen sowie Story-Flags und Skriptvariablen. Chunks werden nur auf Anforderung gesendet; dadurch läuft die Netzwerk-Warteschlange nicht voll. `PrepareHostSave()` ruft vor dem Snapshot `SaveObjectEvents()` auf, damit er die aktiven NPC-Positionen statt des letzten Disk-Speicherstands enthält. Danach sendet der Host Änderungen sofort und alle sichtbaren NPCs im Rundlauf erneut. Der Gast übernimmt deren Position, Blickrichtung und Grafik und friert die unabhängige lokale NPC-Bewegung ein. Der Laufzeitvergleich dieser Synchronisation steht noch aus.

Die namensbasierte Spawntabelle liegt in `gSaveBlock1Ptr->unused_3598`, mit Magic `0x5350`, Version `1` und vier Einträgen. Positionspakete verwenden ObjectEvent-Koordinaten einschließlich `MAP_OFFSET`; `SetWarpDestination()` erhält die Koordinaten ohne diesen Rand.

Vor `CB2_ContinueSavedGame()` setzt `ContinueJoinedGame()` über `CtrCoop_EnterHostWorld()` das Warp-Ziel und leert `gSaveBlock1Ptr->mapView`. Dadurch kann `InitMapFromSavedGame()` keinen Ausschnitt der alten Hostposition über die frisch geladene Gastkarte kopieren. Bei einem Spawn auf einer anderen Karte entfernt der Join zusätzlich aktive NPC-Objekte der alten Karte und lädt die Objektvorlagen des Ziels; das lokale Spielerobjekt bleibt erhalten. Der normale Continue-Pfad lädt Karte, Skripte und Spieler genau einmal. Nach den Karten-Ladeskripten setzt `CtrCoop_OverworldFrame()` die Gastfigur exakt auf den gespeicherten Spawn. Erst danach endet `joinWarpPending` und die normale Bewegungssynchronisation beginnt.

Nach **A** ist die Auswahl im Suchmenü während des Beitritts gesperrt. Die ausgewählte Welt bleibt zwischengespeichert und sichtbar, auch wenn die Suche sie zwischenzeitlich nicht mehr liefert. Der Titel zeigt `VERBINDEN …`; der Hinweis auf eine fehlende Welt wird während des Beitritts nicht angezeigt. **B** kann weiterhin abbrechen.

## Sprint 4: Starter, Team und PC

- Ein neuer Gast ohne Spawn-Eintrag und ohne Pokémon in der Gast-Reihe wählt verbindlich Geckarbor, Flemmli oder Hydropi auf Level 5. Die Auswahl erzeugt genau ein Pokémon in der Gast-Reihe und im Team. `CB2_StarterSelectionActive()` verhindert, dass der Auswahltask im Verbindungsablauf erneut initialisiert wird. `CB2_InitStarterSelection()` gibt die Kontrolle an `CB2_MainMenu()` zurück, damit `RunTasks()` die Auswahl tatsächlich ausführt. `3ds_port/tests/starter_menu_test.c` prüft diesen Schedulerpfad, Hoch/Runter einschließlich Umbruch und die Bestätigung aller drei Starter mit dem echten Menü- und Taskcode. Der Empfang der Join-Info ist verbindliche Voraussetzung für den Beitrittsabschluss; Frameverarbeitung, PC-Öffnung und Weltbetreten bleiben bis dahin und bei neuen Gästen bis zur Starterwahl gesperrt. Danach öffnet der Startteam-PC; erst die bestätigte atomare PC-Übernahme löst den Spawn aus.
- Ein wiederkehrender Gast überspringt die Starterauswahl und verwendet gespeicherte Gast-Reihe und Spawnposition.
- Box 14 (Index `13`) ist die KOOP-Box. Reihe 1 (Indizes `0–5`) gehört dem Host, Reihe 2 (Indizes `6–11`) dem Gast. Bestehende Pokémon werden vor der Reservierung in normale Boxen migriert; ohne ausreichenden Platz wird die Welt nicht geöffnet.
- Eingeloggte Spielerreihen sind gegen Entnahme, Ablegen, Tausch, Freilassen und Mehrfachverschieben gesperrt. Nur eine Rolle darf den PC gleichzeitig verwenden.
- Ein PC-Abschluss überträgt vollständigen `PokemonStorage` und Gruppe mit Prüfsumme. Die Gegenseite übernimmt erst nach vollständigem Empfang und bestätigt danach; erst diese Bestätigung gibt die PC-Sperre frei.
- Persistente Story-Flags und Skriptvariablen werden laufend synchronisiert. Sende- und Empfangsschleifen sind auf `FLAGS_COUNT` beziehungsweise `VARS_COUNT` begrenzt; die früheren Obergrenzen `SPECIAL_FLAGS_START` und `SPECIAL_VARS_START` überschrieben die Baseline-Puffer und nachfolgende Koop-Zustände wie Starterstatus und Sitzungsgeneration. Der Join-Flow-Test sichert alle vier Grenzen. Der Laufzeittest der neuen Abläufe steht noch aus.

## Kollisionen und Menüs

- `LOCALID_COOP_PLAYER` ist im zentralen Kollisionscheck ausgenommen: Die Verbindungsfigur blockiert keine Bewegung. Terrain, NPCs und normale Kollisionen bleiben unverändert.
- Die Figur besitzt kein NPC-Script; ein Dialog wird nicht ausgelöst.
- Menüs, Beutel, PC und Kämpfe verwenden getrennte Engine-Callbacks und keine gemeinsamen Menüzustände im Koop-Modul.
- Kollisionen, NPC-Interaktionen und unabhängige Menüs wurden vom Nutzer im Laufzeittest bestätigt.

## Noch offen

- Die Startkarten- und Join-Anzeige-Fixes durch mehrere vollständige Gast-Neustarts und Joins direkt auf der Startkarte laufzeitprüfen.
- Die aktuellen und laufenden NPC-Positionen auf Host und Gast direkt vergleichen.
- Teamänderungen außerhalb bestätigter PC-Vorgänge und Itemänderungen synchronisieren; Story-Flags und Skriptvariablen werden bereits laufend übertragen.
- Startteam, PC-Sperre, atomare Übernahme und Reihensperren mit zwei Azahar-Instanzen abnehmen. Die Starterwahl selbst ist mit Flemmli im Emulator bestätigt.
- Das sichere Zurücklegen der aktuellen Gruppe einschließlich später Kampfänderungen beim Verbindungsende implementieren.
- Unabhängige Einzelkämpfe und später gemeinsame Doppelkämpfe implementieren.
- Dritte Konsole und bestehende Link-Theken erneut prüfen.
- Tests auf zwei realen 3DS-/2DS-Konsolen durchführen.

Sprint 3 darf erst abgeschlossen werden, wenn alle seine Akzeptanzkriterien erfüllt sind.

## Nächste Arbeitsschritte

1. Den bestätigten Basistest einschließlich Kartenwechsel, Kollisionen und Menüs nicht erneut als offenen Fehler behandeln.
2. Die initiale Kartendarstellung nach mehreren Joins abnehmen und Sprint 3 danach abschließen.
3. Danach mit Sprint 4 und der laufenden Spielstandssynchronisation fortfahren.
4. `Status.md` und diese Datei nach bestätigten Tests aktualisieren.

## Build- und Testregeln

- Quelle liegt im Repository unter `3ds_port/`; `build/upstream/` wird nicht committed.
- Vor einem inkrementellen Build geänderte Portdateien auch nach `build/upstream/3ds_port/` kopieren.
- `main_menu.c` wird im Buildbaum geändert und muss mit `patches/pokeemerald/0083-local-coop.patch` übereinstimmen.
- Patchkontrolle: `git -C build/upstream apply --check --reverse ../../patches/pokeemerald/0083-local-coop.patch`.
- Vor dem Abschluss: `make release pak`, `make -C 3ds_port verify-coop`, `git diff --check` und `python tools/release_audit.py --repo . --strict`.
- ROMs, Spielstände, PAKs, Build-Artefakte und Emulatorprofile niemals committen oder hochladen.

## Fortsetzung in einer neuen Paseo-Sitzung

> Lies zuerst `AGENTS.md`, `HANDOFF.md`, `SETUP.md` und `Status.md`. Arbeite an Sprint 3 weiter. Host-Snapshot, gespeicherter Spawn, Rejoin, Kartenwechsel, Bewegung und Kartendarstellung sind bereits bestätigt. Neue Namen starten in Wurzelheim, alte Namen immer an ihrem gespeicherten Spawn; es gibt keine Spawn-Auswahl. Keine ROMs, Saves oder Build-Artefakte committen. Halte die Dokumentation aktuell und teste jede Änderung im Emulator.
