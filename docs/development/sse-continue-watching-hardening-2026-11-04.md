# SSE.H — Live-Transport und „Weiterschauen“ härten (04.11.2026)

**Status:** Quellbasierter Hardening-Backlog; nicht implementiert, nicht auf yaVDR abgenommen. Kein neuer nummerierter Runtime-Abschnitt/Phase 70. **SSE vor Frontend-Polling** bleibt verbindlich.

**Grundlage:** [ADR-0019 — SSE](../adr/ADR-0019-sse-event-stream-transport-strategy.md), [ADR-0018 — Incremental Snapshot Synchronization](../adr/ADR-0018-incremental-snapshot-synchronization.md), [ADR-0017 — Live Transport Boundary](../adr/ADR-0017-live-transport-boundary.md). Umsetzung gemäß [Strict Roadmap](../planning/roadmap.md).

## Was bereits besser ist

Continue-Watching-SSE wurde auf `main` bis Code-Commit `99790c74d3d45228c07787837baf2d20914a7387` implementiert (Dokumentations-Ausgangsstand `5359940db66e83d8d6de97b237eb0f73bc50f009`):

- Fortschritt bleibt **actor-scoped im Core/SQLite**; CAS/Revision bleibt die Autorität. SSE liefert nur einen Backend-Invalidierungshinweis; der Client liest die eigenen Daten anschließend authentifiziert nach.
- `ContinueWatchingChanged` ist eine gezielte `continueWatching`-Domäne ohne unnötigen VDR-Snapshot-Refresh.
- Home aktualisiert nur betroffene Karten; Poster-DOM und horizontale Scrollposition bleiben erhalten. Überlappende oder veraltete Antworten werden abgewehrt.
- Browser meldet erstes und letztes Fortschrittssample gezielt; normales 5-Sekunden-Speichern wird nicht jedes Mal zur Push-Benachrichtigung.

**Wichtig:** Ein Merge und JavaScript-Mock-Tests sind keine Bestätigung der C++-Kompilierung, des echten Browser-/Device-Verhaltens oder einer erfolgreichen Installation.

## Quellbefunde und Risiken

| ID | Befund | Prüfung |
| --- | --- | --- |
| SSE.H1 | **Bestätigter Codebefund:** `SseLiveTransport::publish()` und `SnapshotChangeFeed::addEntry()` speichern Frames/Entries in wachsenden Vektoren. `LiveTransportController::getStream()` liefert den gesamten von `stream()` zusammengesetzten, **endlichen** SSE-Body. Im geprüften Produktivpfad ist keine begrenzte Retention belegt. Speicher- und Antwortvolumen wachsen potenziell mit der Laufzeit. | Lasttest, Anzahl Frames, Bytes pro Reconnect und Speichergrenze. Ein Produktiv-OOM wurde nicht nachgewiesen. |
| SSE.H2 | **Bestätigter Codebefund:** `createClientLiveUpdateSource()` erzeugt pro Aufruf einen `EventSource('/api/vdr/live')`. `home-recording-discovery.js` und `home-continue-watching.js` verwenden jeweils einen auf Home. Server-seitiger `Last-Event-ID`-Delta-/Gap-Vertrag ist im geprüften Browserpfad nicht nachgewiesen. | Mehrere Rails, gleichzeitige Clients, Reconnect, Sequenzreset, Sessionwechsel. |
| CW.H1 | **Codepfad bestätigt, sichtbare Nutzerwirkung offen:** `continue-watching-sync.js::syncProgress()` überspringt bei `position === lastPosition` auch eine geplante Abschluss-`notifyLive`-Mutation. Das heißt **nicht**, dass gespeicherte Daten verloren gehen. | Sollte ein Stop/Pause bei unveränderter Position einen zusätzlichen bestätigten Status auslösen? Test mit zweitem Client. |
| CW.H2 | **Konkurrierende Aktionen möglich:** `home-continue-watching.js::openItem()` ruft für „Von vorn“ `clearBeforeRestart()` ohne Await auf und startet die Wiedergabe unmittelbar. Ein verzögerter Clear könnte eine neue Position überschneiden. | Reproduktions-Test mit verzögertem Clear, CAS-Konflikt und zwei Clients. |
| SSE.H3 | **Optimierungsfrage, keine belegte Datenoffenlegung:** `ContinueWatchingApiRuntime` sammelt nach `backendId`, `DaemonRuntime` publiziert backendweite Invalidierungen. Das Event enthält keine actor-spezifischen Positionen; andere angemeldete Benutzer können aber unnötig nachlesen. | Aufwand/Nutzen actor-scoped Fanout und bestehende Authentisierung prüfen. |
| DEP.H1 | **Beobachteter Installationsblocker:** Auf yaVDR meldete `install_daemon.py check` `INSTALLED_SOURCE_ID_UNKNOWN`: die alte ausführbare Datei hat keine vom Installer erkennbare `git-<SHA>`-Quellkennung. Der direkte Binäraustausch ersetzt keinen überprüften Alt-Neu-Datenbank-/Rollback-Vertrag. | Altes Binary/Schema read-only beurteilen, bevor produktiv ersetzt wird. Kein automatisches DB-Backup. |

## Umsetzbare Slices (Reihenfolge)

### SSE.H1 — Retention und korrektes Delta-/Gap-Replay (nächster Slice)

**Ziel:** Begrenzte Change-Feed-/SSE-Historie, keine erneute Komplettübertragung bei jedem normalen Reconnect, fehlende Events zuverlässig erkennbar.

