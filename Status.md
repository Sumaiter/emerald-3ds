# Status: Emerald 3DS Koop-Mod

Stand: 2026-11-10

## Ziel

Zwei Spieler nutzen über Nintendo-3DS-Local-Wireless (UDS) gemeinsam den Spielstand und die Welt des Hosts. Der Gast benötigt keinen eigenen Spielstand. Ein Online-Modus über Internet oder WLAN-Router ist nicht Bestandteil dieses Projekts.

## Statusregeln

- Einzelne Aufgaben werden mit `[x]` erst als erledigt markiert, wenn ihre Umsetzung tatsächlich geprüft wurde.
- Ein Sprint ist erst **fertig**, wenn alle zugehörigen Akzeptanzkriterien erfüllt und die relevanten Tests bestanden sind.
- Teilweise fertige Sprints bleiben **in Arbeit**. Nicht begonnene Sprints sind **offen**.
- ROMs, Spielstände, Datenpakete und andere urheberrechtlich geschützte Spieldaten werden niemals committed oder hochgeladen. Die vorhandenen Regeln in `.gitignore` bleiben erhalten.
- Vor jedem Push werden die tatsächlich übertragenen Dateien geprüft.

## Sprintübersicht

| Sprint | Inhalt | Status |
|---|---|---|
| 0 | Projekt einrichten und Ausgangsbasis prüfen | Fertig |
| 1 | Technischen Koop-Prototyp nachweisen | Fertig |
| 2 | Local-Wireless-Lobby und Passwort-Beitritt | Fertig |
| 3 | Gemeinsame Oberwelt und zweiten Spieler darstellen | In Arbeit |
| 4 | Spielerprofil, Mehrspieler-Box und PC-Sperre | Offen |
| 5 | Unabhängige Einzelkämpfe und gemeinsame Trainerwelt | Offen |
| 6 | Gemeinsame Doppelkämpfe | Offen |
| 7 | Abmeldung, Verbindungsabbrüche und sicheres Speichern | Offen |
| 8 | Hardwaretests, Stabilisierung und spielbare Alpha | Offen |

## Festgelegte Spielregeln

- Der Host öffnet seinen geladenen Spielstand mit einem Passwort für Local Wireless.
- Der Gast sucht im Hauptmenü nach Welten in Reichweite, wählt eine aus und gibt deren Passwort ein.
- Beim ersten Beitritt wartet der Gast gegebenenfalls auf den freien PC, wählt sein Startteam und danach Charakter und Namen.
- Ein neuer Gastname startet an einem festen, freien Punkt in Wurzelheim. Für einen bereits bekannten Gastnamen wird immer seine letzte gespeicherte Position verwendet; es gibt keine Spawn-Auswahl.
- Beide Spieler verwenden ausschließlich Pokémon, Gegenstände und Fortschritte des gemeinsamen Host-Spielstands.
- Nur ein Spieler darf gleichzeitig einen PC verwenden. Der andere kann währenddessen normal weiterspielen.
- Eine neue Mehrspieler-Box enthält pro Spieler eine Reihe für dessen Gruppe von bis zu sechs Pokémon.
- Die Boxenreihe eines eingeloggten Spielers ist gegen Entnahme und Verschieben gesperrt.
- Beim Verlassen des Spiels wird die Gruppe des Spielers vollständig in seine Boxenreihe zurückgelegt. Erst danach wird die Reihe entsperrt und ihre Pokémon können wieder am PC entnommen werden.
- Der Gast kann wilde Pokémon und Trainerkämpfe selbst auslösen.
- Einzelkämpfe bestreitet der auslösende Spieler allein mit seiner Gruppe. Der andere Spieler kann währenddessen weiterspielen.
- Bei einem Doppelkampf kämpfen beide Spieler gemeinsam, wenn der Partner auf derselben Karte, in unmittelbarer Nähe und verfügbar ist. Andernfalls bestreitet der auslösende Spieler den Doppelkampf nach den Originalregeln allein.
- Nur der Host schreibt den gemeinsamen Spielstand. Änderungen werden vollständig und in konsistenter Reihenfolge übernommen.

## Sprint 0: Projekt einrichten und Ausgangsbasis prüfen

**Status: Fertig**

### Erledigte Aufgaben

