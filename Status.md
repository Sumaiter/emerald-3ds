# Status: Emerald 3DS Koop-Mod

Stand: 2026-11-08

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
| 2 | Local-Wireless-Lobby und Passwort-Beitritt | In Arbeit |
| 3 | Gemeinsame Oberwelt und zweiten Spieler darstellen | Offen |
| 4 | Spielerprofil, Mehrspieler-Box und PC-Sperre | Offen |
| 5 | Unabhängige Einzelkämpfe und gemeinsame Trainerwelt | Offen |
| 6 | Gemeinsame Doppelkämpfe | Offen |
| 7 | Abmeldung, Verbindungsabbrüche und sicheres Speichern | Offen |
| 8 | Hardwaretests, Stabilisierung und spielbare Alpha | Offen |

## Festgelegte Spielregeln

- Der Host öffnet seinen geladenen Spielstand mit einem Passwort für Local Wireless.
- Der Gast sucht im Hauptmenü nach Welten in Reichweite, wählt eine aus und gibt deren Passwort ein.
- Beim ersten Beitritt wartet der Gast gegebenenfalls auf den freien PC, wählt sein Startteam und danach Charakter und Namen.
- Als Spawnort stehen ein freies Feld beim Host oder eine bereits im Spielstand freigeschaltete Stadt zur Auswahl.
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

**Status: In Arbeit**

### Aufgaben

- [x] Menüpunkt „Welt für Mehrspieler öffnen“ für den Host einbauen. Umgesetzt als LOKALER KOOP mit sichtbarem AUS-/AN-Zustand.
- [x] Passwortvergabe und passwortgeschützte Beitrittsprüfung umgesetzt. Ein falsches Passwort führt nicht in die Karte und kann danach erneut eingegeben werden; das richtige verbindet den Gast.
- [x] Menüpunkt „Spiel suchen“ im Hauptmenü des Gastes einbauen. Mit und ohne eigenen Spielstand auf zwei Instanzen geprüft.
- [x] Gefundene Welten mit ausreichenden Host- und Versionsinformationen anzeigen. Hostname und zentrale Mod-Version `V0.1.0` wurden in der Suche bestätigt.
- [ ] Inkompatible Builds, Sprachen, Datenpakete und bereits belegte Welten ablehnen. Build-ID-Vergleich und Begrenzung auf Host plus einen Gast sind implementiert; die vollständige Laufzeitabnahme steht noch aus.
- [x] Spielstandsdaten erst nach erfolgreicher Authentifizierung freigeben. Der Kartenwechsel erfolgt erst nach der Host-Bestätigung, und der lokale Gast-Spielstand bleibt bytegenau unverändert.
- [ ] Suchen, Beitreten, Abbrechen und erneutes Verbinden ohne Neustart ermöglichen.

### Akzeptanzkriterien

- [x] Ein Gast findet eine geöffnete Welt über das Hauptmenü.
- [x] Ein falsches Passwort gewährt keinen Zugriff auf Welt oder Spielstand.
- [x] Das richtige Passwort verbindet genau einen zweiten Spieler.
- [ ] Die bisherigen Link-Theken funktionieren weiterhin.

## Sprint 3: Gemeinsame Oberwelt und zweiten Spieler darstellen

**Status: Offen**

### Aufgaben

- [ ] Zweite Spielfigur mit eigenem Charakter, Namen und Spielerzustand integrieren.
- [ ] Bewegungsdaten so übertragen und zwischenspeichern, dass veraltete Positionen keine Verzögerung aufbauen.
- [ ] Kartenwechsel, Warps, Türen und unterschiedliche Aufenthaltsorte beider Spieler synchronisieren.
- [ ] Kollisionen und Interaktionen mit der zweiten Figur festlegen und umsetzen.
- [ ] Spawn beim Host auf einem freien, begehbaren Nachbarfeld ermöglichen.
- [ ] Spawn in freigeschalteten Städten an sicheren, festgelegten Positionen ermöglichen.
- [ ] Benötigte Host-Karten-, NPC- und Weltzustände für den Gast bereitstellen.
- [ ] Menüs und Kampf-/Oberweltzustände pro Spieler getrennt halten.

### Akzeptanzkriterien

- [ ] Beide Spieler können gleichzeitig und unabhängig auf derselben Karte laufen.
- [ ] Beide Spieler können sich auf unterschiedlichen Karten aufhalten und später wieder zusammenfinden.
- [ ] Spawnpunkte befinden sich nicht in Wänden, blockierten Objekten oder ungültigen Kartenbereichen.
- [ ] Kartenwechsel und Trennungen erzeugen keine dauerhaften Geisterfiguren oder Desynchronisationen.

