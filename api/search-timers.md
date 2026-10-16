# Public-v1 SearchTimer collection (Device read-only)

The current implemented version is **Public API v1**, not v2.

`GET /api/v1/search-timers?backend=<backendId>&limit=30[&cursor=<opaque>]`

- This is **VDR SearchTimer**, distinct from native VDR Timer and Suite TimerAssignment.
- Requires an authenticated, verified **Device credential** and an **independently granted** `searchtimers.view` permission for exactly one backend. `timers.view`, `epg.view`, and `channels.view` do not grant access. Browser sessions are not a substitute.
- Administrators may grant/revoke `searchtimers.view` through the real Device Grant administration contract. No SearchTimer mutation grant or mutation route is offered.
- `backend` is mandatory, length 1–128, characters A-Z, a-z, 0-9, dot, underscore, hyphen. Unknown/duplicate query fields and invalid/mismatched cursors yield 400.
- Pages are limited to 1–30 items (default 30), ordered by the **server-owned stable opaque** `searchTimerId`. `st1_` cursors bind the backend. Only a bounded page is returned to the TV. The provider itself currently exposes `/searchtimers.json` as a catalogue: each page request performs one read on only that backend; **no backend fanout or retries**.
- `items` contain only `searchTimerId`, `backendId`, `name`, `query` and `state` (`active`, `inactive`, or `unknown`). No native IDs, provider payload, recording directory, or settings used for editing are exposed.
- IDs are persisted under the new `public_search_timer_identity` table; the native mapping is internal and does not make these TimerAssignments.
- A genuinely empty provider list is HTTP 200 with `items: []`. An unavailable provider, unavailable runtime, or identity failure is HTTP 503, not an empty successful response. Missing/invalid Device credentials yield 401 or 403, unauthorized backend 403; all use the Public-v1 Problem model.
- No lossless readback is guaranteed for full epgsearch option sets. **No edit, activation/deactivation, CREATE, DELETE or preview is enabled by this contract.**
- Public response is `no-store`. On permission revocation a client must clear stale SearchTimer cards, not keep displaying a successful prior list.

See `api/openapi.json`, `docs/planning/timer-searchtimer-parity.md` (VIDAA repository), and `docs/client-integration-playbook.md` (VIDAA repository).

This is an API/CI slice. Real Hisense hardware and production yaVDR deployment are separate gates.

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
- This previews **existing** rules only, not a new-rule form. Neither
  SearchTimer CREATE/DELETE, full blacklist/duplicate evaluation, lossless edit,
  nor native VDR Timer mutation is claimed as available on Public-v1.

## Draft preview of a new rule (Device, read-only)

`GET /api/v1/search-timers/draft-preview?backend=<id>&query=<encoded>&mode=<0|1|2|3|5>&compareTitle=<0|1>&compareSubtitle=<0|1>&compareSummary=<0|1>[&tolerance=<0..3>][&limit=<1..30>]`

- Verified paired Device with both `searchtimers.view@backend` and `epg.view@backend`. No Browser-session fallback, no mutation, no native ID or backend credential.
- `backend` must be explicit and exactly scoped; `query` is URL encoded, 2–240 decoded bytes; complete query string ≤900 bytes. Every comparison flag and the mode must be explicit. Allowed modes: phrase 0, all words 1, any word 2, exact 3, fuzzy 5. Regex and unmodeled controls rejected. `tolerance` is valid only for fuzzy matching.
- Uses the **same** `SearchTimerPreviewService` and ready backend EPG cache as the existing rule preview and Web. Only its implemented EPG text comparison/match semantics are covered; channel/duration filters, blacklists, repeat avoidance, recording settings and full epgsearch planning are **not evaluated** and are therefore rejected if supplied. Clients must label the result **EPG match-only, not an exact recording forecast**.
- Response: `{backendId,totalCount,items:[{title,subtitle,channelId,startTime,endTime}],meta:{partial:false,epgInput:"ready",evaluation:"epg-match-only"}}`. `limit` default/max 30. Empty valid match is 200/`[]`; cache/backend unavailable 503. 400 for unsupported options, duplicate/oversized/mismatched query; 401/403 for authentication/grants. `Cache-Control: no-store`.
- **Does not authorize CREATE**. A real SearchTimer CREATE requires separate permission, backend write policy, idempotency, and authoritative post-creation readback. Do not reinterpret this GET as permission to save a rule.

## Durable SearchTimer CREATE admission – internal groundwork (not exposed)

The domain now has a **persistence-only** `SearchTimerCreateAdmissionService`,
using the shared `MutationOperationRepository.reserveWithPayload` and
versioned `SearchTimerCreateOperationPayload`. It does **not** expose
`POST /api/v1/search-timers`, register a runtime dispatcher, contact RESTfulAPI,
create a VDR SearchTimer, or enable client Save.

- Operation family `searchtimer.create`, resource `SearchTimer/new`, mandatory
  `readbackRequired`, backend-generation fence and immutable payload.
- The durable idempotency scope is actor + backend + resource type/id + action
  family + idempotency key. A strict SHA-256 fingerprint identifies the exact
  payload. Exact retries (including after process restart) return the original
  operation; changed payload with the same key produces a conflict.