- [x] Originalprojekt `ZallaxDev/emerald-3ds` geklont.
- [x] Installations-, Builder- und deutsche ROM-Dokumentation geprüft.
- [x] Bereitgestellte ROM als unterstützte deutsche Originalversion `BPED` verifiziert.
- [x] ROM-SHA-1 `61c2eb2b380b1a75f0c94b767a2d4c26cd7ce4e3` bestätigt.
- [x] Windows-Builder und Payload der Release `v0.5.0` heruntergeladen und per SHA-256 geprüft.
- [x] Deutsches Datenpaket mit 6.814 Dateien und 419 Begleiter-Pokémon-Sprites erzeugt.
- [x] Datenpaket mit dem offiziellen Builder erfolgreich verifiziert: Release `v0.5.0`, ABI `734df4ff`.
- [x] Deutsche 3DSX mit der veröffentlichten Release-Prüfsumme abgeglichen.
- [x] Installierbares SD-Paket `Emerald3DS-v0.5.0-de-SD.zip` erzeugt und seine Archivintegrität geprüft.
- [x] Optionalen HOME-Menü-Forwarder aus der offiziellen Release heruntergeladen und per SHA-256 geprüft.
- [x] GitHub-Fork `Sumaiter/emerald-3ds` erstellt.
- [x] Bestehende `.gitignore`-Regeln für ROMs, Saves, CIA-Dateien und Datenpakete geprüft.

### Akzeptanzkriterien

- [x] Die lokale Ausgangsbasis kann mit der unterstützten deutschen ROM reproduziert werden.
- [x] ROM und erzeugte Spieldaten befinden sich nicht in der Git-Versionshistorie.
- [x] Der Fork enthält ausschließlich den Stand des öffentlichen Originalprojekts.

### Noch nicht durchgeführt

- Kein Koop-Quellcode wurde verändert oder implementiert.
- Die Installation wurde nicht auf einem realen 3DS gestartet.
- Local-Wireless-Mehrspielerfunktionen wurden noch nicht mit zwei Geräten getestet.

## Sprint 1: Technischen Koop-Prototyp nachweisen

**Status: Fertig**

### Aufgaben

- [x] Build-, Test- und Debugging-Umgebung für zwei Spielinstanzen eingerichtet. Die lokale MSYS2-/devkitPro-Toolchain und zwei Azahar-Instanzen mit getrennten SD-Umgebungen sind einsatzbereit.
- [x] Bestehende UDS-Sitzungen und den Wireless-Adapter als wiederverwendbare Grundlage abgegrenzt.
- [x] Eine eigenständige Koop-Sitzung außerhalb der bisherigen Link-Theken gestartet. `LOCAL_KIND_COOP`, Zwei-Spieler-Netzwerk und Start/Stopp über `LOCAL CO-OP` sind laufzeitgeprüft.
- [x] Host- und Gastrollen sowie Host-autoritative Nachrichten für den Prototyp festgelegt.
- [x] Kartenkennung, Position, Blickrichtung und Bewegungsanimation eines zweiten Spielers übertragen. Paketformat, Validierung, Heartbeat und Bewegungsentscheidungen sind durch `verify-coop` geprüft.
- [x] Eine ferne Testfigur auf derselben Karte erzeugt, aktualisiert und beim Trennen entfernt. ObjectEvent-Integration, Warp-, Menü- und Disconnect-Verhalten wurden vom Nutzer bestätigt.
- [x] Sichergestellt, dass der normale Einzelspielermodus ohne geöffnete Koop-Welt unverändert bleibt. Der Koop-Pfad ist opt-in und wurde vom Nutzer im Einzelspielermodus bestätigt.

### Bereits ausgeführte Prüfungen

