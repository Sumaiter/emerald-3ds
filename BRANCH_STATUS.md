# Entwicklungsstand: Sprint 6 in Main

Basis: aktueller Koop-Main612a28d; Sprint6ea3d332 ist integriert.
Upstream ZallaxDev/main am2026-11-14 neu abgerufen: bcae4b7 bereits enthalten,
keine neueren fehlenden Merges. Nur Main und Sprint6 betreffen diese Lieferung;
Sprint7/8-Funktionen bleiben eigene folgende Entwicklungsstaende.

Protokoll19, Doppelkampf-IDs40..46, Ergebnisheader20Byte mit getrennten Feldern
fuer Sequenz und Doppelkampfart. Alle neueren Story-/Inventar-/Wallet-/HP-/
Autosave-/Hostverlust-/Zuschauer-Korrekturen aus Main bleiben enthalten.

Behoben: Ablehnungs-Fallback ohne erneute Anfrage, richtige Gast-Teamplaetze,
idempotentes Fullparty-Restore vor WhiteOut, getrennte lokale Rueckkehr ohne
Callbackpointer im Paket, Wiederholung angenommener Einladungen, Ausschluss
belegter Menue-/Skript-/PC-/Transferkontexte und Live-Ledger im Koopkampf.
Der bisherige Ablehnungs-XFAIL ist entfernt; regulaere Regression ist gruen.

Hintergrundtests und ARM-Release/PAK bestehen. Alle98Enginepatches lassen sich
in Bootstrap-Reihenfolge anwenden;0083 wurde dazu ohne neue Funktion aus
sequentiellen Teilpatches zu genau einem Diff pro Datei normalisiert.
Details/Buildkennungen/Belege oben in HANDOFF.md.

NICHT abgeschlossen: Emulator-/Hardwareabnahme des gemeinsamen Kampfes.
EP/EV/Levelup/Lernen und normales Trainerpreisgeld brauchen noch eigene
Koop-Anbindung: native BATTLE_TYPE_LINK-Pfade ueberspringen diese Spiellogik.
Keine pauschale Freigabe der Ergebniszuordnung aus den gruenen Transfer-/
Team-/Ledger-Tests ableiten. Sprint6 bleibt deshalb in Arbeit.

Zurueckgestellt: getrennte GitHub-Issues #1 (entferntes Gras) und #2
(Zuschauer-Ruckeln). Keine neuen Optimierungsreste; Emulatoren/SAVs/Profiles
wurden nicht veraendert. Weitere Benutzerabnahme macht der Nutzer selbst.
