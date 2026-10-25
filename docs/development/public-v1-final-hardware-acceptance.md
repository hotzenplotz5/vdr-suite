# Public API v1 – einmalige End-to-End-Abnahme am realen VDR

**Status: VORBEREITET – HISTORISCHE REAL-VDR-Lese- UND Schreibnachweise für Native Timer, EPG-TimerIntent, Recording-Aktionen und SearchTimer-RESTfulAPI vorhanden; GEMEINSAME RELEASE-ENDABNAHME NICHT AUSGEFÜHRT.**

**Verbindliche Arbeitsregel (22.10.2026):** Erst die gesamte API-v1-Implementierung vollständig integrieren, dann **eine gemeinsame, nachvollziehbare Endabnahme am echten VDR**. Keine separaten, wiederholten Hardware-Slices je API-Funktion. Automatische Source-CI bei Commits ersetzt die Endabnahme nicht. Eine neue Änderung nach Abnahme erfordert nur die betroffenen Regressionen und die zentralen Pflichtprüfungen, nicht einen Neustart der gesamten Entwicklung.

Diese Checkliste ist der **einzige operative Hardware-Abnahmeplan** für API v1. Bestehende fachliche ADRs, OpenAPI und `docs/CURRENT.md` bleiben Vertrags- bzw. Statusquellen; bei Abweichung den offenen Punkt hier notieren, keine zweite parallele Checkliste anlegen.

## Integration-Checkpoint auf `main` (23.10.2026; keine Endabnahme)

