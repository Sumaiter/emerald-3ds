# Upstream-PR-Integration auf Main und Sprint 6

## Sprint 6: Nutzerabnahme und Uebergang zu Sprint 7 (2026-11-15)

Build fb72074e4a979dff vom Nutzer insgesamt als funktionierend bestaetigt,
mit offenen Issues #1..4 und #6..8. Issue #5 behoben/bestaetigt. KP, EP,
Trainer-Fluchtsperre und erneuter Einstieg erhalten damit die Nutzerabnahme;
historische Offenvermerke unten sind ueberholt. Die dokumentierte technische
Luecke beim regulaeren Trainerpreisgeld bleibt bestehen. Naechster Schritt:
Sprint-6-Korrekturen pushen und Sprint 7 auf dem aktuellen Stand integrieren.

## Offenes Issue: sprunghafte NPC-Bewegung beim Gast (2026-11-15)

[Issue #8](https://github.com/Sumaiter/emerald-3ds/issues/8): NPC-Bewegung
beim Gast springt zwischen Feldern und soll wie beim Host fluessig aussehen.
Nur erfasst, Ursache und Umsetzung offen; separates Issue zu Nachlaufen #3.

## Offenes Issue: einzelner Spieler-K.-o. im Doppelkampf (2026-11-15)

[Issue #7](https://github.com/Sumaiter/emerald-3ds/issues/7): Ein Spieler ist
nach Verlust seiner Teilnehmer K. o.; der Partner kaempft allein weiter.
Gemeinsame Niederlage erst bei Ausfall beider Teams. Nur erfasst, keine
Implementierung/neue Abnahme; bisherige erfolgreiche Teamtests bleiben erhalten.

## Sprint 6: KP-Einstieg erneut korrigiert, Trainerflucht gesperrt (2026-11-15)

Build **fb72074e4a979dff**, Boot **E3ID-1de947bbc289d035**, ABI **52daf884**, Protokoll 20.

Nutzerabnahme zu 0107: Lohgocks KP-Box fehlt beim ersten Aussenden weiterhin;
nach Oeffnen und Schliessen des Team-/Beutelmenues erscheinen beide. 0107 ist
damit fuer die sichtbare KP-Korrektur NICHT abgenommen. Neue Logkopien aus
Boot E3ID-86d537e6d7603887/ABI e8ef3639 unter
build/coop-hud-trainer-lock/{host,guest}-baseline.log bestaetigen den Build.

0108 baut die KP-Boxen vor der ersten Aktionsauswahl genau einmal mit demselben
nativen CreateHealthboxSprite-Pfad wie die funktionierende Menue-Rueckkehr neu
auf. Beide Endpunkte erhalten den Abschluss ueber Player- und LinkPartner-
CHOOSEACTION; er haengt nicht am nur beim Master laufenden BattleScript.
Trainer-/Ball-/Shiny-Ablauf ist dann beendet. Die alte Spritegruppe wird
freigegeben, alle Boxen erhalten neue Referenzen/Positionen/Anzeigeinhalte.
Spaeteres Ausblenden bei Faint bleibt erhalten; kein Erzwingen pro Frame.
Einmalige Diagnoselogzeilen erfassen alte IDs/Positionen/Sichtbarkeit.
Der genaue urspruengliche Stoerschritt ist weiterhin nicht isoliert. Dies
ist ein durch den Nutzertest begruendeter stabiler Neuaufbau, kein behaupteter
Nachweis eines konkreten GPU- oder OAM-Rennens. Sichtbare Erstkampf-Abnahme offen.

Trainerflucht: Die Auswahl prueft Koop trotz internem LINK-Flag als echten
Trainerkampf und zeigt die normale Trainer-Fluchtverweigerung. Auch ein schon
eingereihtes/stales RUN darf keine PvP-Aufgabe/LOST/WhiteOut ausloesen. Rauchball
und Angsthase umgehen die Trainerregel nicht. Wildflucht behaelt ihre normale
Chance, Sperren und Rueckkehr ins Gras; echte PvP-Aufgabe bleibt unveraendert.

Storybarriere nur erfasst, nicht implementiert:
[Issue #6](https://github.com/Sumaiter/emerald-3ds/issues/6). Am Ausgang des
zweiten Dorfes nach Route 102 kann der Host die noch aktive Sperre passieren,
wenn der Gast schon im Dialog ist. Das Issue verlangt eine allgemeine Loesung
fuer Storybarrieren/Trigger/Uebergaenge beider Spieler, keine Routen-Sonderliste.
Host-/Gastrollen, gleichzeitiger Eintritt, Abbruch, Kartenwechsel und Rejoin
sind dort als Abnahmefaelle notiert. Bestehende Issues #1..4 bleiben separat.

Vollverify/Koop-Suite, ARM11-Hardfloat-Linkaudit (34 Pflichtsymbole, keine starken
unaufgeloesten Symbole), 6388 Assets, ARM-Release und PAK bestanden. 109 Patches
frisch angewandt; 0108 vor-/rueckwaerts bytegleich und sieben volle native
Dateien zur Live-Stage identisch ausser den bestehenden Locale-Literalen
ausserhalb der Hunkbereiche. Native KP-Wiederaufbauproben (beide Endpunkt-
Einstiege, unsichtbar/offscreen/unregistriert/geparkt, einmalig, spaeterer Faint)
und Trainer-Auswahl/queued RUN, Wildflucht, Rauchball/Angsthase, PvP sowie
gezielte Fehler-Mutationen gruen. Diese begrenzten Hostfixtures ersetzen keinen
sichtbaren GPU-/Emulatornachtest. Optionaler MinGW-Sanitizer weiterhin SKIP.
Nachweise/Hashes: build/coop-hud-trainer-lock/{verify.log,release.log,
release-audit.log,patch.log,hp-commit.log,escape.log,build-hashes.json}.

Kein Emulator bedient/gestartet/neu installiert, kein SAV/Profile geaendert,
kein Commit/Push. Nur das explizit beauftragte GitHub-Issue #6 publiziert.
Vorherige Nutzerabnahmen bleiben erhalten; KP und Trainer-Fluchtverweigerung
brauchen die neue sichtbare Abnahme in LOKAL_TESTEN.md. Nachfolgend Historie.

## Sprint 6: KP-Anzeigen, EP und erneuter Kampfeinstieg (2026-11-15)

Build **56417c3a94f30a61**, Boot **E3ID-86d537e6d7603887**, ABI **e8ef3639**, Protokoll 20.

0107 ergaenzt echte Koop-EP/EV, individuelle EP-Meldungen, Levelaufstiege,
Attackenwahl beim jeweiligen Besitzer und dessen Entwicklungen. Vollstaendige
Pokemon-Fortschrittsdaten werden geordnet gespiegelt; nur der Besitzer antwortet.
Die geliehene NPC-Haelfte erhaelt keine EP und veraendert das eigene Team nicht.
Echte PvP-Linkkaempfe bleiben ausgeschlossen. Im Koop entfallen die nur am
Master bedienbaren numerischen Level-Statistikseiten; Levelmeldung und Animation
sowie die eigene Attackenwahl bleiben aktiv.

Vor Entwicklungen wird die volle echte Ownerparty hergestellt. Test A/B
nutzen ihren nativen RAM-Backup; echtes Story-Troy mappt ausgewaehlte Plaetze
(auch 5/2/4) zurueck. Reserven und ein moegliches Ninjatom bleiben auch nach
dem anschliessenden nativen Restore erhalten; volle Teams erzeugen keinen
Zusatz. Kampf-Mongrafiken werden vor der ersten Entwicklung freigegeben.

Der Test-Sieg setzt seine Trainerflags vor der Reservierungsfreigabe. Damit
blockiert der naechste TEST-A/B-Einstieg nicht mehr auf einer alten Lease.
B beim Schliessen des eigenen Nachkampfdialogs beendet das Folgen nicht mehr;
der freie B-Austritt ausserhalb gesperrter Dialoge bleibt erhalten.

KP: Beide Spielercontroller schliessen ihr Aussenden mit einer einmaligen
Registrierungs-/Sichtbarkeitspruefung ab. Eine verlorene HD-Registrierung wird
repariert, ohne andere Boxen zu loeschen. Der urspruengliche Ausloeser der direkt
nach dem Aussenden fehlenden Box ist aus den alten Logs nicht isoliert.
Die native Wiederherstellungsprobe ist gruen; sichtbare Emulatorabnahme bleibt
offen. Kein pauschales Sichtbarerzwingen pro Frame, normale Faint-/Wechselpfade
bleiben bestehen.

Gesamtverify, O2/Werror-Koop-Suite, ARM11-Hardfloat-Linkaudit (34 Pflichtsymbole,
keine starken unaufgeloesten Symbole) und alle 6388 Assets bestanden. ARM-Release
und passendes PAK neu gebaut. Alle 108 Patches auf frischem gepinntem Stand
bestanden; 0107 vor-/rueckwaerts bytegleich, neun volle native Dateien identisch
zur Live-Stage abgesehen von bestehenden lokalisierten Literalen ausserhalb
der Patchhunks. Native EXP/Lern-/Evolutionsproben, 192 Owner-/Restorefaelle,
HP-Intro/Slide/HD/OAM-Identitaet und Wiederholung beider Testkaempfe gruen;
gezielte Fehler-Mutationen erkannt. Die Proben verwenden begrenzte Engine-
Fixtures und Draw-Sinks, keine laufende GPU-/Emulatorabnahme. Optionaler
MinGW-Sanitizer bleibt SKIP.

Nachweise und Artefakthashes: build/coop-hp-exp-return/{verify-release.log,
release-final.log,release-audit.log,patch-final.log,evolution.log,exp-final.log,
build-hashes.json}. Laufende Emulatoren, Profile und SAVs wurden nicht geaendert;
kein Installieren, Neustart, Commit oder Push in dieser Aenderung.

Nachtest siehe LOKAL_TESTEN.md: beide KP-Boxen sofort, EP/Lernen beider Owner,
Sieg -> Dialog mit B schliessen -> weiter folgen -> Troy erneut ansprechen.
Fruehere Nutzerabnahmen bleiben dokumentiert; die neue konkrete Folge wird
dadurch nicht als bestanden vorausgesetzt. Regulaeres Trainerpreisgeld bleibt
separate Implementierungsarbeit. Issues #1..4 bleiben unveraendert offen.
Die nachfolgenden Abschnitte dokumentieren aeltere Buildstaende.

## Sprint 6: Wildflucht und Test-Pokemon (2026-11-15)

Build **7cd1b68323ee18b7**, Boot **E3ID-6ff528623f015338**, ABI **5becd2e8**, Protokoll 20.

Gesamtverify inkl. Koop-Suite und ARM11-Hardfloat-Linkaudit bestanden;
34 erforderliche Symbole, keine starken unaufgeloesten Symbole, alle
6388 Assets geprueft. ARM-Release und PAK neu gebaut. 107 Patches auf
frischem gepinntem Stand bestanden; 0106 bytegleich vor-/rueckwaerts und
Live-Stage-Bodies identisch. Native Fluchttests lesen den tatsaechlich mit
PORT_BRIDGE vorverarbeiteten Skriptpfad; der EndLinkBattle-Skriptabschluss
ist auch im ARM-Objekt vorhanden. Die alten Link-Aufgabe- und fehlender-
Peerabschluss-Mutationen werden erkannt. Optionaler MinGW-Sanitizer bleibt
SKIP; sichtbarer Emulatornachtest der neuen Flucht noch offen.
Nachweise/Artefakthashes: build/coop-flee-pc/{verify-final.log,release.log,
patch.log,script-symbols.log,build-hashes.json}. Emulatoren blieben aus;
nur die ausdruecklich freigegebene PC-Ergaenzung aenderte den Host-SAV.
Lokale Quellenaenderungen sind noch nicht committed/gepusht.

Vom Nutzer als bestanden gemeldet: besiegte Teams, Troys Leihteam,
Speichern/Wiederbeitritt und Verbindungsabbruch. Diese Rueckmeldung ergaenzt
die vorherige Abnahme von Wild- und anderen Doppelkaempfen; nicht erneut
pauschal als ungetestet fuehren. Einzelne nicht genannte Grenzvarianten sind
dadurch nicht automatisch bestaetigt.

Neuer gemeldeter Fehler: Flucht im wilden Koop-Doppelkampf fuehrte zur
Niederlage-/WhiteOut-Rueckkehr. Patch 0106 trennt echte Wildflucht von
Linkkampf-Aufgeben, erlaubt die native Fluchtchance im Wild-Doppel und
uebertraegt das RAN-Ergebnis ueber den normalen EndLinkBattle-Controller.
Rauchball/Angsthase und Fluchtsperren bleiben erhalten. Beide Parteien
kehren ohne Heilung, Geldstrafe oder Ortswechsel mit ihrem Kampfzustand
zurueck. Native Aktions-/Skript-/Controller-/Feldtests und Backendtests fuer
beide Rollen pruefen dies; sichtbare Abnahme des neuen Fixes bleibt offen.

Mit ausdruecklicher Freigabe nach Host-Speichern und Schliessen beider
Emulatoren wurden 12 Test-Pokemon (je zweimal Gewaldro, Lohgock, Sumpex,
Guardevoir, Voltenso, Stolloss; Level 50) in Box 1, Plaetze 1..12, ergaenzt.
Nur vorher leere Slots und die zugehoerige Sektorpruefsumme wurden geaendert.
Alle bisherigen Pokemon, Gaeste-/HP-Metadaten, Party, Story, Items/Geld und
die zweite Savebank blieben bytegleich; Gast-SAV unveraendert. Native
Entschluesselung, Substrukturen, Pruefsummen, Attacken/AP und Ruecklesung
bestanden. Original unter build/coop-flee-pc/host-save.before.sav und direkt
neben dem Host-SAV als emerald3ds.before-team-test-20261115-133303Z.sav.
Nachweise: build/coop-flee-pc/pc-applied.json und pc-native.log.

Offen zum Nachtesten: neue Wildflucht durch beide Spieler, Fangzuordnung,
1/2/6 Pokemon und Dreiergrenzen samt Reserveerhalt, Solo-/Gruppenbeitritt.
EP/EV/Levelaufstieg/Attackenlernen/regulaeres Trainerpreisgeld bleiben
Implementierungsarbeit. Bekannte Issues #1..4 bleiben unveraendert offen;
Nachlaufen und Touch-Zielauswahl sind bewusst zurueckgestellt.

## Sprint 6: Trainerbild im wilden Doppelkampf korrigiert (2026-11-15)

Build **064fbe57f391508b**, Boot **E3ID-6432b8729065aab9**, ABI **896a4be9**, Protokoll 20.

[Issue #5](https://github.com/Sumaiter/emerald-3ds/issues/5) ist lokal behoben,
die sichtbare Emulatorabnahme bleibt offen. Der MULTI-Introzweig emittierte
fuer OPPONENT_RIGHT auch ohne TRAINER-Flag DRAWTRAINERPIC, obwohl derselbe
Battler im wilden Kampf bereits LOADMONSPRITE bekommen hatte. Patch 0105
bindet das gegnerische Trainerbild an einen tatsaechlichen Trainerkampf.
Beide Spieler-Rueckenbilder und die NPC-Trainerkampf-Einstiege bleiben erhalten.

Der erweiterte test_coop_double_intro.py reproduziert den alten Fehler rot
auf dem echten nativen Befehlspfad. Nach Fix bestehen wilde, normale Trainer-
und Zwei-Trainer-Intros auf zwei Endpunkten fuer 0..7 Frames Netzverzoegerung
(24 Konstellationen). Wilde Gegner bekommen je genau einen Pokemon-Ladebefehl
und keinen Trainer-/Trainerballwurfbefehl; Spieler- und Trainer-Sendouts laufen
bis zur Uebergabe an die erste Runde, ohne offene Completion-Bits oder Queueeintraege.
Zwei Mutationen (alter Doppelbefehl und fehlender Wildkampf-Guard) werden erkannt.
Grafik-/Textdauer werden simuliert; kein Emulator-/Hardwaretest behauptet.

Gesamtverify/Koop-Suite, ARM-Release und PAK bestanden. 106 Patches auf frischem
gepinntem Stand anwendbar; 0105 bytegleich reversibel in LF-Normalform und
geaenderter nativer Funktionskoerper entspricht dem Stage. Belege und finale
Artefakthashes: build/coop-wild-trainer. Emulatoren, Profile und SAVs wurden
nicht veraendert. Optionale MinGW-Sanitizer bleiben als SKIP ausgewiesen.

Offen bleiben der manuelle Nachtest fuer #5 sowie Touch-Zielauswahl #4,
Nachlaufen #3, Zuschauer #2 und Gras #1. Sprint 6 ist weiterhin nicht abgenommen.
Nachfolgende Abschnitte beschreiben vorherige Staende.

## Sprint 6: NPC-Partner-Intro und offenes Nachlauf-Issue (2026-11-15)

Build **b0897187b8da4250**, Boot **E3ID-3d4d9019241c1602**, ABI **aac449b6**, Protokoll 20.

Nachlaufen bleibt laut erneutem Nutzertest asynchron und ist als eigenes
[GitHub-Issue #3](https://github.com/Sumaiter/emerald-3ds/issues/3) erfasst.
Weitere Bewegungsaenderungen sind auf Nutzerwunsch zurueckgestellt; der
bisherige Follow-/Kartenwechselstand bleibt erhalten.

Der NPC-Partnerkampf TEST B/Troy scheiterte weiterhin im Boot
E3ID-68d0d5a414085f28. Beide aktuellen Logs sind unter
build/coop-sprint6-npc-partner gesichert. Read-only-Fensterbeobachtung:
Host zeigt beide Gegnertrainer, Gast bereits keine; beide stehen noch vor
dem Aussenden der Pokemon. CB2=001c72e8 entspricht im passenden alten ELF
BattleMainCB2; laufende Frames belegen keinen vollstaendigen Prozessstillstand.
Keine Eingaben, Emulatorneustarts, Installationen oder SAV-/Profilaenderungen.

Konkret reproduzierter Intro-Fehler: MULTI und TWO_OPPONENTS emittierten
zusammen DRAWTRAINERPIC fuer den zweiten Gegner zweimal. Fuer diesen Battler
existiert nur ein Satz Completion-Bits; der zweite Befehl konnte eine weitere
Spriteinstanz erzeugen und mit Quittierungen des ersten ueberlappen. Patch
0104 macht die beiden Emissionszweige gegenseitig ausschliessend. Der native
Einzelspielerpfad mit nur TWO_OPPONENTS bleibt erhalten.

Neuer Test test_coop_double_intro.py benutzt die echten nativen Intro-Phasen
bis zum Aussenden aller vier Battler, die echten Send-/Empfangstasks und die
vier echten Controller-Abschlussfunktionen auf zwei getrennten Endpunkten.
Ein/zwei Gegner und 0..7 Frames Leitungsverzoegerung: ein Draw/Throw je Bank,
keine offenen Completion-Bits oder Queueeintraege. Wiedereinsetzen des alten
Doppelbefehls wird zur Laufzeit erkannt. Grafik-/Textdauer sind Fixturewerte;
dieser Nachweis ersetzt keine sichtbare Emulatorabnahme und behauptet keine
vollstaendige Reproduktion der beobachteten Wartestelle.

Gesamtverify/Koop-Suite, ARM-Release und PAK bestanden. Alle 105 nativen
Patches auf frischer gepinnter Kopie anwendbar, 0104 bytegleich reversibel in
LF-Normalform; geaenderter Funktionskoerper entspricht dem Live-Stage.
Optionale MinGW-ASan/UBSan-Proben bleiben als SKIP ausgewiesen.

Offen: manueller Nachtest des neuen NPC-Partnerkampfs. Sprint 6 ist damit
weiter nicht abgenommen; bisherige offene EP/EV/Level-/Attacken-/Preisgeld-
Punkte bleiben bestehen. Gras-Issue #1 und Zuschauer-Issue #2 bleiben offen.
Nachfolgende Abschnitte beschreiben vorherige Staende.

## Sprint 6: zweite Korrektur nach Emulatorfeedback (2026-11-15)

Build **cc52f03a239453b8**, Boot **E3ID-68d0d5a414085f28**, ABI **3ab38ed2**, Protokoll 20.

Die erneuten Nutzerfehler sind fuer den zuvor aktuellen Boot E3ID-16c4628f404bb3f4
belegt. Host-/Gastlogs wurden unter build/coop-sprint6-wave2 gesichert, ohne
Emulatorbedienung, Installation oder SAV-/Profilveraenderung.

- Nachlaufen: begrenzter zweiter nativer Bewegungstick bei Rueckstand; kein
  Unterbrechen laufender Schritte, keine neuen Wegpunkte oder Teleports.
  Idle-Posen werden vor dem Sendevergleich normalisiert, damit ein stehender
  Spieler nicht irrtuemlich jedes Frame ein weiteres Positionspaket erzeugt.
- Gebietswechsel: authentifiziertes Exitpaket 86 mit geordnetem Retry; alle
  alten Koop-Objekte kartenunabhaengig entfernen. Session-Avatare werden weder
  gespeichert noch wieder aus alten Saves geladen; wiederverwendete fremde
  Spriteplaetze bleiben erhalten. Nativer Patch 0102.
- Doppelkampf: sechs feste Battle-Plaetze trotz Leerstellen zwischen den
  Dreiergruppen. Das originale eigene Team wird vor seiner normalen Zaehlung
  wiederhergestellt. Beide Touchansichten zeigen das eigene Dreierteam links
  und den Partner rechts; die nativen Auswahl-/Besitzregeln bleiben wirksam.
- Beide vereinbarten Figur-IDs werden im gemeinsamen Kampfsetup eingefroren.
  Eigene und Partner-Rueckenbilder behalten getrennte Frame-/Palettentabellen,
  auch beim Ballwurf. Native ausgewaehlte Figuren behalten ihre Animationen.
- Der Gast zeichnet die echten NPC-Trainer wie der Host; klassischer
  Linkgegner-/Spielertext wird fuer den gemeinsamen KI-Kampf vermieden.
  Beide Seiten bestaetigen Darstellungsbefehle wieder vor deren Fortschritt.
  Die irrefuehrende originale RFU-Funkanzeige entfaellt im Koop-Kampf.
  Nativer Patch 0103; klassische echte Linkkaempfe bleiben im bisherigen Pfad.

Hintergrundpruefung: vollstaendiges verify, Koop-Suite, native Darstellungs-
und Teamreihenfolgeproben fuer 1..3 + 1..3 in beiden Rollen, sieben erkannte
Regressionsmutanten, getrennte gleichzeitige Figurenbilder/Wurfpaletten,
NPC-Teambesitz und Rueckkehr sowie Bewegungs-/Praesenz-/SaveLoad-Regressionen.
Gesamter ARM11-Hardfloat-Link, Assets und Release/PAK bestanden. Alle 104
Patches auf frischem gepinntem Stand angewandt; 0102 und 0103 reversibel,
0103 bytegleich in LF-Normalform. MinGW kann die optionalen ASan/UBSan-Proben
weiterhin nicht linken; diese sind als SKIP protokolliert, nicht bestanden.

Der erste Gesamtbuild fand im neuen load_save-Hook einen fehlenden Include
fuer LOCALID_COOP_PLAYER. Header und 0102 wurden korrigiert; alle drei ganzen
nativen Dateien danach separat mit devkitARM und anschliessend im Gesamtbuild
kompiliert. Die kleine extrahierte Fixture hatte diesen Include-Fehler verdeckt.

Der neue Emulatornachtest ist OFFEN. Noch keine Laufzeitzusage fuer diese
Korrektur und kein Abschluss von Sprint 6. EP/EV/Levelaufstieg/Attackenlernen
und Preisgeld im gemeinsamen nativen LINK-Kampf bleiben die bereits separat
notierten Sprint-6-Punkte. Gras-Issue #1 und Zuschauer-Issue #2 bleiben offen.
Die nachfolgenden Abschnitte beschreiben aeltere Staende.

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
