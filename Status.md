# Status: Emerald 3DS Koop-Mod

## Sprint 8: Stabilisierung integriert, Hardwareabnahme offen (2026-11-15)

Build **de9038e56316c802**, Boot **E3ID-7ece52197182658c**, ABI **a8444026**, Protokoll 21.

Sprint 7 samt beiden bestandenen Nutzertests als 133aa06 auf origin/main
gesichert. Mergequelle Sprint 8: f378496. Zusaetzliche Protokolltests weisen
falsche Versionen/Pakettypen, gekuerzte Pakete und falsche Chunk-Offsets ab.
Der uebernommene Logout-Laengentest prueft jetzt echte Verkuerzung auf neun
Bytes statt versehentlich eine elf Byte lange Ueberlaenge. Native Compile-
Assertions sichern 33744 Byte Boxspeicher, 53380 Byte initialen Snapshot auf
32-Bit-Zielen und 1232 Byte Doppelkampf-Setup. Protokoll bleibt 21.

SETUP verweist auf den aktuellen Main-Stand, die Gesamtpruefung der gestagten
Engine und die bestehenden Abnahmen. Veraltete Einzelpatch-Reversechecks
wurden durch den Hinweis auf die komplette geordnete Patchkette ersetzt.
Das optionale Test-Save-Release ist erreichbar und enthaelt eine SAV-Datei;
es wurde nichts heruntergeladen oder in laufende Profile geschrieben.

Gesamtverify einschliesslich Koop-/Protokoll-/Speicherregressionen, ARM-Release,
PAK und Link-/Assetaudit bestanden (34 Pflichtsymbole, 6388 Assets). Nachweise:
build/sprint8-integration/{verify.log,release.log,release-audit.log,build-hashes.json}.
Sprint 8 bleibt fuer reale Konsolen,
Old-/New-3DS mit Voxel, laengere Sitzungen und native Linkfunktionen manuell
offen; der Branch-Merge ist keine Hardware-/Alpha-Abnahme. Offene Issues
#1..4 und #6..8 bleiben bestehen. Naechste Testschritte in LOKAL_TESTEN.md.

## Sprint 7: Nutzerabnahme bestanden (2026-11-15)

Nutzer bestaetigt beide gezielten Nachtests fuer Build 8af20c2c7a809005:
normales Gast-Abmelden mit sicherer Rueckkehr ins Hauptmenue sowie erneuter
Beitritt und Speichern/Neuladen mit unveraenderten Pokemon/KP/AP/Status ohne
Duplikate. Die neue Sprint-7-Abmeldung ist damit manuell abgenommen. Bereits
in Sprint 6 gepruefte Abbruchpfade bleiben bestaetigt; nicht benannte Varianten
werden nicht zusaetzlich als getestet ausgegeben. Offene Issues #1..4 und
#6..8 bleiben bestehen. Auftrag: Abnahme pushen, Sprint 8 integrieren.

## Sprint 7: Integration von Abmeldung und sicherem Abbruch (2026-11-15)

Build **8af20c2c7a809005**, Boot **E3ID-4fe482705fd68761**, ABI **a8444026**, Protokoll 21.

Sprint 6 ist als 404dd82 auf origin/main gesichert. Der vorbereitete Sprint-7-
Branch b6bb91c ist mit dem aktuellen Stand integriert; Protokoll 21, Logout-
Pakete 47..50. Die bestehende sichere Hostverlust-Menue-Rueckkehr, atomare
Kampfergebnisse, PC-/Story-/Bewegungs-Savebarrieren und RAM-Journale bleiben
erhalten. Der alte Sprint-7-PeerLeft-Pfad, der auch unabhaengige Hostkaempfe
beenden wuerde, wird nicht uebernommen.

Geordnetes Gast-Abmelden friert im sicheren Feldzustand das volle echte Team
ein und uebertraegt es mit Pruefsumme. Erst nach kompletter Uebernahme wird die
Gastreihe freigegeben und ein Host-Autosave vorgemerkt. Verlorene Requests/
Chunks und die finale Bestaetigung werden wiederholt; doppelte BEGIN setzen
keinen Teiltransfer zurueck. Alte Party-/PC-Pakete nach Commit werden ignoriert.
PC-Cursor und andere Transfers verhindern einen konkurrierenden Logout-Commit.
Waehrend der finalen Uebertragung sind Gast-Eingaben gesperrt; Abschluss oder
Timeout verlassen den Engine-Stack ueber den getesteten Hauptmenuepfad.
Ein Abmelden aus unsicheren Kontexten nutzt den bisherigen Abbruchpfad mit
zuletzt vollstaendig uebernommenem Team, niemals ein temporaeres Kampfteam.

Gesamtverify einschliesslich Koop-/Logout-/UI-Regressionen, ARM-Release, PAK
und Link-/Assetaudit bestanden (34 Pflichtsymbole, 6388 Assets). Nachweise:
build/sprint7-integration/{verify.log,release.log,release-audit.log,build-hashes.json}.
Manuelle Sprint-7-Abnahme noch offen, siehe LOKAL_TESTEN.md. Issues #1..4 und
#6..8 bleiben offen. Issue #5 nach bestaetigter Korrektur auf GitHub geschlossen.
Nachfolgend Historie.

## Sprint 6: Nutzerabnahme und Uebergang zu Sprint 7 (2026-11-15)

Nutzerrueckmeldung: "ansonsten klappt jetzt alles von sprint 6". Der zuletzt
bereitgestellte Build fb72074e4a979dff ist damit manuell abgenommen, ausgenommen
die separat erfassten offenen Issues #1, #2, #3, #4, #6, #7 und #8. Die vorher
offenen Nachtests zu KP-Boxen, Trainer-Fluchtsperre, EP und erneutem Kampfeinstieg
gelten damit als bestaetigt. Issue #5 (Trainerbild im Wildkampf) ist behoben und
vom Nutzer bestaetigt. Allgemeine Abnahme ersetzt keine Aussage zu ungenannten
Spezialfaellen. Regulaeres Koop-Trainerpreisgeld bleibt eine bereits dokumentierte
Implementierungsluecke. Die folgenden Build-/Testabschnitte sind Historie.

Sprint-6-Korrekturen werden auf GitHub gesichert; danach wird der vorbereitete
Sprint-7-Branch fuer Abmeldung, Abbrueche und sicheres Speichern integriert.

## Offenes Issue: sprunghafte NPC-Bewegung beim Gast (2026-11-15)