- The payload codec is versioned, length-prefixed and canonical, with bounded
  UTF-8 for rule name/query, explicit modes 0/1/2/3/5, fuzzy tolerance and
  title/subtitle/description match fields. Unsupported fields fail closed.
- **Before exposing POST:** add verified Device-only `searchtimers.create`
  grant and BackendAccessPolicy write checks, durable dispatch arbitration,
  no automatic resend on ambiguous backend outcomes, full backend readback
  using the native ID and all contract fields, reconciliation, and operation
  state transitions. A prepared operation by itself is **not** successful
  backend CREATE.
- Focused regression:
  `make test-searchtimer-create-durable-admission` (in-memory database).

### One-shot dispatch domain groundwork (not a public endpoint)

`SearchTimerCreateDispatchService` now owns a **dormant, separately injected**
one-shot adapter. Its preflight requires exact actor/backend/generation,
explicit operator confirmation, backend write approval, executor opt-in, a
verifiable stored versioned payload and **fresh complete backend readback**.
An atomic `accepted -> dispatching` compare-and-swap is mandatory before a
single `ISearchTimerCommandExecutor.create` invocation; a CAS replay is
never an excuse for a second backend call. The generated operation never
re-enters `accepted` after dispatch. A returned native ID must not already
exist in the baseline; after execution a fresh unpaged provider inventory
must uniquely verify backend ID, query, name, active state, match mode,
tolerance and comparison flags before `succeeded`. Missing/ambiguous
reply, exception, stale ID or mismatched/unavailable readback leaves the
operation `outcome_unknown`; **no automatic retry**.

This mock-proven service is not yet registered with the daemon and no
Public-v1 CREATE endpoint or Device mutation grant has been opened.
`make test-searchtimer-create-one-shot-dispatch` uses an in-memory
SQLite database and mock provider only. Productive activation still needs
an authoritative backend-generation source, the Device permission/grant
boundary and an operation reconciliation/identity-projection path.

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
`DeviceGrantAdministrationService.supportedGrant` still rejects
`searchtimers.create`; before enabling it in the future, the new
Public-v1 CREATE permission, backend scope, capability and no-legacy-fallback
contracts require explicit tests. Regression:
`core/security/tests/test_device_credential_authentication.cpp`.

Additional identity invariant: the full post-CREATE native inventory must
contain **exactly one row** with the returned backend-native identifier.
Even one matching row plus a second conflicting row with the same ID yields
`outcome_unknown` and never triggers a second POST. Duplicate native IDs in
the preflight inventory stop dispatch before any provider write.

### Dormant crash/orphan recovery (no public mutation)

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

### Dormant Device-only CREATE admission composition

`SearchTimerCreateDeviceAdmissionService` composes the existing
`AuthorizationService`, `BackendAccessPolicy`, `BackendRegistryService`
and durable `SearchTimerCreateAdmissionService`; it has **no HTTP handler,
daemon registration or backend CREATE effect**. Its request contains no
caller-provided backend generation: a separately injected *server-owned*
`ISearchTimerBackendGenerationAuthority` must provide the current
generation before one immutable operation can be reserved.

Required prerequisites are a verified active Service/Device actor and active
Device credential, **no Browser session**, fully resolved grants, an exact
`searchtimers.create@backend` tuple (no wildcard, role inheritance,
`searchtimers.view`, `epg.view` or `timers.create` substitution),
the normal `AuthorizationService` mutation check, an enabled, online,
writable known backend and a positive bounded backend generation. Failure
creates **no operation**. Replay with the same idempotency key may only
return the prior durable operation; it never mutates VDR or makes a new POST.

The ordinary Device admin supported-grant allowlist still **rejects**
`searchtimers.create` and the Public-v1 API still has no POST route.
The isolated regression injects an explicitly scoped future grant into
a mock authenticated Device context; this does *not* simulate an
administratively assignable permission or live authentication boundary.
An actual endpoint remains blocked until the normal Device grant/admin
contract, HTTP identity and accountability audit, backend-generation
authority adapter, dispatch/reconciliation wiring and API tests exist.

Focused CI-fast regression: `make test-searchtimer-create-device-admission`.

## Public-v1 SearchTimer CREATE (Device)

`POST /api/v1/search-timers?backend=<id>` accepts JSON
`{"name":"Terra X","query":"Terra X","confirmed":true,"mode":1,"compareTitle":true,"compareSubtitle":false,"compareSummary":false}`
with mandatory `Idempotency-Key` and application/json. Only
`active` (default true) and `tolerance` (default zero)
are optional; unknown/duplicate fields are rejected. Exact backend-scoped
`searchtimers.create` Device grant, verified credential,
server-owned online writable backend and fresh Backend Agent generation
are mandatory. The existing immutable SQLite admission and one-shot
native-ID readback dispatcher are used, without legacy mutation fallback.
HTTP 202 references an Operation; it is **not** itself proof of success.
Timeouts and unverifiable effects are never automatically resent:
inspect Operation state, including `outcome_unknown`.

The daemon performs the first attempt inline after reservation and never
transitions a dispatched operation back to accepted. Further asynchronous
readback/reconciliation is required for operations whose effect is unknown;
the client must not make a new POST with a different idempotency key to
resolve those cases.
