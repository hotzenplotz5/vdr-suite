# ST.V1 – Einheitlicher epgsearch-Suchtimer-CREATE über Public API v1

Arbeitsbranch: `MLD-Suchtimer` (kein Merge nach `main`). Start-HEAD: `037e0545a89e5c86670563b6ce046f24947704e1`.
Diese Roadmap ist ein Arbeitsplan, keine Behauptung bestandener Tests oder einer erfolgten Installation.

## Ziel und verbindliche Architektur

Ein Benutzer erstellt aktiv oder inaktiv dieselbe vollständige epgsearch-Regel über
`POST /api/v1/search-timers`, unabhängig davon, ob Browser, MLD, yaVDR oder
ein autorisierter Device-Client aufruft. Browser- und Device-Authentifizierung
bleiben verschieden; Berechtigungsprüfung, fachliche Validierung, Operation,
Idempotenz, ein einziger RESTfulAPI-Writer und vollständiger Provider-Readback
bleiben gemeinsame Produktionsautorität. Unsichere/unklare Ergebnisse führen
nie zum automatischen zweiten CREATE. Kein weiterer produktiver Legacy-CREATE.

## ST.V1.0 – Inventur und Feldmatrix

**Aus der Quellprüfung zu Beginn:** `SearchTimerCreateRequest` enthält bereits
umfangreiche epgsearch-Optionen. `RestfulApiSearchTimerMapper` liest viele
dieser Optionen aus `/searchtimers.json`. Die Public-v1-Dauerpayload Version 2
enthält dagegen nur Name, Suchbegriff, Aktivität, Match-Modus/Toleranz,
drei Suchbereiche sowie VDR-Margins. `SearchTimerCreateDispatchService`
überträgt/verifiziert nur diese Teilmenge. Die vorhandene Public-v1-Runtime
lehnt `active:true` explizit ab. Im Branch ist im Gegensatz dazu der
Legacy-Browser-CREATE wieder freigeschaltet. Das ist keine Zielarchitektur.

| UI / Benutzerfunktion | Public-v1 / dauerhafte Payload | Core / RESTfulAPI | Provider-Readback | Befund |
|---|---|---|---|---|
| Suchbegriff, Modus, Toleranz, Titel/Untertitel/Beschreibung | Teilmenge vorhanden | Vorhanden | Vorhanden | Teilweise verbunden |
| Aktiv/inaktiv | Nur `false` zugelassen | `active` vorhanden | Status vorhanden | Aktiv gesperrt |
| Groß-/Kleinschreibung, Summary-Match | Fehlt | Vorhanden | Vorhanden | Verbindung fehlt |
| Sender-Modus/Gruppe/Bereich/FTA | Fehlt | `useChannel`, `channels`, `channelMin/Max` | Teilweise vorhanden | Exakte Provider-Modi prüfen |
| Start/Ende, Wochentage, Dauer | Fehlt | Vorhanden | Vorhanden | Verbindung fehlt |
| Verzeichnis, Priorität, Lebensdauer, VPS, Margins | Nur VDR-Margins | Vorhanden | Vorhanden | Verbindung fehlt |
| Serienaufnahme, Aktionen, Aufbewahrung | Fehlt | Vorhanden | Vorhanden | Verbindung fehlt |
| Wiederholungsgrenzen/-vergleiche | Fehlt | Vorhanden | Teilweise vorhanden | Readback-Details prüfen |
| Blacklists, EPG-Kategorien/-Zusatzinformationen | Fehlt | Vorhanden | Teilweise vorhanden | Verlustfreie Abbildung prüfen |
| Gültigkeit, Favoriten, Folgeaktionen | Fehlt | Vorhanden | Vorhanden | Verbindung fehlt |

**Zu verifizieren, bevor produktive Felder geschrieben werden:** exakte
`SearchTimerRestfulApiBodyBuilder.inc`-Felder, Readback-Katalog, Parser,
Browser-Editor, historischer MLD-Stand, Public-v1-HTTP-Router, Operation-Version,
Sicherheitsgrenzen und die epgsearch-Plugin-Version. Bei einer nicht
nachweisbar unterstützten Option muss ein sichtbarer Validierungsfehler
statt stiller Feldverluste erscheinen.

