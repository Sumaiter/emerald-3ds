# Ersteinrichtung auf einem PC

Diese Anleitung richtet Emerald 3DS mit dem Local-Co-op-Mod unter Windows ein. Der aktuelle Mod ist eine Entwicklungsversion; der offene Teststand steht in `HANDOFF.md`.

## Entwicklungsbranch auswählen

- `main`: integrierter Stand bis Sprint 5 mit allen 99 Figuren
- `sprint-6`: aktueller main plus gemeinsame Doppelkämpfe
- `sprint-7`: aktualisierter Sprint 6 plus Abmeldung und Abbruchbehandlung
- `sprint-8`: aktualisierter Sprint 7 plus Stabilisierung und Testvorbereitung

Beispiel: `git switch sprint-8`. Vor einem Build die Portdateien desselben Branches nach `build/upstream/3ds_port/` kopieren. Keine Spielstände oder ROMs aus einem anderen Branch oder PC übernehmen, ohne vorher eine Sicherung anzulegen.

## Was zusätzlich zum Repository benötigt wird

Zum **Spielen eines bereits erzeugten Builds**:

- Azahar, getestet wurde Version **2126.2** (MSYS2-Build): `https://github.com/azahar-emu/azahar/releases`
- die erzeugten Dateien `Emerald3DS.3dsx`, `Emerald3DS.smdh` und `emerald3ds.pak`
- für zwei Spieler auf einem PC: zwei getrennte Azahar-Programmkopien und `azahar-room.exe`

Zusätzlich zum **Selbstbauen**:

- Git
- Python **3.11 oder neuer** mit Pillow
- devkitPro mit **devkitARM**, **libctru**, **citro2d**, **citro3d**, **3dstools** und `picasso`
- GNU Make und ein nativer C/C++-Compiler, unter Windows MSYS2 MinGW64 GCC/G++
- eine eigene, unveränderte deutsche Pokémon-Smaragd-ROM (**BPED**), sofern ein deutscher Build erzeugt werden soll

Die ROM ist nicht im Repository und darf nicht committed oder hochgeladen werden. Spielstände, PAKs und Build-Artefakte gehören ebenfalls nicht in Git.

## 1. Build-Werkzeuge installieren

Git und Python können über ihre offiziellen Windows-Installationen installiert werden. Danach Pillow installieren:

```cmd
py -m pip install Pillow
```

devkitPro stellt für Windows eine MSYS2-Umgebung bereit. In der **devkitPro-/MSYS2-Shell** die benötigten Pakete installieren beziehungsweise kontrollieren:

```sh
pacman -S --needed devkitARM libctru citro2d citro3d 3dstools make \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-g++
```

