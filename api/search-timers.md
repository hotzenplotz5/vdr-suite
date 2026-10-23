# Public-v1 VDR SearchTimers: read, preview, inactive CREATE and native CAS management

The current implemented version is **Public API v1**, not v2.

`GET /api/v1/search-timers?backend=<backendId>&limit=30[&cursor=<opaque>]`

- This is **VDR SearchTimer**, distinct from native VDR Timer and Suite TimerAssignment.
- Requires an authenticated, verified **Device credential** and an **independently granted** `searchtimers.view` permission for exactly one backend. `timers.view`, `epg.view`, and `channels.view` do not grant access. Browser sessions are not a substitute.
- Administrators may independently grant/revoke `searchtimers.view` and backend-scoped `searchtimers.create` through Device grant administration. Read permission never implies create permission.
- `backend` is mandatory, length 1–128, characters A-Z, a-z, 0-9, dot, underscore, hyphen. Unknown/duplicate query fields and invalid/mismatched cursors yield 400.
- Pages are limited to 1–30 items (default 30), ordered by the **server-owned stable opaque** `searchTimerId`. `st1_` cursors bind the backend. Only a bounded page is returned to the TV. The provider itself currently exposes `/searchtimers.json` as a catalogue: each page request performs one read on only that backend; **no backend fanout or retries**.
- `items` contain only `searchTimerId`, `backendId`, `name`, `query` and `state` (`active`, `inactive`, or `unknown`). No native IDs, provider payload, recording directory, or settings used for editing are exposed.
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

## CREATE: durable admission and one-shot dispatch (wired to Public-v1)

`SearchTimerCreateAdmissionService` persists one immutable, versioned
`SearchTimerCreateOperationPayload` in the shared
`MutationOperationRepository.reserveWithPayload`. Idempotency binds actor,
backend, `SearchTimer/new`, `searchtimer.create`, and the request key;
a SHA-256 payload fingerprint enforces byte-stable replays. A changed payload
with the same key fails as a conflict.

**Safety change (2026-10-22):** the Public-v1 JSON body must include
`"active":false` explicitly. Missing `active` or `"active":true`
returns **HTTP 422 before durable admission/provider write**. The daemon
callback independently rejects active requests. The durable CREATE dispatcher
also refuses **previously accepted but still queued active CREATE operations**
before contacting the provider, to prevent pre-upgrade jobs from bypassing
the rule; these historical jobs move atomically to the terminal
`failed_before_dispatch` state (with a policy reason) and remain auditable
instead of being silently rewritten or retried. This makes
Public-v1 CREATE an inactive **staging** operation: epgsearch must not
schedule recordings from the rule until a separately authorized, generation-
fenced activation is explicitly requested through the native CAS mutation endpoint. Existing legacy/browser integrations remain
separate and unchanged. See `docs/development/public-v1-searchtimer-native-generation-fence.md` for
why MODIFY, ACTIVATE, DEACTIVATE and DELETE cannot safely use native numeric IDs
alone, and the required provider generation/CAS contract.

A Device/TV client should present the read-only
`draft-preview` first, prominently show `totalCount`, and require a human
confirmation. That count is **EPG text matches**, not the final number of
recordings. The JavaScript reference client enforces explicit preview
acknowledgement before sending the inactive CREATE, but the backend does
not claim that a GET preview was executed beforehand.

After exact Device HTTP authorization and backend write/online/Agent-generation
checks, `DaemonRuntime` invokes this existing admission service and, only
for an eligible `accepted` operation, the existing
`SearchTimerCreateDispatchService.dispatchOnce`. The dispatcher acquires
an atomic `accepted -> dispatching` CAS **before** a single RESTfulAPI
CREATE attempt. It requires a complete preflight backend rule inventory and
authoritatively verifies the returned new native ID against a fresh complete
post-write inventory, including name, query, active state, match mode,
tolerance and comparison flags. Ambiguous provider results become
`outcome_unknown`, **never an automatic second POST**. The operation read
endpoint is `GET /api/v1/operations/{operationId}`.

This path uses the existing backend-specific RESTfulAPI adapter; it does not
enable Device access to legacy APIs, unrestricted SearchTimer settings
editing, arbitrary native VDR Timer creation, or exact future recording
prediction. Native UPDATE/TOGGLE/DELETE use only the separately authorized
version-fenced SuiteBridge/epgsearch CAS route described below. A confirmed
CREATE requests an epgsearch rule; any automated recording effects depend on
the backend epgsearch configuration and are **not** proven by HTTP 202.

The CI-fast targets `test-searchtimer-create-durable-admission`,
`test-searchtimer-create-one-shot-dispatch`, and
`test-searchtimer-create-orphan-recovery` use isolated SQLite/mock providers.
The compiled daemon wiring and HTTP tests are also checked in GitHub CI.
Real yaVDR backend creation, readback behavior and VIDAA hardware acceptance
remain separate gates.

### Device mutation permission must never unlock the legacy path

The existing `SecurityHttpGate::authenticate` has an **earlier,
independent boundary**: presenting a Device credential to *any* unversioned
`/api/...` path is invalid authentication (HTTP 401). This already protects
legacy SearchTimer mutation actions, including `POST /api/searchtimers`,
`/api/vdr/searchtimers`, update, delete and execute, even if a matching
permission was inserted directly into grant persistence. A second,
later route-specific Device block is unnecessary because Device authentication
has already failed. The CI regression verifies the existing fail-closed
boundary, not a new HTTP 403 policy. Browser legacy behavior is unchanged.
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

## Public-v1 SearchTimer CREATE (Device)

`POST /api/v1/search-timers?backend=<id>` accepts JSON
`{"name":"Terra X","query":"Terra X","active":false,"confirmed":true,"mode":1,"compareTitle":true,"compareSubtitle":false,"compareSummary":false}`
with mandatory `Idempotency-Key` and application/json. `active:false`
MUST be explicit; omitted `active` or `active:true` returns 422 before
dispatch. `tolerance` (default zero) is optional; unknown/duplicate
fields are rejected. Exact backend-scoped
`searchtimers.create` Device grant, verified credential,
server-owned online writable backend and fresh Backend Agent generation
are mandatory. The existing immutable SQLite admission and one-shot
native-ID readback dispatcher are used, without legacy mutation fallback.
HTTP 202 references an Operation; it is **not** itself proof of success.
Timeouts and unverifiable effects are never automatically resent:
inspect Operation state, including `outcome_unknown`.

The daemon performs the first attempt inline after reservation and never
transitions a dispatched operation back to accepted. An `outcome_unknown` operation has **no automatic reconciliation worker**
connected yet. An operator must inspect authoritative backend evidence before
resolving it; the client must not make a new POST with a different idempotency
key. A crash during dispatch likewise does not establish success or authorize
retry.

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
