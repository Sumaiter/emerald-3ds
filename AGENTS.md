# Arbeitsregeln für Paseo-Subagenten

- Jeder Subagent muss so gestartet werden, dass er in Paseo sichtbar ist. Das gilt für Recherche, Planung, Implementierung, Reviews und Tests.
- Dafür immer die Paseo-Orchestrierungswerkzeuge (`create_agent`) oder die Paseo-CLI (`paseo run`) im aktuellen Workspace verwenden.
- Keine lediglich internen Hintergrund-Agent-Tasks verwenden, die nicht in Paseos Subagents-Ansicht erscheinen.
- Dem Subagenten eine eigenständige Aufgabe, das Arbeitsverzeichnis, relevante Pfade, bereits bekannte Ergebnisse und klare Grenzen mitgeben.
- `notifyOnFinish` aktivieren, damit das Hauptergebnis automatisch zurückgemeldet wird. Den Nutzer beim Start kurz über Aufgabe und Sichtbarkeit informieren.
- Alle Agenten und Subagenten ausdrücklich im Berechtigungsmodus `auto-review` starten. Laufende Subagenten bei Bedarf auf `auto-review` umstellen; nicht ohne Nutzeranweisung auf Full Access wechseln.
- Ohne ausdrückliche Vorgabe des Nutzers darf die Denkstufe eines Subagenten höchstens der des Hauptagenten entsprechen. Verlangt der Nutzer ausdrücklich eine bestimmte Stufe, gilt diese Vorgabe auch dann, wenn sie höher ist.
- Modell und Denkstufe anhand der jeweiligen Aufgabe selbstständig passend auswählen. Bestehende Agenten bei Bedarf prüfen und umstellen; konkrete Modell- oder Denkstufenvorgaben des Nutzers haben Vorrang.
- Die Arbeit eines Subagenten nicht unnötig parallel im Hauptagenten wiederholen und nicht fortlaufend abfragen; auf die Abschlussmeldung warten.
