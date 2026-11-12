# Übergabe: Emerald 3DS Local Co-op

Stand: 2026-11-12

Dieser Stand ist eine Entwicklungsversion, **keine vollständig spielbare Alpha**. Sprint 1 bis Sprint 4 sind geprüft und auf `main` zusammengeführt. Die unten dokumentierten Join-, NPC-, Starter-, PC-/Box-, Menü-, Gruppen-, Abbruch- und Wiederbeitrittstests wurden mit zwei isolierten Azahar-Instanzen ausgeführt. Hardwaretests bleiben offen. Sprint 5 wurde noch nicht begonnen. Der Branch `sprint-4` enthält weiterhin den abgenommenen Quellstand; neuere Multiplayer-Korrekturen aus dem bisherigen Arbeitsstand wurden erhalten.

## Übergabe auf main vom 2026-11-12

- Geprüfter Sprint-4-Commit: [`a976ce4`](https://github.com/Sumaiter/emerald-3ds/commit/a976ce4a54a6b0f4b166ad4aa0ff29256ed0eb65).
- Merge nach `main`: `777e778` (`Merge sprint-4: complete co-op storage, PC locking and regression tests`). Der Merge-Inhalt entspricht vollständig dem geprüften Sprint-4-Commit; die bisherigen Änderungen von `main` bleiben enthalten.
- GitHub-Ziele dieser Übergabe: `main` mit dem zusammengeführten Stand bis einschließlich Sprint 4 und `sprint-4` mit dem abgenommenen Quellstand. Implementierungen aus den Branches `sprint-5` bis `sprint-8` werden nicht übernommen.
- Erneute Prüfung des zusammengeführten Stands: alle neun `verify-coop`-Testsuiten, Reverse-Patchkontrolle, Vorwärts-/Rückwärtsprüfung des Integrationspatches, strikter Release-Audit und `git diff --check`. Der bereits vollständig gebaute und im Emulator geprüfte Quellstand bleibt erhalten (Build `e6f233d528c8db82`, PAK-ABI `3a8d61b5`).
- Übertragen werden ausschließlich Quellcode, Tests, Integrationspatches und Dokumentation. ROMs, SAV-Dateien, Emulatorprofile, PAKs und Build-Artefakte bleiben lokal und ignoriert.
- Als Nächstes ist Sprint 5 vorgesehen. Vor dem Weiterarbeiten den vorhandenen Branch `origin/sprint-5` prüfen und den geprüften Stand aus `main` erhalten. Die Grenze bleibt vorerst Host plus ein Gast; Host plus drei Gäste gehört zum späteren Ausbau.

## Multiplayer-Review vom 2026-11-12

Nur die Multiplayer-Erweiterungen seit der Fork-Basis `bcae4b7` wurden bearbeitet:

- Der Gast-Speicherschutz bleibt nach Übernahme der Host-Welt auch bei Abbruch, Abschalten des Koop-Modus und Verbindungsverlust bis zum Soft-Reset aktiv. Ein abgebrochener Suchvorgang ohne übernommene Host-Daten sperrt den lokalen Spielstand anschließend nicht.
- Starterauswahl und Startteam-PC warten im Menü auf den vollständigen Host-Snapshot und die Join-Information. Die Starterbestätigung prüft diese Voraussetzungen ebenfalls.
- Koop-Pakete werden nur vom aktuellen, authentifizierten Mitspieler verarbeitet. Unaufgeforderte und doppelte Authentifizierungsbestätigungen starten keinen neuen Snapshot.
- Eine PC-Freigabe darf nur die Sperre des entfernten Besitzers nach abgeschlossener Übertragung lösen. Beim Übernehmen bleiben die aktuellen eigenen Teamdaten in der eigenen Koop-Reihe erhalten.
- Bei fehlgeschlagenem Ersetzen der Profildatei bleibt die vollständige temporäre Kopie zur Wiederherstellung erhalten. Die Prüfsumme wird vor jeder Normalisierung geprüft; auch eine beschädigte Hauptdatei erlaubt die Wiederherstellung aus einer gültigen temporären Datei.
- Positionspakete mit Blickrichtung null und unbekannter Bewegungsaktion werden verworfen.

`3ds_port/tests/coop_runtime_test.c` führt den tatsächlichen Koop-Code mit simulierten Engine-/Transportfunktionen aus und prüft Join-Reihenfolge, Speicherschutz, Authentifizierung sowie vollständige, atomare PC-Übertragungen. Hinzu kommen Tests für gleichzeitige PC-Anfragen, Menü-Sprites und aus einem Save wiederhergestellte Verbindungsfiguren. Die Tests sind in `verify-coop` integriert. Die ergänzende Sprint-4-Abnahme ist weiter unten dokumentiert.

Prüfstand: `verify-coop`, vollständiger 3DS-Build (`release pak`), Reverse-Patchkontrolle, `git diff --check` und strikter Release-Audit bestanden. Die erneut ausgeführte Builder-Suite meldet 241 Tests, davon 239 bestanden und zwei übersprungen (private Kopie bzw. gesonderter gepatchter Quellbaum fehlen dem jeweiligen Test).

## Laufzeittests vom 2026-11-12

Getestet wurde mit zwei Azahar-2126.2-Instanzen, einem eigenen lokalen Testraum und separaten Kopien der Profile und SAV-Dateien. Die ursprünglichen Spielstände und laufenden Benutzerinstanzen wurden nicht für Eingaben oder Schreibzugriffe verwendet. Speicherzustände wurden zusätzlich über die Debugschnittstelle geprüft.

Bestätigt:

- Suche, Passwort-Beitritt, vollständiger Host-Snapshot mit 53.380 Byte und Prüfsummenprüfung.
- Neuer Gast: Geckarbor auf Level 5, Gruppe mit genau einem Pokémon, PC-Übernahme mit 34.344 Byte und Bestätigung vor dem Spawn in Wurzelheim.
- Gespeicherter Name: Starterauswahl wird übersprungen, Team und gespeicherter Spawn `18,17` werden wiederhergestellt. Das wurde auch mit dem abschließenden Build `4d1d9185242e1fe0` bestätigt.
- Bewegung in beide Richtungen, Host-Kartenwechsel aus dem Haus in die Stadt und gegenseitige Anzeige auf derselben Karte. Beide Figuren können dieselbe Kachel betreten.
- Position und Blickrichtung der drei sichtbaren NPCs stimmen überein; die Gast-NPCs sind eingefroren.
- Der Host bewegt sich weiter, während der Gast den Beutel geöffnet hat. Der Host kann seine Test-SAV speichern; die Gast-SAV bleibt per SHA-256 bytegenau unverändert.
- Hartes Beenden des Host-Prozesses: Verbindungsverlust wird erkannt, die Hostfigur verschwindet beim Gast und der Gast-Speicherschutz bleibt aktiv. Soft-Reset hebt den Schutz wieder auf.
- Die zuvor gespeicherte Gastfigur wird beim Laden des Host-Saves ohne aktive Koop-Sitzung entfernt.
- Drei Beutel-Auf-/Zu-Zyklen bleiben bei laufender NPC-Synchronisation stabil; diese Wiederholung wurde auch im abschließenden Build ausgeführt.

Im Laufzeittest gefundene und korrigierte Fehler:

- NPC-Pakete konnten beim Verlassen des Beutels `ObjectEventTurn()`/`SeekSpriteAnim()` auf inzwischen vom Menü verwendeten Sprites ausführen und zum Absturz an Adresse null führen. NPC- und Avatar-Spritezugriffe sowie deren Bereinigung sind jetzt auf `CB2_Overworld` begrenzt. Wiederholte Beutel-Auf-/Zu-Tests sichern die Rückkehr ab; NPC-Rundlaufpakete aktualisieren den Gast danach wieder.
- Eine in `SaveObjectEvents()` mitgespeicherte Verbindungsfigur blieb nach dem Laden ohne Sitzung sichtbar. Die Bereinigung findet jetzt auch wiederhergestellte Figuren, deren Laufzeit-Slot dem Koop-Modul noch nicht bekannt ist.

Der im Laufzeittest gefundene Fehler bei einem fünften Gastnamen wird durch eine Tabelle mit zehn Einträgen behoben. Festlegung des Nutzers: zehn gespeicherte Gastnamen; keine Verdrängung bestehender Positionen. Der aktuelle Transport bleibt bewusst bei Host plus einem Gast. Host plus drei gleichzeitige Gäste ist das Ziel des nächsten Ausbaus, kein bereits implementiertes Merkmal.

Die Migration von Tabellen-Version 1 auf 2 erhält die ersten vier Einträge sowie die Initialisierungsmarker der Koop-Box. Neue Einträge nutzen denselben reservierten Bereich des Host-Spielstands; die SAV- und Snapshot-Größe bleibt unverändert. Ein unbekannter elfter Name wird vor der Snapshot-Übertragung abgewiesen. Der Suchbildschirm zeigt `10 GASTNAMEN BELEGT` und `BEKANNTEN NAMEN WAEHLEN`; B kehrt zurück. Bekannte Namen bleiben auch bei voller Tabelle zugelassen. Protokoll-Version 8 enthält dafür die eigene Meldung `CTR_COOP_NAME_LIMIT_PACKET`.

Mit Build `5d6a47e821a7a845` wurden zusätzlich im Emulator bestätigt: verlustfreie Migration eines alten Saves mit vier belegten Einträgen, Anlage und tatsächliches Speichern eines fünften Namens in der Host-SAV sowie Ablehnung eines elften Namens mit sichtbarer Fehlermeldung bei einer vorbereiteten vollen Testliste. Die volle Liste und die Gast-SAV bleiben bei der Ablehnung unverändert. Native Verhaltenstests prüfen außerdem alle zehn Positionen nach Wiederherstellung des Save-Blocks und den Beitritt eines bekannten Namens bei voller Liste. Die Integrationspatch-Datei wurde auf isolierten Quellkopien rückwärts und vorwärts angewandt und mit dem Buildbaum verglichen.

## Laufende Gruppensynchronisierung und Ausschalten des Gasts

Vorgabe des Nutzers: Die aktuelle Gruppe muss während des Spielens in der Koop-Box gespiegelt werden. Der Gast meldet sich durch Ausschalten ab; ein gesondertes Abmeldepaket wird nicht vorausgesetzt.

- Beide Rollen spiegeln ihre eigene aktuelle Gruppe in die eigene Reihe. Änderungen der 600 Byte Gruppendaten werden automatisch übertragen; unveränderte Daten erzeugen keinen erneuten Transfer.
- Protokoll 9 ergänzt `PARTY_BEGIN`, `PARTY_CHUNK` und `PARTY_RESULT`. Ein eigener Sendepuffer hält die zu bestätigende Gruppe stabil, während weitere Änderungen bereits auftreten können. Die Gegenseite übernimmt ausschließlich die entfernte Reihe nach vollständigem Empfang und gültiger Prüfsumme. Nach Bestätigung folgt bei Bedarf der neuere Stand.
- PC-Vorgänge und Gruppenübertragung schließen einander aus. Ein Pokémon am PC-Cursor ist noch kein abgeschlossener Vorgang. Die eigene aktive Gruppe wird durch einen entfernten PC-Snapshot nicht zurückgesetzt. Abgewiesene PC-Anfragen setzen das wartende Feldskript im folgenden Frame fort; das gilt auch für gleichzeitige Anfragen und Disconnect während einer Anfrage.
- Nach erkanntem Disconnect bleibt die letzte vollständig übernommene Gast-Reihe erhalten und wird entsperrt. Die Host-Reihe bleibt auch bei ausgeschaltetem Funk gegen Entnahme ihrer gespiegelt vorhandenen Pokémon gesperrt, um Duplikate zu vermeiden.
- Das Spiegeln aktualisiert zunächst den RAM des Hosts. Der normale Host-Speichervorgang schreibt den gemeinsamen Stand auf die SD-Karte. Daten, die beim Ausschalten noch nicht vollständig angekommen sind, können nicht zugesichert werden; ein Teiltransfer ersetzt niemals die bisherige Reihe.

Mit zwei isolierten Emulatoren geprüft: gültige Änderungen an EP (`135 → 136`), AP (`35 → 34`) und getragenem Item (`0 → 13`) in der Gastgruppe. Die Änderungen erschienen ohne PC-Abschluss bytegenau in der Host-Reihe. Anschließend wurde der Gast-Prozess hart beendet. Der Host behielt die aktualisierte Reihe und entfernte die Gastfigur. Nach normalem Host-Speichern wurde die aktualisierte Reihe direkt in der SAV geprüft; nach Soft-Reset und erneutem Laden stimmte sie weiterhin bytegenau überein. Diese Änderungen waren gezielte Debug-Testdaten, kein bereits abgenommener Kampfablauf aus Sprint 5.

Native Tests prüfen zusätzlich Änderungen während einer noch offenen Bestätigung, gleichzeitige Übertragung beider Gruppen, fehlerhafte Prüfsummen, Verbindungsabbruch nach einer vollständigen beziehungsweise einer unvollständigen Übertragung, PC-Konflikte und die Fortsetzung abgewiesener PC-Skripte. Vor der abschließenden Sprint-4-Abnahme wurde dafür Build `129b2efb9dbb3219` einschließlich Datenpaket (ABI `2729bb0c`) geprüft; sein erneuter Start und das Laden der aktualisierten Gast-Reihe bestanden. Dieser frühere Stand umfasste acht Testsuiten. Den neueren Build und die vollständige PC-Abnahme dokumentiert der folgende Abschnitt.

## Sprint-4-Abnahme vom 2026-11-12

Zusätzlich behobene Fehler:

- Ein direkter Ausstieg aus dem PC-Auswahlmenü gab die Sperre bisher nicht frei. Dagegen konnte die Rückkehr aus der Box die Sperre zu früh freigeben. Jetzt umfasst eine Transaktion den gesamten PC-Besuch einschließlich des Auswahlmenüs. Der endgültige Menüausstieg überträgt den abgeschlossenen Zustand; die bestätigte Übernahme gibt die Sperre frei. Die anfängliche Team-Auswahl kann auch ohne Öffnen der eigentlichen Box direkt beendet werden.
- B wurde während der anfänglichen PC-Auswahl gleichzeitig vom übergeordneten Beitrittstask verarbeitet und brach den Beitritt ab. Der Beitrittstask ignoriert B nun bei geöffnetem PC. Ein Test mit dem echten Menü- und Taskcode sichert das ab.
- Eine gesperrte Entnahme wurde erst tief in der Cursor-Animation abgewiesen; nachfolgende Animationsschritte konnten dadurch einen nicht vorhandenen bewegten Pokémon-Sprite verwenden. Die Eingabe wird jetzt vor dem Start der Operation abgewehrt. Auch automatische Auswahl kann keine Mehrfachauswahl in der geschützten Box beginnen.
- Ein beitretender Gast wartet auf das Ende eines offenen Host-PC-Besuchs, damit kein Pokémon am Cursor in einem unvollständigen Snapshot verschwindet. Ein Gast-Abbruch beendet einen offenen lokalen Host-PC nicht. Der Host kann den Besuch anschließend vollständig abschließen.
- Beim Wiederbeitritt werden Lücken in der Gast-Reihe geschlossen. Andernfalls zählte die Engine nach Entnahme des ersten Pokémon eine leere Gruppe, obwohl spätere Reihenplätze noch belegt waren.

Abnahme mit fünf gültigen Pokémon unterschiedlicher Kennung in getrennten Emulator-SAVs:

| Prüfung | Ergebnis |
|---|---|
| Beitritt während geöffnetem Host-PC | Snapshot wartet bis zum abgeschlossenen Host-PC-Besuch |
| Leere Startgruppe per kontrollierter Testvorbereitung | Ausstieg abgewiesen, Auswahl bleibt geöffnet |
| Direkter B-Ausstieg aus der Startteam-Auswahl | Vollständige Übernahme und Spawn, kein Abbruch durch den Beitrittstask |
| Gegenseitige PC-Zugriffsversuche | Nur der Besitzer öffnet den PC; abgewiesene Spieler bleiben bedienbar |
| Normale und automatische Auswahl beider aktiver Spielerreihen | Keine Entnahme und kein veränderter Bestand |
| Gast entnimmt aus normaler Box; Host legt ein Pokémon ab | Nach Bestätigung stimmen beide Bestände überein; alle fünf Kennungen genau einmal vorhanden |
| Rückkehr aus der Box zum PC-Auswahlmenü | Sperre bleibt bestehen; noch keine vorzeitige Übernahme |
| Gast entnimmt ein weiteres Pokémon und wird vor dem PC-Abschluss hart beendet | Letzter bestätigter Host-Bestand bleibt unverändert; PC-Sperre wird freigegeben |
| Host entnimmt aus der freigegebenen Gast-Reihe | Einmalige Entnahme möglich; wiederholter Versuch erzeugt keine Kopie; Host-Spiegelung bleibt gesperrt |
| Host speichert, beide Emulatoren starten neu, Gast tritt wieder bei | Alle fünf Kennungen erhalten; der Gast erhält nur das verbliebene Pokémon, lückenlos an Gruppenposition 0 |

Der Host-Save wurde zusätzlich direkt geprüft: zwei Host-Pokémon, zwei Pokémon in normalen Boxen und ein verbliebenes Gast-Pokémon; jede Kennung genau einmal. Die Gast-SAV bleibt bytegenau unverändert. Die Testdaten liegen nur im ignorierten lokalen Testbereich.

Alle neun Testsuiten aus `verify-coop` bestehen, darunter die zusätzlichen Regressionstests des Sprint-4-Branches mit `CTR_COOP_TEST_MAX_SPRINT=4` und ohne erwartete Fehler. Neue native Prüfungen decken volle Boxen, unveränderte Solo-Boxen, Teilübertragungen, PC-Abbruch, verzögerten Snapshot und Lücken in der Gast-Reihe ab. Der abschließende Release-Build `e6f233d528c8db82` einschließlich PAK besteht (ABI `3a8d61b5`). Reverse- und Vorwärtsanwendung des Integrationspatches ergeben auf Kopien mit einheitlichen LF-Zeilenenden wieder dieselben Quelldateien. Strikter Release-Audit und `git diff --check` bestehen. Hardwaretests und die Kampfabläufe ab Sprint 5 bleiben offen.

## Repository

- Fork: `https://github.com/Sumaiter/emerald-3ds`
- Original: `https://github.com/ZallaxDev/emerald-3ds`
- Veröffentlichung und Ersteinrichtung erfolgen über `main`.
- Übergabestand bis Sprint 4: Merge `777e778`, geprüfter Quellstand `a976ce4`; Details stehen im Abschnitt „Übergabe auf main“.

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
- Gespeicherte Namen wurden nach Entfernung dieses Menüs wiederholt mit ihren gespeicherten Spawnpunkten geprüft.

## Implementierung

Geänderte Dateien:

- `3ds_port/include/3ds_coop.h`
- `3ds_port/include/3ds_coop_protocol.h`
- `3ds_port/src/3ds_coop.c`
- `3ds_port/src/3ds_coop_protocol.c`
- `3ds_port/tests/coop_protocol_test.c`
- `patches/pokeemerald/0083-local-coop.patch`

Das Protokoll hat Version 9. Es enthält Authentifizierung, Identität, Starteranforderung, Spawn, Snapshot-Beginn, Chunk-Anforderung, Chunk, Übernahmebestätigung, NPC-Positionen, PC-Sperre, atomare PC-Übertragungen, laufende Gruppenübertragung sowie Story-Flags und Skriptvariablen. Große Snapshot- und PC-Transfers senden Chunks nur auf Anforderung. Kleine Gruppenänderungen senden einen eingefrorenen Stand in sechs zuverlässigen Nachrichten und warten dann auf Bestätigung. `PrepareHostSave()` ruft vor dem Snapshot `SaveObjectEvents()` auf, damit er die aktiven NPC-Positionen statt des letzten Disk-Speicherstands enthält. Danach sendet der Host Änderungen sofort und alle sichtbaren NPCs im Rundlauf erneut. Der Gast übernimmt deren Position, Blickrichtung und Grafik und friert die unabhängige lokale NPC-Bewegung ein. Position und Blickrichtung wurden im Laufzeitvergleich bestätigt.

Die namensbasierte Spawntabelle liegt in `gSaveBlock1Ptr->unused_3598`, mit Magic `0x5350`, Version `2` und zehn Einträgen. Version 1 mit vier Einträgen wird automatisch migriert; dabei werden auch die verschobenen Koop-Box-Marker erhalten. Die gesamte Erweiterung belegt 248 der 384 reservierten Bytes. Positionspakete verwenden ObjectEvent-Koordinaten einschließlich `MAP_OFFSET`; `SetWarpDestination()` erhält die Koordinaten ohne diesen Rand.

Vor `CB2_ContinueSavedGame()` setzt `ContinueJoinedGame()` über `CtrCoop_EnterHostWorld()` das Warp-Ziel und leert `gSaveBlock1Ptr->mapView`. Dadurch kann `InitMapFromSavedGame()` keinen Ausschnitt der alten Hostposition über die frisch geladene Gastkarte kopieren. Bei einem Spawn auf einer anderen Karte entfernt der Join zusätzlich aktive NPC-Objekte der alten Karte und lädt die Objektvorlagen des Ziels; das lokale Spielerobjekt bleibt erhalten. Der normale Continue-Pfad lädt Karte, Skripte und Spieler genau einmal. Nach den Karten-Ladeskripten setzt `CtrCoop_OverworldFrame()` die Gastfigur exakt auf den gespeicherten Spawn. Erst danach endet `joinWarpPending` und die normale Bewegungssynchronisation beginnt.

Nach **A** ist die Auswahl im Suchmenü während des Beitritts gesperrt. Die ausgewählte Welt bleibt zwischengespeichert und sichtbar, auch wenn die Suche sie zwischenzeitlich nicht mehr liefert. Der Titel zeigt `VERBINDEN …`; der Hinweis auf eine fehlende Welt wird während des Beitritts nicht angezeigt. **B** kann weiterhin abbrechen.

## Sprint 4: Starter, Team und PC

- Ein neuer Gast ohne Spawn-Eintrag und ohne Pokémon in der Gast-Reihe wählt verbindlich Geckarbor, Flemmli oder Hydropi auf Level 5. Die Auswahl erzeugt genau ein Pokémon in der Gast-Reihe und im Team. `CB2_StarterSelectionActive()` verhindert, dass der Auswahltask im Verbindungsablauf erneut initialisiert wird. `CB2_InitStarterSelection()` gibt die Kontrolle an `CB2_MainMenu()` zurück, damit `RunTasks()` die Auswahl tatsächlich ausführt. `3ds_port/tests/starter_menu_test.c` prüft diesen Schedulerpfad, Hoch/Runter einschließlich Umbruch und die Bestätigung aller drei Starter mit dem echten Menü- und Taskcode. Der Empfang der Join-Info ist verbindliche Voraussetzung für den Beitrittsabschluss; Frameverarbeitung, PC-Öffnung und Weltbetreten bleiben bis dahin und bei neuen Gästen bis zur Starterwahl gesperrt. Danach öffnet der Startteam-PC; erst die bestätigte atomare PC-Übernahme löst den Spawn aus.
- Ein wiederkehrender Gast überspringt die Starterauswahl und verwendet gespeicherte Gast-Reihe und Spawnposition.
- Box 14 (Index `13`) ist die KOOP-Box. Reihe 1 (Indizes `0–5`) gehört dem Host, Reihe 2 (Indizes `6–11`) dem Gast. Bestehende Pokémon werden vor der Reservierung in normale Boxen migriert; ohne ausreichenden Platz wird die Welt nicht geöffnet.
- Eingeloggte Spielerreihen sind gegen Entnahme, Ablegen, Tausch, Freilassen und Mehrfachverschieben gesperrt. Nur eine Rolle darf den PC gleichzeitig verwenden.
- Der endgültige Ausstieg aus dem PC-Auswahlmenü überträgt vollständigen `PokemonStorage` und Gruppe mit Prüfsumme. Die Gegenseite übernimmt erst nach vollständigem Empfang und bestätigt danach; erst diese Bestätigung gibt die PC-Sperre frei. Beim Wechsel zwischen Box und Auswahlmenü bleibt die Sperre bestehen. Die anfängliche Team-Auswahl kehrt nach ihrem Abschluss direkt ins Beitrittsmenü zurück.
- Persistente Story-Flags und Skriptvariablen werden laufend synchronisiert. Sende- und Empfangsschleifen sind auf `FLAGS_COUNT` beziehungsweise `VARS_COUNT` begrenzt; die früheren Obergrenzen `SPECIAL_FLAGS_START` und `SPECIAL_VARS_START` überschrieben die Baseline-Puffer und nachfolgende Koop-Zustände wie Starterstatus und Sitzungsgeneration. Der Join-Flow-Test sichert alle vier Grenzen. Der Laufzeittest der neuen Abläufe steht noch aus.

## Kollisionen und Menüs

- `LOCALID_COOP_PLAYER` ist im zentralen Kollisionscheck ausgenommen: Die Verbindungsfigur blockiert keine Bewegung. Terrain, NPCs und normale Kollisionen bleiben unverändert.
- Die Figur besitzt kein NPC-Script; ein Dialog wird nicht ausgelöst.
- Menüs, Beutel, PC und Kämpfe verwenden getrennte Engine-Callbacks und keine gemeinsamen Menüzustände im Koop-Modul.
- Kollisionen, NPC-Interaktionen und unabhängige Menüs wurden vom Nutzer im Laufzeittest bestätigt.

## Noch offen

- Änderungen am gemeinsamen Beutel und anderen bisher nicht laufend übertragenen Inventardaten synchronisieren; Gruppen, Story-Flags und Skriptvariablen werden bereits laufend übertragen.
- Die Gruppenübertragung bei späteren echten Kampfabläufen einschließlich Fang und Niederlage prüfen.
- Unabhängige Einzelkämpfe und später gemeinsame Doppelkämpfe implementieren.
- Dritte Konsole und bestehende Link-Theken erneut prüfen.
- Tests auf zwei realen 3DS-/2DS-Konsolen durchführen.

Sprint 3 und Sprint 4 sind nach den dokumentierten Emulator- und nativen Tests abgeschlossen.

## Nächste Arbeitsschritte

1. Den bestätigten Basistest einschließlich Kartenwechsel, Kollisionen und Menüs nicht erneut als offenen Fehler behandeln.
2. Den abgenommenen Stand auf `main` und `sprint-4` erhalten; vorhandene spätere Sprint-Branches vor der nächsten Implementierung prüfen.
3. Als Nächstes Sprint 5 mit unabhängigen Einzelkämpfen und Trainerreservierungen beginnen.
4. `Status.md` und diese Datei nach bestätigten Tests aktualisieren.

## Build- und Testregeln

- Quelle liegt im Repository unter `3ds_port/`; `build/upstream/` wird nicht committed.
- Vor einem inkrementellen Build geänderte Portdateien auch nach `build/upstream/3ds_port/` kopieren.
- `main_menu.c` wird im Buildbaum geändert und muss mit `patches/pokeemerald/0083-local-coop.patch` übereinstimmen.
- Patchkontrolle: `git -C build/upstream apply --check --reverse ../../patches/pokeemerald/0083-local-coop.patch`.
- Vor dem Abschluss: `make release pak`, `make -C 3ds_port verify-coop`, `git diff --check` und `python tools/release_audit.py --repo . --strict`.
- ROMs, Spielstände, PAKs, Build-Artefakte und Emulatorprofile niemals committen oder hochladen.

## Fortsetzung in einer neuen Paseo-Sitzung

> Lies zuerst `AGENTS.md`, `HANDOFF.md`, `SETUP.md` und `Status.md`. Sprint 3 und Sprint 4 sind abgeschlossen und auf `main` zusammengeführt; `sprint-4` enthält den abgenommenen Quellstand. Prüfe vor Sprint 5 den vorhandenen Branch `origin/sprint-5` und erhalte die abgenommenen Korrekturen aus `main`. Zehn Gastnamen werden gespeichert; aktuell bleibt es bei Host plus einem Gast. Neue Namen starten in Wurzelheim, bekannte Namen an ihrem gespeicherten Spawn. Die aktuelle Gruppe wird laufend in die Koop-Box gespiegelt; der Gast beendet die Sitzung durch Ausschalten. Keine ROMs, Saves oder Build-Artefakte committen. Halte die Dokumentation aktuell und teste jede Änderung im Emulator.
