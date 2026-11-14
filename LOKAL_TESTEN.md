# Host und Gast auf diesem PC starten

**Neuester Entwicklungsstand: Sprint6 integriert, Protokoll19**, Build
**225ca2caa5a7881b**, Boot **E3ID-5790a58262fe8f67**, ABI **73bbc4ee**.
Hintergrundtests und ARM-Release/PAK bestehen; noch kein Emulatornachtest.
Gras und Zuschauer-Ruckeln bleiben die getrennten offenen Issues #1/#2.

Sprint6 ist noch in Arbeit: EP-/Levelup-/EV-/Lern- und Trainerpreisgeldpfade
werden vom nativen Linkmodus noch uebersprungen und brauchen weitere
Implementierung. Diese Luecke ist kein bestandener Spieltest.

Fuer den gezielten Doppelkampf-Test zuerst beide Fenster schliessen und das
Paseo-Script neu starten (F6 installiert keinen Build). Jeweils Host und Gast
ausloesen lassen: benachbarter freier Partner, entfernter/abwesender Partner
und Partner in einem Menue/PC. Eigene Pokemon steuern, Wechsel/KO und
Kampfende pruefen; danach erstes Dreierteam plus unveraenderte Reserveplaetze,
HP/Status/AP und Weltfortschritt vergleichen. Die uebrigen Standardtests aus
Sprint5 bleiben anerkannt. Hardware und Doppelkampf-Disconnects sind weiterhin
gesonderte Abnahmepunkte.

Die folgenden Rueckmeldungen beschreiben den zuvor getesteten Protokoll18-Build.