- Vollständiger deutscher ARM-Build einschließlich Bootstrap, Lokalisierung und Voxel-Daten: bestanden; `build/upstream/3ds_port/emerald3ds.3dsx` wurde erzeugt.
- Vollständige englische Baseline mit `make verify`: bestanden; Locale-Tests anschließend separat erneut bestanden.
- Installierbares deutsches Release mit `make release pak`: bestanden; 3DSX, SMDH und Datenpaket liegen unter `Koop-Prototyp/3ds/emerald3ds`.
- Titelmenü-Erweiterung und Optionsanzeige mit `make release pak`: bestanden; `SPIEL SUCHEN` und `LOKALER KOOP` sind enthalten. Beide Azahar-Instanzen starten mit `RomFS boot marker PASS`.
- Datenpaket direkt per CRC geprüft: 6.393 Dateien, ABI `4f8eefe4`, alle Prüfsummen korrekt. Der offizielle 0.5.0-Builder wird nicht zur Verifikation dieses veränderten Entwicklungsstands verwendet.
- SPIEL SUCHEN ist im Titelmenü mit und ohne Spielstand vorhanden. Der Suchbildschirm zeigt SUCHE oben, ZURÜCK darunter und bis zu drei gleich große Ergebnisblöcke mit dem Spielernamen des Hosts. Hoch/Runter wählt ZURÜCK oder einen Host aus; A bestätigt die gezielte Sitzungs-ID, B bricht ab. LOKALER KOOP zeigt AUS oder AN. Suche, Hostname und gezielter Netzwerkbeitritt wurden vom Nutzer mit zwei Azahar-Instanzen bestätigt.
- MULTIPLAYEROPTIONEN ist mit und ohne Spielstand unter OPTIONEN erreichbar; mit Spielstand scrollt der fünfte Eintrag. Host- und Gast-Untermenüs enthalten 4-Tasten-Passwörter, Charakterauswahl und Gastname. Das Profil wird versioniert und atomar auf der SD-Karte gespeichert.
- Die Charakterauswahl zeigt 84 humanoide Overworld-Charaktere als 2D-Vorderansichten in einem 8x2-Raster mit 16 Figuren pro Box. Die originale PC-Hand markiert die Auswahl; nur die ausgewählte Figur ist animiert, und ihr Name steht in der Kopfleiste. Form, Animation, Handcursor und Menüdarstellung wurden vom Nutzer bestätigt. Wasser-/Link-Sonderfiguren wurden entfernt. Die 15 ausgeschlossenen Figuren ohne echte Laufbewegung sind samt Prüfnachweis in `ZEROWALK.md` dokumentiert. Das Avatar-Protokoll überträgt eine validierte Grafik-ID anstelle des Geschlechts. Die Passwortprüfung erfolgt zusätzlich zur UDS-Passphrase durch einen eigenen Authentifizierungsaustausch im Koop-Protokoll; erst nach Host-Bestätigung erscheint VERBUNDEN und Bewegungspakete werden akzeptiert. Beide Netzwerkpunkte sind laufzeitgeprüft.
- Der Gast wechselt nach der Authentifizierung automatisch in die Karte. Sein lokaler Spielstand bleibt dabei unverändert; der SHA-256 vor und nach dem Beitritt ist identisch. Speichern, Link-Speichern, Sondersektoren und Löschen sind für eine aktive Gast-Sitzung gesperrt.
- Cheatähnliche Spielregeln und Shiny-Rate sind nur für den Host veränderbar. Bereits aktivierte Werte werden beim Gast während der Sitzung ignoriert; die Sperre wurde vom Nutzer bestätigt.
- Beim Verlassen oder Neuladen des Gastes wird dessen Figur beim Host entfernt. Die Disconnect-Korrektur wurde vom Nutzer bestätigt.
- Separate ARM-Kompilierung der neuen Koop- und Optionsdateien mit den Produktionsflags: bestanden.
- `make -C 3ds_port verify-coop`: Protokoll- und Profiltests bestanden, einschließlich Persistenz, Prüfsummenfehler und ungültiger Passwortwerte.
- `python -m unittest discover -s builder/tests -v`: 241 Tests bestanden, sechs Umgebungs-abhängige Auslassungen.
- `python tools/release_audit.py --repo . --strict`: bestanden und frei von ROM-/Spieldaten-Fundstellen.
- `0083-local-coop.patch` gegen eine separate Baseline mit allen 83 vorhandenen Patches: erfolgreich per `git apply --check` geprüft.
- `git diff --check`: ohne Fehler.

### Akzeptanzkriterien

- [x] Zwei Instanzen verbinden sich ohne Link-Theke miteinander.
- [x] Die Bewegung des Hosts wird auf der Gastkonsole als zweite Figur sichtbar.
- [x] Ein Verbindungsabbruch hinterlässt keine blockierte Figur oder Sitzung.
- [x] Der Build und die automatisierten Tests bestehen.