## Umsetzungsslices mit Abnahme

- **ST.V1.0 Inventur:** Ist-Pfad `HTTP → Public-v1-Handler → DaemonRuntime →
  SearchTimerCreateAdmissionService → MutationOperationRepository →
  SearchTimerCreateDispatchService → RestfulApiSearchTimerCommandExecutor →
  /searchtimers.json Readback` und Browser-Legacy vergleichen; Feldmatrix
  nach tatsächlichem Provider ergänzen.
- **ST.V1.1 Public-v1-Vertrag:** Volle strukturierte Optionen in denselben
  Request aufnehmen, strikt validieren, Default-/Altrequests erhalten,
  Payload versionieren und Fingerprint/Idempotenz über alle Optionen binden.
  OpenAPI aktualisieren.
- **ST.V1.2 Core/Backend:** Felder ohne Informationsverlust bis zum vorhandenen
  RESTfulAPI-BodyBuilder übertragen. Keine zweite Service- oder Writer-Instanz.
  Agent-Generationsgrenze und einmalige Dispatch-CAS erhalten.
- **ST.V1.3 Native Readback:** Nach CREATE native neue ID, Backend, Aktivität
  und jede angeforderte wirksame Provider-Option gegen eine vollständige
  neue epgsearch-Inventur prüfen. Abweichung/Backendfehler bleibt
  `outcome_unknown`; keine Wiederholung. Provider-Ausnahmen separat notieren.
- **ST.V1.4 Webfrontend:** Vorhandenen vollständigen Editor erhalten und
  ausschließlich auf Public API v1 umstellen. Aktive/inaktive Auswahl;
  Operation/Readback und SSE für asynchrone Zustände statt zusätzlichem Polling.
  Browser- und Device-Credentials nicht vermischen.
- **ST.V1.5 Clients:** Einheitliches OpenAPI, Device-/Browser-Erstellungsweg,
  Referenz-JS-Client und dokumentierte Consumer-Felder. Keine Samsung-/VIDAA-
  Repository-Änderungen ohne Auftrag.
- **ST.V1.6 Tests:** Kontrakt-/Parser-/Payload-/Idempotenztests, aktive und
  inaktive Fälle, vollständige Feldkombinationen, gezielte Auth-Regressions,
  API-v1-Produktroute, Backendfehler, unbekanntes Ergebnis, Readback-Mismatch
  und doppelte Anlage; exakten C++-Kandidaten kompilieren.
- **ST.V1.7 Installation/Hardware:** Getrennte Paket-/Runtime-/VDR-Tests
  für yaVDR und MLD 6.6. Keine produktive Installation ohne Freigabe.
- **ST.V1.8 Abschluss:** Dokumentation und Status nach belegten Ergebnissen
  berichtigen, Legacy-CREATE produktiv schließen, Feldmatrix und
  Abschluss-SHA mit Ebenen UNIT/CONTRACT, PRODUCTION-INTEGRATION,
  CI und REAL-VDR getrennt festhalten.

## Abnahmekriterien

`active:true` und `active:false` ohne Feldverlust; genau ein Writer und
einmalige Mutation pro Idempotency-Key; native ID und alle schreibbaren Optionen
bestätigt; bestehende Grants unverändert wirksam; Editor nutzt ausschließlich
den gemeinsamen API-v1-Vertrag; alter Legacy-CREATE ist nicht wieder aktiv.
Quellcode-Implementierung und echter Build sind von Real-VDR-Abnahme zu trennen.

## Verifikationsstand

- AGENTS.md (1112 Zeilen) und die SuiteBridge-Unteranweisungen gelesen.
- Remote-Start-HEAD nachgewiesen; `main` nicht verändert.
- **Noch offen:** genaue Feldmatrix gegen Provider, Umsetzung aller Slices,
  produktiver C++-Build und Real-VDR-Tests.

