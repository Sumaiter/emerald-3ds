# Host und Gast auf diesem PC starten

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
- Aktueller gemeinsamer Release-Build: `03ecd34074355974`, Datenpaket-ABI
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