## Sprint 4: Spielerprofil, Mehrspieler-Box und PC-Sperre

**Status: Offen**

### Aufgaben

- [ ] Zweites Spielerprofil mit Charakter, Name und zugeordneter Boxenreihe im Host-Spielstand speichern.
- [ ] Mehrspieler-Box mit einer Reihe und bis zu sechs Pokémon pro Spieler einbauen.
- [ ] Startteam-Auswahl nach erfolgreichem Beitritt öffnen, sobald der PC frei ist.
- [ ] Globale PC-Sperre für jeweils genau einen Spieler umsetzen.
- [ ] PC-Verschiebungen als vollständige, unteilbare Host-Vorgänge ausführen.
- [ ] Boxenreihen eingeloggter Spieler gegen Entnahme und Verschieben sperren.
- [ ] Beim Ausloggen die gesamte Gruppe in die zugehörige Reihe zurücklegen und danach entsperren.
- [ ] Abbrüche während der Startteam-Auswahl ohne teilweise übernommene Änderungen behandeln.
- [ ] Speicherformat beziehungsweise Erweiterung und Kompatibilität mit vorhandenen Spielständen dokumentieren.

### Akzeptanzkriterien

- [ ] Zwei Spieler können nicht gleichzeitig denselben PC verwenden.
- [ ] Kein PC- oder Anmeldevorgang kann Pokémon duplizieren oder verlieren.
- [ ] Pokémon eines eingeloggten Spielers können nicht von einem anderen Spieler entnommen werden.
- [ ] Nach dem Ausloggen befindet sich die vollständige Gruppe in der richtigen Boxenreihe.
- [ ] Nach einem Neustart ist der gemeinsame Spielstand konsistent.

## Sprint 5: Unabhängige Einzelkämpfe und gemeinsame Trainerwelt

**Status: Offen**

### Aufgaben

- [ ] Wilde Begegnungen und Trainerkämpfe auch durch den Gast auslösbar machen.
- [ ] Einzelkämpfe mit der Gruppe des auslösenden Spielers durchführen.
- [ ] Oberwelt auf einer Konsole und Kampf auf der anderen gleichzeitig betreiben.
- [ ] Trainer und nicht wiederholbare Gegner während eines ausgelösten Kampfes reservieren.
- [ ] Kampfsiege, besiegte Trainer und andere Weltänderungen Host-autoritativ übernehmen.
- [ ] EP, Level, Fangversuche und Teamveränderungen der richtigen Spielergruppe zuordnen.
- [ ] Niederlagen, Flucht und Rückkehr zur Oberwelt für beide Spielerrollen behandeln.
- [ ] Doppelte Belohnungen und mehrfaches Auslösen desselben Gegners verhindern.

### Akzeptanzkriterien

- [ ] Beide Spieler können unabhängig voneinander Einzelkämpfe auslösen und abschließen.
- [ ] Ein Spieler kann weiterlaufen oder selbst kämpfen, während der andere im Kampf ist.
- [ ] Derselbe Trainer kann nicht gleichzeitig zweimal herausgefordert werden.
- [ ] Kampfergebnisse sind nach einem Speichern und Neuladen auf beiden Seiten konsistent.

## Sprint 6: Gemeinsame Doppelkämpfe

**Status: Offen**

### Aufgaben

- [ ] Vor Doppelkämpfen Karte, Nähe und Verfügbarkeit des Partners prüfen.
- [ ] Bestehende Link-Multikampf- und Direktnachrichten für normale Doppelkämpfe erweitern.
- [ ] Beiden Spielern die Kontrolle über ihre eigenen Pokémon im gemeinsamen Kampf geben.
- [ ] Doppelkämpfe durch Host oder Gast mit derselben Beitrittslogik unterstützen.
- [ ] Ohne verfügbaren Partner auf den ursprünglichen Doppelkampf mit einem Spieler zurückfallen.
- [ ] Züge, Wechsel, Ohnmacht, Flucht, Fangversuche und Kampfende korrekt synchronisieren.
- [ ] EP, Geld, Items und andere Ergebnisse eindeutig dem gemeinsamen Spielstand zuordnen.

### Akzeptanzkriterien

- [ ] Ein benachbarter, verfügbarer Partner kann einem Doppelkampf von beiden Seiten beitreten.
- [ ] Jeder Spieler kontrolliert ausschließlich seine vorgesehenen Pokémon.
- [ ] Ohne Partner bleibt der originale Einzelspieler-Doppelkampf spielbar.
- [ ] Nach dem Kampf besitzen beide Konsolen denselben Team- und Weltzustand.