---

### ST.V1.0 – Feldmatrix des tatsächlichen Quellpfads (52 wirksame Felder)

Die Tabellenzeilen wurden gegen `SearchTimerCreateRequest.h`, `SearchTimerRestfulApiBodyBuilder.inc`, `SearchTimerCreateDispatchService.cpp` abgeglichen. Der Browser formt dieselben fachlichen Schlüssel in `buildCreatePayload`; der Public-v1-JSON-Parser ordnet `mode→matchMode` und `tolerance→matchTolerance` zu. Alle Felder gehen über die **gemeinsame** `SearchTimerCreateOperationPayload` v3, `SearchTimerCreateAdmissionService` und `SearchTimerCreateDispatchService`. `RestfulApiSearchTimerMapper.cpp` bildet die GET-Inventur zurück auf `SearchTimer`.

| UI-/API-v1-Feld | Core/Operation | RESTfulAPI/epgsearch-Schlüssel | Native Readback | Source-Status |
|---|---|---|---|---|
| `name` | `payload.name` (v3) | `search` | `SearchTimer.name()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `query` | `payload.query` (v3) | `search` | `SearchTimer.query()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `active` | `payload.active` (v3) | `use_as_searchtimer` | `SearchTimer.state()` | im Quellcode verbunden; aktiver Real-VDR-Test ausstehend |
| `directory` | `payload.directory` (v3) | `directory` | `SearchTimer.recordingOptions().directory()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `priority` | `payload.priority` (v3) | `priority` | `SearchTimer.recordingOptions().priority()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `lifetime` | `payload.lifetime` (v3) | `lifetime` | `SearchTimer.recordingOptions().lifetime()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `marginStartMinutes` | `payload.marginStartMinutes` (v3) | `margin_start` | `SearchTimer.scheduleOptions().marginStartMinutes()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `marginStopMinutes` | `payload.marginStopMinutes` (v3) | `margin_stop` | `SearchTimer.scheduleOptions().marginStopMinutes()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `useVps` | `payload.useVps` (v3) | `use_vps` | `SearchTimer.scheduleOptions().useVps()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `useChannel` | `payload.useChannel` (v3) | `use_channel` | `SearchTimer.filterOptions().channelMode()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `channels` | `payload.channels` (v3) | `channels` | `SearchTimer.channelOptions().channels()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `channelMin` | `payload.channelMin` (v3) | `channel_min` | `SearchTimer.channelOptions().channelMinId()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `channelMax` | `payload.channelMax` (v3) | `channel_max` | `SearchTimer.channelOptions().channelMaxId()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `useTime` | `payload.useTime` (v3) | `use_time` | `SearchTimer.filterOptions().useTime()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `startTime` | `payload.startTime` (v3) | `start_time` | `SearchTimer.filterOptions().startTime()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `stopTime` | `payload.stopTime` (v3) | `stop_time` | `SearchTimer.filterOptions().stopTime()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `useDuration` | `payload.useDuration` (v3) | `use_duration` | `SearchTimer.filterOptions().useDuration()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `durationMinMinutes` | `payload.durationMinMinutes` (v3) | `duration_min` | `SearchTimer.filterOptions().durationMinMinutes()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `durationMaxMinutes` | `payload.durationMaxMinutes` (v3) | `duration_max` | `SearchTimer.filterOptions().durationMaxMinutes()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `useDayOfWeek` | `payload.useDayOfWeek` (v3) | `use_dayofweek` | `SearchTimer.filterOptions().useDayOfWeek()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `dayOfWeek` | `payload.dayOfWeek` (v3) | `dayofweek` | `SearchTimer.filterOptions().dayOfWeek()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `avoidRepeats` | `payload.avoidRepeats` (v3) | `avoid_repeats` | `SearchTimer.repeatOptions().avoidRepeats()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `allowedRepeats` | `payload.allowedRepeats` (v3) | `allowed_repeats` | `SearchTimer.repeatOptions().allowedRepeats()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `repeatsWithinDays` | `payload.repeatsWithinDays` (v3) | `repeats_within_days` | `SearchTimer.repeatOptions().repeatsWithinDays()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `compareTitle` | `payload.compareTitle` (v3) | `use_title` | `SearchTimer.comparisonOptions().compareTitle()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `compareSubtitle` | `payload.compareSubtitle` (v3) | `use_subtitle` | `SearchTimer.comparisonOptions().compareSubtitle()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `compareSummary` | `payload.compareSummary` (v3) | `use_description` | `SearchTimer.comparisonOptions().compareSummary()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `compareCategories` | `payload.compareCategories` (v3) | `compare_categories` | `SearchTimer.comparisonOptions().compareCategories()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `compareTime` | `payload.compareTime` (v3) | `compare_time` | `SearchTimer.comparisonOptions().compareTime()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `useSeriesRecording` | `payload.useSeriesRecording` (v3) | `use_series_recording` | `SearchTimer.seriesOptions().useSeriesRecording()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `keepRecordings` | `payload.keepRecordings` (v3) | `keep_recs` | `SearchTimer.seriesOptions().keepRecordings()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `deleteMode` | `payload.deleteMode` (v3) | `del_mode` | `SearchTimer.seriesOptions().deleteMode()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `searchTimerAction` | `payload.searchTimerAction` (v3) | `search_timer_action` | `SearchTimer.seriesOptions().searchTimerAction()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `blacklistMode` | `payload.blacklistMode` (v3) | `blacklist_mode` | `SearchTimer.blacklistOptions().blacklistMode()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `blacklistIds` | `payload.blacklistIds` (v3) | `blacklist_ids` | `SearchTimer.blacklistOptions().blacklistIds() [CSV-normalisiert]` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `mode` | `payload.matchMode` (v3) | `mode` | `SearchTimer.matchOptions().mode()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `matchCase` | `payload.matchCase` (v3) | `match_case` | `SearchTimer.matchOptions().matchCase()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `tolerance` | `payload.matchTolerance` (v3) | `tolerance` | `SearchTimer.matchOptions().tolerance()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `summaryMatch` | `payload.summaryMatch` (v3) | `summary_match` | `SearchTimer.matchOptions().summaryMatch()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `useExtendedEpgInfo` | `payload.useExtendedEpgInfo` (v3) | `use_ext_epg_info` | `SearchTimer.extendedEpgOptions().useExtendedEpgInfo()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `extendedEpgInfo` | `payload.extendedEpgInfo` (v3) | `ext_epg_info` | `SearchTimer.extendedEpgOptions().extendedEpgInfo() [CSV-normalisiert]` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `ignoreMissingEpgCategories` | `payload.ignoreMissingEpgCategories` (v3) | `ignore_missing_epg_cats` | `SearchTimer.extendedEpgOptions().ignoreMissingEpgCategories()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `contentDescriptors` | `payload.contentDescriptors` (v3) | `content_descriptors` | `SearchTimer.extendedEpgOptions().contentDescriptors()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `useInFavorites` | `payload.useInFavorites` (v3) | `use_in_favorites` | `SearchTimer.validityOptions().useInFavorites()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `activeFrom` | `payload.activeFrom` (v3) | `use_as_searchtimer_from` | `SearchTimer.validityOptions().activeFrom()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `activeUntil` | `payload.activeUntil` (v3) | `use_as_searchtimer_til` | `SearchTimer.validityOptions().activeUntil()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `pauseOnRecordings` | `payload.pauseOnRecordings` (v3) | `pause_on_recs` | `SearchTimer.actionOptions().pauseOnRecordings()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `switchMinutesBefore` | `payload.switchMinutesBefore` (v3) | `switch_min_before` | `SearchTimer.actionOptions().switchMinutesBefore()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `unmuteSoundOnSwitch` | `payload.unmuteSoundOnSwitch` (v3) | `unmute_sound_on_switch` | `SearchTimer.actionOptions().unmuteSoundOnSwitch()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `deleteRecordingsAfterDays` | `payload.deleteRecordingsAfterDays` (v3) | `del_recs_after_days` | `SearchTimer.actionOptions().deleteRecordingsAfterDays()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `deleteAfterCountRecordings` | `payload.deleteAfterCountRecordings` (v3) | `del_after_count_recs` | `SearchTimer.actionOptions().deleteAfterCountRecordings()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |
| `deleteAfterDaysOfFirstRecording` | `payload.deleteAfterDaysOfFirstRecording` (v3) | `del_after_days_of_first_rec` | `SearchTimer.actionOptions().deleteAfterDaysOfFirstRecording()` | im Quellcode verbunden; Provider-Roundtrip unbestätigt |

