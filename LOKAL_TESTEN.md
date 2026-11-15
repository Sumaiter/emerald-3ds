# Host und Gast auf diesem PC starten

## Sprint 7: gezielte Nutzertests bestanden (2026-11-15)

Build 8af20c2c7a809005: Gast normal abmelden und mit erhaltenem Team/KP/AP/Status
wieder beitreten bzw. Host speichern/neuladen erfolgreich, keine Duplikate.
Beide gezielten Nachtests bestaetigt. Die umfangreiche Liste unten enthaelt
auch bereits in Sprint 6 gepruefte Regressionen und ist historisch; sie ist
keine erneute Pflichtabnahme aller Varianten. Offene Issues bleiben separat.

## Sprint 7 testen: Abmelden, Abbrueche und Speichern (2026-11-15)

Build **8af20c2c7a809005**, Boot **E3ID-4fe482705fd68761**, ABI **a8444026**, Protokoll 21.

Sprint 6 ist vom Nutzer abgenommen; bekannte Issues #1..4 und #6..8 bleiben
separat offen, #5 ist geschlossen. Sprint 7 ist aus dem vorbereiteten Branch
auf dem aktuellen Stand integriert. Gesamtverify, ARM-Release und PAK bestanden.
Neuer Stand nutzt Protokoll 21; beide
Instanzen ueber **Host und Gast starten** mit dem neuen Paar neu laden.
F6 installiert keinen Build. Die alten Testlisten weiter unten sind Historie.

Vorher den Host normal speichern. Fuer Vergleiche ein verletztes Gast-Pokemon
mit verbrauchten AP und ggf. Status/Item sowie Team- und Boxbelegung merken.

1. **Gast normal abmelden:** Im freien Feld LOKALER KOOP ausschalten. Nach
   kurzer finaler Uebertragung landet der Gast im Hauptmenue. Host bleibt
   bedienbar, Gastfigur verschwindet, dessen Boxenreihe ist wieder verfuegbar.
   HP/AP/Status/Items bleiben erhalten; kein Heilen oder doppeltes Pokemon.
2. **Gast hart trennen:** Gast-Emulator schliessen bzw. F6 beim Gast. In der
   Oberwelt und waehrend eigenem Gast-Dialog wiederholen. Host bleibt
   spielbar; Trainer-/Dialog-/PC-Sperren werden nach Verbindungserkennung
   freigegeben. Erhalten bleiben nur vollstaendig uebernommene Aenderungen.
3. **Host hart trennen:** Host F6, Welt ausschalten oder Host schliessen.
   Gast muss ins Hauptmenue, ohne Freeze oder Speicherung der Hostwelt in
   seinen lokalen Spielstand. Je einmal Gast im Feld, Dialog, Beutel/Team,
   PC und Kampf pruefen. Danach neu beitreten koennen.
4. **Abbruch im Kampf:** Gast trennen, waehrend der Host einen eigenen
   unabhaengigen Einzelkampf fuehrt: dieser darf weitergehen. Danach einen
   gemeinsamen wilden/Trainer-Doppelkampf und Troy-Leihteamkampf durch
   Gastabbruch beenden: Host muss sauber zurueckkehren. Echte Teams samt
   Reserven muessen erhalten sein, keine Leih-Pokemon im eigenen Team oder
   PC. Das Einzelspieler-K.-o.-Verhalten aus Issue #7 bleibt separat offen.
5. **Abbruch am PC:** Gast beim Verschieben eines Pokemon und dann nach
   bestaetigtem Verschieben trennen. Keine verlorenen/duplizierten Pokemon;
   nur vollstaendige PC-Aenderungen duerfen uebernommen sein. Auch testen,
   waehrend der HOST den PC geoeffnet hat: dessen laufende Bedienung darf
   nicht abgebrochen werden. Nach Schliessen sind die Sperren wieder frei.
6. **Aufgeschobenes Autosave:** Gast trennen, waehrend Host im Kampf, Dialog
   oder PC ist. Host erst normal daraus zurueckkehren lassen. Anschliessend
   Host komplett neu starten/laden: Team, Boxen, KP/AP/Status, gemeinsame
   Items/Geld und bestaetigter Fortschritt stimmen. Kein Speichern mitten
   in einer unfertigen Transaktion. Vor dem Neuladen kurz im freien Feld warten.
7. **Wiederbeitritt:** Nach normaler und harter Trennung mit demselben
   Gastprofil erneut beitreten. Richtige Pokemon/Position, keine doppelte
   Figur, keine alte PC-/Trainer-/Folgesperre oder doppelte Belohnung. Danach
   PC benutzen und einen weiteren Kampf starten. Zwei bis drei Zyklen pruefen.

Noch nicht manuell abgenommen. Bei Fehler den Punkt, Host-/Gastkontext und
das Verhalten beider Bildschirme nennen; Logs vor einem weiteren Neustart sichern.

## Sprint 6 vom Nutzer abgenommen (2026-11-15)

Rueckmeldung: "ansonsten klappt jetzt alles von sprint 6" fuer Build
fb72074e4a979dff. Die bisherigen KP-/EP-/Trainerflucht-/Reentry-Nachtests
gelten damit als bestanden. Bekannte Issues #1..4 und #6..8 bleiben separat
offen; #5 ist bestaetigt behoben. Untenstehende Resttestlisten sind Historie.
Nicht genannte Spezialfaelle bleiben ohne separate Abnahme; regulaeres
Trainerpreisgeld ist noch eine dokumentierte Implementierungsluecke.

## Aktueller Nachtest: beide KP-Boxen und Trainer-Fluchtsperre (2026-11-15)

Build **fb72074e4a979dff**, Boot **E3ID-1de947bbc289d035**, ABI **52daf884**, Protokoll 20.

Gesamtverify, ARM-Release und passendes PAK fertig. Ueber **Host und Gast
starten** beide Instanzen mit dem neuen Paar neu laden; F6 installiert es nicht.