| Prüfebene | Nachweis / Status | Grenze und nächste Prüfung |
| --- | --- | --- |
| Quellcode integriert | R1–R3, ADR-0068 (#517), E1 (#519), E2, M1, M2, S1, P1 [#524](https://github.com/hotzenplotz5/vdr-suite/pull/524) merged at `052350714f5b5904a969158925cb05d8f39aed78`; Unified Acceptance Runner [#526](https://github.com/hotzenplotz5/vdr-suite/pull/526) merged at `096a04ad0e6a4eb2555292929d1373df549780bc` | Keine Behauptung einer durchgehenden produktiven Runtime- oder installierten Release-Kette ohne Code-Inventur |
| Produktivpfad verdrahtet | PR #526 beschreibt die bestehende native SearchTimer UPDATE/ACTIVATE/DEACTIVATE/DELETE-Kette, TimerIntent/Assignment atomare Aufnahme/Agent-/native Readback-Kette sowie P1-Source-Komponenten | Am gemeinsamen HEAD anhand Runtime-Komposition, SecurityHttpGate, Grants, Agent/SuiteBridge, Browser/Device/JS und OpenAPI einzeln nachprüfen; SearchTimer CREATE nutzt weiterhin RESTfulAPI, **keine** native epgsearch-CAS-CREATE-Garantie |
| Tests / CI | PR #526 nennt die erfolgreichen exakten PR-Head-Läufe [VDR-Suite CI #40008988460](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40008988460) und [Unified Runner #40008988457](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40008988457); SearchTimer CREATE Regression explizit **mock only**. | Ergebnisse gelten für den jeweiligen getesteten HEAD, nicht als automatisch erneute Tests am dokumentationsaktualisierten `main` oder reale Gerätetests; exakte Integrations-SHA-Nachweise vor Release ermitteln |
| Reales yaVDR | Bestehende GET-/Security-Nachweise **plus echte Schreibabnahmen**: Public-v1 Native Timer CREATE/UPDATE/TOGGLE OFF/TOGGLE ON/DELETE am 21.10.; Public-v1 EPG-TimerIntent CREATE am 19.10.; Public-v1 Recording RENAME/MOVE/DELETE am 21.10.; direkter RESTfulAPI-SearchTimer CREATE/UPDATE/DELETE-Lifecycle mit Readback | Einzelne ACCEPTED-PASS niemals erneut ausführen. Gemeinsamer neuer Installationskandidat, native SearchTimer-Public-v1-CAS-Kette und Geräte-E2E bleiben gesondert; vier historische `outcome_unknown` nie blind wiederholen |
| Offen/blockiert | Finaler einheitlicher Release-Kandidat, Browser↔Device/VIDAA-Playback, **nur noch nicht abgedeckte** Operations-/Recovery-Randfälle, Public-v1-SearchTimer-CAS und CREATE-Garantie, Agent-Boot/Recovery | Bestehende echten Timer-/Recording-/Provider-Schreib-PASS sind verbindlich zu übernehmen, nicht zu wiederholen; keine neuen produktiven Mutationen, Neustarts oder DB-Reparaturen ohne separate Freigabe |

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
  ist für den damaligen Fix-HEAD ebenfalls **SUCCESS**.
  [PR #537](https://github.com/hotzenplotz5/vdr-suite/pull/537) ist
  nach vollständigem final-head CI **gemergt**; `main` enthält die
  Runner-/OpenAPI-Korrektur seit `24ae37888a810b2058ff8181b680e850e9f4c9d1`.
  Die zentrale Matrix bleibt unabhängig von einem PR-Status der
  einzige Abnahmeort. Die GitHub-Commit-Statusabfrage allein
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

**Nachgeholter Abgleich realer Schreibabnahmen (nicht erneut testen):**

- **Public-v1 Native Timer – vollständige reale Hardware-Abnahme 21.10.2026:**
  [Originalprotokoll](native-timer-api-v1-yavdr-hardware-acceptance-20261021.md),
  yaVDR VDR 2.8.3, Backend `default`, Native SuiteBridge SVDRP
  und RESTfulAPI Readback. Zwei eigenständige ausdrücklich testweise
  angelegte künftige Timer-Fixtures. Nach der ersten
  `UPDATE=failed_verified`- und späteren `TOGGLE_OFF=failed_verified`-
  No-Effect-Beobachtung erfolgte **kein Redispatch**. Im zweiten
  Journal war das kontrollierte Endergebnis
  `HARDWARE_ACCEPTANCE=PASS CREATE UPDATE TOGGLE_OFF_RECOVERY TOGGLE_ON DELETE`
  mit nativer Änderung und finaler bestätigter Timer-Abwesenheit.
  Der Unterschied zwischen erfolgreich kontrolliertem Fixture und
  weiterhin ungelösten *anderen* historischen Operations ist strikt.
- **Public-v1 EPG-TimerIntent/native CREATE 19.10.2026:**
  [Originalvertrag mit Live-Nachweis](../../api/epg-timer-intents.md),
  installierter `git-f7e341ad6b97`, echter Device-POST
  `/api/v1/epg/timer-intents` für *Tagesschau* am 20.10.,
  `OPERATION_STATE=succeeded`, `VDR_READBACK=PASS`,
  `TEST_RESULT=PASS`; testweises Device widerrufen.
  [Phase-69.C-Closeout](phase-69c-closeout.md) bestätigt zusätzlich
  an einer echten gebundenen Native-Timer-CREATE-Fixture auf VDR 2.7.9
  Replay mit genau einem Command, 409, 412 und Daemon-Neustart
  ohne erneuten Dispatch. Diese historische 2.7.9-ABI-Besonderheit
  ist **nicht** als neuer VDR-2.8.3-Buildbeweis auszugeben.
- **SearchTimer realer Provider-Lifecycle:**
  [Original yaVDR-Smoke-Protokoll](searchtimer-yavdr-smoke-test-execution-report.md)
  dokumentiert tatsächliches RESTfulAPI `127.0.0.1:8002`
  `PASS CREATE/READBACK/UPDATE/READBACK/DELETE/READBACK`,
  `Result: SUCCESS`, anschließend native Liste wieder leer.
  Die damalige Suite-Real-Test-Route war absichtlich non-mutating
  beziehungsweise nicht über das versehentlich geprüfte Rectools
  `:8080` erreichbar. Daher **Provider-CRUD nachgewiesen**,
  aber **nicht** als modernes Public-v1-Device-/epgsearch-CAS-HTTP-
  Mutation-Ende-zu-Ende auszugeben. Insbesondere
  Public-v1-CREATE-native-CAS wird nicht erfunden.
- **Public-v1 Recording R2/R3 echte Hardware-Abnahme 21.10.2026:**
  [Originalprotokoll](public-v1-recording-r2r3-hardware-acceptance-20261021.md),
  `13e63d277deb5c9e0230f757a8c5733f6e7b0cbb`,
  ausdrücklich freigegebene *Brisant*-Testaufnahme,
  RENAME/MOVE/DELETE je ausgeführt und providerverifiziert,
  `API_V1_RECORDING_HW_ACCEPTANCE=PASS`; Fixture gelöscht.
  Keine neue Löschung zur bloßen Bestätigung dieser Evidenz.

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
3. **Bereits bestandene Schreib-Fixtures nicht wiederholen:** Public-v1
   Native-Timer-Lifecycle, EPG-TimerIntent CREATE, Recording RENAME/MOVE/DELETE
   und RESTfulAPI-SearchTimer-CRUD sind reale historische PASS (Details unten).
   Nur tatsächlich *andere* neue Public-v1-/Provider-/Agent-/Recovery-Teile
   bleiben nach geeigneter Identität/Backup-/Rollback- und gesonderter
   ausdrücklicher Freigabe noch gezielt abzunehmen. Kein Eingriff in
   vorhandene Timer, Aufnahmen oder Regeln.
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

**Source-Korrektur an echter Abnahmeabweichung:** 
[PR #537](https://github.com/hotzenplotz5/vdr-suite/pull/537)
wurde aus HEAD `e4d9976840823b39fb7af2bec27de7bf38932f28`
nach erfolgreicher
[Runner-CI #66](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40086067582)
und [Source-CI #11057](https://github.com/hotzenplotz5/vdr-suite/actions/runs/40086067554)
in `main` gemergt (Merge `24ae37888a810b2058ff8181b680e850e9f4c9d1`).
Damit unterstützt der read-only Acceptance-Runner historische persistierte
Operation-IDs bis 160 ASCII-Bytes ohne Änderung des produktiven Daemons
oder des `op_`-Neuausgabeformats. Die negativen Grenzen (161 Zeichen,
Pfad-/Query-Zeichen sowie eigenständiges `.` und `..`) sind durch
Offline-Regressionsfälle geprüft; OpenAPI und README sind abgeglichen.
Diese Korrektur bestätigt **keine** unbekannte historische Operation
und begründet **keine** erneute Hardware-Probe.

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

**Leseregel für die noch offenen Kästchen in Abschnitt 2–5:** Sie bezeichnen die ausstehende *gemeinsame neue Release-Kandidaten-Freigabe*, **nicht** die Verpflichtung, jeden historisch bestandenen GET, Native Timer, Recording oder Provider-CRUD erneut auszuführen. Was bereits PASS ist, steht in der einzigen kumulativen Matrix am Ende dieses Dokuments; nur der neue geänderte Pfad erhält einen zusätzlichen Nachweis.

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

Diese Tabelle dokumentiert vorhandene **produktive Lese-Evidenz**; akzeptierte echte Schreib-Evidenz steht nun ausdrücklich in der zentralen kumulativen Matrix und ihrem unmittelbar vorangestellten Quellenabgleich, ohne die offene Gesamt-Release-Abnahme vorzeitig abzuhaken. **Die Phase 0-/Mutation-/TV-Voraussetzungen bleiben offen.** Eine historische Autorisierungsentscheidung und ein gegenwärtiger fachlicher API-Lese-PASS sind unterschiedliche Ebenen.

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


## Samsung S3.3 – neue REAL-TV-Evidenz und Client-Überschneidungen (25.10.2026)

**Neue Evidenzebene: REAL-TV / Samsung Tizen 5.5, vom Betreiber durch Fotos bestätigt.** Diese Beobachtungen ergänzen die historisch akzeptierten REAL-VDR-Nachweise; sie sind **kein** neuer vollständiger API-v1-Release-Lauf und kein Nachweis für die unabhängige Hisense-VIDAA-Hardware.

1. `S3.1 / GET /api/v1` und `/capabilities`: Samsung TV erreichte den Server per HTTPS mit dem korrekten Reverse-Proxy-Prefix und meldete **31 Fähigkeiten**. Das belegt den tatsächlichen TV-Netzwerkpfad und die anonyme Discovery, nicht die Nutzungsberechtigung.
2. `S3.2 / POST /api/v1/device-pairings` → Token-Status → Admin-Genehmigung → einmaliges Credential → `GET /api/v1` als `VDR-Suite-Device`: auf dem realen TV ausgeführt. Die neue Geräteidentität erschien nach der Aktivierung im Browser-Admin-Inventar. Der Server zeigte **zwei** Samsung-Geräte mit demselben Anzeigenamen: die ältere Identität blieb nach der Testinstallation aktiv, während das TV-Frontend **„Kein gespeichertes Device-Credential“** meldete. Die neue erfolgreiche Authentifizierung belegt den aktuellen Credential-Pfad, **nicht** dessen Wiederherstellung nach einer weiteren Installation oder einem TV-Neustart.
3. `S3.3 / GET /api/v1/backends` mit expliziten Device-Grants: zunächst meldete die TV-App **„Keine Backends freigegeben“**. Nach ausdrücklicher neuer Admin-Freigabe von `channels.view@default` für die **zweite** Device-Identität wurde das Backend sichtbar. Das ist ein wertvoller positiver/negativer Grant-Nachweis. Es belegt nicht automatisch den isolierten HTTP-403-Pfad für ein explizit verbotenes Channel-Backend.
4. `S3.3 / GET /api/v1/channels?backendId=default&limit=20&sort=backendId,channelId&order=asc`: Anzeige **„20 echte Sender geladen“**, darunter Das Erste HD und SWR RP HD, mit realen Sendernummern. **Erste Seite = REAL-TV PASS.** Die beobachtete Reihenfolge entspricht aber der derzeitigen `channelId`-Sortierung und **nicht** der gewünschten `channels.conf`-/`channelNumber`-Reihenfolge.
5. Admin-Browser-Refresh (PR #539): Das neue Device erschien schließlich in der Liste; das tatsächliche Intervall von **10 Sekunden** wurde nicht gemessen. Eine Bedienung zum gezielten Widerrufen bzw. Ausblenden alter Device-Identitäten fehlt in der aktuellen Geräteoberfläche; der vorhandene autorisierte Device-Revoke-API-Vertrag ist hiervon zu unterscheiden. Kein alter Datensatz oder Grant wurde in dieser Prüfsitzung gelöscht.

**Überlappungen einmal testen, Plattformbesonderheiten getrennt akzeptieren:** Pairing-Flow, Device-Actor/SecurityHttpGate, Public-v1-Routen, Grants, Cursor-/Problem-Verträge und die Suite-eigene MediaSession-Policy werden serverseitig **einmal** für den relevanten unveränderten Produktpfad akzeptiert. Samsung KeyManager, VS-Code/Tizen-Update-Persistenz, AVPlay/Remote/Decoder und Hisense VIDAA KeyStore, MSE/Direct Play/Codec/Packaging sind **eigene** Client-/Hardware-Evidenz. Der bisherige VIDAA-`TV_ACCESS_UNAVAILABLE`-Blocker bleibt spezifisch für Hisense; der Samsung-Test darf weder als VIDAA-PASS noch als VIDAA-BLOCKER ausgegeben werden.

**Nur noch tatsächliche Lücken gezielt testen:** vollständige VDR-`channels.conf`-Reihenfolge **über Cursorseiten** (erst nach serverseitig dokumentierter, implementierter Numeric-Sort-Erweiterung), Samsung `Mehr Sender`/Cursor-409 und gegebenenfalls weitere Backends, widerrufene/fremde Grants/401/403/Backend-Isolation, Geräteschlüssel **über normalen Neustart** und einen **datenbewahrenden Updateweg**; Admin-Polling bei sichtbaren/versteckten Tabs und doppelt benannten Devices sowie ausdrücklich gewollter Revoke-/Retire-Bedienung; echte Samsung AVPlay-Live-Session mit Bild/Ton/Stop/Back, Recording MediaSession/Subtitle/Seek/Resume, EPG/Timer nur nach Client-Slice und den passenden gesonderten Grants. Keine implizite Genehmigung für neue Backend-Schreibtests, Host-Installation, Pairing-Replays oder Service-Neustarts. Die bereits abgenommenen nativen Timer-/Recording-/Provider-CRUD-Prüfungen werden **nicht erneut** ausgeführt.

## S3.4 – Channel-Sortierung nach `channels.conf`: Source-Slice, Hardware offen

**Source-Kandidat VDR-Suite PR #542 (25.10.2026):** Additives `GET /api/v1/channels?backendId=default&limit=20&sort=backendId,channelNumber,channelId&order=asc`; die Produktions-Channel-Projektion sortiert die vollständige aktuelle VDR-Snapshot-Liste nach Backend, numerischer `channelNumber` und `channelId`-Tie-Break **vor** dem Paginieren. Neuer scopegebundener `ch2_`-Cursor; vorhandene `ch1_`-Sortierung/Clients unverändert. REST-, Referenzclient-, OpenAPI- und Security/Source-Regressionen sind Teil des Kandidaten. Der parallele Samsung-S3.4-Client PR #3 fordert diesen Sortiermodus an; ältere installierte Server werden mit einer sichtbaren HTTP-400-Meldung behandelt, nicht mit stillschweigend falscher Sortierung.

**Abnahmegrenze:** Implementierte und getestete **Quellen** ersetzen keinen Build-/Installationsnachweis des API-Servers. Die bereits belegten 20 echten Samsung-S3.3-Sender kamen **aus dem alten ID-sortierten Vertrag**. Erst wenn der neue API-v1-Build auf dem yaVDR installiert und identifiziert ist, darf einmalig read-only auf dem Samsung die erste und zweite Senderseite mit der tatsächlichen `channels.conf`-Reihenfolge verglichen werden. Negative Misch-Cursor/Scope-Proben bleiben Unit/REST-Kontrakt, sofern die Produktfunktion unverändert und der gesicherte HTTP-Pfad nachgewiesen ist. Keine neuen Pairings, Grants oder destruktiven Host-/VDR-Aktionen nur für diesen Slice. P1-Resume bleibt hiervon unabhängig offen.

## Eine zentrale Ergebnis-Matrix – kumulativer Stand 24.10.2026 (keine Endfreigabe)

**Ergänzungsstand 25.10.2026:** Neu sind der oben dokumentierte Samsung S3.1–S3.3-REAL-TV-Nachweis sowie die offenen Numeric-Channel-Order- und Client-Lifecycle-Gates. Die historische Überschrift vom 24.10. bleibt für bestehende Verweise erhalten.

**Eine Matrix, getrennte Evidenzebenen.** `Implementiert` meint hier nachgewiesene Source-Integration, nicht installierte Hardware. `UNIT/CONTRACT/CI` und `REAL-VDR` sind getrennt. Historische PASS gelten für ihren belegten Code-/Host-Kontext; ihr Nutzen wird anhand tatsächlicher Änderungswirkung beurteilt. Diese Tabelle ist die einzige operative Abnahmematrix; der [Dokumentations-Closeout](public-api-v1-documentation-closeout-20261024.md) erklärt Status und Release-Gates, ohne eine zweite Matrix zu eröffnen.

| API-v1-Funktion | Implementiert | Unit / Contract / CI | Echter yaVDR-Test | Originalnachweis | Was fehlt wirklich noch? |
| --- | --- | --- | --- | --- | --- |
| Auth, Device-/Human-Lifecycle, Backend-Grants | **JA**; Human/Device und P1 getrennte Actor-Grenzen | **PASS – UNIT/CONTRACT/CI**, Scoped-Gates; P1 source-merged | **PASS – REAL-VDR / HISTORISCH:** 12/12 Lifecycle; 11/11 Playback-Account-GET; 1.895 erlaubte historische Hisense-Device-Gates; negative Auth-/Backend-Proben | [Abgleich oben](#abgleich-der-bereits-akzeptierten-yavdr-evidenz-24102026-noch-keine-gesamtfreigabe), [P1](../../api/device-recording-playback.md) | Aktuelle Device-Playback-/Grant-E2E **BLOCKED – TV ACCESS**; kein neues Pairing oder Credential-Ersatz. Frühere Gates **NICHT ERNEUT ERFORDERLICH** |
| Discovery, Capabilities, Backend-GET, Operation-ID-Negativfälle | **JA** | **PASS – UNIT/CONTRACT/CI:** 19/19 Baseline, sieben fokussierte Blöcke mit 59 PASS (überlappend); Operation-ID 404/400; PR #537 Offline-Regression + Runner-CI | **PASS – REAL-VDR / HISTORISCH** für geprüfte GET-/Auth-/304- und Operation-ID-Fälle | [PR #537](https://github.com/hotzenplotz5/vdr-suite/pull/537), [Zentrale Matrix-Proben](#vorläufige-reale-yavdr-teilnachweise--keine-endabnahme-23102026) | Vollständige OpenAPI-/SDK-/Methoden-Parität nicht durch 27/27 Runner-Pfade erwiesen. Bereits bestandene GETs **NICHT ERNEUT ERFORDERLICH** |
| Channels, EPG, Window, Search und Genres | **JA**, E1-Pagination source-merged | **PASS – UNIT/CONTRACT/CI** für abgegrenzte Routen/Paging, inklusive E1 | **PASS – REAL-VDR / HISTORISCH**: Sender, EPG, Suche/Genres, ETag/304 im akzeptierten Umfang | [CURRENT](../CURRENT.md), [E1-Vertrag](../../api/public-epg-window-pagination.md), [Abgleich oben](#abgleich-der-bereits-akzeptierten-yavdr-evidenz-24102026-noch-keine-gesamtfreigabe) | Nur bisher unbelegte neue E1-Mehrseiten-/Stale- und Integrationspfade eines tatsächlich anderen Release-Kandidaten gezielt prüfen; alte GET-Fixtures nicht erneut |
| Native Timer CREATE / UPDATE / TOGGLE OFF / ON / DELETE | **JA**, Public-v1-Agent/SuiteBridge/Readback integriert | **PASS – UNIT/CONTRACT/CI**, Native-Mutation- und Security-Verträge | **PASS – REAL-VDR:** vollständiger kontrollierter Lifecycle 21.10.2026 auf VDR 2.8.3; native Feldänderung und DELETE-Abwesenheit belegt | [Native Timer Hardware 21.10.](native-timer-api-v1-yavdr-hardware-acceptance-20261021.md) | **NICHT ERFORDERLICH:** Wiederholung sämtlicher fünf bestandener Operationen. Separat verbleiben unbekannte historische Operations, sonstige Recovery-Randfälle und exakte neue Release-Binär-/ABI-Provenienz |
| EPG → TimerIntent → native CREATE; Assignment / Durable Operation | **JA**, E2 produktiver Route-/Auth-/Agent-Pfad | **PASS – UNIT/CONTRACT/CI** für atomare Admission, Replay, Revision und Readback; E2 source-closed | **PASS – REAL-VDR:** *Tagesschau* Device-POST am 19.10., `succeeded`, nativer Readback. **PASS – HISTORISCH:** Phase 69.C CREATE/409/412/idempotenter Replay/Daemon-Restart ohne Redispatch auf VDR 2.7.9 | [EPG-TimerIntent](../../api/epg-timer-intents.md), [69.C](phase-69c-closeout.md) | Keine erneute CREATE-/Replay-Fixture; 2.7.9-Beleg **kein** ABI-Beweis für neuere 2.8.3-Builds. Agent-Boot, neue Crash-/Unknown-Pfade, installierter Release-Kandidat gesondert |
| Timer-, Assignment- und Operation-GET; Binding-Abgleich | **JA** | **PASS – UNIT/CONTRACT/CI** für Read-/Revision-Verträge | **PASS – REAL-VDR / HISTORISCH** für GET/ETag und native vs. API-Liste 0/0; 7/7 bound/managed Links read-only konsistent | [Abgleich oben](#abgleich-der-bereits-akzeptierten-yavdr-evidenz-24102026-noch-keine-gesamtfreigabe), [69.C](phase-69c-closeout.md) | Nicht vorhandene native Timer-Detail-Fixture nicht künstlich erzeugen; vier separat unklare Operations bleiben **BLOCKED** |
| SearchTimer LIST / DETAIL / PREVIEW / CREATE-Admission | **JA**, serverseitige Device-/Backend-Verträge; CREATE einmaliger RESTfulAPI-Write mit SQLite-CAS und Readback | **PASS – UNIT/CONTRACT/CI** der Source-Verträge; CREATE-Regressions-Mock **nicht** Hardware | **PASS – PROVIDER** nur für darunterliegende RESTfulAPI-CRUD; moderne Public-v1-CREATE-HTTP/Auth-/Device-Kette **OFFEN** | [Public-v1 SearchTimer](../../api/search-timers.md), [Provider-Smoke](searchtimer-yavdr-smoke-test-execution-report.md) | Entscheidung zwingend: native epgsearch-lock-CAS-CREATE nachrüsten **oder** CREATE ausdrücklich aus atomarem Release-Scope herausnehmen. Zwei alte CREATE-`outcome_unknown` niemals neu dispatchen |
| SearchTimer UPDATE / ACTIVATE / DEACTIVATE / DELETE (S1) | **JA – SOURCE**, native epgsearch-CAS, SuiteBridge, Agent, Revision/Generation-Fences und Public-v1-Mutation-Route | **PASS – UNIT/CONTRACT/CI** im begrenzten S1-/PR-#514-/PR-#526-Quellumfang, keine echte installierte Endabnahme | **PASS – PROVIDER** für älteren RESTfulAPI UPDATE/DELETE-Lifecycle mit Readback; **OFFEN – KONKRETER UNGEPRÜFTER PFAD:** Public-v1-Device → nativer patched epgsearch CAS → Readback | [S1-Spezifikation](../../api/search-timers.md#public-v1-guarded-native-searchtimer-mutations-integration-candidate), [S1 Native Fence](public-v1-searchtimer-native-generation-fence.md), [Provider-Smoke](searchtimer-yavdr-smoke-test-execution-report.md) | Einmalige gezielte neue S1-native ABI-/Capability-/Auth-/Revision-/Readback-Abnahme des passenden installierten Builds nur mit separater Host-/Mutationsfreigabe; **Provider-CRUD NICHT wiederholen** |
| Recording RENAME / MOVE / DELETE, Validate / Dry-run | **JA**, R2/R3 Public-ID-/Backend-/Action-Verträge | **PASS – UNIT/CONTRACT/CI** auf akzeptiertem Aktionskandidaten | **PASS – REAL-VDR:** 21.10. auf ausdrücklich freigegebener *Brisant*-Fixture mit nativem und Public-ID-Readback, endgültig gelöscht | [Recording R2/R3 Hardware 21.10.](public-v1-recording-r2r3-hardware-acceptance-20261021.md) | **NICHT ERFORDERLICH:** eine weitere *Brisant*-Fixture, neue RENAME/MOVE/DELETE-Abnahme oder destruktiver Test; nur noch andere Crash-/Rollback-/Release-Installationsrisiken |
| Recordings Collection / Detail / Metadata / Artwork | **JA**, R1 + M1/M2 Source-Integration | **PASS – UNIT/CONTRACT/CI** für akzeptierte Projektionen und Source-Contracts | **PASS – REAL-VDR / HISTORISCH:** Collection, 1.037 reale Aufnahmen, Metadata/Artwork und echte Binärdaten, ETags | [Recording R2/R3](public-v1-recording-r2r3-hardware-acceptance-20261021.md), [Source-Audit](public-v1-source-audit-20261021.md) | Neue versionsspezifische Metadata-/Artwork-/Client-Sonderfälle nur bei konkret betroffenem geänderten Pfad; bestehende Reads nicht erneut |
| Recording Marks / Cut (Public v1) | **Nicht als vollständiger eigenständiger v1-Schreibvertrag nachgewiesen**; historische Schnittfunktionen liegen bei Legacy `/api/vdr/recordings/marks` und `/cut` | **OFFEN – KONKRETER UNGEPRÜFTER PFAD**; keine gleichwertige v1-Source-/Hardware-Evidenz | **KEIN** entsprechend belegter Public-v1-Marks/Cut-Mutationsabschluss | [Source-Audit 21.10.](public-v1-source-audit-20261021.md), [Zentrale Schreib-Checkliste](#3-public-api-v1--schreibende-endpunkte-nur-freigegebene-testobjekte) | Release-Umfang explizit entscheiden: Public-v1-Marks/Cut implementieren und separat sicher abnehmen **oder** für API v1 nicht versprechen; kein Recording ohne ausdrückliche Freigabe ändern |
| Playback Sessions / Resume / Continue Watching / VIDAA (Hisense) | **JA – SOURCE** für P1 Account/Progress/CAS und bestehende Media-Sessions | **PASS – UNIT/CONTRACT/CI** P1-Source, **11/11 PASS** historische Playback-Account GET-Hardware | **PASS – HISTORISCH** Device-Gates; **BLOCKED – HISENSE TV ACCESS** für neue VIDAA-Live-TV-/Recording-Playback-, Browser↔Device-, Track-/Resume-Ende-zu-Ende-Prüfungen; Samsung S3.3-Channel-Reads sind separat PASS (keine Playback-Freigabe) | [P1-Contract](../../api/device-recording-playback.md), [CURRENT](../CURRENT.md) | HISENSE TV_ACCESS_UNAVAILABLE bis Betreiber Hisense-Zugriff ausdrücklich bestätigt; keine neuen VIDAA Device-Credentials/Grants oder Hardwaretests. Samsung Device wurde für S3.3 separat neu gekoppelt und berechtigt; Package-/JS-/Frontend-Komposition unabhängig abgleichbar |
| Negative, Idempotenz, Restart, Crash, Recovery, Rollback | **Teilweise JA**, fail-closed Operations und Quarantäne-/Reconciliation-Pfade | **PASS – UNIT/CONTRACT/CI** für dokumentierte Fälle, **OFFEN** für vollständige finale Recovery-/Rollback-Abdeckung | **PASS – HISTORISCH** Phase-69.C Restart/Replay/409/412; **BLOCKED** vier verschiedene `outcome_unknown` (2 SearchTimer CREATE, 1 TimerAssignment CREATE, 1 NativeTimerBinding TOGGLE) | [69.C](phase-69c-closeout.md), [Unknown-Triage oben](#source-only-recovery-review-für-die-vier-historischen-unknown-operations-23102026) | Neues Risiko nur gezielt prüfen; historisches TOGGLE ohne typisierte No-Effect-Evidenz bleibt unentscheidbar. **Kein** Redispatch, generische `rejected/fenced`-Umdeutung, SQLite-Fix oder Agent-Result-Erfindung |
| S3.4 Channel-Liste nach VDR-`channels.conf` | **JA – SOURCE-KANDIDAT PR #542**, native `channelNumber`-Sortierung vor `limit` + `ch2_`-Cursor; Samsung PR #3 als abhängiger Client | **SOURCE-/CI-REGRESSIONEN GEPLANT**, endgültige PR-Head-Ergebnisse einzeln zu verifizieren | **NOT_VERIFIED – REAL-VDR / REAL-SAMSUNG** für neue numerische Seiten; bisherige S3.3 20er-ID-Seite bleibt PASS | [S3.4-Abgrenzung oben](#s34--channel-sortierung-nach-channelsconf-source-slice-hardware-offen), [Channel-Vertrag](../../api/channel-collection.md) | Ziel-Build/-Deployment-Abgleich, anschließend erster + zweiter `ch2_`-TV-Seitenread und exakte VDR-Positionsfolge; keine erneuten Pairing-/Alt-Hardwaretests |
| Samsung Tizen S3.1–S3.3 Device → Backend → Channels | **JA – SOURCE**, Samsung App S3.3 mit gespeichertem Device und Public-v1-Reads; Server-API unverändert | **PASS – Samsung UNIT/CONTRACT/CI:** 56 Tests auf akzeptiertem S3.3-PR-Head; kein Produktionsruntime-Gesamttest | **PASS – REAL-TV 25.10.:** HTTPS/31 Capabilities, neue Pairing-/Device-Auth, leeres Backend ohne Grant, nach `channels.view@default` 20 echte Channels geladen | [Samsung S3.3-Nachweis oben](#samsung-s33--neue-real-tv-evidenz-und-client-überschneidungen-25102026), [Channel-Vertrag](../../api/channel-collection.md) | Numeric-Sort aus VDR-`channels.conf` aktuell **NICHT implementiert**; Pagination Seite 2, Cursor-409, 401/403/Backend-Isolation, Neustart-/Update-Persistenz, Admin-Polling und echte TV-Wiedergabe gesondert prüfen |
| Geräteverwaltung / doppelte Samsung-Identität | **JA – SOURCE** für Device-Inventar/Grant-Verwaltung und eigenständigen Device-Revoke-HTTP-Vertrag; Browser-Auto-Refresh PR #539 | **PASS – SOURCE/CI** für Admin-Slice; kein belegter 10-Sekunden-Hardware-Timer | **TEILWEISE REAL-TV/Browser:** zweites Device erschien; gleiche Anzeigenamen, alte Serveridentität blieb bestehen, TV-KeyManager nach Run Project leer, Grant explizit neu vergeben | [Device Administration](../../api/device-administration.md), [Device Lifecycle](../../api/device-lifecycle.md) | Quelle des datenverlustfreien Updates und per-Device-Revoke-/Retire-UI klären; 10-Sekunden-Polling im sichtbaren Tab, Tab-Hide und richtige Zielidentität **nicht vollständig abgenommen** |
| Gemeinsamer Build, Packaging, OpenAPI / SDK / CI | **Source zusammengeführt** durch PR #524/#526/#537/#536; OpenAPI weiterhin `partial` | **PASS – UNIT/CONTRACT/CI** für historisch genaue Köpfe; 27/27 Runner-Routen im partiellen OpenAPI-Inventar, **keine** vollständige Parität | **OFFEN:** exakt gemeinsamer neuer versiegelter Daemon/Agent/SuiteBridge/Frontend/Plugin-/JS-Release-Kandidat; kein neuer vollständiger Hardware-Endlauf | [Runner](../../tools/api-v1-acceptance/README.md), [Abgleich oben](#abgleich-der-bereits-akzeptierten-yavdr-evidenz-24102026-noch-keine-gesamtfreigabe), [Closeout](public-api-v1-documentation-closeout-20261024.md) | Exakte releasefähige Version, ABI/Patchebene, installierte Dateien + `RUNTIME_DEPLOYMENT_MATCH=YES`, relevante Gesamt-CI, API-/SDK-Parität, Rollback-Gate und anschließend nur tatsächlich neue REAL-VDR-Risiken |

**Releaseentscheidung:** `DOCUMENTATION_CLOSEOUT=SOURCE_RECONCILED`; `API_V1_RELEASE=BLOCKED`. Die einzelnen PASS-Fälle sind verbindlich akzeptiert. Es gibt keine Freigabe für neue produktive Writes, Host-Neustarts, Deployment oder TV-Tests durch diese Tabelle.