**Nicht eins zu eins als JSON-Array übertragen:** `blacklistIdList` und `extendedEpgInfoList` sind zusätzliche **interne** Provider-Vektorvarianten; Public v1 nutzt die gleichbedeutenden `blacklistIds` und `extendedEpgInfo` als kommagetrennte Strings. Der vorhandene RESTfulAPI-BodyBuilder erzeugt native Arrays. Eine v3-Dauerpayload mit nichtleeren internen Vektoren wird ausdrücklich abgelehnt statt still zu verlieren. `backendId` kommt autoritativ aus dem URL-Scope und der Operation, nicht aus einem frei wählbaren JSON-Feld. Für `name` und `query` verlangt Public v1 denselben Suchtext, weil der Provider in `search` keinen eigenen Anzeigenamen speichert.

### Tatsächlicher Implementierungsstand (source-only, 10.11.2026)

| Slice | Status | Grenze |
|---|---|---|
| ST.V1.0 | Quellinventur und Feldmatrix im Branch dokumentiert | Kein Real-Provider-Test |
| ST.V1.1 | JSON-Parser, DTO, OpenAPI, `active:true/false`, vollständige Payload v3 und Fingerprint implementiert | C++ nicht kompiliert |
| ST.V1.2 | Existing Controller/Admission/Operation/Backend-Agent/RESTfulAPI weiterverwendet; 52 wirksame Felder bis zum Provider-Body geführt | Echte Provider-Response noch offen |
| ST.V1.3 | Native Vorher-/Nachher-Inventur, exklusive neue ID, Feldvergleich, Generationsprüfung, UNKNOWN ohne Retry implementiert | Kein echter epgsearch-Readback bestätigt |
| ST.V1.4 | Vorhandener Browser-Editor sendet v1, bietet aktiv/inaktiv, Operation/SSE und bewahrt alle Felder; Legacy Browser-Writer gesperrt | Browser-Test nicht auf echtem Webserver ausgeführt |
| ST.V1.5 | OpenAPI und Reference-JS `createSearchTimer`, alte inactive-Wrapper-Methode über denselben API-v1-Weg | VIDAA hat einen vereinfachten aktiven Device-CREATE, Samsung einen read-only Client; beide TV-Hardwaretests offen |
| ST.V1.6 | C++/JS-Testfälle und zusätzliche Readback-/Negativfälle quellsseitig ergänzt | **Kein C++-Build und keine Node-CLI-Tests auf dem echten Checkout.** Drei JS-Testdateien wurden in isolierter V8 mit Node-Modul-Shims ausgeführt und bestanden |
| ST.V1.7 | Source-Kandidat vorbereitet; keine Installation/Neustarts/DB-Backups vorgenommen | MLD 6.6 und yaVDR Hardware-Tests **offen** |
| ST.V1.8 | API-/OpenAPI-Dokumentation und Roadmap aktualisiert | Abschluss erst nach belastbaren Build-/Integrationstests |

