# Übergabe: Emerald 3DS Local Co-op

Stand: 2026-11-09

Dieser Stand ist eine Entwicklungsversion, **keine vollständig spielbare Alpha**. Sprint 1 und Sprint 2 sind geprüft. Sprint 3 ist in Arbeit; Host-Spielstand, Spawn, Kartenwechsel und Bewegung funktionieren im Emulator.

## Repository

- Fork: `https://github.com/Sumaiter/emerald-3ds`
- Original: `https://github.com/ZallaxDev/emerald-3ds`
- Veröffentlichung und Ersteinrichtung für den Sprint-3-Test erfolgen über `main` (`6fbae82 Make co-op avatar non-blocking`).
- Sprint 4 wird getrennt auf `sprint-4` entwickelt. Die Koop-Box, Startteam-Auswahl, atomare PC-Übertragung und Reihensperren sind implementiert, aber noch nicht im Emulator abgenommen.
- Vor einem Build die Portdateien des gewählten Branches nach `build/upstream/3ds_port/` kopieren; dieser Arbeitsbaum wird von beiden Branches gemeinsam verwendet.

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

Das Protokoll hat Version 6. Es enthält Authentifizierung, Identität, Spawn, Snapshot-Beginn, Chunk-Anforderung, Chunk, Übernahmebestätigung, PC-Anfrage, Vergabe und Freigabe, atomare PC-Übertragungen sowie Änderungen von Story-Flags und Skriptvariablen. Chunks werden nur auf Anforderung gesendet; dadurch läuft die Netzwerk-Warteschlange nicht voll.

Die namensbasierte Spawntabelle liegt in `gSaveBlock1Ptr->unused_3598`, mit Magic `0x5350`, Version `1` und vier Einträgen. Positionspakete verwenden ObjectEvent-Koordinaten einschließlich `MAP_OFFSET`; `SetWarpDestination()` erhält die Koordinaten ohne diesen Rand.

## Sprint 4: Speicherformat und Koop-Box

- Die Größe der Original-Spielstände bleibt unverändert. `unused_3598` enthält nach der Spawn-Tabelle den KOOP-Versionsmarker `0x4342`, Version `1`.
- Box 14 (Index `13`) wird beim ersten Öffnen einer Koop-Welt als **KOOP** reserviert. Alle darin vorhandenen Pokémon werden zuerst in freie Plätze der Boxen 1–13 verschoben. Reicht der Platz nicht, wird die Welt nicht geöffnet und nichts verschoben.
- Reihe 1 (Indizes `0–5`) spiegelt die Host-Gruppe, Reihe 2 (Indizes `6–11`) die Gast-Gruppe. Während der jeweiligen Anmeldung sind diese Reihen gegen Entnahme, Ablegen, Tausch, Freilassen und Mehrfachverschieben gesperrt. Die übrigen drei Reihen bleiben normal nutzbar.
- Der Gast erhält beim Beitritt nicht die Host-Gruppe, sondern seine gespeicherte Gast-Reihe. `SavePlayerParty()` ist für den Gast gesperrt, damit die Host-Gruppe in `SaveBlock1` erhalten bleibt. Die lokale SAV-Datei des Gastes wird weiterhin nicht verändert.
- Nach der Spielstandübernahme öffnet sich der PC für die Startteam-Auswahl. Mindestens ein Pokémon ist Pflicht. Erst nach bestätigter Übernahme sendet der Host den Spawn und der Beitrittsbildschirm öffnet die Welt.
- Ein PC-Abschluss überträgt **34.392 Byte**: vollständiger `PokemonStorage` plus sechs vollständige Party-Pokémon. Die Gegenseite übernimmt den Speicher erst nach vollständigem Empfang und Prüfsumme, aktualisiert die Reihe des Absenders und bestätigt danach. Erst diese Bestätigung gibt die PC-Sperre frei.
- Diese neuen Abläufe sind gebaut und hostseitig auf Protokollebene getestet. Die vollständige Emulatorabnahme mit Teamwahl, Rejoin, gesperrten Reihen und Abbrüchen steht noch aus.

Nach den Karten-Ladeskripten wird die Gastfigur einmalig exakt auf den gespeicherten Spawn gesetzt. Erst danach endet `joinWarpPending` und die normale Bewegungssynchronisation beginnt. `ContinueJoinedGame()` ruft anschließend `InitMap()` auf, damit `LoadSavedMapView()` nicht den alten Host-Ausschnitt mit der neuen Gastposition kombiniert.

Nach **A** ist die Auswahl im Suchmenü während des Beitritts gesperrt. **B** kann weiterhin abbrechen.

## Kollisionen und Menüs

- `LOCALID_COOP_PLAYER` ist im zentralen Kollisionscheck ausgenommen: Die Verbindungsfigur blockiert keine Bewegung. Terrain, NPCs und normale Kollisionen bleiben unverändert.
- Die Figur besitzt kein NPC-Script; ein Dialog wird nicht ausgelöst.
- Menüs, Beutel, PC und Kämpfe verwenden getrennte Engine-Callbacks und keine gemeinsamen Menüzustände im Koop-Modul.
- Diese neuen Regeln sind gebaut, aber noch nicht vom Nutzer im Laufzeittest bestätigt.