## Sprint 2: Local-Wireless-Lobby und Passwort-Beitritt

**Status: Fertig**

### Aufgaben

- [x] Menüpunkt „Welt für Mehrspieler öffnen“ für den Host einbauen. Umgesetzt als LOKALER KOOP mit sichtbarem AUS-/AN-Zustand.
- [x] Passwortvergabe und passwortgeschützte Beitrittsprüfung umgesetzt. Ein falsches Passwort führt nicht in die Karte und kann danach erneut eingegeben werden; das richtige verbindet den Gast.
- [x] Menüpunkt „Spiel suchen“ im Hauptmenü des Gastes einbauen. Mit und ohne eigenen Spielstand auf zwei Instanzen geprüft.
- [x] Gefundene Welten mit ausreichenden Host- und Versionsinformationen anzeigen. Hostname und zentrale Mod-Version `V0.1.0` wurden in der Suche bestätigt.
- [x] Inkompatible Builds, Sprachen und Datenpakete durch den Build-ID-Vergleich ablehnen und die Sitzung auf Host plus einen Gast begrenzen. Die Laufzeitabnahme mit einer dritten Konsole ist ein späterer Zusatztest außerhalb dieses Sprints.
- [x] Spielstandsdaten erst nach erfolgreicher Authentifizierung freigeben. Der Kartenwechsel erfolgt erst nach der Host-Bestätigung, und der lokale Gast-Spielstand bleibt bytegenau unverändert.
- [x] Suchen und Beitreten ohne Neustart ermöglichen. Ein separates Abbrechen und erneutes Verbinden innerhalb derselben Sitzung ist für diesen Prototyp nicht erforderlich.

### Akzeptanzkriterien

- [x] Ein Gast findet eine geöffnete Welt über das Hauptmenü.
- [x] Ein falsches Passwort gewährt keinen Zugriff auf Welt oder Spielstand.
- [x] Das richtige Passwort verbindet genau einen zweiten Spieler.

### Spätere Zusatztests außerhalb dieses Sprints

- [ ] Beitritt einer dritten Konsole zu einer bereits belegten Welt verweigern.
- [ ] Die bisherigen Link-Theken weiterhin ohne Beeinträchtigung nutzen.

## Sprint 3: Gemeinsame Oberwelt und zweiten Spieler darstellen

**Status: In Arbeit**

Die grundlegende Bewegungs- und Disconnect-Übertragung wurde bereits in Sprint 1 bestätigt. Inzwischen werden beim Beitritt alle benötigten Speicherblöcke mit Prüfsumme vom Host übernommen. Gespeicherter Spawn, Rejoin nach Host-Speichern, Kartenwechsel, Bewegung und die vollständige Kartendarstellung wurden mit zwei Azahar-Instanzen bestätigt. Neue Namen starten fest in Wurzelheim, alte Namen an ihrem gespeicherten Spawn; eine zwischenzeitliche Spawn-Auswahl wurde verworfen. Die lokale Gast-Speicherdatei bleibt dabei bytegenau unverändert. Die Koop-Figur blockiert weder Bewegung noch NPC-Dialoge. Menüs und Kämpfe verwenden weiterhin getrennte Engine-Zustände pro Konsole. Persistente Story-Flags und Skriptvariablen werden nun laufend Host-autoritativ zwischen beiden Konsolen übernommen, auch bei direkten Schreibzugriffen. Team-, Item-, Objekt- und weitere Spielstandssynchronisation stehen noch aus; die neuen Regeln müssen noch im Laufzeittest bestätigt werden.

### Aufgaben