**Definition of Done: Noch NICHT erfüllt.** Änderungen sind auf dem Arbeitsbranch; keine produktive Installation und kein Merge nach `main`. Insbesondere bedeutet ein Commit oder ein Mocktest kein REAL-VDR PASS. Ohne Zugriff auf den tatsächlichen Host-Checkout und Build-Cache dürfen keine C++-/Node-/MLD-Testresultate behauptet werden.

**Noch erforderliche Abnahme:** gezielte C++-Targets `test-searchtimer-create-durable-admission`, `test-searchtimer-create-one-shot-dispatch`, `test-searchtimer-create-orphan-recovery`, `test-searchtimer-create-device-admission`, `test-restful-api-search-timer-command-executor`, `test-public-timer-assignment-collection`; Node-Tests `web/frontend/tests/test_searchtimer_workflows_runtime.js`, `web/frontend/tests/test_phase69f_client_error_mutation_fallback.js`, `clients/reference-js/tests/test_public_v1_search_timer_client.js`, plus OpenAPI-Checker. Danach inkrementellen **Kandidaten-Daemon** bauen und den tatsächlichen VDR/epgsearch-Readback separat auf MLD 6.6 und yaVDR prüfen. Bei jedem Testfehler den Branch korrigieren, nicht die Tests als PASS kennzeichnen.

