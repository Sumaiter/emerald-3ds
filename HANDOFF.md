# Übergabe: Emerald 3DS Local Co-op

Stand: 2026-11-08

Dieser Stand ist eine Entwicklungsversion, **keine spielbare Alpha**. Sprint 1 und Sprint 2 sind geprüft. Sprint 3 ist in Arbeit und der aktuelle Beitritts-/Spawnablauf ist fehlerhaft.

## Repository

- Fork: `https://github.com/Sumaiter/emerald-3ds`
- Original: `https://github.com/ZallaxDev/emerald-3ds`
- Arbeitsbranch: `main`
- Letzter veröffentlichter Stand vor dieser Übergabe: `5962cb6 Complete sprint 2 and start sprint 3`
- Die Sprint-3-Änderungen werden mit dieser Übergabe committed und gepusht.

## Was funktioniert und geprüft ist

- Host und Gast verbinden sich über Local Wireless/UDS.
- Hostname und Mod-Version werden in der Spielsuche angezeigt.
- Passwortablehnung und Passwort-Authentifizierung funktionieren.
- Charakterauswahl, Gastname und Host-only-Cheats sind umgesetzt.
- Der lokale Gast-Spielstand bleibt bytegenau unverändert.
- Bewegungsübertragung, fremde Figur und Entfernen beim Disconnect wurden in Sprint 1 bestätigt.
- `make -C 3ds_port verify-coop` besteht für Protokoll und Profil.
- `make release pak` erzeugt 3DSX und PAK erfolgreich.
- `patches/pokeemerald/0083-local-coop.patch` ist mit dem Buildbaum abgeglichen.

## Neue, noch nicht abgeschlossene Sprint-3-Arbeit

Geänderte Dateien:

- `3ds_port/include/3ds_coop.h`
- `3ds_port/include/3ds_coop_protocol.h`
- `3ds_port/src/3ds_coop.c`
- `3ds_port/src/3ds_coop_protocol.c`
- `3ds_port/tests/coop_protocol_test.c`
- `patches/pokeemerald/0083-local-coop.patch`

Implementiert sind Identitäts- und Spawnpakete, vier namensbasierte Spawn-Einträge im Host-Spielstand und ein automatischer Neustart der Host-Sitzung nach dem Verlassen eines Gastes. Die Tabelle liegt in `gSaveBlock1Ptr->unused_3598`, hat Magic `0x5350` und Version `1`.

Die Positionspakete enthalten ObjectEvent-Koordinaten einschließlich `MAP_OFFSET`. `SetWarpDestination()` erwartet dagegen Koordinaten ohne diesen Rand; beim Warp werden daher sieben Felder abgezogen.

## Aktueller Fehler

Der zuletzt getestete Ablauf ist weiterhin fehlerhaft:

- Der Gast erscheint nicht zuverlässig an der gespeicherten Position.
- Host und Gast sehen sich nach dem Beitritt nicht.
- Der Nutzer beschreibt den aktuellen Ablauf als komplett verbuggt.
- Sprint 3 darf deshalb **nicht** als fertig markiert werden.

Die Diagnoseprotokolle zeigten für den Gastnamen `bcbcff` einen gespeicherten Spawn auf Karte `0.9`, Layout `10`, Position `19,16`. Der Host sendet diesen Spawn korrekt. In den fehlerhaften Läufen blieb die Gastposition bei `14,16`, die Save-Position bei `7,9`, und `join warp complete` fehlte.

Zwei Fehler wurden während der Diagnose behoben:

1. Der Generationszähler der Netzwerksitzung wird nun unmittelbar vor dem Beitritts-Warp aktualisiert.
2. Positionspakete werden bis zum Erreichen des Beitritts-Spawns zurückgehalten, damit der alte lokale Gast-Spielstand den Host-Eintrag nicht überschreibt.

Der frühere asynchrone `DoWarp()` nach `CB2_ContinueSavedGame()` wurde durch `WarpIntoMap()` **vor** `CB2_ContinueSavedGame()` ersetzt. Dadurch soll der normale Ladevorgang den Beitritts-Spawn nicht mehr verwerfen. **Diese letzte Änderung ist noch nicht erfolgreich laufzeitgeprüft.**

## Nächste Arbeitsschritte

1. `SETUP.md` einlesen und zwei getrennte Emulatorprofile mit identischem Build einrichten.
2. Den aktuellen Stand zunächst nur mit Host-Beitritt testen, noch ohne Team-, PC- oder Kampffunktionen.
3. In `guest/.../port.log` prüfen:
   - `coop: send saved spawn ... at X,Y` auf Host-Seite,
   - `coop: join wait actual ... target ... save ...`,
   - `coop: join warp complete at X,Y`.
4. Erwartung für den neuen Ablauf: Save-Position ist `spawn - MAP_OFFSET`, ObjectEvent-Position ist gleich dem Spawn. Danach muss `joinWarpPending` enden und beide Figuren müssen erscheinen.
5. Falls `join warp complete` weiterhin fehlt, zuerst `CtrCoop_EnterHostWorld()`, `ContinueJoinedGame()` und `CB2_ContinueSavedGame()` prüfen. Keine Team- oder Kampfarbeit beginnen, bevor dieser Basistest stabil ist.
6. Nach bestandenem Beitritt: Gast bewegen, Host speichern, Gast mit F6 neu verbinden und denselben Spawn prüfen. Erst danach Kartenwechsel und Städte-Spawns testen.
7. Die temporär ausführlichen `join wait`-Logs nach erfolgreicher Diagnose entfernen oder reduzieren.
8. `Status.md` und diese Datei nach jedem bestätigten Test aktualisieren.

## Build- und Testregeln

- Quelle liegt im Repository unter `3ds_port/`.
- Der vollständige decomp-Baum entsteht unter `build/upstream/` und wird nicht committed.
- Vor einem inkrementellen Build geänderte Portdateien auch nach `build/upstream/3ds_port/` kopieren, sonst können veraltete Objekte verwendet werden.
- `main_menu.c` wird im Buildbaum geändert; die Änderung muss zusätzlich in `patches/pokeemerald/0083-local-coop.patch` enthalten sein.
- Nach Patchänderungen `git -C build/upstream apply --check --reverse ../../patches/pokeemerald/0083-local-coop.patch` ausführen.
- Vor dem Abschluss `make release pak`, `make -C 3ds_port verify-coop`, `git diff --check` und `python tools/release_audit.py --repo . --strict` ausführen.
- ROMs, Spielstände, PAKs, Build-Artefakte und Emulatorprofile niemals committen oder hochladen.

## Fortsetzung in einer neuen Paseo-Sitzung

Empfohlener erster Auftrag:

> Lies zuerst `AGENTS.md`, `HANDOFF.md`, `SETUP.md` und `Status.md`. Arbeite an Sprint 3 weiter. Beginne mit dem dort beschriebenen Spawn-/Sync-Basistest und behebe den offenen Fehler. Sprint 1 und 2 nicht als offen behandeln. Keine ROMs, Saves oder Build-Artefakte committen. Halte die Dokumentation aktuell und teste jede Änderung im Emulator.
