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

## ST.F2.1 – normales Browser-Neuladen nach einem Frontend-Einzeldateiupdate

Beim yaVDR-Praxistest wurde die neue `searchtimers.js` sowohl auf dem
Datenträger als auch per direktem HTTP-GET des Daemons bytegleich
nachgewiesen, während das laufende Browserformular noch ältere Texte
(`Dauerfilter aktivieren`, `Wiederholungen vermeiden`) zeigte.
Ein `fetch(..., {cache: "no-store"})` im Browser sah die neuen Bytes,
bewies aber **nicht**, welche Version der Parser beim Seitennavigieren
bereits ausgeführt hatte.

Die Daemon-Indexauslieferung ergänzt deshalb beim vorhandenen statischen
`<script src="../frontend/modules/searchtimers.js">` eine stabile
`?v=`-Kennung aus den ersten acht SHA-256-Bytes der **tatsächlich
ausgelieferten Datei**. Der HTML-Einstieg bleibt `Cache-Control: no-cache`,
und die versionierte JS-URL wird auf denselben existierenden Public-Asset-
Eintrag aufgelöst. Kein neuer Loader, keine erweiterte Schreibbefugnis,
keine Service-Worker-Änderung und keine Cache-Deaktivierung im Browser.

Dadurch erhält eine normale Navigation per F5 bei einer nachträglich
installierten `searchtimers.js` einen anderen Browser-Cache-Schlüssel.
Andere Skripte, PWA-Artwork-Caches und Public API v1 bleiben unberührt.
Benötigt einmalig die Auslieferung des aktualisierten Daemons; die
Versionsermittlung funktioniert danach auch bei bloßen Datei-Updates
ohne erneuten Daemon-Neustart.

Der bestehende Produktions-Asset-Routentest prüft die Änderung des
Versionswerts nach Austausch der JS-Datei, die Auslieferung per versionierter
URL, den unveränderten `no-cache`-HTML-Vertrag und die Ablehnung
unbekannter Asset-Pfade. CI-/Build-/yaVDR-Abnahme getrennt durchführen.