---

### ST.V1 – Nachprüfung und gezielte Korrekturen (10.11.2026)

Ausgangsstand für diese Nachprüfung war `0c1364d889ab280f4966f27e520289d03f0883f4`
auf **`MLD-Suchtimer`**, nicht der ältere in der Aufgabenstellung genannte
`037e0545a89e5c86670563b6ce046f24947704e1`. Dieser vorhandene
Branch enthielt bereits die Payload-v3-Umsetzung und die 52-Felder-Matrix.
Es wurde **keine** zweite produktive CREATE-Architektur geschrieben.

**Quellbefund und behobene konkrete Abweichungen:**

1. `b93ad280ac2a9c0b46e23d60e66d327f34803f88`:
   `/api/v1/operations/{operationId}/events` sendet nur ein
   autorisiertes `operationId`-Änderungsereignis, keine vollständige Operation
   mit `state`. Der Editor wertete den SSE-Hinweis fälschlich als Zustand aus
   und ignorierte ihn. Jetzt liest er auf einen passenden Hinweis die
   **bereits bestehende actor-gebundene Public-v1-GET-Ressource**, synchronisiert
   auf `hello` und `resync` und fasst überlappende Hinweise zusammen.
   Kein Polling und keine erneute Schreiboperation.
2. `6dc14c2a23bdd5ad39c08d7fb315535bb1c3bbaf`:
   Eindeutig abgewiesene `422 validation_error`-CREATE-Aufrufe geben den
   Formular-Submit für einen **ausdrücklich vom Benutzer ausgelösten** neuen
   Versuch nach Eingabekorrektur wieder frei. Bei unklarem Transportfehler
   oder `409` bleibt der erneute CREATE gesperrt.
3. `7104b09f9efab8e67d54cd01cdcb79a380ad2300`:
   Der exportierte Browser-Validator akzeptiert nun den durch OpenAPI
   und epgsearch unterstützten Regex-Modus 4; ungültiger Modus 6 wird weiter
   abgelehnt. Bestehender Editor und API-v1-Writer bleiben identisch.

**Verifizierte Quell- und JS-Evidenz:**

- Aus `api/openapi.json` extrahierte CREATE-Body-Definition: 53 Felder,
  davon 52 fachliche Felder und `confirmed`. Alle 53 Schlüssel wurden im
  Public-v1-Parser gefunden, alle 52 fachlichen Felder in der Daemon-Zuordnung,
  der v3-Operations-Serialisierung und in `buildCreatePayload`. Dieser
  Abgleich ist **statisch**, kein Provider-Roundtrip.
- `web/frontend/tests/test_searchtimer_workflows_runtime.js` vollständig
  in isolierter V8-Ausführung mit Node-Modul-Shims getestet: PASS,
  einschließlich aktiv/inaktiv, erweiterter Optionen, SSE, 422-Recovery,
  unklarem POST-Ausgang und Regex-Modus. Die komplette Testdatei wurde
  ausgeführt, aber **nicht durch Node auf dem echten Checkout**.
