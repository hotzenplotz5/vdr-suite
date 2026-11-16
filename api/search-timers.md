# Public-v1 VDR SearchTimers: read, preview, complete active/inactive CREATE and native CAS management

**Abnahmestand 24.10.2026:** Der bereits erfolgreich auf yaVDR durchgeführte [direkte RESTfulAPI-SearchTimer-CREATE/UPDATE/DELETE-Smoke-Test](../docs/development/searchtimer-yavdr-smoke-test-execution-report.md) bleibt **PASS – PROVIDER** und ist nicht erneut auszuführen. Die nachträglich implementierte Public-v1-Device-/HTTP-/native-epgsearch-CAS-Kette für UPDATE/ACTIVATE/DEACTIVATE/DELETE ist ein **anderer, noch offener REAL-VDR-Ende-zu-Ende-Pfad**. CREATE bleibt ein einmaliger RESTfulAPI-Write ohne native epgsearch-lock-atomare CREATE-CAS-Garantie; diese Garantie muss vor der Releasefreigabe entweder implementiert oder ausdrücklich aus dem Scope ausgeschlossen werden. Die [einzige zentrale Abnahmematrix](../docs/development/public-v1-final-hardware-acceptance.md) und der [Dokumentations-Closeout](../docs/development/public-api-v1-documentation-closeout-20261024.md) sind für den Test-/Release-Status maßgeblich; historische Implementierungsabschnitte darunter sind keine neuen Testaufträge.

The current implemented version is **Public API v1**, not v2.

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
- Requires an authenticated, verified **Device credential** and an **independently granted** `searchtimers.view` permission for exactly one backend. `timers.view`, `epg.view`, and `channels.view` do not grant access. Browser sessions are not a substitute.
- Administrators may independently grant/revoke `searchtimers.view` and backend-scoped `searchtimers.create` through Device grant administration. Read permission never implies create permission.
- `backend` is mandatory, length 1–128, characters A-Z, a-z, 0-9, dot, underscore, hyphen. Unknown/duplicate query fields and invalid/mismatched cursors yield 400.
- Pages are limited to 1–30 items (default 30), ordered by the **server-owned stable opaque** `searchTimerId`. `st1_` cursors bind the backend. Only a bounded page is returned to the TV. The provider itself currently exposes `/searchtimers.json` as a catalogue: each page request performs one read on only that backend; **no backend fanout or retries**.
- `items` contain only `searchTimerId`, `backendId`, `name`, `query` and `state` (`active`, `inactive`, or `unknown`). No native IDs, provider payload, recording directory, or settings used for editing are exposed.
**Saved rule detail (read-only):**

`GET /api/v1/search-timers/{searchTimerId}?backend=<backendId>`

- Uses the **same** complete authoritative epgsearch catalogue and opaque
  Public-v1 identity reconciliation as the list. Requires a verified paired
  Device with independent `searchtimers.view@backend`; an EPG grant is **not**
  required just to read name, query and active state. No Browser fallback.
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
- **Identity boundary:** the public catalogue's signature-based ID reconciliation alone cannot detect an identically configured native rule deleted and recreated between reads. Public-v1 mutations therefore require the **patched native epgsearch per-rule generation and revision CAS** under the plugin's lock; an ordinary/older provider is rejected. The catalogue heuristic must never authorize a write.
- A genuinely empty provider list is HTTP 200 with `items: []`. An unavailable provider, unavailable runtime, or identity failure is HTTP 503, not an empty successful response. Missing/invalid Device credentials yield 401 or 403, unauthorized backend 403; all use the Public-v1 Problem model.
- The collection GET deliberately does not expose the complete epgsearch option set. Guarded native query-only UPDATE of an inactive rule preserves the full existing settings in the provider; separate version and mutation endpoints are documented below. Unrestricted settings editing is not exposed.
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

