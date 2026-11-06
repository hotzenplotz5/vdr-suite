# SSE.H — Live-Transport und „Weiterschauen“ härten (04.11.2026)

**Status (06.11.2026):** **SSE.H1/H2 und CW.H2: funktionale yaVDR-Abnahme durch den Operator PASS.** Der aktuelle Daemon wurde mit dem verifizierten Installer von `a8b19c56a804` auf `b45c0cfe4a4` aktualisiert (`DAEMON_INSTALL=PASS`, Service `active`, kein Datenbank-Backup). Der Operator meldete PASS für direktes Fortsetzen, „Von vorn“, Tonende beim Zurückgehen und browserübergreifende Continue-Watching-Aktualisierung ohne manuelles Neuladen. **SSE.H3: Evaluierung abgeschlossen; keine Codeänderung angezeigt.** Gesonderte Reconnect-/Langzeitlast- und Mehrbenutzer-Messungen sind dadurch nicht belegt. Frühere Zeitstandsnotizen unten dokumentieren den ursprünglichen Quellbefund und sind nicht mit dem aktuellen Installationsstatus gleichzusetzen.

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
| SSE.H1 | Retention (256 Frames/512 KiB), epochengebundenes Delta und explizites Resync implementiert. Der Browser-Socket wurde in SSE.H2 langlebig gemacht. | Funktionale yaVDR-Abnahme mit SSE.H2 **PASS** (Operator); separater Replay-/Langzeitlasttest im Feld nicht nachgewiesen. |
| SSE.H2 | Browser-Rails teilen eine EventSource pro Dokument; der Server nutzt eine bounded SSE-Queue mit Heartbeat und erneuter HTTP-Authentisierung nach maximal zwei Minuten. | Produktiv installiert; browserübergreifende Live-Aktualisierung laut Operator **PASS**; separater Socket-Dauerlasttest offen. |
| CW.H1 | **Codepfad bestätigt, sichtbare Nutzerwirkung offen:** `continue-watching-sync.js::syncProgress()` überspringt bei `position === lastPosition` auch eine geplante Abschluss-`notifyLive`-Mutation. Das heißt **nicht**, dass gespeicherte Daten verloren gehen. | Sollte ein Stop/Pause bei unveränderter Position einen zusätzlichen bestätigten Status auslösen? Test mit zweitem Client. |
| CW.H2 | „Von vorn“-Ablauf ist in den folgenden Code-/Testkorrekturen gegen die frühere Clear/Start-Race gehärtet. | Fortsetzen, Von-vorn-Start und Stop/Zurück laut Operator **PASS**; zusätzliche konkurrierende Device-CAS-Lastfälle nicht als Feldtest belegt. |
| SSE.H3 | Backendweite Benachrichtigung enthält `backendId`, Sequenz/Generation und Domäne, aber weder Actor-ID noch Wiedergabeposition. Ein fremder authentifizierter Browser kann indirekt eine Aktivität erkennen und einen unnötigen eigenen Readback auslösen; dieser Readback ist actor-scoped. | **EVALUIERT: KISS/BEIBEHALTEN**; actor-scoped Fanout erst bei gemessener Last oder verbindlicher strengeren Privatsphäre-Anforderung. |
| DEP.H1 | Der frühere Provenance-Blocker wurde zwischenzeitlich geklärt; der Installer erkannte `a8b19c56a804` als Altstand und `b45c0cfe4a4` als Ziel. | **AUF yaVDR ERLEDIGT:** versiegelter Kandidat, `DB_BACKUP_DECISION=NOT_REQUIRED`, `DAEMON_INSTALL=PASS`, laufender Service. |

## Umsetzbare Slices (Reihenfolge)

### SSE.H1 — Retention und korrektes Delta-/Gap-Replay (funktionale yaVDR-Abnahme 06.11.2026)

**Ziel:** Begrenzte Change-Feed-/SSE-Historie, keine erneute Komplettübertragung bei jedem normalen Reconnect, fehlende Events zuverlässig erkennbar.

1. Erst die echten Verwendungen von `SnapshotChangeFeed`, `SseLiveTransport`, `LiveTransportController`, `SimpleHttpListener` und Device-Live-Hub erfassen. Bestehende Sequenznummern, Client-Cursor, Auth-/Backend-Grenzen und Tests identifizieren.
2. Ein **begrenzt** aufbewahrtes Ereignisfenster einführen, monoton sequenzieren und erste/letzte verfügbare Sequenz eindeutig machen. Keine unabhängigen konkurrierenden Event-Speicher erfinden.
3. Gültige Cursor: nur neuere Einträge; veraltete, ungültige oder nach Daemon-Neustart unbekannte Cursor: expliziter Catch-up-/Resync-Vertrag statt stiller Datenlücke. ADR-0018 berücksichtigen. Web- und Device-Konsumenten kompatibel halten.
4. Ob der bestehende HTTP-Server echte langlebige SSE-Verbindungen unterstützt, im Code nachweisen. **Kein Pseudo-Streaming mit Endlosschleife.** Falls hierfür ein wesentlicher HTTP-Umbau nötig wäre, einen getrennten, zusammenhängenden Slice vorschlagen statt den Umfang von SSE.H1 still zu erweitern.
5. Gezielte Tests: 0/1/viele Events, Pufferüberlauf, alter/aktueller/ungültiger Cursor, Neustart, Reihenfolge, Authentisierung, Backend-Isolation, Reconnect, Speicher-/Antwortlimit. Keine neuen zyklischen Frontend-Polling-Fallbacks.

