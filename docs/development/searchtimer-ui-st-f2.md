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


## ST.F2.2 – PWA-Live-Aktualisierung statt persistent veralteter Scripts

Der vollständig installierte ST.F2.1-Daemon lieferte die korrekte
`searchtimers.js?v=61a891e9778398d7` sowohl direkt als auch via HTTPS/Nginx.
Trotzdem zeigte die PWA nach Installation weiterhin die frühere UI.
Ursache auf dem tatsächlichen PWA-Pfad: Die zusammengesetzte
`service-worker.js` beantwortete Canonical-HTML-Navigationen und
JS/CSS-Requests aus ihrem persistenten, unveränderten Static-Shell-Cache
**vor** jedem Netzwerkzugriff. Damit konnte auch F5 eine ältere
JavaScript-Version ausführen.

Die Korrektur schaltet ausschließlich für diesen öffentlichen statischen
Shell-Pfad auf Netzwerk zuerst (`cache: no-store`) um. Bei tatsächlichem
Netzwerkfehler bleibt der zuvor byteverifizierte Offline-Cache erhalten.
Versionierte JS-URLs werden bei Offline-Fallback nur dann einem Cache-
Eintrag zugeordnet, wenn ihre Kennung den Manifestbytes entspricht.
Unbekannte Versionen und Backend-/API-Requests werden nicht umgeleitet.
Der Builder hasht das vom Daemon tatsächlich ausgelieferte, versionierte
Index-HTML, damit Offline-Manifest und HTTP-Response wieder übereinstimmen.
Der installierte Worker kann beim Update ohne Schließen aller Tabs
aktivieren; er beansprucht keine laufenden Clients und lädt nicht
automatisch neu. Beim nächsten normalen Navigieren übernimmt der neue
Worker.

Regressions: `node web/frontend/tests/test_pwa_local_shell.js` und
`make test-pwa-shell`. Rollout nur als gezieltes **Service-Worker-Dateiupdate**:
die bestehende kanonische Composer-Funktion
`node tools/build_pwa_shell.js /usr/share/vdr-suite/web/frontend`
setzt den Worker aus der geprüften Source und dem bereits installierten
statischen Frontend zusammen; weder Daemon noch Datenbank noch andere
Frontend-Dateien müssen dabei ersetzt werden. Browser-/yaVDR-Abnahme
bleibt unabhängig.


## ST.F4 – Uhrzeitfilter als native Uhrzeit-Eingabe (20.11.2026)

Der von der yaVDR-Abnahme gemeldete Fehler war eine unverständliche
HHMM-Zahleneingabe: `20` und `22` wurden technisch zu `00:20`
und `00:22` statt `20:00` und `22:00`.

Die vorhandenen `Beginn`- und `Ende`-Felder verwenden jetzt
`<input type="time" step="60">` und zeigen Werte wie `20:00`
direkt als Uhrzeit an. Die Anzeige wandelt bestehende epgsearch-
HHMM-Werte einschließlich Vorlagen und UPDATE-Vorbelegungen in die
Uhrzeitdarstellung um. CREATE/UPDATE/Vorschau behalten ihre **bisherigen**
numerischen HHMM-Payloads (`20:00` -> `2000`), ohne API- oder
Datenbankänderung. Fehlt bei eingeschaltetem Zeitfilter ein Wert, zeigt
die vorhandene Validierung eine eindeutige Eingabemeldung, statt eine
ungültige Uhrzeit stillschweigend zu Mitternacht umzudeuten.
Ausgeblendete gültige Felder behalten ihre Werte.

Der bestehende JavaScript-Workflowtest prüft native Control-Typen,
Minutenschritte, Anlegen mit `09:30–11:30`, den API-Roundtrip
`20:00–22:00` -> `2000–2200`, leere aktive Werte und Vorlagen-`20:45`.
Kleinstes Deployment: nur
`/usr/share/vdr-suite/web/frontend/modules/searchtimers.js`
aktualisieren und den vorhandenen PWA-Service-Worker mit
`tools/build_pwa_shell.js` für das neue statische Release neu
zusammensetzen. Kein Daemon-/VDR-Neustart oder C++-Build.
Realer Browser-Readback nach Installation separat.


## ST.F4.1 – Uhrzeitfilter-Layout

Bei der echten Browserabnahme stand `Beginn` neben dem Toggle,
während `Ende` in die nächste Zeile fiel. Der Toggle belegt jetzt
die gesamte erste Rasterzeile; darunter liegen `Beginn` und
`Ende` nebeneinander (bei schmalen Displays untereinander).
Nur `searchtimers.js` wurde geändert, ohne Einfluss auf HHMM,
CREATE/UPDATE/Vorschau, Service/API oder den restlichen Editor.
Der vorhandene UI-Regressionstest prüft Spanning und DOM-Reihenfolge.