## Sprint 4: PC-Sperre

- Der Host vergibt die Sperre nur an eine Rolle. Gastanfragen werden erst nach vollständiger Spielstandübernahme beantwortet.
- `ShowPokemonStorageSystemPC()` fragt die Berechtigung an. Der Gast öffnet den PC erst nach positiver Antwort im regulären Welt-Frame.
- `CB2_ExitPokeStorage()` startet die atomare Übernahme; die Bestätigung gibt die Sperre frei. Stopp, F6, Softreset und `CtrCoop_PeerLeft()` setzen sie ebenfalls zurück.
- Offlineverhalten und PC-Untermenüs bleiben unverändert. Der Laufzeittest der Vergabe und Bestätigung steht noch aus.

## Szenario-Fortschritt

- Persistente Story-Flags und Skriptvariablen werden alle 15 Frames mit einer kleinen Baseline verglichen. Auch direkte Schreibzugriffe über `GetVarPointer()` werden dadurch erfasst.
- Änderungen gehen an die Gegenseite. Der Host übernimmt Gaständerungen und spiegelt sie anschließend als Hostzustand zurück.
- Empfangene Werte aktualisieren Spielstand und Baseline zugleich, sodass keine Paketschleife entsteht. Höchstens acht Änderungen pro Intervall begrenzen die Netzwerk-Warteschlange.
- Temporäre Spezialflags und Spezialvariablen werden nicht übertragen. Der Laufzeittest mit einem konkreten Szenario steht noch aus.

## Noch offen

- Teamänderungen außerhalb des PCs, Items, Objekte und sonstige laufende Spielstandsänderungen synchronisieren; PC-Zugriffe und der Initialzustand werden bereits vollständig übertragen.
- Beim Verbindungsende die aktuelle Gruppe einschließlich später Kampfänderungen in die Gast-Reihe zurücklegen; derzeit enthält sie den letzten bestätigten PC-Stand.
- Den neuen Beitritt mit Startteam-Auswahl, Host-Bestätigung, Rejoin und Abbruch mit zwei Azahar-Instanzen abnehmen.
- Die Sperren der Host- und Gast-Reihe sowie die Migration eines bereits gefüllten Box-14-Bestands im Emulator prüfen.
- Unabhängige Einzelkämpfe und später gemeinsame Doppelkämpfe implementieren.
- Dritte Konsole und bestehende Link-Theken erneut prüfen.
- Tests auf zwei realen 3DS-/2DS-Konsolen durchführen.
- Durchlässigkeit, unabhängige Menüs und Szenario-Synchronisation aus Sprint 3 ebenfalls abnehmen.

Sprint 3 und Sprint 4 dürfen erst abgeschlossen werden, wenn alle ihre Akzeptanzkriterien erfüllt sind.

## Nächste Arbeitsschritte

1. Auf `main` Durchlässigkeit, fehlende NPC-Dialoge und unabhängige Menüs testen.
2. Auf `sprint-4` einen neuen Gast beitreten lassen, ein Startteam wählen und erst nach der Host-Bestätigung die Welt betreten.
3. Mit demselben Gastnamen erneut beitreten und das gespeicherte Team sowie den Spawn prüfen; die lokale Gast-SAV muss unverändert bleiben.
4. PC-Sperre, atomare Übernahme und gesperrte Spieler-Reihen auf beiden Konsolen testen, anschließend Abbrüche während der Teamwahl und Übertragung prüfen.
5. `Status.md` und diese Datei nach bestätigten Tests aktualisieren.

## Build- und Testregeln

- Quelle liegt im Repository unter `3ds_port/`; `build/upstream/` wird nicht committed.
- Vor einem inkrementellen Build geänderte Portdateien auch nach `build/upstream/3ds_port/` kopieren.
- `main_menu.c` wird im Buildbaum geändert und muss mit `patches/pokeemerald/0083-local-coop.patch` übereinstimmen.
- Patchkontrolle: `git -C build/upstream apply --check --reverse ../../patches/pokeemerald/0083-local-coop.patch`.
- Vor dem Abschluss: `make -C build/upstream/3ds_port release pak`, `make -C 3ds_port verify-coop`, `git diff --check` und `python tools/release_audit.py --repo . --strict`.
- ROMs, Spielstände, PAKs, Build-Artefakte und Emulatorprofile niemals committen oder hochladen.

## Fortsetzung in einer neuen Paseo-Sitzung

> Lies zuerst `AGENTS.md`, `HANDOFF.md`, `SETUP.md` und `Status.md`. Für den Sprint-3-Test `main` verwenden; die neue Koop-Box und Startteam-Auswahl liegen auf `sprint-4`. Vor einem Build die Portdateien des gewählten Branches nach `build/upstream/3ds_port/` kopieren. Keine ROMs, Saves oder Build-Artefakte committen. Die neuen Sprint-4-Abläufe sind gebaut, aber noch nicht mit zwei Azahar-Instanzen abgenommen. Halte die Dokumentation aktuell und teste jede Änderung im Emulator.