**Abnahme:** Lange Ereignishistorien lassen Speicher und Replay-Body nicht unbegrenzt wachsen; normales Replay ist inkrementell, Lücken werden erkennbar und führen zu einmaligem kanonischen Readback.

**Tatsächlich implementiert in Code-Commit d1085f50c42b32488b667cc28bfdd01f0ad63ded:**

- SnapshotChangeFeed begrenzt die Eintragszahl auf **256** und den größengewichteten Retentionsumfang auf **512 KiB**. Die höchste bekannte Sequenz/Generation bleibt auch nach clear() oder einem nicht aufbewahrbaren Eintrag erhalten; eine frühere Sequenz wird innerhalb des Daemon-Lebenszyklus nicht erneut vergeben. Die Grenze ist kein gemessener RSS-Grenzwert.
- SseLiveTransport begrenzt gespeicherte Frames auf **256** bzw. **512 KiB serialisierte Frames**, leitet dieselben LiveUpdateEvents weiterhin an den vorhandenen Device-Hub weiter und verwendet für die Browser-SSE-ID einen neuen Daemon-Epoch-Präfix (sse_<epoch>:<sequence>). Der JSON-Payload behält seine bestehende numerische sequenceNumber, backendId und changedDomains.
- Die vorhandene authentisierte TestHttpServer-Route übergibt den HTTP-Header **Last-Event-ID** über ApiRouter an LiveTransportController. Ein gültiger Cursor erhält nur neuere, lückenlos aufbewahrte Frames; ein aktueller Cursor erhält keine erneut ausgegebene Historie. Fehlende, ungültige, zu alte oder zu einer anderen Epoch gehörende Cursor lösen ein ausdrückliches event: resync mit neuer Epoch-/Sequenzposition aus.
- Die vorhandenen Browser-Konsumenten home-continue-watching.js, home-recording-discovery.js und recordings2-folder-refresh.js reagieren auf resync mit ihrem bestehenden kanonischen Readback und dessen bereits vorhandener Refresh-Zusammenfassung. Ein leerer gültiger Replay-Body wird nicht mehr fälschlich als Daemon-Reset ausgelegt. Keine zusätzliche Frontend-Intervallabfrage, API-Version, Statusdatenbank, Worker-Änderung oder Berechtigungsstufe.
- **Device-SSE unverändert:** Die Route /api/v1/events behält ihren separaten echten Stream-Writer, ihren grant-/backend-spezifischen Subscription-Cursor, ihr Queue-Limit und ihre Device-Epoch. Der Browser-Cursor wird nicht auf die Device-Route übertragen.

**Tests ergänzt:** test_snapshot_change_feed, test_sse_live_transport, test_live_transport_controller, produktive Router-Verbindung in test_api_router, Auth-/Header-Route in test_test_http_server und ein resync-Koaleszenzfall in test_home_continue_watching_sse.js. **Bestätigt:** test_snapshot_change_feed = PASS (VDR-Suite-CI zu d1085f50). **Noch ausstehend:** die übrigen SSE-spezifischen Tests, kompletter Daemon-Link, Browser-Socket-Abnahme. **UNIT/CONTRACT = PARTIAL**, **PRODUCTION-INTEGRATION = NOT_VERIFIED**, **CI = PARTIAL / initial fehlgeschlagen**, **REAL-VDR = NOT_VERIFIED**. Der verbundene GitHub-Zugriff kann den Remote-Quellstand prüfen und committen, aber kein C++-Programm auf dem yaVDR-Host ausführen. Ein lokaler Daemon-Build oder bestandene Tests werden deshalb nicht behauptet.

