# Uebergabe: Emerald 3DS Local Co-op

## Sprint 7: beide Nachtests vom Nutzer bestanden (2026-11-15)

Build 8af20c2c7a809005: normales Gast-Abmelden und anschliessender Wiederbeitritt/
Host-Speichern-Neuladen mit korrektem Team/KP/AP/Status ohne Duplikate bestaetigt.
Damit neue Sprint-7-Abmeldung abgenommen; fruehere Abbruchtests aus Sprint 6
bleiben gueltig. Historische Offenvermerke unten sind insoweit ueberholt.
Offene Issues #1..4 und #6..8 bleiben bestehen. Naechster Auftrag: Sprint 8 mergen.

## Sprint 7: Integration nach Sprint-6-Nutzerabnahme (2026-11-15)

Build **8af20c2c7a809005**, Boot **E3ID-4fe482705fd68761**, ABI **a8444026**, Protokoll 21.

Sprint 6 als 404dd82 auf origin/main gepusht. Sprint-7-Merge von b6bb91c:
Logout 47..50 auf Protokoll 21 portiert; aktuelle Doppel-/Follow-Paketgroessen
erhalten. Alter PeerLeft aus dem Branch verworfen, damit Host-Solokaempfe
weitergehen und sicherer Gast-longjmp/Savebarrieren erhalten bleiben.
Komplette finale Party vor Row-Unlock; Pruefsumme, wiederholbare Pulls,
ACK-Replay und Schutz gegen spaete Party-/PC-Pakete. Kein Kampf-/Leihteam als
kanonische Logoutparty; unsichere Abmeldung bleibt normaler Stack-Abbruch.
CtrCoop_Leaving sperrt Gast-Tasten/Touch bis ACK/Timeout. Host merkt Autosave
nach Commit vor und wartet weiter auf sicheren Kontext. Keine SAV-/Emulator-
Bedienung. Issue #5 bestaetigt behoben und geschlossen, #1..4/#6..8 offen.
Neue coop_logout_test.c samt Make-Integration; native UI-Keyfilter-Regression
erweitert. Gesamtverify/ARM-Release/PAK/Linkaudit bestanden; manueller Nachtest offen.
Nachweise build/sprint7-integration/. Testliste oben in LOKAL_TESTEN.md.

## Sprint 6: Nutzerabnahme und Uebergang zu Sprint 7 (2026-11-15)

Nutzer bestaetigt: "ansonsten klappt jetzt alles von sprint 6". Build
fb72074e4a979dff einschliesslich KP-/EP-/Trainerflucht-/Reentry-Nachtests ist
damit abgenommen; bekannte offene Issues #1..4 und #6..8 bleiben zurueckgestellt.
Issue #5 ist behoben/bestaetigt. Nicht einzeln benannte Spezialfaelle daraus
nicht als separat getestet ableiten; regulaeres Trainerpreisgeld ist weiterhin
eine dokumentierte Implementierungsluecke. Aeltere offene Abnahmehinweise unten
sind historisch. Auftrag: Stand pushen, Sprint 7 mergen und Testliste liefern.

## Offenes Issue: sprunghafte NPC-Bewegung beim Gast (2026-11-15)

