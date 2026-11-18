# Public-v1 VDR SearchTimers: read, preview, complete active/inactive CREATE and stock epgsearch management

**Historischer Provider-Nachweis 24.10.2026:** Der bestandene [RESTfulAPI-CRUD-Smoke-Test](../docs/development/searchtimer-yavdr-smoke-test-execution-report.md) bleibt gültig und wird nicht wiederholt. Die neue Public-v1-Verwaltung benötigt einen eigenen End-to-End-Nachweis.

**Architekturentscheidung 17.11.2026 (Betreiber):** Reguläres epgsearch ist die produktive Basis. Keine epgsearch-Patches oder gepatchte ABI als Installations- oder Freigabevoraussetzung. CREATE/LIST über RESTfulAPI; VERSION/UPDATE/ACTIVATE/DEACTIVATE/DELETE verwenden die vorhandenen Standard-SVDRP-Befehle intern. SQLite-Operationsreservierung, Idempotenz, ETag, Autorisierung und Readback bleiben; native epgsearch-lock-atomare CAS oder persistierte native Generationen werden nicht versprochen. Externe konkurrierende Änderungen zwischen Lesen und Schreiben sowie identische Löschung/Neuanlage zwischen Beobachtungen sind nicht vollständig erkennbar. Historische native-CAS-Abnahmeforderungen sind damit aufgehoben. Ursprünglicher Source-Kandidat `eaa8ab6e122fbb8723456df091b6a57a12311653`; aktueller installierter und im yaVDR-Webfrontend getesteter Code-Kandidat `b5ae3cc27cb25f6950e641781060a1b5a68f510d` (17.11.2026).

The current implemented version is **Public API v1**, not v2.

## Aktueller ST.V1.7-Integrationsstand (17.11.2026)