- `clients/reference-js/tests/test_public_v1_search_timer_client.js`
  vollständig in isolierter V8-Ausführung mit Node-Modul-Shims: PASS
  (inkl. aktiver und inaktiver CREATE-Varianten).
- `web/frontend/tests/test_phase69f_client_error_mutation_fallback.js`
  vollständig in isolierter V8-Ausführung mit Node-Modul-Shims: PASS.
  Das ersetzt weder produktive Browser-/HTTP-Tests noch Node-CLI-Tests.

**Nicht ausgeführt / nicht als PASS markieren:**
C++-Kompilation der 20 vorbestehenden SearchTimer-Quellcommits, C++-Tests,
echter API-v1-HTTP-Produktionspfad, natives RESTfulAPI-/epgsearch-Readback,
MLD-6.6-Paketprüfung, yaVDR-Installation oder Real-VDR-Abnahme. In dieser
Arbeitsumgebung existiert **kein verifizierter yaVDR-Checkout oder dessen
inkrementeller Objektcache**; die GitHub-Anbindung ist nur für
Quellcode-Lesen/Commits verfügbar. Es wurde nicht behauptet, dass ein
GitHub-Commit einen Build nachweist. Auf dem Arbeitsbranch lag zum
Prüfzeitpunkt kein GitHub-Actions-Run vor. Keine Installation,
kein Dienstneustart, keine Datenbanksicherung, kein Branch-/Worktree-Neubau
und kein Merge nach `main`.

**Noch offenes Gate:** Den **exakten Produktkandidaten** mit dem vorhandenen
`OBJECT_CACHE_DIR` auf dem bestehenden Checkout kompilieren, die
fokussierten SearchTimer-C++-Targets und Node-Tests ausführen und alle
auftretenden Fehler auf `MLD-Suchtimer` korrigieren. Erst anschließend
reale MLD-/yaVDR-epgsearch-Readbacks getrennt freigeben und nachweisen.
Die Definition of Done ist bis dahin **nicht erfüllt**.

---

## ST.V1 – Konsolidierung und Übergabe nach Chat-Wechsel (11.11.2026)

**Entscheidung: Eine einzige Public API v1, keine App-spezifischen Versionen.**
Der fachliche SearchTimer-Vertrag wird ausschließlich in
`hotzenplotz5/vdr-suite/api/openapi.json` und
[`api/search-timers.md`](../../api/search-timers.md) definiert.
`MLD-Suchtimer` ist der bereits genehmigte Arbeitsbranch; `main`
und alle TV-Repositorys werden in diesem Abschluss nicht verändert.
Es darf weder eine zweite CREATE-Route noch ein eigener epgsearch-Writer
für MLD, yaVDR, VIDAA oder Samsung entstehen. Das Version-3-Schema der
**internen** dauerhaften CREATE-Payload ist **keine Public API v3**.

### Cross-Repository-Inventur (Quellstand, kein Hardware-PASS)

| Repository/Branch | Suchtimer-Funktion heute | Abgleich mit ST.V1 |
|---|---|---|
| VDR-Suite / `MLD-Suchtimer` | vollständiger Browser- und Device-CREATE via Public v1; native vor/nach CREATE-Inventur | C++-Build, produktive Route und echtes epgsearch-Readback noch nicht nachgewiesen |
| VDR-Suite / `main` | ältere, eingeschränkte Public-v1-CREATE-Implementierung, nur inaktiv | nicht als Beweis für die neue aktive CREATE-Fähigkeit verwenden |
| VIDAA / `work/vidaa-hardware-acceptance-v6-after-logos-20261017` (geprüft `99ee4f8ad716`) | `app/src/searchtimer-browser.js` und `app/src/device-pairing-client.js` mit Device-`POST /search-timers?backend=`, `active:true`, vereinfachten Optionen und Operation-GET | kein eigener Provider/Writer; Vollfeld-/Regex-Editor nicht vorhanden; funktioniert nur mit geeignetem Serverstand und echten Grants |
| Samsung / `work/s6-tv-api-v1` (geprüft `b74b75362677`) | `app/src/api/search-timers.js` nur Liste/Detail/Preview via Public-v1-Device-GET | CREATE fehlt im Client; später dünne UI/Device-Anbindung nach freigegebenem Serververtrag |
| MLD 6.6 / yaVDR | Linux-Distributionen, die denselben VDR-Suite-Service betreiben | Paket-/Agent-/epgsearch-Kompatibilität testen, keine neue API-Schicht implementieren |

