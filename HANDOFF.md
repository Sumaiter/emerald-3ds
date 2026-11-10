# Übergabe: Emerald 3DS Local Co-op

Stand: 2026-11-09

Dieser Stand ist eine Entwicklungsversion, **keine vollständig spielbare Alpha**. Sprint 1 und Sprint 2 sind geprüft. Sprint 3 ist in Arbeit; Host-Spielstand, Spawn, Kartenwechsel und Bewegung funktionieren im Emulator.

## Repository

- Fork: `https://github.com/Sumaiter/emerald-3ds`
- Original: `https://github.com/ZallaxDev/emerald-3ds`
- Veröffentlichung und Ersteinrichtung für den Sprint-3-Test erfolgen über `main` (`6fbae82 Make co-op avatar non-blocking`).
- Sprint 4 wird getrennt auf `sprint-4` entwickelt. Die Koop-Box, Startteam-Auswahl, atomare PC-Übertragung und Reihensperren sind implementiert, aber noch nicht im Emulator abgenommen.
- Sprint 5 baut auf diesem Stand auf und wird auf `sprint-5` entwickelt: unabhängige Kämpfe, Trainerreservierung und Battle-Ergebnissynchronisation.
- Sprint 6 wird auf `sprint-6` entwickelt: gemeinsame Doppelkämpfe mit dem bestehenden Link-Controller und einem Koop-eigenen Start-Handshake.
- Vor einem Build die Portdateien des gewählten Branches nach `build/upstream/3ds_port/` kopieren; dieser Arbeitsbaum wird von allen Branches gemeinsam verwendet.

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
- `patches/pokeemerald/0084-double-coop.patch`

Das Protokoll hat Version 8. Es enthält Authentifizierung, Identität, Spawn, Snapshot-Beginn, Chunk-Anforderung, Chunk, Übernahmebestätigung, PC-Anfrage, Vergabe und Freigabe, atomare PC-Übertragungen, Trainerreservierungen, Battle-Ergebnisse, Doppelkampf-Setup und Änderungen von Story-Flags und Skriptvariablen. Chunks werden nur auf Anforderung gesendet; dadurch läuft die Netzwerk-Warteschlange nicht voll.

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

## Sprint 5: Unabhängige Kämpfe

- Wilde Kämpfe starten auf beiden Konsolen lokal und unabhängig mit der jeweiligen Gruppe. Die andere Konsole bleibt gleichzeitig in der Oberwelt oder kann selbst kämpfen.
- `CB2_InitBattle` fragt bei Gast-Trainerkämpfen zuerst die Trainer-ID beim Host an. Der Host vergibt jede ID nur einmal; auch seine eigene ID bleibt nach dem Kampf bis zur Story-Synchronisation reserviert.
- Vor der Freigabe muss bei einem Sieg `TRAINER_FLAGS_START + trainerId` gesetzt sein. Der Gast sendet dieses Flag vor der Freigabe, damit keine zweite Herausforderung oder doppelte Belohnung zwischen Kampfende und Story-Synchronisation entsteht.
- Beim Kampfbeginn werden `SaveBlock1` und `PokemonStorage` als Baseline gespeichert. Nach dem Kampf enthält die Transaktion die vollständige Gruppe des Auslösers, geänderte Boxplätze sowie Geld-, Münz- und Item-Deltas.
- Fangdaten werden nur hinzugefügt, nicht gelöscht. Ist ein Zielplatz inzwischen belegt, sucht die Übernahme vor dem Schreiben einen anderen freien Platz. Reicht der Platz nicht, wird nichts übernommen und die Übertragung später erneut versucht.
- Item- und Geld-Deltas werden auf den aktuellen Empfängerzustand angewendet. Dadurch überschreibt ein später Kampfabschluss keine zeitgleichen Änderungen des anderen Spielers.
- `ReturnFromBattleToOverworld` startet die Übernahme nach Levelaufstiegen und Evolutionen. Bei Niederlage oder Unentschieden wartet sie bis nach `DoWhiteOut()`, damit Heilung, Geldabzug und Rückkehr enthalten sind.
- Battle- und PC-Übertragungen besitzen getrennte Puffer und Bestätigungen. Die vollständige Azahar-Abnahme mit gleichzeitigen Kämpfen, Fängen, Whiteout und derselben Trainer-ID steht noch aus.

## Sprint 6: Gemeinsame Doppelkämpfe

- Ein Doppelkampf wird nur gemeinsam gestartet, wenn beide Figuren auf derselben Karte und höchstens drei Felder entfernt sind und kein anderer Kampf, PC oder Handshake aktiv ist. Andernfalls startet der originale Einzelspieler-Doppelkampf.
- Host und Gast können auslösen. Die Anfrage wird mit Annahme, Ablehnung, Setup-Chunks und Startbestätigung beantwortet; verlorene Startpakete werden wiederholt.
- Das Setup umfasst **1.232 Byte**: Kampfparameter, gemeinsamer RNG-Start, Gegnergruppe und je drei Pokémon von Host und Gast. Die übrigen drei Plätze jeder Originalgruppe bleiben für die Rückgabe gesichert.
- Nach dem Setup verwendet die Engine ihre vorhandene Zwei-Spieler-Controller-Anordnung. Der Host steuert die linke Position, der Gast die rechte; beide Gegner bleiben KI-gesteuert. Die ursprüngliche Link-Lobby und das Party-Auswahlmenü werden nicht geöffnet.
- Züge, Ziele, Wechsel, Items, Ohnmacht, Flucht und Kampfende laufen über den vorhandenen direkten Battle-Kanal innerhalb der Koop-Sitzung. `CloseLink()` wird für Koop nicht aufgerufen.
- Nach dem Kampf werden die eigenen drei Plätze zurückübernommen. Nur der Auslöser überträgt Belohnungen. Da beide Engines Geld und Items bereits lokal verrechnet haben, überspringt die Bestätigung diese Deltas beim Partner und verhindert Doppelzählungen.
- Niederlagen warten wie in Sprint 5 auf `DoWhiteOut()`. Die vollständige Abnahme mit beiden Auslösern, Zügen, Wechseln, Fängen, Whiteout und dem Fallback ohne Partner steht noch aus.