Die beiden Restfehler sind zurueckgestellt und getrennt auf GitHub erfasst:
[Grasanimation #1](https://github.com/Sumaiter/emerald-3ds/issues/1) und
[Zuschauer-Ruckeln #2](https://github.com/Sumaiter/emerald-3ds/issues/2).
Zuschauen gilt fuer den Nutzer vorerst als nutzbar. Als naechster Ausbau
folgt auf ausdruecklichen Wunsch Sprint6 nach Pruefung des Upstream-Main.

**Neuester Nachtest 2026-11-14: nicht bestanden.** Beim Beobachter fehlt die
Graswackelanimation weiterhin. Zuschauen ist besser, aber weiterhin ruckelig;
am kaempfenden Gast ist Ruckeln allenfalls wenig erkennbar. Der laufende Build
ist per Log als E3ID-6a834575fbbbc71c / ABI bd6f7986 bestaetigt. Beide Befunde
werden erneut untersucht; die bisherigen Hintergrundtests sind kein Ersatz
fuer diese Laufzeitabnahme. Andere bestaetigte Tests bleiben bestanden.

**Fix-Welle vom 2026-11-14 gebaut:** Grasbewegung beim entfernten Spieler
korrigiert und Zuschauen auf lokale Darstellung uebertragener Displayzustaende
umgestellt. Gesamt-Hintergrundtests und ARM-Release/PAK bestehen. Der neue
Emulatornachtest dieser beiden Aenderungen steht noch aus.

Deine bisherigen Standardtests einschliesslich Host-F6/Gast-Hauptmenue,
NPC-Marktfuehrung und **B: Zuschauen verlassen** bleiben als bestanden
dokumentiert. Sie muessen nicht pauschal wiederholt werden.

Fuer den neuen Build beide Fenster schliessen und das Paseo-Script
**Host und Gast starten** manuell starten. F6 installiert keine neue Version.

Gezielter Nachtest:

- **Gras:** Den anderen Spieler beim Gehen und Laufen durch Gras beobachten,
  danach Rollen tauschen. Die Grasbewegung soll am passenden Feld erscheinen;
  Stillstehen und Drehen duerfen sie nicht staendig erneut starten.
- **Zuschauen:** 2D und Voxel, spaeter Einstieg sowie echte Attacken, Text und
  HP-Balken pruefen; moeglichst auch Zuschauer und Kaempfer auf verschiedenen
  Karten. Mit B und Touch verlassen und direkt weiterlaufen. Kampfende und
  Host-F6 waehrend des Zuschauens pruefen, damit die neue Renderansicht sauber
  aufgeraeumt wird. Unter starkem Rueckstau koennen kurze Zwischenzustaende
  weiterhin fehlen; die Hostmessungen garantieren keine ruckelfreie Wiedergabe
  aller Attacken auf Emulator oder Hardware.

## Vom Nutzer getesteter Umfang am 2026-11-14

**Nachtrag:** Der Nutzer meldet die uebrigen Tests dieser Liste als erfolgreich
("der rest hat alles funktioniert"). Die folgende Liste dokumentiert den
geprueften Umfang; sie ist kein Auftrag, alles erneut zu testen. Ausgenommen
bleiben die zwei oben erfassten Fehler sowie die separat genannten Grenzfaelle.

Die Liste beschreibt die gemeldeten Standardtests einschliesslich getauschter
Rollen. PC-Sperre und Figurengalerie waren bereits zuvor akzeptiert.
Einzelne Zahlen wurden nicht unabhaengig gemessen; die Markierungen beruhen
auf der Rueckmeldung des Nutzers.

1. [x] **Unabhaengige Kaempfe:** Einer kaempft, der andere laeuft weiter und kann
   einen anderen Kampf starten. Beide gewinnen und kehren bedienbar zurueck.
   XP, AP, HP, Status und getragene Items bleiben beim richtigen Team.
2. [x] **Derselbe Trainer / Szenario:** Bereits waehrend des Dialogs darf der
   andere Spieler denselben Gegner nicht ebenfalls starten. Nach einem Sieg
   muss die Szene fuer beide abgeschlossen sein, einschliesslich Maike und
   eines normalen Trainers. Kein zweiter Kampf und keine zweite Belohnung.
3. [x] **Gemeinsame Gegenstaende:** Finden, kaufen, benutzen, halten/tauschen und
   Beutel-PC-Transfer kontrollieren; Mengen muessen beim anderen sofort
   folgen, auch wenn er kaempft. Bei nur einem Trank und gleichzeitigem
   Benutzen darf nur eine Wirkung entstehen; kein doppelter Verbrauch.
4. [x] **Gemeinsames Geld:** Kaufen, verkaufen und Trainer-Preisgeld kontrollieren.
   Beide zeigen denselben Betrag; kein nachtraegliches Verdoppeln am Kampfende.
   Zwei gleichzeitige Kaeufe bei zu wenig Gesamtgeld duerfen das Guthaben nicht
   ueberziehen oder kostenlose Ware geben.
5. [x] **Fang und Kampfende:** Fang mit freiem Teamplatz und mit vollem Team
   pruefen: eigenes Team bzw. genau ein Box-Pokemon. Flucht und Niederlage
   sollen zur bedienbaren Oberwelt fuehren und Sperren freigeben. Native
   Heilung/Geldverlust bei Niederlage von ungewolltem Reset unterscheiden.
6. [x] **Gast verlassen / Speichern / Wiederbeitritt:** Vorher HP, Status und AP
   eines Gast-Pokemon notieren, dann den Gast trennen. Host muss weiterspielen
   koennen und im sicheren Feldzustand automatisch speichern; bei laufendem
   Kampf/PC erst nach dessen Abschluss. Gastreihe darf ohne Heilereignis nicht
   geheilt werden. Danach beide neu starten und als derselbe Gast beitreten:
   Team, HP, Status, AP, Items, Geld und Storyfortschritt vergleichen.
7. [x] **Zuschauen funktional:** A am Spieler und am zugehoerigen NPC; B und
   Touch-Verlassen; danach direkt weiterlaufen. Kampfende soll ebenfalls
   sauber zur eigenen Ansicht fuehren. B-Beschriftung ist inzwischen bestanden;
   die neue lokale Darstellung wartet gesondert auf ihren Nachtest.
8. [x] **Darstellung im Spiel:** Ausgewaehlte eigene Figur beim Kampfeinstieg/
   Pokeballwurf und beim Laufen/Radfahren; Gastname in dessen Szenariotexten.
   Die freigegebene HTML-Galerie ersetzt diese Laufzeitprobe nicht.

Volle Boxen und Sonderkampf-Rueckkehr bleiben gezielte Grenztests mit
vorbereitetem Teststand. Die bisher dokumentierte fehlende Reservierung
einmaliger fester Wildgegner bleibt ein eigener offener Implementierungspunkt;
diese Faelle nicht aus den normalen Trainer-/Wildkampftests als bestanden
ableiten. Keine dieser Luecken wird durch die gruenen Hintergrundtests abgenommen.

Aktueller Koop-Stand: **Protokoll 19**, Build **225ca2caa5a7881b**,
Bootkennung **E3ID-5790a58262fe8f67**, ABI **73bbc4ee**. Nach einem neuen Build beide Fenster
schliessen und das Paseo-Script erneut starten; dabei werden die drei passenden
Builddateien in die Profile kopiert. F6 setzt nur das bereits geladene Spiel
zurueck und installiert keinen neuen Build.

Beutel, PC-Items, Geld und Muenzen sind gemeinsam und werden waehrend des
Spielens synchronisiert. A auf den kaempfenden Spieler oder neben dessen
NPC-Gegner startet Zuschauen; B oder **B: Zuschauen verlassen** auf dem Touchscreen
beendet es. Nach einem Gast-Disconnect speichert der Host automatisch, sobald
die Oberwelt sicher ist. Die Gast-Teamreihe behaelt HP/Status/AP.

Trennt der Host die Verbindung, kehrt der Gast ins Hauptmenue zurueck, auch
aus Kampf, PC oder Beutel. Ein stiller Verbindungsabbruch wird nach ungefaehr
sechs Sekunden erkannt. Zum erneuten Mitspielen vom Hauptmenue frisch beitreten.

Die beiden Azahar-Profile liegen zentral ausserhalb der Branch-Arbeitsbaeume:

| Einstellung | Host | Gast |
| --- | --- | --- |
| Emulatorname / Vorgabe beim Direktverbinden | `Host` | `Gast` |
| Profilordner | `C:\paseoprojects\smaragd3ds\.tools\azahar\host\user` | `C:\paseoprojects\smaragd3ds\.tools\azahar\guest\user` |
| Spielordner im Profil | `sdmc\3ds\emerald3ds` | `sdmc\3ds\emerald3ds` |
| Emulator | Azahar 2126.2 | Azahar 2126.2 |

Einstellungen, Tastenbelegung, NAND und Spielstaende bleiben in diesen beiden
portablen Profilen erhalten. Beim Branchwechsel dieselben Profile verwenden.
Beide Fenster vor dem Aktualisieren schliessen.

## Start

In Paseo im Workspace unter **Scripts** den Eintrag **Host und Gast starten**
manuell starten. Die Einbindung steht in `paseo.json`. Es gibt keinen Autostart.

Im Verzeichnis des gewuenschten Branches / Worktrees:

```powershell
powershell -File .\tools\start-coop-test.ps1
```

Das Skript kopiert den Build dieses Arbeitsbaums in beide Spielordner, setzt
die Emulatornamen `Host` und `Gast` und startet den lokalen Raumserver sowie
zwei Spielfenster. Ein bereits laufender eigener Raumserver auf UDP-Port `24872`
wird wiederverwendet. Die Verbindung stellst du in beiden Spielfenstern selbst
her. Die SAV-Dateien werden beim Startskript nicht ersetzt.

Erwartete Build-Dateien:

```text
build\upstream\3ds_port\dist\Emerald3DS.3dsx
build\upstream\3ds_port\dist\Emerald3DS.smdh
build\upstream\3ds_port\build\emerald3ds.pak
```

Bei einem anderen Build-Verzeichnis:

```powershell
powershell -File .\tools\start-coop-test.ps1 -BuildPort C:\Pfad\zum\3ds_port
```

Die zentrale Kopie des Skripts kann auch von Branches ohne diese Datei aufgerufen
werden. Dabei im gewuenschten Branch-Verzeichnis bleiben oder `-RepoRoot` angeben:

```powershell
powershell -File C:\paseoprojects\smaragd3ds\.tools\azahar\start-coop-test.ps1 -RepoRoot C:\Pfad\zum\Arbeitsbaum
```

## Neue Reparaturen gezielt pruefen (Protokoll 15)

Beide alten Spielfenster schliessen und danach das Paseo-Skript **Host und Gast
starten** verwenden. F6 allein laedt keine neu gebaute Programmdatei.

- Rivalen und normale Trainer: schon waehrend des Gespraechs gegeneinander
  sperren; nach Sieg, Niederlage und Abbruch muss die Freigabe stimmen.
- Trainer mit wechselnder Blickrichtung: nach Wartezeit korrekt heranlaufen.
- RusturfTunnel-Szene: Peeko und Mr. Briney muessen ihre Bewegungen beenden.
- Geheimbasis/Frontier und Steven: nach Rueckkehr beweglich; urspruengliche
  Pokemon und gehaltene Items erhalten. Normaler WhiteOut muss weiterhin
  heilen und den vorgesehenen Geldverlust ausfuehren.
- Bei gleichzeitigem Fang in volle Boxen: PC zum Platzschaffen nutzbar,
  wartender Fang nach Schliessen vorhanden und Boxinhalt bei beiden gleich.
  Den eigenen neuen Fang auch verschieben, bearbeiten, ins Team entnehmen
  oder absichtlich freilassen: kein Duplikat, kein Wiederauftauchen und keine
  ueberschriebene Aenderung nach der Synchronisierung.

Automatische Tests fuer diese Faelle laufen im Hintergrund unter verify-coop.
Der manuelle Test der sichtbaren Ablaufe ist damit nicht ersetzt.

## Ereignissperre pruefen

Nach dem Neustart beider Instanzen mit derselben neuen Testversion:

1. Gast spricht zuerst einen Trainer an; Host versucht denselben NPC schon
   waehrend des Gespraechs und spaeter waehrend des Kampfes anzusprechen.
   Host soll im Feld bleiben und unten den Namen des Gasts sehen.
2. Dasselbe mit vertauschten Rollen pruefen, einschliesslich Rivalen und
   normalem Trainer, danach automatisch ausgeloestem Kampfereignis.
3. Nach Kampf und Abschlussdialog erneut pruefen: Story/Trainerstatus soll
   uebernommen sein. Nach Niederlage oder Abbruch muss der Ablauf wieder frei sein.
4. Verschiedene Trainer sowie unabhaengige Zufallskaempfe gleichzeitig pruefen.

Die Sperre beginnt vor dem ersten Skriptbefehl. A/B oder Tippen bestaetigt nur
den Hinweis und fuehrt das abgewiesene Gespraech nicht spaeter automatisch aus.
Diese Hintergrundpruefungen ersetzen nicht die manuelle Emulatorabnahme.
Ein bereits haengender alter Build wird durch F6 nicht aktualisiert; beide
Fenster schliessen und das Paseo-Startskript fuer den neuen Build ausfuehren.

## Verbindung bei Bedarf selbst herstellen

Die gespeicherte Direktverbindungsadresse lautet `127.0.0.1`, Port `24872`.
Der lokale Azahar-Raum **Emerald Koop** wird vom Startskript mitgestartet und
erlaubt zwei Teilnehmer. Das Raumpasswort bleibt leer. Beide Emulatoren brauchen
denselben zusammengehoerigen 3DSX-/SMDH-/PAK-Build.

Wenn die Spielfenster bereits offen sind und nur der Server fehlt:

```powershell
powershell -File .\tools\start-coop-test.ps1 -ServerOnly
```

Der Raumserver laeuft im Hintergrund. Seine Protokolle liegen unter
`C:\paseoprojects\smaragd3ds\.tools\azahar\room`. Zum Start gehoert
`--preferred-app "Emerald 3DS"`; ohne diese Angabe beendet sich Azahar 2126.2
mit `preferred application is empty!`.

Das Koop-Passwort im Spiel gehoert zum jeweiligen Spielprofil und ist vom
Raumpasswort getrennt. Die weitere Einrichtung und die Spielbedienung macht
der Nutzer selbst.

## Gesicherte Einstellungen und aktueller Stand

- Quellstand beim Einrichten: `main`, Commit `fdc4b48` (12. November 2026).
- Die Emulatornamen stehen in den portablen 3DS-Konfigurationen. Das Skript
  sichert die urspruengliche Datei einmalig als `config.before-host-gast`.
- Die beiden Programmdateien heissen `Host.exe` und `Gast.exe`; DLLs und
  portable Profile werden aus dem jeweiligen vorhandenen Azahar-Ordner verwendet.
- Die zentrale Kopie dieser Anleitung liegt unter
  `C:\paseoprojects\smaragd3ds\.tools\azahar\LOKAL_TESTEN.md`.
- Aktueller gemeinsamer Release-Build: `cf712051f18d29ac`, Datenpaket-ABI
  `ebf0f1f4` (13. November 2026), Protokollversion 11. Er enthaelt die
  Gastnamen-, Starter-, Wiederbeitritts- und PC-Besitzermeldungs-Korrekturen
  sowie alle neuen Kampf- und Fahrradgrafiken und die Sprint-5-Integration
  aus origin/sprint-5 (1be2515). Beide Spielfenster vor dem
  Aktualisieren schliessen; F6 allein kopiert keine neuen Build-Dateien.
- Wenn der andere Spieler den PC belegt, zeigt der untere Bildschirm seinen
  Spielnamen. Erst A, B oder ein Tipp schliesst die Meldung und fuehrt ins
  PC-Menue zurueck. Die gegenseitige Sperre bleibt dabei aktiv.
- Alle 99 Figuren sind fuer den Kampf abgedeckt: acht native animierte
  Rueckenansichten und 91 neue statische Bilder. Beim Fahrrad bleiben zwei
  native Animationen erhalten; 97 neue Boegen liefern jeweils 27 Frames.
  Die 15 Figuren aus ZEROWALK.md haben zusaetzlich neun echte Laufbilder und
  sind wieder in der Auswahl. Beide Spieler verwenden ihre eigene Auswahl.
- Release-/PAK-Build, acht Koop-Pruefgruppen, Kampf- und Fahrradtests sowie
  Release-Audit bestehen. Der separate Starter-Menue-Test kann mit dem
  vorhandenen Windows-Testlinker nicht linken; Details stehen in HANDOFF.md.
  Die interaktive Abnahme der neuen Korrekturen/Grafiken steht noch aus.
- ROMs, Spielstaende, PAKs, Emulatorprofile und Build-Dateien bleiben lokal.

Die aktuellen Spieloptionen bleiben in `settings.txt` im jeweiligen
Spielordner gespeichert. Beim Einrichten verwendete Host `voxel=0`,
`voxel_battle=0`, `classic_menus=1`, `local_coop=1`; Gast verwendete `voxel=1`
und `voxel_battle=1`. Beide verwendeten `voxel_pitch=40`, `voxel_zoom=100`,
`voxel_blur=100`, `fps=0`, `run=0` und `speed=1`. Die komplette
Emulatorkonfiguration samt Tastenbelegung liegt in
`user\config\qt-config.ini` des jeweiligen Profils.

## Figuren und Animationen in HTML pruefen

Die Grafiken aller 99 Figuren wurden vom Nutzer nach Sichtung der Galerie
freigegeben ("ok die passen alle nice"). Die folgenden Tests im laufenden
Spiel bleiben davon unabhaengig.

Die vollstaendige lokale Galerie liegt unter
[build/character-gallery/index.html](build/character-gallery/index.html).
Die Datei direkt im bevorzugten Browser oeffnen; kein Server erforderlich.
Sie enthaelt alle 99 Figuren einschliesslich der bisherigen 84. Suche, Filter
fuer neue Figuren, vier Richtungen, Lauf-/Renn-/Mach-/Acro-Animationen,
Kampfbilder, Pause, Tempo und Einzelbilder erlauben den direkten Vergleich.
Notizen und Freigaben koennen als JSON-Pruefbericht gespeichert werden.
Die Datei enthaelt Originalreferenzen und bleibt deshalb nur lokal im Buildordner.
Automatische Daten-/Animations-/Syntaxpruefung bestanden; die sichtbare
Browserpruefung war mangels Computer-Use-Freigabe noch nicht moeglich.

## Sprint 5 testen

Die bereits offenen Spielfenster verwenden noch den vorherigen Build.
Beide Fenster schliessen und in Paseo **Host und Gast starten** ausfuehren.
Damit werden 3DSX/SMDH/PAK gemeinsam aktualisiert; F6 allein reicht nicht.
Verbindung weiterhin 127.0.0.1:24872.

1. Gast beginnt einen wilden Kampf mit seiner eigenen Gruppe; Host laeuft
   dabei weiter. Danach beide gleichzeitig gegen verschiedene Gegner kaempfen.
2. Jeweils EP, Level, AP, Status, getragenes Item und Team vor/nach dem Kampf
   vergleichen. Fangen sowohl mit freiem Teamplatz als auch mit voller Gruppe
   und freiem Boxplatz pruefen; alle Faenge muessen erhalten bleiben.
3. Denselben normalen Trainer gleichzeitig ausloesen. Es darf nur ein Kampf
   gegen diesen Trainer laufen; nach seinem Sieg darf der wartende Spieler
   keinen zweiten Kampf mit erneuter Belohnung starten.
4. Flucht und Niederlage fuer Host und Gast pruefen, danach Rueckkehr in die
   Oberwelt und erneuten Kampf. Der andere Spieler muss bedienbar bleiben.
5. Der PC bleibt waehrend aktiver Kaempfe und unbestaetigter Ergebnisse
   gesperrt. Danach soll er wieder freigegeben sein. Ohne aktiven Kampf die
   gegenseitige PC-Sperre mit Besitzernamen weiter pruefen.
6. Host normal speichern, beide neu starten und Gast erneut beitreten:
   Teams, Faenge, Inventar, Geld und besiegte Trainer muessen erhalten bleiben.
7. Eigene Figur im Kampf/Pokeballwurf, Mach-/Acro-Fahrrad, Gastname in
   Szenariotexten und direkten Wiederbeitritt weiterhin gegenpruefen.

Die Reservierung statischer einmaliger Wildgegner ist im uebernommenen
Sprint-5-Branch noch nicht umgesetzt. Volle Boxen oder nicht anwendbare
Inventaraenderungen werden ohne Teiluebernahme zurueckgehalten; die
Bedienbarkeit und Wiederherstellung dieser Grenzfaelle sind noch offen
fuer die interaktive Abnahme. Automatische Tests ersetzen diese Abnahme nicht.

Paseo-Konfiguration: [Workspace-Skripte](https://paseo.sh/docs/worktrees#scripts-and-services).

Beim erstmaligen Windows-Build dieses main-Stands waren zwei lokale
Vorbereitungsschritte notwendig: wiederholte Dateidiffs aus
`0083-local-coop.patch` der Reihe nach anwenden und die deutsche PC-Lokalisierung
vor den Koop-Aenderungen derselben Datei stagen. Ausserdem enthaelt der
`local_session.c`-Abschnitt von `0055-local-wireless.patch` 136 statt der im
Hunk angegebenen 131 Zeilen; fuer den Build wurden alle 136 Zeilen uebernommen.
Die lokalen Hilfsskripte liegen im ignorierten `build`-Ordner dieses Worktrees.