**Konkrete Überlappung:** VIDAA sendet bereits
`active:true`; auf aktuellem Server-`main` wurde das früher mit
422 abgewiesen. Der Kandidat `MLD-Suchtimer` erweitert denselben
Public-v1-Endpunkt um `active:true/false` und vollständige Optionen.
Die vorhandenen TV-UI-Module sind Clientcode, **keine konkurrierenden
API-Versionen**. Samsung hat im inspizierten Entwicklungsbranch
noch keinen Suchtimer-CREATE. Diese Befunde sind keine Aussagen über die
installierten oder am TV ausgeführten Versionen.

### Priorisierte Restarbeiten (keine neue Phase und keine Parallel-API)

1. **Exakten aktuellen Remote-HEAD** von `MLD-Suchtimer` erneut lesen und
   die seit `8c0440be2ff7ecc47b53c265235eb8dd1fcca46a`
   hinzugekommenen **reinen Dokumentationscommits** getrennt vom
   letzten Produktcode-SHA behandeln. Bereits verifizierte Tests und
   Build-Caches nicht wegen Doku-Änderungen verwerfen.
2. **Gezielte Tests**: Node-CLI für die drei bekannten JS-Tests,
   `python3 tools/check_public_v1_openapi.py`, relevante C++-Make-
   Targets für Admission, One-Shot, Orphan-Recovery, Device-Admission,
   RESTfulAPI-Executor, Public-v1-Router und native Mapper. Exakte
   Targetnamen vor Ausführung aus den vorhandenen `mk/*.mk` übernehmen.
   Reale Kompilier- oder Laufzeitfehler **auf demselben Branch** beheben.
3. **Produktivpfad nachweisen**: HTTP/CSRF-/Device-Authentifizierung →
   Public-v1-Controller → Operation → genau ein RESTfulAPI-POST →
   eindeutige neue native ID/Backendgeneration → voller
   epgsearch-Readback → dauerhafter `succeeded`- oder
   `outcome_unknown`-Status, einschließlich SSE/Operation-GET.
   Testfälle für aktiv und inaktiv, Providerfehler, unklare Antwort,
   idempotentes Replay und vollständige Optionen; kein blindes Retry.
4. **Installations-/Hardwareabnahme separat**: vorhandene
   Build-Artefakte/Objektcache vor Neubau prüfen. Erst danach
   kontrollierte yaVDR-/MLD-6.6-Pakete und echte VDR-Tests mit
   ausdrücklich autorisierter Mutation; **nicht** aus einem
   Quellcode-Mocktest auf Hardware-PASS schließen.
5. **Client-Parität danach**: VIDAA nur an den bestätigten Serververtrag
   anpassen; Samsung CREATE später durch den gleichen Endpunkt
   ergänzen. Kein gleichzeitiger Doppelumbau der TV-Repositories
   während ST.V1.6/1.7. Jede App hat eigene Bedienung, aber keinen
   eigenen epgsearch-Domänenservice. Genau einen Abnahmebeleg
   pro Ebene festhalten: UNIT/CONTRACT, PRODUCTION-INTEGRATION,
   CI und REAL-VDR.

**Übergabestand:** Dokumentation und Source-Inventur sind fertig.
**Nicht fertig** sind ein bestätigter C++-Kompilierlauf des Kandidaten,
die produktive Runtime-Integration und MLD-/yaVDR-/TV-Hardwareabnahmen.
Kein Merge nach `main`, kein CI-Warten, keine neue Branch-/Worktree-
Anlage, keine DB-Sicherung und kein Deployment durch diesen Dokumentationsschritt.