Die Shell muss folgende Werte verwenden:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
export PATH="/mingw64/bin:/opt/devkitpro/devkitARM/bin:/opt/devkitpro/tools/bin:$PATH"
```

Kontrolle:

```sh
arm-none-eabi-gcc --version
gcc --version
make --version
picasso --version
python --version
```

Je nach Installation kann `python` auf das Windows-Python oder das MSYS2-Python zeigen. Wichtig ist Python 3.11+ mit Pillow.

## 2. Repository und eigene ROM bereitstellen

```sh
git clone https://github.com/Sumaiter/emerald-3ds.git
cd emerald-3ds
```

Eine eigene deutsche ROM außerhalb von Git ablegen, beispielsweise unter `C:/Spiele/Pokemon Smaragd-Edition.gba`. Der Builder liest sie nur lokal.

## 3. Decomp-Baum und Spiel bauen

Alle folgenden Build-Kommandos in der devkitPro-/MSYS2-Shell ausführen:

```sh
python tools/bootstrap.py --make --german-rom "C:/Spiele/Pokemon Smaragd-Edition.gba"
```

`bootstrap.py` lädt den in `upstream.lock` festgelegten `pret/pokeemerald`-Commit, wendet die Patches an und spiegelt Port, Builder und Werkzeuge nach `build/upstream/`.

Danach das Release und das zugehörige Datenpaket erzeugen:

```sh
make -C build/upstream/3ds_port release pak -j8 PYTHON=python
```

Die benötigten Dateien liegen anschließend hier:

- `build/upstream/3ds_port/dist/Emerald3DS.3dsx`
- `build/upstream/3ds_port/dist/Emerald3DS.smdh`
- `build/upstream/3ds_port/build/emerald3ds.pak`

Bei späteren Änderungen die geänderten Dateien aus `3ds_port/` auch nach `build/upstream/3ds_port/` kopieren und den Build erneut ausführen. Ein Bootstrap mit `--clean` setzt den Buildbaum zurück und sollte nur bewusst verwendet werden.

## 4. Azahar für zwei Spieler einrichten

Azahar zweimal in getrennte Verzeichnisse installieren, zum Beispiel:

```text
<TOOLS>/azahar/host/azahar.exe
<TOOLS>/azahar/guest/azahar.exe
```

In jedem Verzeichnis ein Unterverzeichnis `user` anlegen. Dadurch verwendet Azahar portable, getrennte Profile für Einstellungen, NAND, Spielstände und SD-Karten.

Die drei Build-Dateien auf **beide** emulierten SD-Karten kopieren:

```text
<TOOLS>/azahar/host/user/sdmc/3ds/emerald3ds/
<TOOLS>/azahar/guest/user/sdmc/3ds/emerald3ds/
```

Beide Instanzen müssen denselben 3DSX-Build und dasselbe PAK erhalten, weil die Netzwerkverbindung die Build-ID vergleicht. Für Diagnoseausgaben in jedem Spielverzeichnis eine leere Datei `debug.txt` anlegen. Das Spiel schreibt dann `port.log` und behält die vorherige Sitzung als `port-prev.log`.

Optional kann der Host-Testspielstand aus <https://github.com/Sumaiter/emerald-3ds/releases/tag/test-save-2026-11-10> verwendet werden. Das Release enthält nur die SAV-Datei, keine Spieldateien. Vorhandene Spielstände zuerst sichern und die heruntergeladene Datei als `emerald3ds.sav` im Host-Spielverzeichnis ablegen.

## 5. Lokalen Mehrspielerraum starten

Für den UDS-Test mit Azahar wird ein Raumprozess benötigt. Die separate Datei `azahar-room.exe` verwenden, nicht `azahar.exe --room`.

Beispiel auf einem PC:

```sh
azahar-room.exe --room-name "Emerald Koop" \
  --room-description "Lokaler Test" \
  --port 24872 --max_members 2
```

Ist der Port belegt, beim Raumstart einen freien alternativen Port verwenden, beispielsweise 24873. Beide Clients müssen denselben Port erhalten.

In **beiden** Azahar-Oberflächen im Multiplayer-Menü **Direkt mit Raum verbinden / Direct Connect** wählen:

- Adresse: `127.0.0.1`
- Port: `24872` oder den gewählten Alternativport
- Passwort: leer, sofern keines gesetzt wurde

Danach HOST und GAST jeweils `Emerald3DS.3dsx` starten. Im Spiel öffnet der Host unter Optionen **LOKALER KOOP**; der Gast wählt im Hauptmenü **SPIEL SUCHEN** und gibt das im Hostprofil festgelegte Koop-Passwort ein.

Bei zwei PCs im selben LAN läuft der Raum auf einem PC. Der andere verbindet sich statt mit `127.0.0.1` mit dessen LAN-Adresse. Ein Internetserver ist für den vorgesehenen lokalen Test nicht erforderlich.

## 6. Prüfungen

```sh
make -C 3ds_port verify-coop
python -m unittest discover -s builder/tests -v
python tools/release_audit.py --repo . --strict
git diff --check
```

Zusätzlich die Engine-Patches in umgekehrter Reihenfolge prüfen:

```sh
git -C build/upstream apply --check --reverse ../../patches/pokeemerald/0084-double-coop.patch
git -C build/upstream apply --check --reverse ../../patches/pokeemerald/0083-local-coop.patch
```

Die manuellen Beitritts-, Kampf-, PC-, Abmelde- und Regressionstests stehen in `HANDOFF.md`. Die aktuelle interaktive Abnahme ist noch nicht bestanden; reale 3DS-/2DS- und Voxel-Tests sind ebenfalls noch ausstehend.

## Was nicht mit umgezogen wird

`.tools/`, `build/`, ROMs, Spielstände, PAKs, 3DSX-Dateien, NANDs und die Azahar-Profile sind lokale Daten. Auf einem neuen PC werden nur das Git-Repository, die oben genannten Werkzeuge und gegebenenfalls eine eigene ROM benötigt. Für reine Spieltests kann statt eines neuen Builds auch das zusammengehörige 3DSX-/SMDH-/PAK-Set vom anderen Rechner kopiert werden.