## Sprint 7: Abmeldung, Verbindungsabbrüche und sicheres Speichern

**Status: Offen**

### Aufgaben

- [ ] Ordentliche Abmeldung mit vollständiger Rückgabe der Gruppe durchführen.
- [ ] Unerwartete Verbindungsabbrüche erkennen und nach einer definierten Zeitüberschreitung bereinigen.
- [ ] Verhalten bei Abbrüchen während Einzel- und Doppelkämpfen festlegen und umsetzen.
- [ ] Beim Schließen der Host-Welt alle Spieler und Gruppen sicher zurück in die Mehrspieler-Box überführen.
- [ ] PC- und Anmeldesperren bei Abbrüchen zuverlässig freigeben.
- [ ] Nur konsistente Zustände speichern und unterbrochene Vorgänge verwerfen oder vollständig wiederherstellen.
- [ ] Wiederbeitritt nach Abbruch ohne doppelte Figuren, Gruppen oder Belohnungen ermöglichen.

### Akzeptanzkriterien

- [ ] Kein getesteter Abbruch führt zu verlorenen oder duplizierten Pokémon.
- [ ] Kein getesteter Abbruch hinterlässt eine dauerhafte PC-, Trainer- oder Spielersperre.
- [ ] Der Host-Spielstand lässt sich nach jedem getesteten Abbruch laden und weiterspielen.
- [ ] Beim Beenden der Welt sind alle Gruppen wieder in ihren Boxenreihen.

## Sprint 8: Hardwaretests, Stabilisierung und spielbare Alpha

**Status: Offen**

### Aufgaben

- [ ] Automatisierte Tests für Speicherformat, Boxensperren und Koop-Protokoll ergänzen.
- [ ] Zwei-Instanzen-Tests mit Azahar durchführen, soweit die benötigten UDS-Funktionen unterstützt werden.
- [ ] Tests mit zwei realen 3DS-/2DS-Konsolen durchführen.
- [ ] Verhalten auf Old 3DS und New 3DS sowie mit aktivierter Voxel-Darstellung prüfen.
- [ ] Längere Spielsitzungen mit Kämpfen, Kartenwechseln, PC-Zugriffen und Wiederverbindungen testen.
- [ ] Bestehende Union-Room-, Tausch-, Kampf- und Minispielfunktionen auf Regressionen prüfen.
- [ ] Build-, Installations- und Mehrspieleranleitung für den Fork aktualisieren.
- [ ] Alle Sprintstände und verbleibenden Fehler in dieser Datei nachführen.

### Akzeptanzkriterien für die Alpha

- [ ] Alle vorherigen Sprints sind fertig.
- [ ] Keine bekannten Fehler führen zu Datenverlust, Pokémon-Duplikaten oder beschädigten Spielständen.
- [ ] Der vollständige Beitritts-, Spiel-, Abmelde- und Wiederbeitrittsablauf funktioniert auf zwei realen Konsolen.
- [ ] Build und automatisierte Tests bestehen aus einem sauberen Checkout.
- [ ] Veröffentlichte Archive enthalten keine ROMs, Spielstände oder anderen nicht freigegebenen Spieldaten.

## Noch zu entscheidende Details

- Dürfen Gäste story-kritische Ereignisse auslösen oder nur normale Trainer und wilde Begegnungen?
- Wie werden Flucht, Fangversuche und Kampfabbrüche in gemeinsamen Doppelkämpfen behandelt?
- Soll bei einem Verbindungsabbruch im Doppelkampf eine KI übernehmen oder der Kampf sicher abgebrochen werden?
- Soll ein wiederkehrender Gast jedes Mal ein Startteam wählen oder automatisch seine letzte Boxenreihe übernehmen?
- Welche Interaktionen zwischen den beiden Spielfiguren sind möglich, beispielsweise Ansprechen, Blockieren oder direkter Tausch?

## Aktueller Gesamtstand

Die Projekteinrichtung und die reproduzierbare deutsche Baseline sind fertig. In Sprint 1 sind Protokoll, Koop-Sitzung, Optionsaktion, RFU-Demultiplexing, Overworld-Avatar und der reproduzierbare Engine-Patch implementiert. Der deutsche ARM-Build, das installierbare Releasepaket, die vollständige englische Baseline-Suite, die Koop- und Builder-Tests sowie der Release-Audit bestehen. Die Zwei-Instanzen- und Hardwareprüfung stehen noch aus; Sprint 1 ist daher nicht abgeschlossen.