- Verified paired Device with both `searchtimers.view@backend` and `epg.view@backend`. No Browser-session fallback, no mutation, no native ID or backend credential.
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
weiterhin ihren eigenen bestehenden native-CAS-Teilpfad.
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
runtime authorization, versioned native CAS transport and exact-generation
readback first. The **single** final, operator-approved real-host acceptance
session for all Public-v1 domains is specified in
[`docs/development/public-v1-final-hardware-acceptance.md`](../docs/development/public-v1-final-hardware-acceptance.md).
No separate recurring SearchTimer hardware test slices.

## Public-v1 guarded native SearchTimer mutations (integration candidate)

**Release status:** implemented in PR #514; exact-head native build and complete
Ubuntu daemon/SuiteBridge package CI plus the one final real-VDR API-v1
acceptance are required before release. An older build's PASS is not proof of
this integration. No legacy RESTfulAPI or SVDRP mutation fallback is permitted.

**Read opaque revision**

`GET /api/v1/search-timers/{searchTimerId}/version?backend=<id>` requires a
verified paired Device and exact `searchtimers.view@backend`. The server
resolves only an active, non-retired public ID and contacts the local native
epgsearch `observe-v1.1` service. Success returns `searchTimerId`,
`backendId`, `revision` and a strong quoted `ETag` (format `sr_...`).
Neither the native numeric ID, its generation nor the 59-field rule settings
are returned to the client. Native metadata not durably persisted, wrong
backend, or unavailable plugin yields fail-closed 503; a missing public binding
yields 404. The result is `Cache-Control: no-store`.

**One versioned, action-scoped mutation**

`POST /api/v1/search-timers/{searchTimerId}/mutations?backend=<id>` accepts
`application/json`, a mandatory `Idempotency-Key`, and an `If-Match` equal
to the quoted ETag returned by `GET /version`. Body must be
`{\"action\":\"update|activate|deactivate|delete\",\"confirmed\":true}`;
`update` additionally requires a `query` string of 1–200 bytes. Unsupported
fields, duplicate keys, missing confirmation and malformed JSON are rejected.
The query-only update requires a currently inactive native rule, preserves all
other native settings and refuses colon/control bytes instead of guessing the
legacy epgsearch field encoding.

Required *independently assigned* Device grants for the exact backend are
`searchtimers.modify` (UPDATE/DEACTIVATE), `searchtimers.execute`
(ACTIVATE), and `searchtimers.delete` (DELETE). The credential and Device
identity are verified at the HTTP security gate; neither `searchtimers.view`
nor `searchtimers.create` implies these mutation grants. The reference JS
client requires explicit fresh EPG-preview acknowledgement before it sends
ACTIVATE. A preview is a list of EPG matches, **not** a timer/recording forecast.

The daemon rechecks writable backend scope and live Agent generation; resolves
the public ID against the current SQLite identity binding; obtains an atomic
native `sg_` generation and `sr_` revision; and compares the observed revision
with `If-Match`. No client-supplied native ID/generation is accepted. A
canonical immutable operation+payload is durably reserved and claimed before
one attempt. The private Unix control-plane transport has no SVDRP writer.
SuiteBridge calls the native epgsearch CAS service and separately observes the
result. Only an exact-generation/revision readback or same-generation absence
can mark the durable operation `succeeded`; ambiguous or failed persistence
must remain `outcome_unknown` with **no automatic retry**. A replay of the
same idempotency key returns the **original** operation without attempting
another native write even if the native revision has since changed.

The HTTP response is 202 with a `Location` referencing an Operation, not a
statement that a scheduled recording exists. Non-mutating failures produce
400/401/403/404/409/415/422/503 as documented by the OpenAPI contract.

**Important implementation scope:** This native operation path requires the
specific patched epgsearch build and matching SuiteBridge ABI; an ordinary
unpatched epgsearch plugin correctly reports capability unavailable. Native
plugin packaging, CI and the [single final API-v1 real-VDR acceptance
checklist](../docs/development/public-v1-final-hardware-acceptance.md) are
release gates. No production installation, VDR restart or destructive
hardware exercise has been performed by this PR.

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
