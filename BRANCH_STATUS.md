# Entwicklungsstand: Sprint 6

Basis: aktueller `main`-Commit `1d9419d`, inklusive Sprint 5, aller 99 Figuren
und reparierter Hintergrundtests. Funktionen aus Sprint 6 bleiben in diesem
Branch. `main` erhaelt durch diese Aktualisierung keine Sprint-6-Funktionen.

Beim Merge erhalten: Gastname, Starter-/Wiederbeitrittsablauf, PC-Sperren,
Gruppenabgleich, Fahrradstatus und sequenzierte atomare Kampfergebnisse.
Doppelkampf-Pakete nutzen eigene IDs 40 bis 46; Protokollversion 12.
Der Kampfergebnis-Header hat getrennte Felder fuer Sequenz und Doppelkampfart
und bleibt fuer die folgenden Pokemon-Daten auf vier Bytes ausgerichtet.
Enginepatch 0084-double-coop ist an die Figurengrafik-Patches angepasst.

Die automatisierte Koop-Pruefung und der Patch-Anwendbarkeitstest bestehen
(Sitzungssuite mit einem erwarteten Fehler). Bekannter, bereits im alten Sprint-6-Test markierter
Abnahmefehler: Nach Ablehnung durch den Partner beginnt die lokale
Doppelkampf-Alternative noch nicht ohne eine erneute Anfrage (XFAIL).
Dieser Entwicklungsbranch ist deshalb keine vollstaendig abgenommene Version.
Emulator- und Hardwaretests uebernimmt der Nutzer.
