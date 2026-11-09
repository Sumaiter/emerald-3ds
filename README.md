# Emerald 3DS – Local Co-op Mod

Experimenteller Local-Wireless-Koop-Mod von **Sumaiter**, basierend auf [Emerald 3DS by Zallax and contributors](https://github.com/ZallaxDev/emerald-3ds).

Dieser Fork enthält den vollständigen Originalquelltext sowie die Änderungen des Mods. Er ist **kein offizielles Projekt von ZallaxDev** und steht nicht unter dessen Schirmherrschaft oder Empfehlung.

## Geänderte und neue Funktionen

- Local-Wireless-/UDS-Suche und gezielter Beitritt über eine Sitzungs-ID
- separate Passwort-Authentifizierung im Koop-Protokoll
- versionierte Host-/Gast-Profile
- Übertragung von Position, Blickrichtung, Animation und Avatar-Grafik
- Menüs für Host, Gast und Passwort
- Charakterauswahl mit 84 Originalfiguren, Namen und PC-Handcursor
- Spielstandschreiben und cheatähnliche Spielregeln nur für den Host
- Entfernen der Gastfigur beim Verbindungsabbruch
- Titelzeile `Coop by Sumaiter` und separate Mod-Version
- Protokoll- und Profiltests
- Integrationspatch `patches/pokeemerald/0083-local-coop.patch`

Ausgeschlossene Figuren ohne echte Laufbewegung sind in `ZEROWALK.md` dokumentiert. Der aktuelle Entwicklungsstand und die übrigen Sprints stehen in `Status.md`. Für die Arbeit an einer neuen Sitzung steht die aktuelle Diagnose in `HANDOFF.md`; die Ersteinrichtung auf einem PC beschreibt `SETUP.md`. **Sprint 3 ist in Arbeit; Host-Snapshot, Spawn, Rejoin und Bewegung funktionieren bereits, weitere Oberweltfunktionen fehlen noch.**

## Bauen

Die geprüfte Originalbasis ist **Emerald 3DS v0.5.0**, Commit `bcae4b70dac61419fc933084084469f6cfbe344b`.

```sh
git clone https://github.com/Sumaiter/emerald-3ds.git
cd emerald-3ds
python tools/bootstrap.py --make --german-rom smaragd.gba
```

Für den deutschen Build wird eine eigene, unveränderte deutsche Smaragd-ROM benötigt. Die ROM wird ausschließlich lokal gelesen. ROMs, Spielstände, Datenpakete und Build-Artefakte gehören nicht in einen Commit.

## Prüfen

```sh
make -C 3ds_port verify-coop
python -m unittest discover -s builder/tests -v
python tools/release_audit.py --repo . --strict
```

Vor einer Veröffentlichung wird außerdem der vollständige 3DS-Build ausgeführt.

## Lizenz und Hinweise

Die Originalarbeit von **Zallax and contributors** und die Modifikationen werden unter [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) geteilt:

- Namensnennung von Emerald 3DS by Zallax and contributors und Verweis auf das Originalprojekt
- keine kommerzielle Nutzung, kein Verkauf und keine monetarisierten Builds oder Bundles
- Weitergabe abgeleiteter Werke unter derselben Lizenz

`LICENSE-PORT.md` und `NOTICE.md` bleiben erhalten und beschreiben Umfang und Drittkomponenten. Pokémon-Code, Grafiken, Texte und andere Spieldaten werden dadurch nicht neu lizenziert und sind nicht Bestandteil dieses Repositorys.