- [x] Zweite Spielfigur mit eigenem Charakter und Namen sowie Basis-Bewegungsdaten integrieren.
- [x] Bewegungsdaten mit Heartbeat, Validierung und Verlustbehandlung übertragen; dies wurde in Sprint 1 bestätigt.
- [x] Namensbasierte Spawnpositionen zuverlässig laden und nach dem Beitritt weiter synchronisieren; einschließlich Host-Snapshot, Rejoin und vollständigem Kartenaufbau bestätigt.
- [x] Kartenwechsel durch Türen und unterschiedliche Aufenthaltsorte beider Spieler synchronisieren; einschließlich Rejoin bestätigt.
- [x] Neue Gastnamen an einem freien, festen Punkt in Wurzelheim spawnen und für alte Namen immer deren gespeicherte Position ohne Spawn-Auswahl verwenden.
- [x] Kollisionen und Interaktionen mit der zweiten Figur festlegen und umsetzen: keine Blockade und kein NPC-Script; Laufzeittest steht noch aus.
- [x] Benötigte Host-Karten-, NPC- und Weltzustände für den Gast als vollständigen Initial-Snapshot bereitstellen.
- [x] Persistente Story-Flags und Skriptvariablen laufend Host-autoritativ übernehmen; direkte Schreibzugriffe werden mit einer Baseline und höchstens acht Änderungen pro Intervall erfasst.
- [x] Menüs und Kampf-/Oberweltzustände pro Spieler über getrennte Engine-Callbacks trennen; Laufzeittest steht noch aus.

### Akzeptanzkriterien

- [x] Beide Spieler können gleichzeitig und unabhängig auf derselben Karte laufen.
- [x] Beide Spieler können sich auf unterschiedlichen Karten aufhalten und später wieder zusammenfinden.
- [x] Spawnpunkte befinden sich nicht in Wänden, blockierten Objekten oder ungültigen Kartenbereichen.
- [x] Kartenwechsel und Trennungen erzeugen keine dauerhaften Geisterfiguren oder Desynchronisationen.

## Sprint 4: Spielerprofil, Mehrspieler-Box und PC-Sperre

**Status: In Arbeit**

### Aufgaben

- [x] Zweites Spielerprofil mit Charakter, Name und zugeordneter Boxenreihe im Host-Spielstand speichern; die vorhandene Spawn-Tabelle und die Gast-Reihe werden verwendet.
- [x] Mehrspieler-Box mit einer Reihe und bis zu sechs Pokémon pro Spieler einbauen: Box 14 wird als KOOP-Box reserviert, Reihe 1 gehört dem Host und Reihe 2 dem Gast; vorhandene Pokémon werden vorher in normale Boxen verschoben.
- [x] Startteam-Auswahl nach erfolgreichem Beitritt öffnen, sobald der PC frei ist: Der Gast muss mindestens ein Pokémon wählen und die Welt öffnet erst nach Host-Bestätigung; Laufzeittest steht noch aus.
- [x] Globale PC-Sperre für jeweils genau einen Spieler umsetzen: Host-Vergabe, verzögerte Gastöffnung und Freigabe bei Ausstieg, Stopp, F6 oder Disconnect; Laufzeittest steht noch aus.
- [x] PC-Verschiebungen als vollständige, unteilbare Host-Vorgänge ausführen: 34.344 Byte Speicher und Gruppe werden mit Prüfsumme übertragen und erst vollständig übernommen; Laufzeittest steht noch aus.
- [x] Boxenreihen eingeloggter Spieler gegen Entnahme, Verschieben, Tausch, Freilassen und Mehrfachverschieben sperren; Laufzeittest steht noch aus.
- [ ] Beim Ausloggen die gesamte Gruppe in die zugehörige Reihe zurücklegen und danach entsperren; die Reihe wird bei PC-Übernahmen aktualisiert, aber noch nicht bei jedem Verbindungsende mit aktuellen Kampfänderungen synchronisiert.
- [x] Abbrüche während der Startteam-Auswahl ohne teilweise übernommene Änderungen behandeln: Die Übernahme erfolgt erst nach vollständigem Empfang und korrekter Prüfsumme; der Abbruchtest steht noch aus.
- [x] Speicherformat beziehungsweise Erweiterung und Kompatibilität mit vorhandenen Spielständen dokumentieren: keine Größenänderung, KOOP-Initialisierung und Versionsmarker sind in `HANDOFF.md` beschrieben.

### Akzeptanzkriterien

- [ ] Zwei Spieler können nicht gleichzeitig denselben PC verwenden.
- [ ] Kein PC- oder Anmeldevorgang kann Pokémon duplizieren oder verlieren.
- [ ] Pokémon eines eingeloggten Spielers können nicht von einem anderen Spieler entnommen werden.
- [ ] Nach dem Ausloggen befindet sich die vollständige Gruppe in der richtigen Boxenreihe.
- [ ] Nach einem Neustart ist der gemeinsame Spielstand konsistent.

