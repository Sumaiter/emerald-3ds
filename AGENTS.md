# Arbeitsregeln für Paseo-Subagenten

- Jeder Subagent muss so gestartet werden, dass er in Paseo sichtbar ist. Das gilt für Recherche, Planung, Implementierung, Reviews und Tests.
- Dafür immer die Paseo-Orchestrierungswerkzeuge (`create_agent`) oder die Paseo-CLI (`paseo run`) im aktuellen Workspace verwenden.
- Keine lediglich internen Hintergrund-Agent-Tasks verwenden, die nicht in Paseos Subagents-Ansicht erscheinen.
- Dem Subagenten eine eigenständige Aufgabe, das Arbeitsverzeichnis, relevante Pfade, bereits bekannte Ergebnisse und klare Grenzen mitgeben.
- `notifyOnFinish` aktivieren, damit das Hauptergebnis automatisch zurückgemeldet wird. Den Nutzer beim Start kurz über Aufgabe und Sichtbarkeit informieren.
- Ohne ausdrückliche Vorgabe des Nutzers darf die Denkstufe eines Subagenten höchstens der des Hauptagenten entsprechen. Verlangt der Nutzer ausdrücklich eine bestimmte Stufe, gilt diese Vorgabe auch dann, wenn sie höher ist.
- Die Arbeit eines Subagenten nicht unnötig parallel im Hauptagenten wiederholen und nicht fortlaufend abfragen; auf die Abschlussmeldung warten.