## Noch offen

- Nicht kampfbedingte Teamänderungen außerhalb des PCs, Objekte und sonstige laufende Spielstandsänderungen synchronisieren; PC-Zugriffe, Kampfergebnisse und der Initialzustand werden bereits übertragen.
- Beim Verbindungsende während einer unbestätigten Übertragung die endgültige Gruppe zuverlässig zurücklegen; die Reihe enthält den letzten bestätigten PC- oder Kampfstand.
- Den neuen Beitritt mit Startteam-Auswahl, Host-Bestätigung, Rejoin und Abbruch mit zwei Azahar-Instanzen abnehmen.
- Die Sperren der Host- und Gast-Reihe sowie die Migration eines bereits gefüllten Box-14-Bestands im Emulator prüfen.
- Unabhängige Einzelkämpfe mit Fang, Niederlage und Trainerreservierung sowie gemeinsame Doppelkämpfe mit beiden Auslösern, Wechseln und Whiteout abnehmen.
- Dritte Konsole und bestehende Link-Theken erneut prüfen.
- Tests auf zwei realen 3DS-/2DS-Konsolen durchführen.
- Durchlässigkeit, unabhängige Menüs und Szenario-Synchronisation aus Sprint 3 ebenfalls abnehmen.

Sprint 3, Sprint 4, Sprint 5 und Sprint 6 dürfen erst abgeschlossen werden, wenn alle ihre Akzeptanzkriterien erfüllt sind.

## Nächste Arbeitsschritte

1. Auf `main` Durchlässigkeit, fehlende NPC-Dialoge und unabhängige Menüs testen.
2. Auf `sprint-4` einen neuen Gast beitreten lassen, ein Startteam wählen und erst nach der Host-Bestätigung die Welt betreten.
3. Mit demselben Gastnamen erneut beitreten und das gespeicherte Team sowie den Spawn prüfen; die lokale Gast-SAV muss unverändert bleiben.
4. PC-Sperre, atomare Übernahme und gesperrte Spieler-Reihen auf beiden Konsolen testen, anschließend Abbrüche während der Teamwahl und Übertragung prüfen.
5. Auf `sprint-5` gleichzeitig verschiedene wilde Kämpfe durchführen, einen Fang auf einem vollen Zielplatz prüfen und EP, Items und Geld auf beiden Seiten vergleichen.
6. Dieselbe Trainer-ID zeitgleich anfordern, danach Sieg, Niederlage mit Whiteout und erneute Herausforderung erst nach Flag-Übernahme prüfen.
7. Auf `sprint-6` Doppelkämpfe von beiden Seiten auslösen, Züge und Wechsel getrennt steuern, Fang und Whiteout prüfen sowie den Fallback bei zu großer Entfernung testen.
8. `Status.md` und diese Datei nach bestätigten Tests aktualisieren.

## Build- und Testregeln

- Quelle liegt im Repository unter `3ds_port/`; `build/upstream/` wird nicht committed.
- Vor einem inkrementellen Build geänderte Portdateien auch nach `build/upstream/3ds_port/` kopieren.
- `main_menu.c` wird im Buildbaum geändert und muss mit `patches/pokeemerald/0083-local-coop.patch` übereinstimmen.
- Patchkontrolle zuerst für `0084-double-coop.patch`, danach für `0083-local-coop.patch`, jeweils mit `git -C build/upstream apply --check --reverse` ausführen.
- Vor dem Abschluss: `make -C build/upstream/3ds_port release pak`, `make -C 3ds_port verify-coop`, `git diff --check` und `python tools/release_audit.py --repo . --strict`.
- ROMs, Spielstände, PAKs, Build-Artefakte und Emulatorprofile niemals committen oder hochladen.

## Fortsetzung in einer neuen Paseo-Sitzung

> Lies zuerst `AGENTS.md`, `HANDOFF.md`, `SETUP.md` und `Status.md`. Für den Sprint-3-Test `main`, für Team und PC `sprint-4`, für unabhängige Kämpfe `sprint-5` und für gemeinsame Doppelkämpfe `sprint-6` verwenden. Vor einem Build die Portdateien des gewählten Branches nach `build/upstream/3ds_port/` kopieren. Keine ROMs, Saves oder Build-Artefakte committen. Die neuen Abläufe sind gebaut, aber noch nicht mit zwei Azahar-Instanzen abgenommen. Halte die Dokumentation aktuell und teste jede Änderung im Emulator.