## Sprint 5: Unabhängige Einzelkämpfe und gemeinsame Trainerwelt

**Status: In Arbeit**

### Aufgaben

- [x] Wilde Begegnungen und Trainerkämpfe auch durch den Gast auslösbar machen; wilde Kämpfe starten lokal, Trainerkämpfe nach Host-Freigabe.
- [x] Einzelkämpfe mit der Gruppe des auslösenden Spielers durchführen; Host- und Gast-Gruppe bleiben getrennt.
- [x] Oberwelt auf einer Konsole und Kampf auf der anderen gleichzeitig betreiben; Kampf- und Welt-Callbacks bleiben getrennt.
- [x] Trainer und nicht wiederholbare Gegner während eines ausgelösten Kampfes reservieren; die Freigabe erfolgt erst nach Story-Synchronisation und gesetztem Trainer-Flag.
- [x] Kampfsiege, besiegte Trainer und andere Weltänderungen Host-autoritativ übernehmen; Flags und Variablen verwenden die bestehende Story-Synchronisation.
- [x] EP, Level, Fangversuche und Teamveränderungen der richtigen Spielergruppe zuordnen; Fang-, Item- und Geldänderungen werden als Deltas übertragen und Boxkollisionen auf freie Plätze umgeleitet.
- [x] Niederlagen, Flucht und Rückkehr zur Oberwelt für beide Spielerrollen behandeln; Niederlagen werden erst nach Whiteout einschließlich Heilung und Geldabzug übernommen.
- [x] Doppelte Belohnungen und mehrfaches Auslösen desselben Gegners durch Reservierung und verzögerte Freigabe verhindern.

Alle Sprint-5-Implementierungen sind kompiliert und auf Protokollebene getestet; die vollständige Abnahme mit zwei unabhängigen Kämpfen, Fang, Niederlage und Trainerreservierung in Azahar steht noch aus.

### Akzeptanzkriterien

- [ ] Beide Spieler können unabhängig voneinander Einzelkämpfe auslösen und abschließen.
- [ ] Ein Spieler kann weiterlaufen oder selbst kämpfen, während der andere im Kampf ist.
- [ ] Derselbe Trainer kann nicht gleichzeitig zweimal herausgefordert werden.
- [ ] Kampfergebnisse sind nach einem Speichern und Neuladen auf beiden Seiten konsistent.

## Sprint 6: Gemeinsame Doppelkämpfe

**Status: In Arbeit**

### Aufgaben

- [x] Vor Doppelkämpfen Karte, Nähe und Verfügbarkeit des Partners prüfen: dieselbe Karte, höchstens drei Felder Abstand und kein anderer Kampf, PC oder Handshake.
- [x] Bestehende Link-Multikampf- und Direktnachrichten für normale Doppelkämpfe erweitern: der Start nutzt einen geprüften 1.232-Byte-Setup-Handshake, danach laufen die originalen Controller-Nachrichten über den Koop-Direktkanal.
- [x] Beiden Spielern die Kontrolle über ihre eigenen Pokémon im gemeinsamen Kampf geben: Host steuert die linke und Gast die rechte Position, Gegner bleiben KI-gesteuert.
- [x] Doppelkämpfe durch Host oder Gast mit derselben Beitrittslogik unterstützen; der Auslöser überträgt Belohnungen nur einmal.
- [x] Ohne verfügbaren Partner auf den ursprünglichen Doppelkampf mit einem Spieler zurückfallen.
- [x] Züge, Wechsel, Ohnmacht, Flucht, Fangversuche und Kampfende über die vorhandene Link-Controller-Steuerung synchronisieren; Niederlagen werden nach Whiteout übernommen.
- [x] EP, Geld, Items und andere Ergebnisse eindeutig dem gemeinsamen Spielstand zuordnen; lokal identische Geld- und Item-Deltas werden beim Partner nicht doppelt angerechnet.

Alle Sprint-6-Implementierungen sind kompiliert und auf Protokollebene getestet. Die vollständige Abnahme mit gemeinsamen Zügen, Wechseln, Fängen, Whiteout, Host- und Gast-Auslösung sowie dem Fallback ohne Partner steht noch aus.