**Abweichungen im ersten CI-Durchlauf und gezielte Korrektur (b950b20a):** Der Frontend-Architekturcheck wies die 240-Zeilen-Obergrenze für recordings2-folder-refresh.js zurück. Der Genre-Architekturcheck erkannte seine erwartete Legacy-GET-Fallback-Schreibweise in ApiRouter.h nicht. Die Korrektur reduziert den Resync-Listener auf den vorgegebenen Modulrahmen und lässt die bestehende Legacy-Fallback-Route für alle normalen GET-Requests wieder ausdrücklich stehen; Last-Event-ID wird nur für die Live-Route weitergegeben. Die entsprechenden statischen Guard-Ausdrücke wurden nach dem Fix gegen die Remote-Dateien erneut geprüft: **PASS** (239 effektive Dateizeilen, Router-Reihenfolge und Legacy-Token vorhanden). Der erste Gesamt-CI-Lauf war noch rot; der **neue Gesamt-CI-Lauf ist nicht abgewartet oder als grün behauptet**.

**Bewusste HTTP-Grenze:** SimpleHttpListener unterstützt echte lange Device-SSE-Sockets über DeviceEventResponseWriterPool. Die Browser-Route /api/vdr/live nutzt diesen Sonderpfad nicht: serializeResponse() schreibt weiterhin Content-Length und Connection: close. Browser-EventSource verbindet sich nach der endlichen Antwort automatisch wieder; der Replay-Body ist jetzt begrenzt und inkrementell, der Browser hat aber **noch keinen dauerhaft offenen Push-Socket**. Eine Einbindung des Browser-Streams in die existierende Writer-Lebenszyklusverwaltung wäre ein gesonderter, getesteter HTTP-Slice außerhalb von SSE.H1. Bis dahin ist dies kein Nachweis für durchgehende Echtzeit-Push-Zustellung.

**Offene Abnahmerisiken:** C++- und JavaScript-Tests tatsächlich ausführen; Browser-/Header-/Authentisierungs- und Backend-Abgrenzung auf installiertem Candidate verifizieren; RSS- und Antwortgrößen unter längerer Last messen; parallele Home-Rails einschließlich Serien-Rail und Recording Discovery prüfen. Produktive yaVDR-Wirksamkeit erfordert **einen neuen Daemon-Build und kontrollierte Installation** der geänderten C++-Quellen. Der getrennte DEP.H1-Altbinary-Provenance-Blocker gilt vor Live-Deployment weiter. Kein automatischer DB-Backup.

**Aktueller Status: funktionale yaVDR-Abnahme PASS (06.11.2026); der vorherige Quellprüfungsstand oben ist historisch.**


### SSE.H2 — Gemeinsamer Browser-SSE-Bezug

**Implementiert; funktionale yaVDR-Abnahme PASS am 06.11.2026.** `createClientLiveUpdateSource()` liefert disposable Rail-Subscriptions auf eine gemeinsame authentifizierte Browser-EventSource pro Web-Dokument. Beim letzten Abonnenten wird sie geschlossen; Logout und Sessionverlust schliessen den physischen Socket sofort, lassen aber die inaktiven Rail-Handles fuer eine moegliche neue Anmeldung erhalten. Eine geaenderte Session erneuert die Verbindung und loest einen einmaligen kanonischen Resync aus. Operation- und Device-SSE bleiben getrennt.

Server: `SseLiveTransport::subscribeBrowser` verbindet das bestehende `Last-Event-ID`-Replay mit einer bounded Queue (64 Frames/1 MiB pro Subscription), ohne zweite persistente Ereignisquelle. Bei Queue-Ueberlauf oder Sequenzluecke wird `resync` gesendet und die Verbindung getrennt. `TestHttpServer` erstellt eine Browser-Subscription ausschliesslich nach dem regulaeren SecurityHttpGate und erfolgreichem GET. `SimpleHttpListener` schreibt SSE ohne `Content-Length` in einem eigenen 4-Worker-Pool, sendet Heartbeats und beendet jeden Stream nach maximal zwei Minuten fuer erneute HTTP-Autorisierung.

**Tests im Repository:** `node web/frontend/tests/test_shared_browser_sse.js`, `make test-sse-live-transport test-test-http-server`, und der Socket-Test `make test-device-live-http-stream`. Noch nicht belegte Abnahme: ausgefuehrte Regressionen, mehrere gleichzeitige Rails, Browser-Proxy, Reconnect und yaVDR im laufenden Betrieb. Kein weiterer Statusspeicher, kein Polling-Fallback, kein automatischer DB-Backup.

### CW.H1 — Bestätigte Stop-/Abschluss-Semantik

Reproduktion zuerst. Falls ein zusätzlicher Abschlusszustand fachlich notwendig ist, nur nach bestätigtem Core-/CAS-Ergebnis melden. Keine künstliche Progress-Mutation oder zweite Datenwahrheit. Tests: gleiche/geänderte Position, Ende, Clear, Konflikt, zweiter Browser/Device.

### CW.H2 — „Von vorn“ ohne Race Condition

Clear und neue Wiedergabesitzung korrekt ordnen, Fehler sichtbar machen und CAS-Revisionsschutz weiterverwenden. Tests: verzögerter Clear, fehlgeschlagener Clear, schneller Doppelklick, konkurrierender Device-Fortschritt.