**Aktueller yaVDR-Web-Nachweis (17.11.2026):** Code `b5ae3cc27cb25f6950e641781060a1b5a68f510d` installiert und laufend; C++-Build, Installation, Frontend-Dateivergleich und [VDR-Suite CI #12790](https://github.com/hotzenplotz5/vdr-suite/actions/runs/44098825862) PASS. CREATE (inaktiv), LIST, UPDATE (Suchbegriff), ACTIVATE, DEACTIVATE und DELETE sind im Browser vom Betreiber bestätigt. Der vorhandene Test-Suchtimer wurde aktiviert, deaktiviert und gelöscht; die Liste zeigt anschließend 0 SearchTimer. Der HTTP-503-Blocker der getesteten Verwaltung ist beseitigt. Keine Wiederholung dieser bestätigten Aktionen. Aktiver CREATE, vollständiges Feld-UPDATE, separat erfasste neue Operations-/Provider-Evidenz, Preview-503-Abnahme und MLD-/TV-Device-End-to-End bleiben im jeweils unbelegten Umfang offen.
Siehe [zentrale Abnahme](../docs/development/public-v1-final-hardware-acceptance.md#stv17--yavdr-web-praxisabnahme-17112026).

Der Web-Browser verwendet LIST (alle Cursor-Seiten), DETAIL, VERSION,
gespeicherte PREVIEW sowie UPDATE, ACTIVATE, DEACTIVATE und DELETE über
Public API v1. Die bereits bestehende Browser-Administrator-Session und
CSRF-Prüfung werden wie bei CREATE wiederverwendet. Die Device-Grants
bleiben unverändert. Browser-LIST/DETAIL enthalten zusätzlich `settings`
mit denselben bekannten Feldgruppen wie die frühere Web-Liste; die
Device-Projektion bleibt kompakt und kein nativer Regel-ID-Wert wird
öffentlich. Der bisherige installierte Nachweis umfasst den Suchbegriff einer
inaktiven Regel. MLD.F3 erweitert den bestehenden UPDATE-Pfad für aktive und
inaktive Regeln um die übrigen Einstellungen (Quellstand; Realtest offen).
Aktive Regeln können deaktiviert, inaktive aktiviert und Regeln gelöscht
werden. Die UI sendet keinen Legacy-DELETE mehr, liest die Revision und
zeigt den Status der unveränderlichen Operation über die bestehende
actor-owned SSE/GET-Kette. Die Vorschau ist eine EPG-Trefferliste, keine
verbindliche Aufnahmeprognose; der explizite Cache-Refresh verwendet den
vorhandenen dedizierten EPG-Transport.

**Nachweis:** Browser-Workflow-Regression in isoliertem JavaScript/V8 mit
Mock-DOM bestanden (einschließlich vier Mutationen, SSE, abgelehnter
Version und unklarem POST-Ausgang); C++-Tests hinzugefügt, noch nicht
ausgeführt. Code-Kandidat `ac3d304671cd27ae304674d6611e5790cfcd5b64`. Die bisherigen vom Betreiber
bestätigten inaktiven CREATE-/Legacy-Leseergebnisse bleiben gültig.
Dieser frühere Source-Checkpoint ist durch den oben dokumentierten yaVDR-Browser-Nachweis und C++-Build aktualisiert; aktiver CREATE und Device-/MLD-Abnahme bleiben offen.
Die Device-only-Angaben in historischen Abschnitten beschreiben die
Device-Bedingungen, nicht mehr die Browser-Administrator-Zulassung.

## Einheitlicher Serververtrag für Web, MLD, yaVDR, VIDAA und Samsung

**Architekturgrenze (Stand 11.11.2026):** `hotzenplotz5/vdr-suite` ist
der **einzige produktive Eigentümer** der epgsearch-Suchtimer-API,
des CREATE-Controllers, der Operations-Persistenz, des RESTfulAPI-Writers
und des nativen Readbacks. MLD und yaVDR installieren denselben
VDR-Suite-Serververtrag; Web, VIDAA und Samsung sind unterschiedliche
Oberflächen und **keine eigenständigen Suchtimer-Backends**. Eine
Clientbibliothek oder TV-Ansicht ist keine weitere API-Version.

| Verbraucher | Tatsächliche Quelle / Funktion | Noch offen |
|---|---|---|
| Webfrontend (VDR-Suite) | `web/frontend/modules/searchtimers.js`: vollständiger aktiver/inaktiver Public-v1-CREATE mit bestehendem Browser-Login, SSE-Hinweis und Operation-GET | Produktivbuild und Browser-/VDR-Abnahme |
| MLD 6.6 / yaVDR | Dieselben `/api/v1/search-timers`-Endpunkte im distributionsübergreifenden Server; keine eigene MLD-API | Reale Paket-/Host-Abnahme auf beiden Distributionen |
| VIDAA (separates Repository) | Entwicklungsbranch `work/vidaa-hardware-acceptance-v6-after-logos-20261017`: `searchtimer-browser.js` und `device-pairing-client.js` nutzen Public-v1-Liste, Preview, vereinfachten CREATE mit `active:true` und Operation-GET | Client-Feldumfang abgleichen, echtes Hisense-Gerät testen |
| Samsung Tizen (separates Repository) | Entwicklungsbranch `work/s6-tv-api-v1`: `app/src/api/search-timers.js` bietet bisher nur Public-v1-Liste, Detail, Preview | CREATE als dünnen Device-Client ergänzen; Tizen-Hardwaretest |

Die TV-Branch-Befunde sind **Quellstandprüfungen**, keine
Geräteabnahmen. Auf dem Server-`main` besteht weiterhin der ältere
inactive-only-CREATE-Vertrag. **Erst** der ungemergte Branch
`MLD-Suchtimer` enthält den unten beschriebenen erweiterten
`active:true/false`-Vertrag. Kein TV-Client darf aus dem Dokument
ableiten, diese Erweiterung sei bereits auf dem laufenden VDR installiert.

**Verbindliche Integration:** Eine einzige veröffentlichte
`api/openapi.json` und dieser Serververtrag bestimmen die Felder.
TV-Clients nutzen die vorhandene Device-Authentifizierung und den
bestehenden backendgebundenen `searchtimers.create`-Grant; das
Webfrontend die vorhandene Browser-Session/CSRF. Verschiedene
Anmeldungen bedeuten **keine** getrennten Domänenservices, Operationen,
Suchtimer-Provider oder Writer. Keine neue `/api/vdr/searchtimers`-
Fallback-Mutation, kein TV-eigenes epgsearch-Protokoll, kein Client-POST
bei unklarem Ausgang und keine neue API-Hauptversion für Client-Parität.

**Geltungsbereich:** CREATE ist Gegenstand von ST.V1. Bestehende
read-only LIST/DETAIL/PREVIEW und native versionierte
UPDATE/ACTIVATE/DEACTIVATE/DELETE-Verträge sind andere Operationen und
werden weder durch CREATE ersetzt noch ohne eigene Abnahme freigegeben.
Die Draft-Preview hat ausdrücklich einen kleineren EPG-Match-Only-Vertrag
und darf nicht als vollständige CREATE-Simulation bezeichnet werden.
Siehe [ST.V1-Roadmap](../docs/planning/mld-searchtimer-public-v1-roadmap.md).


`GET /api/v1/search-timers?backend=<backendId>&limit=30[&cursor=<opaque>]`

- This is **VDR SearchTimer**, distinct from native VDR Timer and Suite TimerAssignment.
- Requires an authenticated, verified **Device credential** and an **independently granted** `searchtimers.view` permission for exactly one backend. `timers.view`, `epg.view`, and `channels.view` do not grant access. Browser administrators may use their existing scoped administrator session; ordinary Browser sessions are not a substitute for Device authorization.
- Administrators may independently grant/revoke `searchtimers.view` and backend-scoped `searchtimers.create` through Device grant administration. Read permission never implies create permission.
- `backend` is mandatory, length 1–128, characters A-Z, a-z, 0-9, dot, underscore, hyphen. Unknown/duplicate query fields and invalid/mismatched cursors yield 400.
- Pages are limited to 1–30 items (default 30), ordered by the **server-owned stable opaque** `searchTimerId`. `st1_` cursors bind the backend. Only a bounded page is returned to the TV. The provider itself currently exposes `/searchtimers.json` as a catalogue: each page request performs one read on only that backend; **no backend fanout or retries**.
- `items` contain only `searchTimerId`, `backendId`, `name`, `query` and `state` (`active`, `inactive`, or `unknown`). No native IDs, provider payload, recording directory, or settings used for editing are exposed.
**Saved rule detail (read-only):**

`GET /api/v1/search-timers/{searchTimerId}?backend=<backendId>`

- Uses the **same** complete authoritative epgsearch catalogue and opaque
  Public-v1 identity reconciliation as the list. Requires a verified paired
  Device with independent `searchtimers.view@backend`; an EPG grant is **not**
  required just to read name, query and active state. No fallback from a non-admin Browser session.
- Exactly one canonical `backend` query field is supported, with no cursor,
  limit, aliases or extra parameters. ID must match `st_[0-9a-f]{32}`.
- 200 returns `{searchTimerId,backendId,name,query,state,meta:{partial:false}}`;
  404 means the opaque ID is not present after successful reconciliation;
  503 means a complete provider snapshot cannot be trusted or obtained.
  Old retired IDs never select a newly reused native rule.
- It exposes no native ID, provider internals, recording options, revision
  token or write capability. `PATCH`/`PUT`/`DELETE` remain unavailable;
  the native rule-generation CAS prerequisite is unchanged.

- IDs are persisted under `public_search_timer_identity`; the native mapping is internal and does not make these TimerAssignments. A **complete and successful** native SearchTimer catalogue read reconciles the backend's active mappings in one SQLite transaction. Disappeared native IDs are retired, and retired public IDs are permanently reserved. Backend errors or malformed/incomplete provider responses do not retire anything. A reused native ID therefore gets a fresh public ID **once its earlier absence has been observed**.
- To migrate legacy identity rows that have no verified rule signature, the **first successful catalogue read rotates those legacy public IDs**, even for a still-present rule. Changes in observable rule settings also conservatively rotate the ID; a pure active/inactive toggle does not. An old ID can no longer select another rule through the saved-rule preview. Consumers must refresh cards and discard old IDs after migration/change.
- **Identity boundary:** Public IDs are reconciled against the observed catalogue; identical deletion/recreation between observations cannot be detected with stock epgsearch. Mutations use full-record optimistic revision checks, not native atomic CAS.
- A genuinely empty provider list is HTTP 200 with `items: []`. An unavailable provider, unavailable runtime, or identity failure is HTTP 503, not an empty successful response. Missing/invalid Device credentials yield 401 or 403, unauthorized backend 403; all use the Public-v1 Problem model.
- The device collection GET remains compact. Browser administrators receive the existing settings projection; the version endpoint also carries exact native repeat and activation modes. See the mutation contract below.
- Public response is `no-store`. On permission revocation a client must clear stale SearchTimer cards, not keep displaying a successful prior list.

See `api/openapi.json`, `docs/planning/timer-searchtimer-parity.md` (VIDAA repository), and `docs/client-integration-playbook.md` (VIDAA repository).

Source CI and package builds are required for the exact integration commit; the single combined real-VDR acceptance is separately gated by `docs/development/public-v1-final-hardware-acceptance.md`.

## Preview for an existing saved rule (read-only)

`GET /api/v1/search-timers/{searchTimerId}/preview?backend=<backendId>[&limit=30]`

- Paired verified Device only. Requires **both** `searchtimers.view` and
  `epg.view` for precisely the requested backend; no browser cookie fallback.
- Uses the Webfrontend's existing `SearchTimerPreviewService` and *ready*
  backend-scoped SearchTimer EPG cache. No second search algorithm.
- Returns `searchTimerId`, `backendId`, `totalCount`, up to 30 EPG
  `items` (title, subtitle, channelId, startTime, endTime) and
  `meta: {partial:false, epgInput:"ready"}`. Zero matches is 200 with [].
  Internal native SearchTimer IDs and provider payloads remain private.
- Missing/stale opaque rule yields 404; unavailable provider, offline backend
  or not-yet-ready EPG cache yields 503, never a fake empty preview.
- The `backend` parameter must match the Device backend authorization;
  `limit` is 1..30. Unknown, repeated, cursor and offset parameters fail.
  Response is no-store.
- This previews **existing** rules only, not a new-rule form. CREATE and version-fenced mutations use separate routes and authorization; this read-only preview does not evaluate full blacklist/duplicate policy, allow lossless settings editing or mutate native VDR Timers.

## Draft preview of a new rule (Device, read-only)

`GET /api/v1/search-timers/draft-preview?backend=<id>&query=<encoded>&mode=<0|1|2|3|5>&compareTitle=<0|1>&compareSubtitle=<0|1>&compareSummary=<0|1>[&tolerance=<0..3>][&limit=<1..30>]`

- Verified paired Device with both `searchtimers.view@backend` and `epg.view@backend`. A Browser administrator uses the same service; no mutation, no native ID or backend credential.
- `backend` must be explicit and exactly scoped; `query` is URL encoded, 2–240 decoded bytes; complete query string ≤900 bytes. Every comparison flag and the mode must be explicit. Allowed modes: phrase 0, all words 1, any word 2, exact 3, fuzzy 5. Regex and unmodeled controls rejected. `tolerance` is valid only for fuzzy matching.
- Uses the **same** `SearchTimerPreviewService` and ready backend EPG cache as the existing rule preview and Web. Only its implemented EPG text comparison/match semantics are covered; channel/duration filters, blacklists, repeat avoidance, recording settings and full epgsearch planning are **not evaluated** and are therefore rejected if supplied. Clients must label the result **EPG match-only, not an exact recording forecast**.
- Response: `{backendId,totalCount,items:[{title,subtitle,channelId,startTime,endTime}],meta:{partial:false,epgInput:"ready",evaluation:"epg-match-only"}}`. `limit` default/max 30. Empty valid match is 200/`[]`; cache/backend unavailable 503. 400 for unsupported options, duplicate/oversized/mismatched query; 401/403 for authentication/grants. `Cache-Control: no-store`.
- **Does not authorize CREATE**. A real SearchTimer CREATE requires separate permission, backend write policy, idempotency, and authoritative post-creation readback. Do not reinterpret this GET as permission to save a rule.

## CREATE: vollständige aktive und inaktive epgsearch-Regel über Public API v1

**ST.V1-Entwicklungsstand (10.11.2026, Branch `MLD-Suchtimer`):**
Der Quellcode des gemeinsamen CREATE-Pfads wurde erweitert. **Das ist
keine Bestätigung einer erfolgreichen produktiven Kompilierung, Installation
oder MLD-/yaVDR-Hardwareabnahme.** Die historischen Provider-Smoke-Tests
dürfen nicht als Prüfung dieses neuen Kandidaten ausgegeben werden.

`POST /api/v1/search-timers?backend=<backendId>`

Erforderlich sind `Idempotency-Key` sowie `Content-Type: application/json`
und im Body `name`, `query`, `active` (explizit `true` oder `false`),
`confirmed:true`, `mode`, `compareTitle`, `compareSubtitle`,
`compareSummary`. Native epgsearch speichert Suchtext und Regelnamen als
denselben Wert: `name` muss `query` gleichen. Fehlende `tolerance` ist
`0`, gültig `0..3` nur bei `mode=5`; die nativen Match-Modi `0..5`
einschließlich Regex-Modus 4 sind für CREATE erlaubt.

**Autorisierung:** Eine eingeloggte Browser-Sitzung benötigt die bestehende
CSRF- und SearchTimer-CREATE-Berechtigung; ein gepaartes Device benötigt den
bereits bestehenden unabhängigen `searchtimers.create@backend`-Grant.
Keine Device-Berechtigung darf unversionierte Legacy-API-Routen freischalten.
Beide Akteure gelangen nach Autorisierung in **denselben** Public-v1-
Controller, die gemeinsame Admission/Operation, den einmaligen
RESTfulAPI-Provider-Writer und die native Readback-Verifikation. MLD,
yaVDR und die späteren Samsung-/VIDAA-Clients benötigen keinen Backend-Fork.

### Unabhängige Such- und Wiederholungsfelder (MLD.F2)

Die bisherigen Pflichtfelder `compareTitle`, `compareSubtitle` und
`compareSummary` bleiben die **Suchfelder** (`use_title`, `use_subtitle`,
`use_description`). Drei optionale Boolean-Felder `repeatCompareTitle`,
`repeatCompareSubtitle`, `repeatCompareSummary` setzen unabhängig davon
`compare_title`, `compare_subtitle`, `compare_summary` beim Provider.
Fehlt ein neues Feld, bleibt die bisherige Zuordnung des entsprechenden
Suchfeldes erhalten. Explizites `false` wird nicht als fehlend behandelt.
Die Administrator-Readback-Einstellungen geben die vom Provider gemeldeten
Werte separat in `comparisonOptions` aus, sofern das jeweilige native Feld
vorliegt. Die CREATE-Operation speichert neue explizite Optionen dauerhaft
als internes Payload-Format 4; unveränderte alte Anfragen verwenden weiterhin
Format 3 mit identischem Fingerprint. Die Public API bleibt v1.
Die vollständige UPDATE-Maske folgt separat; MLD.F3 erweitert unten den
bestehenden öffentlichen UPDATE-Vertrag.

### Vollständige unterstützte epgsearch-Optionen

| Bereich | Zusätzliche JSON-Felder (CamelCase) |
|---|---|
| Suche | `matchCase`, `summaryMatch`, `compareCategories`, `compareTime` |
| Sender | `useChannel` (0–3), `channels`, `channelMin`, `channelMax` |
| Zeit | `useTime`, `startTime`, `stopTime` (HHMM), `useDuration`, `durationMinMinutes`, `durationMaxMinutes`, `useDayOfWeek`, `dayOfWeek` |
| Aufnahme | `directory`, `priority`, `lifetime`, `useVps`, `marginStartMinutes`, `marginStopMinutes`, `useSeriesRecording`, `keepRecordings`, `deleteMode`, `searchTimerAction` |
| Wiederholungen | `avoidRepeats`, `allowedRepeats`, `repeatsWithinDays` |
| Blacklist | `blacklistMode`, `blacklistIds` |
| EPG | `useExtendedEpgInfo`, `extendedEpgInfo`, `ignoreMissingEpgCategories`, `contentDescriptors` |
| Gültigkeit/Favoriten | `useInFavorites`, `activeFrom`, `activeUntil` |
| Aktion/Aufbewahrung | `pauseOnRecordings`, `switchMinutesBefore`, `unmuteSoundOnSwitch`, `deleteRecordingsAfterDays`, `deleteAfterCountRecordings`, `deleteAfterDaysOfFirstRecording` |

`blacklistIds` und `extendedEpgInfo` verwenden die vorhandene
RESTfulAPI-Listenabbildung; Stringwerte werden vor Vergleich kanonisiert.
Die ursprünglichen Core-Listenvarianten bleiben im Provider-Adapter
erhalten, sind aber nicht zusätzlich als eigene Public-v1-Arrayfelder
angeboten. Unbekannte JSON-Felder werden verworfen **durch einen Fehler**,
nicht durch stillschweigendes Ignorieren. Providerabhängig nicht
roundtripfähige Werte bleiben im Ergebnis `outcome_unknown`, nicht
`succeeded`.

Eine neue Regel kann direkt aktiv angelegt werden; `active:true` kann
unmittelbar Timer/Automationsaktionen auslösen. Der Browser verlangt
ausdrückliche Bestätigung. Für `marginStartMinutes` und
`marginStopMinutes` gilt: Bei fehlenden JSON-Feldern werden die live
ermittelten VDR-Setup-Werte der zugehörigen SuiteBridge verwendet;
ausdrücklich gesetzte Zahlen (auch `0`) sind wirksame Überschreibungen.
Die VDR-Setup-Auskunft hat bisher weiterhin einen vorhandenen
Backend-/Agent-Vertrag. Sie ist kein zusätzlich für diese Phase
eingeführtes Berechtigungsgate.

### Beispiel

```http
POST /api/v1/search-timers?backend=default
Idempotency-Key: search-web-0123456789abcdef0123456789abcdef
Content-Type: application/json

{
  "name": "Tatort",
  "query": "Tatort",
  "active": true,
  "confirmed": true,
  "mode": 1,
  "tolerance": 0,
  "compareTitle": true,
  "compareSubtitle": false,
  "compareSummary": false,
  "useChannel": 2,
  "channels": "Öffentlich",
  "useTime": true,
  "startTime": 2015,
  "stopTime": 2359,
  "priority": 50,
  "avoidRepeats": true,
  "allowedRepeats": 1
}
```

### Einmalige Mutation und erfolgreicher Readback

Der bestehende `SearchTimerCreateAdmissionService` schreibt den
vollständigen, unveränderlichen `SearchTimerCreateOperationPayload`
**Version 3** in dieselbe `MutationOperationRepository` wie andere
API-v1-Operations. Ein SHA-256-Fingerprint bindet jede Option an
Akteur, Backend, Idempotency-Key und Generation. Veränderte Requests
unter gleichem Key werden abgelehnt. Ein alter Version-2-Vorgang wird
nicht blind mit neu interpretierten Optionen neu versendet.

Der bestehende `SearchTimerCreateDispatchService` führt exakt einen
`POST /searchtimers` durch, eingerahmt von **einer vollständigen
nativen Vorher-Inventur und einer vollständigen Nachher-Inventur**.
Eine unklare oder fehlende ID in der POST-Antwort führt zu **keiner
weiteren Suchtext-Abfrage**. Nur eine eindeutige neue native ID, eine
eindeutig passende Backendgeneration und der vollständige Vergleich
aller wirksamen angeforderten Optionen können die Operation in
`succeeded` überführen; die native ID bleibt als interne
Result-Referenz nachvollziehbar. Eine fehlende, geänderte oder
mehrdeutige Bestätigung führt zu `outcome_unknown`. Keine automatische
Wiederholung eines CREATE.

Ein HTTP 202 enthält `operationId`, `backendId`, `state` und
`Location: /api/v1/operations/{operationId}`. Die Browser-Oberfläche
kann den Vorgang über die bereits vorhandene Operation-SSE-Route
beobachten und ihn gezielt nachlesen; sie sendet bei Unklarheit
keinen zweiten POST. Ein `succeeded`-Zustand besagt nur, dass epgsearch
die angeforderte Regel bestätigt hat, nicht wie viele Aufnahmen
später tatsächlich erstellt werden.

**Nicht betroffen:** Die separat generation-/revisionsgeschützten
Public-v1-UPDATE-/ACTIVATE-/DEACTIVATE-/DELETE-Mutationen nutzen
den regulären epgsearch-Verwaltungspfad über Public-v1.
Der frühere `POST /api/vdr/searchtimers`-Browser-CREATE ist durch den
Runtime-Mutationspolicy-Executor produktiv gesperrt.

**Test- und Abnahmetrennung:** Die gezielten C++-Targets sind
`test-searchtimer-create-durable-admission`,
`test-searchtimer-create-one-shot-dispatch`,
`test-searchtimer-create-orphan-recovery`,
`test-restful-api-search-timer-command-executor`.
Browser- und Referenzclient-Tests sowie
`test-public-timer-assignment-collection` (Public-v1 HTTP Vertrag)
sind zusätzlich auszuführen. Ein PASS dieser Tests ersetzt
**keine** MLD-6.6-Paketinstallation oder einen echten yaVDR-/epgsearch-Readback.

### Device mutation permission must never unlock the legacy path

The existing `SecurityHttpGate::authenticate` has an **earlier,
independent boundary**: presenting a Device credential to *any* unversioned
`/api/...` path is invalid authentication (HTTP 401). This already protects
legacy SearchTimer mutation actions, including `POST /api/searchtimers`,
`/api/vdr/searchtimers`, update, delete and execute, even if a matching
permission was inserted directly into grant persistence. A second,
later route-specific Device block is unnecessary because Device authentication
has already failed. The CI regression verifies the existing fail-closed
boundary, not a new HTTP 403 policy. Auf dem Branch `MLD-Suchtimer` ist der Browser-Legacy-CREATE
`POST /api/vdr/searchtimers` ebenfalls geschlossen; die Browser-Oberfläche
verwendet den gemeinsamen Public-v1-CREATE. Andere historische Legacy-Routen
sind nicht durch einen Device-Grant nutzbar.
`DeviceGrantAdministrationService::supportedGrant` now accepts
`searchtimers.create` for a **concrete** backend, never `*`. The new
Public-v1 POST independently checks Device identity and exact scope; legacy
Device requests continue to fail authentication. Regression:
`core/security/tests/test_device_credential_authentication.cpp`.

Additional identity invariant: the full post-CREATE native inventory must
contain **exactly one row** with the returned backend-native identifier.
Even one matching row plus a second conflicting row with the same ID yields
`outcome_unknown` and never triggers a second POST. Duplicate native IDs in
the preflight inventory stop dispatch before any provider write.

### Dormant CREATE crash/orphan recovery (repository-only)

`SearchTimerCreateRecoveryService::markOrphanedUnknown` is a separate,
repository-only recovery primitive for a future supervising runtime. It does
**not** call the provider, read EPG, dispatch work, mint an operation, register
a daemon callback or expose an API. An explicit assertion that the prior
dispatcher has stopped, exact operation/actor/backend/generation identity, an
elapsed stale interval of 60..86400 seconds and a revision-fenced durable
operation transition are all required.

Only an already-`dispatching` or `executed_unverified` operation can become
`outcome_unknown`; an `accepted` CREATE stays accepted and can only be
dispatched by the separately guarded first-attempt authority. Already-unknown
operations are read without modification. Durable `resultReference` evidence
is preserved verbatim (never replaced with a guessed provider identity).
The service does **not** assert that a SearchTimer was created or absent.
An orphan remains a reconciliation problem, **never an automatic resubmit**.

A supervisor must independently prove the previous dispatcher stopped before
using this service; elapsed wall-clock time alone is insufficient to authorize
recovery. No production caller is wired yet. The self-contained mock/SQLite
and file-backed restart regression is:
`make test-searchtimer-create-orphan-recovery`. Neither the daemon's
installed binary nor native VDR SearchTimers change as a result of this slice.

### CI execution gate for dormant CREATE services

The top-level GitHub fast regression uses `make test-ci-fast` (not
`make test-phase`). Accordingly, **all three** SearchTimer CREATE internal
contract regressions are explicitly attached to `CI_FAST_TESTS`:
`test-searchtimer-create-durable-admission`,
`test-searchtimer-create-one-shot-dispatch`, and
`test-searchtimer-create-orphan-recovery`. A green workflow now proves that
the mock-only admission, one-shot and orphan recovery tests actually ran;
it does not attest to a live VDR/VIDAA mutation or device hardware.

### Earlier Device-admission test fixture

The earlier `SearchTimerCreateDeviceAdmissionService` remains an isolated
domain test adapter, not a second production authority. Production authorization
is performed by `SecurityHttpGate` with persistent Device grants and audit;
daemon admission uses the shared durable service. Its standalone
`test-searchtimer-create-device-admission` protects the same identity,
write-policy and generation invariants against regression.

## Historischer Hinweis: früherer inactive-only Device-CREATE

Vor ST.V1 erlaubte Public API v1 Device-CREATE ausschließlich mit
`active:false` und einer stark verkleinerten Feldmenge. Der damalige
Browser-Editor nutzte vorübergehend einen separaten Legacy-CREATE.
**Beides ist keine Beschreibung des aktuellen Entwicklungsstands**
auf `MLD-Suchtimer`: Der oben dokumentierte gemeinsame Public-v1-CREATE
nimmt explizit `active:true` **oder** `active:false` an und verarbeitet
alle unterstützten epgsearch-Optionen. Die bestehende Geräteautorisierung
bleibt unverändert. Der alte eingeschränkte Vertrag gilt bis zu einem
entsprechenden Server-Update weiterhin für die bereits ausgelieferten
`main`-/Installationsstände.

Ein bestätigtes HTTP 202 bedeutet lediglich dauerhafte Admission.
Bei ungewissem Dispatch bleibt `outcome_unknown` und ein erneuter
CREATE unter einem neuen Idempotency-Key ist verboten. Das gilt
gleichermaßen für Browser, VIDAA und später Samsung.

### Backend SearchTimer name versus search string

The existing RESTfulAPI `/searchtimers` CREATE body carries the epgsearch
`search` field, but **no separately round-trippable display-name field**.
`RestfulApiSearchTimerMapper` reconstructs `name = query` from readback.
Public-v1 therefore requires `name == query` at admission (otherwise HTTP
422), so an exact native result can be verified. VIDAA presents one combined
search/name field. It does not claim independent rule labels supported by the
native epgsearch backend or invent a second mapping authority.

## One final real-VDR API-v1 acceptance

Do not treat independent GitHub mocks, existing hardware checks of older
commits, or a drafted native provider as a complete API-v1 release. Finish
runtime authorization, stock epgsearch revision checks and independent
readback first. The **single** final, operator-approved real-host acceptance
session for all Public-v1 domains is specified in
[`docs/development/public-v1-final-hardware-acceptance.md`](../docs/development/public-v1-final-hardware-acceptance.md).
No separate recurring SearchTimer hardware test slices.

## Public-v1 guarded native SearchTimer mutations (integration candidate)

Browser administrators use the existing backend-scoped administrator session
and CSRF checks. Device authorization still requires the independently granted
backend permissions: searchtimers.view for reads, searchtimers.modify for
update/deactivate, searchtimers.execute for activate, searchtimers.delete for
delete. No grant is added or implied.

GET /api/v1/search-timers/{searchTimerId}/version?backend=<id> resolves the
public identity through a fresh RESTfulAPI inventory and obtains the complete
stock epgsearch rule using LSTS. The opaque sr_ revision hashes the public
identity and raw rule; the strong quoted ETag is returned with no-store.
The response also carries query and state from that same observation. The
Webfrontend checks its displayed card against these values and uses the
revision directly, without a separate DETAIL request before a mutation.
No numeric native ID or raw record is exposed.

POST /api/v1/search-timers/{searchTimerId}/mutations?backend=<id> uses the
quoted If-Match, Idempotency-Key and explicit confirmed:true with action
update, activate, deactivate or delete. UPDATE accepts optional query and/or
the settings in the OpenAPI mutation request; active and inactive rules are
editable. EDIS starts from the original native record and replaces only the
supplied fields (plus dependent native representations such as disabled
time-filter endpoints). MODS ON/OFF and DELS retain their previous behavior.
The reference client retains its activation preview. Omission preserves values;
false and zero are explicit. For duration fields the existing RESTfulAPI
HHMM integer representation is retained despite the historical Minutes suffix.
Extended EPG data uses CSV provider entries of the form id#value. Native
category mask is a decimal string; exact comparison/activation enums can
be read from VERSION and supplied as repeatCompareSubtitleMode,
repeatCompareTimeMode, repeatCompareCategoriesMask, activeMode. UPDATE may change activeMode within the existing active/inactive state; state
changes remain separate actions with their existing authorization. Boolean
compatibility fields retain existing enabled native modes where possible.
Explicit exact modes take precedence. No public raw record or ID is exposed.
This source change still requires real provider normalization/roundtrip tests.

The daemon serializes its own management requests, validates the observed
revision and backend, reserves and claims one durable Operation and sends at
most one stock command. Success requires acknowledgement and independent
exact native readback (or explicit absence after DELETE). Ambiguous outcomes
remain outcome_unknown and are never automatically retried. Existing native-CAS
idempotency records remain replayable without redispatch. HTTP 202 exposes
the Operation, whose status is available through existing GET/SSE.

The revision check is optimistic concurrency, not epgsearch-lock-atomic CAS.
External concurrent writers or indistinguishable deletion/recreation between
observations remain possible; neither native persisted generations nor fsync
durability is claimed. No patched plugin is required. The installed candidate
`b5ae3cc` passed CI and the documented real yaVDR Browser management tests;
separate Device/MLD and remaining evidence are not verified.

## Browseradministrator: frühere Implementierungen (historisch)

Am 26.10.2026 verwendete das Webfrontend einen inactive-only
Public-v1-CREATE. Ein späterer Arbeitsstand verwendete einen
separaten Browser-Legacy-CREATE für vollständige Optionen.
**Beide Varianten sind überholt.** Im Kandidaten `MLD-Suchtimer`
verwendet auch der vollständige Browser-Editor ausschließlich
`POST /api/v1/search-timers`, einschließlich aktiver Regeln, und
die gleiche dauerhafte Operation samt Readback wie die Device-Clients.
Dies ist noch **nicht** als produktive Installation oder Real-VDR-PASS
bestätigt. Siehe die einzige [ST.V1-Feldmatrix und Abnahmeroadmap](../docs/planning/mld-searchtimer-public-v1-roadmap.md).
