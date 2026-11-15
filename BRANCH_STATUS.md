# Upstream-PR-Integration auf Main und Sprint 6

## Sprint-6-Laufzeitfixes: Nachlaufen und Kampfeinstieg (2026-11-15)

Build **6fd2556e4194dce4**, Boot **E3ID-16c4628f404bb3f4**, ABI **837d59ef**, Protokoll 20.

Die gesicherten Nutzerlogs stammen vom vorherigen Boot E3ID-58cdf81623cdba8b.
Beide Emulatoren blieben in BattleMainCB2 aktiv; der beobachtete schwarze
Bildschirm war kein vollstaendiger Prozessstillstand. Der echte Adapterpfad
verwarf Koop-Kampfmeldungen, weil keine klassische RFU-CONNECT-Verbindung
besteht. Der aktuelle authentifizierte Koop-Teilnehmer darf jetzt denselben
nativen Linkkampfkanal verwenden; fremde Teilnehmer und falsche Sitzungen
bleiben ausgeschlossen. Der neue Test reproduziert den alten Fehler rot.

Nachlaufen zeichnet jeden empfangenen Wegpunkt sofort auf. Die Anzeige spielt
Schritte bis zum Ende ab, bevor sie eine Kurve beginnt, und behaelt alle Ecken
eines Paketbursts. Die native Folgebewegung spart die stationaere Drehpause und
uebernimmt das Lauftempo des Anfuehrers unabhaengig vom eigenen B-/Laufmodus.
Nach Menue/Kampf wird kein unsichtbarer alter Bewegungsverlauf nachgespielt.
Abgewiesene Positionssendungen werden sofort erneut versucht.

Beide Route101-Testkaempfe sind solo mit einem kampffaehigen Pokemon moeglich.
Es gelten hoechstens die ersten drei, nicht drei Pflicht-Pokemon. Bei TEST A
bleibt ein fehlender zweiter Spielerplatz leer; Ball-, Shiny- und KP-Animationen
warten darauf nicht. Beide Tests stellen Reserveplaetze vier bis sechs aus der
RAM-Sicherung wieder her, behalten echte Kampfveraenderungen an den ersten drei
und synchronisieren das temporaer gekuerzte Team nicht als dauerhaftes Team.
Wild-Doppel erzeugt den zweiten Gegner eigenstaendig aus der Karten-Begegnungstabelle.
Story-/Troy-Partnerteams bleiben die Originalteams des NPCs.

Hintergrundabnahme bestanden: vollstaendiges verify, Koop-Regressionen,
Burst-/Kurven-/Tempo-/Queue-Retry-Proben, echte native Wild- und Solo-Intro-
Bodies, Route101 mit Teamgroessen eins bis sechs, ARM11-Hardfloat/Link/Assets
und Release/PAK. Alle 102 Patches auf frischem gepinntem Quellstand angewandt;
0101 vorwaerts/rueckwaerts bytegleich, geaenderte Funktionskoerper und Testtexte
stimmen mit dem gebauten Sprachstand ueberein.
Evidenz lokal unter build/coop-sprint6-runtime-fixes/.

**Manueller Nachtest offen.** Laufende Emulatoren, Profile und SAVs wurden nicht
veraendert und verwenden weiter den alten Build. Nicht als Emulator-PASS werten.
Sprint 6 bleibt in Arbeit: gemeinsame EP/EV/Levelup/Attackenlernen und normales
Trainerpreisgeld sind weiterhin offen; Gras-Issue #1 und Zuschauer-Issue #2
werden durch diesen Kampfeinstiegsfix nicht als erledigt markiert.

Build **7e5dc28033a62d97**, Boot **E3ID-58cdf81623cdba8b**, ABI **bcdc1bb8**, Protokoll 20.
12 PRs uebernommen, 7 bewusst ausgelassen/nicht anwendbar. Dauerhafte Merkliste:
docs/UPSTREAM_PR_REVIEW.md und tools/upstream_pr_decisions.json.
Hintergrundchecks, 101-Patchkette, 188 Raumpruefungen und ARM-Release/PAK bestanden;
manuelle Abnahme offen. Eigene Entwicklungsbranches werden regulaer weitergefuehrt.
Sprint 9-Planung umfasst jetzt VM-Feldaktionen dauerhaft ab Erhalt ohne Lernpflicht.

Die folgende Sektion beschreibt den vorausgehenden Build; ihre Spielregeln gelten weiter.

## Sprint-6-Ausbau: aktive Folgegruppe

Version20, Build 092f6412820bb8d8, Boot E3ID-ab63e7ce08e9de5c, ABI 786f1e4c.
A auf freien Mitspieler schliesst sich ihm an; B beendet das Folgen. Nur ein
aktiver Anfuehrer holt seinen verfuegbaren Folgespieler in den Kampf. Ohne
Folgegruppe bleiben Kaempfe solo. Normale Einzel-Trainer werden zu Doppel-
gegnern mit individuellen Ergaenzungen bis maximal sechs; Storyteams bleiben
erhalten. Beide nutzen nur ihre ersten drei, nach einem erschoepften Dreierteam
kaempft der Partner weiter. Story-NPC-Partner wird vom Folgespieler gesteuert,
ohne dessen eigenes Team zu veraendern. Details und manuelle Testliste in
HANDOFF.md/LOKAL_TESTEN.md. Hintergrundtests,100Patchkette und Release gruen;
Emulatorabnahme sowie die bekannte EP-/Preisgeldanbindung noch offen.

Neu: zwei wiederholbare Level5-Doppeltests auf Route101 (Junge ohne Partner,
Troy mit NPC-Partner). Stand in Main und sprint-6; Nutzung in LOKAL_TESTEN.md.
Der inkrementelle Kartengenerator aktualisiert nun auch schon vorhandene Includes.

Die folgenden Angaben beschreiben die vorherige Sprint-6-Integration.

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