### SSE.H3 — Fanout/Actor-Isolation (Evaluation abgeschlossen 06.11.2026)

**Befund aus Produktcode:** `ContinueWatchingApiRuntime::tryHandlePost()` bzw. `publicProgressUpdate()` schreibt Fortschritt actor-scoped in die vorhandene Repository-/CAS-Schicht. Für SSE werden nur betroffene `backendId`-Werte in einer `std::set`-Queue zusammengefasst. `DaemonRuntime` erzeugt daraus die Domäne `continueWatching`; `LiveUpdateEventJsonSerializer` überträgt Backend, Sequenznummer, Snapshot-Generation und Domänen, aber weder Actor-ID noch Titel oder Abspielposition. `home-continue-watching.js` filtert nach ausgewähltem Backend und lädt bei einem Treffer die eigene Liste authentifiziert nach. `SseLiveTransport::subscribeBrowser()` führt globales Cursor-Replay aus und ist nicht actor-scoped.

**Abwägung:** Es gibt eine geringe Metadaten-Offenlegung: ein anderer bereits authentifizierter Benutzer am selben Backend erkennt, *dass* irgendein Fortschrittsereignis aufgetreten ist, aber nicht, welcher Benutzer welche Aufnahme wo abgespielt hat. Zusätzlich kann jede offene Home-Ansicht eine unnötige, actor-scoped Listenabfrage auslösen. Eine relevante Laststeigerung ist bislang nicht gemessen. Für actor-scoped Fanout wären Identity-Bindung in der Browser-Subscription, Filterung von Live-Replay und Queue bei Reconnect sowie Absicherung von Sessionwechseln und Device-/Browser-Grenzen nötig. Der Aufwand und das Regressionsrisiko rechtfertigen sich aktuell nicht.

**Entscheidung: EVALUIERT / KEIN PRODUKTCODE-UMBAU.** Den bewährten backendweiten Hinweis beibehalten; keine neuen Rechte, Statusspeicher oder Polling-Fallbacks. Eine erneute Entscheidung ist nur bei einer nachgewiesenen Last-/Skalierungsschwelle oder einer verbindlichen Anforderung nötig, dass selbst die Existenz fremder Wiedergabeaktivität nicht erkennbar sein darf. Dann zuerst gezielte Mehrbenutzer-/Reconnect-Tests und das bestehende Authentisierungskonzept prüfen.

### DEP.H1 — Installations-Herkunft vor realer Abnahme klären

Bei unbekanntem Alt-Binary nur nach einem belegten Alt-Neu-Vertrag installieren; VDR/Media Tools/Pairing/Aufnahmen/SQLite unverändert erhalten. Bestehenden Objekt-Cache, geprüfte Binaries und Rollback-Weg wiederverwenden. **Kein automatisches DB-Backup, keine neuen Branches/Worktrees, kein `make clean`, keine CI-Warteschleife und kein Umgehen echter Datenrisiken.**

## Grenzen und Abschlusskriterien

**Historischer Scope der Erstuntersuchung war SSE.H1.** Der aktuelle Stand (06.11.2026) enthält die funktionale SSE.H1/H2- und CW.H2-Abnahme sowie die abgeschlossene SSE.H3-Evaluierung, ohne neue Produktcodeänderung. Kein Refactoring von Media Tools, kein API-v1-Neuentwurf, keine neue Datenbank, kein unerlaubter Git-Branch, keine neue Phase 70. Produktcode nur auf geprüfter Basis des bestehenden `main` bearbeiten und `AGENTS.md` beachten. Gezielte C++-Tests, Browser-Verträge und Device-Kompatibilität prüfen; offene real-yaVDR-Abnahme klar kennzeichnen. Kein CI-Warten als Sperre.

**Code-Einstiege:** `core/vdr/src/{SnapshotChangeFeed,SnapshotChangeFeedService,SseLiveTransport}.cpp`, `api/rest/src/LiveTransportController.cpp`, `core/daemon/src/DaemonRuntime.cpp`, `web/frontend/api/live-remote-client-api.js`, `web/frontend/{home-recording-discovery,home-continue-watching,recordings2-folder-refresh}.js`, `web/frontend/api/continue-watching-sync.js`.

**Test-Einstiege:** `core/vdr/tests/test_snapshot_change_feed.cpp`, `core/vdr/tests/test_sse_live_transport.cpp`, `api/rest/tests/test_live_transport_controller.cpp`, `web/frontend/tests/test_home_continue_watching_sse.js`.

*Belegstand: historische Quellprüfung ab 04.11.2026; produktive Daemon-Installation und funktionale Operatorabnahme vom 06.11.2026. Ein Langzeitlast-/Mehrbenutzer-Messlauf ist nicht belegt.*
