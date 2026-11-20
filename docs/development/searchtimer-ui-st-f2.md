# ST.F2 – Kontextabhängige Suchtimer-Felder

**Stand: 20.11.2026 · Webfrontend · bestehende Public API v1**

Skippys Rückmeldung aus dem MLD-/VDR-Portal-Test wird mit der vorhandenen
Suchtimer-Erstell- und Bearbeitungsmaske umgesetzt:

- `Alle Kanäle`: Keine weiterführenden Kanalfilterfelder.
- `Kanal/Kanalbereich`: Auswahlgruppe (sofern vorhanden) und die Felder
  `Von Sender` / `Bis Sender`. Die manuelle Expertenoption bleibt
  als bewusster Fallback bei fehlender Kanalliste erreichbar.
- `Kanalgruppe`: `Kanalgruppe filtern` statt Senderbereich.
- `Nur frei empfangbare Kanäle`: Keine zusätzlichen Eingabefelder.
- `Aufnahmedauer`: Minimum/Maximum erst nach `Aktivieren`.
- `Uhrzeitfilter`: eigener Bereich mit Beginn/Ende nur bei Aktivierung.
- `Wiederholungen`: Vergleichskriterien erst nach `Vermeiden`.
- `Wochentage filtern`: Maske erst nach Aktivierung.
- Wiederherstellung bestehender Regeln/Vorlagen aktualisiert die Sichtbarkeit
  nach dem Setzen aller Werte; versteckte Eingaben bleiben erhalten.

Die Anzeige steuert nur die Sichtbarkeit. Bestehende CREATE-/UPDATE-Payloads,
Idempotenz, Revision, Berechtigungen und Public-v1-Schreiboperationen bleiben
unverändert. Der Browser wertet Vorschaufilter nicht selbst aus; die
serverseitige Filterimplementierung gehört zu ST.F1 (Issue #562).

**Regression:** `node web/frontend/tests/test_searchtimer_workflows_runtime.js`
und die regulären Frontend-/CI-Tests. MLD-/yaVDR-Browserabnahme bleibt
separat zu bestätigen.
