# Public API v1 – einmalige End-to-End-Abnahme am realen VDR

**Status: VORBEREITET – EINZELNE HISTORISCHE READ-ONLY-/HARDWARE-TEILNACHWEISE VORHANDEN; GEMEINSAME ENDABNAHME NICHT AUSGEFÜHRT.**

**Verbindliche Arbeitsregel (22.10.2026):** Erst die gesamte API-v1-Implementierung vollständig integrieren, dann **eine gemeinsame, nachvollziehbare Endabnahme am echten VDR**. Keine separaten, wiederholten Hardware-Slices je API-Funktion. Automatische Source-CI bei Commits ersetzt die Endabnahme nicht. Eine neue Änderung nach Abnahme erfordert nur die betroffenen Regressionen und die zentralen Pflichtprüfungen, nicht einen Neustart der gesamten Entwicklung.

Diese Checkliste ist der **einzige operative Hardware-Abnahmeplan** für API v1. Bestehende fachliche ADRs, OpenAPI und `docs/CURRENT.md` bleiben Vertrags- bzw. Statusquellen; bei Abweichung den offenen Punkt hier notieren, keine zweite parallele Checkliste anlegen.

## Integration-Checkpoint auf `main` (23.10.2026; keine Endabnahme)

| Prüfebene | Nachweis / Status | Grenze und nächste Prüfung |
| --- | --- | --- |
| Quellcode integriert | R1–R3, ADR-0068 (#517), E1 (#519), E2, M1, M2, S1, P1 [#524](https://github.com/hotzenplotz5/vdr-suite/pull/524) merged at `052350714f5b5904a969158925cb05d8f39aed78`; Unified Acceptance Runner [#526](https://github.com/hotzenplotz5/vdr-suite/pull/526) merged at `096a04ad0e6a4eb2555292929d1373df549780bc` | Keine Behauptung einer durchgehenden produktiven Runtime- oder installierten Release-Kette ohne Code-Inventur |
| Produktivpfad verdrahtet | PR #526 beschreibt die bestehende native SearchTimer UPDATE/ACTIVATE/DEACTIVATE/DELETE-Kette, TimerIntent/Assignment atomare Aufnahme/Agent-/native Readback-Kette sowie P1-Source-Komponenten | Am gemeinsamen HEAD anhand Runtime-Komposition, SecurityHttpGate, Grants, Agent/SuiteBridge, Browser/Device/JS und OpenAPI einzeln nachprüfen; SearchTimer CREATE nutzt weiterhin RESTfulAPI, **keine** native epgsearch-CAS-CREATE-Garantie |
| Tests / CI | PR #526 nennt die erfolgreichen exakten PR-Head-Läufe [VDR-Suite CI #40008988460](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40008988460) und [Unified Runner #40008988457](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40008988457); SearchTimer CREATE Regression explizit **mock only**. | Ergebnisse gelten für den jeweiligen getesteten HEAD, nicht als automatisch erneute Tests am dokumentationsaktualisierten `main` oder reale Gerätetests; exakte Integrations-SHA-Nachweise vor Release ermitteln |
| Reales yaVDR | Die untenstehenden bestehenden GET-/Security-/SQLite-Read-only-Teilnachweise gelten mit ihrer jeweiligen installierten Build-Identität; ein früher eng begrenzter EPG/Recording-Hardwarefall ist separat historisch akzeptiert | Kein Deployment des gemeinsamen Kandidaten nachgewiesen; 30 PASS / 7 SKIP / 17 BLOCKED der älteren GET-Probe bleiben unberührt; vier `outcome_unknown`-Operationen nicht erneut ausführen |
| Offen/blockiert | Finaler einheitlicher Release-Kandidat, Browser↔Device/VIDAA-Playback, vollständige echte Operationen/Readbacks, sichere SearchTimer CREATE Freigabegrenze, Agent-Boot/Recovery | Keine produktiven Mutationen, kein Neustart, keine DB-Reparatur ohne eigene Freigabe; diese **eine** Matrix bleibt maßgeblich |

## Abgleich der bereits akzeptierten yaVDR-Evidenz (24.10.2026; noch keine Gesamtfreigabe)

**Quellprovenienz:** Der geprüfte und auf dem yaVDR installierte Produkt-SHA ist
`463275be53e05db2fd5f09b02d3c98648e82cec0`; GitHub-`main` steht
bei `9cf93fc231fd34be4732caa9d83ae9be843e4e8b` (sechs nachfolgende
Commits; keine Divergenz). Der Versionsstring `0.14.1` identifiziert keinen
Build. Die separate installierte Binäridentität war zuvor nachgewiesen; die
Übertragung dieser Identität auf den neueren `main` ist **nicht** bewiesen.

**Änderungswirkung des sechs-Commit-Deltas:** (a) `mk/install.mk` liefert
`api/session-frontend-sync.js`, `platform/public-url.js` und
`home-recording-discovery.js` künftig mit; die drei Dateien waren auf
dem manuellen yaVDR-Install bereits bytegenau mit dem geprüften SHA verglichen
(3/3 PASS), sodass fehlende `dpkg-query`-Eigentümerschaft **kein Fehler** ist.
(b) `web/frontend/tests/test_live_tv_eof_recovery.js` prüft nun die
Produktiv-Bundle-Reihenfolge und installierte HTML-Script-Abhängigkeiten; das
Session-Modul wird über `channel-day-program.js` eingebunden, nicht zusätzlich
als eigenes HTML-Script. (c) `SuiteBridgeLocalControlTransport` meldet
`VideoDirectory` auf dem lokalen Socket explizit als unverfügbar, weil
`VPATH 1` den SVDRP-Transport voraussetzt, und besitzt eine gezielte
Unit-Regression. Die produktive `DaemonRuntimeInitialization` verwendet
für die VPATH-Abfrage ausdrücklich `SuiteBridgeSvdrpTransport`. Ob der
laufende Host den nativen Video-Root tatsächlich erhält und auswählt,
bleibt ohne neue Live-Evidenz offen. Bei unverfügbarem VPATH ohne
explizite Konfiguration existiert ein Legacy-Root-Fallback: Vor
Recording-Schreibaktionen dessen tatsächliche Übereinstimmung mit dem
VDR-Videoverzeichnis kontrollieren, keine Annahme aus der Source.
(d) unbenutzte Helper für NativeTimerDeleteAssignment wurden entfernt;
die Klammerung einer bestehenden Deadline-Bedingung verändert ihre Semantik
nicht. Die alten API-Read-PASS sind davon nicht pauschal entwertet.
Für dieselbe unveränderte Produkt-Codebasis liegen mittlerweile grüne
Source-CI-Läufe vor: [VDR-Suite CI #11052](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40083986261)
für Matrix-Branch `908dbd7e696064f7c3c227eaac10f23600739774` und
[VDR-Suite CI #11054](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40084664261)
für den gesonderten Operation-ID-Runner-Fix-Branch
`d341ee49e1535de0140f01784f231bc0ae85ed68`, jeweils
**alle sechs Jobs SUCCESS**, inklusive Fast-Regression, Packaging,
Frontend und Architektur. Die produktiven C++-/Frontend-Installations-
Änderungen liegen in beiden Fällen unverändert gegenüber `main`.
Das ist Source-/CI-Evidenz; ein versiegeltes LIVE-Deployment
(`RUNTIME_DEPLOYMENT_MATCH=YES`) oder eine native VDR-ABI-Prüfung
wurde dadurch **nicht** erbracht.

**Getrennte Evidenzebenen, ohne überlappende Fallzahlen zu addieren:**

- `UNIT/CONTRACT`: Frühere API-v1-Vertragsbaseline 19/19 PASS und
  sieben fokussierte Blöcke mit insgesamt 59 PASS wurden akzeptiert;
  diese Zahlen können sich überschneiden. Operation-ID: kompatible
  unbekannte ID `op_invalid` -> HTTP 404; 161 Zeichen -> HTTP 400,
  **2 PASS, 0 FAIL**. Bereits persistierte historische Operations-IDs
  bis 160 Zeichen bleiben gültig; der neue ID-Ausgabeformatvertrag darf
  nicht rückwirkend als Parserzwang gelten.
- `PRODUCTION-INTEGRATION`: Installierte Daemon-Quellidentität zuvor
  separat geprüft; drei benannte manuell installierte Frontend-Dateien
  **3/3 bytegenau PASS**. Dies ersetzt weder einen kanonischen
  `RUNTIME_DEPLOYMENT_MATCH=YES`-Nachweis für den *neuen* Kandidaten
  noch das komplette Browser-/Agent-/Client-Ende-zu-Ende.
- `CI`: Frühere erfolgreiche exakt gepinnte PR-#526-Läufe bleiben
  gültig für ihre damalige Source-Eingabe. Am unveränderten
  `main`-Produktcode `9cf93fc...` sind auf den abgeleiteten Branches
  nun [CI #11052](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40083986261)
  (Matrix-Doku) und
  [CI #11054](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40084664261)
  (separater Runner-/OpenAPI-Fix) mit jeweils **6/6 erfolgreichen Jobs**
  abgeschlossen. [API-v1-Acceptance Runner #64](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40084664255)
  ist für den Fix-HEAD ebenfalls **SUCCESS**. Beide PRs sind noch Draft,
  nicht nach `main` gemergt. Die GitHub-Commit-Statusabfrage allein
  belegt weiterhin keine weiteren nicht gelisteten CI-Gates. Ein CI-PASS
  ersetzt keine REAL-VDR- oder installierte Build-Evidenz.
- `REAL-VDR`: Ergänzend zu den älteren historischen Proben sind akzeptiert:
  Device-Lifecycle, ETag/304, ohne Auth/401; Human Credential und Session
  Collection/Detail/304; Pending Pairings (0); Admin-Login/Logout:
  **12 PASS / 0 FAIL / 0 SKIP / 0 BLOCKED**. Playback-Account-Zuordnung
  des bereits gepaarten Hisense (Inventar, GET 200, JSON, no-store,
  nosniff, 401/400/404): **11 PASS / 0 FAIL / 0 BLOCKED**.
  **Kein verknüpfter Human Account** ist zulässiger Ist-Zustand,
  kein Fehler und kein Auftrag zur Account-Mutation. Ubuntu-SVDRP
  `LSTT`: **0 reale Timer**; Public-v1 `native-timers` für
  `default`: **0 Timer**, **Abgleich PASS**, Admin-Login/Logout
  PASS. Ein nicht vorhandener Timer kann kein Timer-Detail-/ETag-Fixture
  ersetzen. Frühere Discovery/Capabilities, Channels/EPG,
  EPG-/Recording-Suche, reale Artwork-Bytes, TimerAssignments,
  Grants/ETags und negative Auth-/Backend-GET-Prüfungen bleiben akzeptiert.
  Die ursprüngliche 30-PASS/7-SKIP/17-BLOCKED-Matrixprobe ist
  **nicht** der aktuelle kumulative Abschlussstatus.

**Historische Operations und Persistenz:** Vier `outcome_unknown`
(2x SearchTimer CREATE, 1x TimerAssignment/native CREATE,
1x NativeTimerBinding TOGGLE), sieben wechselseitig konsistente
bound/managed-Bindings und zwei `expected_transition` mit
`missing_since` bleiben wie zuvor klassifiziert. Das historische
native TOGGLE-Agent-Result enthält **keine typisierte
No-Effect-Evidenz** (Result-Evidence leer/NULL) und darf nicht aus
generischem `not_started / verified / rejected / fenced`
rekonstruiert werden. Kein Redispatch, keine automatische Recovery,
keine künstlich ergänzten Resultate, keine direkte SQLite-Reparatur.
Ein neues identisches SQLite-Aggregat ist **nicht** erforderlich.
Diese Altzustände sind ohne autoritative Evidenz **BLOCKED**, nicht
als frische Source-Fehlerdiagnose oder Erfolg zu werten.

**Entscheidungsabhängigkeiten bis zur Releasefreigabe:**

1. Exakten **gemeinsamen Produktkandidaten** nach Source-/CI-/Kompositions-
   und Installationsprüfung festlegen; geänderte Frontend-/Transportflächen
   fokussiert validieren, akzeptierte unveränderte Read-Tests nicht erneut
   ausführen. Der neue `main` ist *Source-Kandidat*, nicht automatisch
   der laufende Build.
2. **SearchTimer CREATE**: Der One-shot RESTfulAPI-Provider-Write
   mit SQLite-CAS, vollständigem Inventar und Readback hat weiterhin
   **keine native epgsearch-lock-atomare CREATE-CAS-Semantik**.
   Empfehlung: diese Garantie **nicht** für Public-v1 behaupten;
   für eine atomare Gesamtfreigabe entweder native CAS-CREATE implementieren
   oder CREATE technisch und vertraglich aus dem freigegebenen Umfang
   ausnehmen. Die Wahl ist vor Mutationsabnahme ausdrücklich zu genehmigen.
   UPDATE/ACTIVATE/DEACTIVATE/DELETE sind separat am gepatchten nativen
   Provider nachzuweisen.
3. Hardware-Mutationen erst **nach** Identität/Backup/Restore- und
   Freigabeentscheidung für genau bezeichnete harmlose Testobjekte.
   Danach echte TimerIntent-, NativeTimer-, TimerAssignment-,
   SearchTimer- und Recording-Aktionen einschließlich persistierter
   Operation-/Replay-/Konflikt-Readbacks gegen autoritative
   VDR-/SuiteBridge-/epgsearch-Wahrheit. Kein Eingriff in vorhandene
   Timer, Aufnahmen oder Regeln.
4. **VIDAA E2E:** Die vorhandenen 1.895 positiven historischen
   Device-SecurityGate-Ereignisse sind gültige Auth-Evidenz, aber
   keine HTTP-Payload-/Live-TV-/Recording-/Track-/Resume-Bestätigung.
   Für frische Device-HTTP-Anfragen ein vom **TV selbst** ausgeführter,
   autorisierter Request; kein Admin-Cookie als Device-Credential,
   keine Secret-Extraktion/Rotation/Neukopplung. Human↔Device-Resume
   nur bei separat erlaubter Account-Zuordnung.
5. Negativ-, Parallelitäts-, Restart-, Recovery- und Rollbackfälle
   pro tatsächlich unterstütztem Schreibvertrag, einschließlich
   Ausfall vor/nach Backend-Write, bleiben mit Source-/Hardwarebelegen
   zu schließen; alte Unknowns sind kein geeignetes Test-Fixture.
   Finales Freigabeprotokoll verlangt exakte Candidate-/CI- und
   REAL-VDR-Belegkette, nicht nur HTTP 200.

**Source-Korrektur an echter Abnahmeabweichung:** Separater Draft
[PR #537](https://github.com/hotzenplotz5/vdr-suite/pull/537) am
HEAD `e4d9976840823b39fb7af2bec27de7bf38932f28`
gleicht den read-only Acceptance-Runner an historische persistierte
Operation-IDs an, ohne das neue `op_`-Ausgabeformat oder den produktiven
Daemon zu ändern. Neue und historische sichere ID-Fixtures (bis 160
ASCII-Bytes), negatives `161`-/Query-/Pfadverhalten und insbesondere
die gefährlichen eigenständigen Pfadsegmente `.` und `..` sind durch
OpenAPI-/Runner-/README-Abgleich gedeckt;
[Runner CI #66](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40086067582)
**SUCCESS**. [Source-CI #11054](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40084664261)
war für den vorausgehenden Code-HEAD grün; für den letzten
Pfadsegment-Test-Fix läuft die neue exakte
[Source-CI #11057](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40086067554)
und ist vor Merge als gesondertes Gate zu beurteilen.
Diese Offline-Korrektur bestätigt **keine** unbekannte
historische Operation und begründet **keine** erneute Hardware-Probe.
Vor Release den getrennten Fix nach Review in den gemeinsamen Kandidaten
integrieren; derzeit hat `main` den Fix **noch nicht**.

**OpenAPI-Deckung (Source-Inventar):** Alle **27/27** im bestehenden
`tools/api-v1-acceptance/cases.py`-Runner erforderlichen Routen
sind als Pfade in `api/openapi.json` vorhanden. Dies ist
**nur UNIT/CONTRACT-Inventar**. Die OpenAPI-Datei kennzeichnet sich
weiterhin als `partial`; ihre vollständige API-v1-Route-/Methoden-
und SDK-Parität wird durch den 27/27-Abgleich **nicht** behauptet.

**Operative TV-Zugangsgrenze am 24.10.2026:** Der Betreiber hat derzeit **keinen Zugriff auf den Hisense-TV**. Alle aktuellen VIDAA-/Device-HTTP-/Live-TV-/Playback-/Resume-Handprüfungen sind deshalb ausdrücklich **BLOCKED: TV_ACCESS_UNAVAILABLE**, nicht FAILED und nicht erneut anzufordern. Die bereits akzeptierten 1.895 historischen Device-Gates bleiben unverändert. Bis der Betreiber von sich aus wieder TV-Zugriff bestätigt, ausschließlich unabhängige Source-, CI-, Kompositions-, Security- und dokumentarische Arbeiten fortsetzen. Kein neuer TV-Testblock und kein Credential-/Pairing-Ersatzversuch.

**Safety Gate:** `RELEASE=BLOCKED` bis diese externen Entscheide
und produktiven Pfadnachweise vorliegen. Keine Installation, kein Build,
kein VDR-Neustart, keine Mutation und keine Account-Verknüpfung durch
diese Dokumentationsfortschreibung.

## 0. Vorbedingung: Der Implementierungsumfang ist geschlossen

Alle Kästchen dieses Abschnitts müssen vor dem ersten realen VDR-Mutationstest abgehakt sein. Ein vorhandener Branch, ein grüner Unit-Test oder eine syntaktisch vorhandene Route gilt **nicht** als integrierte Funktion.

- [ ] Sämtliche für die API-v1-Freigabe vorgesehenen Implementierungen sind im gemeinsamen Integration-HEAD enthalten und durch **eine** API-v1-Versionsmatrix abgedeckt (keine schwebenden konkurrierenden Branch-Versionen).
- [ ] OpenAPI, Referenz-JS-Client, Auth-/Grant-Modell, Daemon-Komposition, Provider-/Agent-Transport und tatsächlich installierte Paketinhalte entsprechen demselben Git-Commit.
- [ ] Source-CI, Build der nativen Plugins und Release-/Packaging-CI sind grün für **genau** diesen HEAD; kein älterer erfolgreicher Lauf als Ersatz.
- [ ] Native Suchregel-Schreiboperationen UPDATE/ACTIVATE/DEACTIVATE/DELETE verfügen über epgsearch-lock-atomaren Generations-/Revisionsvergleich, persistierte Metadaten und readback-verifizierte Operationen; kein unsicherer Write-Fallback. **Abweichung bewusst offen:** inaktives SearchTimer CREATE nutzt RESTfulAPI als einmaligen Provider-Write; die Generation-/Readback-Gates machen CREATE nicht nativ atomar. Vor API-v1-Freigabe ausdrücklich entscheiden: native CAS-CREATE als Pflicht umsetzen oder CREATE aus der freigegebenen atomaren API-v1-Scope ausschließen und Einschränkung konsistent in OpenAPI/Release-Notizen festhalten.
- [ ] Backend-Anbindung funktioniert durchgängig: Public-v1-Route → Device-/Browser-Gate → Backend-Grant → dauerhaft reservierter Operation-Datensatz → Agent-/SuiteBridge-Transport → native epgsearch-`Service()` → authoritative readback → Operation-Abschluss. **Keine** vorbereitete Mock-Provider-Schnittstelle wird als produktiv verdrahtete Implementierung gewertet.
- [ ] Echte Provider-Capability wird nur bei kompatiblem epgsearch/SuiteBridge aktiviert. Ist sie nicht nachweisbar, sind UPDATE, Aktivieren, Deaktivieren und Löschen explizit geschlossen (Fehler statt unsicherem Fallback).
- [ ] Ein einziger Release-Kandidat, ein Change-Log und eine Rückfallstrategie stehen fest. Keine Änderung an produktiven Daten vor dokumentierter Backup-Entscheidung.
- [ ] Die Freigabegrenzen und noch bewusst nicht implementierte Endpunkte sind in OpenAPI und Release-Notizen identisch dokumentiert.

**Stopp:** Solange eines dieser Kästchen offen ist, **kein** Hardware-Mutationstest. Dann weiter implementieren und Source-CI reparieren, aber keine neue Phase/Slice erfinden.

## 1. Vorbereitung – genau eine reale VDR-Testsitzung

- [ ] Git-Commit/SHA des Kandidaten: `______________________________`
- [ ] Datum/Zeitzone, VDR-Version, Plugin-Versionen/ABI, SuiteBridge-Version, Daemon-Version und Paketnamen festhalten.
- [ ] Aktuellen Zustand der Dienste, Backend-Zuordnung, Timer-/Suchregel-Anzahl sowie HTTP-/API-Erreichbarkeit **nur lesend** erfassen.
- [ ] Zielsystem verifizieren: Test findet ausschließlich auf dem ausdrücklich freigegebenen yaVDR statt; kein stillschweigendes Deployment aus einem Chat.
- [ ] Vorab entscheiden: Sicherung von `epgsearch.conf` samt Metadaten, VDR-Timern und Suite-SQLite; Pfade/Rechte/Owner/Checksumme dokumentieren. Datenschutzrelevante Inhalte nicht in öffentliche Logs kopieren.
- [ ] Explizite Freigabe für Testobjekte (eindeutig benannte, neu angelegte inaktive Test-Suchregel und je eine Testaufnahme/Test-Timer ohne reale Serienaufnahme). Bestehende echte Timer und Aufnahmen dürfen nicht gelöscht werden.
- [ ] Definiertes Rollback: Pakete/Services, epgsearch-Konfiguration und SQLite können auf den Ausgangszustand zurückgesetzt werden; Prüfpfade und Verantwortlichkeit stehen fest.
- [ ] Ein zentrales Ergebnisprotokoll führt **pro Prüffall** Request-ID, Zeitpunkt, HTTP-Status, korrelierte Operation-ID, nativen Provider-Status, erwartetes/tatsächliches Ergebnis und Beleg/Log-Ausschnitt.

## 2. Public API v1 – lesende Endpunkte und Vertragsprüfung

- [ ] `/api/v1` Discovery/Capabilities, versionspezifische OpenAPI-Pfade und nicht unterstützte Methoden: konsistente Statuscodes, kein Legacy-Fallback.
- [ ] Device-Pairing, Credential-Ausgabe, -Rotation, -Sperrung und Widerruf; negative Authentifizierung mit 401/403 ohne Nebenwirkungen.
- [ ] Backend-Auswahl/Grenzen: exakt berechtigter Backend-Umfang, fremde Backend-IDs abweisen, Offline-Backend als 503 statt leerem 200.
- [ ] Channels/EPG: korrekte Sender- und Ereignisidentität, Zeitfenster, Cursor/Pagination, Grenzwerte, stabile Reihenfolge und stale-event-Verhalten.
- [ ] EPG-Suche/Genres und globale Suche: EPG-/Aufnahmen-Abgrenzung, Filter/Pagination, eindeutige IDs.
- [ ] Timer-/TimerAssignment-/TimerIntent-GETs: geplante vs. wirklich native Timer korrekt unterscheiden, stabile Revisionen/ETags.
- [ ] SearchTimer-Collection/Detail/Preview/Draft-Preview: native IDs bleiben privat, EPG-Preview ist keine Aufnahmegarantie, korrektes Verhalten bei leerer Liste bzw. Provider-Ausfall.
- [ ] Recordings R1/R2/R3: Sammlung, Ordneransicht, Detail, Metadaten, Artwork, Schnittmarken, ETags, korrekte Backend-Bindung.
- [ ] Playback/Resume/ContinueWatching: Medien-URL-Berechtigung, Geräte-/Backend-Scope, Positionspersistenz und keine Weitergabe privater Dateipfade.
- [ ] JSON-Schema, Content-Type, Cache-Control/no-store bei geschützten Antworten, method-not-allowed, falsche UUID-/ID-Formate und unbekannte Ressourcen.
- [ ] Referenz-JS-Client und der vorgesehene VIDAA-Client verwenden ausschließlich die dokumentierten Public-v1-Verträge; keine versteckten alten Schreibendpunkte.

## 3. Public API v1 – schreibende Endpunkte (nur freigegebene Testobjekte)

- [ ] EPG → TimerIntent: gültiger ETag/EPG-Snapshot, einmalige native Erstellung, belastbares Readback; kein Doppel-Timer bei Replay.
- [ ] Native Timer: genehmigte CREATE/MODIFY/TOGGLE/DELETE-Proben, falls Bestandteil des Freigabekandidaten; aufnahme- bzw. laufende Timer nicht unabsichtlich anfassen.
- [ ] TimerAssignment: CRUD/Zuordnung und Revisionskonflikte nur in den öffentlich implementierten Teilverträgen.
- [ ] SearchTimer CREATE: ausschließlich explizit `active:false`, separate Berechtigung und bestätigte inaktive Anlage; keine implizite Aufnahmeplanung.
- [ ] SearchTimer UPDATE: inaktive Testregel, vollständiger erwarteter ETag und Generation, neue persistierte Revision, unveränderte Generation.
- [ ] SearchTimer ACTIVATE: frisch angezeigte, begrenzte EPG-Preview und separate explizite Aktivierungsfreigabe; Zahl gefundener Sendungen nicht als Zahl aufzunehmender Sendungen behaupten.
- [ ] SearchTimer DEACTIVATE: nur richtige native Generation verändern; Revision rotieren.
- [ ] SearchTimer DELETE: keine numerische-ID-only-Löschung, keine fremden VDR-Timer löschen; endgültige Abwesenheit derselben Generation nachweisen.
- [ ] Aufnahmen: RENAME/MOVE und erlaubte MARKS/CUT-Funktionen nur an freigegebenen Testobjekten; keine echte Aufnahme unbeabsichtigt löschen.
- [ ] Jede Mutation: durable Operation, Idempotency-Key, Scope/Actor/Backend, genau ein mutierender Provider-Aufruf, autorisierter Readback-Abschluss, korrekte Audit-Einträge.

## 4. Negative Sicherheits- und Fehlerfälle (Pflicht)

- [ ] Auth fehlt/falsch/widerrufen; Grant fehlt/falscher Backend-Scope; kein Schreibvorgang.
- [ ] Stale Revision/ETag; geänderte native Generation; gelöschte und gleichnamig/-inhaltlich neu angelegte Regel mit wiederverwendeter numerischer ID; kein falscher Schreibvorgang.
- [ ] Zwei parallele Aufrufe mit gleicher Operation/gleichem Idempotency-Key: höchstens **ein** nativer Write, deterministische Konflikt-/Replay-Semantik.
- [ ] Parallel veränderte Regel und ungespeicherte zweite Regel: keinerlei fremde Änderungen durch katalogweites `Save()` unbemerkt persistieren.
- [ ] Agent/Backend/SuiteBridge/epgsearch offline, versionsinkompatibel oder fehlende Fähigkeit: fail-closed, niemals EDIS/MODS/DELS-/RESTfulAPI-Fallback.
- [ ] Simulierter Transportabbruch vor/nach Write oder Persistenzfehler: `outcome_unknown`, **keine** blinde automatische Wiederholung; anschließende autoritative Ermittlung.
- [ ] Neustart von Daemon/VDR/epgsearch: Generation/Revisionskontinuität, Wiederanlauf verwaister Operationen, keine Phantom-Operationen.
- [ ] Bösartige/zu große Felder, ungültiges UTF-8, numerischer Overflow, doppelter JSON-Schlüssel und unzulässige Methoden: verworfen, ohne Nebenwirkungen.
- [ ] Readback mit unvollständigem/dupliziertem Inventar und Out-of-order-Antwort: niemals als erfolgreicher Abschluss bestätigt.
- [ ] OSD und Backend gleichzeitig: Native-Race mit noch geöffnetem OSD-Menü, keine Use-after-free-Situation, keine Änderungen an neu verwendeter ID.

## 5. Paket, Betrieb und abschließende Entscheidung

- [ ] Pakete installieren denselben geprüften Build; alle Binärdateien/Plugins haben passende VDR-ABI und erforderliche Laufzeitabhängigkeiten.
- [ ] Dienststart, Restart und Logs ohne neue kritische Fehler; Datenbankmigration bleibt idempotent.
- [ ] Keine unbeabsichtigten neuen Listener oder erhöhte Netzwerk-/Dateisystemrechte.
- [ ] Native Konfigurationsdateien und SQLite bleiben konsistent; Backups sind bei Bedarf wiederherstellbar.
- [ ] Referenz-Client und reale TV-App lesen/bedienen freigegebene Funktionen ohne Legacy-Write-Fallback.
- [ ] Negativtests nach Produkt-Neustart wiederholen, insbesondere Rechte-Widerruf und Generation/ETag.
- [ ] Testobjekte nach sicherem Readback entfernen; keine Test-Timer/Regeln/Operationen mit unerwünschter Folgewirkung zurücklassen.
- [ ] Alle Pflichtfälle und CI für den **exakten** End-HEAD sind grün; offene Einschränkungen werden als explizite Release-Blocker oder bewusst ausgeschlossen dokumentiert.
- [ ] Abschlussprotokoll mit Entscheidung: **Freigabe** / **Blockiert**, verantwortlicher Review und Datum. Erst dann PR-Review/Merge/Release gemäß ausdrücklicher Freigabe.

## Vorläufige reale yaVDR-Teilnachweise – keine Endabnahme (23.10.2026)

Diese Tabelle dokumentiert bereits vorhandene **produktive Read-only-Evidenz**, ohne die offene Endabnahme-Matrix unten vorzeitig abzuhaken. **Die Phase 0-/Mutation-/TV-Voraussetzungen bleiben offen.** Eine historische Autorisierungsentscheidung und ein gegenwärtiger fachlicher API-Lese-PASS sind unterschiedliche Ebenen.

| Ebene | Exakte Herkunft | Ergebnis | Was dadurch *nicht* nachgewiesen ist |
| --- | --- | --- | --- |
| REAL-VDR / Human-Browser-GET | Installierter Daemon `git-68f6d05f0ed6`, Admin-Session Login `vdr-suite`, Backend `default`, Runner `08a651e63e4ba8672793c673c81f5597a416197a`, 23.10.2026 16:39–16:40 UTC | **30 PASS / 0 FAIL / 7 SKIP / 17 BLOCKED**, INCOMPLETE; Discovery, EPG, Recording-GET und Artwork-Binary real ausgeführt | Sämtliche 17 BLOCKED- und 7 SKIP-Fälle, kontrollierte Mutationen, Device-MediaSession-/TV-Ende-zu-Ende, finale Candidate-Identität |
| REAL-VDR / historisches Device-SecurityGate | **Vollständige UTC-Zeitfensterabfrage** (`AUDIT_WINDOW: FULL_QUERY`, vorbehaltlich noch gespeicherter Events) in der laufenden yaVDR-SQLite, Gerät `Hisense VIDAA TV`, Actor-Typ `service`, Auth-Status `authenticated`, Scope `default`, letzte positive Entscheidung **2026-10-21T22:21:09Z** | **1.895 historisch erfolgreich autorisierte Device-Gate-Ereignisse:** `channels.view=287`, `epg.view=834`, `recordings.view=503`, `timers.view=54`, `searchtimers.view=46`, `media.live.play=114`, `media.recording.play=57`. Echte Host-Ausgabe `DEVICE_AUTHENTICATED_GATE=PASS`, `RESULT=HISTORICAL_DEVICE_GATE_PASS`. Vorherige 500-Ereignis-Stichprobe ist ersetzt. | Kein HTTP-200-/Payload-Beweis, kein nativer Provider-/Operation-Readback, kein TV-Bild/Ton, kein Resume-Endergebnis, keine aktuell erneuerte Device-Anmeldung und kein Nachweis des Daemon-Builds zum historischen Ereigniszeitpunkt |
| REAL-VDR / historisches Timer-Device-SecurityGate | Gleiches Audit, vorhandenes aktives `yaVDR Timer Hardwaretest`, Backend `default`, letztes `allowed` **2026-10-20T13:38:58Z** | `epg.view=16`, `timers.view=35` erlaubte historische Gate-Ereignisse, kein Partial-Hinweis | Timer-/Operation-Abschluss oder schreibende Pfade |
| REAL-VDR / Timer-/Operation-Persistenz | yaVDR SQLite `mode=ro`, Backend `default`, Runner `0ef0f0b3e8a0be081fad38bd0a5a923ec60b0a6d`, tatsächlicher Hostlauf 23.10.2026 | **7 von 7** `bound` TimerAssignments und **7 von 7** `managed` NativeTimerBindings sind gegenseitig identitäts- und generationstreu verknüpft, keine fehlenden Links. **2** gebundene Bindings haben abweichende Drift-/Missing-/Verification-Flags. **1** `provisioning` Assignment älter als 24h. **4** `outcome_unknown` Operations älter als 24h; **1** davon mit überschrittener **Dispatch-Deadline** (keine Reconciliation-Deadline). Alle managed-Binding-Verifizierungsreferenzen verweisen auf bestehende erfolgreiche Operations. | Kein aktueller nativer Timer-Readback, keine HTTP-v1-Detailantwort und keine Ursache/erlaubte Wiederholung der vier unbekannten Operations nachgewiesen; **keine automatische erneute Ausführung** |
| REAL-VDR / Altzustand-Klassifikation | yaVDR SQLite Read-only, Backend `default`, Runner `cd70cbcfcc7fa55d582facf977568b2d6c5eff36`, 23.10.2026 | 7 reziproke bound/managed Links: **5 Drift `none`, 2 Drift `expected_transition` mit gesetztem `missing_since`**, 0 fehlende Verifizierungsreferenzen. Ein `provisioning` Assignment seit >7 Tagen, **ohne** Binding-ID/Managed-Binding. **4 `outcome_unknown`**, alle `readback_required`: 2 SearchTimer, 1 TimerAssignment, 1 NativeTimerBinding; 1 >7 Tage, 3 ohne Dispatch-Deadline, 1 Dispatch-Deadline überschritten. Kein Retry/Reconciliation ausgeführt. | Keine native VDR-Runtime-Wahrheit oder HTTP-Status; erwartete Transition nicht mit externem Delete gleichsetzen, eine Dispatch-Deadline ist kein Reconciliation-Zeitlimit. Ursprünglicher Agent-Command-/Receipt-/Result-Pfad bleibt separat zu korrelieren. |
| REAL-VDR / Agent-Command-Evidenz zu ungeklärten Operations | Echte yaVDR SQLite, `backend=default`, Read-only Runner `46b3c7da72d0d6f7d381c131d7ccf932d8a9ca49`, 23.10.2026 | **4 `outcome_unknown` Operations:** 1 mit Command-Reservierung, 2 mit Backend-Agent-Command, 1 mit `waiting_reconciliation` Command, 2 mit Receipt, 2 mit Command-Result, 1 mit `outcome_unknown` Agent-Result, **0 mit `accepted_by_executor` Agent-Result**. Zähleinheit je Existenzmerkmal: Distinct Operation. | Fehlendes Agent-Result beweist **nicht** ausbleibenden Dispatch; die tatsächliche resultierende native Änderung, HTTP-V1-Ergebnisse und 1:1-Korrelation der möglichen Resultzustände pro Ressourcentyp sind nicht bewiesen. Keine Retries/Reconciliation ausgeführt. |
| REAL-VDR / Ursprüngliche Agent-Ketten nach Ressourcentyp | Backend `default`, yaVDR SQLite read-only, Runner `63b24e8c8fe462b47d721875ba5ce83f86195253`, 23.10.2026 | **1 NativeTimerBinding**: `completed/not_started/verified/rejected`; **1 TimerAssignment**: `waiting_reconciliation/starting/outcome_unknown/outcome_unknown`; **2 SearchTimer**: `no_command/no_result`. SearchTimer-Dispatch kann den direkten SearchTimer-Provider nutzen, weshalb fehlende Agent-Commands nicht per se fehlerhaft sind. | Genaues `action_family`, Agent-Command-Typ und `error_category` noch nicht aus realem Host abgelesen. Keine fachliche abschließende Operation-Finalisierung, kein HTTP- oder natives Readback bewiesen. |
| REAL-VDR / Authentifizierte Agent-Result-Aktion | Echte yaVDR SQLite `mode=ro`, bestehender Runner `8b3c37dfd3b5e82dfbe9068371f6c09b8280f839`, Backend `default`, 23.10.2026 | **1 `NativeTimerBinding/timer.toggle/vdr.timer.toggle`**: Agent `completed`, Dispatch `not_started`, Verification `verified`, Result `rejected`, Error `fenced`; persistente Operation **noch `outcome_unknown`**. **1 `TimerAssignment/timer.create/vdr.timer.create`**: Agent `waiting_reconciliation`, Dispatch `starting`, Verification/Result `outcome_unknown`, Error `executor_unknown`. **2 `SearchTimer/searchtimer.create`**: kein Agent-Command/Result; eigener SearchTimer-Provider-Pfad ist zulässig. Alle vier Readback-Policy `readback_required`. | Agent-Ablehnung reicht ohne Prüfung der originalen Quarantäneprovenienz, Evidenzparsing, korrekten Operation-Revision und aktuellen Binding-Identität nicht zur produktiven State-Finalisierung; `starting` niemals erneut ausführen; SearchTimer-Provider-Resultat nicht aus fehlendem Agent-Command ableiten. |
| SOURCE-ONLY / begrenzte nachträgliche No-Effect-Finalisierung | Draft PR #526, `core/timers/src/NativeTimerModifyDispatchService.cpp`, Commit `b3ed9ae9ee4cf6c0662220a28f0c5ef59c216340` mit C++-Regression für spätere verifizierte Ablehnung nach exakt `native-timer-modify:missing-result-evidence` markierter Quarantäne; weiteres read-only-Evidenz-Gate in `inspect_unknown_operation_delivery.py` | Nur `outcome_unknown` aus der exakten Evidence-free-Quarantäne mit ursprünglichem Claim +1 Revision, unverändertem Binding und ausdrücklich `rejectedWithoutEffect` ohne dispatchStartedAt darf durch CAS zu `failed_verified` werden. Andere Markierungen, unklare Ergebnisse oder Identitätsabweichungen bleiben blockiert. Kein nativer Timer-Write oder Redispatch im Fix. | **Nicht** installiert, **nicht** am Produktiv-VDR ausgeführt, bisher keine bestätigte Eignung der realen `timer.toggle`-Operation, C++-Build/Regression separat prüfen. Keinesfalls produktiven Daemon neu starten oder SQLite direkt umschreiben. |
| CI / Audit-Helfer | PR #526, Commit `eea2cbe7029751b04e669078c9e29d3c48612bfb`, [Runner-CI #39994128018](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39994128018) und Dokumentations-HEAD `7838808fb8dfd52e4ea0cbfed29fbf56de56e034` mit [CI #39994509691](https://github.com/hotzenplotz5/vdr-suite/actions/runs/39994509691) | 41 Python-Tests PASS; SQL-Gesamtaggregation über das erhaltene UTC-Fenster (6-Sekunden-Query-Limit) **anschließend tatsächlich auf yaVDR ausgeführt und bestätigt**, ohne TV. | Produktcode-Installation, ein aktuell erfolgter neuer Device-Request, geschützte Payload-/Readback-Evidenz sowie die vollumfängliche API-v1-Endabnahme |

**Abhängigkeit ohne TV-Zugriff:** Für einen weiteren vollständigen Device-Ende-zu-Ende-Lauf fehlen im yaVDR-Runner ein ausdrücklich ausgewähltes vorhandenes Device-Credential bzw. ein aktuell ausführender Device-Client sowie für Playback/Resume autorisierte Test-Sessions/Medien und deren Readback. Der lokale SQLite-Audit kann ausschließlich Gate-Evidenz belegen und darf diese Blocker **nicht** als PASS markieren. Aktuell sind keine neuen Accounts, Geräte, Grants, Credential-Rotationen, Daemon-Installationen oder Produktiv-Mutationen genehmigt bzw. durchgeführt. Die ursprüngliche historische Hisense-Auswertung ist **inzwischen mit dem korrigierten Collector vollständig nachgelesen**; die unvollständige 500-Ereignis-Zählung ist historisch überholt. **Kein weiterer Hisense-Audit- oder vollständiger Hardware-Runner-Wiederholungslauf ist dafür erforderlich.**

## Source-only Recovery-Review für die vier historischen Unknown-Operations (23.10.2026)

**C++-CI-Befund und enge Reparatur, keine Hardware-Evidenz:** Auf PR-HEAD `b3ca18f34ee71603b25a9b8a02fed303863c8917` scheiterte die GitHub-Action [VDR-Suite CI #40001377834](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40001377834) im Job `fast-regression-test` an der neuen Assertion in `test_native_timer_modify_dispatch.cpp:381`: Für einen möglichen Agent-Effekt nach evidence-freier Quarantäne meldete die Domain einen Revisionskonflikt statt eines expliziten Zustandskonflikts. **Kein unerlaubter SQLite-Übergang trat im Test ein.** Commit `042f873cfcc03cd0e10473e80ee6c557d5b72343` schließt diese Quarantäne gegen alle nicht als `rejectedWithoutEffect` validierten Ergebnisse mit `operationStateConflict`, bevor der generische Replay-/Revisionspfad greift. Commit `5415fa7a43b67d4dfd135401fe0fb360692e27d0` prüft zusätzlich unveränderte Operation-Revision, unveränderten Quarantäne-Marker und das Verbot einer Rejection mit begonnener Dispatch-Zeit. Der alte fehlgeschlagene CI-Lauf gilt **nicht** als bestanden; der konkrete Folgelauf ist gesondert zu bewerten. Dies ist ausschließlich Quellcode-/CI-Evidenz und ändert weder den installierten Daemon noch historische Operations.

**TimerAssignment / `timer.create`:** Der echte Altzustand `waiting_reconciliation / starting / outcome_unknown / executor_unknown` lässt keinen sicheren erneuten nativen CREATE zu. `NativeTimerCreateProductiveRuntime.cpp` aktiviert ausschließlich `accepted` oder `dispatching`-Claims; bei `outcome_unknown` folgt nur das Nachlesen bereits gespeicherter Agent-Result-Evidenz und, wenn sie zulässig ist, `NativeTimerCreateReadbackReconciliation.cpp`. Ohne gültiges Result beziehungsweise autoritatives Inventar bleibt die Operation offen. Das eine alte `provisioning`-Assignment ohne Binding ist deshalb **keine** Anweisung zum Wiederanlegen oder direktes `failed_verified`-Indiz. Hier wurden keine echten Daten verändert und kein neuer Readback ausgeführt.

**SearchTimer / zweimal `searchtimer.create`:** `SearchTimerCreateDispatchService::dispatchOnce()` nutzt seinen eigenen synchronen Providerpfad. Nach vollständiger Inventur und einzigem CAS `accepted -> dispatching` darf er höchstens einmal `executor.create()` ausführen; Exception, mehrdeutiges Ergebnis oder ungültiges Readback bleiben `outcome_unknown`. Die separat vorhandene `SearchTimerCreateRecoveryService::markOrphanedUnknown()` ist laut Source/Contract eine *dormante* Repository-Hilfe zur Quarantänemarkierung, kein produktiver Supervisor, kein Provider-Readback und keine CREATE-Wiederholung. Ohne zuordenbare native Generation/Identity darf keine der zwei historischen Operationen als erfolgreiche Erstellung oder sicher nicht ausgeführt erklärt werden. Fehlende Agent-Commands sind hierfür normal und keine Diagnose des Provider-Ergebnisses.

**REAL-VDR / ausgeführtes read-only Quarantäne-Evidence-Gate (23.10.2026):** Auf dem echten yaVDR wurde der Prüfer aus dem fest gepinnten PR-SHA `ec21b7f6bac12cacfede01075d2d880bbf5135a1` ausgeführt, Backend `default`, SQLite ausschließlich read-only. Gesamtbefund unverändert: **4 `outcome_unknown`** mit den bekannten Ressourcen (1 NativeTimerBinding/`timer.toggle`, 1 TimerAssignment/`timer.create`, 2 SearchTimer/`searchtimer.create`), **2** Agent-Commands und **2** Agent-Results. Für den historischen `timer.toggle`: `UNKNOWN_NATIVE_TOGGLE_OPERATIONS=1`, `WITH_EXACT_EVIDENCE_FREE_QUARANTINE_MARKER=1`, `WITH_NONEMPTY_TYPED_AGENT_NO_EFFECT_RESULT=0`, `WITH_BOTH_SAME_OPERATION=0`. Der exakte Marker ist also **vorhanden**, aber kein zur selben Operation und den generischen Rejection-Kriterien passend nachgewiesenes nichtleeres typisiertes Resultat. Das ist kein Beweis, dass das gesamte Result-Evidence-Feld physisch leer ist; NULL/leer und unerwartete Konstellationen werden deshalb künftig getrennt aggregiert. Eine `not_started/verified/rejected/fenced`-Zeile allein erlaubt **keine** automatische `failed_verified`-Finalisierung. Ergebnis für die historische Toggle-Operation: **RECOVERY BLOCKED**, keine native Wiederholung, keine DB-Reparatur. Der aktuelle produktive NativeTimerBinding-Zustand ist durch diese Abfrage nicht verifiziert. Keine der 30 bestandenen Public-v1-HTTP-Prüfungen muss hierfür erneut laufen.

**REAL-VDR / zweite read-only Legacy-Evidence-Triage (23.10.2026):** Die ergänzte Diagnose wurde tatsächlich auf dem produktiven yaVDR ausgeführt, gepinnt auf `4b658c86667522f532c7e9e4f3476667e29ec3c4`, Backend `default`. Es gibt exakt **1** `outcome_unknown`-`timer.toggle`-Operation mit dem Marker `native-timer-modify:missing-result-evidence`. Für **dieselbe Operation** gilt `WITH_GENERIC_VERIFIED_REJECTION_BUT_EMPTY_OR_NULL_EVIDENCE=1` und `WITH_ANY_NONEMPTY_AGENT_RESULT_EVIDENCE=0`; die neue Klassifikation bestätigt damit, dass das persistierte Agent-Result-Evidence-Feld für die generische `not_started/verified/rejected/fenced`-Ablehnung tatsächlich **leer oder NULL** ist. `TYPED_REJECTION_RECOVERY_GATE=BLOCKED`. Die vier alten Unknown-Operations, zwei Commands, zwei Results sowie die zugeordneten Familien/States sind unverändert. Auch diese Abfrage macht ausschließlich SQL-Lesezugriffe, leitet keine neue Evidenz aus generischen Feldern ab, verändert keine SQLite-Zeile und führt keinen nativen Befehl aus. **Für die historische Toggle-Operation ist der eng begrenzte neue `failed_verified`-Pfad derzeit nicht ausführbar**; eine erneute identische SQL-Aggregatabfrage ist nicht nötig.

**Source-/Recovery-Grenze:** Ein nichtleeres commandgebundenes Agent-Result wäre nur ein Kandidat für die spätere vollständig typisierte C++-Validierung. Das historische generische Ergebnis ist nicht nachträglich als typisierte Evidenz umzudeuten. `NativeTimerModifyProductiveRuntime.cpp` übergibt ein persistiertes evidenzfreies `outcome_unknown` zwar weiterhin an `applyDurableNativeTimerModifyResult()`, doch dessen erster Evidenz-Parser-Guard liefert `invalidEvidence` ohne `applyOutcome`, ohne Binding-Update und ohne Readback-Completion. An der installierten Runtime wurde nichts verändert. Der Offline-Test simuliert zusätzlich `result_evidence=NULL` und prüft den BLOCKED-Status sowie, dass keine privaten IDs ausgegeben werden. Der aktuelle native VDR-Timerzustand bleibt separat und ausdrücklich **nicht verifiziert**. Die bereits erfolgreichen 30 HTTP-GET-Prüfungen werden nicht erneut angestoßen.

## Eine zentrale Ergebnis-Matrix – kumulativer Stand 24.10.2026 (keine Endfreigabe)

| Bereich | Implementierung / UNIT-CONTRACT | CI / PRODUCTION-INTEGRATION | REAL-VDR (akzeptiert / offen) | Entscheidung |
| --- | --- | --- | --- | --- |
| Security / Pairing / Grants | Read-/Security-Contracts und P1-Playback-Account-Vertrag bestanden | CI am neuen Candidate nicht endgültig zugeordnet; Browser-/Device-Komposition gesondert | 12 Device-/Human-Lifecycle-PASS, 11 Playback-Account-PASS, 401/304 und historische 1.895 Auth-Gates; aktive Pairing-/Credential-Rotation-/Grant-Revocation-E2E offen | **TEIL-PASS**; Mutationen nur mit Freigabe |
| Discovery / OpenAPI / SDK | Discovery/Capabilities und GET-Negative-Contracts bestanden | Exakte neue OpenAPI-/SDK-/Client-Vertragsparität nicht endabgenommen | Discovery/Backend-GET erfolgreich; kein komplett gepinnter Client-Endlauf | **TEIL-PASS** |
| EPG / Channels / Genres / Search | Channels/EPG, EPG-/Recording-Suche und GET-/Paging-Verträge im akzeptierten Umfang bestanden | Geänderte Produktintegration gezielt prüfen | Reale GET-Fälle PASS; nicht geprüfte Randfälle bleiben offen | **TEIL-PASS** |
| TimerIntent / TimerAssignment / Native Timer | GET/ETag/Grants und Operation-ID 404/400 PASS | Mutations-/Agent-/Native-Readback am gemeinsamen Candidate noch nicht endabgenommen | Native VDR `LSTT=0` entspricht Public-v1 `native-timers=0` (**PASS**); mangels Timer kein Detail-ETag; 7 konsistente historische Bindings, 2 erwartete Drifts, 1 CREATE unknown | **WRITE BLOCKED**, keine produktiven Fixtures genehmigt |
| SearchTimer Read / CREATE / UPDATE / TOGGLE / DELETE | Leseseite und Einmal-Dispatch-Verträge; CREATE nur RESTfulAPI, **nicht native CAS-CREATE** | CREATE Mock-CI nicht Hardwarebeweis; produktive epgsearch-CAS-Kette und neuer HEAD offen | Reale Such-/Leseproben akzeptiert; 2 historische CREATE unknown; keine genehmigte Regelmutation | **WRITE / CREATE-SCOPE BLOCKED** |
| Recording / Metadata / Artwork / Marks / Cut | GET-Metadaten/Artwork-Schema und Binär-/ETag-Vertrag bestanden | Install-/Frontend-Staging des neuen Candidate noch offen | Reale Recording-Metadaten/Artwork PASS; Aktionen, Marks/Cut nur nach Testobjekt-Freigabe | **READ TEIL-PASS, ACTIONS BLOCKED** |
| Playback / Resume / Clients | P1 Account-GET-Vertrag bestanden, Client-Lifecycle teilweise abgedeckt | Bundle-/Install-Korrekturen im GitHub-`main`, volle VIDAA-Komposition nicht geprüft | Aktives Hisense gekoppelt, nicht mit Human verknüpft; 1.895 Auth-Gates historisch; Live-TV/Recording-Streams, Ton/Tracks, Sessions/Resume fehlen | **E2E BLOCKED** |
| Fehlerfälle / Neustart / Recovery / Rollback | 304/401/400/404 und fehlerhafte Operations-ID PASS; Unknown-Quarantäne in Source behandelt | Restart/Crash/Replay/Native-Race und Rollback des Endkandidaten nicht vollständig bewiesen | Vier historische `outcome_unknown` bleiben unangetastet; kein freigegebener sicherer Produktiv-Restart/-Rollback | **TEIL-PASS / REST BLOCKED** |

**Nicht als fertig markieren**, solange nur GitHub-Mocks laufen, ein Provider bloß ein Interface implementiert oder keine reale Autorisierung/Readback-Verkettung existiert. Keine separat erfundenen „S1a/S1b“-Abnahmephasen.