[Issue #8](https://github.com/Sumaiter/emerald-3ds/issues/8) separat erfasst:
NPCs springen beim Gast sichtbar von Feld zu Feld; Bewegung soll wie beim
Host fluessig sein. Tatsaechlicher Respawn oder direktes Versetzen nicht
untersucht. Abnahme autonomer/geskripteter Bewegung und Kartenwechsel im Issue.
Keine Implementierung/Abnahme; nicht mit Spieler-Nachlaufen #3 zusammenlegen.

## Offenes Issue: einzelner Spieler-K.-o. im Doppelkampf (2026-11-15)

[Issue #7](https://github.com/Sumaiter/emerald-3ds/issues/7) separat erfasst:
Ein Spieler ohne kampffaehige Teilnehmer ist K. o.; der Partner bleibt und
kaempft allein weiter. Gemeinsame Niederlage erst bei Ausfall beider Teams.
Keine neue Implementierung/Abnahme; konkrete aktuelle Fehlfolge nicht berichtet.
Abnahme beider Rollen, Teamgroessen 1 bis 3 und Leihteam siehe Issue. Die
frueher bestaetigten Tests besiegter Teams nicht rueckwirkend umklassifizieren.

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

## Neues Sprint-6-Issue: Trainermodell im wilden Doppelkampf (2026-11-15)

[Issue #5](https://github.com/Sumaiter/emerald-3ds/issues/5): Nutzerbild zeigt
im wilden Doppelkampf gegen Waumpel/Fiffyen auf einer Seite einen Gegner-
Trainer hinter Waumpel; auf der anderen Seite fehlt dieses Modell. Der
Kampftext bezeichnet die Pokemon korrekt als wild. Betroffene Host-/Gast-
bzw. Fuehrungsrolle, Laufzeit-Build und Ursache noch nicht gesichert.
Frische Sitzung sowie Folgebegegnung nach Trainer-/NPC-Partnerkampf spaeter
pruefen. Nur als eigenes offenes Issue aufgenommen, keine Codeaenderung.

## Neues Sprint-6-Issue: Touch-Zielauswahl beim Folgespieler (2026-11-15)

[GitHub-Issue #4](https://github.com/Sumaiter/emerald-3ds/issues/4): Nach der
Attackenwahl kann der folgende Spieler im Doppelkampf kein Angriffsziel per
Touch waehlen. Im Nutzerbild fehlen auf seiner Seite die Zielkarten; nur
ZURUECK ist vorhanden. Der andere Spieler sieht die Zielkarten. Gemeldeter
Fall mit uebernommenem NPC-Team (Stollunior); beide Fuehrungsrollen sowie
normale gemeinsame Doppelfaelle spaeter gegenpruefen. Noch keine isolierte
Reproduktion, Ursachenbestaetigung oder Korrektur; auf Nutzerwunsch nur als
eigenes Issue aufgenommen. Der Screenshot-Build wurde nicht separat bestimmt.

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

## Upstream-PR-Abgleich und Merkliste (2026-11-14)

Build **7e5dc28033a62d97**, Boot **E3ID-58cdf81623cdba8b**, ABI **bcdc1bb8**, Protokoll 20.
12 offene Upstream-PRs integriert, zwei hier nicht anwendbar, fuenf bewusst
ausgelassen. Die Entscheidungen mit vollem Head-Commit und Integrationscommit
stehen in [docs/UPSTREAM_PR_REVIEW.md](docs/UPSTREAM_PR_REVIEW.md) und
[tools/upstream_pr_decisions.json](tools/upstream_pr_decisions.json).
AGENTS.md verlangt, unveraenderte Heads beim naechsten Abgleich zu ueberspringen.
Der abschliessende API-Abgleich bestaetigt dieselben 19 Heads; Upstream/main
bleibt bcae4b70dac61419fc933084084469f6cfbe344b.

Neu: D-Pad auf der Fliegenkarte, zusammengehoerige PID/IVs fuer neue sichtbare
Wild-Pokemon, transparente leere Untergrund-Raender, vier gestapelte
Innenraumkorrekturen, optionale Boot-Seed-Einstellung, franzoesisch/italienische
Starttexte, Pinball-ROM-Hinweis, Linux/macOS-Anleitung und optionale macOS-
Paketierung. Koop-/Zuschauerpfade und sieben Sprachen bleiben erhalten.
Die zwei Route101-Doppeltests und alle Folgegruppenregeln gelten weiter.

Zusaetzliche direkt gefundene Build-/Testfehler repariert: mapjson schreibt
Meta-Includes aus dem nativen Root statt mit falschem ../data-Praefix;
zusammengehoerige Ausgaben sind gruppierte Makeziele und muessen vor beiden
Kartenobjekten fertig sein. Echte -j8-Makeregeln auf isolierten JSON-Kopien
geprueft. Relative Testpfade funktionieren auch im gestagten Port. Der englische
Locale-Hashpruefer erkennt jetzt auch echte deutsche/japanische/PT-BR-Marker;
seine englischen Hash-/Manipulationschecks bleiben unveraendert.

Validierung: vollstaendiges Verify im deutschen gestagten Baum bestanden
(ein erwarteter Skip der nur vor Lokalisierung sinnvollen englischen
Source-Hash-/Stagerklasse), Koop-Suite auch im Quelloverlay bestanden;
neue reale Fly/IV-Funktionskoerper, Seed-, sieben Startup-Sprachen-, Builder-
und Paketplattform-Tests bestanden. 188 Raumkompositionen/Innenraum-Lint ohne
Fehler,77 begruendete bestehende Lint-Ausnahmen. Alle 101 Patches auf frisch
extrahierter gepinnter Quelle angewandt, 0100 bytegleich vorwaerts/rueckwaerts
und mit dem Build verglichen. ARM-Release+PAK bestanden. Verlinkte Route101-
Events im tatsaechlichen Datenimage haben [8,0,9,1]; gMapGroups ist vorhanden.
Strenges Source-Release-Audit separat dokumentiert. Emulatoren/SAVs wurden
nicht bedient oder installiert; manuelle Abnahme ist offen.

Zum Laden beide Fenster schliessen und Paseo **Host und Gast starten** manuell
ausfuehren. F6 installiert keinen Build. Neue manuelle Smokechecks:
Fliegenkarte mit D-Pad/Zoom/A/B, neuer sichtbarer Wildfang und die zwei
Route101-Tests solo/als Folgegruppe mit beiden Anfuehrerrollen. Bekannte
Gras#1-/Zuschauer#2-Issues und Sprint 6-EP-/Preisgeldluecken bleiben offen.
Sprint 9 enthaelt dauerhafte VM-Feldaktionen ab Erhalt ohne Lernpflicht;
das ist weiterhin Planung.

Belege: build/upstream-pr-review/verify-retry.log,verify-coop.log,
verify-coop-staged.log,startup-tests.log,builder-tests.log,platform-tests.log,
imported-bodies.log,rng-tests.log,patch-chain.log,release-final.log,
build-hashes.json und audit.log. Fuer die Hostwerkzeuge HOSTCC=gcc exportieren.

## Route101: zwei wiederholbare Doppelkampf-Tests (2026-11-14)

Build **092f6412820bb8d8**, Boot **E3ID-ab63e7ce08e9de5c**, ABI **786f1e4c**, Protokoll20.
Hintergrundtests und ARM-Release/PAK bestanden; noch kein Emulatornachtest.

Wiederholbare Testkaempfe auf **Route101**, ganz im Norden vor Rosaltstadt:

- Links **Junge / TEST A** bei (8,3): gewoehnlicher Doppelkampf ohne
  NPC-Partner, vier verschiedene Gegner auf Level5. Zwei kampffaehige eigene
  Pokemon auf den ersten drei Plaetzen erforderlich.
- Rechts **Troy / TEST B** bei (12,3): Story-Partner-Doppelkampf gegen zwei
  Trainer mit je drei verschiedenen Pokemon auf Level5. Troy bringt
  Stollunior, Puppance und Tanhel auf Level5 mit. Mindestens ein kampffaehiges
  eigenes Pokemon auf den ersten drei Plaetzen erforderlich.
- Beide mit A ansprechen und bestaetigen. Fuer Koop zuerst mit A dem anderen
  Spieler folgen; der Anfuehrer startet dann den Test. Ohne Folgegruppe ist A
  ein normales eigenes Doppel, B ein Doppel mit computergesteuertem Troy.
  Mit Folgegruppe steuert bei B der Folgespieler Troys drei Pokemon; sein
  eigenes ganzes Team bleibt erhalten.
- Nach jedem Ende erneut ansprechbar. HP/AP/Status und verbrauchte Mittel
  haben die normale Wirkung; kein automatisches Vollheilen fuer einen Test.
  Niederlage verwendet den nativen WhiteOut. Reserveplaetze des Anfuehrers
  werden beim Partner-Test vor Ergebnis/Speichern unveraendert restauriert.
- Beide Emulatorfenster schliessen und Paseo **Host und Gast starten** manuell
  ausfuehren. F6 installiert keinen Build. Route101 verlassen und neu betreten,
  falls ein alter Spielstand noch die zuvor geladenen Kartenobjekte enthaelt.

Manuell noch offen: beide Tests einmal solo und in Folgegruppe starten,
Host/Gast als Anfuehrer tauschen, Attacken/Ziele/Wechsel und Teamgrenzen pruefen,
Sieg/Niederlage und direkte Wiederholung sowie eigene Teams nach Save/Rejoin.
Die bekannten EP-/Preisgeldluecken in gemeinsamen Kaempfen sowie Gras#1 und
Zuschauer#2 bleiben offen. Emulatorabnahme wird nicht behauptet.


Technik:0099-route101-repeatable-double-tests.patch, sieben Engine-/Datenfiles,
drei eigene TrainerIDs855..857 innerhalb vorhandener Flagkapazitaet864.
Nur die zugehoerigen Testflags werden fuer Wiederholung geloescht; kein echtes
Storyszenario wird zurueckgesetzt. Debugroster beeinflussen die768 abgeleiteten
normalen Trainerergaenzungen nicht. Neue sichere RAM-Rueckkehr restauriert
das komplette eigene Team vor Ergebnis/WhiteOut. Originale Storypartner
und Skripte bleiben unveraendert.

Zusaetzlicher Buildfix:gen_missing_map_includes.py erneuert vorhandene
Header/Events/Connections bei neuerem MapJSON/Layout/Compiler; full.mk nimmt
den Generator selbst als Dependency auf. So werden die zwei NPCs auch in
inkrementellen Builds tatsaechlich erzeugt. Mit echtem mapjson wurden
frisch/unveraendert/editiert/fehlend geprueft; gebaute Route101-Events haben
acht Objekte einschliesslich LocalIDs7/8 und korrekter Position/Hoehe.

Belege unter build/coop-test-battles/:verify-coop-final.log, release-final.log,
native-built-final.log, patch-chain-v3.log, build-hashes.json, audit.log.
Alle100Patches frisch angewandt;0099 vorwaerts/rueckwaerts bytegleich mit der
jeweiligen Basis. Neue Testskripte und sieben native/Datenaenderungen stimmen
mit der Patchkette ueberein (bestehende lokalisierte Dialoge ausgenommen).

Die folgende Sektion dokumentiert den vorausgehenden Folgegruppen-Build;
dessen Spielregeln gelten weiter, seine Buildkennung ist historisch.

## Sprint 6: Aktive Folgegruppe statt automatischem Beitritt (2026-11-14)

Aktueller lokaler Build: **0133a0e39c629df1**, Boot **E3ID-865a9244741eeebf**,
ABI **97c8f051**, Protokoll **20**. Beide Instanzen muessen den neuen Build
laden: Fenster schliessen und Paseo **Host und Gast starten** manuell starten.
F6 installiert keine neue Version. Emulatoren, Profile und SAV-Dateien wurden
von dieser Implementierung nicht angefasst; manuelle Abnahme ist offen.

Neue verbindliche Spielregeln:

- A auf einen freien Mitspieler: dessen Folgegruppe beitreten und automatisch
  hinterherlaufen. B beendet das Folgen in der Oberwelt, auch waehrend einer
  ausstehenden Kampfanfrage. Kurven werden ueber die empfangenen Wegpunkte
  verfolgt; Kartenwechsel verwenden gepruefte native Kartenuebergaenge.
- Naehe allein erzeugt keinen gemeinsamen Kampf. Nur ein aktiver Anfuehrer
  mit verfuegbarem Folgespieler bietet einen gemeinsamen Kampf an. Der
  Folgespieler loest beim Nachlaufen keine eigenen Trainer-/Graskaempfe aus.
  Menue/PC/Skriptbelegung und verlorene Gruppenberechtigung erlauben keinen
  erzwungenen Eintritt. Ohne Teilnahme bleibt der originale Solokampf.
- Beide verwenden ihre ersten drei Pokemon; eigene Reserveplaetze vier bis
  sechs werden weder eingesetzt noch ueberschrieben. Nach Erschoepfung eines
  Dreierteams kaempft der andere Spieler weiter. Verloren ist erst, wenn beide
  Dreierteams besiegt sind. Gemeinsames Geld wird bei Niederlage einmal belastet.
- Ein normaler Einzel-Trainerkampf wird als Gruppenkampf zum Doppelkampf.
  Originale Gegner bleiben; zusaetzliche verschiedene Arten passen zu
  Traineridentitaet/-klasse/Typen und Level. Ziel min(6,2*Originalanzahl), bei
  zu wenig passenden Arten weniger; niemals identische Originale klonen, um
  Plaetze zu fuellen. 768 Einzel-Trainerkonfigurationen sind abgedeckt.
- Bestehende Doppel-/Story-Gegnerteams bleiben erhalten. Bei Story mit
  NPC-Partner (Troy) steuert der Folgespieler dessen vorhandene drei Pokemon;
  seine eigenen sechs bleiben unveraendert, auch bei Niederlage. Auch die
  native Storyauswahl des Anfuehrers ist auf seine ersten drei begrenzt.
- Zustimmung traegt den exakten aktuellen Teamhash. Der Aufbau wartet auf
  dessen vollstaendige Uebertragung; alte Join-/PC-Caches werden nicht geladen.
  Ein nach Zustimmung geoeffnetes Menue oder veraendertes Team bricht den
  Beitritt sauber ab. Host-/Gast-Anfragen haben getrennte Tokenbereiche.

Technik und Nachweise:

- Neues lokales Folgemodul src/3ds_coop_follow.inc, authentifiziertes Paket85.
  Doppelkampf-Anfrage8B, Kontrolle10B (Gruppentoken+Teamhash), Version20 auch
  fuer das Inventar. Alte Protokolle koennen nicht beitreten.
- 0098-coop-follow-battles.patch: native Eingabe, Traineraufbau, erste lebende
  Startpokemon, Drei-/Sechser-Ersatzpools, Storyauswahl, getrennte Rueckkehr,
  einmalige Geldstrafe und RAM-Backup fuer Gaeste. SavePlayerParty ist eine
  RAM-Kopie; echte Gast-Flash-/SAV-Schreiber bleiben gesperrt.
- Alle99Patches auf frischer isolierter Basis anwendbar;19 betroffene native
  Funktionskoerper stimmen mit den gebauten Quellen ueberein.
- verify-coop einschliesslich neuer Folge-/Storytests gruen. Echte native
  Verlust-/Wechsel-/Teamauswahl-/Kartenkoerper geprueft;519 Rueckkehrchecks.
  ARM-Release und PAK erfolgreich. Belege unter build/coop-follow/:
  verify-story-final.log, release-story-final.log, patch-chain-final.log,
  native-tests-final.log, return-tests-final.log, build-hashes.json.

**Sprint 6 bleibt in Arbeit:** EP/EV/Levelup/Lernen und regulaeres
Trainerpreisgeld sind weiterhin die bekannte native-Link-Luecke; nicht als
behoben oder manuell bestanden werten. Frontier/Geheimbasis/eReader/Safari
bleiben originale Solokontexte; besondere Wildkaempfe ohne geeignete lokale
Zweittabelle ebenfalls. Fang/Flucht, Rad/Surfen und Gruppen-Disconnects sowie
Emulator-/Hardwaredarstellung brauchen weitere Abnahme. Die Folgebewegung
nutzt native Kollisionen; keine Zusage fuer beliebige Hindernisse/Feld-HM-Pfade.
Gras #1 und Zuschauer-Ruckeln #2 bleiben getrennt offen und unveraendert.

## Sprint6 integriert und ueberarbeitet (2026-11-14)

Auf ausdruecklichen Nutzerauftrag wurde zuerst `upstream/main` von
ZallaxDev/emerald-3ds neu abgerufen. Stand bleibt `bcae4b7` (2026-10-30),
bereits Vorfahr unseres Main; keine neueren fehlenden Commits zu uebernehmen.
Der vorherige gepruefte Koop-Stand wurde als `612a28d` auf origin/main gepusht.
Die zwei Restfehler bleiben getrennt offen: [Gras #1](https://github.com/Sumaiter/emerald-3ds/issues/1)
und [Zuschauen #2](https://github.com/Sumaiter/emerald-3ds/issues/2).

Danach Sprint6 (`ea3d332`) mit Main integriert. Konflikte betreffen Header,
Protokoll und Backend; die neueren Story-/Inventory-/Wallet-/HP-/Disconnect-
Funktionen bleiben erhalten. **Protokoll19**, Ergebnisheader20B, separate
Doppelkampf-Pakete40..46 und unveraenderte getrennte Bereiche60..84.

Direkt korrigiert und regressionsgeprueft:

- Partner-Ablehnung faellt einmalig auf den lokalen Doppelkampf zurueck,
  ohne sofort eine neue Einladung zu senden (vorher XFAIL, jetzt regulaerer Test).
- Gast/Host erhalten ihre veraenderten ersten drei Pokemon in den richtigen
  Originalplaetzen; die restlichen drei bleiben erhalten. Restore ist bei
  aufgeschobener Ergebnisuebertragung idempotent und laeuft vor WhiteOut.
- Wiederholtes CB2_InitBattle setzt keine aktive Baseline zurueck.
- Kein Callbackpointer im Setup-Paket; Initiator behaelt den eigenen Callback,
  eingeladener Spieler bekommt eigene Feldrueckkehr ohne fremde Skriptfortsetzung.
- Bereits akzeptierte Einladungen koennen wiederholt werden; andere Menues,
  laufende Skripte/Kaempfe, PC-/Transferbelegung verhindern neue Annahme.
- Live-Inventar und Wallet erlauben den eigenen Koop-Doppelkampf, waehrend
  native Link-/Tutorial-/Recorded-Kontexte weiter ausgenommen bleiben.
- Alte mehrfach gestueckelte0083-Diffs normalisiert (keine zusaetzliche
  Funktion): sonst scheiterte frisches git apply mit pokemon_storage_system.c
  "wrong type". Jetzt alle98Patches in Bootstrap-Reihenfolge anwendbar.

Gemeinsames verify-coop gruen, Sitzungssuite mit0erwarteten Fehlern, neue
echte native Double-Return-/Link-Setup-Bodytests gruen, ARM-Release/PAK gruen.
98Patches auf isolierter Originalbasis angewendet; neun beruehrte native
Funktionskoerper stimmen mit dem gebauten Baum ueberein. Lokalisierte Daten
werden dabei nicht mit unveraenderten englischen Daten gleichgesetzt.
307Portinputs bytegleich, Release-Audit einschliesslich neuer Dateien gruen.

Build **225ca2caa5a7881b**, Boot **E3ID-5790a58262fe8f67**, ABI **73bbc4ee**.
Belege build/coop-nextwave/sprint6-{final-tests,release,patch-chain}.log,
sprint6-build-hashes.json, sprint6-patch-live-equivalence.log,
sprint6-source-stage-final.log, sprint6-audit.log. Artefakte weiter unter
build/upstream/3ds_port/dist und build/upstream/3ds_port/build/emerald3ds.pak.
Keine Emulator-/SAV-/Profilinstallation, keine Hardwareabnahme.

**Sprint6 bleibt IN ARBEIT:** Zuege/Wechsel/Ende beider Rollen im Emulator
noch nicht abgenommen. Quellreview zeigt ausserdem eine klare Restluecke:
Cmd_getexp schliesst BATTLE_TYPE_LINK aus; HandleEndTurn_BattleWon waehlt
den Link-Abschluss statt des lokalen Trainerabschlusses mit Preisgeld.
Der bisherige Sprint6-Code verwendet diese Flags fuer den Koop-Transport.
Native EP-/Levelup-/EV-/Lern- und Preisgeldpfade muessen gezielt fuer Koop
erweitert werden, mit eindeutiger Spieler-/Hostzustaendigkeit. Nicht nur das
LINK-Bit pauschal loeschen: Master/Player/LinkPartner-Controller haben
unterschiedliche Aufgaben, Partner-ExpUpdate ist nativ ein No-op.
Gruene Infrastrukturtests beweisen diese noch fehlende Spiellogik nicht.

## Auftrag: Issues sichern, GitHub aktualisieren, dann Sprint6 (2026-11-14)

Die zwei Restfehler sind auf Nutzerwunsch getrennte offene GitHub-Issues:

- [#1 Gras beim entfernten Spieler](https://github.com/Sumaiter/emerald-3ds/issues/1)
- [#2 Zuschauer-Ruckeln](https://github.com/Sumaiter/emerald-3ds/issues/2)

Weitere Arbeit daran wurde zugunsten von Sprint6 zurueckgestellt. Astra hat
seit letzter Quellenfreigabe keine Dateien geaendert und ist gestoppt.
Belegt ist vermeidbare Vollpruefsumme vor dem statischen Vergleich im
Spectator-Capture; noch kein kausaler ARMnachweis. Gras-Restursache weiterhin
nicht gesichert. Keine unfertige Optimierung enthalten. Collector sauber
beendet; finale read-only Logs liegen in der oben beschriebenen Belegsession.
Nutzerauftrag: aktuellen geprueften Stand pushen, vor Sprint6 aktuelle Merges
von ZallaxDev/emerald-3ds:main pruefen/uebernehmen, danach Sprint6 mergen und
direkt erkennbare Probleme ueberarbeiten. Emulatoren/SAVs bleiben beim Nutzer.

## Erneuter Emulatorbefund: beide Nachtests offen (2026-11-14)

Nutzer meldet beim neuen Build weiterhin keinerlei Graswackeln auf dem Geraet
des nicht laufenden Spielers. Zuschauen ist verbessert, aber noch laggy.
Am Gast selbst ist Ruckeln schwer erkennbar, wenn dann nur wenig. Beide
Nachtests gelten damit als NICHT bestanden. Alle anderen zuvor bestaetigten
Standardtests bleiben anerkannt. Die folgenden Hintergrundpruefungen des
vorherigen Fixstands ersetzen diese konkrete Laufzeitabnahme nicht.

Aktuell Gast im Wildkampf, Host schaut zu; Root hat beide Fenster ohne Eingaben
beobachtet (Waumpel Lv3, Hydropi Lv5 15/20HP, passende statische Topbilder).
Keine Animationslatenz aus diesen Einzelbildern ableiten. Beide laufenden Logs
bestaetigen E3ID-6a834575fbbbc71c / ABI bd6f7986, also den neuen Protokoll18-Build.
Read-only Collector: build/coop-observation/20261114-171821Z; Baselinezeiten
sind Erfassungszeit, keine urspruengliche Ereigniszeit. Gast zeigt wiederholt
PROF slow mit vbl12.35..12.8ms und DROP33.4ms; noch keine gesicherte Ursache.

Derselbe einzige Astra-Agent 5be417bc-1c7c-48f5-b124-66454b74a3e4 bleibt
auto-review und untersucht/optimiert seine Zuschauerdateien. Zusaetzlich
prueft er Gras nur lesend; Root/Sol implementiert danach den Grasfix.
Keine weiteren Agenten, Emulator-/SAV-/Profil-/Installationsaenderungen.
Auftrag lokal build/coop-nextwave/runtime-followup-task.txt.

## Neue Planung: Sprint 9 (2026-11-14)

Auf Nutzerwunsch in `Status.md` als offen nach Sprint8 ergaenzt:
vollstaendiges Solospiel auf einem Geraet, Solo-Alternativen fuer
Tauschentwicklungen (auch mit Item) und regulaere Ingame-Wege fuer fruehere
Event-Pokemon samt notwendigem Zugang. Weitere externe Erwerbshuerden werden
systematisch erfasst. Aufgaben und Abnahmekriterien stehen im eigenen
Sprint9-Abschnitt. Nur Planung; kein neuer Branch und noch keine Umsetzung.
Die Gras-/Zuschauer-Fixwelle ist unten mit Build und ausstehendem Nachtest dokumentiert.

## Fix-Welle Gras und lokale Zuschaueransicht: gebaut (2026-11-14)

Beide Aenderungen sind implementiert und gemeinsam im Hintergrund geprueft.
Release und PAK sind gebaut: **Protokoll18**, Build **d3d091c44f788f8e**,
Boot **E3ID-6a834575fbbbc71c**, ABI **bd6f7986**. Dieser Stand ersetzt den unten
historisch dokumentierten Protokoll17-Build. Noch keine Emulatorabnahme dieser
beiden Aenderungen; Emulatoren, SAVs und Profile blieben unveraendert.

Der Nutzer bestaetigt alle bisherigen Standardtests einschliesslich Host-F6/
Gast-Hauptmenue, NPC-Marktfuehrung und B-Beschriftung als bestanden. Sie bleiben
anerkannt. Nach dieser Lieferung nur Gras und neue Zuschaueransicht gezielt
nachtesten; vorbereitete Sonderfaelle und einmalige feste Wildgegner bleiben
separat offen (LOKAL_TESTEN.md).

- **Gras (Root/Sol):** Zusammengefasste Remote-Positionskorrekturen landeten
  im nativen Spawnpfad, der die Grasanimation ans Ende setzt. Jetzt wird nur
  nach einer solchen Korrektur einmal der native Schritt-Grundeffekt benutzt.
  Normale Schritte bleiben nativ; Stillstand, Drehen und Heartbeats starten
  nichts erneut. Patch0096 ist im Enginebaum bereits angewendet. Echte
  Backend-/Engine-/TallGrass-/LongGrass-Tests gruen; gesicherte Ausgangsbasis
  reproduziert den Fehler. ARM und isolierter Patchroundtrip bestanden.
- **Zuschauen (einziger Astra-Subagent):** Aufgeloeste pointerfreie
  Darstellungszustaende statt fertiger RGB-Bilder, lokal mit Originalrenderer
  und Zwischenbildern. VRAM/OAM/Paletten/Scanlines plus HD-HUD und privater
  Voxel-Karten-/Kamerakontext; keine fremde Kampfentscheidung, Party-, Save-
  oder Taskausfuehrung. Unterschiedliche Zuschauer-/Quellkarten sind abgedeckt.
  Workerjoin/Jobabbruch und GPU-Fence sichern den Renderbesitzwechsel;
  B/Touch/Stop/Reset stellen die eigene Ansicht wieder her.
  **Kein vollstaendiges semantisches Ereignisprotokoll:** Unter Rueckstau
  koennen kurzlebige Quellzustaende zusammenfallen. Der native Belastungsfall
  mit grossem Tile-/Palettenwechsel und verlorenem Chunk benoetigt 704ms;
  ruckelfreie Wiedergabe jeder Attacke ist nicht nachgewiesen. Messungen mit
  lokalen Zwischenframes sind Hosttests, keine Emulator-/Hardware-FPS.

Astra `5be417bc-1c7c-48f5-b124-66454b74a3e4` lief sichtbar in Paseo als
gpt-6-astra/xhigh/auto-review; Root Sol/xhigh/auto-review uebernahm Gras,
Integration und Gesamtbuild. Quellenfreigabe vom Agenten ausdruecklich erhalten.
Keine weiteren Agenten. Neue Quellen 3ds_battle_present.c/.h sowie beide
3ds_battle_present_video.inc und 3ds_battle_present_assets.inc sind gestaged;
CPPFLAGS und GAMEFLAGS aktivieren CTR_BATTLE_PRESENTATION. Patch0097 (read-only
Fieldmap-Export) ist bereits angewendet. 0096/0097 nicht doppelt anwenden.

Bestanden: `make -C 3ds_port verify-coop -B -j8` inklusive neuer aktivierter
Present-/World-/Fence-/HD-/Patchtests und Grastest, ARM-Release/Link/PAK,
Quellenabgleich und Release-Audit einschliesslich unversionierter Quellen.
Belege: build/coop-nextwave/final-tests.log, final-release.log,
build-hashes.json, source-stage-check.log, audit.log. Detailberichte:
grass-integration.md und spectator-integration.md im selben Belegordner.

Artefakte: build/upstream/3ds_port/dist/Emerald3DS.3dsx und Emerald3DS.smdh,
dazu build/upstream/3ds_port/build/emerald3ds.pak. Keine Installation/Commits/
Pushes. Fuer den neuen Nutzertest beide Fenster schliessen und das Paseo-Script
"Host und Gast starten" manuell starten; F6 installiert keinen neuen Build.

## Vorherige Fix-Welle vom 2026-11-14 (historischer Arbeitsstand)

Aktuelle Abnahme und neue Beauftragung stehen im obersten Abschnitt.

Aktueller ausdruecklicher Auftrag: alle vier Befunde jetzt beheben und die
uebrigen Tests als vom Nutzer bestanden dokumentieren. Die vorherige
Zurueckstellung ist aufgehoben. Nur Hintergrundtests; Emulatorbedienung und
SAV-/Profilaenderungen bleiben beim Nutzer. Gesicherte read-only Portlogs
unter `build/coop-fixes-20261114/{host,guest}-before.log` bestaetigen den
getesteten Build 9ddd5d54d9fa7579 / E3ID-47028c29f2d44c12, ABI 9242b424.

Alle vier Korrekturen sind implementiert und im Hintergrund geprueft;
die beteiligten sichtbaren Paseo-Agenten nutzten auto-review. Der eigene Emulator-Nachtest dieser vier
Faelle bleibt beim Nutzer; die bisherigen Standardtests bleiben anerkannt.

1. **Urspruenglicher Fehler: Host F6 / Gast-Hauptmenue:** Drueckt der Nutzer F6 beim
   Host, friert der Gast ein und erreicht das Hauptmenue nicht. Screenshot:
   Oberwelt oben stehengeblieben, unten schwarz mit Versions-/Credittext.
   Ursache: longjmp aus dem VBlank umgeht das Freigeben von sWaiting in
   CtrPlatform_EndFrame. Der Post-Stackreset setzt diese Framesperre und die
   Zeit-/Pacingdaten vor dem neuen Boot zurueck. Ein echter EndFrame-Test
   prueft den Sprung und vier Folgeframes; ein unabhaengiger Test verfolgt
   die nativen Copyright-/Save-/Menuecallbacks und den Taskscheduler.
2. **Zuschauen ruckelt stark:** Nutzer verlangt Optimierung der sichtbaren
   Uebertragung. Codec-/Queue-Tests gelten nicht als Performanceabnahme im
   Emulator. Die bisherige Vier-Chunk-Abfrage wartete alle 512 Bytes auf
   eine weitere Runde; hinzu kamen 100 ms nach dem Bild-ACK. Neu sind
   XOR/LZ-Aenderungen, 146-Byte-Chunks, ein gleitendes 32-Chunk-Fenster und
   maximal acht Low-Priority-Pakete je 16 ms. Unveraenderte Bilder werden
   nicht erneut uebertragen. Die 32 reservierten Gameplay-Queueplaetze bleiben.
   Im simulierten lokalen Link erreicht die native Assetfixture 7,625 statt
   1,75 geaenderte Bilder/s und ein Erstbild nach 160 statt 672 ms.
   Das sind kontrollierte Hintergrundmessungen, keine Emulator-/Hardwarewerte.
3. **Zuschauer-Text fehlerhaft:** Touch-Schaltflaeche zeigt
   `Zuschauen verlassen ?B?`. Die Fragezeichen um B muessen entfernt werden.
   Neu: `B: Zuschauen verlassen`. Die tatsaechliche Locale-Konvertierung und
   der echte UI-Zweig pruefen die Darstellung ohne Fragezeichen.
4. **Marktfuehrer bewegt sich beim Host nicht mit:** Spricht der Gast den
   Verkaeufer/Marktfuehrer im zweiten Ort an, laeuft dieser beim Gast zum
   Pokemon-Markt, bleibt beim Host aber am vorherigen Ort. Screenshot zeigt
   unterschiedliche NPC-Positionen waehrend derselben Fuehrung. Erwartet ist
   die sichtbare NPC-Bewegung auf beiden Seiten. Der alte Snapshotpfad war
   Host->Gast; permanente Objektupdates erfassten keine laufende Fuehrung.
   Neu uebertragen authentifizierte Pakete 76/77 die ereignisgebundenen
   Akteursposen und ein geordnetes Ende. Die Gegenseite bewegt nur den NPC
   mit nativen Bewegungsschritten und fuehrt kein zweites Skript aus.
   Der Ereignisabschluss wartet bei Backpressure; bereits empfangene
   Bewegungsschritte werden vor Autosave und neuem Join-Snapshot beendet.

Die Zuschauer-Alternative ueber Ereignisse wurde am vorhandenen Code geprueft:
Battlecontroller aendern neben der Anzeige auch Party, Battle-TV und
Animationsglobale. Ein reiner Praesentationskontext mit eigener Animation und
2D/3D-Renderzustand waere ein eigener Ausbau. Dieser Fix nutzt komprimierte
Bildunterschiede und behaelt den vollstaendig gerenderten 400x240-Bildschirm.

PC-Sperre wurde zuvor vom Nutzer als erledigt gemeldet, Wildkampf-XP als gut
beobachtet (keine extern geprueften Zahlen). Figurengalerie wurde akzeptiert.
Neue Nutzerrueckmeldung vom 2026-11-14: "der rest hat alles funktioniert".
Die uebrigen Punkte der zuletzt ausgegebenen Sprint-5-Testliste gelten damit
als vom Nutzer erfolgreich gemeldet; keine eigenen Emulatorpruefungen oder
Messungen daraus ableiten. Die Emulator-Nachabnahme der vier obigen
Korrekturen bleibt offen und erfolgt mit dem neuen Build. Testliste und Grenzfallabgrenzung unter
`LOKAL_TESTEN.md`. Sprint 5 ist damit noch nicht vollstaendig abgenommen.

## Neuer Testbuild nach den vier Korrekturen

Protokoll **17**, Build **11d899b9ae0f4952**, Bootkennung **E3ID-dc155c4f365fa25e**,
ABI **04c2f1eb**. Beide Spieler brauchen den passenden neuen Build samt PAK.
Release und Datenpack sind gebaut; alle Port-Eingaben wurden bytegleich in
den Buildarbeitsbaum uebernommen. Belege unter `build/coop-fixes-20261114/`:
`final-tests.log`, `final-release.log`, `source-stage-check.log`,
`build-hashes.json` und `audit.log`. Es wurden keine Emulatoren bedient,
Profile installiert oder SAVs geaendert.

Der komplette Hintergrundlauf `make -C 3ds_port verify-coop -B -j8` ist gruen,
einschliesslich realem Frame-/Menue-Reset, NPC-Zweiendpunkt-/Engine- und
Speicherbarrieretests sowie Zuschauer-Codec, Queuebudget, Exit, GPU-Fence,
Schriftkonvertierung und Lastbenchmark. Die unabhaengige Nachpruefung steht in
`reset-review.md`, Implementierungsbelege in `npc-integration.md` und
`spectator-integration.md` im selben Belegordner. Patch 0095 ist im
Buildarbeitsbaum bereits angewendet; nicht erneut anwenden.

Nur die vier obigen Fehler gezielt mit dem neuen Build nachtesten. Das
Emulatorergebnis bleibt offen; Standardtests wurden vom Nutzer bestanden
gemeldet. Volle Boxen, Sonderkampf-Rueckkehr und die separat dokumentierte
Reservation einmaliger fester Wildgegner werden dadurch nicht abgenommen.

## Historisch: Live-Koop und Hostverlust (2026-11-13)

Protokoll **16**. Beide Spieler brauchen denselben neuen Build. Die folgenden
aelteren Buildkennungen gehoeren zu den dort bezeichneten historischen Abnahmen.

Finaler Release-/PAK-Build: **9ddd5d54d9fa7579**, Bootkennung
**E3ID-47028c29f2d44c12**, ABI **9242b424**. Build und Datenpack erfolgreich;
6388 Assets/1339 Pointerzeilen sowie Script-/Songrelokationen geprueft.
Alle 284 Port-Eingaben stimmen bytegleich mit den Buildquellen ueberein.
Belege: `build/coop-live/final-release.log`, `source-stage-check.log`,
`build-hashes.json` (SHA256 aller drei Startdateien), `root-patch.log`.

- Beutel und PC-Items werden ueber einen gemeinsamen Hostledger sofort
  uebernommen, auch waehrend unabhaengiger normaler Kaempfe. Verbrauch wird
  vor der Wirkung reserviert; Transfers, Helditemtausch und Rueckgaben sind
  atomar. Eine ungeklaerte Antwort wartet vor der Wirkung auf die Entscheidung.
- Geld und Muenzen verwenden denselben Transaktionskern mit eigenem Ledger.
  Kaeufe, Services, Wetten, Gewinne und Verluste sind gemeinsam. Livebuchungen
  werden am Kampfende kein zweites Mal angewendet.
- A auf den kaempfenden Spieler oder dessen aktuellen NPC-Gegner startet die
  Zuschaueransicht. B oder die Touch-Schaltflaeche verlaesst sie; eine gehaltene
  Richtung kann danach sofort weiterlaufen. Der obere Bildschirm uebertraegt
  das fertig gerenderte Kampfbild mit begrenztem Netzwerkbudget.
- Abgeschlossene Szenarien gleichen Hideflags, aktive NPCs und permanente
  Objektmutationen ab, bevor die Ereignissperre freigegeben wird. Alle sechs
  Route-103-Rivalvarianten sind geprueft. Kartenlokale TEMP-Flags und -Variablen
  bleiben getrennt; NPC- und Skripteinstieg pruefen inzwischen versteckte Objekte.
- Die Gast-Teamreihe im Koop-PC behaelt HP, Status und Mail; der Boxpayload
  behaelt AP, Attacken und weitere Daten. Pruefsummen binden die Vitalwerte an
  das konkrete Pokemon. Normale Boxkonvertierungen bleiben nativ.
- Verlaesst der Gast die Sitzung, behaelt der Host den zuletzt vollstaendig
  empfangenen Zustand im RAM und speichert automatisch im naechsten sicheren,
  entsperrten Oberweltzustand. Fehlgeschlagenes Speichern wird erneut versucht.
- Bei Hostverlust kehrt der Gast automatisch ins Hauptmenue zurueck. Der
  gesamte alte Spiel-/Item-/Geldaufrufstapel wird vor dem RAMreset verworfen;
  es gibt keine Fortsetzung der Gastwelt. Der lokale Spielstand wird beim Boot
  neu geladen. Eine neue Teilnahme authentifiziert sich frisch und uebernimmt
  den aktuellen Hostzustand. Stiller Transportverlust wird nach 6000 ms erkannt.
- Ein ausdruecklicher voller Reset verwirft nach dem Stackabbruch beide lokalen
  Ledger, damit sie keinen anderen Spielstand ueberschreiben. Beim gewoehnlichen
  Gastverlust behaelt der weiterlaufende Host seine Entscheidungen und Abbuchungen.

Hintergrundabnahme: `build/coop-live/final-tests.log` ist gruen, null erwartete
Fehler. Echte Enginekoerper, zwei Endpunkte, verschluesselte Beutel und volle
Taschen, konkurrierende letzte Items/Kaeufe, 48 Gast-Kontext/Disconnectkombinationen,
native Boot-/Hauptmenue-Reihenfolge, 6000-ms-Transporttimeout, Zuschauer-/GPU-Pfade
und die bestehenden Sprint-5-Regressionen sind Teil von `verify-coop`.
Berichte: `build/coop-live/{inventory-integration,inventory-bridge-review,
money-integration,story-integration,spectator-integration,live-integration-review}.md`.
Hostverlust/Reset: `build/coop-live/guest-disconnect-menu.md` und
`guest-disconnect-validation.log` einschliesslich vier erkannter Mutationen.
Patch 0090/0092/0093/0094 sind im Buildbaum bereits angewendet.

Emulator-/Hardwareabnahme bleibt beim Nutzer. Gezielte Nachtests: beide Rollen
beim letzten Item und gleichzeitigen Kauf; A/B/Touch beim Zuschauen; Maike-Sieg
und verschwundener NPC beim anderen Spieler; verletzte/kampfunfaehige Gastreihe
nach Disconnect, Autosave und Rejoin; Hostverlust in Oberwelt/Kampf/PC/Beutel.
Ein Prozesscrash genau zwischen Abbuchung und Effektuebernahme ist ohne
persistent gespeichertes Transaktionsjournal weiterhin keine atomare
Welt-/Party-Saveoperation; bestaetigte normale Buchungen werden dedupliziert.

Stand: 2026-11-13. Der zuvor lokale Stand nach main fdc4b48 wird fuer die
Branch-Aktualisierung versioniert: Sprint 5, alle 99 Figuren und reparierte
Hintergrundtests. Die folgenden aelteren Abnahmen dokumentieren ihre jeweiligen
Builds; fuer den aktuellen Teststand gelten die ersten Abschnitte.

## Reparatur der sieben Reviewbefunde (2026-11-13)

Nutzer hat alle sieben Reparaturen ausdruecklich beauftragt. Neue Version des
Koop-Protokolls: **15**. Beide Spieler muessen denselben neuen Build benutzen.
Die bisherigen roten Probes unter build/coop-review bleiben historische
Vorher-Evidenz; die dauerhaften Regressionen sind jetzt Teil von verify-coop.

- BATTLE_BEGIN fuehrt die Ergebnisnummer mit; alte bereits angewendete Anfaenge
  werden passend bestaetigt, ohne den Empfang erneut zu oeffnen. BATTLE_READY
  bestaetigt ausschliesslich die passende Ergebnisnummer und Pruefsumme.
- Sondertrainer warten nicht mehr auf gewoehnliche Trainerflags. Normale
  Trainer behalten die Flag-/Story-Freigabe. Aufzeichnungen und Wallys
  Fangtutorial erzeugen keine Spieler-Kampftransaktion.
- Patch 0089 schliesst Nicht-WhiteOut-Rueckkehrwege und erfasst Resultate erst
  nach nativer Item-/Teamwiederherstellung. Temporaere Frontier-/Partnerteams
  duerfen das gespeicherte volle Team nicht ersetzen. Normaler WhiteOut bleibt
  nach Geldverlust/Heilung; SoftReset raeumt auch aktive Kampftransaktionen auf.
- Ein vollstaendig empfangener Fang bei vollen Boxen bleibt im RAM erhalten.
  PC-Wiederherstellungsanfrage 64 bindet sich an das wartende Ergebnis. Der PC
  bleibt gegen aktive Kaempfe und unvollstaendige Transfers gesperrt. Nach
  Schliessen wird der Fang vor dem PC-Snapshot uebernommen, damit beide Seiten
  denselben Boxinhalt erhalten. Ein neuerer PC-Snapshot hat Vorrang vor dem
  wartenden Kampfresultat: Verschieben, Bearbeiten, Freilassen und Entnehmen
  eines Fangs erzeugen keine Duplikate oder Wiederherstellungen. Dieser Vorrang
  bleibt auch bei erneutem Transfer nach fehlgeschlagener Inventaruebernahme
  erhalten. Zwoelf Endpunktfaelle (beide PC-Besitzer mit je sechs Aktionen)
  pruefen Boxen, Team, Geld, Items und die abschliessenden Bestaetigungen.
- Patch 0088 schuetzt weitere vom lokalen Skript bewegte/entfernte/gedrehte
  Szenenakteure gegen Host-Snapshots. Pending/Prepared-Trainer behalten ihre
  zur Annaeherung erfasste Position/Richtung. Ablehnung/Timeout/Reset geben
  diesen Schutz frei, unabhaengige NPCs synchronisieren weiter.

Neue Tests: coop_battle_regressions.inc (in Session-Suite),
coop_scene_regression_test.c, test_coop_scene_hooks.py,
test_coop_battle_return_hooks.py; Codec-/Runtime-Fixtures erweitert.
Gegenpruefungen ueber sichtbare Paseo-Agenten mit auto-review.
Abnahme: finales verify-coop gruen, null erwartete Fehler. Echte
Kampf-Rueckkehr/Teamrestore/WhiteOut-Funktionskoerper: 517 Checks; Szenen-/
Annaeherungssuite: null Fehler. Beide PC-Erholungsrollen mit zwei vollstaendig
simulierten Endpunkten bestanden. ARM-Syntax fuer Backend/Codec und alle zehn
geanderten Enginequellen bestanden; 0087/0088/0089 Reverse-Checks bestanden.
Release-Audit inklusive unversionierter neuer Dateien: 1244 Dateien, null Befunde.
Belege: build/coop-review/final-tests.log, fix-arm.log, fix-patches.log,
fix-audit.log. Sonderkampf-Mutationsnachweise unter build/coop-finding4-validation:
Baseline scheitert; vorzeitiger WhiteOut-Abschluss und fehlende Unterdrueckung
eines temporaeren Teamladens werden von den neuen Tests erkannt.
Finaler Release-/PAK-Build erfolgreich: **8c6a3526999fafca**, Bootkennung
**E3ID-6c073b4f16e97129**, ABI **a48e1627**, Protokoll **15**.
Belege: build/coop-review/final-release.log und build-hashes.json mit SHA256
aller drei Startdateien. Alle 15 geaenderten Portdateien stimmen bytegleich
mit den Release-Eingaben ueberein (source-stage-check.log).
Unabhaengiges Abschlussreview: keine offenen Restbefunde im geprueften Umfang;
Sitzungs-/Codec-Suite und separate Snapshot-/Inventar-Retry-Probe bestanden.
Emulator-Nachtests bleiben beim Nutzer. Keine laufenden Emulatoren, Profile
oder SAVs wurden fuer diese Reparatur geaendert; kein Commit/Push.

## Historisch: Hintergrundreview mit sieben offenen Faellen (2026-11-13)

Nutzer bat nach der neuen Sperre um weitere Fehlerpruefung, ohne Emulatoren.
Fuenf gezielte neue Probes plus zwei unabhaengige Reviewer-Probes schlagen
reproduzierbar fehl; die bisherige verify-coop-Suite ist weiterhin gruen.
Produktionscode und letzter Runtimebuild wurden bei diesem Review nicht geaendert.

Offen: wiederholtes altes BATTLE_BEGIN oeffnet abgeschlossenen Transfer erneut
und blockiert neuen; BATTLE_READY ist nicht an konkrete Ergebnisnummer/Checksum
gebunden; Sondertrainer-Sieg kann auf nie gesetztes gewoehnliches Trainerflag
warten; Niederlage ohne WhiteOut (z.B. Kampfturm/Geheimbasis) laesst battleActive
wahr; volles Boxlager + fertiger Gastfang verweigert gerade den fuer Platz
benoetigten PC; Grunt-Szene im RusturfTunnel schuetzt Peeko/Briney-Bewegungen
nicht gegen Host-Snapshots; Trainer-Approach kann nach Wartezeit neue Richtung
mit alter Entfernung kombinieren (Route102 Tiana).

Details, genaue Ausloeser/Codepfade und Fixansaetze: build/coop-review/RESULT.md.
Repros/Logs: main-probes.c/.log (fuenf rote Sicherheitsassertions),
native-assumptions.py/.log (echte Engine-Rueckkehrzweige bestaetigt),
reviewer/probes.c/.log (zwei rote Sicherheitsassertions), baseline-suite.log.
Keine Failures als Abnahme markieren. Der Transport selbst ist zuverlaessig/
geordnet; der alte-BEGIN-Fehler betrifft Wiederholungen des Anwendungsprotokolls.
Sichtbarer Paseo-Pruefer dfd46672-9030-4926-9b55-a5fe49ea1e03, high/auto-review,
keine gemeinsamen Codeaenderungen. Emulatorreproduktion und Reparaturen offen.

## Ereignissperre vor Gespraech und Kampfeinstieg (2026-11-13)

Neuer Nutzerbefund: Gast begann den Rivalenkampf zuerst, danach schwarzer
Host-Bildschirm auf Route 103. Sperre ausdruecklich fuer alle Kaempfe verlangt.
Patch 0087 beansprucht Feldereignisse vor dem ersten Bytecodebefehl: NPCs
ueber Karte+lokale Objekt-ID statt starter-/geschlechtsabhaengiger Trainer-ID;
automatische Szenen als Kartensperre, damit man denselben Ablauf nicht ueber
einen anderen Einstieg/NPC starten kann. Zwei annaehernde Trainer werden
atomar beansprucht. Wanderndes legendaeres Pokemon hat eine globale Identitaet.
Unabhaengige Zufallskaempfe sowie verschiedene einzelne Trainer bleiben moeglich.

Der Host entscheidet verbindlich, bevor der Gast lock/msgbox/bewegen/Kampf
starten darf. Eine Ablehnung beendet den ungestarteten Kontext und raeumt
lediglich inaktive Trainer-Approach-Tasks auf; die untere Anzeige nennt den
anderen Spieler. Bestaetigen startet niemals das abgelehnte Skript. Besitz
bleibt ueber waitstate/Kampf/Abschlussdialog bestehen; Freigabe erst nach
Ergebnisbestaetigung und Versand der Storyaenderungen. Anfragen/Freigaben
werden wiederholt; Token und Objektvergleich verwerfen spaete/fremde Antworten.
Bei Verbindungsverlust startet ein noch nicht freigegebenes Skript nicht.
Host-NPC-Snapshots ueberschreiben waehrenddessen nicht die eigenen NPC-Bewegungen
des Gast-Skripts. Normale Kampfreservierung jetzt zusaetzlich vor der Blende.

Protokoll 14, neue Pakettypen 60..63 (22 Byte); 40..50 bleiben fuer Sprint 6/7
frei. Beide Spieler brauchen den neuen Build. Bei spaeteren Sprint-Merges die
Versionsnummer fuer den kombinierten Protokollumfang neu festlegen, nicht die
alten 12/13 uebernehmen. Eigentlicher Koop-Doppelkampf ist auf main noch nicht
integriert: der eingeladene Partner startet keinen zweiten NPC-Kontext, LINK
bleibt vom Einzelkampf-Gate ausgenommen; Emulatorabnahme nach Sprint-6-Merge offen.

Hintergrundpruefungen: verify-coop ohne erwartete Fehler, neue Host/Gast-
Sitzungsregressionen und echte gestagte Script-/Kampf-/Approach-Funktionskoerper,
ARM-Syntax aller vier Enginequellen plus Backend, Patch-vorwaerts/rueckwaerts
gegen gesicherte Baseline, strikter Release-Audit. Belege build/encounter-lock/.
Release-/PAK-Build bestanden: `1dc8ce521196989d`, Log `E3ID-cdd3d1d6418e037c`,
ABI `7f6b9151`. Payload-Hashes unter build/encounter-lock/build-hashes.json.
Manuelle Emulatorabnahme offen; keine Emulatorbedienung, keine Installation
in laufende Profile und keine SAV-Aenderung.

## Gast bleibt nach Kampf in der sichtbaren Oberwelt stehen (2026-11-13)

Neuer Laufzeitbefund: Gast callback2=001c5890 entspricht im passenden
Build-Map CB2_InitBattle, obwohl noch das Feldbild angezeigt wird.
Getesteter Laufzeitbuild E3ID-8696897c762d96cd; beide Emulatoren laufen weiter.
Der Kampfeinstieg wartete auf vorherige Ergebnis-/Reservierungstransaktionen,
deren Fortschritt und Freigabe bisher andere Callbacks voraussetzten.
Ein neuer Hintergrundtest reproduzierte sechs fehlgeschlagene Assertions.

Korrektur: CtrCoop_BattleStart bedient auch im wartenden Kampfeinstieg den
Kampf-Tick. Der Tick gibt bestaetigte alte Reservierungen ebenfalls frei,
ohne eine weitere Oberwelt-Iteration vorauszusetzen. Neue Regressionen
pruefen fehlende ACKs/Retry, Freigabe vor neuer Anfrage, Start nach neuer
Freigabe sowie Empfang/ACK eines wartenden Fangs vor der naechsten Baseline.
Der vollstaendige verify-coop-Durchlauf besteht mit null erwarteten Fehlern;
ARM-Syntaxpruefung und Release-/PAK-Build bestanden. Neuer Build
`cf712051f18d29ac`, Logkennung `E3ID-b684c40eb8ecd1ec`, ABI `ebf0f1f4`.
Emulator-Nachtest bleibt beim Nutzer; noch keine laufenden Profile aktualisiert.

Nutzer hatte bereits am Host gespeichert. Vor weiteren Schritten wurden
beide SAVs/Profile/Logs unter build/coop-observation/blocked-guest-20261113-172849
gesichert. Die Host-SAV hat zwei vollstaendige, pruefsummengueltige Slots
(Zaehler 16 und 15), Gast ebenfalls (2 und 1). Das bestaetigt strukturelle
Lesbarkeit, nicht die fachliche Richtigkeit aller Storyvariablen.
Originalprofile und laufende Emulatoren bleiben unangetastet. Der neue
Build muss nach Schliessen beider Instanzen ueber das vorhandene Paseo-Script
installiert/gestartet werden; F6 allein installiert keine neuen Dateien.

Der damals separate Meike-/Trainer-Konflikt wird durch die spaetere
Ereignissperre oben adressiert; die manuelle Emulatorabnahme bleibt offen.
Dieser kleine Fortschrittsfix allein war keine vollstaendige Loesung der
Szenarioreservierung. Vollstaendige Beobachtungsuebergabe lokal unter
build/coop-observation/MAIN-TESTSTAND.md.

## Branch-Pflege

Nutzerauftrag: Nur main sowie sprint-6, sprint-7 und sprint-8 behalten; die drei
Sprint-Branches mit dem neuesten main aktualisieren. Die Funktionen aus Sprint
6 bis 8 bleiben in ihren Entwicklungsbranches; ihre Emulator-Abnahme ist offen.
Vorheriger Branchbestand inklusive Remote-Refs ist lokal unter
`build/branch-refresh/before-cleanup.bundle` gesichert. Das Bundle bleibt privat.
Die bereits dateiweise integrierte Sprint-5-Historie wird als Merge-Vorfahr
festgehalten, damit weitere Merges die alten Sprint-5-Aenderungen nicht erneut
als neue Implementierung behandeln. Die Hintergrundtests vor der Pflege bestehen.

## Hintergrundtests: Reparatur vom 2026-11-13

Nutzervorgabe: Emulator-Tests macht der Nutzer selbst. Nur noch Tests im
Hintergrund ausfuehren; keine Emulatorbedienung oder Testinstanzen starten.
Die zuletzt gestarteten isolierten BBHost-/BBGast-Prozesse und ihr Server
wurden beendet; alle acht Originalprofil-Dateien blieben laut SHA256 gleich.

Der komplette `make -C 3ds_port verify-coop` besteht jetzt auch nach einem
erzwungenen Neubau mit dem normalen MinGW-gcc, ohne besondere HOSTCC-Flags.
Beleg: `build/coop-tests-fixed-clean.log`. Alle acht C-Suiten, Join-Flow,
drei Grafik-Laufzeitpruefungen, 8+7+3 Packer-Tests und echte Fahrrad-Enginehooks
bestanden; Sitzungssuite mit null erwarteten Fehlern.

Startertest repariert: `scripts/prepare_starter_menu_test.py` selektiert
unveraenderte Starter-/Join-/Menuecallback-Funktionen aus der gestagten Engine.
Dadurch werden unter MinGW nicht laenger unbenutzte Hauptmenue-Grafiken und
Texte mitverlinkt. Renderer/Mainmenue-Rueckkehr bleiben Testprobes. Der native
Scheduler nutzt die feldbasierte Variante derselben task.c; die festen
32-Bit-ARM-Raw-Offsets sind auf dem 64-Bit-Testrechner nicht anwendbar.
Zusaetzliche Regressionen: abgelehnte Starterwahl und Retry, Callbackwechsel
ohne anschliessendes Rendering, Join-Task ausserhalb Slot 0, andere Tasks
bleiben bestehen, B-Abbruch nur ausserhalb des anfaenglichen PC-Zugriffs.
Der SetMainCallback2-Probe aktualisiert jetzt auch gMain.callback2 korrekt.

Fahrrad-Hooktest repariert: HOSTCC mit Optionen wird in Programm und Argumente
aufgeteilt, statt als einzelner ungueltiger Programmname gestartet zu werden.
Beide Varianten (normaler gcc und HOSTCC mit Optionen) sind erfolgreich geprueft.
Diese Reparaturen betreffen die Testinfrastruktur, nicht den Spielbuild.
Die manuelle Sprint-5-Abnahme bleibt offen.

## ZeroWalk und fertige HTML-Galerie

Nutzerfreigabe: Nach Sichtung der vollstaendigen Galerie wurden die Grafiken
aller 99 Figuren mit "ok die passen alle nice" visuell abgenommen. Keine
weiteren grafischen Korrekturen beauftragt. Diese Freigabe betrifft Bilder
und Animationsdarstellung in der Galerie; sie ersetzt keine Funktionsabnahme
der Koop-/Kampf-/Fahrradablaeufe im Emulator oder auf Hardware.

ZeroWalk-Agent `19fe966c-3634-4d2d-ac8f-1827d90b4d93` ist fertig (auto-review).
Alle 15 Figuren haben neun Lauf-/27 Radframes und ein eigenes Kampfrueckenbild;
alle 45 neuen PNGs sind formatiert und vom Agenten auf fuenf Vergleichsboegen
gegen die Originalreferenzen sichtgeprueft. Alle 158 PNGs der bisherigen
84 Figuren stimmen mit ihren alten Hashes ueberein. Auswahl: 99 Figuren,
Fahrrad:97 generiert/2 nativ, Kampf:91 generiert/8 nativ. Details und finale
Metadaten stehen in assets/coop_walk, assets/coop_bike und assets/coop_battle_back.

Gemeinsame Anbindung abgeschlossen: Walk-Modul und walks.bin in full.mk,
verify-coop-walk samt Bike-Headerabhaengigkeit in Makefile, Engine-Klassifizierung
in staging.py und Auditfreigabe fuer neue Walk-PNGs. Alles nach build/upstream
uebernommen. 0086 ist bereits angewendet; Reverse-Check und Enginehash passen.
Nicht erneut anwenden. Packs: walks 69728, bikes 1345228, backpics 190080 Byte.
C-Lauf-/Fahrrad-/Kampftests, echte Enginehooks sowie Python 8+7+3 bestanden.
Gemeinsamer ARM-/Release-/PAK-Build erfolgreich, aktueller Build siehe unten.

Die verlangte lokale HTML-Galerie ist vollstaendig und selbststaendig oeffenbar:
`build/character-gallery/index.html` (99 Figuren, 15 neu, 0 fehlende Grafiken).
Generator: `tools/build_character_gallery.py`, Vorlage: `tools/character_gallery.html`.
Strikter Aufruf: `C:/Python314/python.exe tools/build_character_gallery.py --expect-count 99`.
Drei Tests in tools/test_character_gallery.py pruefen native Bildfolgen/Dauern,
Quinty-Sonderspiegelung, Richtungen, native Kampfwuerfe und alle 99 Figuren.
Auch neue RUN-Folgen werden aus 3ds_coop_walk.c mit [5,3,5,3] Takt gelesen.
JavaScript-Syntax sowie echte Wiedergabefunktion commandAt bei Bildgrenzen,
Schleifen und endlichen Animationen bestanden. Mach 9/Acro 27 Frames; generierte
Kampfruecken ehrlich statisch. Suche, Filter, Richtung, Tempo, Einzelbilder,
lokale Notizen und JSON-Pruefbericht-Export/Import sind vorhanden.

Die Galerie enthaelt native Originalgrafiken und bleibt im ignorierten build/.
Die visuelle Browser-Abnahme wurde nicht durchgefuehrt: Browserhost nicht
verfuegbar; nach Start von Chrome wurde die Navigation zur lokalen Galerie
mit "Computer Use was not approved to use Google Chrome" abgelehnt. Danach
keine weitere Browserbedienung oder Umgehung. Nutzer fragte nach dem Zweck;
es wurde erklaert, dass Chrome nur fuer die HTML-Sichtpruefung gedacht war.
HTML direkt verlinken; Nutzer kann seinen bevorzugten Browser verwenden.

## Sprint-5-Integration vom 2026-11-13

Quelle: origin/sprint-5 `1be2515ff4d13d8ab3a62d8941882748dfbd7260`
(Implementierung dbb9ff0 plus Tests 1be2515). Nach Fetch dateiweise per
Dreiwege-Zusammenfuehrung in den aktuellen lokalen Stand uebernommen,
keine Ruecknahme neuerer main-Korrekturen und kein Push/Commit. Vollstaendige
Sicherung vor Integration: `build/sprint5-integration/before-sprint5.zip`.
Basis/ours/theirs/merged und Quellen-IDs stehen im selben ignorierten Ordner.

Kampfstart/-ende/-tick sind ueber 0083 an battle_main.c und CB2_WhiteOut
angebunden; der isoliert gepruefte Zusatzpatch ist bereits in build/upstream
angewendet. Nicht nochmals anwenden. Protokoll 11 hat Kampf-Pakettypen 33..39,
unveraendert 15-Byte-Positionspakete und alle bisherigen NPC-/Party-/Join-Typen.
Testsuite jetzt CTR_COOP_TEST_MAX_SPRINT=5, ohne erwartete Fehler.

Zusaetzliche Integrationskorrekturen: wiederholte Reservierung ist idempotent,
verlorene Freigaben werden erneut angefragt, ein waehrend des Wartens besiegter
Trainer startet keinen zweiten Kampf. Ergebnisse tragen Sequenznummern;
Doppel-/Altpakete geben keine mehrfachen Belohnungen. Empfangene Ergebnisse
warten bis zum Ende eines lokalen Kampfs/PC-Besuchs, damit eingehende Effekte
nicht als eigene Aenderungen zurueckgesendet werden. Save-Rekey wird fuer
Geld, Muenzen und Bag-Mengen getrennt von der Kampfbasis entschluesselt.
Box-Ziele werden vorab vollstaendig geplant, aktive Spielerreihen geschuetzt;
Inventarfehler/volle Boxen bestaetigen keine Teiluebernahme. Unausgerichtete
82-Byte-Boxrecords werden vor Enginezugriffen in ausgerichtete Daten kopiert.

Auch wilde Gastkaempfe werden vom Host gegen PC-Zugriffe freigegeben; zwei
unabhaengige Kaempfe sind gleichzeitig erlaubt. Der PC bleibt waehrend
aktiver Kaempfe und noch unbestaetigter Ergebnisse gesperrt, um Faenge nicht
mit einem vollstaendigen PC-Snapshot zu ueberschreiben. Bei vollstaendig vollen
Boxen oder nicht anwendbarem Inventar bleibt das Ergebnis unbestaetigt im
Sendepuffer; diese Grenzfaelle brauchen noch eine interaktive Abnahme.
Statische einmalige Wildgegner besitzen im uebernommenen Sprint-5-Branch
noch keine eigene Reservierung; dazu keine Vollstaendigkeitsbehauptung.

Automatisch bestanden: acht Koop-Pruefgruppen, neue Sprint-5-Faelle fuer
parallele Kaempfe/PC-Rennen, Wiederholung/Retry, Rekey, Faenge/Box-Kollisionen,
Inventarfehler, Kampf-/Fahrradgrafiktests und ARM-Compile/Link.
Die Emulator-/Hardware-Abnahme ist offen. Laufende Host/Gast-Prozesse beim
Bauen nicht beendet oder ueberschrieben; danach ueber das Paseo-Script aktualisieren.

## Aktueller gemeinsamer Testbuild

Release und PAK erfolgreich gebaut: Build `cf712051f18d29ac`, ABI `ebf0f1f4`.
3DSX/SMDH liegen unter `build/upstream/3ds_port/dist`, das zugehoerige PAK
unter `build/upstream/3ds_port/build`. Dieser Build enthaelt alle neuen
Kampf-/Fahrradgrafiken fuer 99 Figuren,15 neue Laufboegen und die PC-, Gastnamen- und Beitrittskorrekturen.

Paseo-Script **Host und Gast starten** kopiert die zusammengehoerigen Dateien
in die zentralen portablen Azahar-Profile und startet den lokalen Raumserver.
Vor einer Aktualisierung beide Spielfenster schliessen; F6 laedt keine neuen
Dateien aus dem Arbeitsbaum. Verbindung: 127.0.0.1:24872, Raumpasswort leer.
Einstellungen, Pfade und Startvarianten stehen in `LOKAL_TESTEN.md`.

## Behobene Koop-Ablaeufe

Nach Starterwahl und beim Wiederbeitritt uebertraegt `OpenPCWhenReady()` das
Gast-Team automatisch unter der Host-PC-Sperre. Erst bestaetigte atomare
Uebertragung und Spawn-Paket erlauben das Betreten der Welt. Neue Gastnamen
starten in Wurzelheim; bekannte Namen verwenden ihre gespeicherte Position.
Es wird dabei kein Feld-PC ueber dem Verbindungsbildschirm geoeffnet.

Nach `ApplyHostSave()` liest der Gast seinen eigenen Profilnamen aus dem
lokalen `SaveBlock2.playerName`. Der Hostname wird fuer Besitzermeldungen
separat bewahrt; Snapshot-Pruefsumme, andere Save-Felder und Gast-Speicherschutz
bleiben erhalten. Ungueltige/leere Namen werden als GAST spielkodiert angezeigt.

Ein durch den Mitspieler belegter PC zeigt unten dessen tatsaechlichen
Spielnamen. A, B oder Tippen bestaetigt die Meldung und setzt das wartende
Skript einmal fort. Die Sperre bleibt beim Besitzer. Paket-Reihenfolge,
Disconnect, noch unbekannter Besitzer und die Eingabetaste beim Oeffnen sind
beruecksichtigt und durch Laufzeit-/Sitzungstests abgesichert.

## Basis der bisherigen 84 Figuren (vor der ZeroWalk-Erweiterung)

Beide ausdruecklich beauftragten, in Paseo sichtbaren Subagenten arbeiteten
mit auto-review und Abschlussbenachrichtigung. Ihre Arbeit ist abgeschlossen:
Kampfbilder `8246f5c0-3978-4a0b-9d38-d5c0090751e3`, Fahrraeder
`0a63a5b1-36c0-4771-9c63-5ee8dbe0656e`. Keine weiteren Bilder generieren.

Kampf: 76 neue 64x64-Rueckenansichten und acht native Zuordnungen. Alle sechs
finalen Vergleichsboegen wurden gegen die Figurenreferenzen geprueft;
Manifest/Provenienz enthalten Sichtfreigabe und finale PNG-Hashes. CBK1 enthaelt
158760 Byte. Nur lokale Spieler-/Safari-Controller verwenden die eigene
Auswahl von Host/Gast. Native Bilder behalten ihre Animationen; generierte
Bilder verwenden fuenf sichere Verweise auf dieselbe statische Ansicht. Der
separate Palettenwechsel beim Pokeballwurf bleibt bei der eigenen Figur.
Integration: `3ds_coop_battle.c`, `pack_coop_battle_back.py`, Patch 0084.

Fahrrad: 82 neue 288x96-Boegen mit jeweils 27 Frames, binaerer Transparenz und
hoechstens 15 sichtbaren RGB555-Farben; dazu zwei native Figuren. Alle 82
Identitaeten und alle Frames wurden vom Bilder-Agenten geprueft. Strikter
CBI1-Gesamtpack: 1137208 Byte. Brendan 100 verwendet native 101/102,
May 105 verwendet 106/107. Prompts und versionierbare Provenienz inklusive
finaler Hashes liegen jeweils neben den Assets; Originalreferenzen bleiben
lokal in `build/bike-art/references`. Bildzeichnungen stammen aus Imagegen;
Python dient nur Format, Palette, Transparenz und Skalierung.

Protokollversion 11 hat 15-Byte-Positions-/Spawnpakete mit eigenem bikeMode
(0 Fuss, 1 Mach, 2 Acro) und movementSequence fuer wiederholte Aktionen am Ort.
SetRemote bedeutet ausschliesslich Kontextaenderung. Beim Moduswechsel wird
nach Ende einer laufenden Bewegung dieselbe Objekt-ID neu gebunden.
Sprites reservieren mindestens 512 Byte auch zu Fuss. ClearRemote entfernt
nur Kontext; Reset gibt Art-/Palettenspeicher erst beim Soft-Reset frei.

Alle ausgewaehlten lokalen/entfernten Spieler haben eigene Extpaletten,
auch bei nativen Figuren und zu Fuss. Der begrenzte Engine-Tag-Lookup loest
Originalpaletten auf. Legacy-Palettenwrites werden nur fuer echte Spieler
umgangen; Grafik, Frames, Geometrie, Kamera und Position werden weiterhin
aktualisiert. Reflexionen besitzen eigene Farben, passende Hoehe und die
originale dunkelblaue Hochbrueckenpalette. Absteigen, Wetter und Menue-Rueckkehr
sind durch Verhaltenstests abgesichert. Voxel-Kontext beruecksichtigt Groesse
und Pixelpointer beim Fusspunkt-Cache.

Die finale 0085 ist bereits angewendet. `build/applied-bike.patch` entspricht
der angewendeten Fassung; nicht erneut darueber anwenden. Sie enthaelt genau
drei Engine-Dateidiffs. Module, Make-Regeln und `staging.py` sind integriert;
beide Assetpacks gehoeren zur Engine-RomFS, native Originalpixel zum privaten
ROM-Datenpfad. Keine ROMs, SAVs, PAKs oder Emulatorprofile versionieren.

## Verifikation und verbleibender Nutzertest

Bestanden: acht Koop-Pruefgruppen (Protokoll, Profile, Speicher, Laufzeit,
Sitzung, lokale Sitzung, Build-Kompatibilitaet, Join-Ablauf), Kampf-C-Suite
und drei Packer-Tests, Fahrrad-C-Suite und sieben Packer-Tests sowie Probes
aus den tatsaechlich angewendeten Engine-Grafik-/Reflexionsfunktionen.
Gemeinsamer ARM-Compile/Link und Release-/PAK-Build bestanden. Finaler Audit
inklusive aller neuen Dateien ohne Befund;
git diff --check ohne Fehler. Der separate starter_menu_test kann mit dem
vorhandenen MinGW-Linker wegen unaufgeloester anderer Hauptmenue-Textsymbole
nicht linken und zaehlt nicht als bestanden. Diese Einschraenkung betrifft
nicht den ARM-Release-Link.

Die neuen Grafik-/Palettenablaeufe und die aktuellen PC-/Join-Korrekturen
wurden noch nicht interaktiv im Emulator oder auf Hardware abgenommen.
Mit verschiedenen Figuren fuer Host und Gast testen: eigener Kampfruecken
auch beim Pokeballwurf; Mach/Acro in allen Richtungen samt Huepfen, Auf- und
Absteigen, Spiegelung, Wetter und Menue-Rueckkehr; gegenseitige PC-Sperre mit
richtigem Namen; Gastname in Szenariotexten; Starterbeitritt und Wiederbeitritt.
Die aelteren Emulatorabnahmen unten sind kein Nachweis dieser neuen Aenderungen.

## Fruehere Abnahmen und technischer Hintergrund

Dieser Stand ist eine Entwicklungsversion, **keine vollständig spielbare Alpha**. Sprint 1 bis Sprint 4 sind geprüft und auf `main` zusammengeführt. Die unten dokumentierten Join-, NPC-, Starter-, PC-/Box-, Menü-, Gruppen-, Abbruch- und Wiederbeitrittstests wurden mit zwei isolierten Azahar-Instanzen ausgeführt. Hardwaretests bleiben offen. Sprint 5 wurde noch nicht begonnen. Der Branch `sprint-4` enthält weiterhin den abgenommenen Quellstand; neuere Multiplayer-Korrekturen aus dem bisherigen Arbeitsstand wurden erhalten.

## Übergabe auf main vom 2026-11-12

- Geprüfter Sprint-4-Commit: [`a976ce4`](https://github.com/Sumaiter/emerald-3ds/commit/a976ce4a54a6b0f4b166ad4aa0ff29256ed0eb65).
- Merge nach `main`: `777e778` (`Merge sprint-4: complete co-op storage, PC locking and regression tests`). Der Merge-Inhalt entspricht vollständig dem geprüften Sprint-4-Commit; die bisherigen Änderungen von `main` bleiben enthalten.
- GitHub-Ziele dieser Übergabe: `main` mit dem zusammengeführten Stand bis einschließlich Sprint 4 und `sprint-4` mit dem abgenommenen Quellstand. Implementierungen aus den Branches `sprint-5` bis `sprint-8` werden nicht übernommen.
- Erneute Prüfung des zusammengeführten Stands: alle neun `verify-coop`-Testsuiten, Reverse-Patchkontrolle, Vorwärts-/Rückwärtsprüfung des Integrationspatches, strikter Release-Audit und `git diff --check`. Der bereits vollständig gebaute und im Emulator geprüfte Quellstand bleibt erhalten (Build `e6f233d528c8db82`, PAK-ABI `3a8d61b5`).
- Übertragen werden ausschließlich Quellcode, Tests, Integrationspatches und Dokumentation. ROMs, SAV-Dateien, Emulatorprofile, PAKs und Build-Artefakte bleiben lokal und ignoriert.
- Als Nächstes ist Sprint 5 vorgesehen. Vor dem Weiterarbeiten den vorhandenen Branch `origin/sprint-5` prüfen und den geprüften Stand aus `main` erhalten. Die Grenze bleibt vorerst Host plus ein Gast; Host plus drei Gäste gehört zum späteren Ausbau.

## Multiplayer-Review vom 2026-11-12

Nur die Multiplayer-Erweiterungen seit der Fork-Basis `bcae4b7` wurden bearbeitet:

- Der Gast-Speicherschutz bleibt nach Übernahme der Host-Welt auch bei Abbruch, Abschalten des Koop-Modus und Verbindungsverlust bis zum Soft-Reset aktiv. Ein abgebrochener Suchvorgang ohne übernommene Host-Daten sperrt den lokalen Spielstand anschließend nicht.
- Starterauswahl und Startteam-PC warten im Menü auf den vollständigen Host-Snapshot und die Join-Information. Die Starterbestätigung prüft diese Voraussetzungen ebenfalls.
- Koop-Pakete werden nur vom aktuellen, authentifizierten Mitspieler verarbeitet. Unaufgeforderte und doppelte Authentifizierungsbestätigungen starten keinen neuen Snapshot.
- Eine PC-Freigabe darf nur die Sperre des entfernten Besitzers nach abgeschlossener Übertragung lösen. Beim Übernehmen bleiben die aktuellen eigenen Teamdaten in der eigenen Koop-Reihe erhalten.
- Bei fehlgeschlagenem Ersetzen der Profildatei bleibt die vollständige temporäre Kopie zur Wiederherstellung erhalten. Die Prüfsumme wird vor jeder Normalisierung geprüft; auch eine beschädigte Hauptdatei erlaubt die Wiederherstellung aus einer gültigen temporären Datei.
- Positionspakete mit Blickrichtung null und unbekannter Bewegungsaktion werden verworfen.

`3ds_port/tests/coop_runtime_test.c` führt den tatsächlichen Koop-Code mit simulierten Engine-/Transportfunktionen aus und prüft Join-Reihenfolge, Speicherschutz, Authentifizierung sowie vollständige, atomare PC-Übertragungen. Hinzu kommen Tests für gleichzeitige PC-Anfragen, Menü-Sprites und aus einem Save wiederhergestellte Verbindungsfiguren. Die Tests sind in `verify-coop` integriert. Die ergänzende Sprint-4-Abnahme ist weiter unten dokumentiert.

Prüfstand: `verify-coop`, vollständiger 3DS-Build (`release pak`), Reverse-Patchkontrolle, `git diff --check` und strikter Release-Audit bestanden. Die erneut ausgeführte Builder-Suite meldet 241 Tests, davon 239 bestanden und zwei übersprungen (private Kopie bzw. gesonderter gepatchter Quellbaum fehlen dem jeweiligen Test).

## Laufzeittests vom 2026-11-12

Getestet wurde mit zwei Azahar-2126.2-Instanzen, einem eigenen lokalen Testraum und separaten Kopien der Profile und SAV-Dateien. Die ursprünglichen Spielstände und laufenden Benutzerinstanzen wurden nicht für Eingaben oder Schreibzugriffe verwendet. Speicherzustände wurden zusätzlich über die Debugschnittstelle geprüft.

Bestätigt:

- Suche, Passwort-Beitritt, vollständiger Host-Snapshot mit 53.380 Byte und Prüfsummenprüfung.
- Neuer Gast: Geckarbor auf Level 5, Gruppe mit genau einem Pokémon, PC-Übernahme mit 34.344 Byte und Bestätigung vor dem Spawn in Wurzelheim.
- Gespeicherter Name: Starterauswahl wird übersprungen, Team und gespeicherter Spawn `18,17` werden wiederhergestellt. Das wurde auch mit dem abschließenden Build `4d1d9185242e1fe0` bestätigt.
- Bewegung in beide Richtungen, Host-Kartenwechsel aus dem Haus in die Stadt und gegenseitige Anzeige auf derselben Karte. Beide Figuren können dieselbe Kachel betreten.
- Position und Blickrichtung der drei sichtbaren NPCs stimmen überein; die Gast-NPCs sind eingefroren.
- Der Host bewegt sich weiter, während der Gast den Beutel geöffnet hat. Der Host kann seine Test-SAV speichern; die Gast-SAV bleibt per SHA-256 bytegenau unverändert.
- Hartes Beenden des Host-Prozesses: Verbindungsverlust wird erkannt, die Hostfigur verschwindet beim Gast und der Gast-Speicherschutz bleibt aktiv. Soft-Reset hebt den Schutz wieder auf.
- Die zuvor gespeicherte Gastfigur wird beim Laden des Host-Saves ohne aktive Koop-Sitzung entfernt.
- Drei Beutel-Auf-/Zu-Zyklen bleiben bei laufender NPC-Synchronisation stabil; diese Wiederholung wurde auch im abschließenden Build ausgeführt.

Im Laufzeittest gefundene und korrigierte Fehler:

- NPC-Pakete konnten beim Verlassen des Beutels `ObjectEventTurn()`/`SeekSpriteAnim()` auf inzwischen vom Menü verwendeten Sprites ausführen und zum Absturz an Adresse null führen. NPC- und Avatar-Spritezugriffe sowie deren Bereinigung sind jetzt auf `CB2_Overworld` begrenzt. Wiederholte Beutel-Auf-/Zu-Tests sichern die Rückkehr ab; NPC-Rundlaufpakete aktualisieren den Gast danach wieder.
- Eine in `SaveObjectEvents()` mitgespeicherte Verbindungsfigur blieb nach dem Laden ohne Sitzung sichtbar. Die Bereinigung findet jetzt auch wiederhergestellte Figuren, deren Laufzeit-Slot dem Koop-Modul noch nicht bekannt ist.

Der im Laufzeittest gefundene Fehler bei einem fünften Gastnamen wird durch eine Tabelle mit zehn Einträgen behoben. Festlegung des Nutzers: zehn gespeicherte Gastnamen; keine Verdrängung bestehender Positionen. Der aktuelle Transport bleibt bewusst bei Host plus einem Gast. Host plus drei gleichzeitige Gäste ist das Ziel des nächsten Ausbaus, kein bereits implementiertes Merkmal.

Die Migration von Tabellen-Version 1 auf 2 erhält die ersten vier Einträge sowie die Initialisierungsmarker der Koop-Box. Neue Einträge nutzen denselben reservierten Bereich des Host-Spielstands; die SAV- und Snapshot-Größe bleibt unverändert. Ein unbekannter elfter Name wird vor der Snapshot-Übertragung abgewiesen. Der Suchbildschirm zeigt `10 GASTNAMEN BELEGT` und `BEKANNTEN NAMEN WAEHLEN`; B kehrt zurück. Bekannte Namen bleiben auch bei voller Tabelle zugelassen. Protokoll-Version 8 enthält dafür die eigene Meldung `CTR_COOP_NAME_LIMIT_PACKET`.

Mit Build `5d6a47e821a7a845` wurden zusätzlich im Emulator bestätigt: verlustfreie Migration eines alten Saves mit vier belegten Einträgen, Anlage und tatsächliches Speichern eines fünften Namens in der Host-SAV sowie Ablehnung eines elften Namens mit sichtbarer Fehlermeldung bei einer vorbereiteten vollen Testliste. Die volle Liste und die Gast-SAV bleiben bei der Ablehnung unverändert. Native Verhaltenstests prüfen außerdem alle zehn Positionen nach Wiederherstellung des Save-Blocks und den Beitritt eines bekannten Namens bei voller Liste. Die Integrationspatch-Datei wurde auf isolierten Quellkopien rückwärts und vorwärts angewandt und mit dem Buildbaum verglichen.

## Laufende Gruppensynchronisierung und Ausschalten des Gasts

Vorgabe des Nutzers: Die aktuelle Gruppe muss während des Spielens in der Koop-Box gespiegelt werden. Der Gast meldet sich durch Ausschalten ab; ein gesondertes Abmeldepaket wird nicht vorausgesetzt.

- Beide Rollen spiegeln ihre eigene aktuelle Gruppe in die eigene Reihe. Änderungen der 600 Byte Gruppendaten werden automatisch übertragen; unveränderte Daten erzeugen keinen erneuten Transfer.
- Protokoll 9 ergänzt `PARTY_BEGIN`, `PARTY_CHUNK` und `PARTY_RESULT`. Ein eigener Sendepuffer hält die zu bestätigende Gruppe stabil, während weitere Änderungen bereits auftreten können. Die Gegenseite übernimmt ausschließlich die entfernte Reihe nach vollständigem Empfang und gültiger Prüfsumme. Nach Bestätigung folgt bei Bedarf der neuere Stand.
- PC-Vorgänge und Gruppenübertragung schließen einander aus. Ein Pokémon am PC-Cursor ist noch kein abgeschlossener Vorgang. Die eigene aktive Gruppe wird durch einen entfernten PC-Snapshot nicht zurückgesetzt. Abgewiesene PC-Anfragen setzen das wartende Feldskript im folgenden Frame fort; das gilt auch für gleichzeitige Anfragen und Disconnect während einer Anfrage.
- Nach erkanntem Disconnect bleibt die letzte vollständig übernommene Gast-Reihe erhalten und wird entsperrt. Die Host-Reihe bleibt auch bei ausgeschaltetem Funk gegen Entnahme ihrer gespiegelt vorhandenen Pokémon gesperrt, um Duplikate zu vermeiden.
- Das Spiegeln aktualisiert zunächst den RAM des Hosts. Der normale Host-Speichervorgang schreibt den gemeinsamen Stand auf die SD-Karte. Daten, die beim Ausschalten noch nicht vollständig angekommen sind, können nicht zugesichert werden; ein Teiltransfer ersetzt niemals die bisherige Reihe.

Mit zwei isolierten Emulatoren geprüft: gültige Änderungen an EP (`135 → 136`), AP (`35 → 34`) und getragenem Item (`0 → 13`) in der Gastgruppe. Die Änderungen erschienen ohne PC-Abschluss bytegenau in der Host-Reihe. Anschließend wurde der Gast-Prozess hart beendet. Der Host behielt die aktualisierte Reihe und entfernte die Gastfigur. Nach normalem Host-Speichern wurde die aktualisierte Reihe direkt in der SAV geprüft; nach Soft-Reset und erneutem Laden stimmte sie weiterhin bytegenau überein. Diese Änderungen waren gezielte Debug-Testdaten, kein bereits abgenommener Kampfablauf aus Sprint 5.

Native Tests prüfen zusätzlich Änderungen während einer noch offenen Bestätigung, gleichzeitige Übertragung beider Gruppen, fehlerhafte Prüfsummen, Verbindungsabbruch nach einer vollständigen beziehungsweise einer unvollständigen Übertragung, PC-Konflikte und die Fortsetzung abgewiesener PC-Skripte. Vor der abschließenden Sprint-4-Abnahme wurde dafür Build `129b2efb9dbb3219` einschließlich Datenpaket (ABI `2729bb0c`) geprüft; sein erneuter Start und das Laden der aktualisierten Gast-Reihe bestanden. Dieser frühere Stand umfasste acht Testsuiten. Den neueren Build und die vollständige PC-Abnahme dokumentiert der folgende Abschnitt.

## Sprint-4-Abnahme vom 2026-11-12

Zusätzlich behobene Fehler:

- Ein direkter Ausstieg aus dem PC-Auswahlmenü gab die Sperre bisher nicht frei. Dagegen konnte die Rückkehr aus der Box die Sperre zu früh freigeben. Jetzt umfasst eine Transaktion den gesamten PC-Besuch einschließlich des Auswahlmenüs. Der endgültige Menüausstieg überträgt den abgeschlossenen Zustand; die bestätigte Übernahme gibt die Sperre frei. Die anfängliche Team-Auswahl kann auch ohne Öffnen der eigentlichen Box direkt beendet werden.
- B wurde während der anfänglichen PC-Auswahl gleichzeitig vom übergeordneten Beitrittstask verarbeitet und brach den Beitritt ab. Der Beitrittstask ignoriert B nun bei geöffnetem PC. Ein Test mit dem echten Menü- und Taskcode sichert das ab.
- Eine gesperrte Entnahme wurde erst tief in der Cursor-Animation abgewiesen; nachfolgende Animationsschritte konnten dadurch einen nicht vorhandenen bewegten Pokémon-Sprite verwenden. Die Eingabe wird jetzt vor dem Start der Operation abgewehrt. Auch automatische Auswahl kann keine Mehrfachauswahl in der geschützten Box beginnen.
- Ein beitretender Gast wartet auf das Ende eines offenen Host-PC-Besuchs, damit kein Pokémon am Cursor in einem unvollständigen Snapshot verschwindet. Ein Gast-Abbruch beendet einen offenen lokalen Host-PC nicht. Der Host kann den Besuch anschließend vollständig abschließen.
- Beim Wiederbeitritt werden Lücken in der Gast-Reihe geschlossen. Andernfalls zählte die Engine nach Entnahme des ersten Pokémon eine leere Gruppe, obwohl spätere Reihenplätze noch belegt waren.

Abnahme mit fünf gültigen Pokémon unterschiedlicher Kennung in getrennten Emulator-SAVs:

| Prüfung | Ergebnis |
|---|---|
| Beitritt während geöffnetem Host-PC | Snapshot wartet bis zum abgeschlossenen Host-PC-Besuch |
| Leere Startgruppe per kontrollierter Testvorbereitung | Ausstieg abgewiesen, Auswahl bleibt geöffnet |
| Direkter B-Ausstieg aus der Startteam-Auswahl | Vollständige Übernahme und Spawn, kein Abbruch durch den Beitrittstask |
| Gegenseitige PC-Zugriffsversuche | Nur der Besitzer öffnet den PC; abgewiesene Spieler bleiben bedienbar |
| Normale und automatische Auswahl beider aktiver Spielerreihen | Keine Entnahme und kein veränderter Bestand |
| Gast entnimmt aus normaler Box; Host legt ein Pokémon ab | Nach Bestätigung stimmen beide Bestände überein; alle fünf Kennungen genau einmal vorhanden |
| Rückkehr aus der Box zum PC-Auswahlmenü | Sperre bleibt bestehen; noch keine vorzeitige Übernahme |
| Gast entnimmt ein weiteres Pokémon und wird vor dem PC-Abschluss hart beendet | Letzter bestätigter Host-Bestand bleibt unverändert; PC-Sperre wird freigegeben |
| Host entnimmt aus der freigegebenen Gast-Reihe | Einmalige Entnahme möglich; wiederholter Versuch erzeugt keine Kopie; Host-Spiegelung bleibt gesperrt |
| Host speichert, beide Emulatoren starten neu, Gast tritt wieder bei | Alle fünf Kennungen erhalten; der Gast erhält nur das verbliebene Pokémon, lückenlos an Gruppenposition 0 |

Der Host-Save wurde zusätzlich direkt geprüft: zwei Host-Pokémon, zwei Pokémon in normalen Boxen und ein verbliebenes Gast-Pokémon; jede Kennung genau einmal. Die Gast-SAV bleibt bytegenau unverändert. Die Testdaten liegen nur im ignorierten lokalen Testbereich.

Alle neun Testsuiten aus `verify-coop` bestehen, darunter die zusätzlichen Regressionstests des Sprint-4-Branches mit `CTR_COOP_TEST_MAX_SPRINT=4` und ohne erwartete Fehler. Neue native Prüfungen decken volle Boxen, unveränderte Solo-Boxen, Teilübertragungen, PC-Abbruch, verzögerten Snapshot und Lücken in der Gast-Reihe ab. Der abschließende Release-Build `e6f233d528c8db82` einschließlich PAK besteht (ABI `3a8d61b5`). Reverse- und Vorwärtsanwendung des Integrationspatches ergeben auf Kopien mit einheitlichen LF-Zeilenenden wieder dieselben Quelldateien. Strikter Release-Audit und `git diff --check` bestehen. Hardwaretests und die Kampfabläufe ab Sprint 5 bleiben offen.

## Repository

- Fork: `https://github.com/Sumaiter/emerald-3ds`
- Original: `https://github.com/ZallaxDev/emerald-3ds`
- Veröffentlichung und Ersteinrichtung erfolgen über `main`.
- Übergabestand bis Sprint 4: Merge `777e778`, geprüfter Quellstand `a976ce4`; Details stehen im Abschnitt „Übergabe auf main“.

## Mit zwei Azahar-Instanzen bestätigt

- Der Gast lädt seine Welt nicht mehr aus seiner lokalen SAV-Datei.
- Der Host überträgt `SaveBlock2`, `SaveBlock1` und `PokemonStorage` mit Prüfsumme vollständig in den RAM des Gastes: **53.380 Byte** in 128-Byte-Chunks.
- Der Gast übernimmt den Zustand erst nach vollständigem Empfang und korrekter Prüfsumme. Party, ObjectEvents, Beutel und Verschlüsselungsschlüssel werden daraus initialisiert.
- Ein neuer Gastname startet an einem festen, freien Punkt in Wurzelheim, auch wenn der Host sich an einem anderen Ort befindet.
- Nach Bewegung, Host-Speichern und erneutem Beitritt erscheint der Gast wieder an derselben gespeicherten Position.
- Kartenwechsel durch Türen funktionieren einschließlich Verschwinden, gemeinsamer Sichtbarkeit und Rejoin an der richtigen Position.
- Die Karte wird beim Beitritt genau einmal durch `CB2_ContinueSavedGame()` aufgebaut. Ein zusätzlicher `InitMap()`-Aufruf wurde entfernt, weil er die Kartendarstellung beschädigte.
- Host und Gast sehen sich und können sich bewegen.
- Die lokale Gast-SAV bleibt bytegenau unverändert; vorher und nachher war der SHA-256 identisch.

Die wichtigen Protokollmeldungen sind `coop: host save applied 53380 bytes` und `coop: join warp complete at X,Y`.

## Verbindliche Spawnregel

- **Neuer Gastname:** fester Spawn in Wurzelheim (`0.9`, Layout `10`, Position `19,16`).
- **Bekannter Gastname:** immer die letzte unter diesem Namen gespeicherte Position.
- Es gibt **keine Stadt- oder Spawn-Auswahl**. Ein zwischenzeitlich eingebautes Auswahlmenü wurde auf Nutzerentscheid wieder entfernt.
- Gespeicherte Namen wurden nach Entfernung dieses Menüs wiederholt mit ihren gespeicherten Spawnpunkten geprüft.

## Implementierung

Geänderte Dateien:

- `3ds_port/include/3ds_coop.h`
- `3ds_port/include/3ds_coop_protocol.h`
- `3ds_port/src/3ds_coop.c`
- `3ds_port/src/3ds_coop_protocol.c`
- `3ds_port/tests/coop_protocol_test.c`
- `patches/pokeemerald/0083-local-coop.patch`

Das Protokoll hat Version 9. Es enthält Authentifizierung, Identität, Starteranforderung, Spawn, Snapshot-Beginn, Chunk-Anforderung, Chunk, Übernahmebestätigung, NPC-Positionen, PC-Sperre, atomare PC-Übertragungen, laufende Gruppenübertragung sowie Story-Flags und Skriptvariablen. Große Snapshot- und PC-Transfers senden Chunks nur auf Anforderung. Kleine Gruppenänderungen senden einen eingefrorenen Stand in sechs zuverlässigen Nachrichten und warten dann auf Bestätigung. `PrepareHostSave()` ruft vor dem Snapshot `SaveObjectEvents()` auf, damit er die aktiven NPC-Positionen statt des letzten Disk-Speicherstands enthält. Danach sendet der Host Änderungen sofort und alle sichtbaren NPCs im Rundlauf erneut. Der Gast übernimmt deren Position, Blickrichtung und Grafik und friert die unabhängige lokale NPC-Bewegung ein. Position und Blickrichtung wurden im Laufzeitvergleich bestätigt.

Die namensbasierte Spawntabelle liegt in `gSaveBlock1Ptr->unused_3598`, mit Magic `0x5350`, Version `2` und zehn Einträgen. Version 1 mit vier Einträgen wird automatisch migriert; dabei werden auch die verschobenen Koop-Box-Marker erhalten. Die gesamte Erweiterung belegt 248 der 384 reservierten Bytes. Positionspakete verwenden ObjectEvent-Koordinaten einschließlich `MAP_OFFSET`; `SetWarpDestination()` erhält die Koordinaten ohne diesen Rand.

Vor `CB2_ContinueSavedGame()` setzt `ContinueJoinedGame()` über `CtrCoop_EnterHostWorld()` das Warp-Ziel und leert `gSaveBlock1Ptr->mapView`. Dadurch kann `InitMapFromSavedGame()` keinen Ausschnitt der alten Hostposition über die frisch geladene Gastkarte kopieren. Bei einem Spawn auf einer anderen Karte entfernt der Join zusätzlich aktive NPC-Objekte der alten Karte und lädt die Objektvorlagen des Ziels; das lokale Spielerobjekt bleibt erhalten. Der normale Continue-Pfad lädt Karte, Skripte und Spieler genau einmal. Nach den Karten-Ladeskripten setzt `CtrCoop_OverworldFrame()` die Gastfigur exakt auf den gespeicherten Spawn. Erst danach endet `joinWarpPending` und die normale Bewegungssynchronisation beginnt.

Nach **A** ist die Auswahl im Suchmenü während des Beitritts gesperrt. Die ausgewählte Welt bleibt zwischengespeichert und sichtbar, auch wenn die Suche sie zwischenzeitlich nicht mehr liefert. Der Titel zeigt `VERBINDEN …`; der Hinweis auf eine fehlende Welt wird während des Beitritts nicht angezeigt. **B** kann weiterhin abbrechen.

## Sprint 4: Starter, Team und PC

- Ein neuer Gast ohne Spawn-Eintrag und ohne Pokémon in der Gast-Reihe wählt verbindlich Geckarbor, Flemmli oder Hydropi auf Level 5. Die Auswahl erzeugt genau ein Pokémon in der Gast-Reihe und im Team. `CB2_StarterSelectionActive()` verhindert, dass der Auswahltask im Verbindungsablauf erneut initialisiert wird. `CB2_InitStarterSelection()` gibt die Kontrolle an `CB2_MainMenu()` zurück, damit `RunTasks()` die Auswahl tatsächlich ausführt. `3ds_port/tests/starter_menu_test.c` prüft diesen Schedulerpfad, Hoch/Runter einschließlich Umbruch und die Bestätigung aller drei Starter mit dem echten Menü- und Taskcode. Der Empfang der Join-Info ist verbindliche Voraussetzung für den Beitrittsabschluss; Frameverarbeitung, PC-Öffnung und Weltbetreten bleiben bis dahin und bei neuen Gästen bis zur Starterwahl gesperrt. Danach öffnet der Startteam-PC; erst die bestätigte atomare PC-Übernahme löst den Spawn aus.
- Ein wiederkehrender Gast überspringt die Starterauswahl und verwendet gespeicherte Gast-Reihe und Spawnposition.
- Box 14 (Index `13`) ist die KOOP-Box. Reihe 1 (Indizes `0–5`) gehört dem Host, Reihe 2 (Indizes `6–11`) dem Gast. Bestehende Pokémon werden vor der Reservierung in normale Boxen migriert; ohne ausreichenden Platz wird die Welt nicht geöffnet.
- Eingeloggte Spielerreihen sind gegen Entnahme, Ablegen, Tausch, Freilassen und Mehrfachverschieben gesperrt. Nur eine Rolle darf den PC gleichzeitig verwenden.
- Der endgültige Ausstieg aus dem PC-Auswahlmenü überträgt vollständigen `PokemonStorage` und Gruppe mit Prüfsumme. Die Gegenseite übernimmt erst nach vollständigem Empfang und bestätigt danach; erst diese Bestätigung gibt die PC-Sperre frei. Beim Wechsel zwischen Box und Auswahlmenü bleibt die Sperre bestehen. Die anfängliche Team-Auswahl kehrt nach ihrem Abschluss direkt ins Beitrittsmenü zurück.
- Persistente Story-Flags und Skriptvariablen werden laufend synchronisiert. Sende- und Empfangsschleifen sind auf `FLAGS_COUNT` beziehungsweise `VARS_COUNT` begrenzt; die früheren Obergrenzen `SPECIAL_FLAGS_START` und `SPECIAL_VARS_START` überschrieben die Baseline-Puffer und nachfolgende Koop-Zustände wie Starterstatus und Sitzungsgeneration. Der Join-Flow-Test sichert alle vier Grenzen. Der Laufzeittest der neuen Abläufe steht noch aus.

## Kollisionen und Menüs

- `LOCALID_COOP_PLAYER` ist im zentralen Kollisionscheck ausgenommen: Die Verbindungsfigur blockiert keine Bewegung. Terrain, NPCs und normale Kollisionen bleiben unverändert.
- Die Figur besitzt kein NPC-Script; ein Dialog wird nicht ausgelöst.
- Menüs, Beutel, PC und Kämpfe verwenden getrennte Engine-Callbacks und keine gemeinsamen Menüzustände im Koop-Modul.
- Kollisionen, NPC-Interaktionen und unabhängige Menüs wurden vom Nutzer im Laufzeittest bestätigt.

## Noch offen

- Änderungen am gemeinsamen Beutel und anderen bisher nicht laufend übertragenen Inventardaten synchronisieren; Gruppen, Story-Flags und Skriptvariablen werden bereits laufend übertragen.
- Die Gruppenübertragung bei späteren echten Kampfabläufen einschließlich Fang und Niederlage prüfen.
- Unabhängige Einzelkämpfe und später gemeinsame Doppelkämpfe implementieren.
- Dritte Konsole und bestehende Link-Theken erneut prüfen.
- Tests auf zwei realen 3DS-/2DS-Konsolen durchführen.

Sprint 3 und Sprint 4 sind nach den dokumentierten Emulator- und nativen Tests abgeschlossen.

## Nächste Arbeitsschritte

1. Den bestätigten Basistest einschließlich Kartenwechsel, Kollisionen und Menüs nicht erneut als offenen Fehler behandeln.
2. Den abgenommenen Stand auf `main` und `sprint-4` erhalten; vorhandene spätere Sprint-Branches vor der nächsten Implementierung prüfen.
3. Als Nächstes Sprint 5 mit unabhängigen Einzelkämpfen und Trainerreservierungen beginnen.
4. `Status.md` und diese Datei nach bestätigten Tests aktualisieren.

## Build- und Testregeln

- Quelle liegt im Repository unter `3ds_port/`; `build/upstream/` wird nicht committed.
- Vor einem inkrementellen Build geänderte Portdateien auch nach `build/upstream/3ds_port/` kopieren.
- `main_menu.c` wird im Buildbaum geändert und muss mit `patches/pokeemerald/0083-local-coop.patch` übereinstimmen.
- Patchkontrolle: `git -C build/upstream apply --check --reverse ../../patches/pokeemerald/0083-local-coop.patch`.
- Vor dem Abschluss: `make release pak`, `make -C 3ds_port verify-coop`, `git diff --check` und `python tools/release_audit.py --repo . --strict`.
- ROMs, Spielstände, PAKs, Build-Artefakte und Emulatorprofile niemals committen oder hochladen.

## Fortsetzung in einer neuen Paseo-Sitzung

> Lies zuerst `AGENTS.md`, `HANDOFF.md`, `SETUP.md` und `Status.md`. Sprint 3 und Sprint 4 sind abgeschlossen und auf `main` zusammengeführt; `sprint-4` enthält den abgenommenen Quellstand. Prüfe vor Sprint 5 den vorhandenen Branch `origin/sprint-5` und erhalte die abgenommenen Korrekturen aus `main`. Zehn Gastnamen werden gespeichert; aktuell bleibt es bei Host plus einem Gast. Neue Namen starten in Wurzelheim, bekannte Namen an ihrem gespeicherten Spawn. Die aktuelle Gruppe wird laufend in die Koop-Box gespiegelt; der Gast beendet die Sitzung durch Ausschalten. Keine ROMs, Saves oder Build-Artefakte committen. Halte die Dokumentation aktuell und teste jede Änderung im Emulator.