1. TEST A mit eigenen Pokemon frisch beginnen, Host und danach Gast als
   Anfuehrer. OHNE vorher Team/Beutel zu oeffnen muessen Gewaldro UND Lohgock
   ihre KP-Box haben. Der Aufbau erfolgt nun bei der ersten Aktionsauswahl
   einmalig ueber den nach Menue-Rueckkehr funktionierenden nativen Pfad.
   Auch Gast/Host, wilder Doppel- und Troy-Kampf sowie erneuter Kampf pruefen.
2. Im Trainerkampf zuerst Host, dann Gast FLUCHT waehlen: normale Ablehnung,
   Kampf bleibt offen, kein Flucht-/Aufgabe-/Niederlageabschluss oder WhiteOut.
   Auch mit Rauchball/Angsthase nicht fliehen. Anschliessend normal angreifen.
3. Treffer/Faint/Wechsel und Menue-Rueckkehr gegenpruefen: beide lebenden
   Pokemon behalten die richtige Anzeige; ein besiegtes wird normal ausgeblendet.
4. Wildflucht bleibt erlaubt: normale Fluchtchance/Sperren, bei erfolgreicher
   Flucht wieder im Gras, keine Heilung/Geldstrafe. Das ist keine Trainerflucht.

Die Rueckmeldung zum vorherigen Build ist dokumentiert: Box fehlt anfangs,
erscheint nach Menue-Rueckkehr. Die neue KP-Rekonstruktion ist nativ geprueft,
der urspruengliche konkrete Stoerschritt jedoch noch nicht isoliert; sichtbare
Abnahme offen. Einmalige Logzeilen "coop: intro HP" helfen bei Restfehlern.