### Akzeptanzkriterien

- [ ] Ein benachbarter, verfügbarer Partner kann einem Doppelkampf von beiden Seiten beitreten.
- [ ] Jeder Spieler kontrolliert ausschließlich seine vorgesehenen Pokémon.
- [ ] Ohne Partner bleibt der originale Einzelspieler-Doppelkampf spielbar.
- [ ] Nach dem Kampf besitzen beide Konsolen denselben Team- und Weltzustand.

## Sprint 7: Abmeldung, Verbindungsabbrüche und sicheres Speichern

**Status: In Arbeit**

### Aufgaben

- [x] Ordentliche Abmeldung mit vollständiger Rückgabe der Gruppe durchführen: 600 Byte in geprüften Chunks, Trennung erst nach Host-Bestätigung, Abbruch der ersten Teamwahl ohne Überschreiben der alten Reihe.
- [x] Unerwartete Verbindungsabbrüche erkennen und nach einer definierten Zeitüberschreitung bereinigen; Abmeldungen werden nach 120 Frames erneut gesendet und nach 600 Frames mit dem letzten bestätigten Stand beendet.
- [x] Verhalten bei Abbrüchen während Einzel- und Doppelkämpfen festlegen und umsetzen: sicherer Abbruch als Flucht mit Gruppenrückgabe statt KI-Übernahme; ausstehende Handshakes kehren zur Oberwelt zurück.
- [x] Beim Schließen der Host-Welt alle Spieler und Gruppen sicher zurück in die Mehrspieler-Box überführen; die zuletzt bestätigte vollständige Partnergruppe wird in deren Reihe gespeichert.
- [x] PC- und Anmeldesperren bei Abbrüchen zuverlässig freigeben; PC-, Trainer-, Logout- und Doppelkampfzustände werden zurückgesetzt.
- [x] Nur konsistente Zustände speichern und unterbrochene Vorgänge verwerfen oder vollständig wiederherstellen; Empfänger übernehmen Daten erst nach vollständigem Empfang und korrekter Prüfsumme.
- [x] Wiederbeitritt nach Abbruch ohne doppelte Figuren, Gruppen oder Belohnungen ermöglichen; der vollständige Host-Snapshot ersetzt den RAM-Zustand des Gastes, nicht dessen lokale SAV.

Alle Sprint-7-Implementierungen sind kompiliert und auf Protokollebene getestet. Die Abnahme mit bestätigter Abmeldung, Abbrüchen während PC- und Battle-Transfers, Doppelkämpfen, F6 und Wiederbeitritt steht noch aus.

### Akzeptanzkriterien

- [ ] Kein getesteter Abbruch führt zu verlorenen oder duplizierten Pokémon.
- [ ] Kein getesteter Abbruch hinterlässt eine dauerhafte PC-, Trainer- oder Spielersperre.
- [ ] Der Host-Spielstand lässt sich nach jedem getesteten Abbruch laden und weiterspielen.
- [ ] Beim Beenden der Welt sind alle Gruppen wieder in ihren Boxenreihen.

## Sprint 8: Hardwaretests, Stabilisierung und spielbare Alpha

**Status: In Arbeit**

### Aufgaben

- [x] Automatisierte Tests für Speicherformat, Boxensperren und Koop-Protokoll ergänzen: Compile-Zeit-Prüfungen für Speicher- und Setupgrößen sowie Fehlerfälle für Pakettyp, Länge, Version und Offset. Die Prüfung korrigierte die Dokumentation des PC-Transfers auf 34.344 Byte.
- [ ] Zwei-Instanzen-Tests mit Azahar durchführen, soweit die benötigten UDS-Funktionen unterstützt werden. Identischer Build und PAK wurden auf beide SD-Umgebungen kopiert und Spielstände gesichert. Ein privater Raum auf Port 24873 wurde geöffnet. Die interaktive Abnahme erfolgt auf einem separaten Test-PC und wurde noch nicht als bestanden markiert.
- [ ] Tests mit zwei realen 3DS-/2DS-Konsolen durchführen; dafür werden die Geräte benötigt.
- [ ] Verhalten auf Old 3DS und New 3DS sowie mit aktivierter Voxel-Darstellung prüfen.
- [ ] Längere Spielsitzungen mit Kämpfen, Kartenwechseln, PC-Zugriffen und Wiederverbindungen testen.
- [ ] Bestehende Union-Room-, Tausch-, Kampf- und Minispielfunktionen auf Regressionen prüfen.
- [x] Build-, Installations- und Mehrspieleranleitung für den Fork aktualisieren.
- [x] Alle Sprintstände und verbleibenden Fehler in dieser Datei nachführen.