1. Erst die echten Verwendungen von `SnapshotChangeFeed`, `SseLiveTransport`, `LiveTransportController`, `SimpleHttpListener` und Device-Live-Hub erfassen. Bestehende Sequenznummern, Client-Cursor, Auth-/Backend-Grenzen und Tests identifizieren.
2. Ein **begrenzt** aufbewahrtes Ereignisfenster einführen, monoton sequenzieren und erste/letzte verfügbare Sequenz eindeutig machen. Keine unabhängigen konkurrierenden Event-Speicher erfinden.
3. Gültige Cursor: nur neuere Einträge; veraltete, ungültige oder nach Daemon-Neustart unbekannte Cursor: expliziter Catch-up-/Resync-Vertrag statt stiller Datenlücke. ADR-0018 berücksichtigen. Web- und Device-Konsumenten kompatibel halten.
4. Ob der bestehende HTTP-Server echte langlebige SSE-Verbindungen unterstützt, im Code nachweisen. **Kein Pseudo-Streaming mit Endlosschleife.** Falls hierfür ein wesentlicher HTTP-Umbau nötig wäre, einen getrennten, zusammenhängenden Slice vorschlagen statt den Umfang von SSE.H1 still zu erweitern.
5. Gezielte Tests: 0/1/viele Events, Pufferüberlauf, alter/aktueller/ungültiger Cursor, Neustart, Reihenfolge, Authentisierung, Backend-Isolation, Reconnect, Speicher-/Antwortlimit. Keine neuen zyklischen Frontend-Polling-Fallbacks.

**Abnahme:** Lange Ereignishistorien lassen Speicher und Replay-Body nicht unbegrenzt wachsen; normales Replay ist inkrementell, Lücken werden erkennbar und führen zu einmaligem kanonischen Readback.

### SSE.H2 — Gemeinsamer Browser-SSE-Bezug

Nach SSE.H1 prüfen, ob Serien-Rail, Recording Discovery, „Weiterschauen“ und Recordings2 eine **bestehende** SSE-Verbindung pro authentifizierter Browser-Session teilen können; gegebenenfalls minimalen Fanout ergänzen statt parallele Transportarchitektur. Tests: Logout, Backendwechsel, Sichtbarkeit, Tabwechsel, Reconnect, doppelte Events, keine verwaisten Verbindungen.

### CW.H1 — Bestätigte Stop-/Abschluss-Semantik

Reproduktion zuerst. Falls ein zusätzlicher Abschlusszustand fachlich notwendig ist, nur nach bestätigtem Core-/CAS-Ergebnis melden. Keine künstliche Progress-Mutation oder zweite Datenwahrheit. Tests: gleiche/geänderte Position, Ende, Clear, Konflikt, zweiter Browser/Device.

### CW.H2 — „Von vorn“ ohne Race Condition

Clear und neue Wiedergabesitzung korrekt ordnen, Fehler sichtbar machen und CAS-Revisionsschutz weiterverwenden. Tests: verzögerter Clear, fehlgeschlagener Clear, schneller Doppelklick, konkurrierender Device-Fortschritt.

### SSE.H3 — Fanout/Actor-Isolation evaluieren

Backend-weiten, inhaltlich schmalen Hinweis nur dann durch actor-scoped Fanout ersetzen, wenn belastbare Leistungs-/Privacy-Anforderungen und ein schlanker Weg nachgewiesen sind. Keine ungefragten zusätzlichen Rechte- oder Aktivierungsmechanismen.

### DEP.H1 — Installations-Herkunft vor realer Abnahme klären

Bei unbekanntem Alt-Binary nur nach einem belegten Alt-Neu-Vertrag installieren; VDR/Media Tools/Pairing/Aufnahmen/SQLite unverändert erhalten. Bestehenden Objekt-Cache, geprüfte Binaries und Rollback-Weg wiederverwenden. **Kein automatisches DB-Backup, keine neuen Branches/Worktrees, kein `make clean`, keine CI-Warteschleife und kein Umgehen echter Datenrisiken.**

## Grenzen und Abschlusskriterien

**Nächster Arbeitsauftrag = nur SSE.H1**, nicht alle Slices gleichzeitig. Kein Refactoring von Media Tools, kein API-v1-Neuentwurf, keine neue Datenbank, kein unerlaubter Git-Branch, keine neue Phase 70. Produktcode nur auf geprüfter Basis des bestehenden `main` bearbeiten und `AGENTS.md` beachten. Gezielte C++-Tests, Browser-Verträge und Device-Kompatibilität prüfen; offene real-yaVDR-Abnahme klar kennzeichnen. Kein CI-Warten als Sperre.

**Code-Einstiege:** `core/vdr/src/{SnapshotChangeFeed,SnapshotChangeFeedService,SseLiveTransport}.cpp`, `api/rest/src/LiveTransportController.cpp`, `core/daemon/src/DaemonRuntime.cpp`, `web/frontend/api/live-remote-client-api.js`, `web/frontend/{home-recording-discovery,home-continue-watching,recordings2-folder-refresh}.js`, `web/frontend/api/continue-watching-sync.js`.

**Test-Einstiege:** `core/vdr/tests/test_snapshot_change_feed.cpp`, `core/vdr/tests/test_sse_live_transport.cpp`, `api/rest/tests/test_live_transport_controller.cpp`, `web/frontend/tests/test_home_continue_watching_sse.js`.

*Stand der Belege: reine Repository-Quellprüfung am 04.11.2026; kein realer Last-/Hardwarelauf in diesem Dokument.*
