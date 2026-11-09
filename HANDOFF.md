# Übergabe: Emerald 3DS Local Co-op

Stand: 2026-11-09

Dieser Stand ist eine Entwicklungsversion, **keine vollständig spielbare Alpha**. Sprint 1 und Sprint 2 sind geprüft. Sprint 3 ist in Arbeit; Host-Spielstand, Spawn, Kartenwechsel und Bewegung funktionieren im Emulator.

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
- Die Karte wird mit `InitMap()` vollständig aufgebaut und nicht mehr aus dem alten gespeicherten Kartenausschnitt des Hosts zusammengesetzt.
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

Das Protokoll hat Version 3. Es enthält Authentifizierung, Identität, Spawn, Snapshot-Beginn, Chunk-Anforderung, Chunk und Übernahmebestätigung. Der Host sendet einen Chunk erst nach Anforderung durch den Gast; dadurch läuft die Netzwerk-Warteschlange nicht voll.

Die namensbasierte Spawntabelle liegt in `gSaveBlock1Ptr->unused_3598`, mit Magic `0x5350`, Version `1` und vier Einträgen. Positionspakete verwenden ObjectEvent-Koordinaten einschließlich `MAP_OFFSET`; `SetWarpDestination()` erhält die Koordinaten ohne diesen Rand.

Nach den Karten-Ladeskripten wird die Gastfigur einmalig exakt auf den gespeicherten Spawn gesetzt. Erst danach endet `joinWarpPending` und die normale Bewegungssynchronisation beginnt. `ContinueJoinedGame()` ruft anschließend `InitMap()` auf, damit `LoadSavedMapView()` nicht den alten Host-Ausschnitt mit der neuen Gastposition kombiniert.

Nach **A** ist die Auswahl im Suchmenü während des Beitritts gesperrt. **B** kann weiterhin abbrechen.

## Noch offen

- Kollisionen und Interaktionen mit der zweiten Figur festlegen.
- Laufende Team-, PC-, Item- und Storyänderungen synchronisieren; der vollständige Transfer gilt nur für den Initialzustand beim Beitritt.
- PC-Sperre, Mehrspieler-Box und Startteam-Auswahl in Sprint 4 umsetzen.
- Unabhängige Einzelkämpfe und später gemeinsame Doppelkämpfe implementieren.
- Dritte Konsole und bestehende Link-Theken erneut prüfen.
- Tests auf zwei realen 3DS-/2DS-Konsolen durchführen.
- Den direkten Beitritt von BB ohne Spawn-Menü nachbestätigen.

Sprint 3 darf erst abgeschlossen werden, wenn alle seine Akzeptanzkriterien erfüllt sind.

## Nächste Arbeitsschritte

1. Den bestätigten Basistest einschließlich Kartenwechsel nicht erneut als offenen Fehler behandeln.
2. Kollisionen und Interaktionen mit der zweiten Figur festlegen.
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
