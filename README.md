# Emerald 3DS – Local Co-op Mod

Experimenteller Local-Wireless-Koop-Mod für [ZallaxDev/emerald-3ds](https://github.com/ZallaxDev/emerald-3ds).

Dieses Repository ist **kein eigenständiger Fork und kein vollständiger Quellcode des Originalprojekts**. `main` enthält ausschließlich Dateien, die für den Mod geändert oder neu angelegt wurden. Unveränderte Originaldateien, ROMs, Spielstände, Datenpakete und Build-Artefakte sind nicht enthalten.

## Auf das Originalprojekt anwenden

Die geprüfte Basis ist **Emerald 3DS v0.5.0**, Commit `bcae4b70dac61419fc933084084469f6cfbe344b`.

```sh
git clone https://github.com/ZallaxDev/emerald-3ds.git
cd emerald-3ds
git checkout bcae4b70dac61419fc933084084469f6cfbe344b
git switch -c feature/local-coop
git remote add mod https://github.com/Sumaiter/emerald-3ds.git
git fetch mod main
git checkout mod/main -- .
```

Der letzte Befehl legt nur die Mod-Dateien über diese Basis. Die übrigen Originaldateien bleiben erhalten. Kein `git merge mod/main` ausführen: Das Overlay enthält absichtlich keine Kopien unveränderter Originaldateien.

Danach gelten die Build- und Installationsanweisungen des Originalprojekts. Für einen deutschen Build wird eine eigene, unveränderte deutsche Smaragd-ROM benötigt:

```sh
python tools/bootstrap.py --make --german-rom smaragd.gba
```

Die ROM wird ausschließlich lokal gelesen und gehört weder in dieses Repository noch in einen Commit des Originalprojekts.

## Enthalten

- Local-Wireless-/UDS-Suche und gezielter Beitritt über eine Sitzungs-ID
- separate Passwort-Authentifizierung im Koop-Protokoll
- versionierte Host-/Gast-Profile
- Übertragung von Position, Blickrichtung, Animation und Avatar-Grafik
- Menüs für Host, Gast und Passwort
- Charakterauswahl mit 104 Originalfiguren, Namen und PC-Handcursor
- Protokoll- und Profiltests
- Patch `patches/pokeemerald/0083-local-coop.patch` für die Engine-Integration
- aktueller Arbeitsstand und verbleibende Laufzeitprüfungen in `Status.md`

Der Mod ist noch keine spielbare Vollversion. Insbesondere die Oberwelt-, Disconnect- und Einzelspieler-Laufzeittests aus Sprint 1 stehen noch aus. Die weiteren Sprints sind in `Status.md` aufgeführt.

## Prüfen

Nach dem Anwenden auf die Originalbasis:

```sh
make -C 3ds_port verify-coop
```

Vor einem Veröffentlichen werden zusätzlich der vollständige Original-Build, die Original-Tests und `python tools/release_audit.py --repo . --strict` ausgeführt.