[Issue #8](https://github.com/Sumaiter/emerald-3ds/issues/8): Bewegte NPCs
erscheinen beim Gast jeweils auf dem naechsten Feld, statt wie beim Host
fluessig zu laufen. Sichtbarer Effekt gemeldet, technische Ursache offen.
Eigenes NPC-Issue neben Spieler-Nachlaufen #3; nur erfasst, noch nicht behoben.

## Offenes Issue: einzelner Spieler-K.-o. im Doppelkampf (2026-11-15)

[Issue #7](https://github.com/Sumaiter/emerald-3ds/issues/7): Sind alle
teilnehmenden Pokemon eines Spielers besiegt, ist dieser Spieler K. o.; der
Partner bleibt im Kampf und kaempft allein weiter. Verloren ist erst, wenn
beide Teams ausfallen. Nur erfasst, keine Implementierung oder neue Abnahme.
Die fruehere Nutzerabnahme besiegter Teams bleibt als historischer Test erhalten.

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

[Issue #5](https://github.com/Sumaiter/emerald-3ds/issues/5): In einem wilden
Doppelkampf erscheint laut Nutzerbild auf einer Seite zusaetzlich ein
Gegner-Trainer hinter den wilden Pokemon. Als eigener offener Fehler
erfasst; Ursache und betroffene Rolle noch nicht isoliert, keine Korrektur.

## Neues Sprint-6-Issue: Touch-Zielauswahl beim Folgespieler (2026-11-15)

[Issue #4](https://github.com/Sumaiter/emerald-3ds/issues/4): Der folgende
Spieler kann nach der Attackenwahl im Doppelkampf kein Angriffsziel per Touch
auswaehlen. Nutzerbild: Zielkarten fehlen auf seiner Seite, beim anderen
Spieler sind sie sichtbar. Als eigener offener Fehler erfasst; keine neue
Implementierung oder Abnahme. Beide Fuehrungsrollen und NPC-Partnerfaelle
sind fuer den spaeteren Nachtest vorgemerkt.

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

Sprint-6-Fixes vom 2026-11-15: nativer Koop-Linkkampfempfang korrigiert,
Nachlaufen bei Kurven/Paketbursts samt Lauftempo ueberarbeitet, beide Route101-
Tests solo ab einem Pokemon und maximal den ersten drei. Hintergrundabnahme,
102-Patchkette und ARM-Release/PAK bestanden. Manueller Nachtest steht aus;
laufende Emulatoren und SAVs unangetastet. Build **6fd2556e4194dce4**, Boot **E3ID-16c4628f404bb3f4**, ABI **837d59ef**, Protokoll 20.

Stand: 2026-11-15

Upstream-PR-Pruefung abgeschlossen: 12 uebernommen, 7 bewusst ausgelassen oder hier
nicht anwendbar. Entscheidungen mit Commit-IDs stehen dauerhaft in
[docs/UPSTREAM_PR_REVIEW.md](docs/UPSTREAM_PR_REVIEW.md); unveraenderte PR-Heads
werden nicht erneut geprueft. Neue Hintergrundtests und ARM-Release/PAK bestanden,
manuelle Abnahme offen. Neueste Buildkennung und Grenzen in HANDOFF.md.

Neuester Ausbau: explizite Folgegruppe mit A/B, gemeinsame Kaempfe nur beim
Hinterherlaufen, erste drei pro Spieler, Partner spielt nach drei KO weiter,
Trainerergaenzungen bis sechs und steuerbarer Story-NPC-Partner. Protokoll20;
Hintergrundtests/101Patchkette/Release bestanden, Emulatorabnahme offen.

Neu fuer die manuelle Sprint6-Abnahme: zwei wiederholbare Level5-Doppelkaempfe
auf Route101, links Junge ohne NPC-Partner und rechts Troy mit NPC-Partner.
Koop nur mit Folgegruppe; der Folgespieler steuert die drei geliehenen Pokemon.
Hintergrundchecks und Build bestanden, die manuellen Tests stehen noch aus.

Vorheriger Integrationsstand: Upstream/main erneut geprueft, keine fehlenden
neuen Commits. Sprint6 in Main integriert und direkt erkannte Fallback-/
Team-/Callback-/Ledgerprobleme repariert; Hintergrundtests und ARM-Build gruen.
Sprint6 bleibt IN ARBEIT: native Linkpfade ueberspringen bisher EP und normales
Trainerpreisgeld, und die gemeinsame Kampfsteuerung ist noch nicht im Emulator
abgenommen. Gras #1 und Zuschauer-Ruckeln #2 bleiben getrennt zurueckgestellt.
Neueste Build-/Belegangaben stehen oben in HANDOFF.md und LOKAL_TESTEN.md.


Neuester Emulatornachtest: Gras beim entfernten Spieler fehlt weiterhin;
Zuschauen ist verbessert, aber noch laggy. Beide Nachtests sind NICHT bestanden
und als getrennte Issues zurueckgestellt: [Gras #1](https://github.com/Sumaiter/emerald-3ds/issues/1)
und [Zuschauen #2](https://github.com/Sumaiter/emerald-3ds/issues/2).
Auf Nutzerwunsch folgen jetzt Upstreampruefung und Sprint6-Integration.
Der neue Build wurde anhand beider Logs bestaetigt.
Die anderen vom Nutzer bestaetigten Standardtests bleiben anerkannt.

Aktuelle Sprint-5-Abnahme: Der Nutzer bestaetigt die bisherigen Standardtests
sowie Host-F6/Gast-Hauptmenue, Zuschauer-B-Beschriftung und NPC-Marktfuehrung.
Grasbewegung und lokale Zuschauerpraesentation sind jetzt implementiert,
gemeinsam im Hintergrund geprueft und als neuer Release/PAK gebaut.
Der gezielte Emulatornachtest dieser zwei Aenderungen steht aus; insbesondere
ist die praktische Zuschauerfluessigkeit noch nicht abgenommen. Unter Rueckstau
koennen kurze Displayzustaende weiterhin fehlen. Einzelne Grenzfaelle und die
Reservierung einmaliger fester Wildgegner bleiben offen. Details in HANDOFF.md
und LOKAL_TESTEN.md. Sprint 9 ist fuer vollstaendiges Solospiel eingeplant.

## Ziel

Ein Host und Gäste nutzen über Nintendo-3DS-Local-Wireless (UDS) gemeinsam den Spielstand und die Welt des Hosts. Ein Gast benötigt keinen eigenen Spielstand. Ein Online-Modus über Internet oder WLAN-Router ist nicht Bestandteil dieses Projekts.

Festgelegte Grenzen: maximal zehn gespeicherte Gastnamen pro Host-Spielstand, ohne alte Positionen zu verdrängen. Dieser Ausbau betrifft nur die Namens-/Spawn-Speicherung. Der aktuelle Prototyp erlaubt weiterhin einen Host und einen Gast gleichzeitig. Im nächsten Ausbau sollen höchstens vier Spieler gleichzeitig zugelassen werden: Host plus drei Gäste.

## Statusregeln

- Einzelne Aufgaben werden mit `[x]` erst als erledigt markiert, wenn ihre Umsetzung tatsächlich geprüft wurde.
- Ein Sprint ist erst **fertig**, wenn alle zugehörigen Akzeptanzkriterien erfüllt und die relevanten Tests bestanden sind.
- Teilweise fertige Sprints bleiben **in Arbeit**. Nicht begonnene Sprints sind **offen**.
- ROMs, Spielstände, Datenpakete und andere urheberrechtlich geschützte Spieldaten werden niemals committed oder hochgeladen. Die vorhandenen Regeln in `.gitignore` bleiben erhalten.
- Vor jedem Push werden die tatsächlich übertragenen Dateien geprüft.

## Sprintübersicht

| Sprint | Inhalt | Status |
|---|---|---|
| 0 | Projekt einrichten und Ausgangsbasis prüfen | Fertig |
| 1 | Technischen Koop-Prototyp nachweisen | Fertig |
| 2 | Local-Wireless-Lobby und Passwort-Beitritt | Fertig |
| 3 | Gemeinsame Oberwelt und zweiten Spieler darstellen | Fertig |
| 4 | Spielerprofil, Mehrspieler-Box und PC-Sperre | Fertig |
| 5 | Unabhängige Einzelkämpfe und gemeinsame Trainerwelt | In Arbeit: Standardtests bestanden; Gras-/Zuschauer-Nachtest und Grenzfaelle offen |
| 6 | Gemeinsame Doppelkämpfe | In Arbeit: integriert, EP/Preisgeld und Emulatorabnahme offen |
| 7 | Abmeldung, Verbindungsabbrüche und sicheres Speichern | Offen |
| 8 | Hardwaretests, Stabilisierung und spielbare Alpha | Offen |
| 9 | Vollständiges Solospiel: Entwicklungen, frühere Event-Inhalte und VM-Feldaktionen ab Erhalt | Offen |

## Multiplayer-Review vom 2026-11-12

Speicherschutz nach Gast-Abmeldung, Reihenfolge von Snapshot und Starterauswahl, Authentifizierung eingehender Pakete, PC-Sperrfreigabe, Erhalt der eigenen Teamreihe, Profilwiederherstellung und Validierung der Blickrichtung wurden korrigiert. Zusätzliche Verhaltenstests laufen über `make -C 3ds_port verify-coop`; Details stehen in `HANDOFF.md`.

Zwei isolierte Azahar-Instanzen bestätigen Host-Snapshot, Starter auf Level 5, PC-Übernahme vor dem Spawn, Bewegung, Kartenwechsel, nicht blockierende Avatare, autoritative NPC-Positionen, unabhängige Menüs, Host-Speichern und Gast-Speicherschutz bei hartem Host-Abbruch. Ein Absturz beim Beutel-Verlassen durch NPC-Zugriffe auf Menü-Sprites sowie eine nach dem Laden verbliebene Gastfigur wurden gefunden und korrigiert. Die bisherige Vier-Namen-Tabelle wird verlustfrei auf zehn Einträge migriert; unbekannte elfte Namen werden vor der Spielstandsübertragung mit einer eigenen Meldung abgewiesen. Bereits gespeicherte Namen können auch bei voller Tabelle wieder beitreten. Sprint 3 und Sprint 4 sind abgenommen; Hardwaretests und die folgenden Sprints bleiben offen.

## Festgelegte Spielregeln

- Der Host öffnet seinen geladenen Spielstand mit einem Passwort für Local Wireless.
- Der Gast sucht im Hauptmenü nach Welten in Reichweite, wählt eine aus und gibt deren Passwort ein.
- Beim ersten Beitritt wartet der Gast gegebenenfalls auf den freien PC, wählt sein Startteam und danach Charakter und Namen.
- Ein neuer Gastname startet an einem festen, freien Punkt in Wurzelheim. Für einen bereits bekannten Gastnamen wird immer seine letzte gespeicherte Position verwendet; es gibt keine Spawn-Auswahl.
- Beide Spieler verwenden ausschließlich Pokémon, Gegenstände und Fortschritte des gemeinsamen Host-Spielstands.
- Nur ein Spieler darf gleichzeitig einen PC verwenden. Der andere kann währenddessen normal weiterspielen.
- Eine neue Mehrspieler-Box enthält pro Spieler eine Reihe für dessen Gruppe von bis zu sechs Pokémon.
- Die Boxenreihe eines eingeloggten Spielers ist gegen Entnahme und Verschieben gesperrt.
- Die aktuelle Gruppe wird laufend in die eigene Boxenreihe gespiegelt und bei Änderungen übertragen. Der Gast beendet seine Sitzung durch Ausschalten. Nach erkanntem Verbindungsende wird seine zuletzt vollständig übernommene Reihe entsperrt; ein Abmeldepaket ist dafür nicht erforderlich. Der Host speichert den gemeinsamen Stand auf der SD-Karte.
- Der Gast kann wilde Pokémon und Trainerkämpfe selbst auslösen.
- Ohne aktive Folgegruppe bestreitet der ausloesende Spieler seinen Kampf allein; der andere spielt unabhaengig weiter.
- A auf einen freien Mitspieler startet das automatische Folgen; B loest es. Nur ein aktiver Anfuehrer mit verfuegbarem Folgespieler startet gemeinsame Kaempfe. Normale Einzel-Trainer werden dabei zu Doppelkaempfen mit passenden Ergaenzungen bis sechs Gegnern.
- In gemeinsamen Kaempfen nutzt jeder nur seine ersten drei Pokemon. Der Partner kaempft nach drei KO alleine weiter; Niederlage erst bei beiden Dreierteams. Bei Story mit NPC-Partner steuert der Folgespieler dessen Pokemon, sein eigenes Team bleibt unveraendert.
- Nur der Host schreibt den gemeinsamen Spielstand. Änderungen werden vollständig und in konsistenter Reihenfolge übernommen.

## Sprint 0: Projekt einrichten und Ausgangsbasis prüfen

**Status: Fertig**

### Erledigte Aufgaben

- [x] Originalprojekt `ZallaxDev/emerald-3ds` geklont.
- [x] Installations-, Builder- und deutsche ROM-Dokumentation geprüft.
- [x] Bereitgestellte ROM als unterstützte deutsche Originalversion `BPED` verifiziert.
- [x] ROM-SHA-1 `61c2eb2b380b1a75f0c94b767a2d4c26cd7ce4e3` bestätigt.
- [x] Windows-Builder und Payload der Release `v0.5.0` heruntergeladen und per SHA-256 geprüft.
- [x] Deutsches Datenpaket mit 6.814 Dateien und 419 Begleiter-Pokémon-Sprites erzeugt.
- [x] Datenpaket mit dem offiziellen Builder erfolgreich verifiziert: Release `v0.5.0`, ABI `734df4ff`.
- [x] Deutsche 3DSX mit der veröffentlichten Release-Prüfsumme abgeglichen.
- [x] Installierbares SD-Paket `Emerald3DS-v0.5.0-de-SD.zip` erzeugt und seine Archivintegrität geprüft.
- [x] Optionalen HOME-Menü-Forwarder aus der offiziellen Release heruntergeladen und per SHA-256 geprüft.
- [x] GitHub-Fork `Sumaiter/emerald-3ds` erstellt.
- [x] Bestehende `.gitignore`-Regeln für ROMs, Saves, CIA-Dateien und Datenpakete geprüft.

### Akzeptanzkriterien

- [x] Die lokale Ausgangsbasis kann mit der unterstützten deutschen ROM reproduziert werden.
- [x] ROM und erzeugte Spieldaten befinden sich nicht in der Git-Versionshistorie.
- [x] Der Fork enthält ausschließlich den Stand des öffentlichen Originalprojekts.

### Noch nicht durchgeführt

- Kein Koop-Quellcode wurde verändert oder implementiert.
- Die Installation wurde nicht auf einem realen 3DS gestartet.
- Local-Wireless-Mehrspielerfunktionen wurden noch nicht mit zwei Geräten getestet.

## Sprint 1: Technischen Koop-Prototyp nachweisen

**Status: Fertig**

### Aufgaben

- [x] Build-, Test- und Debugging-Umgebung für zwei Spielinstanzen eingerichtet. Die lokale MSYS2-/devkitPro-Toolchain und zwei Azahar-Instanzen mit getrennten SD-Umgebungen sind einsatzbereit.
- [x] Bestehende UDS-Sitzungen und den Wireless-Adapter als wiederverwendbare Grundlage abgegrenzt.
- [x] Eine eigenständige Koop-Sitzung außerhalb der bisherigen Link-Theken gestartet. `LOCAL_KIND_COOP`, Zwei-Spieler-Netzwerk und Start/Stopp über `LOCAL CO-OP` sind laufzeitgeprüft.
- [x] Host- und Gastrollen sowie Host-autoritative Nachrichten für den Prototyp festgelegt.
- [x] Kartenkennung, Position, Blickrichtung und Bewegungsanimation eines zweiten Spielers übertragen. Paketformat, Validierung, Heartbeat und Bewegungsentscheidungen sind durch `verify-coop` geprüft.
- [x] Eine ferne Testfigur auf derselben Karte erzeugt, aktualisiert und beim Trennen entfernt. ObjectEvent-Integration, Warp-, Menü- und Disconnect-Verhalten wurden vom Nutzer bestätigt.
- [x] Sichergestellt, dass der normale Einzelspielermodus ohne geöffnete Koop-Welt unverändert bleibt. Der Koop-Pfad ist opt-in und wurde vom Nutzer im Einzelspielermodus bestätigt.

### Bereits ausgeführte Prüfungen

- Vollständiger deutscher ARM-Build einschließlich Bootstrap, Lokalisierung und Voxel-Daten: bestanden; `build/upstream/3ds_port/emerald3ds.3dsx` wurde erzeugt.
- Vollständige englische Baseline mit `make verify`: bestanden; Locale-Tests anschließend separat erneut bestanden.
- Installierbares deutsches Release mit `make release pak`: bestanden; 3DSX, SMDH und Datenpaket liegen unter `Koop-Prototyp/3ds/emerald3ds`.
- Titelmenü-Erweiterung und Optionsanzeige mit `make release pak`: bestanden; `SPIEL SUCHEN` und `LOKALER KOOP` sind enthalten. Beide Azahar-Instanzen starten mit `RomFS boot marker PASS`.
- Datenpaket direkt per CRC geprüft: 6.393 Dateien, ABI `4f8eefe4`, alle Prüfsummen korrekt. Der offizielle 0.5.0-Builder wird nicht zur Verifikation dieses veränderten Entwicklungsstands verwendet.
- SPIEL SUCHEN ist im Titelmenü mit und ohne Spielstand vorhanden. Der Suchbildschirm zeigt SUCHE oben, ZURÜCK darunter und bis zu drei gleich große Ergebnisblöcke mit dem Spielernamen des Hosts. Hoch/Runter wählt ZURÜCK oder einen Host aus; A bestätigt die gezielte Sitzungs-ID, B bricht ab. LOKALER KOOP zeigt AUS oder AN. Suche, Hostname und gezielter Netzwerkbeitritt wurden vom Nutzer mit zwei Azahar-Instanzen bestätigt.
- MULTIPLAYEROPTIONEN ist mit und ohne Spielstand unter OPTIONEN erreichbar; mit Spielstand scrollt der fünfte Eintrag. Host- und Gast-Untermenüs enthalten 4-Tasten-Passwörter, Charakterauswahl und Gastname. Das Profil wird versioniert und atomar auf der SD-Karte gespeichert.
- Die Charakterauswahl zeigt 84 humanoide Overworld-Charaktere als 2D-Vorderansichten in einem 8x2-Raster mit 16 Figuren pro Box. Die originale PC-Hand markiert die Auswahl; nur die ausgewählte Figur ist animiert, und ihr Name steht in der Kopfleiste. Form, Animation, Handcursor und Menüdarstellung wurden vom Nutzer bestätigt. Wasser-/Link-Sonderfiguren wurden entfernt. Die 15 ausgeschlossenen Figuren ohne echte Laufbewegung sind samt Prüfnachweis in `ZEROWALK.md` dokumentiert. Das Avatar-Protokoll überträgt eine validierte Grafik-ID anstelle des Geschlechts. Die Passwortprüfung erfolgt zusätzlich zur UDS-Passphrase durch einen eigenen Authentifizierungsaustausch im Koop-Protokoll; erst nach Host-Bestätigung erscheint VERBUNDEN und Bewegungspakete werden akzeptiert. Beide Netzwerkpunkte sind laufzeitgeprüft.
- Der Gast wechselt nach der Authentifizierung automatisch in die Karte. Sein lokaler Spielstand bleibt dabei unverändert; der SHA-256 vor und nach dem Beitritt ist identisch. Speichern, Link-Speichern, Sondersektoren und Löschen sind für eine aktive Gast-Sitzung gesperrt.
- Cheatähnliche Spielregeln und Shiny-Rate sind nur für den Host veränderbar. Bereits aktivierte Werte werden beim Gast während der Sitzung ignoriert; die Sperre wurde vom Nutzer bestätigt.
- Beim Verlassen oder Neuladen des Gastes wird dessen Figur beim Host entfernt. Die Disconnect-Korrektur wurde vom Nutzer bestätigt.
- Separate ARM-Kompilierung der neuen Koop- und Optionsdateien mit den Produktionsflags: bestanden.
- `make -C 3ds_port verify-coop`: Protokoll- und Profiltests bestanden, einschließlich Persistenz, Prüfsummenfehler und ungültiger Passwortwerte.
- `python -m unittest discover -s builder/tests -v`: 241 Tests bestanden, sechs Umgebungs-abhängige Auslassungen.
- `python tools/release_audit.py --repo . --strict`: bestanden und frei von ROM-/Spieldaten-Fundstellen.
- `0083-local-coop.patch` gegen eine separate Baseline mit allen 83 vorhandenen Patches: erfolgreich per `git apply --check` geprüft.
- `git diff --check`: ohne Fehler.

### Akzeptanzkriterien

- [x] Zwei Instanzen verbinden sich ohne Link-Theke miteinander.
- [x] Die Bewegung des Hosts wird auf der Gastkonsole als zweite Figur sichtbar.
- [x] Ein Verbindungsabbruch hinterlässt keine blockierte Figur oder Sitzung.
- [x] Der Build und die automatisierten Tests bestehen.

## Sprint 2: Local-Wireless-Lobby und Passwort-Beitritt

**Status: Fertig**

### Aufgaben

- [x] Menüpunkt „Welt für Mehrspieler öffnen“ für den Host einbauen. Umgesetzt als LOKALER KOOP mit sichtbarem AUS-/AN-Zustand.
- [x] Passwortvergabe und passwortgeschützte Beitrittsprüfung umgesetzt. Ein falsches Passwort führt nicht in die Karte und kann danach erneut eingegeben werden; das richtige verbindet den Gast.
- [x] Menüpunkt „Spiel suchen“ im Hauptmenü des Gastes einbauen. Mit und ohne eigenen Spielstand auf zwei Instanzen geprüft.
- [x] Gefundene Welten mit ausreichenden Host- und Versionsinformationen anzeigen. Hostname und zentrale Mod-Version `V0.1.0` wurden in der Suche bestätigt.
- [x] Inkompatible Builds, Sprachen und Datenpakete durch den Build-ID-Vergleich ablehnen und die Sitzung auf Host plus einen Gast begrenzen. Die Laufzeitabnahme mit einer dritten Konsole ist ein späterer Zusatztest außerhalb dieses Sprints.
- [x] Spielstandsdaten erst nach erfolgreicher Authentifizierung freigeben. Der Kartenwechsel erfolgt erst nach der Host-Bestätigung, und der lokale Gast-Spielstand bleibt bytegenau unverändert.
- [x] Suchen und Beitreten ohne Neustart ermöglichen. Ein separates Abbrechen und erneutes Verbinden innerhalb derselben Sitzung ist für diesen Prototyp nicht erforderlich.

### Akzeptanzkriterien

- [x] Ein Gast findet eine geöffnete Welt über das Hauptmenü.
- [x] Ein falsches Passwort gewährt keinen Zugriff auf Welt oder Spielstand.
- [x] Das richtige Passwort verbindet genau einen zweiten Spieler.

### Spätere Zusatztests außerhalb dieses Sprints

- [ ] Beitritt einer dritten Konsole zu einer bereits belegten Welt verweigern.
- [ ] Die bisherigen Link-Theken weiterhin ohne Beeinträchtigung nutzen.

## Sprint 3: Gemeinsame Oberwelt und zweiten Spieler darstellen

**Status: Fertig**

Zwei isolierte Azahar-Instanzen bestätigen wiederholte Beitritte, den vollständigen Host-Snapshot, neue und gespeicherte Spawnpunkte, Bewegung, Kartenwechsel, gemeinsame und getrennte Karten sowie nicht blockierende Avatare. NPC-Position und Blickrichtung wurden direkt verglichen. Wiederholte Beutel-Auf-/Zu-Zyklen und die Bereinigung nach Verbindungsabbruch sowie nach dem Laden eines gespeicherten Avatars wurden geprüft. Der alte Kartenausschnitt wird beim Join verworfen. Die lokale Gast-SAV bleibt unverändert. Laufende Gruppen- und PC-Synchronisierung gehören zu Sprint 4, Kämpfe zu Sprint 5.

### Aufgaben

- [x] Zweite Spielfigur mit eigenem Charakter und Namen sowie Basis-Bewegungsdaten integrieren.
- [x] Bewegungsdaten mit Heartbeat, Validierung und Verlustbehandlung übertragen; dies wurde in Sprint 1 bestätigt.
- [x] Namensbasierte Spawnpositionen zuverlässig laden und nach dem Beitritt weiter synchronisieren; einschließlich Host-Snapshot und Rejoin bestätigt.
- [x] Kartenwechsel durch Türen und unterschiedliche Aufenthaltsorte beider Spieler synchronisieren; einschließlich Rejoin bestätigt.
- [x] Neue Gastnamen an einem freien, festen Punkt in Wurzelheim spawnen und für alte Namen immer deren gespeicherte Position ohne Spawn-Auswahl verwenden.
- [x] Kollisionen und Interaktionen mit der zweiten Figur festlegen und umsetzen: `LOCALID_COOP_PLAYER` blockiert keine Bewegung und besitzt kein NPC-Script; vom Nutzer laufzeitgeprüft.
- [x] Benötigte Host-Karten-, NPC- und Weltzustände für den Gast als initialen Snapshot bereitstellen; der Snapshot enthält aktive ObjectEvents und NPC-Positionen werden auch danach Host-autoritativ aktualisiert. Position und Blickrichtung sind im Laufzeitvergleich bestätigt. Team-, Item- und Storyänderungen nach dem Beitritt gehören zu Sprint 4.
- [x] Menüs und Kampf-/Oberweltzustände pro Spieler über getrennte Engine-Callbacks getrennt halten; unabhängige Menüs wurden vom Nutzer laufzeitgeprüft.

### Akzeptanzkriterien

- [x] Beide Spieler können gleichzeitig und unabhängig auf derselben Karte laufen.
- [x] Beide Spieler können sich auf unterschiedlichen Karten aufhalten und später wieder zusammenfinden.
- [x] Spawnpunkte befinden sich nicht in Wänden, blockierten Objekten oder ungültigen Kartenbereichen.
- [x] Kartenwechsel und Trennungen erzeugen keine dauerhaften Geisterfiguren oder Desynchronisationen.

## Sprint 4: Spielerprofil, Mehrspieler-Box und PC-Sperre

**Status: Fertig**

Abgenommen auf Branch `sprint-4` mit zwei isolierten Azahar-Instanzen und neun automatisierten Testsuiten. Fünf gültige Test-Pokémon wurden über Entnahme, Ablage, ausgeschalteten Gast, freigegebene Gast-Reihe, Host-Speichern, vollständigen Neustart und Wiederbeitritt anhand ihrer eindeutigen Kennungen verfolgt. Jedes blieb genau einmal im gemeinsamen Bestand vorhanden. Die aus der Gast-Reihe entnommenen Pokémon erschienen beim Wiederbeitritt nicht erneut. Hardwaretests gehören weiterhin zu Sprint 8.

### Aufgaben

- [x] Zweites Spielerprofil mit Charakter, Name und zugeordneter Boxenreihe im Host-Spielstand speichern; Spawn-Tabelle und Gast-Reihe werden verwendet.
- [x] Mehrspieler-Box mit einer Reihe und bis zu sechs Pokémon pro Spieler einbauen: Box 14 als KOOP-Box, Reihe 1 für den Host und Reihe 2 für den Gast; Bestand wird vorher migriert.
- [x] Neue Gäste zwischen Geckarbor, Flemmli und Hydropi wählen lassen und den Starter direkt in die Gast-Reihe legen; die Auswahl ist vor dem Startteam-PC verbindlich und darf nicht dupliziert werden. Hoch/Runter und die Bestätigung von Flemmli wurden im Emulator abgenommen; anschließend öffnete der Startteam-PC.
- [x] Startteam-Auswahl nach erfolgreichem Beitritt öffnen, sobald der PC frei ist; mindestens ein Pokémon und vollständige Host-Bestätigung vor dem Spawn; mit zwei Instanzen bestätigt.
- [x] Globale PC-Sperre für jeweils genau einen Spieler umsetzen. Sie bleibt beim Wechsel zwischen Box und PC-Auswahlmenü bestehen; direkte Menüausstiege, gegenseitige Zugriffsversuche und Disconnect sind laufzeitgeprüft. Ein Beitritt wartet auf das Ende eines offenen Host-PC-Besuchs.
- [x] PC-Verschiebungen als vollständige, unteilbare Vorgänge mit Prüfsumme ausführen. Gast-Entnahme und Host-Ablage wurden auf beiden Seiten geprüft. Ein hart beendeter Gast-PC-Besuch erhält den vorher bestätigten Host-Bestand. Native Tests prüfen zusätzlich Teilübertragungen und falsche Prüfsummen.
- [x] Boxenreihen eingeloggter Spieler gegen Entnahme, Verschieben, Tausch, Freilassen und Mehrfachverschieben sperren. Die Eingabe wird vor dem Start einer Cursor-Operation abgewehrt; normale und automatische Auswahl wurden mit beiden aktiven Reihen geprüft. Nach Gast-Abbruch ist dessen Reihe wieder verwendbar; die eigene Host-Spiegelung bleibt gesperrt.
- [x] Die aktuelle Gruppe laufend in der eigenen Reihe spiegeln; geänderte Daten vollständig mit Prüfsumme übertragen und bestätigen. Beim Ausschalten des Gasts bleibt die zuletzt übernommene Gruppe beim Host und wird nach dem Disconnect entsperrt. EP, AP und getragenes Item wurden mit zwei Instanzen sowie nach Host-Speichern und Neuladen geprüft. Unterbrochene Übertragungen erhalten die bisherige Reihe.
- [x] Abbrüche während der Startteam-Auswahl ohne teilweise übernommene Änderungen behandeln; Übernahme erst nach vollständigem Empfang und korrekter Prüfsumme.
- [x] Speicherformat beziehungsweise Erweiterung und Kompatibilität mit vorhandenen Spielständen dokumentieren.

### Akzeptanzkriterien

- [x] Zwei Spieler können nicht gleichzeitig denselben PC verwenden; gegenseitige Versuche sind im Emulator und gleichzeitige Protokollanfragen nativ geprüft.
- [x] Die geprüften PC- und Anmeldevorgänge duplizieren oder verlieren keine Pokémon; der Bestand bleibt über bestätigte Vorgänge, Abbrüche und Wiederbeitritt erhalten.
- [x] Pokémon eines eingeloggten Spielers können nicht von einem anderen Spieler entnommen werden; beide aktiven Reihen wurden geprüft.
- [x] Nach dem Ausschalten des Gasts bleibt die zuletzt vollständig synchronisierte Gruppe in der richtigen Boxenreihe; im Emulator geprüft.
- [x] Nach einem vollständigen Neustart und Wiederbeitritt ist der gemeinsame Spielstand konsistent. Lücken in der Gast-Reihe werden beim Laden zu einer gültigen Gruppe zusammengeschoben.

## Sprint 5: Unabhängige Einzelkämpfe und gemeinsame Trainerwelt

**Status: In Arbeit. Standardablaeufe vom Nutzer im Emulator bestanden; Gras-/Zuschauer-Nachtest, einzelne Grenzfaelle und Hardware-Abnahme offen.**

### Aufgaben

- [x] Wilde Begegnungen und Trainerkämpfe auch durch den Gast auslösbar machen.
- [x] Einzelkämpfe mit der Gruppe des auslösenden Spielers durchführen.
- [x] Oberwelt auf einer Konsole und Kampf auf der anderen gleichzeitig betreiben.
- [x] Trainer bereits waehrend Dialog und Kampf reservieren; erneuten Einstieg nach gemeinsamem Szenarioabschluss verhindern (Nutzertest bestanden).
- [ ] Einmalige feste Wildgegner waehrend eines ausgeloesten Kampfes reservieren.
- [x] Kampfsiege, besiegte Trainer und andere Weltänderungen Host-autoritativ übernehmen.
- [x] EP, Level, Fangversuche und Teamveränderungen der richtigen Spielergruppe zuordnen.
- [x] Niederlagen, Flucht und Rückkehr zur Oberwelt für beide Spielerrollen behandeln.
- [x] Doppelte Belohnungen und mehrfaches Auslösen desselben Gegners verhindern.


Urspruenglich integrierte Quelle: `1be2515` (Implementierung `dbb9ff0` plus Sprint-5-Tests).
Der aktuelle Stand nutzt Protokoll18 und behaelt die Grafik-, Gastnamen-,
Beitritts- und PC-Meldungskorrekturen. Trainer- und Szenariosperren greifen
bereits vor dem Kampfeinstieg; abgeschlossene Szenen werden gemeinsam
uebernommen. Ergebnisse sind sequenziert und werden ohne doppelte Belohnungen
angewendet. Gemeinsame Gegenstaende und Geld werden waehrend des Spiels
synchronisiert; am Kampfende werden diese Liveaenderungen nicht erneut addiert.
Getrennte Spielergruppen und zwei unabhaengige Kaempfe bleiben moeglich.
Neu hinzu kommen Schritt-Graseffekte fuer entfernte Spieler und lokale
Zuschauerpraesentation. Buildkennungen und Nachtestgrenzen in LOKAL_TESTEN.md.

Automatisch geprueft: Reservierung/Wiederholung, getrennte Spielerreihen,
verschluesselte Geld-/Itemmengen nach Save-Rekey, wiederholte/alte Ergebnisse,
Kollisionen mehrerer Faenge, volle Boxen ohne Teiluebernahme, Inventarfehler,
Transferfortsetzung sowie Kampf-/PC-Rennen. Statische einmalige Wildgegner
haben im uebernommenen Branch noch keine eigene Gegnerreservierung; dieser
Teil der Aufgabe bleibt offen. Die Standardabnahme unten beruht auf der
Rueckmeldung des Nutzers vom 2026-11-14. Die aktuelle Gras-/Zuschauer-Fixwelle
wartet auf ihren gezielten Nachtest; Einzelheiten und weitere Grenzfaelle
stehen in `LOKAL_TESTEN.md`.

### Akzeptanzkriterien

- [x] Beide Spieler können unabhängig voneinander Einzelkämpfe auslösen und abschließen.
- [x] Ein Spieler kann weiterlaufen oder selbst kämpfen, während der andere im Kampf ist.
- [x] Derselbe Trainer kann nicht gleichzeitig zweimal herausgefordert werden.
- [x] Kampfergebnisse sind nach einem Speichern und Neuladen auf beiden Seiten konsistent.

## Sprint 6: Folgegruppe und gemeinsame Doppelkaempfe

**Status: Nutzerabnahme am 2026-11-15 erfolgt, mit separat offenen Issues #1..4 und #6..8. EP integriert; regulaeres Trainerpreisgeld bleibt Implementierungsluecke.**

### Aufgaben

- [x] Explizites Folgen mit A und Aufloesen mit B statt automatischem Naehe-Beitritt (Hintergrundtests).
- [x] Gruppenberechtigung, Karte, Naehe und aktuelle Verfuegbarkeit vor gemeinsamem Einstieg pruefen; Abbruch und Teamhash absichern.
- [x] Nur erste drei Pokemon pro Spieler; getrennte Ersatzpools, Reserve unveraendert, Partner spielt nach drei KO weiter (Hintergrundtests).
- [x] Normale Einzel-Trainer fuer Folgegruppen in Doppelkaempfe umwandeln; verschiedene passende Ergaenzungen, maximal sechs.
- [x] Bestehende Story-Gegnerteams erhalten; Folgespieler uebernimmt das NPC-Partnerteam, ohne eigene Pokemon zu verlieren (Hintergrundtests).
- [x] Ohne Gruppe/verfuegbaren Folgespieler auf den Original-Solokampf zurueckfallen.
- [ ] Eingaben, Attackenziele, Wechsel, KO, Fang/Flucht und Kampfende auf beiden Emulatoren abnehmen.
- [ ] EP/EV/Levelup/Attackenlernen und regulaeres Preisgeld fuer gemeinsame Kaempfe integrieren.

### Akzeptanzkriterien

- [ ] A/B und automatisches Nachlaufen funktionieren mit beiden Rollen einschliesslich Kartenwechsel.
- [ ] Nur Folgegruppen kaempfen gemeinsam; beide steuern ihre eigenen ersten drei.
- [ ] Ein erschoepfter Spieler kann den Partner weiterkaempfen lassen, ohne fremde/reservierte Pokemon zu benutzen.
- [ ] Einzel-Trainer erhalten passende verschiedene Ergaenzungen bis sechs; vorhandene Storyteams bleiben erhalten.
- [ ] Der Folgespieler steuert den Story-NPC-Partner; sein eigenes Team bleibt nach Sieg/Niederlage unveraendert.
- [ ] Nach Kampf und Save/Rejoin stimmen Team- und Weltfortschritt, ohne doppelte Belohnungen.

## Sprint 7: Abmeldung, Verbindungsabbrüche und sicheres Speichern

**Status: In Main integriert und vom Nutzer abgenommen (2026-11-15); bekannte Issues bleiben separat offen.**

### Aufgaben

- [x] Ausschalten des Gasts ohne Abmeldepaket unterstützen: laufende Gruppenübertragung und Erhalt der zuletzt vollständig übernommenen Reihe; mit zwei Emulatoren geprüft.
- [ ] Unerwartete Verbindungsabbrüche erkennen und nach einer definierten Zeitüberschreitung bereinigen.
- [ ] Verhalten bei Abbrüchen während Einzel- und Doppelkämpfen festlegen und umsetzen.
- [ ] Beim Schließen der Host-Welt alle Spieler und Gruppen sicher zurück in die Mehrspieler-Box überführen.
- [ ] PC- und Anmeldesperren bei Abbrüchen zuverlässig freigeben.
- [ ] Nur konsistente Zustände speichern und unterbrochene Vorgänge verwerfen oder vollständig wiederherstellen.
- [ ] Wiederbeitritt nach Abbruch ohne doppelte Figuren, Gruppen oder Belohnungen ermöglichen.

### Akzeptanzkriterien

- [ ] Kein getesteter Abbruch führt zu verlorenen oder duplizierten Pokémon.
- [ ] Kein getesteter Abbruch hinterlässt eine dauerhafte PC-, Trainer- oder Spielersperre.
- [ ] Der Host-Spielstand lässt sich nach jedem getesteten Abbruch laden und weiterspielen.
- [ ] Beim Beenden der Welt sind alle Gruppen wieder in ihren Boxenreihen.

## Sprint 8: Hardwaretests, Stabilisierung und spielbare Alpha

**Status: In Main integriert; Gesamtverify, ARM-Release und Datenpaket bestanden. Hardware-/Langzeitabnahme offen.**

### Aufgaben

- [x] Automatisierte Tests für Speicherformat, Boxensperren und Koop-Protokoll ergänzen.
- [ ] Zwei-Instanzen-Tests mit Azahar durchführen, soweit die benötigten UDS-Funktionen unterstützt werden.
- [ ] Tests mit zwei realen 3DS-/2DS-Konsolen durchführen.
- [ ] Verhalten auf Old 3DS und New 3DS sowie mit aktivierter Voxel-Darstellung prüfen.
- [ ] Längere Spielsitzungen mit Kämpfen, Kartenwechseln, PC-Zugriffen und Wiederverbindungen testen.
- [ ] Bestehende Union-Room-, Tausch-, Kampf- und Minispielfunktionen auf Regressionen prüfen.
- [ ] Build-, Installations- und Mehrspieleranleitung für den Fork aktualisieren.
- [ ] Alle Sprintstände und verbleibenden Fehler in dieser Datei nachführen.

### Akzeptanzkriterien für die Alpha

- [ ] Alle vorherigen Sprints sind fertig.
- [ ] Keine bekannten Fehler führen zu Datenverlust, Pokémon-Duplikaten oder beschädigten Spielständen.
- [ ] Der vollständige Beitritts-, Spiel-, Abmelde- und Wiederbeitrittsablauf funktioniert auf zwei realen Konsolen.
- [ ] Build und automatisierte Tests bestehen aus einem sauberen Checkout.
- [ ] Veröffentlichte Archive enthalten keine ROMs, Spielstände oder anderen nicht freigegebenen Spieldaten.

## Sprint 9: Vollständiges Solospiel und dauerhaft erreichbare Pokémon

**Status: Offen — nach Sprint 8 geplant**

Das Spiel soll einschließlich seiner vorgesehenen Pokémon und Inhalte auf
einem Gerät allein durchspielbar sein. Ein zweiter Spieler, ein externer
Tausch und historische Event-Verteilungen dürfen dafür keine Voraussetzung
sein. Koop bleibt optional nutzbar. Dieser Sprint ist ein eigener Ausbau;
die laufenden Fehlerkorrekturen aus Sprint 5 werden dadurch nicht ersetzt.

### Aufgaben

- [ ] Alle Entwicklungen und Erwerbswege auf Abhängigkeiten von Tausch,
  anderen Editionen, externen Verteilungen und heute fehlendem Event-Zugang
  prüfen; vollständige Liste der betroffenen Pokémon und Inhalte erstellen.
- [ ] Für reine Tauschentwicklungen regulär erreichbare Solo-Bedingungen
  festlegen und umsetzen; auch Tauschentwicklungen mit getragenem Item abdecken.
- [ ] Ehemalige Event-Pokémon und zugehörige Orte, Schlüsselitems oder
  Freischaltungen durch dauerhaft verfügbare Ingame-Wege erreichbar machen,
  etwa über Aufgaben, NPCs oder Funde mit passendem Storyfortschritt.
- [ ] Verbleibende externe Erwerbshürden für die im Projekt vorgesehenen Arten
  durch nachvollziehbare Solo-Fundorte oder alternative Erwerbswege schließen.
- [ ] Jede VM ab ihrem Erhalt dauerhaft als Feldaktion freischalten. Der
  VM-Besitz genügt; die Attacke muss keinem Pokémon beigebracht werden und
  setzt kein bestimmtes Team-Pokémon voraus. Alle acht VM-Feldaktionen abdecken.
- [ ] Jede Änderung mit ursprünglicher Voraussetzung, neuer Bedingung sowie
  Fundort/Freischaltpunkt dokumentieren und im Spiel verständlich vermitteln.
- [ ] Story- und Nachspiel-Durchlauf ohne Netzwerk prüfen; Entwicklungen,
  einmalige Belohnungen, Speichern/Neuladen und bestehende Spielstände abdecken.
- [ ] Optionale Koop-Nutzung auf Regressionen prüfen, einschließlich gemeinsamem
  Fortschritt und Schutz vor doppelten Event-Belohnungen.

### Akzeptanzkriterien

- [ ] Story und vorgesehene Nachspiel-Inhalte lassen sich vollständig allein
  auf einem Gerät ohne externe Event-Verteilung abschließen.
- [ ] Jede betroffene Tauschentwicklung besitzt einen funktionierenden,
  dokumentierten Solo-Weg, einschließlich der Varianten mit getragenem Item.
- [ ] Jedes zuvor ausschließlich an frühere Events gebundene vorgesehene
  Pokémon und der notwendige Zugang sind regulär im Spiel erreichbar.
- [ ] Jede erhaltene VM lässt sich ohne erlernte Attacke im passenden Feldkontext
  verwenden und bleibt nach Speichern/Neuladen freigeschaltet. Nicht erhaltene
  VMs werden dadurch nicht vorzeitig freigeschaltet.
- [ ] Die Erwerbs-/Entwicklungsliste enthält für jede erfasste externe Hürde
  eine getestete Alternative; keine Art bleibt nur wegen eines nötigen zweiten
  Spielers oder einer vergangenen Verteilung unerreichbar.
- [ ] Entwicklungen, Zugänge und Belohnungen bleiben nach Neuladen korrekt
  erhalten; bestehende Spielstände bleiben spielbar und Koop bleibt optional.

## Noch zu entscheidende Details

- Dürfen Gäste story-kritische Ereignisse auslösen oder nur normale Trainer und wilde Begegnungen?
- Wie werden Flucht, Fangversuche und Kampfabbrüche in gemeinsamen Doppelkämpfen behandelt?
- Soll bei einem Verbindungsabbruch im Doppelkampf eine KI übernehmen oder der Kampf sicher abgebrochen werden?
- Soll ein wiederkehrender Gast jedes Mal ein Startteam wählen oder automatisch seine letzte Boxenreihe übernehmen?
- Welche Interaktionen zwischen den beiden Spielfiguren sind möglich, beispielsweise Ansprechen, Blockieren oder direkter Tausch?

## Aktueller Gesamtstand

Sprint 0 bis Sprint 4 sind abgeschlossen und mit zwei Azahar-Instanzen
geprueft. Die Standardablaeufe aus Sprint 5 sind vom Nutzer als bestanden
gemeldet. Die anschliessenden Gras-/Zuschauer-Aenderungen bestehen die
gemeinsamen Hintergrundtests und den ARM-Release/PAK-Build; ihre gezielte
Emulatorabnahme steht aus. Einmalige feste Wildgegner und vorbereitete
Sonderfaelle bleiben separat offen. Hardwaretests sind weiterhin offen.
Sprint 6 bis 8 bleiben als folgende Ausbaustufen bestehen; Sprint 9 ergaenzt
vollstaendiges Solospiel mit alternativen Entwicklungen und Event-Zugaengen.
HANDOFF.md enthaelt den neuesten Build-/Teststand, SETUP.md die Ersteinrichtung.
Es gibt noch keine vollstaendig abgenommene Alpha.