Storybarriere am Weg nach Route 102 separat als [Issue #6](https://github.com/Sumaiter/emerald-3ds/issues/6)
erfasst. Noch nicht behoben; soll fuer alle entsprechenden Storysperren gelten.
Vorherige Nutzerabnahmen bleiben dokumentiert, neue EP-/Troy-Reentry-Nachtests
aus dem vorherigen Abschnitt gelten weiterhin, soweit noch nicht bestaetigt.
Nachfolgende Anleitungen und Buildkennungen sind historische Staende.

## Aktueller Nachtest: KP, EP und erneuter Troy-Einstieg (2026-11-15)

Build **56417c3a94f30a61**, Boot **E3ID-86d537e6d7603887**, ABI **e8ef3639**, Protokoll 20.

Gesamtverify, ARM-Release und passendes PAK sind fertig. Beide Instanzen ueber
**Host und Gast starten** neu laden; F6 installiert keinen neuen Build.
Die laufenden Emulatoren und ihre SAVs wurden fuer diese Aenderung nicht bedient.

1. Eigenen Koop-Doppelkampf starten, zuerst Host und dann Gast als Anfuehrer:
   Direkt nach dem Aussenden muessen BEIDE eigenen KP-Boxen sowie beide
   Gegnerboxen erscheinen. Treffer, Beutel/Team und Wechsel kontrollieren.
   Die neue native Wiederherstellungsprobe besteht; der urspruengliche
   Ausloeser der fehlenden Box ist noch nicht aus dem alten Lauf isoliert.
2. Einen Gegner besiegen: individuelle EP-Meldungen fuer beide echten Owner
   kontrollieren. Mit Pokemon nahe am naechsten Level auch Levelaufstieg,
   Attacken ersetzen/ablehnen jeweils auf dem Geraet des Besitzers pruefen.
   Danach stimmen EP/Level/Attacken/AP auch nach erneutem Kampf und Rejoin.
   Zahlen-Statistikseiten beim Levelaufstieg entfallen im Koop; die normalen
   Levelmeldungen/Animationen und Besitzer-Attackenwahl bleiben aktiv.
3. Sieg -> eigenen Nachkampfdialog mit B schliessen -> Anfuehrer weiterlaufen:
   Gast bleibt in der Gruppe. Sofort TEST B/Troy ansprechen, bis zur bedienbaren
   Runde und anschliessend erneut spielen. Mit beiden Anfuehrerrollen pruefen.
   Geliehene NPC-Pokemon erhalten keine EP; das eigene urspruengliche Team und
   Reserven muessen nach Rueckkehr erhalten bleiben.
4. Bei Gelegenheit Entwicklung pruefen (Besitzer kann abbrechen). Nincada
   auf Level 20: mit freiem echten Teamplatz Ninjatom, bei sechs Pokemon keines.
   Dies ist nativ automatisiert inklusive Troy-Auswahl/Reserven geprueft.

Vorher vom Nutzer bestanden: Wild-/andere Doppelkaempfe, besiegte Teams,
Troys grundlegender Leihteamkampf, Speichern/Wiederbeitritt und Disconnect.
Issue #5 (Trainerbild im Wildkampf) bestaetigt behoben. Die neue Sieg-/Reentry-
Folge oben braucht eigene Abnahme. Noch nicht einzeln bestaetigte Wildflucht,
Fangzuordnung und 1/2/6-Teamgrenzen aus dem vorherigen Abschnitt bleiben offen.
Regulaeres Trainerpreisgeld ist separate Implementierungsarbeit.
Nachlaufen #3, Touch-Zielauswahl #4, Gras #1 und Zuschauen #2 bleiben offen.
Alle nachfolgenden Anleitungen und Buildkennungen sind historische Staende.

## Aktuelle Resttests fuer Sprint 6 (2026-11-15)

Build **7cd1b68323ee18b7**, Boot **E3ID-6ff528623f015338**, ABI **5becd2e8**, Protokoll 20.

Hintergrundtests, ARM-Release und PAK sind fertig. Beide Instanzen ueber
**Host und Gast starten** neu laden; F6 installiert keinen neuen Build.

Vom Nutzer bestaetigt bestanden: Wild-/andere Doppelkaempfe, besiegte Teams,
Troys Leihteam, Speichern/Wiederbeitritt und Verbindungsabbruch. Issue #5
(Trainerbild im Wildkampf) ist manuell als behoben bestaetigt.

Neue Wildflucht-Korrektur (0106) braucht noch sichtbare Abnahme:

1. Gemeinsam im Gras kaempfen; erst Host, dann Gast fluechten lassen.
   Erfolgreiche Flucht muss beide an ihre bisherige Position zurueckbringen.
   HP/AP/Status bleiben wie vor der Flucht, keine Heilung oder Geldstrafe.
   Auch nach einem gescheiterten Fluchtversuch und mit nur einem wilden
   Gegner pruefen; danach erneut laufen/kaempfen koennen.
2. Teamgrenzen mit 1, 2 und 6 Pokemon: nur erste drei pro Spieler waehlbar,
   ausschliesslich eigene wechseln; Plaetze 4..6 nach Kampf unveraendert.
   Dafuer liegen in gemeinsamem PC Box 1, Plaetze 1..12, je zweimal
   Gewaldro/Lohgock/Sumpex/Guardevoir/Voltenso/Stolloss auf Level 50.
   Je sechs entnehmen, Reserven 4..6 unterscheidbar lassen und vor/nach
   dem Kampf HP/AP/Status/Items vergleichen. Kein bestehendes Pokemon ersetzt.
3. Fang bei einem verbliebenen Gegner: Zuordnung zu Team/Box, keine Duplikate.
4. Falls noch nicht mit beiden Rollen erfolgt: ohne Folgen solo, mit Folgen
   gemeinsam; B trennt. TEST A/B allein mit nur einem kampffaehigen Pokemon;
   B waehrend Gruppenbeitritt darf keinen Wartezustand hinterlassen.

Besiegte Teams, Leihteam, Speichern/Rejoin und Disconnect nicht pauschal
erneut als offen behandeln. Die explizite Rueckmeldung ersetzt keine
Aussage zu ungenannten Einzelvarianten.

Touch-Zielauswahl #4 ist ein bekannter Fehler; nach Korrektur separat abnehmen.
EP/EV/Levelaufstieg/Attackenlernen und regulaeres Trainerpreisgeld sind noch
Implementierungsarbeit, daher erst nach deren Ergaenzung abnehmen. Nachlaufen
#3 ist zurueckgestellt; Gras #1 und Zuschauen #2 bleiben separat offen.
Nachfolgende Testanleitungen sind aeltere Staende.

## Nachtest: wilde Doppelfaelle ohne Gegner-Trainer (2026-11-15)

Build **064fbe57f391508b**, Boot **E3ID-6432b8729065aab9**, ABI **896a4be9**, Protokoll 20.

ARM-Release und passendes PAK sind neu gebaut. Beide Instanzen ueber
**Host und Gast starten** neu laden; F6 installiert keinen neuen Build.

1. Host fuehrt/Gast folgt: wilden Doppelkampf im Gras ausloesen. Auf beiden
   Geraeten duerfen auf der Gegnerseite nur die beiden wilden Pokemon stehen,
   kein Trainer. Intro, erste Runde und Flucht/Sieg kontrollieren.
2. Gast fuehrt/Host folgt wiederholen. Auch nach einem vorangegangenen
   Trainer-/Troy-Kampf pruefen, dass kein Gegner-Trainer im Wildkampf erscheint.
3. TEST A und TEST B/Troy kurz gegenpruefen: Spieler-Rueckenbilder, echte
   NPC-Trainerbilder und deren Sendouts muessen weiterhin vorhanden sein.

[Issue #5](https://github.com/Sumaiter/emerald-3ds/issues/5) bleibt bis zur
sichtbaren Bestaetigung offen. Touch-Zielauswahl #4 und Nachlaufen #3 sind
weiter bekannte offene Fehler. Nachfolgende Anleitungen sind aeltere Staende.

## Nachtest: NPC-Partnerkampf / Nachlaufen zurueckgestellt (2026-11-15)

Build **b0897187b8da4250**, Boot **E3ID-3d4d9019241c1602**, ABI **aac449b6**, Protokoll 20.

Neuer ARM-Release und PAK sind gebaut. Die laufenden Emulatoren enthalten
weiter den alten Boot E3ID-68d0d5a414085f28 und wurden nicht bedient.
Zum Nachtesten beide Instanzen ueber **Host und Gast starten** mit dem neuen
Build neu laden; F6 installiert keinen neuen Build.

1. TEST B/Troy gemeinsam starten, zuerst Host, dann Gast als Anfuehrer:
   beide Geraete muessen ueber Trainerintro und Ballwurf bis zur ersten
   bedienbaren Runde kommen, ohne doppelte/liegengebliebene Trainersprites.
2. Der folgende Spieler steuert STOLLUNIOR/PUPPANCE/TANHEL aus dem NPC-Team;
   der Anfuehrer behaelt seine ersten bis zu drei Pokemon. Wechsel und
   mindestens eine komplette Runde von beiden Spielern ausfuehren.
3. Sieg und Niederlage/Rueckkehr pruefen. Das eigene urspruengliche Team
   des NPC-Team-Steuernden muss danach unveraendert zurueckkehren; Reserven
   ausserhalb der ersten drei duerfen nicht ersetzt werden.
4. TEST B solo mit einem kampffaehigen Pokemon und TEST A sowie Wild-Doppel
   kurz gegenpruefen. Die vorhandenen Sprint-6-Abnahmepunkte bleiben offen.

Das noch asynchrone Nachlaufen ist als
[Issue #3](https://github.com/Sumaiter/emerald-3ds/issues/3) bewusst verschoben.
Keine weitere Bewegungskorrektur in dieser Runde. Nachfolgende Anleitungen
beschreiben aeltere Staende und gelten nicht als neue Laufzeitfreigabe.

## Nachtest: Bewegung, Gebietswechsel und Kampfansichten (2026-11-15)

Build **cc52f03a239453b8**, Boot **E3ID-68d0d5a414085f28**, ABI **3ab38ed2**, Protokoll 20.

Hintergrundtests, ARM-Release und PAK bestanden; sichtbare Abnahme noch offen.
Die vom Nutzer laufend gelassenen Emulatoren wurden nicht veraendert. Fuer
diesen neuen Stand beide Fenster selbst schliessen und das Paseo-Script
**Host und Gast starten** ausfuehren; F6 installiert keinen neuen Build.

1. Beide Fuehrungsrollen testen: gehen, rennen, schnelle Richtungswechsel und
   laengere Strecken. Ein empfangener Rueckstand soll sich abbauen; volle
   Schritte, besuchte Kurven und moeglichst ein Feld Abstand muessen bleiben.
   B bei weiter gehaltener Richtung soll sofortiges Weiterlaufen erlauben.
2. Haus/Tuer und verbundene Gebiete wechseln, waehrend der andere Spieler auf
   der alten Karte bleibt: dort darf keine alte/zweite Koop-Figur stehenbleiben.
   Wieder zusammenkommen, speichern/neuladen und ohne Gast kontrollieren.
3. TEST A mit beiden Spielern und je einem Pokemon: beide Hydropi muessen in
   den beiden Dreiergruppen sichtbar sein. Host und Gast sehen ihr eigenes
   Team links, den Partner rechts; nur eigene Pokemon duerfen eingewechselt
   werden. Mit verschiedenen ausgewaehlten Figuren erneut starten und die
   Rueckenbilder/Paletten inklusive Wurf kontrollieren. Kein rotes Funkkaestchen.
4. TEST B/Troy mit beiden moeglichen Anfuehrern: der Folgespieler steuert das
   NPC-Team STOLLUNIOR/PUPPANCE/TANHEL. Echte Gegnerbilder, erster Zug auf beiden
   Seiten, Wechsel, Sieg/Niederlage und Rueckkehr pruefen. Das gesamte eigene
   Team des Folgespielers darf danach weder ersetzt noch geheilt worden sein.
5. Beide Tests auch solo ab einem kampffaehigen Pokemon wiederholen. Mit mehr
   als drei eigenen Pokemon muessen Reserveplaetze 4..6 unveraendert bleiben.
   Im gemeinsamen Kampf: ein Dreierteam besiegen lassen, Partner spielt weiter;
   verloren ist erst bei beiden. Wild-Doppel mit erster Runde und Flucht testen.

Weitere Sprint-6-Abnahme (EP/EV/Level/Attacken/Preisgeld) bleibt separat offen.
Die nachfolgenden Testanleitungen sind historische Staende.

## Nachtest der Sprint-6-Fixes vom 2026-11-15

Build **6fd2556e4194dce4**, Boot **E3ID-16c4628f404bb3f4**, ABI **837d59ef**, Protokoll 20.
Gesamtpruefung, ARM-Build und PAK bestanden; Emulatornachtest noch offen.
Die laufenden Instanzen verwenden weiterhin E3ID-58cdf81623cdba8b.
Erst zum Nachtesten beide Fenster schliessen und das vorhandene Paseo-Script
**Host und Gast starten** manuell ausfuehren. F6 installiert keinen neuen Build.

- Host fuehrt, Gast folgt: gerade laufen, wiederholt enge Rechts-/Links-/
  Oben-/Unten-Kurven, gehen/rennen und unterschiedliche eigene Laufoptionen.
  Erwartung: gleiche Wegpunkte, moeglichst ein Feld Abstand, keine seitlichen
  Spruenge; B loest die Gruppe. Danach Rollen tauschen.
- In der Folgegruppe mit je einem Pokemon ins Gras: zwei wilde Gegner,
  beide Spieler koennen im ersten Zug waehlen, mindestens eine volle Runde,
  dann Sieg/Flucht und Rueckkehr ohne schwarzen Bildschirm pruefen.
- Ohne Folgegruppe TEST A (linker Junge) und TEST B (rechter Troy) auf Route101
  jeweils solo mit nur einem kampffaehigen Pokemon starten, einen Zug spielen,
  beenden und wiederholen. TEST A zeigt keinen erfundenen zweiten Spieler-Mon;
  TEST B hat Troys NPC-Team. Keine automatische Testheilung.
- Beide Tests in der Folgegruppe ausloesen, mit beiden moeglichen Anfuehrern.
  Bei Troy steuert der Folgespieler STOLLUNIOR/PUPPANCE/TANHEL und bekommt danach
  sein eigenes unveraendertes Team zurueck.
- Mit sechs eigenen Pokemon pruefen: pro Spieler nur Plaetze eins bis drei
  verwendbar, Plaetze vier bis sechs nach dem Kampf unveraendert. Ein bis zwei
  Pokemon genuegen ebenfalls. Nach KO des einen Spielers kaempft der Partner
  weiter; Niederlage erst, wenn beide Teams erschoepft sind.

Diese Checks sind **noch nicht manuell bestanden**. EP/EV/Lernen/Trainerpreisgeld
bleiben eigene offene Sprint-6-Arbeit. Grasanimation #1 und Zuschauerruckeln #2
bleiben getrennt offen. Aeltere Buildangaben unten sind Historie.

**Neuester Entwicklungsstand: Sprint 6 mit aktiver Folgegruppe**, Protokoll 20,
Build **7e5dc28033a62d97**, Boot **E3ID-58cdf81623cdba8b**, ABI **bcdc1bb8**.
Hintergrundtests und ARM-Release/PAK bestanden. Noch kein Emulatornachtest.
Beide Fenster schliessen, dann **Host und Gast starten** manuell ausfuehren.
F6 installiert keinen Build.

## Neue Upstream-Smokechecks

Die [PR-Merkliste](docs/UPSTREAM_PR_REVIEW.md) erfasst 12 uebernommene und 7 nicht
uebernommene PRs; unveraenderte Heads werden kuenftig uebersprungen.
Fliegenkarte einmal mit D-Pad, gehaltenem Wiederholen, Zoom sowie A/B pruefen.
Neue sichtbare Wild-Pokemon behalten die bei ihrem Erscheinen erzeugten IVs.
Die Hintergrundtests sind bestanden; die Emulatorabnahme der neuen PRs ist offen.
Die untenstehenden Route101-Tests bleiben fuer Sprint 6 erforderlich.

## Neue Testtrainer auf Route101

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

Gezielter manueller Test fuer die neuen Regeln, jeweils Host/Gast vertauschen:

1. A auf den freien anderen Spieler: automatisch folgen. Kurven, Stillstand,
   Tueren/Kartenwechsel pruefen. B loest die Gruppe sofort; mit gehaltener
   Richtung muss man direkt selbst weiterlaufen koennen.
2. Nur nebeneinander stehen: Trainer-/Wildkampf bleibt solo. Mit aktivem
   Folgen einen normalen Einzel-Trainer ausloesen: beide treten als Doppel an.
   Gegnerarten/-level passend zum Trainer, insgesamt hoechstens sechs.
3. Vorher alle sechs eigenen Pokemon notieren. Im gemeinsamen Kampf nur die
   ersten drei einsetzen/wechseln. Die Reserveplaetze vier bis sechs bleiben
   nach dem Kampf unveraendert und duerfen nicht ausgeliehen werden.
4. Ein Dreierteam komplett besiegen lassen: Partner kaempft alleine weiter.
   Erst nach beiden Dreierteams Niederlage. Heilung danach gemaess nativem
   WhiteOut; das gemeinsame Geld darf nicht zweimal halbiert werden.
5. Vorhandene Story-Doppelgegner behalten ihr Team. Bei Troy-Partnerkampf
   steuert der Folgespieler Troys Pokemon; sein eigenes komplettes Team bleibt
   auch nach Verlust unveraendert. Anfuehrer kann in der Storyauswahl keine
   Reserveplaetze vier bis sechs waehlen.
6. Direkt nach PC-/Teamwechsel gemeinsam kaempfen: neue Reihenfolge und
   aktuelle HP/Status/AP muessen verwendet werden. Partner im Menue/PC/Kampf
   darf nicht herausgerissen werden. B waehrend der Beitrittsphase darf keinen
   wartenden schwarzen Bildschirm erzeugen.
7. Nach Sieg/Niederlage Rueckkehr, Bewegung und Menues pruefen. Erledigter
   Trainer/Storyabschnitt darf nicht nochmal starten. Host speichern, neu
   laden und Gast erneut verbinden: eigene Teams und Fortschritt vergleichen.

EP/EV/Levelup/Attackenlernen und regulaeres Trainerpreisgeld im gemeinsamen
Kampf sind weiterhin NICHT implementiert und werden nach dem separaten Fix
abgenommen. Originale Sonderformate (Frontier/Geheimbasis/eReader/Safari)
bleiben solo. Weitere Rad-/Surf-/Fang-/Flucht-/Disconnect-/Hardwaretests sind
keine hier behaupteten bestandenen Tests. Gras #1 und Zuschauer-Ruckeln #2
bleiben offen. Die bestaetigten Standardtests aus Sprint5 bleiben anerkannt.

Die folgenden Rueckmeldungen beschreiben den zuvor getesteten Protokoll18-Build.

Die beiden Restfehler sind zurueckgestellt und getrennt auf GitHub erfasst:
[Grasanimation #1](https://github.com/Sumaiter/emerald-3ds/issues/1) und
[Zuschauer-Ruckeln #2](https://github.com/Sumaiter/emerald-3ds/issues/2).
Zuschauen gilt fuer den Nutzer vorerst als nutzbar. Als naechster Ausbau
folgt auf ausdruecklichen Wunsch Sprint6 nach Pruefung des Upstream-Main.

**Neuester Nachtest 2026-11-14: nicht bestanden.** Beim Beobachter fehlt die
Graswackelanimation weiterhin. Zuschauen ist besser, aber weiterhin ruckelig;
am kaempfenden Gast ist Ruckeln allenfalls wenig erkennbar. Der laufende Build
ist per Log als E3ID-6a834575fbbbc71c / ABI bd6f7986 bestaetigt. Beide Befunde
werden erneut untersucht; die bisherigen Hintergrundtests sind kein Ersatz
fuer diese Laufzeitabnahme. Andere bestaetigte Tests bleiben bestanden.

**Fix-Welle vom 2026-11-14 gebaut:** Grasbewegung beim entfernten Spieler
korrigiert und Zuschauen auf lokale Darstellung uebertragener Displayzustaende
umgestellt. Gesamt-Hintergrundtests und ARM-Release/PAK bestehen. Der neue
Emulatornachtest dieser beiden Aenderungen steht noch aus.

Deine bisherigen Standardtests einschliesslich Host-F6/Gast-Hauptmenue,
NPC-Marktfuehrung und **B: Zuschauen verlassen** bleiben als bestanden
dokumentiert. Sie muessen nicht pauschal wiederholt werden.

Fuer den neuen Build beide Fenster schliessen und das Paseo-Script
**Host und Gast starten** manuell starten. F6 installiert keine neue Version.

Gezielter Nachtest:

- **Gras:** Den anderen Spieler beim Gehen und Laufen durch Gras beobachten,
  danach Rollen tauschen. Die Grasbewegung soll am passenden Feld erscheinen;
  Stillstehen und Drehen duerfen sie nicht staendig erneut starten.
- **Zuschauen:** 2D und Voxel, spaeter Einstieg sowie echte Attacken, Text und
  HP-Balken pruefen; moeglichst auch Zuschauer und Kaempfer auf verschiedenen
  Karten. Mit B und Touch verlassen und direkt weiterlaufen. Kampfende und
  Host-F6 waehrend des Zuschauens pruefen, damit die neue Renderansicht sauber
  aufgeraeumt wird. Unter starkem Rueckstau koennen kurze Zwischenzustaende
  weiterhin fehlen; die Hostmessungen garantieren keine ruckelfreie Wiedergabe
  aller Attacken auf Emulator oder Hardware.

## Vom Nutzer getesteter Umfang am 2026-11-14

**Nachtrag:** Der Nutzer meldet die uebrigen Tests dieser Liste als erfolgreich
("der rest hat alles funktioniert"). Die folgende Liste dokumentiert den
geprueften Umfang; sie ist kein Auftrag, alles erneut zu testen. Ausgenommen
bleiben die zwei oben erfassten Fehler sowie die separat genannten Grenzfaelle.

Die Liste beschreibt die gemeldeten Standardtests einschliesslich getauschter
Rollen. PC-Sperre und Figurengalerie waren bereits zuvor akzeptiert.
Einzelne Zahlen wurden nicht unabhaengig gemessen; die Markierungen beruhen
auf der Rueckmeldung des Nutzers.

1. [x] **Unabhaengige Kaempfe:** Einer kaempft, der andere laeuft weiter und kann
   einen anderen Kampf starten. Beide gewinnen und kehren bedienbar zurueck.
   XP, AP, HP, Status und getragene Items bleiben beim richtigen Team.
2. [x] **Derselbe Trainer / Szenario:** Bereits waehrend des Dialogs darf der
   andere Spieler denselben Gegner nicht ebenfalls starten. Nach einem Sieg
   muss die Szene fuer beide abgeschlossen sein, einschliesslich Maike und
   eines normalen Trainers. Kein zweiter Kampf und keine zweite Belohnung.
3. [x] **Gemeinsame Gegenstaende:** Finden, kaufen, benutzen, halten/tauschen und
   Beutel-PC-Transfer kontrollieren; Mengen muessen beim anderen sofort
   folgen, auch wenn er kaempft. Bei nur einem Trank und gleichzeitigem
   Benutzen darf nur eine Wirkung entstehen; kein doppelter Verbrauch.
4. [x] **Gemeinsames Geld:** Kaufen, verkaufen und Trainer-Preisgeld kontrollieren.
   Beide zeigen denselben Betrag; kein nachtraegliches Verdoppeln am Kampfende.
   Zwei gleichzeitige Kaeufe bei zu wenig Gesamtgeld duerfen das Guthaben nicht
   ueberziehen oder kostenlose Ware geben.
5. [x] **Fang und Kampfende:** Fang mit freiem Teamplatz und mit vollem Team
   pruefen: eigenes Team bzw. genau ein Box-Pokemon. Flucht und Niederlage
   sollen zur bedienbaren Oberwelt fuehren und Sperren freigeben. Native
   Heilung/Geldverlust bei Niederlage von ungewolltem Reset unterscheiden.
6. [x] **Gast verlassen / Speichern / Wiederbeitritt:** Vorher HP, Status und AP
   eines Gast-Pokemon notieren, dann den Gast trennen. Host muss weiterspielen
   koennen und im sicheren Feldzustand automatisch speichern; bei laufendem
   Kampf/PC erst nach dessen Abschluss. Gastreihe darf ohne Heilereignis nicht
   geheilt werden. Danach beide neu starten und als derselbe Gast beitreten:
   Team, HP, Status, AP, Items, Geld und Storyfortschritt vergleichen.
7. [x] **Zuschauen funktional:** A am Spieler und am zugehoerigen NPC; B und
   Touch-Verlassen; danach direkt weiterlaufen. Kampfende soll ebenfalls
   sauber zur eigenen Ansicht fuehren. B-Beschriftung ist inzwischen bestanden;
   die neue lokale Darstellung wartet gesondert auf ihren Nachtest.
8. [x] **Darstellung im Spiel:** Ausgewaehlte eigene Figur beim Kampfeinstieg/
   Pokeballwurf und beim Laufen/Radfahren; Gastname in dessen Szenariotexten.
   Die freigegebene HTML-Galerie ersetzt diese Laufzeitprobe nicht.

Volle Boxen und Sonderkampf-Rueckkehr bleiben gezielte Grenztests mit
vorbereitetem Teststand. Die bisher dokumentierte fehlende Reservierung
einmaliger fester Wildgegner bleibt ein eigener offener Implementierungspunkt;
diese Faelle nicht aus den normalen Trainer-/Wildkampftests als bestanden
ableiten. Keine dieser Luecken wird durch die gruenen Hintergrundtests abgenommen.

Aktueller Koop-Stand: **Protokoll 19**, Build **225ca2caa5a7881b**,
Bootkennung **E3ID-5790a58262fe8f67**, ABI **73bbc4ee**. Nach einem neuen Build beide Fenster
schliessen und das Paseo-Script erneut starten; dabei werden die drei passenden
Builddateien in die Profile kopiert. F6 setzt nur das bereits geladene Spiel
zurueck und installiert keinen neuen Build.

Beutel, PC-Items, Geld und Muenzen sind gemeinsam und werden waehrend des
Spielens synchronisiert. A auf den kaempfenden Spieler oder neben dessen
NPC-Gegner startet Zuschauen; B oder **B: Zuschauen verlassen** auf dem Touchscreen
beendet es. Nach einem Gast-Disconnect speichert der Host automatisch, sobald
die Oberwelt sicher ist. Die Gast-Teamreihe behaelt HP/Status/AP.

Trennt der Host die Verbindung, kehrt der Gast ins Hauptmenue zurueck, auch
aus Kampf, PC oder Beutel. Ein stiller Verbindungsabbruch wird nach ungefaehr
sechs Sekunden erkannt. Zum erneuten Mitspielen vom Hauptmenue frisch beitreten.

Die beiden Azahar-Profile liegen zentral ausserhalb der Branch-Arbeitsbaeume:

| Einstellung | Host | Gast |
| --- | --- | --- |
| Emulatorname / Vorgabe beim Direktverbinden | `Host` | `Gast` |
| Profilordner | `C:\paseoprojects\smaragd3ds\.tools\azahar\host\user` | `C:\paseoprojects\smaragd3ds\.tools\azahar\guest\user` |
| Spielordner im Profil | `sdmc\3ds\emerald3ds` | `sdmc\3ds\emerald3ds` |
| Emulator | Azahar 2126.2 | Azahar 2126.2 |

Einstellungen, Tastenbelegung, NAND und Spielstaende bleiben in diesen beiden
portablen Profilen erhalten. Beim Branchwechsel dieselben Profile verwenden.
Beide Fenster vor dem Aktualisieren schliessen.

## Start

In Paseo im Workspace unter **Scripts** den Eintrag **Host und Gast starten**
manuell starten. Die Einbindung steht in `paseo.json`. Es gibt keinen Autostart.

Im Verzeichnis des gewuenschten Branches / Worktrees:

```powershell
powershell -File .\tools\start-coop-test.ps1
```

Das Skript kopiert den Build dieses Arbeitsbaums in beide Spielordner, setzt
die Emulatornamen `Host` und `Gast` und startet den lokalen Raumserver sowie
zwei Spielfenster. Ein bereits laufender eigener Raumserver auf UDP-Port `24872`
wird wiederverwendet. Die Verbindung stellst du in beiden Spielfenstern selbst
her. Die SAV-Dateien werden beim Startskript nicht ersetzt.

Erwartete Build-Dateien:

```text
build\upstream\3ds_port\dist\Emerald3DS.3dsx
build\upstream\3ds_port\dist\Emerald3DS.smdh
build\upstream\3ds_port\build\emerald3ds.pak
```

Bei einem anderen Build-Verzeichnis:

```powershell
powershell -File .\tools\start-coop-test.ps1 -BuildPort C:\Pfad\zum\3ds_port
```

Die zentrale Kopie des Skripts kann auch von Branches ohne diese Datei aufgerufen
werden. Dabei im gewuenschten Branch-Verzeichnis bleiben oder `-RepoRoot` angeben:

```powershell
powershell -File C:\paseoprojects\smaragd3ds\.tools\azahar\start-coop-test.ps1 -RepoRoot C:\Pfad\zum\Arbeitsbaum
```

## Neue Reparaturen gezielt pruefen (Protokoll 15)

Beide alten Spielfenster schliessen und danach das Paseo-Skript **Host und Gast
starten** verwenden. F6 allein laedt keine neu gebaute Programmdatei.

- Rivalen und normale Trainer: schon waehrend des Gespraechs gegeneinander
  sperren; nach Sieg, Niederlage und Abbruch muss die Freigabe stimmen.
- Trainer mit wechselnder Blickrichtung: nach Wartezeit korrekt heranlaufen.
- RusturfTunnel-Szene: Peeko und Mr. Briney muessen ihre Bewegungen beenden.
- Geheimbasis/Frontier und Steven: nach Rueckkehr beweglich; urspruengliche
  Pokemon und gehaltene Items erhalten. Normaler WhiteOut muss weiterhin
  heilen und den vorgesehenen Geldverlust ausfuehren.
- Bei gleichzeitigem Fang in volle Boxen: PC zum Platzschaffen nutzbar,
  wartender Fang nach Schliessen vorhanden und Boxinhalt bei beiden gleich.
  Den eigenen neuen Fang auch verschieben, bearbeiten, ins Team entnehmen
  oder absichtlich freilassen: kein Duplikat, kein Wiederauftauchen und keine
  ueberschriebene Aenderung nach der Synchronisierung.

Automatische Tests fuer diese Faelle laufen im Hintergrund unter verify-coop.
Der manuelle Test der sichtbaren Ablaufe ist damit nicht ersetzt.

## Ereignissperre pruefen

Nach dem Neustart beider Instanzen mit derselben neuen Testversion:

1. Gast spricht zuerst einen Trainer an; Host versucht denselben NPC schon
   waehrend des Gespraechs und spaeter waehrend des Kampfes anzusprechen.
   Host soll im Feld bleiben und unten den Namen des Gasts sehen.
2. Dasselbe mit vertauschten Rollen pruefen, einschliesslich Rivalen und
   normalem Trainer, danach automatisch ausgeloestem Kampfereignis.
3. Nach Kampf und Abschlussdialog erneut pruefen: Story/Trainerstatus soll
   uebernommen sein. Nach Niederlage oder Abbruch muss der Ablauf wieder frei sein.
4. Verschiedene Trainer sowie unabhaengige Zufallskaempfe gleichzeitig pruefen.

Die Sperre beginnt vor dem ersten Skriptbefehl. A/B oder Tippen bestaetigt nur
den Hinweis und fuehrt das abgewiesene Gespraech nicht spaeter automatisch aus.
Diese Hintergrundpruefungen ersetzen nicht die manuelle Emulatorabnahme.
Ein bereits haengender alter Build wird durch F6 nicht aktualisiert; beide
Fenster schliessen und das Paseo-Startskript fuer den neuen Build ausfuehren.

## Verbindung bei Bedarf selbst herstellen

Die gespeicherte Direktverbindungsadresse lautet `127.0.0.1`, Port `24872`.
Der lokale Azahar-Raum **Emerald Koop** wird vom Startskript mitgestartet und
erlaubt zwei Teilnehmer. Das Raumpasswort bleibt leer. Beide Emulatoren brauchen
denselben zusammengehoerigen 3DSX-/SMDH-/PAK-Build.

Wenn die Spielfenster bereits offen sind und nur der Server fehlt:

```powershell
powershell -File .\tools\start-coop-test.ps1 -ServerOnly
```

Der Raumserver laeuft im Hintergrund. Seine Protokolle liegen unter
`C:\paseoprojects\smaragd3ds\.tools\azahar\room`. Zum Start gehoert
`--preferred-app "Emerald 3DS"`; ohne diese Angabe beendet sich Azahar 2126.2
mit `preferred application is empty!`.

Das Koop-Passwort im Spiel gehoert zum jeweiligen Spielprofil und ist vom
Raumpasswort getrennt. Die weitere Einrichtung und die Spielbedienung macht
der Nutzer selbst.

## Gesicherte Einstellungen und aktueller Stand

- Quellstand beim Einrichten: `main`, Commit `fdc4b48` (12. November 2026).
- Die Emulatornamen stehen in den portablen 3DS-Konfigurationen. Das Skript
  sichert die urspruengliche Datei einmalig als `config.before-host-gast`.
- Die beiden Programmdateien heissen `Host.exe` und `Gast.exe`; DLLs und
  portable Profile werden aus dem jeweiligen vorhandenen Azahar-Ordner verwendet.
- Die zentrale Kopie dieser Anleitung liegt unter
  `C:\paseoprojects\smaragd3ds\.tools\azahar\LOKAL_TESTEN.md`.
- Aktueller gemeinsamer Release-Build: `cf712051f18d29ac`, Datenpaket-ABI
  `ebf0f1f4` (13. November 2026), Protokollversion 11. Er enthaelt die
  Gastnamen-, Starter-, Wiederbeitritts- und PC-Besitzermeldungs-Korrekturen
  sowie alle neuen Kampf- und Fahrradgrafiken und die Sprint-5-Integration
  aus origin/sprint-5 (1be2515). Beide Spielfenster vor dem
  Aktualisieren schliessen; F6 allein kopiert keine neuen Build-Dateien.
- Wenn der andere Spieler den PC belegt, zeigt der untere Bildschirm seinen
  Spielnamen. Erst A, B oder ein Tipp schliesst die Meldung und fuehrt ins
  PC-Menue zurueck. Die gegenseitige Sperre bleibt dabei aktiv.
- Alle 99 Figuren sind fuer den Kampf abgedeckt: acht native animierte
  Rueckenansichten und 91 neue statische Bilder. Beim Fahrrad bleiben zwei
  native Animationen erhalten; 97 neue Boegen liefern jeweils 27 Frames.
  Die 15 Figuren aus ZEROWALK.md haben zusaetzlich neun echte Laufbilder und
  sind wieder in der Auswahl. Beide Spieler verwenden ihre eigene Auswahl.
- Release-/PAK-Build, acht Koop-Pruefgruppen, Kampf- und Fahrradtests sowie
  Release-Audit bestehen. Der separate Starter-Menue-Test kann mit dem
  vorhandenen Windows-Testlinker nicht linken; Details stehen in HANDOFF.md.
  Die interaktive Abnahme der neuen Korrekturen/Grafiken steht noch aus.
- ROMs, Spielstaende, PAKs, Emulatorprofile und Build-Dateien bleiben lokal.

Die aktuellen Spieloptionen bleiben in `settings.txt` im jeweiligen
Spielordner gespeichert. Beim Einrichten verwendete Host `voxel=0`,
`voxel_battle=0`, `classic_menus=1`, `local_coop=1`; Gast verwendete `voxel=1`
und `voxel_battle=1`. Beide verwendeten `voxel_pitch=40`, `voxel_zoom=100`,
`voxel_blur=100`, `fps=0`, `run=0` und `speed=1`. Die komplette
Emulatorkonfiguration samt Tastenbelegung liegt in
`user\config\qt-config.ini` des jeweiligen Profils.

## Figuren und Animationen in HTML pruefen

Die Grafiken aller 99 Figuren wurden vom Nutzer nach Sichtung der Galerie
freigegeben ("ok die passen alle nice"). Die folgenden Tests im laufenden
Spiel bleiben davon unabhaengig.

Die vollstaendige lokale Galerie liegt unter
[build/character-gallery/index.html](build/character-gallery/index.html).
Die Datei direkt im bevorzugten Browser oeffnen; kein Server erforderlich.
Sie enthaelt alle 99 Figuren einschliesslich der bisherigen 84. Suche, Filter
fuer neue Figuren, vier Richtungen, Lauf-/Renn-/Mach-/Acro-Animationen,
Kampfbilder, Pause, Tempo und Einzelbilder erlauben den direkten Vergleich.
Notizen und Freigaben koennen als JSON-Pruefbericht gespeichert werden.
Die Datei enthaelt Originalreferenzen und bleibt deshalb nur lokal im Buildordner.
Automatische Daten-/Animations-/Syntaxpruefung bestanden; die sichtbare
Browserpruefung war mangels Computer-Use-Freigabe noch nicht moeglich.

## Sprint 5 testen

Die bereits offenen Spielfenster verwenden noch den vorherigen Build.
Beide Fenster schliessen und in Paseo **Host und Gast starten** ausfuehren.
Damit werden 3DSX/SMDH/PAK gemeinsam aktualisiert; F6 allein reicht nicht.
Verbindung weiterhin 127.0.0.1:24872.

1. Gast beginnt einen wilden Kampf mit seiner eigenen Gruppe; Host laeuft
   dabei weiter. Danach beide gleichzeitig gegen verschiedene Gegner kaempfen.
2. Jeweils EP, Level, AP, Status, getragenes Item und Team vor/nach dem Kampf
   vergleichen. Fangen sowohl mit freiem Teamplatz als auch mit voller Gruppe
   und freiem Boxplatz pruefen; alle Faenge muessen erhalten bleiben.
3. Denselben normalen Trainer gleichzeitig ausloesen. Es darf nur ein Kampf
   gegen diesen Trainer laufen; nach seinem Sieg darf der wartende Spieler
   keinen zweiten Kampf mit erneuter Belohnung starten.
4. Flucht und Niederlage fuer Host und Gast pruefen, danach Rueckkehr in die
   Oberwelt und erneuten Kampf. Der andere Spieler muss bedienbar bleiben.
5. Der PC bleibt waehrend aktiver Kaempfe und unbestaetigter Ergebnisse
   gesperrt. Danach soll er wieder freigegeben sein. Ohne aktiven Kampf die
   gegenseitige PC-Sperre mit Besitzernamen weiter pruefen.
6. Host normal speichern, beide neu starten und Gast erneut beitreten:
   Teams, Faenge, Inventar, Geld und besiegte Trainer muessen erhalten bleiben.
7. Eigene Figur im Kampf/Pokeballwurf, Mach-/Acro-Fahrrad, Gastname in
   Szenariotexten und direkten Wiederbeitritt weiterhin gegenpruefen.

Die Reservierung statischer einmaliger Wildgegner ist im uebernommenen
Sprint-5-Branch noch nicht umgesetzt. Volle Boxen oder nicht anwendbare
Inventaraenderungen werden ohne Teiluebernahme zurueckgehalten; die
Bedienbarkeit und Wiederherstellung dieser Grenzfaelle sind noch offen
fuer die interaktive Abnahme. Automatische Tests ersetzen diese Abnahme nicht.

Paseo-Konfiguration: [Workspace-Skripte](https://paseo.sh/docs/worktrees#scripts-and-services).

Beim erstmaligen Windows-Build dieses main-Stands waren zwei lokale
Vorbereitungsschritte notwendig: wiederholte Dateidiffs aus
`0083-local-coop.patch` der Reihe nach anwenden und die deutsche PC-Lokalisierung
vor den Koop-Aenderungen derselben Datei stagen. Ausserdem enthaelt der
`local_session.c`-Abschnitt von `0055-local-wireless.patch` 136 statt der im
Hunk angegebenen 131 Zeilen; fuer den Build wurden alle 136 Zeilen uebernommen.
Die lokalen Hilfsskripte liegen im ignorierten `build`-Ordner dieses Worktrees.