### Akzeptanzkriterien für die Alpha

- [ ] Alle vorherigen Sprints sind fertig.
- [ ] Keine bekannten Fehler führen zu Datenverlust, Pokémon-Duplikaten oder beschädigten Spielständen.
- [ ] Der vollständige Beitritts-, Spiel-, Abmelde- und Wiederbeitrittsablauf funktioniert auf zwei realen Konsolen.
- [ ] Build und automatisierte Tests bestehen aus einem sauberen Checkout.
- [ ] Veröffentlichte Archive enthalten keine ROMs, Spielstände oder anderen nicht freigegebenen Spieldaten.

## Noch zu entscheidende Details

- Dürfen Gäste story-kritische Ereignisse auslösen oder nur normale Trainer und wilde Begegnungen?
- Wie werden Flucht, Fangversuche und Kampfabbrüche in gemeinsamen Doppelkämpfen behandelt?

## Aktueller Gesamtstand

Sprint 0, Sprint 1 und Sprint 2 sind abgeschlossen und mit zwei Azahar-Instanzen geprüft. Sprint 3 ist auf `main` für den Nutzertest vorbereitet: Host-Spielstand, Spawn, Rejoin, Kartenwechsel und Bewegung sind bestätigt; Durchlässigkeit, Menütrennung und Story-Synchronisation sind implementiert, aber noch nicht abgenommen. Auf `sprint-4` sind zusätzlich die KOOP-Box in Box 14, Spieler-Reihen, Startteam-Auswahl, atomare PC-Übertragung, globale PC-Sperre und Reihensperren implementiert. Der Gast verwendet seine eigene gespeicherte Team-Reihe im gemeinsamen Host-Spielstand und verändert seine lokale SAV nicht. Auf `sprint-5` sind wilde Einzelkämpfe und Trainerkämpfe auf beiden Konsolen unabhängig möglich. Trainer-IDs werden reserviert und erst nach Flag-Übernahme freigegeben; Gruppen, EP, Evolutionen, Fänge, Items, Geld und Whiteout-Folgen werden über einen separaten Battle-Kanal übertragen. Fangkollisionen werden auf freie Boxplätze umgeleitet. Auf `sprint-6` starten Host und Gast gemeinsame Doppelkämpfe nach einer Karten-, Nähe- und Verfügbarkeitsprüfung. Ein bestätigter Setup-Handshake überträgt Gegner, Gruppen und RNG-Start; danach steuern Host und Gast ihre jeweilige Position über den originalen Direkt-Controller. Belohnungen werden nicht doppelt angerechnet und Whiteout wird übernommen. Auf `sprint-7` trennt eine ordentliche Abmeldung erst nach bestätigter Team-Rückgabe. Abbrüche beenden aktive Kämpfe sicher als Flucht, setzen alle Sperren zurück und verwerfen unvollständige Transfers; der Wiederbeitritt erfolgt über den vollständigen Host-Snapshot ohne Änderung der lokalen Gast-SAV. Auf `sprint-8` sind Speicherformat- und Protokolltests erweitert, Build und PAK auf beiden Azahar-Profilen identisch installiert und ein privater Raum geöffnet. Die interaktive Abnahme erfolgt auf einem separaten Test-PC; reale Hardware-, Voxel- und Regressionstests stehen ebenfalls noch aus. Alle neuen Abläufe sind gebaut und automatisiert auf Protokoll- und Speicherregeln getestet; die vollständigen Emulator- und Hardwareabnahmen stehen noch aus. Nicht kampfbedingte Teamänderungen außerhalb bestätigter PC-Vorgänge und das garantierte Zurücklegen bei einem Abbruch mitten in einer Übertragung sind noch offen. `HANDOFF.md` enthält die Branch-Trennung und Testschritte, `SETUP.md` die Ersteinrichtung. Es gibt noch keine vollständig spielbare Alpha.
